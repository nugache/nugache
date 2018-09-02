#pragma once

#include <windows.h>
#include "socketwrapper.h"
#include "thread.h"
#include "firewall.h"

class TCPTunnel : public Thread
{
public:
	TCPTunnel(USHORT LocalPort, USHORT RemotePort, PCHAR RemoteHost);
	~TCPTunnel();

private:
	class Connection : public Thread
	{
	public:
		Connection(Socket & Socket, TCPTunnel* TCPTunnel);
		~Connection();

	private:
		VOID ThreadFunc(VOID);

		TCPTunnel* TCPTunnel;
		Socket Socket;
	};

	VOID ThreadFunc(VOID);
	Socket ListeningSocket;
	USHORT LocalPort;
	USHORT RemotePort;
	CHAR RemoteHost[256];
};