/*++

Copyright (c) : Pankaj Garg & www.intellectualheaven.com

Module Name:

    cdutil.cpp

Module Description:

	Implements the common utility functions required for CrashDoctor

Author:

    Pankaj Garg		2004-09-26
    
Most recent update:

    Pankaj Garg     2005-03-13

--*/

//System-Includes!!!
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <shlobj.h>
#include <objidl.h>

// Local-Includes!!!
#include "cdutil.h"


//
// Used to convert a character to uppercase
//
TO_UPPER gToUpper = cdToUpper;



TCHAR
__cdecl
cdToUpper(TCHAR c)
{
	return (TCHAR)_totupper(c);
}



void
CenterDialog(
	HWND hwndDlg)
/*++

Routine Description:
	
	Moves the dialog box to the center of its parent. If parent is NULL then
    dialog box is moved to the center of the desktop

Returns:

	None

--*/
{
	HWND hwndOwner;

	if ((hwndOwner = GetParent(hwndDlg)) == NULL) 
	{
		hwndOwner = GetDesktopWindow(); 
	}

	RECT rcOwner;
	RECT rcDlg;
	RECT rc;

	GetWindowRect(hwndOwner, &rcOwner); 
	GetWindowRect(hwndDlg, &rcDlg); 
	CopyRect(&rc, &rcOwner); 

	//
	// Offset the owner and dialog box rectangles so that 
	// right and bottom values represent the width and 
	// height, and then offset the owner again to discard 
	// space taken up by the dialog box. 
	//
	OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top); 
	OffsetRect(&rc, -rc.left, -rc.top); 
	OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom); 

	// The new position is the sum of half the remaining 
	// space and the owner's original position.
	SetWindowPos(
			hwndDlg, 
			HWND_TOP, 
			rcOwner.left + (rc.right / 2), 
			rcOwner.top + (rc.bottom / 2), 
			0, 0,          // ignores size arguments 
			SWP_NOSIZE);

	return;
}



void
__cdecl
cdShowMessage(
	HWND	inHwnd,
	LPCTSTR inErrorMsg,
	...)
/*++

Routine Description:
	
	Display the error message

Return:

	none

--*/
{
	if (inErrorMsg)
	{
		TCHAR str[1024];
		va_list arg;
		va_start(arg, inErrorMsg);
		_vstprintf(str, inErrorMsg, arg);
		MessageBox(inHwnd, str, _T("CrashDoctor Message"), MB_OK | MB_ICONINFORMATION);
		va_end(arg);
	}
}



void
__cdecl
cdHandleError(
	HWND	inHwnd,
	DWORD	inErrorCode,
	LPCTSTR	inErrorMsg,
	...)
/*++

Routine Description:
	
	Process CrashDoctor specific or Windows error code and display that error
    to the user.

Return:

	none

--*/
{
	tstring errorMessage;

	if (inErrorMsg)
	{
		TCHAR str[1024];
		va_list arg;
		va_start(arg, inErrorMsg);
		_vstprintf(str, inErrorMsg, arg);
		errorMessage = str;
		va_end(arg);
	}

	if (!errorMessage.empty())
	{
		errorMessage += _T("\n");
	}

	switch(inErrorCode)
	{
		case 0:
		{
			if (!errorMessage.empty())
			{
				cdShowMessage(inHwnd, errorMessage.c_str());
			}

			break;
		}
		case ERR_INVALID_PROCESS_ID:
		{
			cdShowMessage(
                    inHwnd,
                    _T("%sError: Invalid Process ID (PID) specified."),
                    errorMessage.c_str());

			break;
		}
		default:
		{
			LPTSTR lpMsgBuf = NULL;

			if (FormatMessage( 
					FORMAT_MESSAGE_ALLOCATE_BUFFER | 
					FORMAT_MESSAGE_FROM_SYSTEM | 
					FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					inErrorCode,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					(LPTSTR)&lpMsgBuf,
					0,
					NULL))
			{
				cdShowMessage(
                            inHwnd,
                            _T("%s%s"),
                            errorMessage.c_str(),
                            lpMsgBuf);

				LocalFree(lpMsgBuf);
			}
			else
			{
				cdShowMessage(
                        inHwnd,
                        _T("%sUnknown error occured. Error code = %x"),
                        errorMessage.c_str(),
                        inErrorCode);
			}

			break;
		}
	}
}



BOOL
cdObtainSeDebugPrivilege(void)
/*++

Routine Description:
	
	Obtain the debugging privilege for our processes. Without this privilege
	we are not able to debug any services
    
Arguments:
    
    none

Return:

	TRUE/FALSE

--*/
{
	BOOL				fStatus = TRUE;
	HANDLE				hToken;
	TOKEN_PRIVILEGES	tp;
	LUID				luidPrivilege;

	// Make sure we have access to adjust and to get the old token privileges
	if (!OpenProcessToken(
					GetCurrentProcess(),
					TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
					&hToken))
	{
		fStatus = FALSE;
		goto funcEnd;
	}

	// Initialize the privilege adjustment structure
	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luidPrivilege))
	{
		fStatus = FALSE;
		goto funcEnd;
	}

	tp.PrivilegeCount			= 1;
	tp.Privileges[0].Luid		= luidPrivilege;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	fStatus = AdjustTokenPrivileges(
								hToken,
								FALSE,
								&tp,
								0,
								NULL,
								NULL);


funcEnd:

	if (hToken)
	{
		CloseHandle(hToken);
	}

	return fStatus;
}



HRESULT
CreateLink(
	LPCTSTR lpszPathObj,
	LPCTSTR lpszPathLink,
	LPCTSTR lpszDesc)
/*++

Routine Description:
	
	Creates a link for the given path at a given location
    
Arguments:
    
    lpszPathObj - Path of the target for the link
    
    lpszPathLink - Path of the link (.lnk)
    
    lpszDesc - Description for the link

Return:

	HRESULT

--*/
{
	HRESULT hres;
	IShellLink* psl;

	CoInitialize(NULL);

	// Get a pointer to the IShellLink interface. 
	hres = CoCreateInstance(
						CLSID_ShellLink,
						NULL,
						CLSCTX_INPROC_SERVER,
						IID_IShellLink,
						(LPVOID*)&psl);

	if (SUCCEEDED(hres)) 
	{
		IPersistFile* ppf; 

		// Set the path to the shortcut target and add the description. 
		psl->SetPath(lpszPathObj); 
		psl->SetDescription(lpszDesc); 

		// Query IShellLink for the IPersistFile interface for saving the 
		// shortcut in persistent storage. 
		hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf); 

		if (SUCCEEDED(hres)) 
		{
#ifdef _UNICODE
			hres = ppf->Save(lpszPathLink, TRUE); 
#else
			WCHAR wsz[MAX_PATH];

			// Ensure that the string is Unicode. 
			MultiByteToWideChar(CP_ACP, 0, lpszPathLink, -1, wsz, MAX_PATH); 
			
			// To-Do!!! Check return value from MultiByteWideChar to ensure success.

			// Save the link by calling IPersistFile::Save. 
			hres = ppf->Save(wsz, TRUE); 
#endif
			ppf->Release(); 
		} 

		psl->Release(); 
	} 

	CoUninitialize();

	return hres; 
}



#ifndef _UNICODE

/***
*void Parse_Cmdline(cmdstart, argv, lpstr, numargs, numbytes)
*
*Purpose:
*       Parses the command line and sets up the Unicode argv[] array.
*       On entry, cmdstart should point to the command line,
*       argv should point to memory for the argv array, lpstr
*       points to memory to place the text of the arguments.
*       If these are NULL, then no storing (only counting)
*       is done.  On exit, *numargs has the number of
*       arguments (plus one for a final NULL argument),
*       and *numbytes has the number of bytes used in the buffer
*       pointed to by args.
*
*Entry:
*       LPSTR cmdstart - pointer to command line of the form
*           <progname><nul><args><nul>
*       TCHAR **argv - where to build argv array; NULL means don't
*                      build array
*       LPSTR lpstr - where to place argument text; NULL means don't
*                      store text
*
*Exit:
*       no return value
*       INT *numargs - returns number of argv entries created
*       INT *numbytes - number of bytes used in args buffer
*
*Exceptions:
*
*******************************************************************************/

void Parse_CmdlineA (
    LPCSTR cmdstart,
    LPSTR* argv,
    LPSTR  lpstr,
    INT *  numargs,
    INT *  numbytes
    )
{
    LPCSTR p;
    char c;
    INT inquote;                    /* 1 = inside quotes */
    INT copychar;                   /* 1 = copy char to *args */
    WORD numslash;                  /* num of backslashes seen */

    *numbytes = 0;
    *numargs = 1;                   /* the program name at least */

    /* first scan the program name, copy it, and count the bytes */
    p = cmdstart;
    if (argv)
        *argv++ = lpstr;

    /* A quoted program name is handled here. The handling is much
       simpler than for other arguments. Basically, whatever lies
       between the leading double-quote and next one, or a terminal null
       character is simply accepted. Fancier handling is not required
       because the program name must be a legal NTFS/HPFS file name.
       Note that the double-quote characters are not copied, nor do they
       contribute to numbytes. */
    if (*p == TEXT('\"'))
    {
        /* scan from just past the first double-quote through the next
           double-quote, or up to a null, whichever comes first */
        while ((*(++p) != TEXT('\"')) && (*p != TEXT('\0')))
        {
            *numbytes += sizeof(char);
            if (lpstr)
                *lpstr++ = *p;
        }
        /* append the terminating null */
        *numbytes += sizeof(char);
        if (lpstr)
            *lpstr++ = TEXT('\0');

        /* if we stopped on a double-quote (usual case), skip over it */
        if (*p == TEXT('\"'))
            p++;
    }
    else
    {
        /* Not a quoted program name */
        do {
            *numbytes += sizeof(char);
            if (lpstr)
                *lpstr++ = *p;

            c = (char) *p++;

        } while (c > TEXT(' '));

        if (c == TEXT('\0'))
        {
            p--;
        }
        else
        {
            if (lpstr)
                *(lpstr - 1) = TEXT('\0');
        }
    }

    inquote = 0;

    /* loop on each argument */
    for ( ; ; )
    {
        if (*p)
        {
            while (*p == TEXT(' ') || *p == TEXT('\t'))
                ++p;
        }

        if (*p == TEXT('\0'))
            break;                  /* end of args */

        /* scan an argument */
        if (argv)
            *argv++ = lpstr;         /* store ptr to arg */
        ++*numargs;

        /* loop through scanning one argument */
        for ( ; ; )
        {
            copychar = 1;
            /* Rules: 2N backslashes + " ==> N backslashes and begin/end quote
                      2N+1 backslashes + " ==> N backslashes + literal "
                      N backslashes ==> N backslashes */
            numslash = 0;
            while (*p == TEXT('\\'))
            {
                /* count number of backslashes for use below */
                ++p;
                ++numslash;
            }
            if (*p == TEXT('\"'))
            {
                /* if 2N backslashes before, start/end quote, otherwise
                   copy literally */
                if (numslash % 2 == 0)
                {
                    if (inquote)
                        if (p[1] == TEXT('\"'))
                            p++;    /* Double quote inside quoted string */
                        else        /* skip first quote char and copy second */
                            copychar = 0;
                    else
                        copychar = 0;       /* don't copy quote */

                    inquote = !inquote;
                }
                numslash /= 2;          /* divide numslash by two */
            }

            /* copy slashes */
            while (numslash--)
            {
                if (lpstr)
                    *lpstr++ = TEXT('\\');
                *numbytes += sizeof(char);
            }

            /* if at end of arg, break loop */
            if (*p == TEXT('\0') || (!inquote && (*p == TEXT(' ') || *p == TEXT('\t'))))
                break;

            /* copy character into argument */
            if (copychar)
            {
                if (lpstr)
                        *lpstr++ = *p;
                *numbytes += sizeof(char);
            }
            ++p;
        }

        /* null-terminate the argument */

        if (lpstr)
            *lpstr++ = TEXT('\0');         /* terminate string */
        *numbytes += sizeof(char);
    }

}


/***
*CommandLineToArgvW - set up Unicode "argv" for C programs
*
*Purpose:
*       Read the command line and create the argv array for C
*       programs.
*
*Entry:
*       Arguments are retrieved from the program command line
*
*Exit:
*       "argv" points to a null-terminated list of pointers to UNICODE
*       strings, each of which is an argument from the command line.
*       The list of pointers is also located on the heap or stack.
*
*Exceptions:
*       Terminates with out of memory error if no memory to allocate.
*
*******************************************************************************/

LPSTR * CommandLineToArgvA (LPCSTR lpCmdLine, int*pNumArgs)
{
    LPSTR*argv_U;
    LPCSTR cmdstart;                 /* start of command line to parse */
    INT    numbytes;

    if (pNumArgs == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    cmdstart = lpCmdLine;

    /* first find out how much space is needed to store args */
    Parse_CmdlineA(cmdstart, NULL, NULL, pNumArgs, &numbytes);

    /* allocate space for argv[] vector and strings */
    argv_U = (LPSTR*)LocalAlloc(LMEM_ZEROINIT,
                                (*pNumArgs + 1) * sizeof(LPSTR) + numbytes);
    if (!argv_U) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return (NULL);
    }

    /* store args and argv ptrs in just allocated block */
    Parse_CmdlineA(cmdstart,
                   argv_U,
                   (LPSTR)(((LPBYTE)argv_U) + *pNumArgs * sizeof(LPSTR)),
                   pNumArgs,
                   &numbytes);

    return argv_U;
}

#endif
