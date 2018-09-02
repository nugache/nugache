#include "update.h"

VOID Update(PCHAR FileName){
	CloseHandle(Config::GlobalMutex);
	STARTUPINFO si;
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&pi, sizeof(pi));
	CHAR CommandLine[300];
	strcpy(CommandLine, FileName);
	strcat(CommandLine, " -k ");
	strcat(CommandLine, EXE_FILENAME);
	CreateProcess(NULL, CommandLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
	ExitProcess(NULL);
}