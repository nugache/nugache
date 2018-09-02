#pragma once

#include <windows.h>
#include "thread.h"
#include "timeout.h"

class IdleTrack : public Thread
{
public:
	IdleTrack();
	ULONGLONG GetIdleTime(VOID);

private:
	VOID ThreadFunc(VOID);

	Timeout Timeout;
};

extern IdleTrack IdleTrack;