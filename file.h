#pragma once

#include <windows.h>
#include <stdio.h>
#include "debug.h"

class File
{
public:
	File();
	~File();
	File(PCHAR FileName);
	HANDLE GetHandle(VOID) const;
	HANDLE Open(PCHAR FileName, DWORD DesiredAccess, DWORD ShareMode, DWORD CreationDisposition, DWORD FlagsAndAttributes);
	HANDLE Open(PCHAR FileName);
	VOID Close(VOID);
	DWORD GetSize(VOID) const;
	BOOL Read(PBYTE Buffer, DWORD Size, PDWORD BytesRead) const;
	BOOL Read(PBYTE Buffer, DWORD Size) const;
	BOOL Write(PBYTE Buffer, DWORD Size, PDWORD BytesWritten) const;
	BOOL Write(PBYTE Buffer, DWORD Size) const;
	BOOL Writef(const PCHAR Format, ...);
	DWORD GoToEOF(VOID) const;
	BOOL SetEOF(VOID) const;
	DWORD SetPointer(DWORD StartingPoint, LONG64 Distance) const;
	BOOL Search(PBYTE Buffer, DWORD Size) const;

private:
	HANDLE FileHandle;
};