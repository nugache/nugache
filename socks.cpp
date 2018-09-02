#include <winsock2.h>
#include "socks.h"

Socks::Socks(){
	RecvBuf.Create(1024);
	Position = CLIENT;
	ErrorCode = ERR_CONNECTIONCLOSED;
	AdjunctThread = NULL;
	Attached = FALSE;
}

Socks::~Socks(){
	if(!Attached){
		RecvBuf.Cleanup();
		Socket.Disconnect(CLOSE_BOTH_HDL);
	}
}

VOID Socks::Connect(PCHAR Host, USHORT Port, PCHAR DestHost, USHORT DestPort, PCHAR User, PCHAR Pass, UCHAR Version){
	if(/*Version != SOCKS_VER_4 && */Version != SOCKS_VER_5)
		return;
	Socks::Version = Version;
	State = CONNECT;
	strncpy(Socks::Host, DestHost, sizeof(Socks::Host));
	Socks::Port = DestPort;
	strncpy(Socks::User, User, sizeof(Socks::User));
	strncpy(Socks::Pass, Pass, sizeof(Socks::Pass));
	RequestType = REQ_CONNECT;
	Socket.Disconnect(CLOSE_SOCKET_HDL);
	Socket.Create(SOCK_STREAM);
	Socket.EventSelect(FD_CONNECT);
	Socket.Connect(Host, Port);
}

VOID Socks::Attach(::Socket & Socket, ReceiveBuffer & RecvBuf, BOOL Position){
	Socks::Position = Position;
	Socks::Socket.Disconnect(CLOSE_BOTH_HDL);
	Socks::Socket = Socket;
	Socks::RecvBuf.Cleanup();
	Socks::RecvBuf = RecvBuf;
	if(Position == HOST){
		State = GET_VERSION;
		Socks::Socket.EventSelect(FD_READ | FD_CLOSE);
		Socks::RecvBuf.StartRL(1); // First byte tells version
	}
	Attached = TRUE;
}

VOID Socks::Process(WSANETWORKEVENTS NetEvents){
	if(NetEvents.lNetworkEvents & FD_CLOSE){
		OnFail(ErrorCode);
		Socket.Disconnect(CLOSE_BOTH_HDL);
		if(AdjunctThread)
			AdjunctThread->ConnectionClosed();
		//dprintf("Socks connection closed\r\n");
		return;
	}
	if(!Ready()){
		if(GetPosition() == CLIENT){
			if(NetEvents.lNetworkEvents & FD_CONNECT){
				if(State == CONNECT){
					if(!NetEvents.iErrorCode[FD_CONNECT_BIT]){
						Socket.EventSelect(FD_READ | FD_CLOSE);
						CHAR Methods[4];
						Methods[0] = Version;
						Methods[1] = 2; // Number of methods
						Methods[2] = METHOD_NOAUTH;
						Methods[3] = METHOD_USERPASS;
						Socket.Send(Methods, 4);
						RecvBuf.StartRL(sizeof(Method.Data));
						Method.BytesRead = 0;
						State = GET_METHOD;
					}else{
						if(NetEvents.iErrorCode[FD_CONNECT_BIT] == WSAETIMEDOUT)
							OnFail(ERR_CONTIMEDOUT);
						else
							OnFail(ERR_UNREACHABLE);
					}
				}
			}else
			if(NetEvents.lNetworkEvents & FD_READ){
				if(RecvBuf.Read(Socket) > 0){
					if(State == GET_METHOD){
						BOOL Done = RecvBuf.PopRLBuffer();
						Method.Read(RecvBuf.PoppedItem, RecvBuf.GetRLBufferSize());
						if(Done){
							UCHAR AcceptedMethod = Method.Data[1];
							if(AcceptedMethod == METHOD_NONEACCEPTABLE){
								Close(ERR_METHOD);
							}else
							if(AcceptedMethod == METHOD_NOAUTH){
								AuthDone();
							}else
							//if(AcceptedMethod == METHOD_GSSAPI){
								//todo
							//}else
							if(AcceptedMethod == METHOD_USERPASS){
								MethodType = METHOD_USERPASS;

								INT UserLen = strlen(User);
								if(UserLen > 255) UserLen = 255;
								INT PassLen = strlen(Pass);
								if(PassLen > 255) PassLen = 255;

								CHAR Request[1 + 1 + 255 + 1 + 255];
								Request[0] = 0x01;
								Request[1] = UserLen;
								strncpy(&Request[2], User, 255);
								Request[1 + 1 + UserLen] = PassLen;
								strncpy(&Request[1 + 1 + UserLen + 1], Pass, 255);
								Socket.Send(Request, 1 + 1 + UserLen + 1 + PassLen);
								Response.BytesRead = 0;
								RecvBuf.StartRL(1 + 1);
								State = AUTHENTICATE;
							}else
								Close(ERR_METHOD);
						}
					}else
					if(State == AUTHENTICATE){
						if(MethodType == METHOD_USERPASS){
							BOOL Done = RecvBuf.PopRLBuffer();
							Response.Read(RecvBuf.PoppedItem, RecvBuf.GetRLBufferSize());
							if(Done){
								if(Response.Data[1] != 0x00){
									Close(ERR_AUTHENTICATION);
								}else{
									AuthDone();
								}
							}
						}
					}else
					if(State == GET_R1){
						BOOL Done = RecvBuf.PopRLBuffer();
						R.Read(RecvBuf.PoppedItem, RecvBuf.GetRLBufferSize());
						if(Done){
							if(R.Data[1] == 0x00){
								UINT ReadLength = 0;
								switch(R.Data[3]){
									case ATYPE_V4: ReadLength = (4 - 1) + 2; break;
									case ATYPE_V6: ReadLength = (16 - 1) + 2; break;
									case ATYPE_DNS: ReadLength = R.Data[4] + 2; break;
								};
								if(!ReadLength){
									Close(ERR_REPLYFAIL);
								}else{
									RecvBuf.StartRL(ReadLength);
									State = GET_R2;
									goto get_reply2;
								}
							}else{
								Close(ERR_REPLYFAIL);
							}
						}
					}else get_reply2:
					if(State == GET_R2){
						BOOL Done = RecvBuf.PopRLBuffer();
						R.Read(RecvBuf.PoppedItem, RecvBuf.GetRLBufferSize());
						if(Done){
							State = READY;
							OnReady();
						}
					}
				}
			}
		}else
		if(GetPosition() == HOST){
			if(NetEvents.lNetworkEvents & FD_READ){
				if(RecvBuf.Read(Socket) > 0){
					if(State == GET_VERSION){
						if(RecvBuf.PopRLBuffer()){
							Version = RecvBuf.PoppedItem[0];
							if(/*Version != SOCKS_VER_4 && */Version != SOCKS_VER_5){
								Close(ERR_VERSION);
							}else{
								if(Version == SOCKS_VER_5){
									State = GET_NUMMETHODS;
									RecvBuf.StartRL(1);
									goto get_nummethods;
								}
							}
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
						if(Done){
							MethodType = METHOD_NONEACCEPTABLE;
							for(UINT i = 0; i < Generic256.BytesRead; i++){
								if(Generic256.Data[i] == METHOD_USERPASS){
									MethodType = METHOD_USERPASS;
									break;
								}
							}
							if(MethodType == METHOD_USERPASS){
								State = AUTHENTICATE;
								UserPassState = GET_ULEN;
								CHAR Reply[2];
								Reply[0] = SOCKS_VER_5;
								Reply[1] = METHOD_USERPASS;
								Socket.Send(Reply, 2);
								Method.BytesRead = 0;
								RecvBuf.StartRL(1 + 1);
								goto authenticate;
							}else
							if(MethodType == METHOD_NONEACCEPTABLE){
								Close(ERR_METHOD);
							}
						}
					}else authenticate:
					if(State == AUTHENTICATE){
						if(MethodType == METHOD_USERPASS){
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
								if(Done){
									if(memcmp(Generic256.Data, SOCKS_USER, Generic256.BytesRead > strlen(SOCKS_USER) ? strlen(SOCKS_USER) : Generic256.BytesRead) == 0){
										UserPassState = GET_PLEN;
										RecvBuf.StartRL(1);
										goto get_plen;
									}else{
										SendUserPassResponse(USERPASS_FAILURE);
										Close(ERR_AUTHENTICATION);
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
								if(Done){
									if(memcmp(Generic256.Data, SOCKS_PASS, Generic256.BytesRead > strlen(SOCKS_PASS) ? strlen(SOCKS_PASS) : Generic256.BytesRead) == 0){
										SendUserPassResponse(USERPASS_SUCCESS);
										AuthDone();
										goto get_request1;
									}else{
										SendUserPassResponse(USERPASS_FAILURE);
										Close(ERR_AUTHENTICATION);
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
								case ATYPE_V4: ReadLength = (4 - 1) + 2; break;
								case ATYPE_V6: ReadLength = (16 - 1) + 2; break;
								case ATYPE_DNS: ReadLength = R.Data[4] + 2; break;
							};
							if(!ReadLength){
								Close(ERR_REQUEST);
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
							if(R.Data[3] == ATYPE_V4){
								ULONG Addr;
								memcpy(&Addr, &R.Data[4], sizeof(Addr));
								memcpy(&Port, &R.Data[4 + 4], sizeof(Port));
								strncpy(Host, inet_ntoa(SocketFunction::Stoin(Addr)), sizeof(Host));
							}else
							if(R.Data[3] == ATYPE_V6){
								//todo
							}else
							if(R.Data[3] == ATYPE_DNS){
								strncpy(Host, (PCHAR)&R.Data[5], R.Data[4]);
								Host[R.Data[4]] = NULL;
								memcpy(&Port, &R.Data[1 + 1 + 1 + 1 + 1 + R.Data[4]], sizeof(Port));
							}
							Port = htons(Port);
							AdjunctThread = new class AdjunctThread(this);
							State = PERFORM_CMD;
							if(R.Data[1] == REQ_CONNECT)
								AdjunctThread->Connect();
							else
							if(R.Data[1] == REQ_BIND)
								AdjunctThread->Bind();
							else
							if(R.Data[1] == REQ_UDPASSOCIATE)
								AdjunctThread->UDPAssociate();
						}
					}
				}
			}
		}
	}else
		ProcessSpecial(NetEvents);
}

BOOL Socks::GetPosition(VOID){
	return Position;
}

VOID Socks::Close(UINT ErrorCode){
	Socket.Shutdown();
	Socks::ErrorCode = ErrorCode;
}

VOID::Socks::AuthDone(VOID){
	if(GetPosition() == CLIENT)
		SendR(RequestType, Host, Port);
	State = GET_R1;
	RecvBuf.StartRL(1 + 1 + 1 + 1 + /* 1st byte of addr */ 1);
	R.BytesRead = 0;
}

VOID Socks::SendUserPassResponse(BOOL Success){
	CHAR Response[2];
	Response[0] = 0x01;
	Response[1] = Success;
	Socket.Send(Response, 2);
}

UINT Socks::SetAddressField(PCHAR AddressField, PCHAR Host){
	UINT Size;
	UCHAR AddressType;
	if(inet_addr(Host) != INADDR_NONE){
		AddressType = ATYPE_V4;
	}else
	if(strstr(Host, ":")){
		AddressType = ATYPE_V6;
	}else
		AddressType = ATYPE_DNS;

	AddressField[0] = AddressType;
	if(AddressType == ATYPE_DNS){
		AddressField[1] = strlen(Host);
		strncpy(&AddressField[2], Host, 255);
		Size = 1 + strlen(Host);
	}else
	if(AddressType == ATYPE_V4){
		ULONG Addr = inet_addr(Host);
		memcpy(&AddressField[1], &Addr, sizeof(Addr));
		Size = 4;
	}else
	if(AddressType == ATYPE_V6){
		//todo
		Size = 16;
	}

	return Size;
}

VOID Socks::SendR(UCHAR Type, PCHAR Host, USHORT Port){
	CHAR Request[1 + 1 + 1 + 1 + 255 + 2];
	Request[0] = Version;
	Request[1] = Type;
	Request[2] = 0x00;
	UINT Size = SetAddressField(&Request[3], Host);
	USHORT NSPort = htons(Port);
	memcpy((PCHAR)&Request[4] + Size, &NSPort, sizeof(USHORT));
	Socket.Send((PCHAR)&Request, 1 + 1 + 1 + 1 + Size + sizeof(USHORT));
}

VOID Socks::CommandComplete(BOOL Success){
	if(State == PERFORM_CMD && Success){
		State = READY;
		OnReady();
	}
}

VOID Socks::AdjunctThreadEnded(VOID){
	AdjunctThread->Mutex.WaitForAccess();
	class AdjunctThread* AdjunctThreadOld = AdjunctThread;
	InterlockedExchangePointer(AdjunctThread, NULL);
	Socket.Shutdown();
	AdjunctThreadOld->Mutex.Release();
}

Socks::AdjunctThread::AdjunctThread(Socks* SocksObject){
	AdjunctThread::SocksObject = SocksObject;
	StartThread();
	while(!IsThreadStarted())	// May slow down other
		Sleep(0);				// connections a tiny bit
}

Socks::AdjunctThread::~AdjunctThread(VOID){
	Mutex.WaitForAccess();
	if(SocksObject)
		SocksObject->AdjunctThreadEnded();
	Socket.Disconnect(CLOSE_BOTH_HDL);
	dprintf("Closed\r\n");
	Mutex.Release();
}

VOID Socks::AdjunctThread::Connect(VOID){
	PostThreadMessage(GetThreadID(), WM_REQ_CONNECT, NULL, NULL);
}

VOID Socks::AdjunctThread::Bind(VOID){
	PostThreadMessage(GetThreadID(), WM_REQ_BIND, NULL, NULL);
}

VOID Socks::AdjunctThread::UDPAssociate(VOID){
	PostThreadMessage(GetThreadID(), WM_REQ_UDPASSOCIATE, NULL, NULL);
}

VOID Socks::AdjunctThread::ConnectionClosed(VOID){
	Mutex.WaitForAccess();
	InterlockedExchangePointer(SocksObject, NULL);
	if(Socket.GetSelectedEvents() & FD_CLOSE){
		Socket.Shutdown();
	}else{
		PostThreadMessage(AdjunctThread::GetThreadID(), WM_QUIT, NULL, NULL);
	}
	Mutex.Release();
}

VOID Socks::AdjunctThread::ThreadFunc(VOID){
	WSAEVENT EventList[1];
	Socket.CreateEvent();
	EventList[0] = Socket.GetEventHandle();
	WSANETWORKEVENTS NetEvents;
	INT SignalledEvent;
	MSG msg;
	while(1){
		if(PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE) != 0){
			if(msg.message == WM_REQ_CONNECT){
				Mutex.WaitForAccess();
				if(SocksObject){
					Socket.Create(SOCK_STREAM);
					Socket.EventSelect(FD_CONNECT);
					Socket.Connect(SocksObject->Host, SocksObject->Port);
					dprintf("Connecting to %s:%d\r\n", SocksObject->Host, SocksObject->Port);
				}else{
					Mutex.Release();
					return;
				}
				Mutex.Release();
			}else
			if(msg.message == WM_REQ_BIND){
				Mutex.WaitForAccess();
				if(SocksObject){
					Socket.Create(SOCK_STREAM);
					Socket.Bind(0);
					FireWall FireWall;
					FireWall.OpenPort(Socket.GetPort(), NET_FW_IP_PROTOCOL_TCP, L"null");
					if(Socket.Listen(4) != SOCKET_ERROR){
						Socket.EventSelect(FD_ACCEPT);
						SocksObject->SendR(0x00, inet_ntoa(SocketFunction::Stoin(Socket.GetAddr())), Socket.GetPort());
						dprintf("BIND REPLY %s %d\r\n", inet_ntoa(SocketFunction::Stoin(Socket.GetAddr())), Socket.GetPort());
					}else{
						SocksObject->SendR(0x01, "0.0.0.0", 0);
						Mutex.Release();
						return;
					}
				}else{
					Mutex.Release();
					return;
				}
				Mutex.Release();
			}else
			if(msg.message == WM_REQ_UDPASSOCIATE){
				Mutex.WaitForAccess();
				if(SocksObject){
					Socket.Create(SOCK_DGRAM);
					if(Socket.Bind(ADDR_ANY) == SOCKET_ERROR){
						SocksObject->SendR(0x01, "0.0.0.0", 0);
						Mutex.Release();
						return;
					}
					dprintf("bound address (%s:%d)\r\n", inet_ntoa(SocketFunction::Stoin(Socket.GetAddr())), Socket.GetPort());
					SocksObject->SendR(0x00, inet_ntoa(SocketFunction::Stoin(Socket.GetAddr())), Socket.GetPort());
					Socket.EventSelect(FD_READ);
				}else{
					Mutex.Release();
					return;
				}
				Mutex.Release();
			}else
			if(msg.message == WM_QUIT){
				return;
			}
		}
		if((SignalledEvent = MsgWaitForMultipleObjects(1, EventList, FALSE, INFINITE, QS_ALLPOSTMESSAGE)) != WSA_WAIT_TIMEOUT){
			SignalledEvent -= WSA_WAIT_EVENT_0;
			if(SignalledEvent == 0){
				Mutex.WaitForAccess();
				Socket.EnumEvents(&NetEvents);
				if(SocksObject){
					if(NetEvents.lNetworkEvents & FD_CONNECT){
						if(!NetEvents.iErrorCode[FD_CONNECT_BIT]){
							SocksObject->CommandComplete(TRUE);
							SocksObject->SendR(0x00, "127.0.0.1", Socket.GetPort());
							Socket.EventSelect(FD_READ | FD_CLOSE);
						}else{
							SocksObject->SendR(0x01, "0.0.0.0", 0);
							SocksObject->Close(ERR_UNREACHABLE);
							dprintf("Failed\r\n");
							Mutex.Release();
							return;
						}
					}else
					if(NetEvents.lNetworkEvents & FD_ACCEPT){
						dprintf("Accepted\r\n");
						::Socket NewSocket;
						Socket.Accept(NewSocket);
						Socket.Disconnect(CLOSE_BOTH_HDL);
						Socket = NewSocket;
						SocksObject->SendR(0x00, inet_ntoa(SocketFunction::Stoin(Socket.GetPeerAddr())), Socket.GetPeerPort());
						SocksObject->CommandComplete(TRUE);
						Socket.EventSelect(FD_READ | FD_CLOSE);
						EventList[0] = Socket.GetEventHandle();
					}else
					if(NetEvents.lNetworkEvents & FD_READ){
						CHAR Buffer[65535];
						INT Length;
						if(Socket.GetProtocol() == IPPROTO_TCP){
							while((Length = Socket.Recv(Buffer, sizeof(Buffer))) > 0)
								SocksObject->GetSocketP()->Send(Buffer, Length);
							//Buffer[Length] = NULL;
							//dprintf("READ %s\r\n", Buffer);
						}else
						if(Socket.GetProtocol() == IPPROTO_UDP){
							dprintf("sending udp\r\n");
						}
					}
				}
				if(NetEvents.lNetworkEvents & FD_CLOSE){
					Mutex.Release();
					return;
				}
				Mutex.Release();
			}
		}
	}
}

template <class S>
VOID SocksServer::SocksQueue<S>::OnEvent(WSANETWORKEVENTS NetEvents){
	Socks[SignalledEvent]->Process(NetEvents);
}

template <class S>
VOID SocksServer::SocksQueue<S>::OnAdd(VOID){
	S* Socks = new S;
	BOOL Position = HOST;
	if(SocketList.back().GetSelectedEvents() & FD_CONNECT)
		Position = CLIENT;
	Socks->Attach(SocketList.back(), RecvBuf.back(), Position);
	SocksQueue::Socks.push_back(Socks);
}

template <class S>
VOID SocksServer::SocksQueue<S>::OnClose(VOID){
	delete Socks[SignalledEvent];
	Socks.erase(Socks.begin() + SignalledEvent);
}

template <class S>
S* SocksServer::SocksQueue<S>::GetLastAdded(VOID){
	return Socks.back();
}

template <class S>
S* SocksServer::SocksQueue<S>::GetItem(UINT Index){
	return Socks[Index];
}

SocksServer::SocksServer(USHORT Port, UCHAR Version){
	if(/*Version != SOCKS_VER_4 && */Version != SOCKS_VER_5)
		return;
	SocksServer::Port = Port;
	SocksServer::Version = Version;
	StartThread();
}

VOID SocksServer::ThreadFunc(VOID){
	FireWall FireWall;
	FireWall.OpenPort(Port, NET_FW_IP_PROTOCOL_TCP, L"null");
	Socket ListeningSocket;
	ListeningSocket.Create(SOCK_STREAM);
	ListeningSocket.EventSelect(FD_ACCEPT);
	if(ListeningSocket.Bind(Port) != SOCKET_ERROR){
		if(ListeningSocket.Listen(4) == SOCKET_ERROR)
			return;
	}else
		return;
	WSAEVENT hEvent = ListeningSocket.GetEventHandle();
	WSANETWORKEVENTS NetEvents;
	INT SignalledEvent;
	while(1){
		if((SignalledEvent = WSAWaitForMultipleEvents(1, &hEvent, FALSE, 5000, FALSE)) != WSA_WAIT_TIMEOUT){
			SignalledEvent -= WSA_WAIT_EVENT_0;
			if(SignalledEvent == 0){
				ListeningSocket.EnumEvents(&NetEvents);
				if(NetEvents.lNetworkEvents & FD_ACCEPT){
					dprintf("New Connection\r\n");
					Socket NewSocket;
					ListeningSocket.Accept(NewSocket);
					NewSocket.EventSelect(FD_READ | FD_CLOSE);
					ConnectionList.Add(NewSocket);
				}
			}
		}
	}
}