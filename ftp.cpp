#include <winsock2.h>
#include "ftp.h"

#define FTP_NONE 0
#define FTP_PASV 1
#define FTP_PORT 2

// some clients dont close the connection after sending the STOR data still need to figure out how to know when it is done sending

FTPServer::FTPServer(USHORT Port, UCHAR UserHash[16], UCHAR PassHash[16]){
	FTPServer::Port = Port;
	memcpy(FTPServer::UserHash, UserHash, sizeof(FTPServer::UserHash));
	memcpy(FTPServer::PassHash, PassHash, sizeof(FTPServer::PassHash));
	ListeningSocket.Create(SOCK_STREAM);
	ListeningSocket.CreateEvent();
	StartThread();
}

FTPServer::~FTPServer(){
	ListeningSocket.Disconnect(CLOSE_BOTH_HDL);
}

VOID FTPServer::ThreadFunc(VOID){
	FireWall FireWall;
	FireWall.OpenPort(Port, NET_FW_IP_PROTOCOL_TCP, L"null");
	ListeningSocket.EventSelect(FD_ACCEPT);
	if(ListeningSocket.Bind(Port) != SOCKET_ERROR){
		if(ListeningSocket.Listen(4) == SOCKET_ERROR)
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
					NewSocket.EventSelect(FD_READ | FD_WRITE | FD_CLOSE);
					new FTPConnection(NewSocket, this);
				}
			}
		}
	}
}

FTPServer::FTPConnection::FTPConnection(Socket & Socket, ::FTPServer* FTPServer){
	FTPServer::FTPConnection::FTPServer = FTPServer;
	ControlSocket = Socket;
	ListeningSocket.CreateEvent();
	DataSocket.CreateEvent();
	RecvBuf.Create(1024);
	WelcomeSent = FALSE;
	UserReceived = FALSE;
	LoggedIn = FALSE;
	DataConnected = FALSE;
	DataQueued = FALSE;
	Type = 'I';
	strcpy(WorkingDirectory, "/");
	RenameFrom[0] = NULL;
	RetrEvent = WSACreateEvent();
	StorActive = FALSE;
	TransferMode = FTP_NONE;
	ControlShutdown = FALSE;
	StartThread();
}

FTPServer::FTPConnection::~FTPConnection(){
	RecvBuf.Cleanup();
	ControlSocket.Disconnect(CLOSE_BOTH_HDL);
	ListeningSocket.Disconnect(CLOSE_BOTH_HDL);
	DataSocket.Disconnect(CLOSE_BOTH_HDL);
	CloseHandle(RetrEvent);
}

VOID FTPServer::FTPConnection::ThreadFunc(VOID){
	WSAEVENT EventList[4];
	EventList[0] = ControlSocket.GetEventHandle();
	EventList[1] = ListeningSocket.GetEventHandle();
	EventList[2] = DataSocket.GetEventHandle();
	EventList[3] = RetrEvent;
	WSANETWORKEVENTS NetEvents;
	INT SignalledEvent;
	while(1){
		if((SignalledEvent = WSAWaitForMultipleEvents(4, EventList, FALSE, INFINITE, FALSE)) != WSA_WAIT_TIMEOUT){
			SignalledEvent -= WSA_WAIT_EVENT_0;
			if(SignalledEvent == 1){
				ListeningSocket.EnumEvents(&NetEvents);
				if(NetEvents.lNetworkEvents & FD_ACCEPT){
					//dprintf("accepted connection\r\n");
					ListeningSocket.Accept(DataSocket);
					DataConnected = TRUE;
					DataSocket.EventSelect(FD_WRITE | FD_READ | FD_CLOSE);
					ListeningSocket.Disconnect(CLOSE_SOCKET_HDL);
				}
			}else
			if(SignalledEvent == 2){
				DataSocket.EnumEvents(&NetEvents);
				if(NetEvents.lNetworkEvents & FD_WRITE){
					//dprintf("write\r\n");
					if(DataQueued){
						ParseDataCommands(DataCommand, DataArguments);
						DataQueued = FALSE;
					}
				}else
				if(NetEvents.lNetworkEvents & FD_CONNECT){
					//dprintf("connected\r\n");
					if(!NetEvents.iErrorCode[FD_CONNECT_BIT]){
						DataConnected = TRUE;
						DataSocket.EventSelect(FD_WRITE | FD_READ | FD_CLOSE);
						if(DataQueued){
							ParseDataCommands(DataCommand, DataArguments);
							DataQueued = FALSE;
						}
					}else{
						ControlSocket.Sendf("425 Can't connect\r\n");
					}
				}else
				if(NetEvents.lNetworkEvents & FD_READ){
					//dprintf("read\r\n");
					if(StorActive){
						BYTE Buffer[2048];
						DWORD BytesReceived;
						BytesReceived = DataSocket.Recv((PCHAR)Buffer, sizeof(Buffer));
						if(BytesReceived > 0){
							TransferingFile.Write(Buffer, BytesReceived);
						}
					}
				}else
				if(NetEvents.lNetworkEvents & FD_CLOSE || Timeout.TimedOut()){
					//dprintf("close\r\n");
					if(StorActive){
						TransferingFile.Close();
						ClosingDataConnection();
						StorActive = FALSE;
					}
					DataSocket.Disconnect(CLOSE_SOCKET_HDL);
					DataConnected = FALSE;
					Timeout.SetTimeout(NULL);
				}
			}else
			if(SignalledEvent == 3){
				ResetEvent(RetrEvent);
				BYTE Buffer[2048];
				DWORD BytesRead;			
				TransferingFile.Read(Buffer, sizeof(Buffer), &BytesRead);
				if(BytesRead > 0){
					DataSocket.BlockingMode(BLOCKING);
					SendData((PCHAR)Buffer, BytesRead);
					DataSocket.BlockingMode(NONBLOCKING);
					SetEvent(RetrEvent);
				}else{
					TransferingFile.Close();
					DataSocket.Shutdown();
					DataConnected = FALSE;
					ClosingDataConnection();
					Timeout.SetTimeout(SHUTDOWN_TIMEOUT);
					Timeout.Reset();
				}
			}else
			if(SignalledEvent == 0){
				ControlSocket.EnumEvents(&NetEvents);
				if(NetEvents.lNetworkEvents & FD_WRITE){
					if(!WelcomeSent){
						//dprintf("sending welcome\r\n");
						ControlSocket.Sendf("220-\r\n220-\tWelcome (%s)\r\n220 \r\n", Config::GetUUIDAscii());
						WelcomeSent = TRUE;
					}
				}else
				if(NetEvents.lNetworkEvents & FD_READ){
					while(RecvBuf.Read(ControlSocket) > 0){
						while(RecvBuf.PopItem("\n", 1)){
							if(RecvBuf.PoppedItem[strlen(RecvBuf.PoppedItem) - 1] == '\r')
								RecvBuf.PoppedItem[strlen(RecvBuf.PoppedItem) - 1] = NULL;
							PCHAR Command = RecvBuf.PoppedItem;
							PCHAR Arguments = NULL;
							if(strchr(Command, ' ')){
								Command[strchr(Command, ' ') - Command] = NULL;
								Arguments = Command + strlen(Command) + 1;
							}
							for(UINT i = 0; i < strlen(Command); i++)
								Command[i] = toupper(Command[i]);
							
							//dprintf("Command: %s, Arguments: %s\r\n", Command, Arguments);
							if(strlen(Command) > 4 || (Arguments && strlen(Arguments) >= 0xFF)){
								ControlSocket.Sendf("500 Command was too long\r\n");
								TerminateConnection();
							}else
							if(strcmp(Command, "QUIT") == 0){
								ControlSocket.Sendf("221 Goodbye\r\n");
								DataSocket.Shutdown();
								ControlSocket.Shutdown();
								ControlShutdown = TRUE;
								Timeout.SetTimeout(SHUTDOWN_TIMEOUT);
								Timeout.Reset();
							}else
							if(strcmp(Command, "NOOP") == 0){
								ControlSocket.Sendf("200 NOOP ok.\r\n");
							}else
							if(!LoggedIn){
								if(strcmp(Command, "USER") == 0){
									strncpy(User, Arguments, sizeof(User));
									ControlSocket.Sendf("331 Password required for %s.\r\n", Arguments);
									UserReceived = TRUE;
								}else
								if(strcmp(Command, "PASS") == 0){
									if(!UserReceived){
										ControlSocket.Sendf("503 Login with USER first.\r\n");
									}else{
										UCHAR Hash[16], Hash2[16];
										MD5 MD5, MD52;
										MD5.Update((PUCHAR)User, strlen(User));
										MD5.Finalize(Hash);
										MD52.Update((PUCHAR)Arguments, strlen(Arguments));
										MD52.Finalize(Hash2);
										if(memcmp(FTPServer->UserHash, Hash, sizeof(Hash)) == 0 && memcmp(FTPServer->PassHash, Hash2, sizeof(Hash2)) == 0){
											ControlSocket.Sendf("230 %s user logged in.\r\n", User);
											LoggedIn = TRUE;
										}else{
											ControlSocket.Sendf("530 Login incorrect\r\n");
										}
										UserReceived = FALSE;
									}
								}else{
									ControlSocket.Sendf("530 Please login with USER and PORT.\r\n");
								}
							}else{
								if(strcmp(Command, "PWD") == 0 || strcmp(Command, "XPWD") == 0){
									ControlSocket.Sendf("257 \"%s\" is current directory.\r\n", WorkingDirectory);
								}else
								if(strcmp(Command, "CWD") == 0 || strcmp(Command, "CDUP") == 0){
									PCHAR Directory = Arguments;
									if(strcmp(Command, "CDUP") == 0)
										Directory = "..";
									if(ChangeWorkingDirectory(Directory))
										ControlSocket.Sendf("250 Directory successfully changed.\r\n");
									else
										ControlSocket.Sendf("550 Failed to change directory.\r\n");
								}else
								if(strcmp(Command, "PORT") == 0){
									UINT ArgumentSize = strlen(Arguments);
									PCHAR Token = strtok(Arguments, ",");
									ULONG Addr = NULL;
									USHORT Port = NULL;
									UINT ShiftAddr = 32;
									BOOL Okay = FALSE;
									while(Token){
										if(ShiftAddr){
											ShiftAddr -= 8;
											Addr |= BYTE(atoi(Token)) << ShiftAddr;
										}else{
											Port |= BYTE(atoi(Token)) << 8;
											if(ArgumentSize > (Token - Arguments)){
												Port |= BYTE(atoi(Token + strlen(Token) + 1));
												Okay = TRUE;
											}
											break;
										}
										Token = strtok(NULL, ",");
									}
									if(Okay){
										//dprintf("Port = %d,%d (%d) %s\r\n", (Port & 0xFF00) >> 8, (BYTE)Port, Port, inet_ntoa(SocketFunction::Stoin(ControlSocket.GetAddr())));
										/*DataSocket.Disconnect(CLOSE_SOCKET_HDL);
										DataSocket.Create(SOCK_STREAM);
										DataSocket.EventSelect(FD_CONNECT);
										DataSocket.Connect(inet_ntoa(SocketFunction::Stoin(ControlSocket.GetPeerAddr())), Port);*/
										//dprintf("%d\r\n", WSAGetLastError());
										ControlSocket.Sendf("200 PORT command successful.\r\n");
										PortPort = Port;
										TransferMode = FTP_PORT;
									}else{
										ControlSocket.Sendf("500 Illegal PORT command.\r\n");
									}
								}else
								if(strcmp(Command, "PASV") == 0){
									ListeningSocket.Disconnect(CLOSE_SOCKET_HDL);
									ListeningSocket.Create(SOCK_STREAM);
									ListeningSocket.EventSelect(FD_ACCEPT);
									ListeningSocket.Bind(INADDR_ANY);
									ULONG Addr = ControlSocket.GetAddr();
									USHORT Port = ListeningSocket.GetPort();
									FireWall FireWall;
									FireWall.OpenPort(Port, NET_FW_IP_PROTOCOL_TCP, L"null");
									if(ListeningSocket.Listen(1) == SOCKET_ERROR){
										ControlSocket.Sendf("425 Can't open passive connection\r\n");
									}else{
										ControlSocket.Sendf("227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)\r\n", (BYTE)Addr, (Addr & 0xFF00) >> 8, (Addr & 0xFF0000) >> 16, (Addr & 0xFF000000) >> 24, (Port & 0xFF00) >> 8, Port & 0xFF);
										TransferMode = FTP_PASV;
										DataQueued = FALSE;
									}
								}else
								if(strcmp(Command, "TYPE") == 0){
									if(strcmp(Arguments, "I") == 0 || strcmp(Arguments, "i") == 0){
										ControlSocket.Sendf("200 Switching to Binary mode.\r\n");
										Type = 'I';
									}else
									if(strcmp(Arguments, "A") == 0 || strcmp(Arguments, "a") == 0){
										ControlSocket.Sendf("200 Switching to ASCII mode.\r\n");
										Type = 'A';
									}else{
										ControlSocket.Sendf("500 Unrecognised TYPE commmand.\r\n");
									}
								}else
								if(strcmp(Command, "ABOR") == 0){
									DataSocket.Shutdown();
									DataConnected = FALSE;
									ControlSocket.Sendf("225 ABOR command successful.\r\n");
									Timeout.SetTimeout(SHUTDOWN_TIMEOUT);
									Timeout.Reset();
								}else
								if(strcmp(Command, "FEAT") == 0){
									ControlSocket.Sendf("211-FEAT\r\n SIZE\r\n REST STREAM\r\n211 END\r\n");
								}else
								if(strcmp(Command, "DELE") == 0){
									if(!DeleteFile(ParsePathArgument(Arguments))){
										PermissionDenied();
									}else{
										ControlSocket.Sendf("250 File deletion successful.\r\n");
									}
								}else
								if(strcmp(Command, "RMD") == 0){
									if(!RemoveDirectory(ParsePathArgument(Arguments))){
										PermissionDenied();
									}else{
										ControlSocket.Sendf("250 Directory removed.\r\n");
									}
								}else
								if(strcmp(Command, "MKD") == 0){
									if(!CreateDirectory(ParsePathArgument(Arguments), NULL)){
										PermissionDenied();
									}else{
										ControlSocket.Sendf("250 Directory created.\r\n");
									}
								}else
								if(strcmp(Command, "SIZE") == 0){
									File TempFile;
									if(TempFile.Open(ParsePathArgument(Arguments), GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL) == INVALID_HANDLE_VALUE){
										PermissionDenied();
									}else{
										ControlSocket.Sendf("213 %d\r\n", TempFile.GetSize());
									}
								}else
								if(strcmp(Command, "RNFR") == 0){
									if(GetFileAttributes(ParsePathArgument(Arguments)) == INVALID_FILE_ATTRIBUTES){
										PermissionDenied();
									}else{
										ControlSocket.Sendf("350 Ok. Ready for destination name.\r\n");
										strcpy(RenameFrom, Arguments);
									}
								}else
								if(strcmp(Command, "RNTO") == 0){
									if(!RenameFrom[0]){
										PermissionDenied();
									}else{
										CHAR RenameTo[MAX_PATH];
										strcpy(RenameTo, ParsePathArgument(Arguments));
										if(MoveFile(ParsePathArgument(RenameFrom), RenameTo)){
											ControlSocket.Sendf("250 Directory renamed.\r\n");
										}else{
											PermissionDenied();
										}
									}
									RenameFrom[0] = NULL;
								}else
								if(strcmp(Command, "REST") == 0){
									UINT Temp = atoi(Arguments);
									if(Temp < 0 || Temp >= 0xFFFFFFFF){
										ControlSocket.Sendf("501 Reply marker is invalid.\r\n");
									}else{
										Rest = Temp;
										ControlSocket.Sendf("350 Restarting at %d.\r\n", Rest);
									}
								}else
								if(ParseDataCommands(Command, Arguments)){
									
								}else{
									ControlSocket.Sendf("500 '%s': command not understood\r\n", Command);
								}
							}
						}
					}
				}else
				if(NetEvents.lNetworkEvents & FD_CLOSE || ControlShutdown && Timeout.TimedOut()){
					return;
				}
			}
		}
	}
}

VOID FTPServer::FTPConnection::TerminateConnection(VOID){
	ControlSocket.Sendf("421 Terminating connection.\r\n");
	ControlSocket.Shutdown();
	ControlShutdown = TRUE;
	Timeout.SetTimeout(SHUTDOWN_TIMEOUT);
	Timeout.Reset();
}

VOID FTPServer::FTPConnection::ConnectPORT(VOID){
	DataSocket.Disconnect(CLOSE_SOCKET_HDL);
	DataSocket.Create(SOCK_STREAM);
	DataSocket.EventSelect(FD_CONNECT);
	DataSocket.Connect(inet_ntoa(SocketFunction::Stoin(ControlSocket.GetPeerAddr())), PortPort);
}

BOOL FTPServer::FTPConnection::ParseDataCommands(PCHAR Command, PCHAR Arguments){
	if(strcmp(Command, "LIST") == 0){
		//dprintf("LIST\r\n");
		if(DataConnected){
			ProcessLIST(Arguments);
		}else
			QueueDataCommand(Command, Arguments);
		return TRUE;
	}else
	if(strcmp(Command, "RETR") == 0){
		//dprintf("RETR\r\n");
		if(DataConnected){
			ProcessRETR(Arguments);
		}else
			QueueDataCommand(Command, Arguments);
		return TRUE;
	}else
	if(strcmp(Command, "STOR") == 0){
		if(DataConnected){
			ProcessSTOR(Arguments);
		}else
			QueueDataCommand(Command, Arguments);
		return TRUE;
	}
	return FALSE;
}

VOID FTPServer::FTPConnection::QueueDataCommand(PCHAR Command, PCHAR Arguments){
	//dprintf("queued command\r\n");
	DataQueued = TRUE;
	strcpy(DataCommand, Command);
	if(Arguments)
		strcpy(DataArguments, Arguments);
	else
		DataArguments[0] = NULL;
	if(TransferMode == FTP_PORT)
		ConnectPORT();
}

VOID FTPServer::FTPConnection::SendData(PCHAR Data, UINT Length){
	if(Type == 'I'){
		//dprintf("send\r\n");
		DataSocket.Send(Data, Length);
	}else
	if(Type == 'A'){
		for(UINT i = 0; i < Length; i++){
			if(Data[i] == '\n'){
				DataSocket.Send("\r\n", 2);
			}else{
				CHAR Temp = Data[i] & 0x7F;
				DataSocket.Send(&Temp, 1);
			}
		}
	}
}

VOID FTPServer::FTPConnection::ProcessLIST(PCHAR Arguments){
	OpeningDataConnection();
	CHAR Path[MAX_PATH];
	strncpy(Path, WorkingDirectory, sizeof(Path));
	if(strcmp(Path, "/") == 0){
		SetErrorMode(SEM_FAILCRITICALERRORS);
		CHAR Drive[5] = "A:\\";
		for(UINT i = 'A'; i <= 'Z'; i++){
			Drive[0] = i;
			UINT DriveType = GetDriveType(Drive);
			if(DriveType != DRIVE_UNKNOWN && DriveType != DRIVE_NO_ROOT_DIR){
				CHAR Temp[64];
				sprintf(Temp, "drwxrwxrwx   1 ftp ftp          0 Jan  1 12:00 %c:\r\n", i);
				SendData(Temp, strlen(Temp));
			}
		}
	}else{
		strncpy(Path, ConvertPath(Path), sizeof(Path));
		strcat(Path, "\\*");
		//dprintf("%s\r\n", Path);
		WIN32_FIND_DATA FindData;
		HANDLE Handle = FindFirstFile(Path, &FindData);
		Path[strrchr(Path, '\\') - Path] = NULL;
		if(Handle != INVALID_HANDLE_VALUE){
			do{
				CHAR FileType = '-';
				if(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					FileType = 'd';

				CHAR Path2[MAX_PATH];
				strcpy(Path2, Path);
				strcat(Path2, "\\");
				strncat(Path2, FindData.cFileName, sizeof(Path));
				//dprintf("%s\r\n", Path2);
				
				SYSTEMTIME WTime, STime;
				FileTimeToSystemTime(&FindData.ftLastWriteTime, &WTime);
				GetSystemTime(&STime);
				PCHAR Months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
				CHAR Date[64];
				if(WTime.wYear < STime.wYear)
					sprintf(Date, "%s %s%d  %d", Months[WTime.wMonth % 12], WTime.wDay < 9 ? " " : "", WTime.wDay, WTime.wYear);
				else
					sprintf(Date, "%s %s%d %.2d:%.2d", Months[WTime.wMonth % 12], WTime.wDay < 9 ? " " : "", WTime.wDay, WTime.wHour, WTime.wSecond);
				CHAR Temp[MAX_PATH + 32];
				CHAR Spaces[10];
				memset(Spaces, ' ', sizeof(Spaces));
				UINT NumberOfSpaces = 9;
				UINT TempSize = FindData.nFileSizeLow;
				while(TempSize > 10){
					TempSize /= 10;
					NumberOfSpaces--;
				}
				Spaces[NumberOfSpaces] = NULL;
				sprintf(Temp, "%crwxrwxrwx   1 ftp ftp %s%d %s %s\r\n", FileType, Spaces, FindData.nFileSizeLow, Date, FindData.cFileName);
				SendData(Temp, strlen(Temp));
			}while(FindNextFile(Handle, &FindData));
			FindClose(Handle);
		}
	}
	DataSocket.Shutdown();
	Timeout.SetTimeout(SHUTDOWN_TIMEOUT);
	Timeout.Reset();
	DataConnected = FALSE;
	ClosingDataConnection();
}

VOID FTPServer::FTPConnection::ProcessRETR(PCHAR Arguments){
	if(TransferingFile.Open(ParsePathArgument(Arguments), GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL) == INVALID_HANDLE_VALUE){
		ControlSocket.Sendf("550 Failed to open file.\r\n");
	}else{
		OpeningDataConnection();
		if(Rest < TransferingFile.GetSize())
			TransferingFile.SetPointer(FILE_BEGIN, Rest);
		SetEvent(RetrEvent);
/*		BYTE Buffer[2048];
		DWORD BytesRead;
		DataSocket.BlockingMode(BLOCKING);
		do{
			TransferingFile.Read(Buffer, sizeof(Buffer), &BytesRead);
			SendData((PCHAR)Buffer, BytesRead);
		}while(BytesRead > 0);
		TransferingFile.Close();
		DataSocket.BlockingMode(NONBLOCKING);
		DataSocket.Shutdown();
		ClosingDataConnection();*/
	}
}

VOID FTPServer::FTPConnection::ProcessSTOR(PCHAR Arguments){
	if(TransferingFile.Open(ParsePathArgument(Arguments)) == INVALID_HANDLE_VALUE){
		ControlSocket.Sendf("553 File name not allowed.\r\n");
	}else{
		OpeningDataConnection();
		StorActive = TRUE;
		if(Rest <= TransferingFile.GetSize())
			TransferingFile.SetPointer(FILE_BEGIN, Rest);
	}
}

VOID FTPServer::FTPConnection::OpeningDataConnection(VOID){
	ControlSocket.Sendf("150 Transfer starting.\r\n");
}

VOID FTPServer::FTPConnection::ClosingDataConnection(VOID){
	ControlSocket.Sendf("226 Transfer complete.\r\n");
}

VOID FTPServer::FTPConnection::PermissionDenied(VOID){
	ControlSocket.Sendf("550 Permission denied.\r\n");
}

BOOL FTPServer::FTPConnection::ChangeWorkingDirectory(PCHAR Path){
	CHAR NewWorkingDirectory[MAX_PATH];
	strcpy(NewWorkingDirectory, WorkingDirectory);
	CHAR PathW[MAX_PATH];
	strncpy(PathW, Path, sizeof(PathW));
	for(UINT i = 0; i < strlen(PathW); i++)
		if(PathW[i] == '\\') PathW[i] = '/';
	PCHAR Slash;
	PCHAR PathP = PathW;

	if(PathW[0] == '/')
		NewWorkingDirectory[0] = NULL;

	while((Slash = strchr(PathP, '/'))){
		*Slash = NULL;
		PCHAR Directory = PathP;
		PathP = Slash + 1;
		ProcessDirectoryElement(Directory, NewWorkingDirectory);
	}
	if(*PathP)
		ProcessDirectoryElement(PathP, NewWorkingDirectory);

	if(!NewWorkingDirectory[0])
		strcpy(NewWorkingDirectory, "/");

	if(GetFileAttributes(ConvertPath(NewWorkingDirectory)) != INVALID_FILE_ATTRIBUTES || strcmp(NewWorkingDirectory, "/") == 0){
		strcpy(WorkingDirectory, NewWorkingDirectory);
		return TRUE;
	}
	return FALSE;
}

VOID FTPServer::FTPConnection::ProcessDirectoryElement(PCHAR Element, CHAR Directory[]){
	if(!*Element || strcmp(Element, ".") == 0){
		// do nothing
	}else
	if(strcmp(Element, "..") == 0){
		PCHAR Previous = strrchr(Directory, '/');
		if(Previous){
			*Previous = NULL;
			if(Directory[0] == NULL)
				strcpy(Directory, "/");
		}
	}else{
		if(strcmp(Directory, "/") != 0)
			strcat(Directory, "/");
		strcat(Directory, Element);
	}
}

PCHAR FTPServer::FTPConnection::ParsePathArgument(PCHAR Arguments){
	if(Arguments[0] == '/'){
		strcpy(ParsedPath, ConvertPath(Arguments));
	}else{
		strncpy(ParsedPath, ConvertPath(WorkingDirectory), sizeof(ParsedPath));
		strcat(ParsedPath, "\\");
		strncat(ParsedPath, Arguments, sizeof(ParsedPath));
	}
	return ParsedPath;
}

PCHAR FTPServer::FTPConnection::ConvertPath(PCHAR Path){
	strncpy(ConvertedPath, &Path[1], sizeof(ConvertedPath));
	for(UINT i = 0; i < strlen(ConvertedPath); i++)
		if(ConvertedPath[i] == '/')
			ConvertedPath[i] = '\\';
	return ConvertedPath;
}