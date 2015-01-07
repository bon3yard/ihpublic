/*++

Copyright (c) : Pankaj Garg & www.intellectualheaven.com

Module Name:

    cdinstall.cpp

Module Description:

	CrashDoctor installation module. The installation module is implemented
	as a part of CrashDoctor itself. If CrashDoctor is invoked without any
	arguments, it brings up the install/uninstall dialog

Author:

    Pankaj Garg		2004-09-17
    
Most recent update:

    Pankaj Garg     2005-03-18

--*/
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <shlobj.h>
#include <objidl.h>
#include <shlwapi.h>
#include <string>
#include <algorithm>

// Local-Includes!!!
#include "cdutil.h"
#include "cdinstall.h"
#include "resource.h"
#include "cdmain.h"

#define REG_APP_ROOT			_T("Software\\InellectualHeaven\\CrashDoctor")
#define REG_APP_ROOT_6432		_T("Software\\Wow6432Node\\InellectualHeaven\\CrashDoctor")
#define REG_APP_DEBUGGERS		_T("\\Debuggers")
#define REG_AEDEBUG_PATH		_T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug")
#define REG_AEDEBUG_PATH_6432	_T("SOFTWARE\\Wow6432Node\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug")


//
// external function
//
extern TO_UPPER gToUpper;


class CInstallCrashDoctor
{
public:
	CInstallCrashDoctor(HWND inHwnd)
	{
		m_Hwnd = inHwnd;
	}

	bool IsInstalled(LPTSTR inAeDebugPath);
	void Install();
	void Uninstall();

private:

	void PreparePaths();

	bool CopyFiles();
	void DeleteFiles();

	bool CreateLinks();
	void DeleteLinks();

	bool CreateRegistry(LPCTSTR inAppRoot, LPCTSTR inAeDebug, tstring &inExePath);
	void RestoreRegistry(LPCTSTR inAppRoot,	LPCTSTR inAeDebug);

	HWND		m_Hwnd;
	tstring		m_InstallPath;
	tstring		m_ExePathTarget32;
	tstring		m_ExePathTarget64;
	tstring		m_ExePathNative;
	tstring		m_ChmPathTarget;
	tstring		m_PsapiPathTarget;
	tstring		m_ShFolderPathTarget;
	tstring		m_DbgHelpPathTarget;
	tstring		m_StartMenuPath;

};



bool
CInstallCrashDoctor::IsInstalled(
	LPTSTR inAeDebugPath)
/*++

Routine Description:
	
	Function to check if CrashDoctor is already installed or not

Returns:

	true - installed
	false - not installed

--*/
{
	bool funcResult = false;
	tstring currentDebugger;

	TCHAR tempPath[MAX_PATH];
	DWORD tempSize = 0;

	int nReturnValue		= 0;
	HKEY hAeDebugKey		= NULL;
	

	nReturnValue = RegOpenKey(
							HKEY_LOCAL_MACHINE,
							inAeDebugPath,
							&hAeDebugKey);

	if (nReturnValue != ERROR_SUCCESS)
	{
		// To-Do!!! Handle Error
		goto funcExit;
	}

	tempSize = sizeof(tempPath) * sizeof(TCHAR);
	nReturnValue = RegQueryValueEx(
							hAeDebugKey,
							_T("Debugger"),
							NULL,
							NULL,
							(LPBYTE)tempPath,
							&tempSize);

	if (nReturnValue == ERROR_SUCCESS)
	{
		currentDebugger = tempPath;
	}

	if (currentDebugger.length() > 0)
	{
		std::transform(	currentDebugger.begin(),
						currentDebugger.end(),
						currentDebugger.begin(), 
						gToUpper);

		if (_tcsstr(currentDebugger.c_str(), _T("CRASHDOCTOR.EXE")))
		{
			funcResult = true;
			goto funcExit;
		}
	}

funcExit:

	if (hAeDebugKey)
	{
		RegCloseKey(hAeDebugKey);
	}

	return funcResult;
}



void
CInstallCrashDoctor::Install()
/*++

Routine Description:
	
	This one mammoth function implements a very simple installation of
	CrashDoctor

Returns:

	none

--*/
{
	bool filesCopied	= false;
	bool linksCreated	= false;
	bool regCreated		= false;
#if defined(_M_AMD64)
	bool regCreated6432	= false;
#endif
	bool installSuccess = true;

	// Prepare all the target paths
	PreparePaths();

	// Copy files
	if (!CopyFiles())
	{
		installSuccess = false;
		goto funcEnd;
	}
	filesCopied = true;

	// Create links
	if (!CreateLinks())
	{
		installSuccess = false;
		goto funcEnd;
	}
	linksCreated = true;

	// Create registry entries
	if (!CreateRegistry(
					REG_APP_ROOT,
					REG_AEDEBUG_PATH,
					m_ExePathNative))
	{
		installSuccess = false;
		goto funcEnd;
	}
	regCreated = true;

#if defined(_M_AMD64)
	// Create Wow6432Node registry entries
	if (!CreateRegistry(
					REG_APP_ROOT_6432,
					REG_AEDEBUG_PATH_6432,
					m_ExePathTarget32))
	{
		installSuccess = false;
		goto funcEnd;
	}
	regCreated6432 = true;
#endif
	
	cdShowMessage(
			m_Hwnd,
			_T("Installation Successful. CrashDoctor will activate whenever\n")
			_T("a program crashes on this system."));

funcEnd:

	if (installSuccess == false)
	{
		if (filesCopied)
		{
			DeleteFiles();
		}

		if (linksCreated)
		{
			DeleteLinks();
		}

		if (regCreated)
		{
			RestoreRegistry(
						REG_APP_ROOT,
						REG_AEDEBUG_PATH);
		}

#if defined(_M_AMD64)
		if (regCreated6432)
		{
			RestoreRegistry(
						REG_APP_ROOT_6432,
						REG_AEDEBUG_PATH_6432);
		}
#endif
	}

	return;
}



void
CInstallCrashDoctor::Uninstall()
/*++

Routine Description:
	
	This function uninstalls the CrashDoctor application from a system if it
	is installed

Returns:

	none

--*/
{


	// Fetch information for all the paths
	PreparePaths();

	// Restore registry information
	RestoreRegistry(
				REG_APP_ROOT,
				REG_AEDEBUG_PATH);

#if defined(_M_AMD64)
	RestoreRegistry(
				REG_APP_ROOT_6432,
				REG_AEDEBUG_PATH_6432);
#endif

	// Remove links
	DeleteLinks();

	// Delete files
	DeleteFiles();

	cdShowMessage(
			m_Hwnd,
			_T("Uninstall Successful. CrashDoctor is removed from this \n")
			_T("system."));

	return;
}



bool
CInstallCrashDoctor::CopyFiles()
{
	tstring currentDir;
	tstring exePathSource;
	tstring chmPathSource;
	tstring redistDLLSource;

	tstring tempString;
	TCHAR	tempPath[MAX_PATH];
	DWORD	tempSize = 0;


	GetCurrentDirectory(
					MAX_PATH,
					tempPath);

	currentDir = tempPath;

	if (currentDir.empty())
	{
		cdHandleError(
					m_Hwnd,
					GetLastError(),
					_T("GetCurrentDirectory failed. Installation aborted."));

		goto funcExit;
	}

	if (currentDir[currentDir.length() - 1] != _T('\\'))
	{
		currentDir += _T("\\");
	}

	CreateDirectory(m_InstallPath.c_str(), NULL);

#if defined(_M_AMD64)

	//
	// For 64-bit we don't copy redistributable DLL because they are already in
	// in the shipping platform
	//

	exePathSource = currentDir + _T("CrashDoctor.exe");
	if (!CopyFile(exePathSource.c_str(), m_ExePathTarget64.c_str(), FALSE))
	{
		goto funcExit;
	}

	exePathSource = currentDir + _T("CrashDoctor.32");
	if (!CopyFile(exePathSource.c_str(), m_ExePathTarget32.c_str(), FALSE))
	{
		goto funcExit;
	}

#else

	exePathSource = currentDir + _T("CrashDoctor.exe");
	if (!CopyFile(exePathSource.c_str(), m_ExePathTarget32.c_str(), FALSE))
	{
		goto funcExit;
	}

	if (gOSExactVersion <= WIN32_NT4)
	{
		redistDLLSource = currentDir + _T("shfolder.dll");
		if (!CopyFile(
					redistDLLSource.c_str(),
					m_ShFolderPathTarget.c_str(),
					FALSE))
		{
			goto funcExit;
		}

		redistDLLSource = currentDir + _T("dbghelp.dll");
		if (!CopyFile(
					redistDLLSource.c_str(),
					m_DbgHelpPathTarget.c_str(),
					FALSE))
		{
			goto funcExit;
		}
	}

	if (gOSExactVersion == WIN32_NT4)
	{
		redistDLLSource = currentDir + _T("psapi.dll");
		if (!CopyFile(
					redistDLLSource.c_str(),
					m_PsapiPathTarget.c_str(),
					FALSE))
		{
			goto funcExit;
		}
	}

#endif

	// We don't have a chm help file right now so use info.txt for help
	chmPathSource = currentDir + _T("Info.txt");

	if (!CopyFile(chmPathSource.c_str(), m_ChmPathTarget.c_str(), FALSE))
	{
		goto funcExit;
	}

	return true;

funcExit:

	cdHandleError(
				m_Hwnd,
				GetLastError(),
				_T("Installation failed. Unable to copy files to the ")
				_T("installation folder. Please make sure that you ")
				_T("extract the files in the zip folder to a temporary ")
				_T("directory before installing CrashDoctor"));

	return false;
}



void
CInstallCrashDoctor::DeleteFiles()
{
	DeleteFile(m_ExePathTarget32.c_str());
	DeleteFile(m_ExePathTarget64.c_str());
	DeleteFile(m_ChmPathTarget.c_str());
	DeleteFile(m_PsapiPathTarget.c_str());
	DeleteFile(m_ShFolderPathTarget.c_str());
	DeleteFile(m_DbgHelpPathTarget.c_str());
	RemoveDirectory(m_InstallPath.c_str());
}



bool
CInstallCrashDoctor::CreateLinks()
{
	// Create all the required startmenu shortcuts

	CreateDirectory(m_StartMenuPath.c_str(), NULL);

	CreateLink(
		m_ExePathNative.c_str(),
		(m_StartMenuPath + _T("\\CrashDoctor Application.lnk")).c_str(),
		_T("IntellectualHeaven (R) CrashDoctor"));

	CreateLink(
		m_ChmPathTarget.c_str(),
		(m_StartMenuPath + _T("\\CrashDoctor Help.lnk")).c_str(),
		_T("CrashDoctor Help"));

	return true;
}



void
CInstallCrashDoctor::DeleteLinks()
{
	DeleteFile((m_StartMenuPath + _T("\\CrashDoctor Application.lnk")).c_str());
	DeleteFile((m_StartMenuPath + _T("\\CrashDoctor Help.lnk")).c_str());
	RemoveDirectory(m_StartMenuPath.c_str());
}



bool
CInstallCrashDoctor::CreateRegistry(
	LPCTSTR inAppRoot,
	LPCTSTR inAeDebug,
	tstring &inExePath)
{
	bool funcStatus			= false;
	int nReturnValue		= 0;
	HKEY hAeDebugKey		= NULL;
	HKEY hAppInfoKey		= NULL;
	HKEY hAppDebuggerKey	= NULL;

	tstring currentDebugger;
	tstring crashDoctorDebugger;
	tstring debuggersKey;

	tstring tempString;
	TCHAR	tempPath[MAX_PATH];
	DWORD	tempSize = 0;
	

	nReturnValue = RegCreateKey(
							HKEY_LOCAL_MACHINE,
							inAeDebug,
							&hAeDebugKey);

	if (nReturnValue != ERROR_SUCCESS)
	{
		goto funcExit;
	}
	
	nReturnValue = RegCreateKey(
							HKEY_LOCAL_MACHINE,
							inAppRoot,
							&hAppInfoKey);

	if (nReturnValue != ERROR_SUCCESS)
	{
		goto funcExit;
	}
	
	debuggersKey = inAppRoot;

	nReturnValue = RegCreateKey(
							HKEY_LOCAL_MACHINE,
							(debuggersKey + REG_APP_DEBUGGERS).c_str(),
							&hAppDebuggerKey);

	if (nReturnValue != ERROR_SUCCESS)
	{
		goto funcExit;
	}
	

	//
	// Get the name of the debugger currently configured to use
	// when an application crashes
	//
	if (gOSVersion == WIN32_9X)
	{
		tempPath[0] = 0;

		GetProfileString(
					_T("AEDEBUG"),
					_T("DEBUGGER"),
					_T(""),
					tempPath,
					MAX_PATH);

		currentDebugger = tempPath;
	}
	else
	{
		tempSize = sizeof(tempPath) * sizeof(TCHAR);
		nReturnValue = RegQueryValueEx(
								hAeDebugKey,
								_T("Debugger"),
								NULL,
								NULL,
								(LPBYTE)tempPath,
								&tempSize);

		if (nReturnValue == ERROR_SUCCESS)
		{
			currentDebugger = tempPath;
		}
	}


	//
	// if existing debugger is us only, it means someone is trying
	// to install us again. Simply ignore the existing debugger key
	// value and don't add it to our info registry
	//
	if (currentDebugger.length() > 0)
	{
		tempString = currentDebugger;

		std::transform(	tempString.begin(),
						tempString.end(),
						tempString.begin(), 
						gToUpper);

		if (_tcsstr(tempString.c_str(), _T("CRASHDOCTOR.EXE")))
		{
			currentDebugger = _T("");
		}
	}


	//
	// Create required registry entries
	// Following entries will be set
	// 1. aedebug key
	// 2. We will add the existing debugger to our list of debuggers
	//

	crashDoctorDebugger = _T("\"") + inExePath + _T("\" -p %ld -e %ld");

	nReturnValue = RegSetValueEx(
							hAeDebugKey,
							_T("Debugger"),
							0,
							REG_SZ,
							(LPBYTE)crashDoctorDebugger.c_str(),
							(DWORD)(crashDoctorDebugger.length() * sizeof(TCHAR)));

	if (nReturnValue != ERROR_SUCCESS)
	{
		goto funcExit;
	}

	// we don't care failure of setting this value
	RegSetValueEx(
			hAeDebugKey,
			_T("Auto"),
			0,
			REG_SZ,
			(LPBYTE)_T("1"),
			sizeof(_T("1")) * sizeof(TCHAR));
	

	//
	// Add existing debugger keys to Application Info
	//
	if (!currentDebugger.empty())
	{
		DWORD numDebuggers;
		tempSize = sizeof(numDebuggers);
		
		nReturnValue = RegQueryValueEx(
									hAppDebuggerKey,
									_T("Count"),
									NULL,
									NULL,
									(LPBYTE)&numDebuggers,
									&tempSize);

		if (nReturnValue != ERROR_SUCCESS)
		{
			numDebuggers = 1;

			RegSetValueEx(
					hAppDebuggerKey,
					_T("Count"),
					0,
					REG_DWORD,
					(LPBYTE)&numDebuggers,
					sizeof(numDebuggers));
		}

		RegSetValueEx(
				hAppDebuggerKey,
				_T("Debugger00"),
				0,
				REG_SZ,
				(LPBYTE)currentDebugger.c_str(),
				(DWORD)(currentDebugger.length() * sizeof(TCHAR)));

		RegSetValueEx(
				hAppDebuggerKey,
				_T("PreCrashDoctorDebugger"),
				0,
				REG_SZ,
				(LPBYTE)currentDebugger.c_str(),
				(DWORD)(currentDebugger.length() * sizeof(TCHAR)));
	}

	//
	// Add CrashDoctor Installation information
	//
	RegSetValueEx(
				hAppInfoKey,
				_T("InstallDir"),
				0,
				REG_SZ,
				(LPBYTE)m_InstallPath.c_str(),
				(DWORD)(m_InstallPath.length() * sizeof(TCHAR)));

	RegSetValueEx(
				hAppInfoKey,
				_T("StartMenuDir"),
				0,
				REG_SZ,
				(LPBYTE)m_StartMenuPath.c_str(),
				(DWORD)(m_StartMenuPath.length() * sizeof(TCHAR)));

	funcStatus = true;

funcExit:

	if (hAppDebuggerKey)
	{
		RegCloseKey(hAppDebuggerKey);
	}

	if (hAppInfoKey)
	{
		RegCloseKey(hAppInfoKey);
	}

	if (hAeDebugKey)
	{
		RegCloseKey(hAeDebugKey);
	}

	if (funcStatus == false)
	{
		cdHandleError(
			m_Hwnd,
			GetLastError(),
			_T("Installation failed. Unable to create/update registry ")
			_T("keys."));
	}

	return funcStatus;
}



void
CInstallCrashDoctor::RestoreRegistry(
	LPCTSTR inAppRoot,
	LPCTSTR inAeDebug)
{
	tstring originalDebugger;

	tstring tempString;
	TCHAR tempPath[MAX_PATH];
	DWORD tempSize = 0;

	int nReturnValue		= 0;
	HKEY hAeDebugKey		= NULL;
	HKEY hAppInfoKey		= NULL;
	HKEY hAppDebuggerKey	= NULL;

	tstring debuggersKey;

	nReturnValue = RegOpenKey(
							HKEY_LOCAL_MACHINE,
							inAeDebug,
							&hAeDebugKey);

	nReturnValue = RegOpenKey(
							HKEY_LOCAL_MACHINE,
							inAppRoot,
							&hAppInfoKey);

	debuggersKey = inAppRoot;

	nReturnValue = RegCreateKey(
							HKEY_LOCAL_MACHINE,
							(debuggersKey + REG_APP_DEBUGGERS).c_str(),
							&hAppDebuggerKey);

	if (nReturnValue != ERROR_SUCCESS)
	{
		cdHandleError(
					m_Hwnd,
					GetLastError(),
					_T("Installation failed. Could not create registry keys."));

		goto funcExit;
	}

	if (!hAeDebugKey || !hAppInfoKey || !hAppDebuggerKey)
	{
		goto funcExit;
	}


	tempSize = sizeof(tempPath) * sizeof(TCHAR);
	nReturnValue = RegQueryValueEx(
							hAppDebuggerKey,
							_T("PreCrashDoctorDebugger"),
							NULL,
							NULL,
							(LPBYTE)tempPath,
							&tempSize);

	if (nReturnValue == ERROR_SUCCESS)
	{
		originalDebugger = tempPath;
	}


	//
	// Restore original debugger, ignore the error if it can't be restored
	//
	RegSetValueEx(
			hAeDebugKey,
			_T("Debugger"),
			0,
			REG_SZ,
			(LPBYTE)originalDebugger.c_str(),
			(DWORD)(originalDebugger.length() * sizeof(TCHAR)));
	
	RegSetValueEx(
			hAeDebugKey,
			_T("Auto"),
			0,
			REG_SZ,
			(LPBYTE)_T("0"),
			sizeof(_T("0")) * sizeof(TCHAR));

funcExit:

	if (hAppInfoKey)
	{
		RegCloseKey(hAppInfoKey);
	}

	if (hAeDebugKey)
	{
		RegCloseKey(hAeDebugKey);
	}

	if (hAppDebuggerKey)
	{
		RegCloseKey(hAppDebuggerKey);
	}

	SHDeleteKey(HKEY_LOCAL_MACHINE, inAppRoot);
}



void
CInstallCrashDoctor::PreparePaths()
{
	tstring startMenuPath;
	tstring installRoot;

	tstring tempString;
	TCHAR tempPath[MAX_PATH];
	DWORD tempSize = 0;

	int nReturnValue		= 0;
	HKEY hAppInfoKey		= NULL;

	m_InstallPath = _T("");
	m_StartMenuPath = _T("");

	nReturnValue = RegOpenKey(
							HKEY_LOCAL_MACHINE,
							REG_APP_ROOT,
							&hAppInfoKey);

	if (hAppInfoKey)
	{
		tempSize = sizeof(tempPath) * sizeof(TCHAR);
		nReturnValue = RegQueryValueEx(
								hAppInfoKey,
								_T("InstallDir"),
								NULL,
								NULL,
								(LPBYTE)tempPath,
								&tempSize);

		if (nReturnValue == ERROR_SUCCESS)
		{
			m_InstallPath = tempPath;
		}

		tempSize = sizeof(tempPath) * sizeof(TCHAR);
		nReturnValue = RegQueryValueEx(
								hAppInfoKey,
								_T("StartMenuDir"),
								NULL,
								NULL,
								(LPBYTE)tempPath,
								&tempSize);

		if (nReturnValue == ERROR_SUCCESS)
		{
			m_StartMenuPath = tempPath;
		}

		RegCloseKey(hAppInfoKey);
	}

	if (m_InstallPath.empty())
	{
		if (SHGetFolderPath(
					m_Hwnd,
					CSIDL_PROGRAM_FILES | CSIDL_FLAG_CREATE,
					NULL,
					SHGFP_TYPE_CURRENT,
					tempPath) == S_OK)
		{
			installRoot = tempPath;
		}
		else
		{
			GetWindowsDirectory(
							tempPath,
							MAX_PATH);

			installRoot = tempPath;
		}

		m_InstallPath = installRoot + _T("\\CrashDoctor\\");
	}


	if (m_StartMenuPath.empty())
	{
		//
		// First get the path which exist on both
		// 9x and NT i.e. per user start menu path
		//
		if (SHGetFolderPath(
						m_Hwnd,
						CSIDL_PROGRAMS | CSIDL_FLAG_CREATE,
						NULL,
						SHGFP_TYPE_CURRENT,
						tempPath) == S_OK)
		{
			startMenuPath = tempPath;
		}

		if (gOSVersion == WIN32_NT)
		{
			if (SHGetFolderPath(
							m_Hwnd,
							CSIDL_COMMON_PROGRAMS | CSIDL_FLAG_CREATE,
							NULL,
							SHGFP_TYPE_CURRENT,
							tempPath) == S_OK)
			{
				startMenuPath = tempPath;
			}
		}

		m_StartMenuPath	= startMenuPath + _T("\\CrashDoctor");
	}

	m_ExePathTarget32		= m_InstallPath + _T("CrashDoctor.exe");
	m_ExePathTarget64		= m_InstallPath + _T("CrashDoctor64.exe");
	m_ChmPathTarget			= m_InstallPath + _T("Info.txt");
	m_PsapiPathTarget		= m_InstallPath + _T("psapi.dll");
	m_ShFolderPathTarget	= m_InstallPath + _T("shfolder.dll");
	m_DbgHelpPathTarget		= m_InstallPath + _T("dbghelp.dll");	

#if defined(_M_IX86)
	m_ExePathNative			= m_ExePathTarget32;
#elif defined(_M_AMD64)
	m_ExePathNative			= m_ExePathTarget64;
#endif
}



INT_PTR
CALLBACK
InstallDlgProc(
	HWND hDlg,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam)
/*++

Routine Description:
	
	Dialog box which will be used to install or uninstall
	CrashDoctor application

Returns:

	TRUE - Handled, FALSE - Not handled

--*/
{
	//
	// dlgResult will be used to return value when the
	// dialog terminates. This value can be used by the
	// function which created the dialog to see if user
	// pressed Ok or Cancel
	//
	int dlgResult = 0;

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

			CInstallCrashDoctor installCrashDoctor(hDlg);

			bool bInstalled = false;
			
			bInstalled = (
						installCrashDoctor.IsInstalled(REG_AEDEBUG_PATH) ||
						installCrashDoctor.IsInstalled(REG_AEDEBUG_PATH_6432));

			if (bInstalled)
			{
				EnableWindow(GetDlgItem(hDlg, IDC_BTN_INSTALL), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_BTN_UNINSTALL), TRUE);
			}
			else
			{
				EnableWindow(GetDlgItem(hDlg, IDC_BTN_UNINSTALL), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_BTN_INSTALL), TRUE);
			}

			return TRUE;
		}
		case WM_COMMAND:
		{
			switch(wParam)
			{
				case IDC_BTN_INSTALL:
				{
					CInstallCrashDoctor installCrashDoctor(hDlg);
					installCrashDoctor.Install();

					bool bInstalled = false;
			
					bInstalled = (
						installCrashDoctor.IsInstalled(REG_AEDEBUG_PATH) ||
						installCrashDoctor.IsInstalled(REG_AEDEBUG_PATH_6432));

					if (bInstalled)
					{
						EnableWindow(GetDlgItem(hDlg, IDC_BTN_INSTALL), FALSE);
						EnableWindow(GetDlgItem(hDlg, IDC_BTN_UNINSTALL), TRUE);
					}
					else
					{
						EnableWindow(GetDlgItem(hDlg, IDC_BTN_UNINSTALL), FALSE);
						EnableWindow(GetDlgItem(hDlg, IDC_BTN_INSTALL), TRUE);
					}
					return TRUE;
				}
				case IDC_BTN_UNINSTALL:
				{
					CInstallCrashDoctor installCrashDoctor(hDlg);
					installCrashDoctor.Uninstall();

					bool bInstalled = false;
			
					bInstalled = (
						installCrashDoctor.IsInstalled(REG_AEDEBUG_PATH) ||
						installCrashDoctor.IsInstalled(REG_AEDEBUG_PATH_6432));

					if (bInstalled)
					{
						EnableWindow(GetDlgItem(hDlg, IDC_BTN_INSTALL), FALSE);
						EnableWindow(GetDlgItem(hDlg, IDC_BTN_UNINSTALL), TRUE);
					}
					else
					{
						EnableWindow(GetDlgItem(hDlg, IDC_BTN_UNINSTALL), FALSE);
						EnableWindow(GetDlgItem(hDlg, IDC_BTN_INSTALL), TRUE);
					}
					return TRUE;
				}
				case IDC_BTN_CLOSE:
				{
					EndDialog(hDlg, dlgResult);
                    return TRUE;
				}
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
