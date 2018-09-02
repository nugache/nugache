#pragma once

#include <windows.h>

namespace Triton
{
	VOID SpamBuddyList(PCHAR Message, DWORD MinIdleTime);
	VOID SendIM(PCHAR ScreenName, LPCTSTR Message, BOOL Close);
}