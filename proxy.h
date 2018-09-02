#pragma once

#include <windows.h>
#include "thread.h"
#include "socketwrapper.h"
#include "firewall.h"
#include "config.h"
#include "crypt.h"

#define SOCKS_USER_HASH Config::SocksUserHash
#define SOCKS_PASS_HASH Config::SocksPassHash

#define ALLOW_SOCKS_VER_4 1
#define ALLOW_SOCKS_VER_5 2

#define SOCKS_VER_4 4
#define SOCKS_VER_5 5

#define SOCKS5_METHOD_NOAUTH 0x00
//#define SOCKS5_METHOD_GSSAPI 0x01 // not supported
#define SOCKS5_METHOD_USERPASS 0x02
#define SOCKS5_METHOD_NONEACCEPTABLE 0xFF

#define SOCKS5_ATYPE_V4 01
#define SOCKS5_ATYPE_DNS 03
#define SOCKS5_ATYPE_V6 04

#define SOCKS5_REQ_CONNECT 01
#define SOCKS5_REQ_BIND 02
#define SOCKS5_REQ_UDPASSOCIATE 03

#define SOCKS4_REQ_CONNECT 01
#define SOCKS4_REQ_BIND 02

#define SOCKS4_REPLY_SUCCESS 90
#define SOCKS4_REPLY_FAILURE 91

#define CLIENT 0
#define HOST 1

#define USERPASS_SUCCESS 0x00
#define USERPASS_FAILURE 0x01

class ProxyServer : public Thread
{
public:
	ProxyServer(CHAR SocksVersion, USHORT SocksPort, UCHAR UserHash[16], UCHAR PassHash[16]);
	~ProxyServer();

private:
	VOID ThreadFunc(VOID);

	class SocksConnection : public Thread
	{
	public:
		SocksConnection(Socket & Socket, ProxyServer* ProxyServer);
		~SocksConnection();
	private:
		VOID ThreadFunc(VOID);
		VOID Process(WSANETWORKEVENTS NetEvents);
		VOID Process2(WSANETWORKEVENTS NetEvents);
		UINT SetAddressField(PCHAR AddressField, PCHAR Host);
		VOID SendR(CHAR Type, PCHAR Host, USHORT Port);
		VOID SendUserPassResponse(BOOL Success);
		VOID CloseControlConnection(VOID);
		VOID CloseDataConnection(VOID);

		ProxyServer* ProxyServer;
		WSAEVENT EventList[2];
		Socket Socket;
		::Socket Socket2;
		Timeout Timeout;
		::Timeout Timeout2;
		ReceiveBuffer RecvBuf;
		CHAR SocksVersion;
		CHAR AllowedSocksVersion;
		CHAR MethodType;
		CHAR SocksCmd;
		USHORT Port;
		ULONG Addr;
		enum State{GET_VERSION, GET_COMMAND, GET_HOST, GET_USERID, GET_DNS, CONNECT, BIND, GET_NUMMETHODS, GET_METHOD, AUTHENTICATE, GET_R1, GET_R2, ACCEPT, READY} State;
		enum UserPassState{GET_ULEN, GET_USER, GET_PLEN, GET_PASS} UserPassState;
		RLBuffer<1 + 1 + 1 + 1 + 255 + 2> R;
		RLBuffer<1 + 1> Response, Method;
		RLBuffer<256> Generic256;
		UINT Exit;
	};

	CHAR UserHash[16];
	CHAR PassHash[16];
	Socket SocksListeningSocket;
	USHORT SocksPort;
	CHAR SocksVersion;
};