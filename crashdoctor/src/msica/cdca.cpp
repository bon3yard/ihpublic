
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

    WcaLog(LOGMSG_STANDARD,
        "CrashDoctor: Restoring Registry=%S, with Value=%S.",
        AeDebugPath.c_str(),
        OrigDebugString.c_str());

    nReturnValue = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        AeDebugPath.c_str(),
        0,
        KEY_ALL_ACCESS,
        &hAeDebugKey);

    if (hAeDebugKey == NULL)
    {
        WcaLog(LOGMSG_STANDARD,
            "CrashDoctor: Failed to open registry key=%S, Error=%d.",
            AeDebugPath.c_str(),
            nReturnValue);
        goto funcExit;
    }

    //
    // Example on how to pop-up message boxes. Very useful for debugging.
    //
    // ::MessageBox(NULL,
    //      OrigDebugString.c_str(),
    //      AeDebugPath.c_str(),
    //      MB_OK | MB_ICONINFORMATION);

    nReturnValue = RegSetValueEx(
        hAeDebugKey,
        L"Debugger",
        0,
        REG_SZ,
        (LPBYTE)OrigDebugString.c_str(),
        (DWORD)(OrigDebugString.length() * sizeof(TCHAR)));

    if (nReturnValue != ERROR_SUCCESS)
    {
        WcaLog(
            LOGMSG_STANDARD,
            "CrashDoctor: Failed to set registry=%S\\Debugger, Error=%d.",
            AeDebugPath.c_str(),
            nReturnValue);
        goto funcExit;
    }

    nReturnValue = RegSetValueEx(
        hAeDebugKey,
        L"Auto",
        0,
        REG_SZ,
        (LPBYTE)L"1",
        sizeof(L"1"));

    if (nReturnValue != ERROR_SUCCESS)
    {
        WcaLog(
            LOGMSG_STANDARD,
            "CrashDoctor: Failed to set registry=%S\\Auto, Error=%d.",
            AeDebugPath.c_str(),
            nReturnValue);
        goto funcExit;
    }

funcExit:

    if (hAeDebugKey != NULL)
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
    if (FAILED(hr))
    {
        WcaLog(LOGMSG_STANDARD, "CrashDoctor: WcaInitialize Failed:%x", hr);
        goto Exit;
    }

	WcaLog(LOGMSG_STANDARD, "CrashDoctor: Initialized.");

    // TODO: Add your custom action code here.

    WCHAR szActionData[MAX_PATH * 2] = { 0 };
    DWORD size;

    size = (sizeof(szActionData) / sizeof(WCHAR)) - sizeof(WCHAR);
    MsiGetProperty(hInstall, L"CustomActionData", szActionData, &size);
    WcaLog(LOGMSG_STANDARD, "CrashDoctor: CustomActionData=%S.", szActionData);

    RestoreRegistry(szActionData);


Exit:

    //
    // We always return success because our custom action runs after
    // uninstall is done so failing is useless at this point.
    //
	return WcaFinalize(ERROR_SUCCESS);
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
