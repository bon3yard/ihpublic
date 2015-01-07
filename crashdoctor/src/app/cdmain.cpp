/*++

Copyright (c) : Pankaj Garg & www.intellectualheaven.com

Module Name:

    cdmain.cpp

Module Description:

	Crash Doctor file which contains WinMain

Author:

    Pankaj Garg		2004-09-17
    
Most recent update:

    Pankaj Garg     2005-03-18

--*/

// System Include Files!!!
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <Shellapi.h>
#include <tchar.h>

// Local Include Files!!!
#include "cdmain.h"
#include "cdutil.h"
#include "cdinstall.h"
#include "cdui.h"
#include "resource.h"
#include "cdcore.h"



//
// Global application instance
//
HINSTANCE ghInstance;


//
// Global OS Version
//
OS_VERSION			gOSVersion;
OS_EXACT_VERSION	gOSExactVersion;




int 
WINAPI
WinMain(
	HINSTANCE	hInstance,
    HINSTANCE	hPrevInstance,
    PSTR		szCmdLine,
    int			iCmdShow)
/*++

Routine Description:
	
	Entry point for the application

Returns:

	An exit code of application - we always return 0
	

--*/
{
	int funcResult = 0;

	// Common controls must be initialized
    InitCommonControls();

	ghInstance = hInstance;

	//
	// Set the global variable for version information
	//
	OSVERSIONINFO versionInfo;
	versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&versionInfo);

	if (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
	{
		gOSVersion = WIN32_9X;

		if (versionInfo.dwMinorVersion == 0)
		{
			gOSExactVersion	= WIN32_95;
		}
		else if (versionInfo.dwMinorVersion == 10)
		{
			gOSExactVersion	= WIN32_98;
		}
		else if (versionInfo.dwMinorVersion == 90)
		{
			gOSExactVersion	= WIN32_ME;
		}
	}
	else
	{
		gOSVersion		= WIN32_NT;

		if (versionInfo.dwMajorVersion == 4)
		{
			gOSExactVersion	= WIN32_NT4;
		}
		else if (versionInfo.dwMajorVersion == 5)
		{
			if (versionInfo.dwMinorVersion == 0)
			{
				gOSExactVersion	= WIN32_2K;
			}
			else if (versionInfo.dwMinorVersion == 1)
			{
				gOSExactVersion	= WIN32_XP;
			}
			else if (versionInfo.dwMinorVersion == 2)
			{
				gOSExactVersion	= WIN32_2K3;
			}
		}
	}

	LPTSTR cmdLine = GetCommandLine();
	int argC = 0;
	LPTSTR *argV = CommandLineToArgv(cmdLine, &argC);

	if (argC <= 1)
	{
		HWND hwnd = GetDesktopWindow();
		int err = GetLastError();
		int n =
		DialogBoxParam(
				ghInstance,
				MAKEINTRESOURCE(IDD_DIALOG_INSTALL),
				GetDesktopWindow(),
				(DLGPROC)InstallDlgProc,
				(LPARAM)NULL);

		err = GetLastError();
		err = err + 1;
	}
	else
	{
		DWORD pid		= 0;
		HANDLE hEvent	= NULL;

		if (cdProcessArguments(argC, argV, pid, hEvent))
		{
			PROC_DBG_DATA procDbgData;

			procDbgData.processId	= pid;
			procDbgData.eventHandle	= hEvent;

			int dlgResult = DialogBoxParam(
								ghInstance,
								MAKEINTRESOURCE(IDD_DIALOG_HANDLE_CRASH),
								GetDesktopWindow(),
								(DLGPROC)HandleCrashDlgProc,
								(LPARAM)&procDbgData);

			if (dlgResult == IDC_BTN_RECOVER)
			{
				CRecoveryHandler recoveryHandler(
										procDbgData.processId,
										procDbgData.eventHandle);

				int tempResult = DialogBoxParam(
									ghInstance,
									MAKEINTRESOURCE(IDD_DIALOG_RECOVER_STATUS),
									GetDesktopWindow(),
									(DLGPROC)RecoveryStatusDlgProc,
									(LPARAM)&recoveryHandler);
				
			} 
			else if(dlgResult == IDC_BTN_TERMINATE)
			{
				HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

				if (hProcess)
				{
					TerminateProcess(hProcess, -1);
					CloseHandle(hProcess);
				}

				if (hEvent)
				{
					SetEvent(hEvent);
					hEvent = NULL;
				}
			}
		}
	}

	goto funcExit;

funcExit:
	return funcResult;
}



bool
cdProcessArguments(
	int			argC,
	TCHAR		*argV[],
	DWORD		&oPID,
	HANDLE		&ohEvent)
/*++

Routine Description:
	
	Process command line arguments for CrashDoctor

Return:

	true means valid arguments are supplied, false means otherwise

--*/
{
	bool		funcReturn = true;
	tstring		userParam;

	oPID	= 0;
	ohEvent = NULL;

	// We start with index 1 because 0 is the process name
	// itself
	for (int indexArgs = 1; indexArgs < argC; ++indexArgs)
	{
		if (_tcsicmp(argV[indexArgs], _T("-p")) == 0 ||
			_tcsicmp(argV[indexArgs], _T("/p")) == 0)
		{
			if (indexArgs == (argC - 1))
			{
				funcReturn = false;
				cdShowMessage(NULL, _T("Process Id (PID) is *NOT* specified."));
				goto funcExit;
			}

			oPID = _ttoi(argV[++indexArgs]);
		}
		else if (	_tcsicmp(argV[indexArgs], _T("-e")) == 0 ||
					_tcsicmp(argV[indexArgs], _T("/e")) == 0)
		{
			if (indexArgs == (argC - 1))
			{
				funcReturn = false;
				cdShowMessage(NULL, _T("Event Id is *NOT* specified."));
				goto funcExit;
			}

			#if defined(_WIN64)
				ohEvent = (HANDLE)_ttoi64(argV[++indexArgs]);
			#else
				ohEvent = (HANDLE)_ttoi(argV[++indexArgs]);
			#endif
		}		
		else
		{
			funcReturn = false;
			cdShowMessage(NULL, _T("Unknown option specified."));
			goto funcExit;
		}
	}

	if (oPID == 0)
	{
		funcReturn = false;
		cdShowMessage(NULL, _T("Process ID is not specified."));
		goto funcExit;
	}

funcExit:

	return funcReturn;
}
