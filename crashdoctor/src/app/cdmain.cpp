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

// System Include Files!!!
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <Shellapi.h>
#include <tchar.h>

// Local Include Files!!!
#include "cdmain.h"
#include "cdutil.h"
#include "cdui.h"
#include "resource.h"
#include "cdcore.h"


//
// Global application instance
//
HINSTANCE ghInstance;


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

	LPTSTR cmdLine = GetCommandLine();
	int argC = 0;
	LPTSTR *argV = CommandLineToArgv(cmdLine, &argC);

	if (argC <= 1)
	{
		HWND hwnd = GetDesktopWindow();
		int err = GetLastError();

		DialogBoxParam(
				ghInstance,
				MAKEINTRESOURCE(IDD_DIALOG_ABOUT),
				GetDesktopWindow(),
				(DLGPROC)AboutDlgProc,
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

			INT_PTR dlgResult = DialogBoxParam(
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

				DialogBoxParam(
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
#if 0
            else if (dlgResult = IDC_BTN_ABOUT)
            {
                DialogBoxParam(
                    ghInstance,
                    MAKEINTRESOURCE(IDD_DIALOG_ABOUT),
                    GetDesktopWindow(),
                    (DLGPROC)AboutDlgProc,
                    (LPARAM)NULL);
            }
#endif
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
