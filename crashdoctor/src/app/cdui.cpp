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

    cdui.cpp

Module Description:

    This file implements most of the UI handling code including Crash handling
    dialog, install dialog and recovery status dialog box code. It also
    implements the CRecoveryHandler class.

--*/

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <commctrl.h>
#include <string>
#include <algorithm>
#include <commdlg.h>
#include <shlwapi.h>
#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>

// Local-Includes!!!
#include "ihulib.h"
#include "resource.h"
#include "cdutil.h"
#include "cdmain.h"
#include "cdui.h"
#include "cdcore.h"


//
// Persistent registry keys for storing CrashDoctor configuration information
//
#define REG_APP_ROOT			_T("Software\\IntellectualHeaven CrashDoctor")
#define REG_APP_DEBUGGERS		_T("\\Debuggers")


//
// data type to store information about a debugger
//
typedef struct tagDEBUGGER_INFO
{
	tstring	debuggerPath;
	tstring cmdLine;

}DEBUGGER_INFO, *PDEBUGGER_INFO;


INT_PTR
CALLBACK
AboutDlgProc(
    HWND hDlg,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
/*++

Routine Description:

dialog box to show information about YahDecode

Returns:

TRUE - Message handled by the dialog proc
FALSE - Message not handled

--*/
{
    static HFONT hFontNormal = NULL;
    static bool linkHigh = false;
    int result = 0;

    switch (msg)
    {
    case WM_INITDIALOG:
    {
        HICON hIcon = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_APP_ICON_SM));
        SendMessage(hDlg, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)hIcon);
        SendMessage(hDlg, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)hIcon);
        CenterDialog(hDlg);

        if (hFontNormal == NULL)
        {
            hFontNormal = CreateFont(
                12, 0, 0, 0, FW_BOLD,
                FALSE, FALSE, FALSE,
                ANSI_CHARSET,
                OUT_DEFAULT_PRECIS,
                CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY,
                DEFAULT_PITCH | FW_DONTCARE,
                _T("Arial"));
        }

        SendDlgItemMessage(
            hDlg,
            IDC_COPYRIGHTWARNING,
            WM_SETFONT,
            (WPARAM)hFontNormal,
            NULL);

        SetDlgItemText(
            hDlg,
            IDC_COPYRIGHTWARNING,
            L"Warning: This computer program is a free program. It is "
            L"distributed under the terms of BSD License as published "
            L"by IntellectualHeaven. For a copy of this license, "
            L" write to pankaj@intellectualheaven.com");

        linkHigh = false;

        break;
    }
#if 0
    case WM_CTLCOLORDLG:
    {
        return (BOOL)GetSysColorBrush(COLOR_WINDOW);
    }
#endif
    case WM_CTLCOLORSTATIC:
    {
        HDC hdc = (HDC)wParam;

        if (GetDlgItem(hDlg, IDC_STATIC_LINK) == (HWND)lParam)
        {
            if (linkHigh)
            {
                if (SetTextColor(hdc, RGB(200, 100, 0)) == CLR_INVALID)
                {
                    cdHandleError(hDlg, GetLastError(), _T("Failed to set control color"));
                    break;
                }
            }
            else
            {
                if (SetTextColor(hdc, RGB(0, 0, 255)) == CLR_INVALID)
                {
                    cdHandleError(hDlg, GetLastError(), _T("Failed to set control color"));
                    break;
                }
            }

            SetBkMode(hdc, TRANSPARENT);
            return (BOOL)GetSysColorBrush(COLOR_BTNFACE);
        }

        if (GetDlgItem(hDlg, IDC_COPYRIGHTWARNING) == (HWND)lParam)
        {
            if (SetTextColor(hdc, RGB(64, 64, 64)) == CLR_INVALID)
            {
                cdHandleError(hDlg, GetLastError(), _T("Failed to set control color"));
                break;
            }

            SetBkMode(hdc, TRANSPARENT);
            return (BOOL)GetSysColorBrush(COLOR_BTNFACE);
        }
        
        break;
    }
    case WM_MOUSEMOVE:
    {
        short xbase = 120;
        short ybase = 66;
        short x = (short)LOWORD(lParam);
        short y = (short)HIWORD(lParam);
        if (x >= xbase && x <= (xbase + 178) && y >= ybase && y <= (ybase + 12))
        {
            if (!linkHigh)
            {
                linkHigh = true;
                InvalidateRect(GetDlgItem(hDlg, IDC_STATIC_LINK), NULL, FALSE);
            }

            SetCursor(LoadCursor(NULL, IDC_HAND));
        }
        else
        {
            if (linkHigh)
            {
                linkHigh = false;
                InvalidateRect(GetDlgItem(hDlg, IDC_STATIC_LINK), NULL, FALSE);
            }
            SetCursor(LoadCursor(NULL, IDC_ARROW));
        }

        return 0;
    }
    case WM_LBUTTONDOWN:
    {
        if (linkHigh)
        {
            ShellExecute(
                NULL,
                NULL,
                _T("http://www.intellectualheaven.com"),
                NULL,
                NULL,
                SW_SHOW | SW_MAXIMIZE);

            return 0;
        }

        break;
    }
    case WM_COMMAND:
    {
        switch (wParam)
        {
        case IDOK:
        {
            EndDialog(hDlg, IDOK);
            return TRUE;
        }
        case IDCANCEL:
        {
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        }

        break;
    }
    }

    return FALSE;
}


void
AddDebuggerToRegistry(
	HWND	inHwnd,	
	tstring &inDebuggerPath)
/*++

Routine Description:
	
	Adds a new debugger to the registry

Return:

	none

--*/
{
	DWORD numDebuggers;

	tstring tempString;
	DWORD tempSize = 0;

	int nReturnValue		= 0;
	HKEY hAppDebuggerKey	= NULL;
	

	nReturnValue = RegOpenKey(
							HKEY_LOCAL_MACHINE,
							REG_APP_ROOT REG_APP_DEBUGGERS,
							&hAppDebuggerKey);

	if (!hAppDebuggerKey)
	{
		cdHandleError(
			inHwnd,
			GetLastError(),
			_T("Add Debugger Failed. Unable to open required registry key."));

		goto funcExit;
		
	}

	numDebuggers = 0;
	tempSize = sizeof(numDebuggers);

	RegQueryValueEx(
			hAppDebuggerKey,
			_T("Count"),
			NULL,
			NULL,
			(LPBYTE)&numDebuggers,
			&tempSize);

	if (nReturnValue != ERROR_SUCCESS)
	{
		cdHandleError(
					inHwnd,
					GetLastError(),
					_T("Add Debugger Failed. Unable to open required registry key."));

		goto funcExit;
	}

	TCHAR valueName[32];
	_stprintf(valueName, _T("Debugger%02d"), numDebuggers);

	RegSetValueEx(
			hAppDebuggerKey,
			valueName,
			0,
			REG_SZ,
			(LPBYTE)inDebuggerPath.c_str(),
			(DWORD)(inDebuggerPath.length() * sizeof(TCHAR)));

	numDebuggers++;

	RegSetValueEx(
			hAppDebuggerKey,
			_T("Count"),
			0,
			REG_DWORD,
			(LPBYTE)&numDebuggers,
			sizeof(numDebuggers));

funcExit:

	if (hAppDebuggerKey)
	{
		RegCloseKey(hAppDebuggerKey);
	}

	return;
}



void
AddDebuggersToListCtrl(
	HWND hListCtrl)
/*++

Routine Description:
	
	Populate the list control with the list of the debuggers in the registry

Return:

	none

--*/
{
	ListView_DeleteAllItems(hListCtrl);

	tstring debuggerPath;

	DWORD numDebuggers;

	tstring tempString;
	TCHAR tempPath[MAX_PATH];
	DWORD tempSize = 0;

	int nReturnValue		= 0;
	HKEY hAppDebuggerKey	= NULL;
	

	nReturnValue = RegOpenKey(
							HKEY_LOCAL_MACHINE,
							REG_APP_ROOT REG_APP_DEBUGGERS,
							&hAppDebuggerKey);

	if (!hAppDebuggerKey)
	{
		cdHandleError(
			hListCtrl,
			GetLastError(),
			_T("CrashDoctor initialization failed. ")
			_T("Some features might be disabled."));

		goto funcExit;
		
	}

	numDebuggers = 0;
	tempSize = sizeof(numDebuggers);

	RegQueryValueEx(
			hAppDebuggerKey,
			_T("Count"),
			NULL,
			NULL,
			(LPBYTE)&numDebuggers,
			&tempSize);

	for (DWORD i = 0; i < numDebuggers; ++i)
	{
		TCHAR valueName[32];
		_stprintf(valueName, _T("Debugger%02d"), i);

		tempSize = sizeof(tempPath) * sizeof(TCHAR);
		nReturnValue = RegQueryValueEx(
								hAppDebuggerKey,
								valueName,
								NULL,
								NULL,
								(LPBYTE)tempPath,
								&tempSize);

		if (nReturnValue == ERROR_SUCCESS)
		{
			debuggerPath = tempPath;

			LVITEM  lvItem;
			ZeroMemory(&lvItem, sizeof(LVITEM));

			lvItem.mask		= LVIF_TEXT | LVIF_PARAM;
			lvItem.iItem	= 0;
			lvItem.lParam	= i;
			lvItem.iSubItem = 0;
			lvItem.pszText	= (LPTSTR)debuggerPath.c_str();

			int itemIndex =
			ListView_InsertItem(
							hListCtrl,
							&lvItem);
		}
	}

funcExit:

	if (hAppDebuggerKey)
	{
		RegCloseKey(hAppDebuggerKey);
	}

	return;
}



INT_PTR
CALLBACK
HandleCrashDlgProc(
	HWND	hDlg,
	UINT	msg,
	WPARAM	wParam,
	LPARAM	lParam)
/*++

Routine Description:
	
	Dialog box used to ask user what action he wants to take when an application
	crashes.

Arguments:

	Refer to DialogProc in MSDN

Returns:

	TRUE - Message Handled
	FALSE - Message not handled

--*/
{
	static PROC_DBG_DATA sProcDbgData;

	switch(msg)
	{
		case WM_INITDIALOG:
		{
			//
			// Insert *specific* dialog initialization code here.
			// lParam can contain information required for initialization.
			//

			//
			// Set dialog and application Icon
			//
			HICON hIcon = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_APP_ICON_SM));
		    SendMessage (hDlg, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)hIcon);
			hIcon = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_APP_ICON));
		    SendMessage (hDlg, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)hIcon);

			SetWindowText(hDlg, _T("IntellectualHeaven(R) CrashDoctor"));

			//
			// Set list control properties
			//
			HWND hListCtrl;
			hListCtrl = GetDlgItem(hDlg, IDC_LIST_DEBUGGER);
			ListView_SetExtendedListViewStyle(
										hListCtrl,
										LVS_EX_FULLROWSELECT);

			ListView_SetTextColor(	hListCtrl,
									RGB(0, 0, 255));

			LVCOLUMN lvColumn;
			lvColumn.mask = LVCF_TEXT | LVCF_WIDTH;
			
			lvColumn.cx			= 260;
			lvColumn.pszText	= _T(" Debuggers Available");

			ListView_InsertColumn(  hListCtrl,
									0,
									&lvColumn);

			CenterDialog(hDlg);

			PPROC_DBG_DATA procDbgData	= (PPROC_DBG_DATA)lParam;

			//
			// Store the PROC_DBG_DATA, it will be needed later
			//
			sProcDbgData = *procDbgData;

			
			// Find the process given in PROC_DBG_DATA
			DWORD processId = sProcDbgData.processId;

			IHU_PROCESS_INFO			processInfo;
			IHU_PROCESS_LIST		processList;
			IHU_PROCESS_LIST_ITER	processListIter;
			bool						processFound = false;

			IhuGetProcessList(processList);

			for (	processListIter = processList.begin();
					processListIter != processList.end();
					++processListIter)
			{
				processInfo = *processListIter;
				if (processId == processInfo.mProcessId)
				{
					processFound = true;
					break;
				}
			}

			//
			// Set process name and image
			//
			if (processFound)
			{
				SetWindowText(
						GetDlgItem(hDlg, IDC_EDIT_PROCESS_NAME),
						processInfo.mBinaryName.c_str());

				hIcon = NULL;
				
				IhuGetFileIcon(
							processInfo.mBinaryName,
							hIcon);

				if (hIcon)
				{
					SendMessage(
						GetDlgItem(hDlg, IDC_ICON_PROCESS),
						STM_SETICON,
						(WPARAM)hIcon,
						0);
				}
			}
			else
			{
				SetWindowText(
						GetDlgItem(hDlg, IDC_EDIT_PROCESS_NAME),
						_T("<Unknown Process>"));
				//
				// To-Do!!!
				// This should *NEVER* happen. How to handle this?
				//
			}
			

			//
			// Add debugger list to the list ctrl
			//
			AddDebuggersToListCtrl(
							hListCtrl);

			return TRUE;
		}
		case WM_COMMAND:
		{
			switch(wParam)
			{
				case IDC_BTN_RECOVER:
				{
					EndDialog(hDlg, IDC_BTN_RECOVER);
					return TRUE;
				}
				case IDC_BTN_TERMINATE:
				{
					EndDialog(hDlg, IDC_BTN_TERMINATE);
					return TRUE;
				}
				case IDC_BTN_DEBUG:
				{
					HWND hListCtrl = GetDlgItem(hDlg, IDC_LIST_DEBUGGER);
					int nSelectedItem = ListView_GetSelectionMark(hListCtrl);

					if (nSelectedItem >= 0)
					{
						TCHAR debugCmdFormat[MAX_PATH] = {0};

						LVITEM lvItem		= {0};
						lvItem.mask			= LVIF_TEXT;
						lvItem.iItem		= nSelectedItem;
						lvItem.iSubItem		= 0;
						lvItem.pszText		= debugCmdFormat;
						lvItem.cchTextMax	= MAX_PATH;
						
						if (ListView_GetItem(hListCtrl, &lvItem))
						{
							if (_tcslen(debugCmdFormat) > 0)
							{
								TCHAR launchDebuggerCmd[MAX_PATH * 2] = {0};
								_stprintf(	launchDebuggerCmd, 
											debugCmdFormat,
											sProcDbgData.processId,
											sProcDbgData.eventHandle);

								STARTUPINFO			startupInfo;
								PROCESS_INFORMATION procInfo;

								ZeroMemory(&startupInfo, sizeof(startupInfo));
								startupInfo.cb = sizeof(startupInfo);

								ZeroMemory(&procInfo, sizeof(procInfo));

								BOOL bResult = CreateProcess(
													NULL,
													launchDebuggerCmd,
													NULL,
													NULL,
													TRUE,
													0,
													NULL,
													NULL,
													&startupInfo,
													&procInfo);
								

								if (!bResult)
								{
									cdHandleError(
											hDlg,
											GetLastError(),
											_T("Unable to launch debugger."));
								}
								else
								{
									ShowWindow(hDlg, SW_HIDE);
									if (sProcDbgData.eventHandle)
									{
										//
										// wait till either the actual debugger dies or it sets the event
										// This wait is necessary because if we exit before the debugger
										// got attached to the target. The target dies because it thinks
										// we are the debugger.
										//
										HANDLE waitHandle[2];
										waitHandle[0] = sProcDbgData.eventHandle;
										waitHandle[1] = procInfo.hProcess;
										WaitForMultipleObjects(2, (const HANDLE *)&waitHandle, FALSE, INFINITE);
									}
									else
									{
										//
										// wait till either the actual debugger dies or 10 seconds are over
										// This wait is necessary because if we exit before the debugger
										// got attached to the target. The target dies because it thinks
										// we are the debugger.
										//
										WaitForSingleObject(procInfo.hProcess, 10000);
									}

									EndDialog(hDlg, IDC_BTN_DEBUG);
									return FALSE;
								}
							}
						}
					}

					break;
				}
				case IDC_BTN_ADD_DEBUGGER:
				{
					TCHAR tempFileName[MAX_PATH];
					tempFileName[0] = 0;

					OPENFILENAME ofn = {0};

					ofn.lStructSize		= OPENFILENAME_SIZE_VERSION_400;
					ofn.hwndOwner		= hDlg;
					ofn.hInstance		= ghInstance;
					ofn.lpstrFilter		= _T("Executable (*.exe)\0*.exe\0\0");
					ofn.lpstrFile		= tempFileName;
					ofn.nMaxFile		= MAX_PATH;
					ofn.lpstrTitle		= _T("Select a debugger");
					ofn.Flags			= OFN_HIDEREADONLY | OFN_LONGNAMES | OFN_PATHMUSTEXIST;

					if (GetOpenFileName(&ofn))
					{
						DEBUGGER_INFO debuggerInfo;
						debuggerInfo.debuggerPath	= ofn.lpstrFile;
						debuggerInfo.cmdLine		= _T("-p %ld -e %ld");

						DialogBoxParam(
									ghInstance,
									MAKEINTRESOURCE(IDD_DIALOG_ADD_DEBUGGER),
									hDlg,
									(DLGPROC)DebuggerDataDlgProc,
									(LPARAM)&debuggerInfo);

						tstring debuggerCmd = _T("\"") + debuggerInfo.debuggerPath + _T("\" ") + debuggerInfo.cmdLine;
						AddDebuggerToRegistry(hDlg, debuggerCmd);
						AddDebuggersToListCtrl(GetDlgItem(hDlg, IDC_LIST_DEBUGGER));
					}

					break;
				}
				case IDC_BTN_MODIFY_DEBUGGER:
				{
					HWND hListCtrl = GetDlgItem(hDlg, IDC_LIST_DEBUGGER);
					int nSelectedItem = ListView_GetSelectionMark(hListCtrl);

					if (nSelectedItem >= 0)
					{
						TCHAR debugCmdLine[MAX_PATH] = {0};

						LVITEM lvItem		= {0};
						lvItem.mask			= LVIF_TEXT | LVIF_PARAM;
						lvItem.iItem		= nSelectedItem;
						lvItem.iSubItem		= 0;
						lvItem.pszText		= debugCmdLine;
						lvItem.cchTextMax	= MAX_PATH;
						
						if (ListView_GetItem(hListCtrl, &lvItem))
						{
							LPARAM regIndex = lvItem.lParam;

							if (_tcslen(debugCmdLine) > 0)
							{
								DEBUGGER_INFO debuggerInfo;					
								debuggerInfo.cmdLine = debugCmdLine;

								DialogBoxParam(
											ghInstance,
											MAKEINTRESOURCE(IDD_DIALOG_MODIFY_DEBUGGER),
											hDlg,
											(DLGPROC)DebuggerDataDlgProc,
											(LPARAM)&debuggerInfo);

								if (_tcscmp(debugCmdLine, debuggerInfo.cmdLine.c_str()) != 0)
								{
									//
									// Modify the particular registry entry
									//
									TCHAR valueName[32];
									_stprintf(valueName, _T("Debugger%02d"), regIndex);

									int nReturnValue		= 0;
									HKEY hAppDebuggerKey	= NULL;
									

									nReturnValue = RegOpenKey(
															HKEY_LOCAL_MACHINE,
															REG_APP_ROOT REG_APP_DEBUGGERS,
															&hAppDebuggerKey);

									if (hAppDebuggerKey)
									{
										if (RegSetValueEx(
													hAppDebuggerKey,
													valueName,
													0,
													REG_SZ,
													(LPBYTE)debuggerInfo.cmdLine.c_str(),
													(DWORD)(debuggerInfo.cmdLine.length() * sizeof(TCHAR))) != ERROR_SUCCESS)
										{
											cdHandleError(
													hDlg,
													GetLastError(),
													_T("Modify Debugger Failed. Unable to update the registry key."));
										}

										RegCloseKey(hAppDebuggerKey);

										AddDebuggersToListCtrl(GetDlgItem(hDlg, IDC_LIST_DEBUGGER));
									}
									else
									{
										cdHandleError(
													hDlg,
													GetLastError(),
													_T("Modify Debugger Failed. Unable to open required registry key."));

									}
								}
							}
						}
					}
					
					break;
				}
				case IDC_BTN_DELETE_DEBUGGER:
				{
					HWND hListCtrl = GetDlgItem(hDlg, IDC_LIST_DEBUGGER);
					int nSelectedItem = ListView_GetSelectionMark(hListCtrl);

					if (nSelectedItem >= 0)
					{
						TCHAR debugCmdLine[MAX_PATH] = {0};

						LVITEM lvItem		= {0};
						lvItem.mask			= LVIF_TEXT | LVIF_PARAM;
						lvItem.iItem		= nSelectedItem;
						lvItem.iSubItem		= 0;
						lvItem.pszText		= debugCmdLine;
						lvItem.cchTextMax	= MAX_PATH;
						
						if (ListView_GetItem(hListCtrl, &lvItem))
						{
							LPARAM regIndex = lvItem.lParam;
									
							//
							// Modify the particular registry entry
							//
							TCHAR valueName[32];
							_stprintf(valueName, _T("Debugger%02d"), regIndex);

							int nReturnValue		= 0;
							HKEY hAppDebuggerKey	= NULL;
							

							nReturnValue = RegOpenKey(
													HKEY_LOCAL_MACHINE,
													REG_APP_ROOT REG_APP_DEBUGGERS,
													&hAppDebuggerKey);

							if (hAppDebuggerKey)
							{
								if (RegDeleteValue(
											hAppDebuggerKey,
											valueName) != ERROR_SUCCESS)
								{
									cdHandleError(
											hDlg,
											GetLastError(),
											_T("Delete Debugger Failed. Unable to delete the registry key."));
								}

								RegCloseKey(hAppDebuggerKey);

								AddDebuggersToListCtrl(GetDlgItem(hDlg, IDC_LIST_DEBUGGER));
							}
							else
							{
								cdHandleError(
											hDlg,
											GetLastError(),
											_T("Delete Debugger Failed. Unable to open required registry key."));

							}
						}
					}
					
					break;
				}
				case IDCANCEL:
				{
					EndDialog(hDlg, IDCANCEL);
                    return TRUE;
				}
			}

			break;
		}
		case WM_CTLCOLORSTATIC:
		{
			HDC hdc = (HDC)wParam;
			HWND hwndCtl = (HWND)lParam;

			if (hwndCtl == GetDlgItem(hDlg, IDC_EDIT_PROCESS_NAME))
			{	
				if (SetTextColor(hdc, RGB(255, 0, 0)) == CLR_INVALID)
				{
					cdHandleError(hDlg, GetLastError(), _T("Failed to set control color"));
					break;
				}
				SetBkMode(hdc, TRANSPARENT);
				SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
				SelectObject(hdc, GetSysColorBrush(COLOR_WINDOW));
				return TRUE;
			}

			break;
		}
		case WM_DESTROY:
		{
			return TRUE;
		}
		case WM_CLOSE:
		{
			break;
		}
	}

	//
	// Not handled by us
	//
	return FALSE;
}


INT_PTR
CALLBACK
DebuggerDataDlgProc(
	HWND	hDlg,
	UINT	msg,
	WPARAM	wParam,
	LPARAM	lParam)
/*++

Routine Description:
	
	Common dialog box to get some input from the user

Arguments:

	Refer to DialogProc in MSDN

Returns:

	TRUE - Message Handled
	FALSE - Message not handled

--*/
{
	static PDEBUGGER_INFO debuggerInfo;

	switch(msg)
	{
		case WM_INITDIALOG:
		{
			//
			// Insert *specific* dialog initialization code here.
			// lParam can contain information required for initialization.
			//

			//
			// Set dialog and application Icon
			//
			HICON hIcon = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_APP_ICON_SM));
		    SendMessage (hDlg, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)hIcon);
			hIcon = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_APP_ICON));
		    SendMessage (hDlg, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)hIcon);

			CenterDialog(hDlg);

			debuggerInfo = (PDEBUGGER_INFO)lParam;

			SetWindowText(
					GetDlgItem(hDlg, IDC_TEXT_DEBUGGER),
					debuggerInfo->debuggerPath.c_str());

			SetWindowText(
					GetDlgItem(hDlg, IDC_EDIT_CMDLINE),
					debuggerInfo->cmdLine.c_str());

			return TRUE;
		}
		case WM_COMMAND:
		{
			switch(wParam)
			{
				case IDOK:
				{
					//
					// Set result here and terminate the dialog
					//

					TCHAR dataBuffer[64] = {0};

					GetWindowText(
							GetDlgItem(hDlg, IDC_EDIT_CMDLINE),
							dataBuffer,
							sizeof(dataBuffer) / sizeof(TCHAR));

					debuggerInfo->cmdLine = dataBuffer;

					EndDialog(hDlg, IDOK);
                    return TRUE;
				}
			}

			break;
		}
		case WM_DESTROY:
		{
			return TRUE;
		}
	}

	return FALSE;
}



INT_PTR
CALLBACK
RecoveryStatusDlgProc(
	HWND	hDlg,
	UINT	msg,
	WPARAM	wParam,
	LPARAM	lParam)
/*++

Routine Description:
	
	This dialog box opens up if the user selects Recover on HandleCrash
	dialog. This dialog box basically transfer control to CRecoveryHandler
	in most cases. It also allows the user to terminate the crashing process
	in case it is not recovering.

Arguments:

	Refer to DialogProc in MSDN

Returns:

	TRUE - Message Handled
	FALSE - Message not handled

--*/
{
	CRecoveryHandler *pRecoveryHandler = NULL;

	if (msg == WM_INITDIALOG)
	{
		SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
		pRecoveryHandler = (CRecoveryHandler *)lParam;
	}
	else
	{
		pRecoveryHandler = (CRecoveryHandler *)
								GetWindowLongPtr(hDlg, GWLP_USERDATA);
	}

	switch(msg)
	{
		case WM_INITDIALOG:
		{
			HICON hIcon = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_APP_ICON_SM));
		    SendMessage (hDlg, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)hIcon);
			hIcon = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_APP_ICON));
		    SendMessage (hDlg, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)hIcon);

			SetWindowText(hDlg, _T("IntellectualHeaven(R) CrashDoctor"));

			CenterDialog(hDlg);
			pRecoveryHandler->InitInstance(hDlg, GetDlgItem(hDlg, IDC_LIST_STATUS));
			return TRUE;
		}
		case WM_COMMAND:
		{
			switch(wParam)
			{
				case IDC_BTN_TERMINATE:
				{
					pRecoveryHandler->ExitInstance();
					return TRUE;
				}
				case IDC_BTN_CLOSE:
				{
					EndDialog(hDlg, IDC_BTN_CLOSE);
					return TRUE;
				}
			}

			break;
		}
		case WM_NOTIFY:
		{
			LPNMHDR pNM = (LPNMHDR)lParam;

			if(pNM->hwndFrom == GetDlgItem(hDlg, IDC_LIST_STATUS))
			{
				switch(pNM->code)
				{
					case NM_CUSTOMDRAW:
					{
						LPNMLVCUSTOMDRAW pCD = (LPNMLVCUSTOMDRAW)lParam;
						SetWindowLong(
									hDlg,
									DWLP_MSGRESULT,
									(LONG)pRecoveryHandler->HandleCustomDraw(pCD));
						return TRUE;
					}
				}
			}
			break;
        }
	}

	return FALSE;
}



DWORD
WINAPI
RecoveryThread(
	LPVOID inParam)
/*++

Routine Description:

	This thread simply handles the debug events and take appropriate
	action accordingly to recover the process from crashing. It simply
	calls StartRecovery of the CRecoveryHandler which basically transfer
	control to CRecoverCrash for the recovery. CRecoverCrash does all
	the heavyweight lifting of crash recovery process.

Arguments:

	Refer to ThreadProc in MSDN

Return Value:

	DWORD - we always return 0

--*/
{
	DWORD funcResult = 0;

	CRecoveryHandler *pRecoveryHandler = (CRecoveryHandler *)inParam;

	pRecoveryHandler->StartRecovery();
	pRecoveryHandler->SignalRecoveryThreadExit();

	return funcResult;
}



CRecoveryHandler::CRecoveryHandler(
	DWORD	inProcessId,
	HANDLE	inEventHandle)
/*++

Routine Description:

	Constructor for our CRecoveryHandler class

Arguments:

	inProcessId - Process Id of the faulty process

	inEventHandle - Handle of the event which windows create when a process
		crash. Debugger can signal this event once they are attached to the
		process.

--*/
{
	int fontHeight	= 12;
	int fontWidth	= 0;

	m_TraceListFont = CreateFont(
							fontHeight, fontWidth,
							0, 0, FW_NORMAL,
							FALSE,FALSE,FALSE,
							ANSI_CHARSET,
							OUT_DEFAULT_PRECIS,
							CLIP_DEFAULT_PRECIS,
							DEFAULT_QUALITY,
							DEFAULT_PITCH | FF_SWISS,
							_T("Lucida Console"));

	m_TraceListFontBold = CreateFont(
							fontHeight, fontWidth,
							0, 0, FW_BOLD,
							FALSE,FALSE,FALSE,
							ANSI_CHARSET,
							OUT_DEFAULT_PRECIS,
							CLIP_DEFAULT_PRECIS,
							DEFAULT_QUALITY,
							DEFAULT_PITCH | FF_SWISS,
							_T("Lucida Console"));

	m_ProcessId = inProcessId;
	m_hEvent	= inEventHandle;
}



CRecoveryHandler::~CRecoveryHandler()
/*++

Routine Description:

	Destructor for our CRecoveryHandler class

Arguments:

	none

--*/
{
	if (m_TraceListFont)
	{
		DeleteObject(m_TraceListFont);
		m_TraceListFont = NULL;
	}

	if (m_TraceListFontBold)
	{
		DeleteObject(m_TraceListFontBold);
		m_TraceListFontBold = NULL;
	}
}



void
CRecoveryHandler::InitInstance(
	HWND	inDialogHwnd,
	HWND	inListCtrlHwnd)
/*++

Routine Description:

	This functions does the UI related intialization first and then launches
	the recovery thread. This function is called from Recovery dialog's
	WM_INITDIALOG.

Arguments:

	inDialogHwnd - Handle to the Recovery dialog

	inListCtrlHwnd - Handle to the recovery status list view

Returns:

	None

--*/
{
	m_hwndDlg		= inDialogHwnd;
	m_hwndListCtrl	= inListCtrlHwnd;

	// Disable the close & terminate button
	EnableWindow(
			GetDlgItem(m_hwndDlg, IDC_BTN_CLOSE),
			FALSE);

	EnableWindow(
			GetDlgItem(m_hwndDlg, IDC_BTN_TERMINATE),
			FALSE);

	LVCOLUMN lvColumn;
	lvColumn.mask	= LVCF_WIDTH;
	lvColumn.cx		= 1024;

	ListView_InsertColumn(inListCtrlHwnd, 0, &lvColumn);
	ListView_SetBkColor(m_hwndListCtrl, RGB(230, 232, 208));

	ListView_SetExtendedListViewStyle(
								m_hwndListCtrl,
                                LVS_EX_FULLROWSELECT);

	//
	// Launch recovery thread
	//
	mThreadHandle = CreateThread(	
							NULL,
							0,
							RecoveryThread,
							(LPVOID)this,
							0,
							&mThreadId);

	// If thread creation failed, then enable the close button
	// if it succeeded then enable the terminate button
	if (mThreadHandle == NULL)
	{
		EnableWindow(
			GetDlgItem(m_hwndDlg, IDC_BTN_CLOSE),
			TRUE);
	}
	else
	{
		EnableWindow(
			GetDlgItem(m_hwndDlg, IDC_BTN_TERMINATE),
			TRUE);
	}
}



void
CRecoveryHandler::ExitInstance()
/*++

Routine Description:

	If this function is called this means that user pressed terminate on the
	recovery dialog. We will kill the recovery thread in this function which
	would cause faulty process to get terminated as well. Then we will do any
	other UI processing required like enabling/disabling window controls

Arguments:

	none

Returns:

	None

--*/
{
	TerminateThread(mThreadHandle, 0);

	EnableWindow(
			GetDlgItem(m_hwndDlg, IDC_BTN_CLOSE),
			TRUE);

	EnableWindow(
			GetDlgItem(m_hwndDlg, IDC_BTN_TERMINATE),
			FALSE);
}



void
CRecoveryHandler::StartRecovery()
/*++

Routine Description:

	This functions is called by our recovery thread. We delegate the recovery
	work to CRecoverCrash here

Arguments:

	none

Returns:

	None

--*/
{
    CRecoverCrash recoverCrash;
    recoverCrash.AttachToProcess(
                        m_ProcessId,
                        m_hEvent,
						this);
}



void
CRecoveryHandler::SignalRecoveryThreadExit()
/*++

Routine Description:

	If the recovery thread is terminating due to the faulty process exit or some
	other reason then it should call this function before exiting. This gives us
	a chance to reflect the stage in UI.

Arguments:

	None

Returns:

	None

--*/
{
	EnableWindow(
			GetDlgItem(m_hwndDlg, IDC_BTN_CLOSE),
			TRUE);

	EnableWindow(
			GetDlgItem(m_hwndDlg, IDC_BTN_TERMINATE),
			FALSE);
}



INT_PTR
CRecoveryHandler::HandleCustomDraw(
	LPNMLVCUSTOMDRAW pCD)
/*++

Routine Description:

	Handles the custom draw notification for Recovery dialog list view

Arguments:

	pCD - pointer to customdraw struct supplied by windows

Returns:

	None

--*/
{
	pCD->clrTextBk = RGB(230, 232, 208);

	if(pCD->nmcd.dwDrawStage == CDDS_PREPAINT)
	{
		return CDRF_NOTIFYITEMDRAW;
	}
	else if(pCD->nmcd.dwDrawStage == CDDS_ITEMPREPAINT)
	{
		if ((pCD->nmcd.lItemlParam & 0xFF) == PRINT_TITLE)
		{
			SelectObject(pCD->nmcd.hdc, m_TraceListFontBold);
			pCD->clrText = RGB(128, 0, 255);
		}
		else if ((pCD->nmcd.lItemlParam & 0xFF) == PRINT_MSG)
		{
			SelectObject(pCD->nmcd.hdc, m_TraceListFont);
			pCD->clrText = RGB(0, 0, 255);
		}
		else if ((pCD->nmcd.lItemlParam & 0xFF) == PRINT_INFO)
		{
			SelectObject(pCD->nmcd.hdc, m_TraceListFont);
			pCD->clrText = RGB(0, 0, 0);
		}
		else if ((pCD->nmcd.lItemlParam & 0xFF) == PRINT_TRACE)
		{
			SelectObject(pCD->nmcd.hdc, m_TraceListFont);
			pCD->clrText = RGB(128, 128, 0);
		}
		else if ((pCD->nmcd.lItemlParam & 0xFF) == PRINT_WARN)
		{
			SelectObject(pCD->nmcd.hdc, m_TraceListFont);
			pCD->clrText = RGB(128, 64, 64);
		}
		else if ((pCD->nmcd.lItemlParam & 0xFF) == PRINT_ERROR)
		{
			SelectObject(pCD->nmcd.hdc, m_TraceListFont);
			pCD->clrText = RGB(255, 0, 0);
		}
		else
		{
			SelectObject(pCD->nmcd.hdc, m_TraceListFont);
			pCD->clrText = RGB(64, 64, 64);
		}

		return CDRF_DODEFAULT;
	}

	return CDRF_DODEFAULT;
}

void
CRecoveryHandler::PrintW(
	DWORD inPrintWhat,
	LPCWSTR inFormat,
	va_list inArgList)
/*++

Routine Description:

	Does for unicode strings what PrintA does for ascii strings

Arguments:

	refer PrintA

Returns:

	None

--*/
{
	wchar_t szMsg[1024];

	StringCchVPrintfW(
					szMsg,
					sizeof(szMsg)/sizeof(wchar_t),
					inFormat,
					inArgList);

	std::wstring trcMsg = szMsg;

	size_t i_CR = 0;

	while (i_CR != -1)
	{
		i_CR = trcMsg.find_first_of(L'\r', 0);

		if (i_CR != -1)
		{
			trcMsg.erase(i_CR, 1);
		}
	}

	std::wstring lastMsg;
	bool bUseLastMsg = false;

	int itemCount = ListView_GetItemCount(m_hwndListCtrl);
	if (itemCount > 0)
	{
		LVITEMW lastLvItem;
		ZeroMemory(&lastLvItem, sizeof(LVITEMW));
		lastLvItem.mask			= LVIF_TEXT | LVIF_PARAM;
		lastLvItem.pszText		= szMsg;
		lastLvItem.cchTextMax	= 512;
		lastLvItem.iItem		= itemCount - 1;
		lastLvItem.iSubItem		= 0;
		SendMessageW(m_hwndListCtrl, LVM_GETITEMW, 0, (LPARAM)&lastLvItem);

		if ((lastLvItem.lParam & 0x80000000) == 0 &&
			(lastLvItem.lParam & 0xFF) == inPrintWhat)
		{
			bUseLastMsg = true;
			lastMsg = lastLvItem.pszText;
			itemCount--;
		}
	}

	size_t i_begin = 0;
    size_t i_end = 0;

	do
	{
		if (i_begin >= trcMsg.length())
		{
			break;
		}

		DWORD templParam = 0;
		std::wstring newMsg;

		i_end = trcMsg.find_first_of('\n', i_begin);

		if (i_end != -1)
		{
			templParam = 0x80000000;
			newMsg = trcMsg.substr(i_begin, i_end - i_begin);
			i_begin = i_end + 1;
		}
		else
		{
			newMsg = trcMsg.substr(i_begin, trcMsg.length() - i_begin);
		}
		

		newMsg = lastMsg + newMsg;
		lastMsg = L"";

		LVITEMW lvItem;
		ZeroMemory(&lvItem, sizeof(LVITEMW));
		lvItem.mask		= LVIF_TEXT | LVIF_PARAM;
		lvItem.iItem	= itemCount++;
		lvItem.iSubItem = 0;
		lvItem.pszText	= (LPWSTR)newMsg.c_str();
		lvItem.lParam	= inPrintWhat | templParam;

		if(bUseLastMsg)
		{
			bUseLastMsg = false;
			SendMessageW(m_hwndListCtrl, LVM_SETITEMW, 0, (LPARAM)&lvItem);
		}
		else
		{
			SendMessageW(m_hwndListCtrl, LVM_INSERTITEMW, 0, (LPARAM)&lvItem);
			ListView_EnsureVisible(m_hwndListCtrl, itemCount - 1, FALSE);
		}
	}while (i_end != -1);
}

void
CRecoveryHandler::PrintTitleW(
	LPCWSTR inFormat,
	...)
{
	va_list	argList;
	va_start(argList, inFormat);
	PrintW(PRINT_TITLE, inFormat, argList);
}

void
CRecoveryHandler::PrintMessageW(
	LPCWSTR inFormat,
	...)
{
	va_list	argList;
	va_start(argList, inFormat);
	PrintW(PRINT_MSG, inFormat, argList);
}

void
CRecoveryHandler::PrintInfoW(
	LPCWSTR inFormat,
	...)
{
	va_list	argList;
	va_start(argList, inFormat);
	PrintW(PRINT_INFO, inFormat, argList);
}

void
CRecoveryHandler::PrintTraceW(
	LPCWSTR inFormat,
	...)
{
	va_list	argList;
	va_start(argList, inFormat);
	PrintW(PRINT_TRACE, inFormat, argList);
}

void
CRecoveryHandler::PrintWarningW(
	LPCWSTR inFormat,
	...)
{
	va_list	argList;
	va_start(argList, inFormat);
	PrintW(PRINT_WARN, inFormat, argList);
}

void
CRecoveryHandler::PrintErrorW(
	LPCWSTR inFormat,
	...)
{
	va_list	argList;
	va_start(argList, inFormat);
	PrintW(PRINT_ERROR, inFormat, argList);
}


