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

/* ++

Module Name

    debug.cpp

Module Description

    Platform independent debugging module helper functions.

-- */

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
...     - Multiple arguments in printf style

Return Value:

None

--*/
void
__cdecl
IhuDbgLog(LPCWSTR sFormat, ...)
{
    const size_t    K_SIZE = 512;
    wchar_t         szMsg[K_SIZE];
    va_list         argList;

    va_start(argList, sFormat);

    _vsnwprintf(szMsg, K_SIZE - 1, sFormat, argList);

    OutputDebugStringW(szMsg);
}
