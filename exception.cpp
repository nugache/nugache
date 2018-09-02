#include <winsock2.h>
#include "exception.h"

LONG WINAPI UnhandledExceptionFilterFunction(_EXCEPTION_POINTERS* ExceptionInfo){
	CHAR Text[100];
	sprintf(Text, "%X %X %X", ExceptionInfo->ExceptionRecord->ExceptionCode, ExceptionInfo->ExceptionRecord->ExceptionAddress, ExceptionInfo->ContextRecord->Eip);
	IRCList.QuitAll(Text);
	CloseHandle(Config::GlobalMutex);
	CHAR CommandLine[300];
	strcpy(CommandLine, Config::GetExecuteFilename());
	STARTUPINFO si;
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&pi, sizeof(pi));
	CreateProcess(NULL, CommandLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
	ExitProcess(NULL);
	return EXCEPTION_EXECUTE_HANDLER;
}