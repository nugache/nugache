#include <winsock2.h>
#include "tcptunnel.h"

TCPTunnel::TCPTunnel(USHORT LocalPort, USHORT RemotePort, PCHAR RemoteHost){
	TCPTunnel::LocalPort = LocalPort;
	TCPTunnel::RemotePort = RemotePort;
	strncpy(TCPTunnel::RemoteHost, RemoteHost, sizeof(TCPTunnel::RemoteHost));
	ListeningSocket.Create(SOCK_STREAM);
	ListeningSocket.CreateEvent();
	StartThread();
}

TCPTunnel::~TCPTunnel(){
	ListeningSocket.Disconnect(CLOSE_BOTH_HDL);
}

VOID TCPTunnel::ThreadFunc(VOID){
	FireWall FireWall;
	FireWall.OpenPort(LocalPort, NET_FW_IP_PROTOCOL_TCP, L"null");
	ListeningSocket.EventSelect(FD_ACCEPT);
	if(ListeningSocket.Bind(LocalPort) != SOCKET_ERROR){
		if(ListeningSocket.Listen(SOMAXCONN) == SOCKET_ERROR)
			return;
	}else
		return;
	WSAEVENT EventList[1];
	EventList[0] = ListeningSocket.GetEventHandle();
	WSANETWORKEVENTS NetEvents;
	INT SignalledEvent;
	while(1){
		if((SignalledEvent = WSAWaitForMultipleEvents(1, EventList, FALSE, INFINITE, FALSE)) != WSA_WAIT_TIMEOUT){
			SignalledEvent -= WSA_WAIT_EVENT_0;
			if(SignalledEvent == 0){
				ListeningSocket.EnumEvents(&NetEvents);
				if(NetEvents.lNetworkEvents & FD_ACCEPT){
					Socket NewSocket;
					ListeningSocket.Accept(NewSocket);
					NewSocket.EventSelect(FD_READ | FD_CLOSE);
					new Connection(NewSocket, this);
				}
			}
		}
	}
}

TCPTunnel::Connection::Connection(::Socket & Socket, ::TCPTunnel* TCPTunnel){
	Connection::TCPTunnel = TCPTunnel;
	Connection::Socket = Socket;
	StartThread();
}

TCPTunnel::Connection::~Connection(){
	Socket.Disconnect(CLOSE_BOTH_HDL);
}

VOID TCPTunnel::Connection::ThreadFunc(VOID){
	::Socket RemoteSocket;
	RemoteSocket.Create(SOCK_STREAM);
	if(RemoteSocket.Connect(TCPTunnel->RemoteHost, TCPTunnel->RemotePort) != SOCKET_ERROR){
		RemoteSocket.EventSelect(FD_READ | FD_CLOSE);
		WSAEVENT EventList[2];
		EventList[0] = Socket.GetEventHandle();
		EventList[1] = RemoteSocket.GetEventHandle();
		WSANETWORKEVENTS NetEvents;
		INT SignalledEvent;
		INT Disconnected = 0;
		while(1){
			if((SignalledEvent = WSAWaitForMultipleEvents(2, EventList, FALSE, INFINITE, FALSE)) != WSA_WAIT_TIMEOUT){
				SignalledEvent -= WSA_WAIT_EVENT_0;
				if(SignalledEvent == 0){
					Socket.EnumEvents(&NetEvents);
					if(NetEvents.lNetworkEvents & FD_READ){
						CHAR Buffer[1024];
						INT Read = Socket.Recv(Buffer, sizeof(Buffer));
						if(Read > 0)
							RemoteSocket.Send(Buffer, Read);
					}else
					if(NetEvents.lNetworkEvents & FD_CLOSE){
						Socket.Disconnect(CLOSE_SOCKET_HDL);
						RemoteSocket.Shutdown();
						Disconnected++;
					}
				}else
				if(SignalledEvent == 1){
					RemoteSocket.EnumEvents(&NetEvents);
					if(NetEvents.lNetworkEvents & FD_READ){
						CHAR Buffer[1024];
						INT Read = RemoteSocket.Recv(Buffer, sizeof(Buffer));
						if(Read > 0)
							Socket.Send(Buffer, Read);
					}else
					if(NetEvents.lNetworkEvents & FD_CLOSE){
						RemoteSocket.Disconnect(CLOSE_SOCKET_HDL);
						Socket.Shutdown();
						Disconnected++;
					}
				}
				if(Disconnected == 2){
					RemoteSocket.Disconnect(CLOSE_BOTH_HDL);
					return;
				}
			}
		}
	}else{
		RemoteSocket.Disconnect(CLOSE_BOTH_HDL);
		return;
	}
}