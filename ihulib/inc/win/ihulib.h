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

/**
 * @file    ihulib.h
 * 
 * @brief   The helper library for various intellectualheaven.com projects.
 *
 **/

#ifndef _IHULIB_H_
#define _IHULIB_H_

#include <string>
#include <vector>
#include <windows.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
* @addtogroup  DebugHelper Debugging Helper Functions.
*
* @brief       Functions and Macros are useful for debugging.
*
*              An application can use these macros to do tracing
*              logging and other debugging actions.
*
* @{
**/
#define IHU_LEVEL_FATAL     1
#define IHU_LEVEL_ERROR     11
#define IHU_LEVEL_WARN      21
#define IHU_LEVEL_INFO      31
#define IHU_LEVEL_LOUD      41
#define IHU_LEVEL_FLOOD     51

//! Macro for turning Unit level logging ON
#define IHU_LOGGING_ON      1

//! Macro for turning Unit level logging OFF
#define IHU_LOGGING_OFF     0

/*
#define HX_DBG_ENTER                HX_DBG_ENTERW
#define HX_DBG_LEAVE                HX_DBG_LEAVEW
#define HX_DBG_FN_ENTER             HX_DBG_FN_ENTERW
#define HX_DBG_FN_LEAVE             HX_DBG_FN_LEAVEW
*/

typedef void(__cdecl *PFN_IHU_DBG_LOG)(LPCWSTR fmt, ...);

// Global debug logging function
extern PFN_IHU_DBG_LOG _IhuDbgLogFn;

// Global debug logging level
extern DWORD _IhuDbgLogLevel;

#define IHU_DBG_LOG(unit, level, format)        \
{                                               \
    if (level <= _IhuDbgLogLevel &&            \
        unit == IHU_LOGGING_ON)                 \
    {                                           \
        (*_IhuDbgLogFn)(L"[U] - ");               \
        (*_IhuDbgLogFn) format;                  \
    }                                           \
}

//! Function to configure debug logging attributes
void IhuSetDbgLogLevel(DWORD DebugLogLevel);
DWORD IhuGetDbgLogLevel();
void __cdecl IhuDbgLog(LPCWSTR sFormat, ...);

/**
* @addtogroup  VersionHelpers Version related macros.
*
* @brief       These macros are useful for version related management.
*
*              An application can use these macros to define version
*              strings and version numbers.
*
* @{
**/
#define IHU_MAKE_STR(_X_)           IHU_MAKE_STR_REAL(_X_)
#define IHU_MAKE_STR_REAL(_X_)      #_X_

#define IHU_CASTBYTE(b) ((DWORD)(b) & 0xFF)

#define IHU_DWORD_VERSION(VER_MAJOR, VER_MINOR, VER_BUILD, VER_STEP)    \
            (IHU_CASTBYTE(VER_MAJOR) << 24 | \
             IHU_CASTBYTE(VER_MINOR) << 16 | \
             IHU_CASTBYTE(VER_BUILD) << 8  | \
             IHU_CASTBYTE(VER_STEP))

/**
 * @addtogroup  DriverInstallation Driver Installation Functions
 *
 * @brief       Win32 routines to facilitate easy installation and
 *              uninstallation of a dynamically loadable driver.
 *
 *              A dynamically loadable windows device driver can be
 *              installed on the fly by using service manager apis.
 *
 * @{
 **/

/**
 * @brief   Install a dynamically loadable device driver
 *
 * @param   DriverName - User defined name of the driver 
 * @param   ServiceExe - Full path of the driver binary
 *  
 * @return  TRUE/FALSE
 **/
BOOL
__cdecl 
IhuInstallDriver(
    LPCWSTR DriverName,
    LPCWSTR ServiceExe
    );


/**
 * @brief   Remove/Uninstall a device driver
 *
 * @param   DriverName - User defined name of the driver 
 *  
 * @return  TRUE/FALSE
 **/
BOOL
__cdecl
IhuRemoveDriver(
    LPCWSTR DriverName
    );


/**
 * @brief   Starts the execution of driver
 *
 * @param   DriverName - User defined name of the driver 
 *  
 * @return  TRUE/FALSE
 **/
BOOL
__cdecl
IhuStartDriver(
    LPCWSTR DriverName
    );


/**
 * @brief   Stops the execution of driver
 *
 * @param   DriverName - User defined name of the driver 
 *  
 * @return  TRUE/FALSE
 **/
BOOL
__cdecl
IhuStopDriver(
    LPCWSTR DriverName
    );


/**
 * @addtogroup  GenericUtil General Purpose Utility Functions
 *
 * @brief       General purpose utility functions for Windows.
 * 
 * @{
 **/

DWORD
__cdecl
IhuGetProcessIcon(
   UINT32 inProcessId,
   HICON &ohIcon
   );

DWORD
__cdecl
IhuGetFileIcon(
    std::wstring    inFilePath,
    HICON           &ohIcon
    );


/**
 * @addtogroup  ProcessAndThreadMgmt Process and Thread Management
 *
 * @brief       Provides functions for process and thread
 *              management. 
 * 
 * @warning     These routines are dependent upon psapi.lib which is
 *              only availalbe on Windows NT, 2000, XP and 2003, so
 *              these routines will not work on Windows 9x series
 *              and Windows ME.
 *
 * @{
 **/

//! Stores process information
typedef struct _IHU_PROCESS_INFO
{
    //! contains the ID of the process
    UINT32              mProcessId;

    //! contains the process name
    std::wstring        mProcessName;

    //! contains the full path to the process binary
    std::wstring        mBinaryName;

}IHU_PROCESS_INFO;

//! Stores modules information of a process
typedef struct _IHU_MODULE_INFO
{
    //! contains handle to the module
    HANDLE              mModuleHandle;

    //! contains the name of module
    std::wstring        mModuleBaseName;

    //! contains the base address where module is loaded
    LPVOID              mModuleBaseAddress;

}IHU_MODULE_INFO;

//! Stores a list of processes and their information
typedef std::vector<IHU_PROCESS_INFO>       IHU_PROCESS_LIST;

//! Iterator for IHU_PROCESS_LIST
typedef IHU_PROCESS_LIST::iterator          IHU_PROCESS_LIST_ITER;

//! Stores a list of modules and their information
typedef std::vector<IHU_MODULE_INFO>        IHU_MODULE_LIST;

//! Iterator for IHU_MODULE_LIST
typedef IHU_MODULE_LIST::iterator           IHU_MODULE_LIST_ITER;

/**
 * @brief   Get the list of running processes.
 *
 *          This function takes a snapshot of the running processes
 *          at the time of call to this function. The processes may
 *          not exist by the time you try to use the process id. So
 *          it will be a good idea to do a reality check if the
 *          process exist or not, by calling OpenProcess with
 *          mProcessId as parameter, before using it.
 *
 * @param   oProcessList - Stores the list of all running processes 
 *  
 * @return  If the function succeed, the return value is = 0.
 *          If the function fails, the return value is non-zero.
 *
 * @remarks In case of failure, call GetLastError for more information.
 **/
DWORD
__cdecl
IhuGetProcessList(
    IHU_PROCESS_LIST &oProcessList
    );

/**
 * @brief   Get the list of loaded modules in a process.
 *
 *          This function takes a snapshot of the loaded modules
 *          in a process at the time of this call.
 *
 * @param   inProcessId - Id to the process
 * @param   oModuleList - Stores the list of loaded modules
 *  
 * @return  If the function succeed, the return value is = 0.
 *          If the function fails, the return value is non-zero.
 *
 * @remarks In case of failure, call GetLastError for more information.
 **/
DWORD
__cdecl
IhuGetModuleList(
    DWORD inProcessId,
    IHU_MODULE_LIST &oModuleList
    );


/**
 * @addtogroup  ServiceManager Service Manager Functions
 *
 * @brief       Provides abstraction for easy use of Service Manger
 *              functions.
 * 
 * @warning     Service manager is only available for Windows NT
 *              series of OS, so these routines will not work
 *              on Windows 9x series and ME.
 * 
 * @{
 **/

//! A wrapper class for service manager APIs.
class IHU_SERVICE_MANAGER
{
public:

    /**
     * @brief   constructor
     **/
    IHU_SERVICE_MANAGER();


    /**
     * @brief   destructor
     **/
    ~IHU_SERVICE_MANAGER();


    /**
     * @brief   It is a wrapper around OpenSCManager
     *
     *          Call to this function, if successful will open a handle to
     *          service manager. The handle will be automatically closed
     *          when the object of this class is destroyed.
     *
     * @param   * - Please refer to documentation of OpenSCManager
     *          in MSDN
     *  
     * @return  If the function succeed, the return value is true.
     *          If the function fails, the return value is false.
     **/
    bool
    OpenSCManager(
        DWORD desiredAccess,
        LPCWSTR machineName = NULL,
        LPCWSTR databaseName = NULL);


    /**
     * @brief   It is a wrapper around CreateService
     *
     *          It will create a new service in service manager
     *          database, if successful.
     *
     * @param   * - Please refer to documentation of CreateService
     *          in MSDN
     *  
     * @return  If the function succeed, the return value is true.
     *          If the function fails, the return value is false.
     **/
    bool
    CreateService(
        LPCWSTR serviceName,
        LPCWSTR displayName,
        DWORD desiredAccess,
        DWORD serviceType,
        DWORD startType,
        DWORD errorControl,
        LPCWSTR binaryPathName = NULL,
        LPCWSTR loadOrderGroup = NULL,
        LPDWORD tagID = NULL,
        LPCWSTR dependencies = NULL,
        LPCWSTR serviceStartName = NULL,
        LPCWSTR password = NULL);


    /**
     * @brief   It is a wrapper around OpenService
     *
     *          It will open a handle to a service which already
     *          exist in service manager database, if successful.
     *
     * @param   * - Please refer to documentation of CreateService
     *          in MSDN
     *  
     * @return  If the function succeed, the return value is true.
     *          If the function fails, the return value is false.
     **/
    bool
    OpenService(
        LPCWSTR serviceName,
        DWORD desiredAccess);

    /**
     * @brief   It is a wrapper around DeleteService
     *
     *          It will delete a service from service manager
     *          database, if successful.
     *
     * @param   none
     *  
     * @return  If the function succeed, the return value is true.
     *          If the function fails, the return value is false.
     **/
    bool
    DeleteService();


    /**
     * @brief   It is a wrapper around CloseServiceHandle
     *
     *          It will close the handle to already open service,
     *          if successful.
     *
     * @param   none
     *  
     * @return  If the function succeed, the return value is true.
     *          If the function fails, the return value is false.
     **/
    bool
    CloseService();


    /**
     * @brief   It is a wrapper around CloseServiceHandle
     *
     *          It will close the handle already opened by a previous
     *          call to OpenSCManager, if successful.
     *
     * @param   none
     *  
     * @return  If the function succeed, the return value is true.
     *          If the function fails, the return value is false.
     **/
    bool
    CloseSCManager();


    /**
     * @brief   It is a wrapper around StartService
     *
     *          This routine starts an installed service. It is caller's
     *          responsibility to make sure that the service is opened,
     *          before attempting to start it.
     *
     * @param   * - Please refer to documentation of CreateService
     *          in MSDN
     *  
     * @return  If the function succeed, the return value is true.
     *          If the function fails, the return value is false.
     **/
    bool
    StartService(
        DWORD numServiceArgs = 0,
        LPCWSTR* serviceArgsVector = NULL);
    

    /**
     * @brief   It is a wrapper around StopService
     *
     *          This routine attempts to stop a running service.
     *
     * @param   * - Please refer to documentation of CreateService
     *          in MSDN
     *  
     * @return  If the function succeed, the return value is true.
     *          If the function fails, the return value is false.
     **/
    bool
    StopService(
        LPSERVICE_STATUS serviceStatus = NULL);


private:
    /*
     * Private datatypes for managing service
     */
    SC_HANDLE   mService;
    SC_HANDLE   mSCManager;
};


#ifdef __cplusplus
}
#endif

#endif // _IHULIB_H_
