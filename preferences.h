#pragma once

#include <windows.h>
#include <stdio.h>
#include <commctrl.h>
#include "resource.h"
#include "config.h"
#include "thread.h"
#include "debug.h"
#include "file.h"
#include "display.h"
#include "crypt.h"
#include "registry.h"
#include "p2p2.h"

class GenerateKeyThread : public Thread
{
public:
	GenerateKeyThread() { StartThread(); };

private:
	VOID ThreadFunc(VOID);
};

BOOL CALLBACK PreferencesProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);