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

#ifndef _CDUTIL_H_
#define _CDUTIL_H_

//System-Includes!!!
#include <string>

#define ASSERT

//
// Error code to indicate that CrashDoctor is invoked on invalid process
//
#define ERR_INVALID_PROCESS_ID	0xFFFFFF


//
// ANSI Unicode independent string
//
#ifdef _UNICODE
typedef std::wstring	tstring;
#else
typedef std::string		tstring;
#endif


#ifdef _UNICODE
#define CommandLineToArgv CommandLineToArgvW
#else
#define CommandLineToArgv CommandLineToArgvA
LPSTR * CommandLineToArgvA(LPCSTR lpCmdLine, int*pNumArgs);
#endif


// Function typedef to convert a character to uppercase
typedef TCHAR (__cdecl *TO_UPPER)(TCHAR);


void
__cdecl
cdShowMessage(
	HWND	inHwnd,
	LPCTSTR	inErrorMsg = NULL,
	...);

void
__cdecl
cdHandleError(
	HWND	inHwnd,
	DWORD	inErrorCode = 0,
	LPCTSTR	inErrorMsg = NULL,
	...);


// Function to convert a char to uppercase
TCHAR
__cdecl
cdToUpper(TCHAR c);


// Function to obtain SE_DEBUG privilege
BOOL
cdObtainSeDebugPrivilege(void);


void
CenterDialog(
	HWND hwndDlg);

HRESULT
CreateLink(
	LPCTSTR lpszPathObj,
	LPCTSTR lpszPathLink,
	LPCTSTR lpszDesc);



#endif