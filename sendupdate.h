#include <windows.h>
#include "file.h"
#include "thread.h"
#include "crypt.h"
#include "connections.h"
#include "resource.h"

BOOL CALLBACK SendUpdateProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);