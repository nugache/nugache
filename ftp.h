#pragma once

#include <windows.h>
#include "socketwrapper.h"
#include "firewall.h"
#include "crypt.h"

class FTPServer : public Thread
{
public:
	FTPServer(USHORT Port, UCHAR UserHash[16], UCHAR PassHash[16]);
	~FTPServer();

private:
	VOID ThreadFunc(VOID);

	class FTPConnection : public Thread
	{
	public:
		FTPConnection(Socket & Socket, FTPServer* FTPServer);

	protected:
		~FTPConnection();

	private:
		VOID ThreadFunc(VOID);
		VOID TerminateConnection(VOID);
		BOOL ParseDataCommands(PCHAR Command, PCHAR Arguments);
		VOID QueueDataCommand(PCHAR Command, PCHAR Arguments);
		VOID SendData(PCHAR Data, UINT Length);
		VOID ConnectPORT(VOID);
		VOID ProcessLIST(PCHAR Arguments);
		VOID ProcessRETR(PCHAR Arguments);
		VOID ProcessSTOR(PCHAR Arguments);
		VOID OpeningDataConnection(VOID);
		VOID ClosingDataConnection(VOID);
		VOID PermissionDenied(VOID);
		BOOL ChangeWorkingDirectory(PCHAR Path);
		VOID ProcessDirectoryElement(PCHAR Element, CHAR Directory[MAX_PATH]);
		PCHAR ParsePathArgument(PCHAR Argument);
		PCHAR ConvertPath(PCHAR Path);

		FTPServer* FTPServer;
		Socket ControlSocket;
		Socket ListeningSocket;
		Socket DataSocket;
		ReceiveBuffer RecvBuf;
		File TransferingFile;
		BOOL WelcomeSent;
		BOOL UserReceived;
		BOOL LoggedIn;
		BOOL DataConnected;
		CHAR User[0xFF];
		CHAR WorkingDirectory[MAX_PATH];
		CHAR ConvertedPath[MAX_PATH];
		CHAR ParsedPath[MAX_PATH];
		CHAR Type;
		UINT Rest;
		BOOL DataQueued;
		BOOL StorActive;
		CHAR DataCommand[5];
		CHAR DataArguments[0xFF];
		CHAR TransferMode;
		USHORT PortPort;
		CHAR RenameFrom[0xFF];
		WSAEVENT RetrEvent;
		Timeout Timeout;
		BOOL ControlShutdown;
	};

	UCHAR UserHash[16];
	UCHAR PassHash[16];
	Socket ListeningSocket;
	USHORT Port;
};