
#include "targetver.h"
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <msiquery.h>
#include <wcautil.h>
#include <vector>
#include <string>

using namespace std;

#define REG_AEDEBUG_PATH		L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug"
#define REG_AEDEBUG_PATH_6432	L"SOFTWARE\\Wow6432Node\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug"

vector<wstring> 
StringSplit(LPCWSTR str)
{
    vector<wstring> result;

    do
    {
        LPCWSTR begin = str;

        while (*str != L';' && *str != '=' && *str)
            str++;

        result.push_back(wstring(begin, str));

    } while (*str++ != 0);

    return result;
}

VOID
RestoreRegistry(wstring OrigDebugString, wstring AeDebugPath)
{
    int nReturnValue = 0;
    HKEY hAeDebugKey = NULL;

    nReturnValue = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        AeDebugPath.c_str(),
        0,
        KEY_ALL_ACCESS,
        &hAeDebugKey);

    if (hAeDebugKey == NULL)
    {
        goto funcExit;
    }

    ::MessageBox(NULL, OrigDebugString.c_str(), AeDebugPath.c_str(), MB_OK | MB_ICONINFORMATION);

    nReturnValue = RegSetValueEx(
        hAeDebugKey,
        L"Debugger",
        0,
        REG_SZ,
        (LPBYTE)OrigDebugString.c_str(),
        (DWORD)(OrigDebugString.length() * sizeof(TCHAR)));

    if (nReturnValue != ERROR_SUCCESS)
    {
        wchar_t buff[100];
        wsprintf(buff, L"%d", nReturnValue);
        ::MessageBox(NULL, buff, AeDebugPath.c_str(), MB_OK | MB_ICONINFORMATION);
    }

    RegSetValueEx(
        hAeDebugKey,
        L"Auto",
        0,
        REG_SZ,
        (LPBYTE)L"1",
        sizeof(L"1"));

    ::MessageBox(NULL, OrigDebugString.c_str(), L"Done.", MB_OK | MB_ICONINFORMATION);

funcExit:

    if (hAeDebugKey)
    {
        RegCloseKey(hAeDebugKey);
    }
}

VOID
RestoreRegistry(LPCWSTR OrigDebugStrings)
{
    vector<wstring> strList;
    
    strList = StringSplit(OrigDebugStrings);

    if (strList.size() < 4)
    {
        goto funcExit;
    }

    if (strList[0] != L"DEBUGSTRING" || strList[2] != L"DEBUGSTRING32")
    {
        goto funcExit;
    }

    ::MessageBox(NULL, L"Input Validated", L"Deferred Custom Action", MB_OK | MB_ICONINFORMATION);

    if (!strList[1].empty())
    {   
        RestoreRegistry(strList[1], REG_AEDEBUG_PATH);
    }

    if (!strList[3].empty())
    {
        RestoreRegistry(strList[3], REG_AEDEBUG_PATH_6432);
    }

funcExit:

    return;
}

UINT __stdcall CrashDoctorRestoreRegistry(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;

	hr = WcaInitialize(hInstall, "CrashDoctorRestoreRegistry");
	ExitOnFailure(hr, "Failed to initialize");

	WcaLog(LOGMSG_STANDARD, "Initialized.");

    // TODO: Add your custom action code here.

    WCHAR szActionData[MAX_PATH * 2] = { 0 };
    DWORD size;

    size = sizeof(szActionData) / sizeof(WCHAR);
    MsiGetProperty(hInstall, L"CustomActionData", szActionData, &size);

    ::MessageBox(NULL, szActionData, L"Deferred Custom Action", MB_OK | MB_ICONINFORMATION);

    RestoreRegistry(szActionData);

LExit:
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}


extern "C" BOOL WINAPI DllMain(
	__in HINSTANCE hInst,
	__in ULONG ulReason,
	__in LPVOID
	)
{
	switch(ulReason)
	{
	case DLL_PROCESS_ATTACH:
		WcaGlobalInitialize(hInst);
		break;

	case DLL_PROCESS_DETACH:
		WcaGlobalFinalize();
		break;
	}

	return TRUE;
}
