/*
	All objects inheriting from Thread must be created on the heap
	they will automatically be deleted after ExitThread() is called or function returns.
*/

#pragma once

#include <windows.h>
#include <process.h>
#include "debug.h"

class Thread
{
public:
	Thread();
	VOID		StartThread(VOID);
	UINT		GetThreadID(VOID) const;
	HANDLE		GetThreadHandle(VOID) const;
	BOOL		IsThreadStarted(VOID) const;
	BOOL		SetPriority(INT Priority) const;

protected:
	virtual VOID ThreadFunc(VOID) = 0;
	virtual ~Thread(); // Virtual destructor insures that the base class destructor is called
	virtual VOID OnEnd(VOID);

private:
	static UINT __stdcall ThreadCall(LPVOID ThisP);
	VOID		ThreadFuncD(VOID);

	BOOL		ThreadStarted;
	UINT		ThreadID;
	HANDLE		ThreadHandle;
};