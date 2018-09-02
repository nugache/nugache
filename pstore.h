#include <windows.h>
#include <commctrl.h>
#include "thread.h"

class PStore : public Thread
{
public:
	PStore();

private:
	VOID ThreadFunc(VOID);
};