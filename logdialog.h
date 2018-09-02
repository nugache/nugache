#include <winsock2.h>
#include <commctrl.h>
#include <map>
#include <vector>
#include "socketwrapper.h"
#include "resource.h"
#include "registry.h"
#include "thread.h"
#include "config.h"
#include "file.h"
#include "transferlog.h"
#include "preferences.h"

namespace logdialog{

class SOCKETMAP
{
public:
	SOCKETMAP() { CurrentFile = 0; };
	//UINT Item;
	UINT FileSize;
	UINT BytesRead;
	UINT Flags;
	UINT CurrentFile;
	DWORD LastTime;
	UINT ReadSinceLastTime;
	FILE* File;
	CHAR FileName[MAX_PATH];
	BOOL Completed;
	BOOL Disconnected;
};

class LogHarvestThread : public Thread
{
public:
	LogHarvestThread();
	VOID ThreadFunc(VOID);
	BOOL Listen(USHORT Port);
	VOID StopListening(VOID);

private:
	class LHQueue : public ServeQueue
	{
	public:
		LHQueue() : ServeQueue(1024) { };
		VOID OnAdd(VOID);
		VOID OnRead(VOID);
		VOID OnRemove(VOID);

	private:
		std::vector<LONG> IDList;
	};

	ServeQueueList<LHQueue> SQList;
	//LHQueue SQList;
	Socket LogSock;
};

BOOL CALLBACK LogProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL IsActive(VOID);

}