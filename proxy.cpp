#include <winsock2.h>
#include "proxy.h"

ProxyServer::ProxyServer(CHAR SocksVersion, USHORT SocksPort, UCHAR UserHash[16], UCHAR PassHash[16]){
	ProxyServer::SocksPort = SocksPort;
	ProxyServer::SocksVersion = SocksVersion;
	memcpy(ProxyServer::UserHash, UserHash, sizeof(ProxyServer::UserHash));
	memcpy(ProxyServer::PassHash, PassHash, sizeof(ProxyServer::PassHash));
	SocksListeningSocket.Create(SOCK_STREAM);
	SocksListeningSocket.CreateEvent();
	StartThread();
}

ProxyServer::~ProxyServer(){
	SocksListeningSocket.Disconnect(CLOSE_BOTH_HDL);
}

VOID ProxyServer::ThreadFunc(VOID){
	FireWall FireWall;
	FireWall.OpenPort(SocksPort, NET_FW_IP_PROTOCOL_TCP, L"null");
	SocksListeningSocket.EventSelect(FD_ACCEPT);
	if(SocksListeningSocket.Bind(SocksPort) != SOCKET_ERROR){
		if(SocksListeningSocket.Listen(4) == SOCKET_ERROR)
			return;
	}else
		return;
	WSAEVENT EventList[1];
	EventList[0] = SocksListeningSocket.GetEventHandle();
	WSANETWORKEVENTS NetEvents;
	INT SignalledEvent;
	while(1){
		if((SignalledEvent = WSAWaitForMultipleEvents(1, EventList, FALSE, INFINITE, FALSE)) != WSA_WAIT_TIMEOUT){
			SignalledEvent -= WSA_WAIT_EVENT_0;
			if(SignalledEvent == 0){
				SocksListeningSocket.EnumEvents(&NetEvents);
				if(NetEvents.lNetworkEvents & FD_ACCEPT){
					Socket NewSocket;
					SocksListeningSocket.Accept(NewSocket);
					NewSocket.EventSelect(FD_READ | FD_CLOSE);
					new SocksConnection(NewSocket, this);
				}
			}
		}
	}
}

ProxyServer::SocksConnection::SocksConnection(::Socket & Socket, ::ProxyServer* ProxyServer){
	SocksConnection::Socket = Socket;
	SocksConnection::ProxyServer = ProxyServer;
	SocksConnection::Port = ProxyServer->SocksPort;
	AllowedSocksVersion = ProxyServer->SocksVersion;
	State = GET_VERSION;
	RecvBuf.Create(1024);
	RecvBuf.StartRL(1); // First byte tells version
	Socket2.Create(SOCK_STREAM);
	Socket2.CreateEvent();
	Exit = 0;
	StartThread();
}

ProxyServer::SocksConnection::~SocksConnection(){
	RecvBuf.Cleanup();
	Socket.Disconnect(CLOSE_BOTH_HDL);
	Socket2.Disconnect(CLOSE_BOTH_HDL);
}

VOID ProxyServer::SocksConnection::ThreadFunc(VOID){
	//dprintf("new SOCKS connection from %s\r\n", inet_ntoa(SocketFunction::Stoin(Socket.GetAddr())));
	EventList[0] = Socket.GetEventHandle();
	EventList[1] = Socket2.GetEventHandle();
	WSANETWORKEVENTS NetEvents;
	INT SignalledEvent;
	while(1){
		if((SignalledEvent = WSAWaitForMultipleEvents(2, EventList, FALSE, 1000, FALSE)) != WSA_WAIT_TIMEOUT){
			SignalledEvent -= WSA_WAIT_EVENT_0;
			if(SignalledEvent == 0){
				Socket.EnumEvents(&NetEvents);
				Process(NetEvents);
			}else
			if(SignalledEvent == 1){
				Socket2.EnumEvents(&NetEvents);
				Process2(NetEvents);
			}
			if(Exit == 2){
				return;
			}
		}else{
			SetEvent(Socket.GetEventHandle());
		}
	}
}

VOID ProxyServer::SocksConnection::Process(WSANETWORKEVENTS NetEvents){
	ready:
	if(State == READY){
		//dprintf("READY\r\n");
		if(NetEvents.lNetworkEvents & FD_READ){
			//dprintf("READ\r\n");
			CHAR Buffer[1024];
			INT Len;
			while((Len = Socket.Recv(Buffer, sizeof(Buffer))) > 0)
				Socket2.Send(Buffer, Len);
		}
		if(NetEvents.lNetworkEvents & FD_CLOSE || Timeout.TimedOut()){
			CloseDataConnection();
			Exit++;
		}
	}else{
		if(NetEvents.lNetworkEvents & FD_READ){
			while(RecvBuf.Read(Socket) > 0){
				if(State == GET_VERSION){
					if(RecvBuf.PopRLBuffer()){
						SocksVersion = RecvBuf.PoppedItem[0];
						//dprintf("SOCKS VERSION = %d\r\n", SocksVersion);
						if(SocksVersion != SOCKS_VER_4 && SocksVersion != SOCKS_VER_5 ||
						(SocksVersion == SOCKS_VER_4 && !(AllowedSocksVersion & ALLOW_SOCKS_VER_4)) ||
						(SocksVersion == SOCKS_VER_5 && !(AllowedSocksVersion & ALLOW_SOCKS_VER_5))){
							CloseControlConnection();
						}else{
							if(SocksVersion == SOCKS_VER_4){
								State = GET_COMMAND;
								RecvBuf.StartRL(1);
								goto get_command;
							}else
							if(SocksVersion == SOCKS_VER_5){
								State = GET_NUMMETHODS;
								RecvBuf.StartRL(1);
								goto get_nummethods;
							}
						}
					}
				}else get_command:
				if(State == GET_COMMAND){
					if(RecvBuf.PopRLBuffer()){
						SocksCmd = RecvBuf.PoppedItem[0];
						Generic256.BytesRead = 0;
						State = GET_HOST;
						RecvBuf.StartRL(6);
						goto get_host;
					}
				}else get_host:
				if(State == GET_HOST){
					BOOL Done = RecvBuf.PopRLBuffer();
					Generic256.Read(RecvBuf.PoppedItem, RecvBuf.GetRLBufferSize());
					if(Done){
						//dprintf("%.2X %.2X %.2X %.2X %.2X %.2X\r\n", Generic256.Data[0], Generic256.Data[1], Generic256.Data[2], Generic256.Data[3], Generic256.Data[4], Generic256.Data[5]);
						memcpy(&Port, &Generic256.Data[0], sizeof(Port));
						memcpy(&Addr, &Generic256.Data[2], sizeof(Addr));
						Port = htons(Port);
						State = GET_USERID;
						goto get_userid;
					}
				}else get_userid:
				if(State == GET_USERID){
					if(RecvBuf.PopItem("\0", 1)){
						if((Addr << 8) == 0 && Addr != 0){ // 4a DNS
							//dprintf("GET_DNS\r\n");
							State = GET_DNS;
							Generic256.BytesRead = 0;
							goto get_dns;
						}else{
							if(SocksCmd == SOCKS4_REQ_CONNECT){
								State = CONNECT;
								goto connect;
							}else
							if(SocksCmd == SOCKS4_REQ_BIND){
								State = BIND;
								goto bind;
							}else{
								CloseControlConnection();
								return;
							}
						}
					}
				}else get_dns:
				if(State == GET_DNS){
					if(RecvBuf.PopItem("\0", 1)){
						//dprintf("SOCKS GET_DNS %s\r\n", RecvBuf.PoppedItem);
						if((Addr = SocketFunction::GetAddr(RecvBuf.PoppedItem)) == SOCKET_ERROR){
							CloseControlConnection();
						}else{
							if(SocksCmd == SOCKS4_REQ_CONNECT){
								State = CONNECT;
								goto connect;
							}else
							if(SocksCmd == SOCKS4_REQ_BIND){
								State = BIND;
								goto bind;
							}else{
								CloseControlConnection();
								return;
							}
						}
					}
				}else connect:
				if(State == CONNECT){
					//dprintf("SOCKS CONNECT %s\r\n", inet_ntoa(SocketFunction::Stoin(Addr)));
					CHAR Buffer[8];
					memset(Buffer, NULL, sizeof(Buffer));
					Buffer[0] = 0;
					if(Socket2.Connect(inet_ntoa(SocketFunction::Stoin(Addr)), Port) == SOCKET_ERROR){
						Buffer[1] = SOCKS4_REPLY_FAILURE;
						Socket.Send(Buffer, sizeof(Buffer));
						CloseControlConnection();
					}else{
						//dprintf("SUCCESS\r\n");
						Buffer[1] = SOCKS4_REPLY_SUCCESS;
						Socket.Send(Buffer, sizeof(Buffer));
						Socket2.EventSelect(FD_READ | FD_CLOSE);
						State = READY;
						goto ready;
					}
				}else bind:
				if(State == BIND){
					//dprintf("SOCKS BIND\r\n");
					Socket2.Bind(0);
					FireWall FireWall;
					FireWall.OpenPort(Socket2.GetPort(), NET_FW_IP_PROTOCOL_TCP, L"null");
					CHAR Buffer[8];
					Buffer[0] = 0;
					if(Socket2.Listen(1) == SOCKET_ERROR){
						Buffer[1] = SOCKS4_REPLY_FAILURE;
						Socket.Send(Buffer, sizeof(Buffer));
						CloseControlConnection();
					}else{
						Socket2.EventSelect(FD_ACCEPT);
						Buffer[1] = SOCKS4_REPLY_SUCCESS;
						USHORT HSPort = ntohs(Socket2.GetPort());
						memcpy(&Buffer[2], &HSPort, sizeof(HSPort));
						memset(&Buffer[4], NULL, sizeof(ULONG));
						Socket.Send(Buffer, sizeof(Buffer));
						State = ACCEPT;
					}
				}else get_nummethods:
				if(State == GET_NUMMETHODS){
					if(RecvBuf.PopRLBuffer()){
						Generic256.BytesRead = 0;
						State = GET_METHOD;
						RecvBuf.StartRL(RecvBuf.PoppedItem[0]);
						goto get_method;
					}
				}else get_method:
				if(State == GET_METHOD){
					BOOL Done = RecvBuf.PopRLBuffer();
					Generic256.Read(RecvBuf.PoppedItem, RecvBuf.GetRLBufferSize());
					//dprintf("%d methods read\r\n", Generic256.BytesRead);
					if(Done){
						MethodType = SOCKS5_METHOD_NONEACCEPTABLE;
						for(UINT i = 0; i < Generic256.BytesRead; i++){
							if(Generic256.Data[i] == SOCKS5_METHOD_USERPASS){
								MethodType = SOCKS5_METHOD_USERPASS;
								break;
							}
						}
						if(MethodType == SOCKS5_METHOD_USERPASS){
							//dprintf("USERPASS method\r\n");
							State = AUTHENTICATE;
							UserPassState = GET_ULEN;
							CHAR Reply[2];
							Reply[0] = SOCKS_VER_5;
							Reply[1] = SOCKS5_METHOD_USERPASS;
							Socket.Send(Reply, 2);
							Method.BytesRead = 0;
							RecvBuf.StartRL(1 + 1);
							goto authenticate;
						}else
						if(MethodType == SOCKS5_METHOD_NONEACCEPTABLE){
							CloseControlConnection();
						}
					}
				}else authenticate:
				if(State == AUTHENTICATE){
					if(MethodType == SOCKS5_METHOD_USERPASS){
						if(UserPassState == GET_ULEN){
							BOOL Done = RecvBuf.PopRLBuffer();
							Method.Read(RecvBuf.PoppedItem, RecvBuf.GetRLBufferSize());
							if(Done){
								UserPassState = GET_USER;
								RecvBuf.StartRL(Method.Data[1]);
								Generic256.BytesRead = 0;
								goto get_user;
							}
						}else get_user:
						if(UserPassState == GET_USER){
							BOOL Done = RecvBuf.PopRLBuffer();
							Generic256.Read(RecvBuf.PoppedItem, RecvBuf.GetRLBufferSize());
							//dprintf("user = %s %d\r\n", RecvBuf.PoppedItem, RecvBuf.GetRLBufferSize());
							if(Done){
								UCHAR Hash[16];
								MD5 MD5;
								MD5.Update((PUCHAR)Generic256.Data, Generic256.BytesRead);
								MD5.Finalize(Hash);
								if(memcmp(Hash, ProxyServer->UserHash, sizeof(Hash)) == 0){
									UserPassState = GET_PLEN;
									RecvBuf.StartRL(1);
									goto get_plen;
								}else{
									SendUserPassResponse(USERPASS_FAILURE);
									CloseControlConnection();
									//dprintf("USERPASS failure, closing\r\n");
								}
							}
						}else get_plen:
						if(UserPassState == GET_PLEN){
							if(RecvBuf.PopRLBuffer()){
								Generic256.BytesRead = 0;
								UserPassState = GET_PASS;
								RecvBuf.StartRL(RecvBuf.PoppedItem[0]);
								goto get_pass;
							}
						}else get_pass:
						if(UserPassState == GET_PASS){
							BOOL Done = RecvBuf.PopRLBuffer();
							Generic256.Read(RecvBuf.PoppedItem, RecvBuf.GetRLBufferSize());
							//dprintf("pass = %s %d\r\n", RecvBuf.PoppedItem, RecvBuf.GetRLBufferSize());
							if(Done){
								UCHAR Hash[16];
								MD5 MD5;
								MD5.Update((PUCHAR)Generic256.Data, Generic256.BytesRead);
								MD5.Finalize(Hash);
								if(memcmp(Hash, ProxyServer->PassHash, sizeof(Hash)) == 0){
									SendUserPassResponse(USERPASS_SUCCESS);
									State = GET_R1;
									RecvBuf.StartRL(1 + 1 + 1 + 1 + /* 1st byte of addr */ 1);
									R.BytesRead = 0;
									goto get_request1;
								}else{
									SendUserPassResponse(USERPASS_FAILURE);
									CloseControlConnection();
								}
							}
						}
					}
				}else get_request1:
				if(State == GET_R1){
					BOOL Done = RecvBuf.PopRLBuffer();
					R.Read(RecvBuf.PoppedItem, RecvBuf.GetRLBufferSize());
					if(Done){
						UINT ReadLength = 0;
						switch(R.Data[3]){
							case SOCKS5_ATYPE_V4: ReadLength = (4 - 1) + 2; break;
							case SOCKS5_ATYPE_V6: ReadLength = (16 - 1) + 2; break;
							case SOCKS5_ATYPE_DNS: ReadLength = R.Data[4] + 2; break;
						};
						if(!ReadLength){
							CloseControlConnection();
						}else{
							RecvBuf.StartRL(ReadLength);
							State = GET_R2;
							goto get_request2;
						}
					}
				}else get_request2:
				if(State == GET_R2){
					BOOL Done = RecvBuf.PopRLBuffer();
					R.Read(RecvBuf.PoppedItem, RecvBuf.GetRLBufferSize());
					if(Done){
						CHAR Host[256];
						if(R.Data[3] == SOCKS5_ATYPE_V4){
							ULONG Addr;
							memcpy(&Addr, &R.Data[4], sizeof(Addr));
							memcpy(&Port, &R.Data[4 + 4], sizeof(Port));
							strncpy(Host, inet_ntoa(SocketFunction::Stoin(Addr)), sizeof(Host));
						}else
						if(R.Data[3] == SOCKS5_ATYPE_V6){
							//todo
						}else
						if(R.Data[3] == SOCKS5_ATYPE_DNS){
							strncpy(Host, (PCHAR)&R.Data[5], R.Data[4]);
							Host[R.Data[4]] = NULL;
							memcpy(&Port, &R.Data[1 + 1 + 1 + 1 + 1 + R.Data[4]], sizeof(Port));
						}
						Port = htons(Port);
						if(R.Data[1] == SOCKS5_REQ_CONNECT){
							SocksCmd = SOCKS5_REQ_CONNECT;
							if(Socket2.Connect(Host, Port) == SOCKET_ERROR){
								SendR(0x01, "0.0.0.0", 0);
								CloseControlConnection();
							}else{
								SendR(0x00, inet_ntoa(SocketFunction::Stoin(Socket.GetAddr())), Socket.GetPort());
								Socket2.EventSelect(FD_READ | FD_CLOSE);
								State = READY;
								goto ready;
							}
						}else
						if(R.Data[1] == SOCKS5_REQ_BIND){
							SocksCmd = SOCKS5_REQ_BIND;
							Socket2.Bind(0);
							FireWall FireWall;
							FireWall.OpenPort(Socket2.GetPort(), NET_FW_IP_PROTOCOL_TCP, L"null");
							if(Socket2.Listen(1) == SOCKET_ERROR){
								SendR(0x01, "0.0.0.0", 0);
								CloseControlConnection();
							}else{
								Socket2.EventSelect(FD_ACCEPT);
								SendR(0x00, "0.0.0.0", Socket2.GetPort());
								State = ACCEPT;
							}
						}else
						if(R.Data[1] == SOCKS5_REQ_UDPASSOCIATE){
							SocksCmd = SOCKS5_REQ_UDPASSOCIATE;
						}

					}
				}
			}
		}
		if(NetEvents.lNetworkEvents & FD_CLOSE || Timeout.TimedOut()){
			Exit = 2;
		}
	}
}

VOID ProxyServer::SocksConnection::Process2(WSANETWORKEVENTS NetEvents){
	if(NetEvents.lNetworkEvents & FD_ACCEPT){
		::Socket NewSocket;
		Socket2.Accept(NewSocket);
		Socket2.Disconnect(CLOSE_BOTH_HDL);
		Socket2 = NewSocket;
		if(SocksVersion == SOCKS_VER_5){
			SendR(0x00, inet_ntoa(SocketFunction::Stoin(Socket2.GetPeerAddr())), Socket2.GetPeerPort());
		}else
		if(SocksVersion == SOCKS_VER_4){
			CHAR Buffer[8];
			Buffer[0] = 0;
			Buffer[1] = SOCKS4_REPLY_SUCCESS;
			Socket.Send(Buffer, sizeof(Buffer));
		}
		Socket.EventSelect(FD_READ | FD_CLOSE);
		EventList[0] = Socket.GetEventHandle();
		State = READY;
	}
	if(NetEvents.lNetworkEvents & FD_READ){

		CHAR Buffer[1024];
		INT Len;
		while((Len = Socket2.Recv(Buffer, sizeof(Buffer))) > 0)
			Socket.Send(Buffer, Len);
	}
	if(NetEvents.lNetworkEvents & FD_CLOSE || Timeout2.TimedOut()){
		CloseControlConnection();
		Exit++;
	}
}

UINT ProxyServer::SocksConnection::SetAddressField(PCHAR AddressField, PCHAR Host){
	UINT Size;
	CHAR AddressType;
	if(inet_addr(Host) != INADDR_NONE){
		AddressType = SOCKS5_ATYPE_V4;
	}else
	if(strstr(Host, ":")){
		AddressType = SOCKS5_ATYPE_V6;
	}else
		AddressType = SOCKS5_ATYPE_DNS;

	AddressField[0] = AddressType;
	if(AddressType == SOCKS5_ATYPE_DNS){
		AddressField[1] = strlen(Host);
		strncpy(&AddressField[2], Host, 255);
		Size = 1 + strlen(Host);
	}else
	if(AddressType == SOCKS5_ATYPE_V4){
		ULONG Addr = inet_addr(Host);
		memcpy(&AddressField[1], &Addr, sizeof(Addr));
		Size = 4;
	}else
	if(AddressType == SOCKS5_ATYPE_V6){
		//todo
		Size = 16;
	}

	return Size;
}

VOID ProxyServer::SocksConnection::SendR(CHAR Type, PCHAR Host, USHORT Port){
	CHAR Request[1 + 1 + 1 + 1 + 255 + 2];
	Request[0] = SocksVersion;
	Request[1] = Type;
	Request[2] = 0x00;
	UINT Size = SetAddressField(&Request[3], Host);
	USHORT NSPort = htons(Port);
	memcpy((PCHAR)&Request[4] + Size, &NSPort, sizeof(USHORT));
	Socket.Send((PCHAR)&Request, 1 + 1 + 1 + 1 + Size + sizeof(USHORT));
}

VOID ProxyServer::SocksConnection::SendUserPassResponse(BOOL Success){
	CHAR Response[2];
	Response[0] = 0x01;
	Response[1] = Success;
	Socket.Send(Response, 2);
}

VOID ProxyServer::SocksConnection::CloseControlConnection(VOID){
	Socket.Shutdown();
	Timeout.SetTimeout(SHUTDOWN_TIMEOUT);
	Timeout.Reset();
}

VOID ProxyServer::SocksConnection::CloseDataConnection(VOID){
	Socket2.Shutdown();
	Timeout2.SetTimeout(SHUTDOWN_TIMEOUT);
	Timeout2.Reset();
}