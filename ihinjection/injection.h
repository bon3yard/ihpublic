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

    injection.h

Module Description:

	Declares data types, structure and functions to implement
	DLL injection in another process.

--*/

#ifndef _INJDLL_H_
#define _INJDLL_H_

//
// Application specific error codes
//
#define ERR_INJDLL_ERROR_BASE		0x20001000
#define ERR_PROCESS_NOT_FOUND		(ERR_INJDLL_ERROR_BASE + 1)
#define ERR_INVALID_PROCESS_ID		(ERR_INJDLL_ERROR_BASE + 2)

#define MAX_INC_EXC_SIZE			5120
#define MAX_FN_NAME_LENGTH			64

typedef struct _INJECTION_DATA
{
	// Required to inject DLL
	LPVOID			mLoadLibraryW;
	LPVOID			mGetProcAddr;
	LPVOID			mFreeLibrary;
	LPVOID			mGetModuleHandleW;
	LPVOID			mDeleteFileW;
	LPVOID			mSleep;

	// Full path of DLL
	WCHAR	mDllName[MAX_PATH];

	// Injector DLL's initialize function
	CHAR    mInitFnName[MAX_FN_NAME_LENGTH];

	// Injector DLL's de-initialize function
	CHAR	mDeInitFnName[MAX_FN_NAME_LENGTH];

	// Injector DLL's Thread reference count function
	CHAR	mThreadUsageFnName[MAX_FN_NAME_LENGTH];

	// Injector's Init function parameters
	CHAR	mFnIncludes[MAX_INC_EXC_SIZE];
	CHAR	mFnExcludes[MAX_INC_EXC_SIZE];

}INJECTION_DATA, *PINJECTION_DATA;

// LoadLibraryW typedef
typedef HINSTANCE (WINAPI *PFNLOADLIBRARY)(LPCWSTR);

// FreeLibrary typedef
typedef HINSTANCE (WINAPI *PFNFREELIBRARY)(HMODULE);

// GetModuleHandle typedef
typedef HMODULE (WINAPI *PFNGETMODULEHANDLE)(LPCWSTR);

// GetProcAddress typedef
typedef FARPROC (WINAPI *PFNGETPROCADDRESS)(HMODULE,
											LPCSTR);

// DeleteFileW typedef
typedef BOOL (WINAPI *PFNDELETEFILE)(LPCWSTR);

// Sleep typedef
typedef void (WINAPI *PFNSLEEP)(DWORD);

// DLL's Initiate Patching function prototype
typedef void (WINAPI *PFNSERUMLOAD)(LPCSTR, LPCSTR);

// DLL's Deinit patching function typedef
typedef void (WINAPI *PFNSERUMUNLOAD)(void);

// DLL's thread usage count function typedef
typedef volatile LONG (WINAPI *PFNSERUMGETREFCOUNT)(void);


DWORD
WINAPI
ihiGetProcessIdByName(
	LPCWSTR	inProcessName);


DWORD
WINAPI
ihiLaunchNewProcess(
	LPCWSTR	inExePath);


bool
ihiInitInjectionData(
	PINJECTION_DATA 	injData,
	LPCWSTR		inDllPath,
	LPCSTR			inFnIncludes,
	LPCSTR			inFnExcludes);


bool
WINAPI
ihiInjectDll(
	HANDLE			hProcess,	
	LPCWSTR	inDllPath,
	LPCSTR		inFnIncludes,
	LPCSTR		inFnExcludes);

bool
WINAPI
ihiUninjectDll(
	HANDLE			hProcess,
	LPCWSTR	inDllPath);

static
void
ihiInjectedCode(
	LPVOID *inAddress);

static
void
ihiInjectedCodeEnd();

static
void
ihiUnloadCode(
	LPVOID *inAddress);

static
void
ihiUnloadCodeEnd();

#endif