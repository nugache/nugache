#pragma once

#include <windows.h>
#include "debug.h"

class Clipboard
{
public:
	Clipboard();
	~Clipboard();
	BOOL Open(HWND NewOwner);
	BOOL Close(VOID);
	BOOL GetData(UINT Format, PBYTE Data, UINT Size, BOOL String);
	BOOL SetData(UINT Format, PBYTE Data, UINT Size);
	UINT RegisterFormat(PCHAR Name);
	BOOL Empty(VOID);
};