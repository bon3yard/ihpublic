/*++

Copyright (c) : Pankaj Garg & www.intellectualheaven.com

Module Name:

    cdutil.h

Module Description:

	Common utility functions required for CrashDoctor

Author:

    Pankaj Garg		2004-09-26
    
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