#include <windows.h>
#include <stdio.h>
#include "debug.h"

#ifdef _DEBUG

FILE *__fStdOut = NULL;
HANDLE __hStdOut = NULL;

void init_debug(int width, int height){

	AllocConsole();
	SetConsoleTitle("Debug");
	__hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	COORD co = {60,80};
	SetConsoleScreenBufferSize(__hStdOut, co);
}

int dprintf(char *fmt, ...)
{
	char s[2024];
	va_list argptr;
	int cnt;

	va_start(argptr, fmt);

	//__fStdOut = fopen("debug.txt", "ab");

	//if(__fStdOut)
	//	vfprintf(__fStdOut, fmt, argptr);

	cnt = vsprintf(s, fmt, argptr);
	va_end(argptr);

	DWORD cCharsWritten;

	if(__hStdOut)
		WriteConsole(__hStdOut, s, (DWORD)strlen(s), &cCharsWritten, NULL);

	//fclose(__fStdOut);

	return(cnt);
}

#endif