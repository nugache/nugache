#include <winsock2.h>
#include "connections.h"

extern Links Links;
extern Display Display;
extern HWND UpdateWindowActive;
extern PCHAR UpdateBuffer;
extern DWORD UpdateBufferSize;
extern CHAR Signature[RSAPublicKeyMasterLen];
extern HINSTANCE hInst;
extern HWND hWndg;
HWND ListLinksWindow = NULL;

Connection::Connection(){
	StartThread();
}

VOID Connection::SetLink(Link* Link){
	ActualConnection.Link = Link;
}

Link* Connection::GetLink(VOID){
	return ActualConnection.Link;
}

VOID Connection::Connect(PCHAR Host, USHORT Port){
	WaitForAccess();
	ActualConnection.Link->Connecting();
	ActualConnection.Active = TRUE;
	ActualConnection.Connect(Host, Port, P2P2::Negotiation::TYPE_CONTROL);
	Release();
}

VOID Connection::Disconnect(VOID){
	WaitForAccess();
	ActualConnection.Close(NULL);
	Release();
}

VOID Connection::GetInfo(VOID){
	ActualConnection.GetInfo();
}

VOID Connection::ListLinks(VOID){
	ActualConnection.ListLinks();
}

Socket* Connection::GetSocketP(VOID){
	return ActualConnection.GetSocketP();
}

VOID Connection::WaitForAccess(VOID){
	ActualConnection.Mutex.WaitForAccess();
}

VOID Connection::Release(VOID){
	ActualConnection.Mutex.Release();
}

VOID Connection::SendMsg(DWORD MID, PCHAR Message, PCHAR SendTo, UINT TTL, PBYTE Signature, BOOL RunLocally){
	ActualConnection.SendMsg(MID, Message, SendTo, TTL, Signature, RunLocally);
}

BOOL Connection::Ready(VOID){
	return ActualConnection.Ready();
}

BOOL Connection::Active(VOID){
	return ActualConnection.Active;
}

VOID Connection::ThreadFunc(VOID){
	WSAEVENT hEvent = ActualConnection.GetSocketP()->GetEventHandle();
	WSANETWORKEVENTS NetEvents;
	while(!P2P2Instance)
		Sleep(50);
	while(1){
		if(WSAWaitForMultipleEvents(1, &hEvent, FALSE, 1000, FALSE) != WSA_WAIT_TIMEOUT && ActualConnection.Active){
			ActualConnection.GetSocketP()->EnumEvents(&NetEvents);
			WaitForAccess();
			ActualConnection.Process(NetEvents);
			Release();
		}else
		if(ActualConnection.GetSocketP()->GetSelectedEvents()){
			SetEvent(ActualConnection.GetSocketP()->GetEventHandle());
		}
	}
}

VOID Connection::ActualConnection::GetInfo(VOID){
	Mutex.WaitForAccess();
	Link->Waiting();
	Socket.Sendf("%d\r\n", CMD_GETINFO);
	State = GET_INFO;
	Mutex.Release();
}

VOID Connection::ActualConnection::ListLinks(VOID){
	Mutex.WaitForAccess();
	Socket.Sendf("%d\r\n", CMD_LISTLINKS);
	Mutex.Release();
}

VOID Connection::ActualConnection::SendMsg(DWORD MID, PCHAR Message, PCHAR SendTo, UINT TTL, PBYTE Signature, BOOL RunLocally){
	Mutex.WaitForAccess();
	CHAR SignatureA[(RSAPublicKeyMasterLen * 2) + 1];
	memset(SignatureA, 0, sizeof(SignatureA));
	for(UINT i = 0; i < RSAPublicKeyMasterLen; i++){
		CHAR Digit[3];
		sprintf(Digit, "%.2X", (BYTE)Signature[i]);
		strcat(SignatureA, Digit);
	}
	//Socket.Sendf("%d|%X|%s|%s|%d|%s|%d\r\n", CMD_SENDMSG, MID, Message, SendTo, TTL, SignatureA, RunLocally);
	Socket.Sendf("%d|%X|%d|%s|%d|%d|0\r\n", MSG_MESSAGE, MID, strlen(Message) + 1, SendTo, TTL, RunLocally);
	Socket.Send((PCHAR)Signature, RSAPublicKeyMasterLen);
	if(strlen(Message) > 0)
	Socket.Send(Message, strlen(Message));
	CHAR Null = 0;
	Socket.Send(&Null, sizeof(Null));
	Mutex.Release();
}

VOID Connection::ActualConnection::OnReady(VOID){
	Link->Connected();
	Timeout.SetTimeout(PING_TIMEOUT);
	Timeout.Reset();
	Display.PrintStatus("Connection ready");
	GetInfo();
}

VOID Connection::ActualConnection::OnFail(UINT ErrorCode){
	switch(ErrorCode){
		case ERR_CONNECTIONCLOSED:
			Link->Unreachable();
			Display.PrintStatus("Connection closed");
		break;
		case ERR_UNREACHABLE:
			Link->Unreachable();
			Display.PrintStatus("Host unreachable");
		break;
		case ERR_CONNECTION_CLASS_FULL:
			Link->Unreachable();
			Display.PrintStatus("Host has too many connections");
		break;
		case ERR_NETWORK:
			Link->Unreachable();
			Display.PrintStatus("Wrong network ID");
		break;
		case ERR_TIMEOUT:
			Link->Unreachable();
			Display.PrintStatus("Connection attempt timed out");
		break;
		default:
			Link->Unreachable();
			Display.PrintStatus("Connection failed");
		break;
	}
	Active = FALSE;
	cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
	UINT AutoRemove = Reg.GetInt(REG_AUTOREMOVELINKS);
	if(LinkCache::GetLinkCount() > AutoRemove){
		CHAR Encoded[256];
		LinkCache::EncodeName(Encoded, sizeof(Encoded), Link->GetHostname(), Link->GetPort());
		LinkCache::RemoveLink(Encoded);
	}
}

VOID Connection::ActualConnection::ProcessSpecial(WSANETWORKEVENTS NetEvents){
	if(NetEvents.lNetworkEvents & FD_READ){
		Timeout.Reset();
		while(RecvBuf.Read(Socket) > 0){
			while(RecvBuf.PopItem("\r\n", 2)){
				PCHAR Item[3];
				UINT Ordinal = TokenizeStr(RecvBuf.PoppedItem, Item, 3, "|");
				//dprintf("%d\r\n", Ordinal);
				if(Ordinal == RPL_GETINFO){
					if(State == GET_INFO){
						Links.RemoveNeighbors(Link);
						Link->SetClients(atoi(Item[1]));
						Link->Updated();
						
						PCHAR Host = strtok(Item[2], ",");
						cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
						UINT AutoAdd = Reg.GetInt(REG_AUTOADDLINKS);
						while(Host){
							CHAR Hostname[256];
							CHAR Encoded[256];
							USHORT Port;
							LinkCache::DecodeName(Host, Hostname, sizeof(Hostname), &Port);
							LinkCache::EncodeName(Encoded, sizeof(Encoded), Hostname, Port);
							Links.Add(Link, Encoded);
							if(LinkCache::GetLinkCount() < AutoAdd)
								LinkCache::AddLink(Encoded);
							Host = strtok(NULL, ",");
						}
						/*for(UINT i = 0; i < 20; i++){
							CHAR Name[256];
							sprintf(Name, "127.0.0.%d", i);
							Links.Add(Link, Name);
						}*/
						Display.UpdateStats();
						Display.PrintStatus("Updated information");
						Socket.Sendf("%d\r\n", CMD_GETVERSION);
					}
				}else
				if(Ordinal == CMD_PING){
					Socket.Sendf("%d\r\n", RPL_PING);
				}else
				if(Ordinal == CMD_REQUEST_UPGRADE){
					if(UpdateBufferSize){
						Socket.Sendf("%d|%d\r\n", RPL_UPGRADE_INFORMATION, UpdateBufferSize);
						DWORD TotalSent = 0;
						while(TotalSent < UpdateBufferSize){
							DWORD ToSend = 1024;
							if(UpdateBufferSize - TotalSent < 1024)
								ToSend = UpdateBufferSize - TotalSent;
							Socket.Send(UpdateBuffer + TotalSent, ToSend);
							TotalSent += ToSend;
							CHAR Text[32];
							sprintf(Text, "%d / %d", TotalSent, UpdateBufferSize);
							Mutex.Release();
							SetWindowText(GetDlgItem(UpdateWindowActive, IDC_STATIC_STATUS), Text);
							Mutex.WaitForAccess();
						}
						Socket.Send(Signature, sizeof(Signature));
						Mutex.Release();
						SetWindowText(GetDlgItem(UpdateWindowActive, IDC_STATIC_STATUS), "Completed");
						delete[] UpdateBuffer;
						UpdateBufferSize = 0;
						Mutex.WaitForAccess();
					}else{
						Socket.Sendf("%d\r\n", RPL_UPGRADE_UNAVAILABLE);
					}
				}else
				if(Ordinal == RPL_LINKITEM){
					Mutex.Release();
					if(strcmp(Item[1], "END") == 0){
						//end
					}else{
						SendMessage(hWndg, WM_APP + 1, (WPARAM)Item[1], NULL);
					}
					Mutex.WaitForAccess();
				}else
				if(Ordinal == RPL_GETVERSION){
					Display.PrintStatus(Item[1]);
				}
			}
		}
	}
	if(NetEvents.lNetworkEvents & FD_CLOSE){
		Socket.Disconnect(CLOSE_SOCKET_HDL);
	}
	if(Timeout.TimedOut()){
		if(!ShuttingDown){
			Display.PrintStatus("Ping time out");
			Socket.Shutdown();
			ShuttingDown = TRUE;
		}else{
			Socket.Disconnect(CLOSE_SOCKET_HDL);
		}
	}
}



/*
extern Links Links;
extern HWND hWndStatus;
extern HWND hWndg;

VOID Connection::Connect(LinkEx *Link, PCHAR Host, INT Port){
	PostThreadMessage(ConnectionThread.GetThreadID(), CON_MSG_SETLINK, (WPARAM)Link, 0);
	PostThreadMessage(ConnectionThread.GetThreadID(), CON_MSG_CONNECT, (WPARAM)Host, Port);
}

VOID Connection::Disconnect(VOID){
	PostThreadMessage(ConnectionThread.GetThreadID(), CON_MSG_DISCONNECT, 0, 0);
}

VOID Connection::ConnectionThread::ThreadFunc(VOID){
	MSG msg;
	memset(&msg,0,sizeof(msg));
	INT SignalledEvent = 0;

	while(1){
		WSAEVENT hEvent = Socket.GetEventHandle();
	if(!Socket.GetEventHandle() || !Link)
		Sleep(200);
	else
		if(WSAWaitForMultipleEvents(1, &hEvent, TRUE, 300, FALSE) != WSA_WAIT_TIMEOUT){
			WSANETWORKEVENTS NetEvents;
			Socket.EnumEvents(&NetEvents);
			if(NetEvents.lNetworkEvents & FD_READ){
				while(RecvBuf.Read(Socket) > 0){
					while(RecvBuf.PopItem("\r\n", 2)){
						
						PCHAR Item[3];
						memset(&Item, 0, sizeof(Item));
						Item[0] = strtok(RecvBuf.PoppedItem, ":");
						for(INT f = 1; f < 3; f++){
							if(Item[f-1])
								Item[f] = strtok(NULL, ":");
							if(!Item[f])
								break;
						}
						INT Prefix = -1;
						if(Item[0])
							Prefix = atoi(Item[0]);

							if(Prefix == RPL_GETINFO){
								SendMessage(hWndStatus, SB_SETTEXT, 0, (LPARAM)"Done");
								Link->SetStatus(LINK_STATUS_UPDATED);
								PCHAR Host[4];
								memset(&Host, 0, sizeof(Host));
								Host[0] = strtok(Item[2], "|");
								for(INT f = 1; f <= 4; f++){
									if(Host[f-1]){
										Host[f] = strtok(NULL, "|");
										if(atoi(Host[f - 1])){
											if(!Link->L[(f - 1 + 2)%4]){
												LinkEx *Temp = Links.FindHost(inet_addr(Host[f - 1]));
												if(Temp){
													Link->L[(f - 1 + 2)%4] = Temp;
												}else{
													Link->SetNumClients(atoi(Item[1]));
													Temp = Links.Add( *Link, (f - 1 + 2)%4, inet_addr(Host[f - 1]), 0 );
													Temp->L[f - 1] = Link;
												}
											}
										}
									}
									if(!Host[f])
										break;
								}
								Redraw();
								LinkEx *Temp = Links.GetWaiting();
								if(Temp){
									((Connection *)ThisP)->Connect(Temp, inet_ntoa(Stoin(Temp->GetAddr())), 51000);
								}
							}
					}
				}
			}
			if(NetEvents.lNetworkEvents & FD_CLOSE){
				SetStatus(CON_STATUS_DISCONNECTED);
				Link->ClearStatus(LINK_STATUS_CONNECTED);
				Link->ClearStatus(LINK_STATUS_CONNECTING);
				SendMessage(hWndStatus, SB_SETTEXT, 0, (LPARAM)"Connection Closed");
				Socket.Disconnect(CLOSE_BOTH_HDL);
			}

		}

		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)){
			switch(msg.message){
				case CON_MSG_SETLINK:
					Link = (LinkEx *)msg.wParam;
					break;
				case CON_MSG_CONNECT:
					SetStatus(CON_STATUS_CONNECTING);
					Link->ClearStatus(LINK_STATUS_WAITING);
					Link->ClearStatus(LINK_STATUS_CONNECTIONREFUSED);
					Link->SetStatus(LINK_STATUS_CONNECTING);
					Link->SetAddr(Socket.GetAddr((PCHAR)msg.wParam));
					char temp[64];
					sprintf(temp, "Connecting to %s...", (PCHAR)msg.wParam);
					SendMessage(hWndStatus, SB_SETTEXT, 0, (LPARAM)temp);
					Redraw();
					if(Socket.GetSocketHandle())
						Socket.Disconnect(CLOSE_BOTH_HDL);
					Socket.Create(SOCK_STREAM);
					Socket.Connect((PCHAR)msg.wParam, (INT)msg.lParam);
					if(Socket.GetLastError() == ERROR_SUCCESS){
						Link->ClearStatus(LINK_STATUS_CONNECTING);
						Link->SetStatus(LINK_STATUS_CONNECTED);
						SendMessage(hWndStatus, SB_SETTEXT, 0, (LPARAM)"Connected, requesting data...");
						SetStatus(CON_STATUS_CONNECTED);
						Socket.EventSelect(FD_READ | FD_CLOSE);
						Socket.Sendf("%d\r\n", CMD_GETINFO);
						Redraw();
					}else{
						SendMessage(hWndStatus, SB_SETTEXT, 0, (LPARAM)"Unable to connect");
						Link->SetStatus(LINK_STATUS_CONNECTIONREFUSED);
						Link->ClearStatus(LINK_STATUS_CONNECTING);
						SetStatus(CON_STATUS_CONNECTFAILED);
						Redraw();
						LinkEx *Temp = Links.GetWaiting();
						if(Temp){
							((Connection *)ThisP)->Connect(Temp, inet_ntoa(Stoin(Temp->GetAddr())), 51000);
						}
					}
				break;
				case CON_MSG_DISCONNECT:
					if(Socket.GetSocketHandle())
						Socket.Disconnect(CLOSE_BOTH_HDL);
					SetStatus(CON_STATUS_DISCONNECTED);
				break;
				case CON_MSG_SENDMESSAGE:
					Socket.Sendf("%d:%d|%d:%s\r\n", CMD_SENDMESSAGE, msg.lParam>>2, msg.lParam % 4, (PCHAR)msg.wParam);
				break;
			}
		}
	}



}*/