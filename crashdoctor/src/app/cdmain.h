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

#ifndef _GUIDLGSKELETON_H_
#define _GUIDLGSKELETON_H_

// Local Include Files!!!
#include "cdutil.h"


enum OS_VERSION
{
	WIN32_9X,
	WIN32_NT
};

enum OS_EXACT_VERSION
{
	WIN32_95,
	WIN32_98,
	WIN32_ME,
	WIN32_NT4,
	WIN32_2K,
	WIN32_XP,
	WIN32_2K3,
	WIN32_LH
};

extern HINSTANCE ghInstance;

typedef struct tagPROC_DBG_DATA
{
	DWORD	processId;
	HANDLE	eventHandle;

}PROC_DBG_DATA, *PPROC_DBG_DATA;

bool
cdProcessArguments(
	int		argC,
	TCHAR	*argV[],
	DWORD	&oPID,
	HANDLE	&ohEvent);

#endif