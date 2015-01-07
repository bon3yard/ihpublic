/*++

Copyright (c) 2011, Pankaj Garg (pankajgarg@intellectualheaven.com).
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

--*/

/*++

Module Name:

    processutil.cpp

Module Description:

    Implements various helper routines for process management.
    These routines are dependent upon psapi.lib which is only
    availalbe on Windows NT, 2000, XP and 2003, so these
    routines will not work on Windows 9x series and ME.

--*/

#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include "ihulib.h"

#define M_MAX_PROCESSES     512
#define M_MAX_MODULES       512

//
// PSAPI Functions
//
typedef DWORD (WINAPI *PFN_GET_MODULE_BASE_NAME)(HANDLE, HMODULE, LPCWSTR, DWORD);
typedef DWORD (WINAPI *PFN_GET_MODULE_FILE_NAME_EX)(HANDLE, HMODULE, LPCWSTR, DWORD);
typedef BOOL (WINAPI *PFN_ENUM_PROCESS_MODULES)(HANDLE, HMODULE*, DWORD, LPDWORD);
typedef BOOL (WINAPI *PFN_ENUM_PROCESSES)(DWORD*, DWORD, DWORD*);

//
// Toolhelp functions
//
typedef HANDLE  (WINAPI *PFN_CREATE_TOOLHELP32_SNAPSHOT)(DWORD, DWORD);
typedef BOOL    (WINAPI *PFN_PROCESS32_FIRST)(HANDLE, LPPROCESSENTRY32W);
typedef BOOL    (WINAPI *PFN_PROCESS32_NEXT)(HANDLE, LPPROCESSENTRY32W);


DWORD
__cdecl
IhuGetProcessList(
    IHU_PROCESS_LIST &oProcessList)
{
    DWORD   funcResult  = 0;
    DWORD           errorCode   = 0;

    oProcessList.clear();

    HMODULE hKernel32   = GetModuleHandleA("kernel32");

    if (!hKernel32)
    {
        //
        // To-Do!!!
        // Handle error
        //
        goto funcEnd;
    }

    //
    // We don't use ToolHelp for UNICODE because ToolHelp is only
    // used by us for WIndows 9x and for Windows 9x we only build
    // ANSI tools
    //
    HMODULE hPsapi = LoadLibraryA("psapi");
    if (hPsapi)
    {
        PFN_ENUM_PROCESSES pfnEnumProcesses;
        PFN_ENUM_PROCESS_MODULES pfnEnumProcessModules;
        PFN_GET_MODULE_BASE_NAME pfnGetModuleBaseNameW;
        PFN_GET_MODULE_FILE_NAME_EX pfnGetModuleFileNameExW;

        pfnEnumProcesses = 
            (PFN_ENUM_PROCESSES)GetProcAddress(hPsapi,
                                               "EnumProcesses");

        pfnEnumProcessModules = 
            (PFN_ENUM_PROCESS_MODULES)GetProcAddress(hPsapi,
                                                     "EnumProcessModules");

        pfnGetModuleBaseNameW = 
            (PFN_GET_MODULE_BASE_NAME)GetProcAddress(hPsapi,
                                                  "GetModuleBaseNameW");

        pfnGetModuleFileNameExW = 
            (PFN_GET_MODULE_FILE_NAME_EX)GetProcAddress(hPsapi,
                                                        "GetModuleFileNameExW");

        if (!pfnEnumProcesses ||
            !pfnEnumProcessModules ||
            !pfnGetModuleBaseNameW ||
            !pfnGetModuleFileNameExW)
        {
            goto funcEnd;
        }

        DWORD *processIdList        = NULL;
        DWORD listSizeUsed          = 0;
        BOOL status = FALSE;

        processIdList = new DWORD[M_MAX_PROCESSES];

        if (processIdList == NULL)
        {
            errorCode   = ERROR_NOT_ENOUGH_MEMORY;
            funcResult  = -1;
            goto funcEnd;
        }

        status = pfnEnumProcesses(  
                                processIdList,
                                M_MAX_PROCESSES * sizeof(DWORD),
                                &listSizeUsed);

        DWORD actualNumProcess = listSizeUsed / sizeof(DWORD);

        if (status)
        {
            for (   DWORD procIndex = 0;
                    procIndex < actualNumProcess;
                    ++procIndex)
            {
                HANDLE hProcess = OpenProcess( 
                                    PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                                    FALSE,
                                    processIdList[procIndex]);

                if (NULL != hProcess)
                {
                    HMODULE hMod;
                    DWORD   bytesNeeded;
                    
                    if (pfnEnumProcessModules(
                                            hProcess,
                                            &hMod,
                                            sizeof(hMod),
                                            &bytesNeeded))
                    {
                        wchar_t processName[MAX_PATH]   = {0};
                        wchar_t binaryPath[MAX_PATH]    = {0};

                        //
                        // Get process name and full path of process binary.
                        // failures can be safely ignored here
                        //
                        pfnGetModuleBaseNameW(
                                        hProcess,
                                        hMod,
                                        processName,
                                        sizeof(processName));

                        pfnGetModuleFileNameExW(
                                            hProcess,
                                            hMod,
                                            binaryPath,
                                            MAX_PATH);

                        //
                        // Add the process data to process list
                        //
                        IHU_PROCESS_INFO processInfo;

                        processInfo.mProcessId      = processIdList[procIndex];
                        processInfo.mProcessName    = processName;
                        processInfo.mBinaryName     = binaryPath;

                        oProcessList.push_back(processInfo);
                    }

                    CloseHandle(hProcess);
                }
            }
        }
        else
        {
            errorCode   = GetLastError();
            funcResult  = -1;
        }

        if (processIdList)
        {
            delete [] processIdList;
        }

        FreeLibrary(hPsapi);
    }
    else
    {
        //
        // Use ToolHelp functions exported by kernel32
        //
        HANDLE hProcessSnap             = NULL;
        PROCESSENTRY32W processEntry    = {0};
        PFN_CREATE_TOOLHELP32_SNAPSHOT pfnCreateToolhelp32Snapshot;
        PFN_PROCESS32_FIRST pfnProcess32FirstW;
        PFN_PROCESS32_NEXT pfnProcess32NextW;

        pfnCreateToolhelp32Snapshot =
            (PFN_CREATE_TOOLHELP32_SNAPSHOT)GetProcAddress(hKernel32,
                                                           "CreateToolhelp32Snapshot");

        pfnProcess32FirstW =
            (PFN_PROCESS32_FIRST)GetProcAddress(hKernel32,
                                                "Process32FirstW");

        pfnProcess32NextW = 
            (PFN_PROCESS32_NEXT)GetProcAddress(hKernel32,
                                               "Process32NextW");

        if (!pfnCreateToolhelp32Snapshot ||
            !pfnProcess32FirstW ||
            !pfnProcess32NextW)
        {
            goto funcEnd;
        }

        hProcessSnap = pfnCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

        if (hProcessSnap != INVALID_HANDLE_VALUE)
        {
            processEntry.dwSize = sizeof(processEntry);

            if (pfnProcess32FirstW(hProcessSnap, &processEntry))
            {
                do
                {
                    //
                    // Add the process data to process list
                    //
                    IHU_PROCESS_INFO processInfo;

                    std::wstring processName = processEntry.szExeFile;

                    //
                    // We could use find_last_of here but it use ntdll export
                    // memchr which doesn't work on win9x
                    //
                    size_t nPos = -1;

                    for (   size_t n = processName.length() - 1;
                            n >= 0;
                            --n)
                    {
                        if (processName[n] == L'\\')
                        {
                            nPos = n;
                            break;
                        }
                    }

                    if (nPos != -1)
                    {
                        processName = processName.substr(nPos + 1, processName.length() - nPos - 1);
                    }

                    processInfo.mProcessId      = processEntry.th32ProcessID;
                    processInfo.mProcessName    = processName;
                    processInfo.mBinaryName     = processEntry.szExeFile;

                    oProcessList.push_back(processInfo);

                }while(pfnProcess32NextW(
                            hProcessSnap,
                            &processEntry));
            }

            CloseHandle(hProcessSnap);
        }
    }

funcEnd:

    SetLastError(errorCode);
    return funcResult;
}


DWORD
__cdecl
IhuGetModuleList(
    DWORD inProcessId,
    IHU_MODULE_LIST &oModuleList)
{
    DWORD   funcResult  = 0;
    DWORD           errorCode   = 0;

    oModuleList.clear();


    HANDLE hProcess = OpenProcess(
                            PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                            FALSE,
                            inProcessId);

    if (hProcess == NULL)
    {
        errorCode   = GetLastError();
        funcResult  = -1;

        goto FuncEnd;
    }


    HMODULE *hModuleList = new HMODULE[M_MAX_MODULES];

    if (hModuleList == NULL)
    {
        errorCode   = ERROR_NOT_ENOUGH_MEMORY;
        funcResult  = -1;

        goto FuncEnd;
    }


    BOOL status     = FALSE;
    DWORD cbNeeded  = 0;

    status = EnumProcessModules(
                            hProcess,
                            hModuleList,
                            M_MAX_MODULES * sizeof(HMODULE),
                            &cbNeeded);

    if (status)
    {
        ULONG numModules = cbNeeded / sizeof(HMODULE);

        wchar_t moduleName[MAX_PATH]    = {0};

        for (ULONG moduleIndex = 0; moduleIndex < numModules; ++moduleIndex)
        {
            GetModuleBaseNameW(
                            hProcess,
                            hModuleList[moduleIndex],
                            moduleName,
                            sizeof(moduleName));

            MODULEINFO moduleInfo;
            GetModuleInformation(
                                hProcess,
                                hModuleList[moduleIndex],
                                &moduleInfo,
                                sizeof(moduleInfo));

            IHU_MODULE_INFO ourModuleInfo;

            ourModuleInfo.mModuleHandle         = hModuleList[moduleIndex];
            ourModuleInfo.mModuleBaseName       = moduleName;
            ourModuleInfo.mModuleBaseAddress    = moduleInfo.lpBaseOfDll;

            oModuleList.push_back(ourModuleInfo);
        }
    }
    else
    {
        errorCode   = GetLastError();
        funcResult  = -1;
    }


FuncEnd:

    if (hModuleList)
    {
        delete [] hModuleList;
    }

    if (hProcess)
    {
        CloseHandle(hProcess);
    }

    SetLastError(errorCode);
    return funcResult;
}

