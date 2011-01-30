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

    injection.cpp

Module Description:

	Implements core functionality of injecting a DLL into another
	process. You can inject a DLL in a running process, by giving
	its PID or name. In case a name is given, the very first victim
	process is injected with the given DLL.

	It also provides functionality for launching a process and
	then injecting the DLL into that.

--*/

#include <windows.h>
#include <stdio.h>
#include "ihulib.h"
#include "injection.h"


/*++

Routine Name:
	
	ihiGetProcessIdByName

Routine Description:
	
	This function should called to find the Process Id
	of a running process by supplying the process
	name.

Return:

	If the function is successful then processId of the process
	is returned and If the function fails then 0 is returned.

	In case of failure, call GetLastError for more information

--*/
DWORD
WINAPI
ihiGetProcessIdByName(
	LPCWSTR	inProcessName)
{
	DWORD processId = 0;
	DWORD errorCode = 0;

	IHU_PROCESS_LIST		processList;
	IHU_PROCESS_LIST_ITER	processListIter;
	IHU_PROCESS_INFO		processInfo;

	if (IhuGetProcessList(processList) < 0)
	{
		errorCode = GetLastError();
		goto funcExit;
	}
	
	bool processFound = false;

	for (	processListIter = processList.begin();
			processListIter != processList.end();
			++processListIter)
	{
		processInfo = *processListIter;

		if (_wcsicmp(	processInfo.mProcessName.c_str(),
						inProcessName) == 0)
		{
			processFound = true;
			break;
		}
	}

	if (processFound)
	{
		processId = processInfo.mProcessId;
	}
	else
	{
		errorCode = ERR_PROCESS_NOT_FOUND;
	}

funcExit:

	SetLastError(errorCode);
	return processId;
}



/*++

Routine Name:
	
	ihiLaunchNewProcess

Routine Description:
	
	This function will create a new process and return
	process Id.

Return:

	If the function is successful then processId of the process
	is returned and If the function fails then 0 is returned.

	In case of failure, call GetLastError for more information

--*/
DWORD
WINAPI
ihiLaunchNewProcess(
	LPCWSTR	inExePath)
{
	STARTUPINFO			startupInfo;
    PROCESS_INFORMATION procInfo;

	DWORD	processId = 0;
	DWORD	errorCode = 0;

    ZeroMemory(&startupInfo, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);

	ZeroMemory(&procInfo, sizeof(procInfo));

	wchar_t processCommandLine[MAX_PATH];
	wcscpy(processCommandLine, inExePath);

	BOOL bResult = CreateProcess(
						NULL,
						processCommandLine,
						NULL,
						NULL,
						FALSE,
						CREATE_SEPARATE_WOW_VDM | DEBUG_ONLY_THIS_PROCESS,
						NULL,
						NULL,
						&startupInfo,
						&procInfo);

	if (!bResult)
	{
		errorCode = GetLastError();
		goto funcExit;
	}

	processId = procInfo.dwProcessId;

funcExit:

	SetLastError(errorCode);
	return processId;
}


/*++

Routine Name:

	ihiInitInjectionData

Routine Description:

	Initialize the data which is injected into the target
	process before we create a remote thread in it. This
	data is used by our injector DLL's patching function

Return:

	true	- represent success
	false	- represent failure

--*/
bool
ihiInitInjectionData(
	PINJECTION_DATA injData,
	LPCWSTR	inDllPath,
	LPCSTR		inFnIncludes,
	LPCSTR		inFnExcludes)
{	
	bool funcResult	= false;

	memset(	injData,
			0,
			sizeof(INJECTION_DATA));

	HMODULE hModule = GetModuleHandleA("kernel32.dll");

	if (hModule == NULL)
	{
		goto funcExit;
	}

	PVOID loadLibraryW = GetProcAddress(hModule, "LoadLibraryW");

	if (loadLibraryW == NULL)
	{
		goto funcExit;
	}

	injData->mLoadLibraryW = loadLibraryW;

	PVOID getProcAddr = GetProcAddress(hModule, "GetProcAddress");

	if (getProcAddr == NULL)
	{
		goto funcExit;
	}

	injData->mGetProcAddr = getProcAddr;

	PVOID freeLibrary = GetProcAddress(hModule, "FreeLibrary");

	if (freeLibrary == NULL)
	{
		goto funcExit;
	}

	injData->mFreeLibrary = freeLibrary;

	PVOID getModuleHandle = GetProcAddress(hModule, "GetModuleHandleW");

	if (getModuleHandle == NULL)
	{
		goto funcExit;
	}

	injData->mGetModuleHandleW = getModuleHandle;

	PVOID deleteFile = GetProcAddress(hModule, "DeleteFileW");

	if (deleteFile == NULL)
	{
		goto funcExit;
	}

	injData->mDeleteFileW = deleteFile;

	PVOID sleep = GetProcAddress(hModule, "Sleep");

	if (sleep == NULL)
	{
		goto funcExit;
	}

	injData->mSleep = sleep;

	strcpy((LPSTR)injData->mInitFnName, "IhSerumLoad");
	strcpy((LPSTR)injData->mDeInitFnName, "IhSerumUnload");
	strcpy((LPSTR)injData->mThreadUsageFnName, "IhSerumGetRefCount");
	wcscpy(injData->mDllName, inDllPath);

	strncpy((LPSTR)injData->mFnIncludes, inFnIncludes, MAX_INC_EXC_SIZE - 1);
	strncpy((LPSTR)injData->mFnExcludes, inFnExcludes, MAX_INC_EXC_SIZE - 1);

	injData->mFnIncludes[MAX_INC_EXC_SIZE - 1] = 0;
	injData->mFnExcludes[MAX_INC_EXC_SIZE - 1] = 0;

	funcResult = true;

funcExit:

	return funcResult;
}



/*++

Routine Name:

	ihiInjectDll

Routine Description:

	Inject a DLL into a running process by calling
	CreateRemoteThread

Return:

	true	- represent success
	false	- represent failure

	In case of failure, please call GetLastError for
	more information.

--*/
bool
WINAPI
ihiInjectDll(
	HANDLE			hProcess,	
	LPCWSTR	inDllPath,
	LPCSTR		inFnIncludes,
	LPCSTR		inFnExcludes)
{
	LPVOID			pInjectionData;
	LPVOID			pInjectionCode;
	SIZE_T			notUsed;
	bool			funcResult = false;


	INJECTION_DATA injData;

	//
	// Allocate the memory inside target process
	//
	pInjectionData = VirtualAllocEx(
								hProcess,
								NULL,
								sizeof(injData),
								MEM_COMMIT,
								PAGE_READWRITE);

	if (pInjectionData == NULL)
	{
		goto funcExit;
	}

	ihiInitInjectionData(
					&injData,
					inDllPath,
					inFnIncludes,
					inFnExcludes);

	WriteProcessMemory(
					hProcess,
					pInjectionData,
					&injData,
					sizeof(injData),
					&notUsed);

	ULONG codeSize = 0;

	codeSize = (DWORD_PTR)ihiInjectedCodeEnd - (DWORD_PTR)ihiInjectedCode;

	//
	// Allocate the memory inside target process
	//
	pInjectionCode = VirtualAllocEx(
								hProcess,
								NULL,
								codeSize,
								MEM_COMMIT,
								PAGE_EXECUTE_READWRITE);

	if (pInjectionCode == NULL)
	{
		goto funcExit;
	}
	
	WriteProcessMemory(
					hProcess,
					pInjectionCode,
					ihiInjectedCode,
					codeSize,
					&notUsed);

	DWORD threadId = 0;

	HANDLE hThread = CreateRemoteThread(	
								hProcess,
								0,
								0,
								(LPTHREAD_START_ROUTINE)pInjectionCode,
								pInjectionData,
								0,
								&threadId);

	if (hThread)
	{
		// Set the return status
		funcResult = true;
	}

funcExit:

	return funcResult;
}



bool
WINAPI
ihiUninjectDll(
	HANDLE hProcess,
	LPCWSTR	inDllPath)
/*++

Routine Description:
	
	This routine removes an already injected DLL from the process

Arguments:

	hProcess - Handle to the process with injected DLL
	inDllPath - Path of the injected DLL to be removed

--*/
{
	LPVOID			pInjectionData;
	LPVOID			pInjectionCode;
	SIZE_T			notUsed;
	bool			funcResult = false;

	INJECTION_DATA injData;

	//
	// Allocate the memory inside target process
	//
	pInjectionData = VirtualAllocEx(
								hProcess,
								NULL,
								sizeof(injData),
								MEM_COMMIT,
								PAGE_READWRITE);

	if (pInjectionData == NULL)
	{
		goto funcExit;
	}

	ihiInitInjectionData(
					&injData,
					inDllPath,
					"",
					"");

	WriteProcessMemory(
					hProcess,
					pInjectionData,
					&injData,
					sizeof(injData),
					&notUsed);

	ULONG codeSize = 0;

	codeSize = (DWORD_PTR)ihiUnloadCodeEnd - (DWORD_PTR)ihiUnloadCode;

	//
	// Allocate the memory inside target process
	//
	pInjectionCode = VirtualAllocEx(
								hProcess,
								NULL,
								codeSize,
								MEM_COMMIT,
								PAGE_EXECUTE_READWRITE);

	if (pInjectionCode == NULL)
	{
		goto funcExit;
	}
	
	WriteProcessMemory(
					hProcess,
					pInjectionCode,
					ihiUnloadCode,
					codeSize,
					&notUsed);

	DWORD threadId = 0;

	HANDLE hThread = CreateRemoteThread(	
								hProcess,
								0,
								0,
								(LPTHREAD_START_ROUTINE)pInjectionCode,
								pInjectionData,
								0,
								&threadId);

	if (hThread)
	{
		// Set the return status
		funcResult = true;
	}

funcExit:

	return funcResult;
}


//
// Turn off optimizations here because they can cause problem with
// our injection code which shouldn't be optimized without our knowledge
//
#pragma optimize("g", off)


/*++

Routine Name:

	ihiInjectedCode

Routine Description:

	This function is actually injected in the target process
	and it executes in the context of target process. It
	first loads our injector DLL in target process and then
	calls its patching initiate function to patch the IAT
	of target process.

Return:

	none

--*/
static
void
ihiInjectedCode(
	LPVOID *inAddress)
{
	HMODULE hMod;

    _asm push eax;
	_asm mov eax, 1;
debug:
	_asm cmp eax, 0;
	_asm je debug;
	_asm pop eax;

	//
	// This function cannot make direct function calls
	// because it is injected in target process by calling
	// WriteProcessMemory and if we use any direct function
	// calls, its address in new process may not be correct.
	// Hence, we use function pointers only. This is *ONLY*
	// safe because kernel32.dll is loaded at same address
	// in all the modules
	//

	INJECTION_DATA *injData = (INJECTION_DATA *)inAddress;
	PFNLOADLIBRARY pfnLoadLibrary = (PFNLOADLIBRARY)injData->mLoadLibraryW;

	hMod = pfnLoadLibrary(
					injData->mDllName);

	if (hMod)
	{
		PFNGETPROCADDRESS pfnGetProcAddress = (PFNGETPROCADDRESS)injData->mGetProcAddr;

		// Get the DLL's actual init function
		PFNSERUMLOAD pfnSerumLoad =
            (PFNSERUMLOAD)pfnGetProcAddress(hMod, (LPCSTR)injData->mInitFnName);

		if (pfnSerumLoad != NULL)
		{
			pfnSerumLoad((LPCSTR)injData->mFnIncludes, (LPCSTR)injData->mFnExcludes);
		}
	}
}


/*++

Routine Name:

	ihiInjectedCodeEnd

Routine Description:

	This is just a dummy function and is required in
	ihiInjectedCode function's size calculation

--*/
static
void
ihiInjectedCodeEnd()
{
	bool unused;
	unused = true;
}



/*++

Routine Name:

	ihiUnloadCode

Routine Description:

	This function is actually injected in the target process
	and it executes in the context of target process. It simply
	tries to unload our DLL from the target process

Return:

	none

--*/
static
void
ihiUnloadCode(
	LPVOID *inAddress)
{
	HMODULE hMod;

	_asm push eax;
	_asm mov eax, 1;
debug:
	_asm cmp eax, 0;
	_asm je debug;
	_asm pop eax;

	//
	// This function cannot make direct function calls
	// because it is injected in target process by calling
	// WriteProcessMemory and if we use any direct function
	// calls, its address in new process may not be correct.
	// Hence, we use function pointers only. This is *ONLY*
	// safe because kernel32.dll is loaded at same address
	// in all the modules
	//
	INJECTION_DATA *injData = (INJECTION_DATA *)inAddress;

	PFNGETMODULEHANDLE pfnGetModuleHandle =
							(PFNGETMODULEHANDLE)injData->mGetModuleHandleW;

	hMod = pfnGetModuleHandle(
					injData->mDllName);

	if (hMod)
	{
		PFNGETPROCADDRESS pfnGetProcAddress = (PFNGETPROCADDRESS)injData->mGetProcAddr;

		PFNFREELIBRARY pfnFreeLibrary = (PFNFREELIBRARY)injData->mFreeLibrary;

		PFNSLEEP pfnSleep = (PFNSLEEP)injData->mSleep;

		PFNSERUMUNLOAD pfnSerumUnload =
            (PFNSERUMUNLOAD)pfnGetProcAddress(hMod, (LPCSTR)injData->mDeInitFnName);

		PFNSERUMGETREFCOUNT pfnSerumGetRefCount =
            (PFNSERUMGETREFCOUNT)pfnGetProcAddress(hMod, (LPCSTR)injData->mThreadUsageFnName);

		if (pfnSerumUnload != NULL)
		{
			pfnSerumUnload();

			while (true)
			{
				if (pfnSerumGetRefCount() == 0)
				{
					if (!pfnFreeLibrary(hMod))
					{
						pfnFreeLibrary(hMod);
					}

					break;
				}
				else
				{
					pfnSleep(500);
				}
			}
		}
	}
	else
	{
		// Issue error here and bail out
	}

	PFNDELETEFILE deleteFile = (PFNDELETEFILE)injData->mDeleteFileW;
	if (!deleteFile(injData->mDllName))
	{
		// Issue error here and bail out
	}
}


/*++

Routine Name:

	ihiUnloadCodeEnd

Routine Description:

	This is just a dummy function and is required in
	ihiUnloadCode function's size calculation

--*/
static
void
ihiUnloadCodeEnd(void)
{
	bool unused;
	unused = false;
}

#pragma optimize( "g", on)