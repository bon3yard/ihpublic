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

    miscutil.cpp

Module Description:

    Implements misc general purpose utility functions for Windows

--*/

#include <windows.h>
#include <shellapi.h>
#include <objbase.h>
#include "ihulib.h"


DWORD
__cdecl
IhuGetProcessIcon(
   UINT32   inProcessId,
   HICON    &ohIcon)
{
    DWORD   valueReturn = 0;

    goto FuncEnd;

FuncEnd:
    return valueReturn;
}


DWORD
__cdecl
IhuGetFileIcon(
    std::wstring    inFilePath,
    HICON           &ohIcon)
{
    DWORD   valueReturn = 0;

    CoInitialize(NULL);

    SHFILEINFOW shellFileInfo;

    if (SHGetFileInfoW(
            inFilePath.c_str(),
            0,
            &shellFileInfo,
            sizeof(shellFileInfo),
            SHGFI_ICON))
    {
        ohIcon = shellFileInfo.hIcon;
    }
    else
    {
        valueReturn = GetLastError();
    }

    CoUninitialize();

    goto FuncEnd;

FuncEnd:
    return valueReturn;
}

