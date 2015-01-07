/*++

Copyright (c) : Pankaj Garg & www.intellectualheaven.com

Module Name:

    cdui.h

Module Description:

	Refer to cdui.cpp

Author:

    Pankaj Garg     2005-03-17
    
Most recent update:

    Pankaj Garg     2005-03-18

--*/
#ifndef _CDUI_H_
#define _CDUI_H_

//System-Includes!!!
#include <windows.h>
#include "ihulib.h"


INT_PTR
CALLBACK
HandleCrashDlgProc(
	HWND	hDlg,
	UINT	msg,
	WPARAM	wParam,
	LPARAM	lParam);


INT_PTR
CALLBACK
RecoveryStatusDlgProc(
	HWND	hDlg,
	UINT	msg,
	WPARAM	wParam,
	LPARAM	lParam);



//
// Various defines for recovery class handling
//
#define PRINT_TITLE			1
#define PRINT_MSG			2
#define PRINT_INFO			3
#define PRINT_TRACE			4
#define PRINT_WARN			5
#define PRINT_ERROR			6

#define PrintTitle		PrintTitleW
#define PrintMessage	PrintMessageW
#define PrintInfo		PrintInfoW
#define PrintTrace		PrintTraceW
#define PrintWarning	PrintWarningW
#define PrintError		PrintErrorW

//
// Class to manage recovery of a faulting process. This class mainly
// implements the UI for recovery management and the core recovery work
// is delegated to CRecoverCrash
//
class CRecoveryHandler
{
public:
	CRecoveryHandler(
		DWORD	inProcessId,
		HANDLE	inEventHandle);

	~CRecoveryHandler();

	void InitInstance(HWND inDialogHwnd, HWND inListCtrlHwnd);
	void ExitInstance();
	void SignalRecoveryThreadExit();

	void StartRecovery();

	INT_PTR HandleCustomDraw(
		LPNMLVCUSTOMDRAW pCD);

	void PrintTitleW(LPCWSTR inFormat, ...);
	void PrintMessageW(LPCWSTR inFormat, ...);
	void PrintInfoW(LPCWSTR inFormat, ...);
	void PrintTraceW(LPCWSTR inFormat, ...);
	void PrintWarningW(LPCWSTR inFormat, ...);
	void PrintErrorW(LPCWSTR inFormat, ...);

private:
	
	void PrintW(
			DWORD inPrintWhat,
			LPCWSTR inFormat,
			va_list inArgList);

	// Dialog handle and other dialog control handles
	HWND	m_hwndDlg;
	HWND	m_hwndListCtrl;

	// Font handles
	HFONT	m_TraceListFont;
	HFONT	m_TraceListFontBold;

	// recovery thread handle and Id
	HANDLE	mThreadHandle;
	DWORD	mThreadId;

	// Process Id of faulting process
	DWORD	m_ProcessId;

	// Associated event handle
	HANDLE	m_hEvent;
};

#endif
