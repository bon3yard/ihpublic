/*++

Copyright (c) 2015, Pankaj Garg <pankaj@intellectualheaven.com>
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
    
    cdcore.cpp

Module Description:

    Implements CrashDoctor as a debugger to protect an application from
    crashing. This file implements the core functionality of CrashDoctor, It
    attaches to the application as a debugger, if the application generates an
    exception, it generates a call stack and tries to bypass the faulting
    function such that application can continue execution.

--*/

//System-Includes!!!
#include <windows.h>
#include <stdio.h>
#include <string>
#include <dbghelp.h>
#include <tchar.h>
#include <vector>
#include <map>
#include <stack>
#include <shlobj.h>
#include <objidl.h>
#include <strsafe.h>

// Local-Includes!!!
#include "ihulib.h"
#include "trcdefs.h"
#include "cdutil.h"
#include "cdcore.h"
#include "cdmain.h"

extern "C" int _stdcall LDE(PVOID address, int mode);

//
// Symbol initialization thread.
//
DWORD
WINAPI
cdSymInitializeThread(LPVOID inParam);



//
// Global variables to store the loaded address of the main .exe image of the
// faulting process. They are not used right now during crash recovery but
// they can be if the crash is happening in a DLL then we may want to pass
// control all the way back upto the original process exe.
//
DWORD_PTR gStartAddr;
DWORD_PTR gEndAddr;


CRecoverCrash::CRecoverCrash()
/*++

Routine Description:
	
	CRecoverCrash Constructor. It initializes the required variables and set
	mCrashedProcessInfo.notInitialized to true. It will be set to false once
	we get a Create process debug event for the crashing process

--*/
{
    mCrashedProcessInfo.notInitialized  = true;
    mKeepAlive                          = true;
    mCountCrash                         = 0;
    mSymbolInitialized                  = FALSE;
	mCrashRecovered						= false;

    memset(mDbgString, 0, sizeof(mDbgString));

	//
	// Initialize the functions information that we should map
	//
	mFunctionNameToHandlerMap["CreateFileW"] = 
										&CRecoverCrash::HandleCreateFileW;

	mFunctionNameToHandlerMap["CreateFileA"] = 
										&CRecoverCrash::HandleCreateFileA;
}


CRecoverCrash::~CRecoverCrash()
/*++

Routine Description:
	
	CRecoverCrash Destructor

--*/
{
}


void
__cdecl
CRecoverCrash::HandleError(
	DWORD	inErrorCode,
	LPCTSTR	inErrorMsg,
	...)
/*++

Routine Description:
	
	Process CrashDoctor specific or Windows error code and display that error
    to the user.

Return:

	none

--*/
{
	tstring		errorMessage;
	TCHAR		szMsg[1024];
	va_list		arg;

	ASSERT(inErrorCode);

	if (inErrorMsg)
	{
		va_start(arg, inErrorMsg);

		StringCchVPrintf(
						szMsg,
						sizeof(szMsg)/sizeof(TCHAR),
						inErrorMsg,
						arg);
		va_end(arg);

		errorMessage = szMsg;
	}

	if (!errorMessage.empty())
	{
		errorMessage += _T("\n");
	}

	switch(inErrorCode)
	{	
		case ERR_INVALID_PROCESS_ID:
		{
			mRecoveryHandler->PrintError(
                    _T("%sError: Invalid Process ID (PID) specified."),
                    errorMessage.c_str());

			break;
		}
		default:
		{
			LPTSTR lpMsgBuf = NULL;

			if (FormatMessage( 
					FORMAT_MESSAGE_ALLOCATE_BUFFER | 
					FORMAT_MESSAGE_FROM_SYSTEM | 
					FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					inErrorCode,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					(LPTSTR)&lpMsgBuf,
					0,
					NULL))
			{
				mRecoveryHandler->PrintError(
										_T("%s%s"),
										errorMessage.c_str(),
										lpMsgBuf);

				LocalFree(lpMsgBuf);
			}
			else
			{
				mRecoveryHandler->PrintError(
                        _T("%sUnknown error occured. Error code = %x"),
                        errorMessage.c_str(),
                        inErrorCode);
			}

			break;
		}
	}
}


void
CRecoverCrash::HandleCreateFileW(
	CONTEXT	&inContext)
/*++

Routine Description:

	This function is called when the crashing process calls CreateFileW. If
	the faulting process is opening a file for write, then we would create
	a backup of the file

Arguments:

    inContext - CONTEXT of the thread calling CreateFileW

Return:

	none

--*/
{
	HandleCreateFileCommon(inContext, true);
}


void
CRecoverCrash::HandleCreateFileA(
	CONTEXT	&inContext)
/*++

Routine Description:

	This function is called when the crashing process calls CreateFileA. If
	the faulting process is opening a file for write, then we would create
	a backup of the file

Arguments:

    inContext - CONTEXT of the thread calling CreateFileA

Return:

	none

--*/
{
	HandleCreateFileCommon(inContext, false);
}


void
CRecoverCrash::HandleCreateFileCommon(
	CONTEXT	&inContext,
	bool	inUnicodeFileName)
/*++

Routine Description:

	This function first checks if the file is opened for write access, if true
	then it reads the file name supplied in the call to CreateFile. It then
	generates a destination file name and copies the file, that faulting
	process is trying to open, to the destination file

Arguments:

    inContext - CONTEXT of the thread calling CreateFileA/W

	inUnicodeFileName - tells if we are called as a result of CreateFileW or A

Return:

	none

--*/
{
	DWORD		requestedAccess;
	wchar_t		unicodePath[MAX_PATH]		= {0};
	char		ansiPath[MAX_PATH]			= {0};
	PVOID		pFilePath					= NULL;
	tstring		filePath;
	tstring		fileName;
	tstring		destFilePath;
	TCHAR		tempPath[MAX_PATH];

    //
    // TODO: Make helper functions called read first parameter, read second
    // parameter etc.
    //

#if defined(_M_IX86)
	if (!DebuggeeReadMemory(
			(LPVOID)(inContext.Esp + 8),
			&requestedAccess,
			sizeof(requestedAccess)))
	{
		goto funcEnd;
	}
#elif defined(_M_AMD64)
	requestedAccess = (DWORD)inContext.Rdx;
#endif

	// If any write bit is on then we should backup the file
	if ((GENERIC_WRITE & requestedAccess) == 0)
	{
		goto funcEnd;
	}

#if defined(_M_IX86)
	if (!DebuggeeReadMemory(
			(LPVOID)(inContext.Esp + 4),
			&pFilePath,
			sizeof(pFilePath)))
	{
		goto funcEnd;
	}
#elif defined(_M_AMD64)
	pFilePath = (LPVOID)(DWORD_PTR)inContext.Rcx;
#endif

	if (!DebuggeeReadMemory(
			pFilePath,
			(inUnicodeFileName) ? unicodePath : (wchar_t *)ansiPath,
			(inUnicodeFileName) ? sizeof(unicodePath) : sizeof(ansiPath)))
	{
		goto funcEnd;
	}

	if (!inUnicodeFileName)
	{
		// If the file name read from debuggee is not unicode then 
        // convert it to unicode.
		MultiByteToWideChar(
						CP_UTF8,
						0,
						ansiPath,
						-1,
						unicodePath,
						MAX_PATH);
	}
	
	filePath = unicodePath;

	size_t index = filePath.find_last_of(_T('\\'));
	if (index != -1)
	{
		fileName = filePath.substr(index + 1, filePath.length() - index);
	}
	
	if (SHGetFolderPath(
				NULL,
				CSIDL_APPDATA | CSIDL_FLAG_CREATE,
				NULL,
				SHGFP_TYPE_CURRENT,
				tempPath) == S_OK)
	{
		destFilePath = tempPath;
		destFilePath += _T("\\CrashDoctor\\");
		CreateDirectory(destFilePath.c_str(), NULL);
	}

	SYSTEMTIME time;
	::GetSystemTime(&time);

	StringCchPrintf(
				tempPath,
				MAX_PATH,
				_T("%04d%02d%02d%02d%02d%02d%03d"),
				time.wYear,
				time.wMonth,
				time.wDay,
				time.wHour,
				time.wMinute,
				time.wSecond,
				time.wMilliseconds);

	destFilePath += tempPath;
	destFilePath += _T(".");
	destFilePath += fileName;

	mRecoveryHandler->PrintInfo(
				_T("Faulty process is trying to open %s for write access\n"),
				filePath.c_str());

	mRecoveryHandler->PrintInfo(
				_T("CrashDoctor will make a backup copy of this file at: %s\n"),
				destFilePath.c_str());

	CopyFile(filePath.c_str(), destFilePath.c_str(), TRUE);

	// We introduced sleep here because without it, if another CreateFile
	// is called by debuggee immediately, then we were not able to generate
	// a unique file name. Performance is not important here because we are
	// only do this when a fauling process calls CreateFile with write access
	// on a file
	Sleep(100);

funcEnd:
	return;
}


void
CRecoverCrash::HandleCreateProcess(
    DEBUG_EVENT				&inDebugEvent,
	CD_SYMBOL_PROCESS_INFO	&symProcInfo)
/*++

Routine Description:

	This is mostly the first event we get once we attach to a process. This
	would simply store the process information supplied in the debug event.

Arguments:

    inDebugEvent - Reference to corresponding debug event

Return:

	none

--*/
{
    IHU_DBG_LOG(TRC_CDCORE, IHU_LEVEL_INFO, (L"Create Process\n"));

    if (mCrashedProcessInfo.notInitialized)
    {
        mCrashedProcessInfo.notInitialized = false;

		//
		// Launch the symbol initialization thread. Once this thread is
		// launched we will wait for it to finish in 5 seconds. If it takes
		// more than 5 seconds, then we will assume that symbol initialization
		// failed and we will use our in-house stack generaiton approach
		// rather than relying upon symbol support for that
		//
		symProcInfo.ProcessId = inDebugEvent.dwProcessId;
		symProcInfo.ProcessHandle = inDebugEvent.u.CreateProcessInfo.hProcess;
		symProcInfo.SymbolInitialized = false;

		InitializeSymbolHelper(symProcInfo);

        mCrashedProcessInfo.hProcess = 
                    inDebugEvent.u.CreateProcessInfo.hProcess;
        
        mCrashedProcessInfo.processId =
                    inDebugEvent.dwProcessId;

        //
        // Check if this is a 32-bit process.
        //
        mCrashedProcessInfo.isProcess32bit = TRUE;

#if defined (_M_AMD64)
        mCrashedProcessInfo.isProcess32bit = FALSE;
        
        BOOL isWow64Process = FALSE;
        if (IsWow64Process(mCrashedProcessInfo.hProcess, &isWow64Process) && isWow64Process)
        {
            mCrashedProcessInfo.isProcess32bit = TRUE;
        }
#endif

        //
        // TODO - One approach to recover the crash is to unwind the stack till
        // we bypass all DLLs and reach the process's main executable's
        // address. This is a very aggressive approach to bypass crash and can
        // cause many DLLs function calls to be skipped. As of now i am not
        // using this approach. If in future, this approach is implemented then
        // we would need to find the size of process image so that we can
        // calculate the gEndAddr. We have to calculate the End address ourself
        // because the start address is given in CreateProcessInfo but
        // gEndAddress is not
        //
        gStartAddr =
            (DWORD_PTR)
                inDebugEvent.u.CreateProcessInfo.lpBaseOfImage;
        
        gEndAddr =
            (DWORD_PTR)((char *)gStartAddr + 0x10000);


        CD_THREAD_INFO threadInfo;

        threadInfo.hThread	= 
                        inDebugEvent.u.CreateProcessInfo.hThread;
        
        threadInfo.threadId	= inDebugEvent.dwThreadId;

        mCrashedProcessInfo.threadList.push_back(threadInfo);

		// Treat this process as DLL as well for inserting breakpoints
		// On NT et al we only insert breakpoint in CreateFileA/W but
		// since 9x doesn't support copy on write, we hook IAT for that
		HandleCreateFileHooking(
					(LPBYTE)inDebugEvent.u.CreateProcessInfo.lpBaseOfImage);
    }
}


void
CRecoverCrash::HandleExitProcess(
    DEBUG_EVENT &inDebugEvent)
/*++

Routine Description:

	We will terminate the debugger loop once we get this event.

Arguments:

    inDebugEvent - Reference to corresponding debug event

Return:

	none

--*/
{
    mKeepAlive = false;
	mRecoveryHandler->PrintMessage(
		_T("Faulty process has exited. Exit code: 0x%X\n"),
		inDebugEvent.u.ExitProcess.dwExitCode);
}


void
CRecoverCrash::HandleCreateThread(
    DEBUG_EVENT &inDebugEvent)
/*++

Routine Description:

	A new thread is created in the system, store its information in the
	process information struct.

Arguments:

    inDebugEvent - Reference to corresponding debug event

Return:

	none

--*/
{
    CD_THREAD_INFO threadInfo;
                    
    threadInfo.hThread  = inDebugEvent.u.CreateThread.hThread;
    threadInfo.threadId = inDebugEvent.dwThreadId;

    mCrashedProcessInfo.threadList.push_back(threadInfo);
}


void
CRecoverCrash::HandleExitThread(
    DEBUG_EVENT &inDebugEvent)
/*++

Routine Description:

	A thread is terminating in the system, remove its information from the
	process information struct.

Arguments:

    inDebugEvent - Reference to corresponding debug event

Return:

	none

--*/
{
    //
    // Remove the thread which matches Id
    //
    THREAD_LIST_ITER threadIter;

    for (	threadIter = mCrashedProcessInfo.threadList.begin();
            threadIter != mCrashedProcessInfo.threadList.end();
            ++threadIter)
    {
        CD_THREAD_INFO threadInfo = *threadIter;

        if (threadInfo.threadId == inDebugEvent.dwThreadId)
        {
            mCrashedProcessInfo.threadList.erase(threadIter);
            
            break;
        }
    }
}


void
CRecoverCrash::HandleOutputDebugString(
    DEBUG_EVENT &inDebugEvent)
/*++

Routine Description:

	Handles the OUTPUT_DEBUG_STRING_EVENT generated by the faulting process

Arguments:

    inDebugEvent - Reference to corresponding debug event

Return:

	none

--*/
{
    SIZE_T cbRead = 0;

    DebuggeeReadMemory(
                inDebugEvent.u.DebugString.lpDebugStringData,
                mDbgString,
                inDebugEvent.u.DebugString.nDebugStringLength);

    if (inDebugEvent.u.DebugString.fUnicode)
    {
        mRecoveryHandler->PrintTrace(_T("%ws"), mDbgString);
    }
    else
    {
        mRecoveryHandler->PrintTrace(_T("%s"), mDbgString);
    }
}


void
CRecoverCrash::HandleLoadDll(
    DEBUG_EVENT &inDebugEvent)
/*++

Routine Description:

	Handles the LOAD_DLL_DEBUG_EVENT generated by the faulting process. When
	the load dll event for kernel32.dll is received, we should insert breakpt
	in the CreateFileA/W apis

Arguments:

    inDebugEvent - Reference to corresponding debug event

Return:

	none

--*/
{
	HandleCreateFileHooking((PBYTE)inDebugEvent.u.LoadDll.lpBaseOfDll);
}


void
CRecoverCrash::HandleCreateFileHooking(
	LPBYTE inImageAddress)
/*++

Routine Description:

	For windows NT series this function attempts to insert breakpoint in
	kernel32!CreateFileA/W

	For windows 9x, this function attempts to patch IAT table of other modules
	which import CreateFileA/W from kernel32.dll

Arguments:

    inImageAddress - Address of the loaded executable or dll

Return:

	none

--*/
{
	//
	// We are #ifdef'ing the code because the other approach used below to
	// match base address of DLL with module handle is working. If somehow
	// that approach doesn't work then we should enable this #ifed out
    // approach to find when kernel32.dll load notificaiton is received
	//
#if USE_READ_IMAGE_NAME_FUNCTION
	char imageName[MAX_PATH];

	if (ReadDllImageName(
				inImageAddress,
				imageName,
				MAX_PATH))
	{
		if (_stricmp(imageName, "kernel32.dll") == 0)
		{
			InsertBreakPoints(inImageAddress);
			return;
		}
	}
#endif

	//
	// A dll's handle is its base address as well. We know that kernel32.dll
	// is loaded at same address in all the processes, hence we can compare
	// the dll base address in debuggee process with module handle of
	// kernel32.dll in debugger process
	//
	if (GetModuleHandle(_T("kernel32.dll")) == (HMODULE)inImageAddress)
	{
		//
		// For windows NT, we insert a breakpoint in CreateFileW to track when
		// it is called.
        //
        InsertBreakPoints(inImageAddress);
	}

    //
    // For Win9x earlier code present here was. It is kept for history right now.
    // Will be removed in next checkin.
    //
    else
    {
        //
        // For windows 9x series, hook all the dlls IAT entries which import
        // kernel32.dll's CreateFileW/A functions
        //
        //if (gOSVersion == WIN32_9X)
        // {
            //HookCreateFileIATEntries(inImageAddress);
        //}
    }
}


bool
CRecoverCrash::InsertBreakPoints(
	PBYTE		inImageBase)
/*++

Routine Description:

	This function will traverse through the export table of the given DLL
	and insert breakpoint in all functions that we need to monitor

Arguments:

    inImageBase - loaded address of the DLL

Return:

	true - always (TODO - see if we ever need to return false)

--*/
{
	DWORD cbRead = 0;
	IMAGE_DOS_HEADER dh;

    if (!DebuggeeReadMemory(
			(LPVOID)inImageBase,
			&dh,
			sizeof(IMAGE_DOS_HEADER)))
	{
        goto funcEnd;
    }

    if (dh.e_magic != IMAGE_DOS_SIGNATURE)
	{
        goto funcEnd;
    }

    IMAGE_NT_HEADERS nh;

    if (!DebuggeeReadMemory(
				(LPVOID)(inImageBase + dh.e_lfanew),
				&nh,
				sizeof(IMAGE_NT_HEADERS)))
	{
        goto funcEnd;
    }

	PIMAGE_DATA_DIRECTORY expDataDir = (PIMAGE_DATA_DIRECTORY)
		&nh.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];

    if (!expDataDir->VirtualAddress)
	{
        goto funcEnd;
    }

    PIMAGE_EXPORT_DIRECTORY expDir = 
							(PIMAGE_EXPORT_DIRECTORY)malloc(expDataDir->Size);

	if (!expDir)
	{
		mRecoveryHandler->PrintError(
				_T("Unable to allocate 0x%X bytes in the debugger.\n"),
				expDataDir->Size);
		goto funcEnd;
	}

    if (!DebuggeeReadMemory(
				(LPVOID)(inImageBase + expDataDir->VirtualAddress),
				expDir,
				expDataDir->Size))
	{
        goto funcEnd;
    }

	//
	// dbgrRelImageBase is the base (fake) of DLL image relative to expDir
	// in debuggers memory. It helps calculating VA from RVA in debuggers
	// memory. Let us understand this more:
	//
	// A sample layout of DLL image in debuggee's memory:
	// inImageBase |expDataDir->VirtualAddress|Export directory
	//      ->start|->>>>>>>>>>>>>>>>>>>>>>>>>|>>>>>>>>>....
	//
	// Debugger process reads Export directory in expDir. But all the entries
	// in expDir uses RVA, which means they refer offset from the base of DLL
	// and not the start of Export directory. We simply calculate a fake DLL
	// image base address in debugger memory relative to expDir and do all the
	// RVA to VA conversion based on this address
	//
	PBYTE dbgrRelImageBase = 
						(PBYTE)((PBYTE)expDir - expDataDir->VirtualAddress);

	PULONG fnAddrs		=
				(PULONG)(dbgrRelImageBase + expDir->AddressOfFunctions);

	PUSHORT nameOrds	=
				(PUSHORT)(dbgrRelImageBase + expDir->AddressOfNameOrdinals);

	PULONG fnNames		=
				(PULONG)(dbgrRelImageBase + expDir->AddressOfNames);

	//
	// Refer Window NT Native API book, page 450 section Exception/Debugging
	// The example in the book even handles functions exported by Ordinal but
	// since we are only interested in CreateFileA/W which are exported by
	// name hence we will use NumberOfNames instead of NumberOfFunctions as
	// described in the book
	//
	for (DWORD fnIndex = 0; fnIndex < expDir->NumberOfNames; fnIndex++)
	{
		PSTR fnName = (PSTR)(dbgrRelImageBase + fnNames[fnIndex]);

		FUNCTION_NAME_TO_HANDLER_MAP::iterator fnNameToHndlrMapIter;

		fnNameToHndlrMapIter = mFunctionNameToHandlerMap.find(fnName);

		if (fnNameToHndlrMapIter != mFunctionNameToHandlerMap.end())
		{
			DWORD ordinal = nameOrds[fnIndex];

			//
			// According to PE format, if the RVA of function address falls
			// in the Export directory section itself then that export is a
			// forwarded export and we ignore any forwarded export
			//
			if (fnAddrs[ordinal] >= expDataDir->VirtualAddress &&
				fnAddrs[ordinal] <= (expDataDir->VirtualAddress + expDataDir->Size))
			{
				continue;
			}

			// Calculate functions VA in the debuggee process
			PBYTE func = (PBYTE)(inImageBase + fnAddrs[fnIndex]);

			InsertBreakPoint(func, fnName);
		}
	}

funcEnd:

	return true;
}


void
CRecoverCrash::InsertBreakPoint(
	LPVOID		inAddress,
	std::string	inFunctionName)
/*++

Routine Description:

	Insert the breakpoint inside the given function at the given address. If
	the breakpoint insertion is complete, store this information so that when
	we hit this breakpoint we can restore the original instruction.

Arguments:

    inAddress - Memory address at which we should insert the breakpoint

	inFunctionName - Name of the funciton inside which we are putting the bp

Return:

	none

--*/
{
	BYTE	bp				= 0xCC;
	BYTE	op				= 0;
	DWORD	dwOldProtect;

	if (VirtualProtectEx(
				mCrashedProcessInfo.hProcess,
				inAddress,
				sizeof(BYTE),
				PAGE_READWRITE,
				&dwOldProtect))
	{
		if (!DebuggeeReadMemory(
					(LPVOID)inAddress,
					&op,
					sizeof(op)))
		{
			goto funcEnd;
		}

		if (!DebuggeeWriteMemory(
					(LPVOID)inAddress,
					&bp,
					sizeof(bp)))
		{
			goto funcEnd;
		}

		BP_INFO &bpInfo		= mAddrToBpInfoMap[inAddress];
		bpInfo.OriginalByte = op;
		bpInfo.FunctionName	= inFunctionName;

		DWORD dwTemp;
		VirtualProtectEx(
				mCrashedProcessInfo.hProcess,
				inAddress,
				sizeof(BYTE),
				dwOldProtect,
				&dwTemp);
	}
	else
	{
		HandleError(
			GetLastError(),
			_T("Unable to set memory access at 0x%X in the faulty process."),
			inAddress);
	}

funcEnd:

	return;
}


void
CRecoverCrash::ReInsertBreakPoint(
	LPVOID	inAddress)
/*++

Routine Description:

	Reinsert simply change one byte at given address with 0xCC

Arguments:

    inAddress - Memory address at which we should insert the breakpoint

Return:

	none

--*/
{
	BYTE bp = 0xcc;

	if (!DebuggeeWriteMemory(
				(LPVOID)inAddress,
				&bp,
				sizeof(bp)))
	{
		goto funcEnd;
	}

funcEnd:

	return;
}


#if 0 // 9X code gone.
bool
CRecoverCrash::HookCreateFileIATEntries(
	PBYTE		inImageBase)
/*++

Routine Description:

	For windows 9x et al, it is not possible to insert breakpoint in CreateFile
	because it resides in shared address space, so we instead hook the IAT of
	all the modules which call CreateFileA/W

Arguments:

    inImageBase - base address of the image whose IAT should be hooked

Return:

	false - an error occured during hooking
	true - no error occured, if the module was importing CreateFile then it
		is hooked

--*/
{
	DWORD cbRead = 0;
	IMAGE_DOS_HEADER dh;

    if (!DebuggeeReadMemory(
			(LPVOID)inImageBase,
			&dh,
			sizeof(IMAGE_DOS_HEADER)))
	{
        goto funcEnd;
    }

    if (dh.e_magic != IMAGE_DOS_SIGNATURE)
	{
		mRecoveryHandler->PrintError(
				_T("The image doesn't have a valid DOS header.\n"));

        goto funcEnd;
    }

    IMAGE_NT_HEADERS nh;

    if (!DebuggeeReadMemory(
				(LPVOID)(inImageBase + dh.e_lfanew),
				&nh,
				sizeof(IMAGE_NT_HEADERS)))
	{
        goto funcEnd;
    }

	PIMAGE_DATA_DIRECTORY impAddrDir = (PIMAGE_DATA_DIRECTORY)
		&nh.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];

	// No import table
    if (!impAddrDir->VirtualAddress)
	{
		mRecoveryHandler->PrintError(
				_T("The image doesn't have an Import Address Table.\n"));

        goto funcEnd;
    }

    PIMAGE_IMPORT_DESCRIPTOR pIIDArray = 
						(PIMAGE_IMPORT_DESCRIPTOR)malloc(impAddrDir->Size);

	if (!pIIDArray)
	{
		mRecoveryHandler->PrintError(
			_T("Unable to allocate 0x%X bytes in the debugger.\n"),
			impAddrDir->Size);

		goto funcEnd;
	}

    if (!DebuggeeReadMemory(
				(LPVOID)(inImageBase + impAddrDir->VirtualAddress),
				pIIDArray,
				impAddrDir->Size))
	{
        goto funcEnd;
    }

	PIMAGE_IMPORT_DESCRIPTOR pIID = pIIDArray;

	while(true)
	{
		if (pIID->FirstThunk == 0 || pIID->OriginalFirstThunk == 0)
		{
			// Loop exit condition
            break;
		}

		char pszModule[MAX_PATH] = {0};

		if (!DebuggeeReadMemory(
				(LPVOID)(inImageBase + pIID->Name),
				pszModule,
				sizeof(pszModule)))
		{
			goto funcEnd;
		}

		if (_stricmp(pszModule, "kernel32.dll") != 0)
		{
			++pIID;
			continue;
		}


		ULONG firstThunkRVA = pIID->FirstThunk;
		ULONG origThunkRVA	= pIID->OriginalFirstThunk;

		// Initialize with one less because we increment at the start of while
		// loop below
		firstThunkRVA	-= sizeof(IMAGE_THUNK_DATA);
		origThunkRVA	-= sizeof(IMAGE_THUNK_DATA);

		while (true)
		{
			firstThunkRVA	+= sizeof(IMAGE_THUNK_DATA);
			origThunkRVA	+= sizeof(IMAGE_THUNK_DATA);

			IMAGE_THUNK_DATA firstITD;
			IMAGE_THUNK_DATA origITD;

			if (!DebuggeeReadMemory(
					(LPVOID)(inImageBase + firstThunkRVA),
					&firstITD,
					sizeof(firstITD)))
			{
				continue;
			}

			if (firstITD.u1.Ordinal == 0)
			{
				break;
			}

			if (!DebuggeeReadMemory(
					(LPVOID)(inImageBase + origThunkRVA),
					&origITD,
					sizeof(origITD)))
			{
				continue;
			}

			if (IMAGE_SNAP_BY_ORDINAL(origITD.u1.Ordinal))
			{
				continue;
			}

			char iinMaxBuffer[MAX_PATH + sizeof(IMAGE_IMPORT_BY_NAME)];
			IMAGE_IMPORT_BY_NAME *pIIN = (IMAGE_IMPORT_BY_NAME *)iinMaxBuffer;

			if (!DebuggeeReadMemory(
					(LPVOID)(inImageBase + origITD.u1.AddressOfData),
					pIIN,
					sizeof(iinMaxBuffer)))
			{
				continue;
			}

			FUNCTION_NAME_TO_HANDLER_MAP::iterator fnNameToHndlrMapIter;

			LPSTR fnName = NULL;
			fnName = (LPSTR)pIIN->Name;

			fnNameToHndlrMapIter = mFunctionNameToHandlerMap.find(fnName);
			if (fnNameToHndlrMapIter == mFunctionNameToHandlerMap.end())
			{
				continue;
			}

			DWORD_PTR fnEntryRVA = (DWORD_PTR)(inImageBase + firstThunkRVA);

			DWORD dwOldProtect;

			// Make the page writable and replace the original function
			// address with our hook function address
			if (!VirtualProtectEx(
						mCrashedProcessInfo.hProcess,
						(LPVOID)fnEntryRVA,
						sizeof(DWORD_PTR),
						PAGE_READWRITE,
						&dwOldProtect))
			{
				HandleError(
					GetLastError(),
					_T("Unable to set permissions at 0x%X in faulty process."),
					(fnEntryRVA + inImageBase));

				continue;
			}
			//
			// Allocate code here which does a jump on the old address,
			// then insert bp on the allocated address and set the IAT
			// entry to point to the allocated code
			//
			PATCH_CODE *patchCode = 
						(PATCH_CODE *)DebuggeeMemAlloc(
											sizeof(PATCH_CODE));

			//
			// We don't ever free this memory even if the patch couldn't get
			// applied. We do this because for 9x, we allocate process in
			// debuggee using memory mapped file
			//
			if (!patchCode)
			{
				HandleError(
					GetLastError(),
					_T("Failed to allocate shared memory."));

				continue;
			}

		#if defined(_M_IX86) || defined(_M_AMD64)

			patchCode->JMP[0]		= 0xFF;
			patchCode->JMP[1]		= 0x25;
			patchCode->pdwAddress	= (DWORD_PTR)&(patchCode->dwAddress);
			patchCode->dwAddress	= (DWORD_PTR)firstITD.u1.Function;

		#elif defined(_M_IA64)

			#error ("IA64 not implemented yet.")

		#else

			#error ("Uknown target machine type.")

		#endif

			if (!DebuggeeWriteMemory(
						(LPVOID)(fnEntryRVA),
						&patchCode,
						sizeof(DWORD_PTR)))
			{
				continue;
			}

			//
			// TODO: Make sure that patch code size is big enough to
			// accomodate the breakpoint code. On x86 and x86-64 the
			// breakpoint instruction is only 1 byte. But on IA64 it
			// is either 6 or 16 bytes
			//
			InsertBreakPoint(patchCode, fnName);
			
			DWORD dwTemp;
			VirtualProtectEx(
					mCrashedProcessInfo.hProcess,
					(LPVOID)fnEntryRVA,
					sizeof(DWORD_PTR),
					dwOldProtect,
					&dwTemp);
		}

		++pIID;
	}

	return true;

funcEnd:

	return false;
}
#endif


bool
CRecoverCrash::DebuggeeReadMemoryGreedy(
    LPCVOID	inAddress,
    LPVOID	ioBuffer,
    SIZE_T	inSize,
    SIZE_T	*oBytesRead)
/*++

Routine Description:

This function is used to read the memory of the debuggee process. If
memory is not read, it logs and error as well

Arguments:

Refer ReadProcessMemory

Return:

true - memory is read
false - memory couldn't be read due to some error

--*/
{
    SIZE_T tmpSize;
    bool retVal;

    retVal = false;
    tmpSize = inSize;

    while (tmpSize > 0)
    {
        if (ReadProcessMemory(
            mCrashedProcessInfo.hProcess,
            inAddress,
            ioBuffer,
            tmpSize--,
            oBytesRead))
        {
            retVal = true;
            break;
        }
    }

    if (!retVal)
    {
        HandleError(
            GetLastError(),
            _T("Unable to read memory at 0x%X in faulty process."),
            inAddress);
    }

    return retVal;
}
bool
CRecoverCrash::DebuggeeReadMemory(
	LPCVOID	inAddress,
	LPVOID	ioBuffer,
	SIZE_T	inSize,
	SIZE_T	*oBytesRead)
/*++

Routine Description:

	This function is used to read the memory of the debuggee process. If
	memory is not read, it logs and error as well

Arguments:

    Refer ReadProcessMemory

Return:

	true - memory is read
	false - memory couldn't be read due to some error

--*/
{
	if (!ReadProcessMemory(
				mCrashedProcessInfo.hProcess,
				inAddress,
				ioBuffer,
				inSize,
				oBytesRead))
	{
		HandleError(
				GetLastError(),
				_T("Unable to read memory at 0x%X in faulty process."),
				inAddress);

			return false;
	}

	return true;
}


bool
CRecoverCrash::DebuggeeWriteMemory(
	LPVOID	inAddress,
	LPVOID	inBuffer,
	SIZE_T	inSize,
	SIZE_T	*oBytesWrote)
/*++

Routine Description:

	This function is used to write the memory of the debuggee process. If
	memory is not written, it logs and error as well

Arguments:

    Refer WriteProcessMemory

Return:

	true - memory is written
	false - memory couldn't be written due to some error

--*/
{
	if (!WriteProcessMemory(
				mCrashedProcessInfo.hProcess,
				inAddress,
				inBuffer,
				inSize,
				oBytesWrote))
	{
		HandleError(
				GetLastError(),
				_T("Unable to write memory at 0x%X in faulty process."),
				inAddress);

			return false;
	}

	return true;
}


LPVOID
CRecoverCrash::DebuggeeMemAlloc(
	DWORD	inSize)
/*++

Routine Description:

	This function is used to allocate memory in the debuggee's address space.
	For NT et al, we can allocate memory in other process using VirtualAllocEx
	but that function doesn't exist for windows 9x. So for 9x we create a
	memory mapped file. A memory mapped file automatically gets mapped in all
	the running processes on windows 9x et al

Arguments:

    inSize - size in bytes to allocate

Return:

	LPVOID - address of newly allocated memory or NULL if it failed

--*/
{
    return VirtualAllocEx(
        mCrashedProcessInfo.hProcess,
        NULL,
        inSize,
        MEM_COMMIT,
        PAGE_EXECUTE_READWRITE);
}


bool
CRecoverCrash::ReadDllImageName(
	PBYTE		inImageBase,
	LPSTR		oImageName,
	DWORD		inImageNameLength)
/*++

Routine Description:

	Reads the name of the DLL image from the DLL exports for the crashed
	process

Arguments:

    inImageBase - Base address of the loaded DLL

	oImageName - Variable to receive the name of image

	inImageNameLength - Maximum number of characters that oImageName can store

Return:

	true - if the image name is read
	false - otherwise

--*/
{
	DWORD cbRead = 0;
	IMAGE_DOS_HEADER dh;

    if (!DebuggeeReadMemory(
			(LPVOID)inImageBase,
			&dh,
			sizeof(IMAGE_DOS_HEADER)))
	{
        return false;
    }

    if (dh.e_magic != IMAGE_DOS_SIGNATURE)
	{
        return false;
    }

    IMAGE_NT_HEADERS nh;

    if (!DebuggeeReadMemory(
				(LPVOID)(inImageBase + dh.e_lfanew),
				&nh,
				sizeof(IMAGE_NT_HEADERS)))
	{
        return false;
    }

	PIMAGE_DATA_DIRECTORY expDataDir = (PIMAGE_DATA_DIRECTORY)
		&nh.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];

    if (!expDataDir->VirtualAddress)
	{
        return false;
    }

    IMAGE_EXPORT_DIRECTORY expdir;

    if (!DebuggeeReadMemory(
				(LPVOID)(inImageBase + expDataDir->VirtualAddress),
				&expdir,
				sizeof(IMAGE_EXPORT_DIRECTORY)))
	{
            return false;
    }

    if (!DebuggeeReadMemory(
				(LPVOID)(inImageBase + expdir.Name),
				oImageName,
				inImageNameLength))
	{
		return false;
    }

	return true;
}


bool
CRecoverCrash::HandleException(
    DEBUG_EVENT &inDebugEvent)
/*++

Routine Description:

	This function handles exception in a process. It basically see what kind
	of exception has occured and accordingly decide what action to take

Arguments:

    inDebugEvent - Reference to corresponding debug event

Return:

	none

--*/
{
	switch (inDebugEvent.u.Exception.ExceptionRecord.ExceptionCode)
	{
		case EXCEPTION_BREAKPOINT:
		{
			return HandleBreakPoint(inDebugEvent);
		}
		case EXCEPTION_SINGLE_STEP:
		{
			return HandleSingleStep(inDebugEvent);
		}
		default:
		{
			return HandleFatalException(inDebugEvent);
		}
	}

	return false;
}


bool
CRecoverCrash::HandleBreakPoint(
    DEBUG_EVENT &inDebugEvent)
/*++

Routine Description:

	This function handles the breakpoint exception in the faulting process.

	We will insert breakpoints in some important functions like CreateFileW
	and CreateFileA such that if faulting process calls these functions, we
	create a backup copy of the file it is trying to open.

	If the exception occured due to our inserted breakpoint, we	will suspend
	all the running threads of faulting process. Then we would read arguments
	of the function , remove the breakpoint and replace it with the original
	instruction there, execute the thread for single step (by setting
	appropriate debug registers). After	executing the instruciton, we will
	get an EXCEPTION_SINGLE_STEP and the HandleSingleStep function will be
	called. See HandleSingleStep comments for further processing details.

Arguments:

    inDebugEvent - Reference to corresponding debug event

Return:

	none

--*/
{
	BP_INFO				bpInfo;
	HANDLE				hThread = NULL;
	THREAD_LIST_ITER	threadIter;
	CONTEXT				context;

	LPVOID exceptionAddress = 
			inDebugEvent.u.Exception.ExceptionRecord.ExceptionAddress;

	ADDR_TO_BP_MAP::iterator addrToBpMapIter = 
								mAddrToBpInfoMap.find(exceptionAddress);

	//
	// If an entry for this breakpoint is not found or if the original
	// instruction is a breakpoint as well
	//
	if (addrToBpMapIter == mAddrToBpInfoMap.end() ||
		0xCC			== addrToBpMapIter->second.OriginalByte)
	{
		goto funcEnd;
	}

	bpInfo = addrToBpMapIter->second;
	
	for(	threadIter = mCrashedProcessInfo.threadList.begin();
			threadIter != mCrashedProcessInfo.threadList.end();
			++threadIter)
	{
		CD_THREAD_INFO threadInfo = *threadIter;
		if (threadInfo.threadId == inDebugEvent.dwThreadId)
		{
			hThread = threadInfo.hThread;
			break;
		}
	}

	if (hThread == NULL)
	{
		goto funcEnd;
	}
	
	context.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
	GetThreadContext(hThread, &context);

	DebuggeeWriteMemory(
				exceptionAddress,
				&bpInfo.OriginalByte,
				sizeof(bpInfo.OriginalByte));

	// Call the handler function if any for this breakpoint
	{
		FUNCTION_NAME_TO_HANDLER_MAP::iterator fnNameToHndlrMapIter;

		fnNameToHndlrMapIter = 
						mFunctionNameToHandlerMap.find(bpInfo.FunctionName);

		if (fnNameToHndlrMapIter != mFunctionNameToHandlerMap.end())
		{
			PFN_FunctionHandler functionHandler = fnNameToHndlrMapIter->second;
			(this->*functionHandler)(context);
		}
	}

	//
	// Put the exception address on the thread stack so that when the single
	// step exception is hit, we can retrieve this address to reinsert the
	// breakpoint
	//
	ADDR_STACK &threadAddressStack = 
							mThreadToAddrStackMap[inDebugEvent.dwThreadId];

	threadAddressStack.push(exceptionAddress);

	//
	// We replaced our breakpoint with original instructions. Now set
	// appropriate registers to make sure that we execute only one instruction
	// and break again with SINGLE_STEP_EXCEPTION. In single step handler, we
	// will restore our breakpoint. Also move program counter back by the size
	// of breakpoint instruction (which is 1 for x86/x86-64 and (??) for IA64),
	// so that original instruction can be executed as if there was no
	// breakpoint
	//
#if defined(_M_IX86)

	context.EFlags |= 0x100;
	context.Eip -= 1;

#elif defined(_M_AMD64)

	context.EFlags |= 0x100;
	context.Rip -= 1;

#else

	#error ("Uknown target machine type.")

#endif

	SetThreadContext(hThread, &context);

funcEnd:

	return true;
}


bool
CRecoverCrash::HandleSingleStep(
    DEBUG_EVENT &inDebugEvent)
/*++

Routine Description:

	If this single step event occured because of out breakpoint handling then
	we will reinsert the removed breakpoint, resume all threads and continue
	exceution of the process

Arguments:

    inDebugEvent - Reference to corresponding debug event

Return:

	none

--*/
{
	THREAD_TO_ADDR_STACK_MAP::iterator threadAddrStackIter = 
						mThreadToAddrStackMap.find(inDebugEvent.dwThreadId);

	if (threadAddrStackIter == mThreadToAddrStackMap.end())
	{
		goto funcEnd;
	}

	ADDR_STACK &addrStack = threadAddrStackIter->second;

	if (addrStack.empty())
	{
		goto funcEnd;
	}

	PVOID addr = addrStack.top();

	ReInsertBreakPoint(addr);

	addrStack.pop();

funcEnd:

	return true;
}


bool
CRecoverCrash::HandleFatalException(
    DEBUG_EVENT &inDebugEvent)
/*++

Routine Description:

	This function handles exception in a process. It its a first chance
	excpetion then we don't handle it. If its a second chance exception
	then we try to revert it because a second chance exception is fatal
	and terminates the process if not handled.

	Note here that we handles all the exceptions in this process itself
	except EXCEPTION_BREAKPOINT and EXCEPTION_SINGLE_STEP

Arguments:

    inDebugEvent - Reference to corresponding debug event

Return:

	none

--*/
{
    if (inDebugEvent.u.Exception.dwFirstChance)
    {
		mRecoveryHandler->PrintWarning(
			_T("Process generated a first-chance exception at address: 0x%X\n"),
			inDebugEvent.u.Exception.ExceptionRecord.ExceptionAddress);

        return false;
    }
    else
    {
		mRecoveryHandler->PrintError(
			_T("Process generated a second-chance exception at address: 0x%X\n"),
			inDebugEvent.u.Exception.ExceptionRecord.ExceptionAddress);

		mRecoveryHandler->PrintMessage(
			_T("CrashDoctor will now attempt to recover your process\n"));

        //
        // This is a second chance exception, it will
        // cause the process to terminate, so we need
        // to handle it
        //

        //
        // Increment the crash count. After certain
        // number of crashes, we should think about
        // terminating the process or generate a core
        // dump etc.
        //
        ++mCountCrash;
        
        return Recover(inDebugEvent.dwThreadId);
    }
}


bool
CRecoverCrash::RecoverSkipInstruction(CONTEXT *Context)
{
    char instr[32];
    SIZE_T	bytesRead = 0;

#if defined(_M_IX86)
    if (DebuggeeReadMemoryGreedy(
            (LPCVOID)Context->Eip,
            instr,
            sizeof(instr),
            &bytesRead))
    {
        int len = LDE(instr, 0);
        mRecoveryHandler->PrintMessage(
            _T("Faulting Instruction Address: 0x%X, Length: %d.\n"),
            Context->Eip, len);
        Context->Eip += len;
        return true;
    }
#else
    if (DebuggeeReadMemoryGreedy(
            (LPCVOID)Context->Rip,
            instr,
            sizeof(instr),
            &bytesRead))
    {
        int len = LDE(instr, 64);
        mRecoveryHandler->PrintMessage(
            _T("Faulting Instruction Address: 0x%X, Length: %d.\n"),
            Context->Rip, len);
        Context->Rip += len;
        return true;
    }
#endif

    return false;
}


bool
CRecoverCrash::RecoverSkipFunction(HANDLE ThreadHandle, CONTEXT *Context)
{
	STACKFRAME64 stackFrame;
	bool crashRecovered = false;
    DWORD machineType;

    if (mSymbolInitialized)
    {
        #if defined(_M_IX86)
            machineType						= IMAGE_FILE_MACHINE_I386;
            stackFrame.AddrPC.Offset		= Context->Eip;
            stackFrame.AddrPC.Mode			= AddrModeFlat;
            stackFrame.AddrStack.Offset		= Context->Esp;
            stackFrame.AddrStack.Mode		= AddrModeFlat;
            stackFrame.AddrFrame.Offset		= Context->Ebp;
            stackFrame.AddrFrame.Mode		= AddrModeFlat;
        #elif defined(_M_AMD64)
            machineType						= IMAGE_FILE_MACHINE_AMD64;
            stackFrame.AddrPC.Offset		= Context->Rip;
            stackFrame.AddrPC.Mode			= AddrModeFlat;
            stackFrame.AddrStack.Offset		= Context->Rsp;
            stackFrame.AddrStack.Mode		= AddrModeFlat;
            stackFrame.AddrFrame.Offset		= Context->Rbp;
            stackFrame.AddrFrame.Mode		= AddrModeFlat;
        #endif

        CONTEXT	contextTemp = *Context;

        if (StackWalk64(
                    machineType,
                    mCrashedProcessInfo.hProcess,
                    ThreadHandle,
                    &stackFrame,
                    &contextTemp,
                    NULL,
                    SymFunctionTableAccess64,
                    SymGetModuleBase64,
                    NULL))
        {											
            if (StackWalk64(
                        machineType,
                        mCrashedProcessInfo.hProcess,
                        ThreadHandle,
                        &stackFrame,
                        &contextTemp,
                        NULL,
                        SymFunctionTableAccess64,
                        SymGetModuleBase64,
                        NULL))
            {
                crashRecovered = true;

                #if defined(_M_IX86)

                    mRecoveryHandler->PrintMessage(
                        _T("Moving execution control from 0x%X->0x%X\n"),
                        Context->Eip, stackFrame.AddrPC.Offset);

                    Context->Esp = (DWORD)stackFrame.AddrStack.Offset;
                    Context->Ebp = (DWORD)stackFrame.AddrFrame.Offset;
                    Context->Eip = (DWORD)stackFrame.AddrPC.Offset;
				    Context->Eax = 0xFFFFFFFF;

                #elif defined(_M_AMD64)

                    mRecoveryHandler->PrintMessage(
                        _T("Moving execution control from 0x%X->0x%X\n"),
                        Context->Rip, stackFrame.AddrPC.Offset);

                    Context->Rsp = stackFrame.AddrStack.Offset;
                    Context->Rbp = stackFrame.AddrFrame.Offset;
                    Context->Rip = stackFrame.AddrPC.Offset;
				    Context->Rax = 0xFFFFFFFFFFFFFFFF;

                #else

                    #error ("Uknown target machine type.")

                #endif
            }
        }
    }

    return crashRecovered;
}


bool
CRecoverCrash::RecoverSkipFunctionHomebrew(CONTEXT *Context)
{
    bool crashRecovered = false;

#if defined(_M_IX86)
    DWORD	EbpEip[2];
    SIZE_T	bytesRead = 0;

    //
    // Esp value is set as to further protect the crashing
    // thread. We try to make sure that when we bypass
    // the faulting function, we restore the stack for the
    // previous function. This restoration is not perfect
    // because we don't know how the stack is manipulated
    // by the function when it crashes. Also if __stdcall
    // calling convention is used, then parameters are popped
    // of by callee but because we don't know that, we can't
    // remove paramters off the stack.
    //
    // Esp = saved Ebp + 8
    // 8 bytes are due to Return value and saved Ebp
    //
    Context->Esp = Context->Ebp + 8;

    if (DebuggeeReadMemory(
                        (LPCVOID)Context->Ebp,
                        EbpEip,
                        sizeof(EbpEip),
                        &bytesRead))
    {
        if (bytesRead == sizeof(EbpEip))
        {
            mRecoveryHandler->PrintMessage(
                _T("Moving execution control from 0x%X->0x%X.\n"),
                Context->Eip, EbpEip[1]);

            Context->Ebp = EbpEip[0];
            Context->Eip = EbpEip[1];
            Context->Eax = 0xFFFFFFFF;

            crashRecovered = true;
        }
        else
        {
            HandleError(
                GetLastError(),
                _T("Unable to read memory at 0x%X in faulty process."),
                Context->Ebp);
        }
    }
#elif defined(_M_AMD64)

    // Nothing to do, generating stack on AMD64 is hard work and
    // is left to dbghelp. Without dbghelp, its better to let the
    // process die.
    crashRecovered = false;

#else

    #error ("Uknown target machine type.")

#endif

    return crashRecovered;
}


bool
CRecoverCrash::Recover(
	DWORD	inThreadId)
/*++

Routine Description:
	
	This function tries to generate a stack trace of the crashed program such
    that it can bypass the crash and jumps out of the called function to the
    calling function.

	It does so by unwinding the stack one call at a time and comparing the
    return address with the	crashed process's loaded address. If an address
	falls in the process address, it sets that EIP and corresponding EBP and
    change the crashed thread's context.

	During this unwinding, there may be case when it can't find a valid address
    to return too, due to stack corruption or exception occured in functions
	like main. In such cases, it set the ContinueStatus	to 
    DBG_EXCEPTION_NOT_HANDLED, which terminates	the process.
    
Arguments:

    inThreadId - ID of the thread which generated exception
    
    inRevertNCalls - How many calls should we revert in the call chain? 
        Basically when a thread crashes, we start unwinding the call chain and
        this variable tells us how many upper level calling functions to bypass
        E.g. If inRevertNCalls = 2 and Call stack is foo->foo1->foo2->foo3 then
        we will bypass the execution of foo2 and foo3 and return to the next
        instruction where foo1 calls foo2 (Do i make sense here?)
        
Return:

	none

--*/
{
	HANDLE hThread = NULL;
	THREAD_LIST_ITER threadIter;
    bool crashRecovered = false;
	CONTEXT context;

	
	for(threadIter = mCrashedProcessInfo.threadList.begin();
		threadIter != mCrashedProcessInfo.threadList.end();
		++threadIter)
	{
		CD_THREAD_INFO threadInfo = *threadIter;
		if (threadInfo.threadId == inThreadId)
		{
			hThread = threadInfo.hThread;
			break;
		}
	}

	context.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
	
	if (hThread != NULL)
	{
		GetThreadContext(hThread, &context);

        crashRecovered = RecoverSkipInstruction(&context);
        if (!crashRecovered)
        {
            crashRecovered = RecoverSkipFunction(hThread, &context);
            if (!crashRecovered)
            {
                crashRecovered = RecoverSkipFunctionHomebrew(&context);
            }
        }
    }

    if (crashRecovered)
    {
        SetThreadContext(hThread, &context);
    }
	else
	{
		mRecoveryHandler->PrintError(
				_T("CrashDoctor was unable to recover fault.\n"));
	}

	mCrashRecovered = crashRecovered;
    return crashRecovered;
}


void
CRecoverCrash::AttachToProcess(
	DWORD				processId,
	HANDLE				hEvent,
	CRecoveryHandler	*pRecoveryHandler)
/*++

Routine Description:
	
	Our mini-debugger implementation. It does following
	things:

	- Attach to a running process

	- On first breakpoint, Set the event to indicate that
	  debugger is attached successfully

	- If a second chance exception occurs, then
		- Backup all opened file (This may not be reqd)
		- Put break point in CreateFile to backup any files 
		  that the program open subsequently
		- Bypass the exception

Arguments:

    none

Return:

	none

--*/
{
	// Assign the recover handler
	mRecoveryHandler = pRecoveryHandler;

    mRecoveryHandler->PrintTitle(
        _T("\nIntellectualHeaven (R) CrashDoctor for XP, 2K3, Vista, Windows 7 and 8.\n"));

	mRecoveryHandler->PrintTitle(
			_T("Copyright (c) Pankaj Garg. All rights reserved.\n\n"));

	// Obtain the SE_DEBUG privilege so that we can attach to win32 service
	// as well as processes
	cdObtainSeDebugPrivilege();

	// This variable will be used to store symbol related information
    CD_SYMBOL_PROCESS_INFO symProcInfo;
	symProcInfo.SymbolInitialized = false;

	mCrashedProcessInfo.notInitialized = true;

	if (!DebugActiveProcess(processId))
	{
		HandleError(
				GetLastError(),
				_T("Failed to attach to the process (PID = %d)."),
				processId);

		goto funcExit;
	}

	mRecoveryHandler->PrintMessage(
			_T("CrashDoctor is now watching your process\n"));

#if DETACH_DEBUGGER_ON_EXIT

    DebugSetProcessKillOnExit(FALSE);

#endif


	DEBUG_EVENT debugEvent;
	DWORD		dwContinueStatus = DBG_CONTINUE;

	mKeepAlive = true;

	//
	// countCrash should be used to determine if we should continue to try to
    // recover a process from crash or not. If countCrash hits a limit (5), we
    // should terminate the process.
	//
    // Other things that we can do to recover crash are:
	// On each incremental crash we can take more aggressive approach to
    // recover. If there are multiple threads in a process we can even
    // terminate the faulting thread if it is not recovering. Terminating
    // crashing thread approach sounds too extreme but it is not, because if we
    // don't do anything, it will anyways end up crashing
	//
	mCountCrash = 0;

	//
	// processAttached is used to see if we successfully attached
	// to the target process or not
	//
	bool processAttached = false;

	while(mKeepAlive)
	{
		// Loader lock, if the target process is holding the loader lock then
        // WaitForDebugEvent will deadlock. In this case we basically will
        // generate a synthetic debug event which will continue the exeuction
        // of target process and generate Create process event
		DWORD waitTime = 0;

		if (!processAttached)
		{
			waitTime = 20000;	// 20 seconds
		}
		else
		{
			waitTime = 2000;	// 2 second
		}

		if (!WaitForDebugEvent(&debugEvent, waitTime))
		{
			if (!processAttached)
			{
    			// Synthesize a breakpoint event if we are not yet attached to
                // the process
                debugEvent.dwProcessId      = processId;

				debugEvent.dwDebugEventCode = EXCEPTION_DEBUG_EVENT;
                
				debugEvent.u.Exception.ExceptionRecord.ExceptionCode =
                                                        EXCEPTION_BREAKPOINT;
			}
			else
			{
                // Process is already attached to the debugger, it seems it is
                // not generating any debug events. If mCrashRecovered is true
				// that means we successfully recovered a crash because no other
				// exception got generated in last 1 second
				if (mCrashRecovered)
				{
					mCrashRecovered = false;
					mRecoveryHandler->PrintMessage(
						_T("CrashDoctor successfully recovered your process ")
						_T("from crash.\n"));
				}

				// Loop back and wait for debug events
				continue;
			}
		}

		dwContinueStatus = DBG_CONTINUE;

		if (processId == debugEvent.dwProcessId)
		{
			switch (debugEvent.dwDebugEventCode)
			{
				case EXCEPTION_DEBUG_EVENT:
				{
					if (debugEvent.u.Exception.ExceptionRecord.ExceptionCode ==
														EXCEPTION_BREAKPOINT)
					{
						if (hEvent)
						{
							SetEvent(hEvent);							
							hEvent          = NULL;
							processAttached = true;
						}
					}

					if (!HandleException(debugEvent))
					{
                        dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
                    }

					break;
				}
				case CREATE_THREAD_DEBUG_EVENT:
				{
                    HandleCreateThread(debugEvent);
					break;
				}
				case CREATE_PROCESS_DEBUG_EVENT:
				{
                    HandleCreateProcess(debugEvent, symProcInfo);
					break;
				}
				case EXIT_THREAD_DEBUG_EVENT:
				{
                    HandleExitThread(debugEvent);
					break;
				}
				case EXIT_PROCESS_DEBUG_EVENT:
				{
					HandleExitProcess(debugEvent);
					break;
				}
				case LOAD_DLL_DEBUG_EVENT:
				{
					HandleLoadDll(debugEvent);
					break;
				}
				case UNLOAD_DLL_DEBUG_EVENT:
				{
                    // Remove breakpoint insertion info if kernel32 is unloaded
					break;
				}
				case OUTPUT_DEBUG_STRING_EVENT:
				{
					HandleOutputDebugString(debugEvent);
					break;
				}
			}
		}

		ContinueDebugEvent(	
                        debugEvent.dwProcessId,
					    debugEvent.dwThreadId,
					    dwContinueStatus);
	}

    DeinitializeSymbolHelper(symProcInfo);

funcExit:
	return;
}


void
CRecoverCrash::InitializeSymbolHelper(
    CD_SYMBOL_PROCESS_INFO &ioSymProcInfo)
/*++

Routine Description:

	This function tries to initialize the symbol support provided by
	dbghelp.dll for the faulting process. This support provides	efficient
	stack generation and other useful features. The problem with the
	intiialization is though that, if the process is holding loader lock
	this deadlocks. To avoid the deadlock, i would run the symbol
	initialization code in a thread and if the thread doesn't finish in
	specified time, i will terminate it and set variable to indicate
	no symbol support.

Arguments:

    ioSymProcInfo - Variable which contains information about the process
		for which symbol support should be initialized. On return it contains
		the status for symbol initialization.

Return:

	none

--*/
{
    DWORD       threadId 	= 0;
    HANDLE      hThread		= NULL;

    hThread = CreateThread(
                        NULL,
                        0,
                        cdSymInitializeThread,
                        &ioSymProcInfo,
                        0,
                        &threadId);

    if (hThread)
    {
        if (WaitForSingleObject(hThread, 5000) == WAIT_TIMEOUT)
        {
            TerminateThread(hThread, 0);
        }
    }

    mSymbolInitialized = ioSymProcInfo.SymbolInitialized;
}


void
CRecoverCrash::DeinitializeSymbolHelper(
    CD_SYMBOL_PROCESS_INFO &ioSymProcInfo)
/*++

Routine Description:

	Reverse what is done in InitializeSymbolHelper

Arguments:

    ioSymProcInfo - Variable which contains information about the process
		for which symbol support should be deinitialized

Return:

	none

--*/
{
    if (ioSymProcInfo.SymbolInitialized)
	{
        SymCleanup(ioSymProcInfo.ProcessHandle);
	}

    ioSymProcInfo.ProcessHandle = NULL;
}


DWORD
WINAPI
cdSymInitializeThread(
    LPVOID inParam)
/*++

Routine Description:
	
	This function initializes the symbol support for our process. These 
    functions are exported in dbghelp.dll.

Return:

	true - Symbol support is available and initialized
    false - No symbol support

--*/
{
    PCD_SYMBOL_PROCESS_INFO pSymProcInfo = (PCD_SYMBOL_PROCESS_INFO)inParam;

    pSymProcInfo->SymbolInitialized =
        SymInitialize(pSymProcInfo->ProcessHandle, "", TRUE);

    return 0;
}


