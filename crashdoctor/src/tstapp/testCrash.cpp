#include <windows.h>
#include <stdio.h>


int
__stdcall
testCrash1(int a, int b, int c)
{
	printf("New test started\n");

	int xx;
	xx = 0;
	xx++;
	char *p = NULL;
	*p = '5';

	printf("New test done\n");

    return 5;
}

int
__stdcall
testCrash(int a, int b, int c)
{
	printf("Crash test started\n");

	int xx;
	xx = 0;
	xx = testCrash1(4,5,6);

	printf("Crash test done = %x\n", xx);

	char *p = NULL;
	*p = 'A';

    return 10;
}

int 
__cdecl
main()
{
	printf("main application routine entering..\n");


	bool b = false;
	while(b)
	{
		int i = 0;
	}

	int *p = NULL;

    int xx = 0;

	xx = testCrash(5,10,15);

	printf("main application routine exiting..= %x\n", xx);

    try
    {
		ULONG *pp = (ULONG *)(DWORD_PTR)0x8294F000;
		printf("0x%X 0x%X\n", pp[0], p[1]);
    }
    catch(...)
    {
    }

	printf("passed...\n");

    HANDLE hFile = CreateFileW(
                        L"c:\\test.txt",
                        GENERIC_READ | GENERIC_WRITE,
                        0,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (hFile)
    {
        WriteFile(hFile, "Test", 5, NULL, NULL);
        CloseHandle(hFile);
    }

    printf("Unicode CreateFile Done\n");

    hFile = CreateFileA(
                    "c:\\test.txt",
                    GENERIC_READ | GENERIC_WRITE,
                    0,
                    NULL,
                    CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);

    printf("Ansi CreateFile Called\n");

    if (hFile)
    {
        printf("Calling CloseHandle\n");
        CloseHandle(hFile);
        printf("Called CloseHandle\n");
    }

    printf("All CreateFile Done\n");

	fflush(stdin);
	scanf("%d", &xx);
	getchar();

	return 0;
}

