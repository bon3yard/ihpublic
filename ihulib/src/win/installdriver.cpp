/*++

Copyright (c) 2011, Pankaj Garg (pankajgarg@intellectualheaven.com).
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

Module Name
    
    installdriver.cpp

Module Description
            
    Win32 routines to install and uninstall a dynamically
    loadable device driver.

    A dynamically loadable windows device driver can be
    installed on the fly by using service manager apis.

--*/
  
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ihulib.h"

BOOL 
__cdecl
IhuInstallDriver(
    IN LPCWSTR    DriverName,
    IN LPCWSTR    ServiceExe)
{
    IHU_SERVICE_MANAGER serviceManager;

    BOOL bResult = FALSE;

    bResult = serviceManager.OpenSCManager(SC_MANAGER_ALL_ACCESS);

    if (bResult == FALSE)
    {
        //IHU_DBG_LOGW(IHU_, IHU_LEVEL_ERROR, (L"OpenSCManagerW failed. GetLastError = 0x%x\n", GetLastError()));
        goto FuncEnd;
    }

    bResult = serviceManager.CreateService(
                                        DriverName,
                                        DriverName,
                                        SERVICE_ALL_ACCESS,
                                        SERVICE_KERNEL_DRIVER,
                                        SERVICE_DEMAND_START,
                                        SERVICE_ERROR_NORMAL,
                                        ServiceExe);

    if (bResult == FALSE)
    {
        //IHU_DBG_LOGW(IHU_, IHU_LEVEL_ERROR, (L"CreateService failed. GetLastError = 0x%x\n", GetLastError()));
        goto FuncEnd;
    }

    bResult = TRUE;

FuncEnd:
    return bResult;
}
 

BOOL
__cdecl
IhuRemoveDriver(
    IN LPCWSTR DriverName)
{
    IHU_SERVICE_MANAGER serviceManager;

    BOOL bResult = FALSE;

    bResult = serviceManager.OpenSCManager(SC_MANAGER_ALL_ACCESS);

    if (bResult == FALSE)
    {
        //IHU_DBG_LOGW(IHU_, IHU_LEVEL_ERROR, (L"OpenSCManagerW failed. GetLastError = 0x%x\n", GetLastError()));
        goto FuncEnd;
    }

    bResult = serviceManager.OpenService(
                                    DriverName, 
                                    SERVICE_ALL_ACCESS);

    if (bResult == FALSE)
    {
        //IHU_DBG_LOGW(IHU_, IHU_LEVEL_ERROR, (L"OpenService failed. GetLastError = 0x%x\n", GetLastError()));
        goto FuncEnd;
    }

    bResult = serviceManager.DeleteService();

    if (bResult == FALSE)
    {
        //IHU_DBG_LOGW(IHU_, IHU_LEVEL_ERROR, (L"DeleteService failed. GetLastError = 0x%x\n", GetLastError()));
        goto FuncEnd;
    }

    bResult = TRUE;

FuncEnd:
    return bResult;
}


BOOL
__cdecl
IhuStartDriver(
    IN LPCWSTR DriverName)
{
    IHU_SERVICE_MANAGER serviceManager;

    BOOL bResult = FALSE;

    bResult = serviceManager.OpenSCManager(SC_MANAGER_ALL_ACCESS);

    if (bResult == FALSE)
    {
        //IHU_DBG_LOGW(IHU_, IHU_LEVEL_ERROR, (L"OpenSCManagerW failed. GetLastError = 0x%x\n", GetLastError()));
        goto FuncEnd;
    }

    bResult = serviceManager.OpenService(
                                        DriverName, 
                                        SERVICE_ALL_ACCESS);

    if (bResult == FALSE)
    {
        //IHU_DBG_LOGW(IHU_, IHU_LEVEL_ERROR, (L"OpenService failed. GetLastError = 0x%x\n", GetLastError()));
        goto FuncEnd;
    }

    bResult = serviceManager.StartService();

    if (bResult == FALSE)
    {
        //IHU_DBG_LOGW(IHU_, IHU_LEVEL_ERROR, (L"StartService failed. GetLastError = 0x%x\n", GetLastError()));
        goto FuncEnd;
    }

    bResult = TRUE;

FuncEnd:
    return bResult;
}



BOOL
__cdecl
IhuStopDriver(
    IN LPCWSTR DriverName)
{
    IHU_SERVICE_MANAGER serviceManager;

    BOOL bResult = FALSE;

    bResult = serviceManager.OpenSCManager(SC_MANAGER_ALL_ACCESS);

    if (bResult == FALSE)
    {
        //IHU_DBG_LOGW(IHU_, IHU_LEVEL_ERROR, (L"OpenSCManagerW failed. GetLastError = 0x%x\n", GetLastError()));
        goto FuncEnd;
    }

    bResult = serviceManager.OpenService(
                                        DriverName, 
                                        SERVICE_ALL_ACCESS);

    if (bResult == FALSE)
    {
        //IHU_DBG_LOGW(IHU_, IHU_LEVEL_ERROR, (L"OpenService failed. GetLastError = 0x%x\n", GetLastError()));
        goto FuncEnd;
    }

    bResult = serviceManager.StopService();

    if (bResult == FALSE)
    {
        //IHU_DBG_LOGW(IHU_, IHU_LEVEL_ERROR, (L"StopService failed. GetLastError = 0x%x\n", GetLastError()));
        goto FuncEnd;
    }

    bResult = TRUE;

FuncEnd:
    return bResult;
}

