#pragma once

#include <windows.h>
#include <commctrl.h>
#include <map>
#include "thread.h"
#include "resource.h"
#include "debug.h"
#include "connections.h"
#include "p2p2.h"
#include "messagedialogs.h"

namespace messagequeue {

extern PCHAR Params;
extern PCHAR Comments;
extern PCHAR Script;

class MessageSignThread;

class MESSAGEMAP
{
public:
	MESSAGEMAP() { SignThread = NULL; TTL = 0; };
	DWORD MID;
	CHAR SendTo[(UUID_LEN * 2) + 1];
	BOOL RunLocally;
	BOOL Broadcast;
	UINT TTL;
	//CHAR Params[1024];
	//CHAR Comments[256];
	CHAR Name[64];
	CHAR Script[2048];
	MessageSignThread* SignThread;
	BYTE Signature[RSAPublicKeyMasterLen];
	enum State{WAITING, SIGNING, READY, SENT} State;
};

class MessageSignThread : public Thread
{
public:
	MessageSignThread(LONG ID);
	VOID ThreadFunc(VOID);

private:
	LONG ID;
};

BOOL CALLBACK MessageQueueProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL IsActive(VOID);

}