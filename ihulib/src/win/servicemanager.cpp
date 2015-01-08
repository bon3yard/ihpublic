/*++

Copyright (c) 2011, Pankaj Garg <pankaj@intellectualheaven.com>
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

/* ++

Module Name
    
    servicemanager.cpp

Module Description

    Provides routines to easily manage service manager
    by wrapping service manager APIs in a class.
    Service manager is only available for Windows NT
    series of OS, so these routines will not work
    on Windows 9x series and ME.

-- */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ihulib.h"

IHU_SERVICE_MANAGER::IHU_SERVICE_MANAGER()
{
    mSCManager = NULL;
    mService = NULL;
}


IHU_SERVICE_MANAGER::~IHU_SERVICE_MANAGER()
{
    if (mService)
    {
        CloseService();
    }

    if (mSCManager)
    {
        CloseSCManager();
    }
}


bool
IHU_SERVICE_MANAGER::OpenSCManager(
    DWORD           desiredAccess,
    LPCWSTR machineName,
    LPCWSTR databaseName)
{
    bool bResult    = false;

    mSCManager = ::OpenSCManagerW(
                                machineName,
                                databaseName,
                                desiredAccess);

    if (!mSCManager)
    {
        //IHU_DBG_LOGA(IHU_, IHU_LEVEL_ERROR, ("OpenSCManager Failed. GetLastError = 0x%x\n", GetLastError()));
        goto FuncEnd;
    }

    bResult = true;

FuncEnd:
    return bResult;
}



bool
IHU_SERVICE_MANAGER::CreateService(
    LPCWSTR serviceName,
    LPCWSTR displayName,
    DWORD           desiredAccess,
    DWORD           serviceType,
    DWORD           startType,
    DWORD           errorControl,
    LPCWSTR binaryPathName,
    LPCWSTR loadOrderGroup,
    LPDWORD         tagID,
    LPCWSTR dependencies,
    LPCWSTR serviceStartName,
    LPCWSTR password)
{
    //IHU_DBG_ASSERT(mSCManager && !mService);

    bool bResult    = false;

    mService = ::CreateServiceW(
                            mSCManager,
                            serviceName,
                            displayName,
                            desiredAccess,
                            serviceType,
                            startType,
                            errorControl,
                            binaryPathName,
                            loadOrderGroup,
                            tagID,
                            dependencies,
                            serviceStartName,
                            password);

    if (!mService)
    {
        //IHU_DBG_LOGA(IHU_, IHU_LEVEL_ERROR, ("CreateService Failed. GetLastError = 0x%x\n", GetLastError()));
        goto FuncEnd;
    }

    bResult = true;

FuncEnd:
    return bResult;
}



bool
IHU_SERVICE_MANAGER::OpenService(
    LPCWSTR serviceName,
    DWORD           desiredAccess)
{
    //IHU_DBG_ASSERT(mSCManager && !mService);

    bool bResult    = false;

    mService = ::OpenServiceW(
                            mSCManager,
                            serviceName,
                            desiredAccess);

    if (!mService)
    {
        //IHU_DBG_LOGA(IHU_, IHU_LEVEL_ERROR, ("OpenService Failed. GetLastError = 0x%x\n", GetLastError()));
        goto FuncEnd;
    }

    bResult = true;

FuncEnd:
    return bResult;
}



bool
IHU_SERVICE_MANAGER::DeleteService()
{
    //IHU_DBG_ASSERT(mSCManager && mService);

    bool bResult    = false;

    if (!::DeleteService(mService))
    {
        //IHU_DBG_LOGA(IHU_, IHU_LEVEL_ERROR, ("DeleteService Failed. GetLastError = 0x%x\n", GetLastError()));
        goto FuncEnd;
    }

    /*
     * We don't care for its return value :)
     */
    CloseService();

    bResult = true;

FuncEnd:
    return bResult;
}


bool
IHU_SERVICE_MANAGER::CloseService()
{
    //IHU_DBG_ASSERT(mSCManager && mService);

    bool bResult    = false;

    if (!::CloseServiceHandle(mService))
    {
        //IHU_DBG_LOGA(IHU_, IHU_LEVEL_ERROR, ("CloseService Failed. GetLastError = 0x%x\n", GetLastError()));
        goto FuncEnd;
    }

    mService    = NULL;
    bResult     = true;

FuncEnd:
    return bResult;
}



bool
IHU_SERVICE_MANAGER::CloseSCManager()
{
    //IHU_DBG_ASSERT(mSCManager);

    bool bResult    = false;

    if (!::CloseServiceHandle(mSCManager))
    {
        //IHU_DBG_LOGA(IHU_, IHU_LEVEL_ERROR, ("CloseSCManager Failed. GetLastError = 0x%x\n", GetLastError()));
        goto FuncEnd;
    }

    mSCManager  = NULL;
    bResult     = true;

FuncEnd:
    return bResult;
}



bool
IHU_SERVICE_MANAGER::StartService(
    DWORD numServiceArgs,
    LPCWSTR* serviceArgsVector)
{
    //IHU_DBG_ASSERT(mService);

    bool bResult    = false;

    BOOL bTempResult = ::StartServiceW(
                                    mService,
                                    numServiceArgs,
                                    (LPCWSTR *)serviceArgsVector);

    if (!bTempResult)
    {
        //IHU_DBG_LOGA(IHU_, IHU_LEVEL_ERROR, ("StartService Failed. GetLastError = 0x%x\n", GetLastError()));
        goto FuncEnd;
    }

    bResult = true;

FuncEnd:
    return bResult;
}




bool
IHU_SERVICE_MANAGER::StopService(
    LPSERVICE_STATUS inServiceStatus)
{
    //IHU_DBG_ASSERT(mService);

    bool bResult    = false;
    SERVICE_STATUS serviceStatus;

    if (inServiceStatus == NULL)
    {
        inServiceStatus = &serviceStatus;
    }

    BOOL bTempResult = ::ControlService(    
                            mService,
                            SERVICE_CONTROL_STOP,
                            inServiceStatus);

    if (!bTempResult)
    {
        if (GetLastError() != ERROR_SERVICE_NOT_ACTIVE)
        {
            //IHU_DBG_LOGA(IHU_, IHU_LEVEL_ERROR, ("StartService Failed. GetLastError = 0x%x\n", GetLastError()));
            goto FuncEnd;
        }
    }

    bResult = true;

FuncEnd:
    return bResult;
}

