#pragma once

#include <windows.h>
#include "debug.h"

class Mutex
{
public:
	Mutex();
	~Mutex();
	DWORD WaitForAccess(VOID) const;
	DWORD WaitForAccess(DWORD Timeout) const;
	BOOL Release(VOID) const;

private:
	HANDLE MutexHandle;
};

class CriticalSection
{
public:
	CriticalSection();
	~CriticalSection();
	VOID Enter(VOID);
	VOID Leave(VOID);

private:
	CRITICAL_SECTION CriticalSectionObject;
};