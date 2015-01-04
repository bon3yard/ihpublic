/*++

Copyright (c) : Pankaj Garg & www.intellectualheaven.com

Module Name:

    debug.cpp

Module Description:

    Platform independent debugging module helper functions

Author:

    Pankaj Garg		2004-08-03
    
Revision History:

--*/

#include "ihulib.h"

extern "C"
{
    DWORD _IhuDbgLogLevel = IHU_LEVEL_ERROR;
    PFN_IHU_DBG_LOG _IhuDbgLogFn = IhuDbgLog;
}

void
IhuSetDbgLogLevel(DWORD DbgLogLevel)
{
    _IhuDbgLogLevel = DbgLogLevel;
}

DWORD
IhuGetDbgLogLevel()
{
    return _IhuDbgLogLevel;
}

/*++

Routine Name:

_IhuDbgLog

Routine Description:

Sends a formatted string to debugger. Their is no restriction on
the length of Format, but at maximum 512 characters are sent to
debugger with each call to this function and any trailing chacters
will be ignored.

Arguments:

sFormat - Format string
...		- Multiple arguments in printf style

Return Value:

None

--*/
void
__cdecl
IhuDbgLog(LPCWSTR sFormat, ...)
{
    const size_t	K_SIZE = 512;
    wchar_t			szMsg[K_SIZE];
    va_list			argList;

    va_start(argList, sFormat);

    _vsnwprintf(szMsg, K_SIZE - 1, sFormat, argList);

    OutputDebugStringW(szMsg);
}
