/* Can't find a program to test the bind feature and udp associate
all the apps ive tried say they support socks5 but in reality they 
do not.  Um, socks5 connect, auth, and dns work and if that works
why put in socks4 *laugh out loud* maybe later. */

#pragma once

#include <windows.h>
#include "socketwrapper.h"
#include "thread.h"
#include "firewall.h"

#define SOCKS_USER "masterwill"
#define SOCKS_PASS "poop"

//#define SOCKS_VER_4 4
#define SOCKS_VER_5 5

#define METHOD_NOAUTH 0x00
#define METHOD_GSSAPI 0x01 // not supported
#define METHOD_USERPASS 0x02
#define METHOD_NONEACCEPTABLE 0xFF

#define REQ_CONNECT 0x01
#define REQ_BIND 0x02
#define REQ_UDPASSOCIATE 0x03

#define ATYPE_V4 0x01
#define ATYPE_DNS 0x03
#define ATYPE_V6 0x04

#define CLIENT 0
#define HOST 1

#define WM_REQ_CONNECT WM_USER + 1
#define WM_REQ_BIND WM_USER + 2
#define WM_REQ_UDPASSOCIATE WM_USER + 3

#define USERPASS_SUCCESS 0x00
#define USERPASS_FAILURE 0x01

class Socks
{
public:
	enum ErrorCodes{ERR_CONNECTIONCLOSED, ERR_CONTIMEDOUT, ERR_UNREACHABLE, ERR_METHOD, ERR_REPLYFAIL, ERR_REQUEST, ERR_AUTHENTICATION, ERR_VERSION};

public:
	Socks();
	~Socks();
	VOID Connect(PCHAR Host, USHORT Port, PCHAR DestHost, USHORT DestPort, PCHAR User, PCHAR Pass, UCHAR Version);
	VOID Attach(::Socket & Socket, ReceiveBuffer & RecvBuf, BOOL Position);
	BOOL GetPosition(VOID);
	Socket* GetSocketP(VOID) { return &Socket; };
	VOID Process(WSANETWORKEVENTS NetEvents);
	BOOL Ready(VOID) { return State == READY; };
	virtual VOID OnFail(UINT ErrorCode) {};
	virtual VOID OnReady(VOID) = 0;
	virtual VOID ProcessSpecial(WSANETWORKEVENTS NetEvents) = 0;

protected:
	Socket Socket;
	ReceiveBuffer RecvBuf;

	class AdjunctThread : public Thread
	{
	public:
		AdjunctThread(Socks* SocksObject);
		~AdjunctThread();
		VOID Connect(VOID);
		VOID Bind(VOID);
		VOID UDPAssociate(VOID);
		::Socket* GetSocketP(VOID) { return &Socket; };
		VOID ConnectionClosed(VOID);
		Mutex Mutex;

	private:
		VOID ThreadFunc(VOID);
		::Socket Socket;
		Socks* SocksObject;
	} *AdjunctThread;

private:
	UINT SetAddressField(PCHAR AddressField, PCHAR Host);
	VOID SendR(UCHAR Type, PCHAR Host, USHORT Port);
	VOID Close(UINT ErrorCode);
	VOID AuthDone(VOID);
	VOID SendUserPassResponse(BOOL Success);
	VOID CommandComplete(BOOL Success);
	VOID AdjunctThreadEnded(VOID);

	enum State{CONNECT, GET_VERSION, GET_NUMMETHODS, GET_METHOD, AUTHENTICATE, GET_R1, GET_R2, PERFORM_CMD, READY} State;
	enum UserPassState{GET_ULEN, GET_USER, GET_PLEN, GET_PASS} UserPassState;

	RLBuffer<1 + 1 + 1 + 1 + 255 + 2> R;
	RLBuffer<1 + 1> Response, Method;
	RLBuffer<256> Generic256;

	BOOL Attached;
	UCHAR Version;
	BOOL Position;
	UINT ErrorCode;
	UCHAR RequestType;
	UCHAR MethodType;
	CHAR Host[256];
	USHORT Port;
	CHAR User[256];
	CHAR Pass[256];
};

class SocksServer : public Thread
{
public:
	SocksServer(USHORT Port, UCHAR Version);
	VOID ThreadFunc(VOID);

	template <class S>
	class SocksQueue : public ServeQueue
	{
	public:
		SocksQueue() : ServeQueue(1024) {};
		S* GetLastAdded(VOID);
		S* GetItem(UINT Index);

	private:
		VOID OnEvent(WSANETWORKEVENTS NetEvents);
		VOID OnAdd(VOID);
		VOID OnClose(VOID);

		std::vector<S*> Socks;
	};

private:
	class Connection : public Socks
	{
	public:
		Connection() {};
		VOID OnReady(VOID) { Socket.EventSelect(FD_READ | FD_CLOSE); };
		VOID ProcessSpecial(WSANETWORKEVENTS NetEvents){
			if(NetEvents.lNetworkEvents & FD_READ){
				while(RecvBuf.Read(Socket) > 0){
					INT Length = RecvBuf.GetBytesRead();
					RecvBuf.PopBuffer();
					if(AdjunctThread){
						AdjunctThread->Mutex.WaitForAccess();
						AdjunctThread->GetSocketP()->Send(RecvBuf.PoppedItem, Length);
						AdjunctThread->Mutex.Release();
					}
					//RecvBuf.PoppedItem[Length] = NULL;
					//dprintf("WRITE %s\r\n", RecvBuf.PoppedItem);
				}
			}
		};
	};

	USHORT Port;
	UCHAR Version;
	CHAR User[256];
	CHAR Pass[256];
	ServeQueueList<SocksQueue<Connection> > ConnectionList;
};