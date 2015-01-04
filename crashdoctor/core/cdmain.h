/*++

Copyright (c) : Pankaj Garg & www.intellectualheaven.com

Module Name:

    cdmain.h

Module Description:

	Crash Doctor Header file for WinMain

Author:

    Pankaj Garg		2004-09-17
    
Revision History:

--*/

#ifndef _GUIDLGSKELETON_H_
#define _GUIDLGSKELETON_H_

// Local Include Files!!!
#include "cdutil.h"
#include "cdinstall.h"


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
extern OS_VERSION gOSVersion;
extern OS_EXACT_VERSION	gOSExactVersion;


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