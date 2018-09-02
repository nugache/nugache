#pragma once

#include <windows.h>

class Timeout
{
public:
	Timeout();
	VOID SetTimeout(UINT Timeout);
	VOID Reset(VOID);
	BOOL TimedOut(VOID);
	ULONGLONG GetElapsedTime(VOID);

private:
	VOID CheckOverlap(VOID);

	ULONGLONG OldTime;
	UINT TimeoutTime;
};