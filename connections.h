#pragma once

#include <windows.h>
#include <commctrl.h>
#include "thread.h"
#include "socketwrapper.h"
#include "p2p2.h"
#include "link.h"
#include "display.h"
#include "mutex.h"
#include "sendupdate.h"
#include "listlinks.h"

class Connection : public Thread
{
public:
	Connection();
	VOID SetLink(Link* Link);
	Link* GetLink(VOID);
	VOID Connect(PCHAR Host, USHORT Port);
	VOID Disconnect(VOID);
	BOOL Ready(VOID);
	BOOL Active(VOID);
	VOID SendMsg(DWORD MID, PCHAR Message, PCHAR SendTo, UINT TTL, PBYTE Signature, BOOL RunLocally);
	VOID GetInfo(VOID);
	VOID ListLinks(VOID);
	Socket* GetSocketP(VOID);
	VOID WaitForAccess();
	VOID Release();

private:
	VOID ThreadFunc(VOID);

	class ActualConnection : public P2P2::Negotiation
	{
	public:
		ActualConnection() : Negotiation(TYPE_CONTROL){ Active = FALSE; };
		VOID GetInfo(VOID);
		VOID ListLinks(VOID);
		VOID SendMsg(DWORD MID, PCHAR Message, PCHAR SendTo, UINT TTL, PBYTE Signature, BOOL RunLocally);
		Link* Link;
		BOOL Active;
		Mutex Mutex;

	private:
		enum State{CONNECT, GET_INFO} State;
		VOID OnReady(VOID);
		VOID OnFail(UINT ErrorCode);
		VOID ProcessSpecial(WSANETWORKEVENTS NetEvents);
		::Timeout Timeout;
		BOOL ShuttingDown;
	} ActualConnection;
};







/*
#define CON_MSG_CONNECT (WM_APP + 2)
#define CON_MSG_DISCONNECT (WM_APP + 3)
#define CON_MSG_SETLINK (WM_APP + 4)
#define CON_MSG_SENDMESSAGE (WM_APP + 5)

#define CON_STATUS_NULL 0
#define CON_STATUS_CONNECTING 1
#define CON_STATUS_CONNECTED 2
#define CON_STATUS_CONNECTFAILED 3
#define CON_STATUS_DISCONNECTED 4

class Connection
{
public:
	Connection() : ConnectionThread(this) { SetStatus(CON_STATUS_NULL); };
	VOID Connect(LinkEx *Link, PCHAR Host, INT Port);
	VOID Disconnect(VOID);
	DWORD GetStatus(VOID) const { return Status; };
	VOID SendMsg(PCHAR Message, BOOL L, BOOL C, BYTE TTL) { PostThreadMessage(ConnectionThread.GetThreadID(), CON_MSG_SENDMESSAGE, (WPARAM)Message, (LPARAM)((TTL<<2)+(C + (L*2)))); };

private:
	VOID SetStatus(DWORD Status) { Connection::Status = Status; };
	class ConnectionThread : public Thread
	{
		VOID ThreadFunc(VOID);
		VOID SetStatus(DWORD Status) { ((Connection*)ThisP)->SetStatus(Status); };

		Socket Socket;
		ReceiveBuffer RecvBuf;
		PVOID ThisP;
		LinkEx *Link;
	public:
		ConnectionThread(PVOID ThisP) { ConnectionThread::ThisP = ThisP; RecvBuf.Create(1024); StartThread(); };
		~ConnectionThread() { RecvBuf.Cleanup(); };
	};
	ConnectionThread ConnectionThread;
	DWORD Status;
};*/