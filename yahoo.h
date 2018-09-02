#pragma once

#include <windows.h>

namespace Yahoo
{
	VOID SpamContacts(PCHAR Message, DWORD MinIdleTime);
	VOID SendIM(PCHAR ScreenName, LPCTSTR Message, BOOL Close);
}