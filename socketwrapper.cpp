#include <winsock2.h>
#include "socketwrapper.h"

PBYTE WordToArray(WORD Word, PBYTE Array){
	Array[0] = (BYTE)Word;
	Array[1] = (BYTE)(Word >> 8);
	return Array;
}

WORD ArrayToWord(PBYTE Array){
	return WORD((Array[1] << 8) + (Array[0]));
}

INT TokenizeStr(PCHAR Buffer, PCHAR Item[], UINT Size, PCHAR Delimiter){
	if(Size <= 1)
		return -1;

	for(INT i = 0; i < Size; i++)
		Item[i] = NULL;

	Item[0] = strtok(Buffer, Delimiter);
	for(INT f = 1; f < Size; f++){
		if(Item[f-1]){
			if(f == (Size - 1))
				Item[f] = Item[f - 1] + strlen(Item[f - 1]) + 1;
			else
				Item[f] = strtok(NULL, Delimiter);
		}
		if(!Item[f])
			break;
	}
	INT Prefix = -1;
	if(Item[0])
		Prefix = atoi(Item[0]);
	
	return Prefix;
}

ULONG SocketFunction::GetAddr(PCHAR Host){
	ULONG Addr;
	if((Addr = inet_addr(Host)) == INADDR_NONE){
		struct hostent *H;
		H = gethostbyname(Host);
		if(H)
			Addr = ((IN_ADDR*)H->h_addr)->S_un.S_addr;
	}
	return Addr;
}

USHORT SocketFunction::Checksum(PUSHORT Buffer, UINT Size){
	ULONG ChkSum;

	for(ChkSum = 0; Size > 0; Size--)
		ChkSum += *Buffer++;

	ChkSum = (ChkSum >> 16) + (ChkSum & 0xffff);
	ChkSum += (ChkSum >> 16);

	return ~ChkSum;
}

in_addr SocketFunction::Stoin(ULONG Addr){
	in_addr tmp;
	tmp.S_un.S_addr = Addr;
	return tmp;
}

Socket::Socket(){
	hEvent = WSA_INVALID_EVENT;
	hSocket = INVALID_SOCKET;
	Events = NULL;
	ConnectHost[0] = NULL;
}

INT Socket::Create(INT Type){
	Socket::Type = Type;
	Family = AF_INET;
	if(Type == SOCK_STREAM)
		Protocol = IPPROTO_TCP;
	else if(Type == SOCK_DGRAM)
		Protocol = IPPROTO_UDP;
	else{
		Protocol = NULL;
	}
	return Create(Family, Type, Protocol);
}

INT Socket::Create(INT Family, INT Type, INT Protocol){
	Socket::Family = Family;
	Socket::Type = Type;
	Socket::Protocol = Protocol;
	hSocket = socket(Socket::Family, Socket::Type, Socket::Protocol);
	return hSocket;
}

#ifdef CRYPT_ENABLED
VOID Socket::Crypt(::CFB CFB){
	Socket::CFBS = CFB;
	Socket::CFBR = CFB;
}

VOID Socket::StopCrypt(VOID){
	CFBS.ResetKey();
	CFBR.ResetKey();
}

BOOL Socket::IsCrypted(VOID){
	return CFBS.KeySet() || CFBR.KeySet();
}
#endif

VOID Socket::FillAddrStruct(sockaddr_in *SockAddr, PCHAR Host, USHORT Port){
	memset(SockAddr->sin_zero, 0, sizeof(SockAddr->sin_zero));
	SockAddr->sin_family = Socket::Family;
	SockAddr->sin_addr.s_addr = SocketFunction::GetAddr(Host);
	SockAddr->sin_port = Port == 0 ? htons(Socket::Port) : htons(Port);
}

INT Socket::Bind(USHORT Port){
	Socket::Port = Port;
	sockaddr_in SockAddr;
	FillAddrStruct(&SockAddr, "");
	INT Return = bind(hSocket, (sockaddr *)&SockAddr, sizeof(SockAddr));
	Socket::Port = GetPort();
	return Return;
}

INT Socket::BlockingMode(BOOL Mode){
	ULONG Opt = Mode;
	if(Mode == BLOCKING)
		EventSelect(NULL);
	return(ioctlsocket(hSocket, FIONBIO, &Opt));
}

INT Socket::Listen(INT Backlog){
	return(listen(hSocket, Backlog));
}

INT Socket::Accept(Socket & Socket){
	sockaddr_in Addr;
	INT AddrLen = sizeof(Addr);
	INT Return = (INT)accept(hSocket,(sockaddr *)&Addr,&AddrLen);
	Socket.hSocket = Return;
	Socket.Family = Addr.sin_family;
	Socket.Port = ntohs(Addr.sin_port);
	Socket.Protocol = IPPROTO_TCP;
	Socket.Type = SOCK_STREAM;
	return Return;
}

INT Socket::Connect(const PCHAR Host, USHORT Port){
	Socket::Port = Port;
	strncpy(ConnectHost, Host, sizeof(ConnectHost));

	sockaddr_in SockAddr;
	FillAddrStruct(&SockAddr, Host);

	return(connect(hSocket, (struct sockaddr *)&SockAddr, sizeof(SockAddr)));
}

WSAEVENT Socket::CreateEvent(VOID){
	hEvent = WSACreateEvent();
	return hEvent;
}

INT Socket::EventSelect(LONG Events){
	if(hEvent == WSA_INVALID_EVENT)
		CreateEvent();
	Socket::Events = Events;
	return(WSAEventSelect(hSocket, hEvent, Events));
}

LONG Socket::GetSelectedEvents(VOID) const {
	return Events;
}

INT Socket::AsyncSelect(HWND hWnd, UINT Msg, LONG Events){
	return(WSAAsyncSelect(hSocket, hWnd, Msg, Events));
}

PCHAR Socket::GetConnectHost( VOID ) const {
	return (PCHAR)ConnectHost;
}

USHORT Socket::GetConnectPort( VOID ) const {
	return Port;
}

WSAEVENT Socket::GetEventHandle(VOID) const {
	return hEvent;
}

SOCKET Socket::GetSocketHandle(VOID) const {
	return hSocket;
}

INT Socket::GetProtocol(VOID) const {
	return Protocol;
}

INT Socket::GetPeerName(sockaddr *SockAddr, PINT Size){
	return(getpeername(hSocket, SockAddr, Size));
}

VOID Socket::GetPeerName(PCHAR Name, UINT Size){
	strncpy(Name, inet_ntoa(SocketFunction::Stoin(GetPeerAddr())), Size);
}

ULONG Socket::GetPeerAddr(VOID){
	sockaddr_in Temp;
	INT Size = sizeof(Temp);
	GetPeerName((sockaddr *)&Temp, &Size);
	return Temp.sin_addr.S_un.S_addr;
}

USHORT Socket::GetPeerPort(VOID){
	sockaddr_in Temp;
	INT Size = sizeof(Temp);
	GetPeerName((sockaddr *)&Temp, &Size);
	return htons(Temp.sin_port);
}

ULONG Socket::GetAddr(VOID){
	sockaddr_in Temp;
	INT Size = sizeof(Temp);
	GetName((sockaddr *)&Temp, &Size);
	return Temp.sin_addr.S_un.S_addr;
}

INT Socket::GetName(sockaddr* SockAddr, PINT Size){
	return(getsockname(hSocket, SockAddr, Size));
}

USHORT Socket::GetPort(VOID){
	sockaddr_in Temp;
	INT Size = sizeof(Temp);
	GetName((sockaddr *)&Temp, &Size);
	return htons(Temp.sin_port);
}

BOOL Socket::ResetEvent(VOID) const{
	return WSAResetEvent(hEvent);
}

INT Socket::EnumEvents(LPWSANETWORKEVENTS NetEvents) const {
	if(!hSocket)
		ResetEvent();
	return WSAEnumNetworkEvents(hSocket, hEvent, NetEvents);
}

INT Socket::Send(const PCHAR Buffer, INT Length){
	PCHAR SendBuf = Buffer;
#ifdef CRYPT_ENABLED
	SendBuf = new CHAR[Length];
	memcpy(SendBuf, Buffer, Length);
	if(CFBS.KeySet())
		CFBS.Crypt((PUCHAR)SendBuf, Length);
#endif
	INT Total = 0;
	INT Bytes;
	while(Total < Length){
		Bytes = send(hSocket, SendBuf + Total, Length - Total, 0);
		if(Bytes > 0){
			Total += Bytes;
		}else{
			if(WSAGetLastError() != WSAEWOULDBLOCK)
				break;
		}
	}
#ifdef CRYPT_ENABLED
	delete[] SendBuf;
#endif
	return Bytes;
}

INT Socket::Sendf(const PCHAR Format, ...){
	va_list Args;
	va_start(Args, Format);
	INT Length = _vscprintf(Format, Args);
	PCHAR SendBuf = new CHAR[Length + 1];
	vsprintf(SendBuf, Format, Args);
	INT Bytes = Send(SendBuf, Length);
	delete[] SendBuf;
	return Bytes;
}

INT Socket::SendTo(const PCHAR Buffer, INT Length, sockaddr* SockAddr, INT Size){
	INT Bytes = sendto(hSocket, Buffer, Length, NULL, SockAddr, Size);
	return Bytes;
}

INT Socket::Recv(PCHAR Buffer, INT Length){
	INT Return = recv(hSocket, Buffer, Length, 0);
#ifdef CRYPT_ENABLED
	if(CFBR.KeySet() && Return > 0)
		CFBR.Crypt((PUCHAR)Buffer, Return);
#endif
	return Return;
}

INT	Socket::RecvFrom(PCHAR Buffer, INT Length, sockaddr* SockAddr, PINT Size){
	return(recvfrom(hSocket, Buffer, Length, NULL, SockAddr, Size));
}

VOID Socket::Disconnect(INT Close){
	Shutdown();
	CHAR buf[1024];
	Events = NULL;

	while(Recv(buf, sizeof(buf)) > 0);

	if(hSocket != INVALID_SOCKET && Close & CLOSE_SOCKET_HDL){
		closesocket(hSocket);
		hSocket = INVALID_SOCKET;
	}
	if(hEvent != WSA_INVALID_EVENT && Close & CLOSE_EVENT_HDL){
		WSACloseEvent(hEvent);
		hEvent = WSA_INVALID_EVENT;
	}
	ResetEvent();
}

INT Socket::Shutdown(VOID){
	return Shutdown(SD_BOTH);
}

INT Socket::Shutdown(INT How){
	return(shutdown(hSocket, How));
}

AsyncDNS::AsyncDNS(DWORD ThreadID, PCHAR Hostname, ULONG* Addr){
	AsyncDNS::ThreadID = ThreadID;
	AsyncDNS::Hostname = new CHAR[strlen(Hostname) + 1];
	strcpy(AsyncDNS::Hostname, Hostname);
	AsyncDNS::Addr = Addr;
	StartThread();
}

AsyncDNS::~AsyncDNS(){
	delete Hostname;
}

VOID AsyncDNS::ThreadFunc(VOID){
	*Addr = SocketFunction::GetAddr(Hostname);
	PostThreadMessage(ThreadID, WM_USER_GETHOSTBYNAME, WSAGetLastError(), NULL);
}

ReceiveBuffer::ReceiveBuffer(){
	RecvBuf = NULL;
	PoppedItem = NULL;
	Reset();
}

VOID ReceiveBuffer::Cleanup(VOID){
	if(RecvBuf){
		delete RecvBuf;
		RecvBuf = NULL;
	}
	if(PoppedItem){
		delete PoppedItem;
		PoppedItem = NULL;
	}
}

VOID ReceiveBuffer::Create(INT Length){
	ReceiveBuffer::Length = Length;
	RecvBuf = new CHAR[Length];
	PoppedItem = new CHAR[Length];
}

INT ReceiveBuffer::GetBytesRead(VOID) const {
	return BytesRead;
}

INT ReceiveBuffer::Read(Socket & Socket){
	memset(RecvBuf + Offset, 0, Length - Offset);
	INT Return = Socket.Recv((PCHAR)RecvBuf + Offset, (Length - Offset));
	if(Return >= 0){
		BytesRead = Return + Offset;
		if(InRLMode()){
			RLOldTotalRead = RLTotalRead;
			if((RLSize - RLTotalRead) < BytesRead){
				RLTotalRead += (RLSize - RLTotalRead);
			}else
				RLTotalRead += BytesRead;
		}
	}
	return Return;
}

VOID ReceiveBuffer::Reset(VOID){
	Offset = 0;
	OverFlow = FALSE;
	RLSize = 0;
	RLTotalRead = 0;
	RLOldTotalRead = 0;
	BytesRead = 0;
}

BOOL ReceiveBuffer::PopItem(const PCHAR Delimiter, INT DSize){
	INT i;
	for(i = 0; i < BytesRead; i++){
		PoppedItem[i] = RecvBuf[i];
		for(INT i2 = 0; i2 <= DSize; i2++){
			if(RecvBuf[i+i2] != Delimiter[i2]){
				i2 = DSize + 1;
				break;
			}
			if(i2 == (DSize - 1)){
				memset(PoppedItem + i, 0, DSize);
				memcpy(RecvBuf, RecvBuf + i + DSize, Length/* added -> */ - i - DSize /* */);
				BytesRead -= (i + DSize);
				if(OverFlow){
					OverFlow = FALSE;
					PoppedItem[0] = NULL;
				}
				return 1;
			}
		}
		if(i == Length - DSize){
			OverFlow = TRUE;
			BytesRead = 0;
			PoppedItem[0] = NULL;
			break;
		}
	}
	Offset = i;
	return 0;
}

//BytesRead contains number of bytes read in the buffer that have not been popped

VOID ReceiveBuffer::PopBuffer(VOID){
	memcpy(PoppedItem, RecvBuf, GetBytesRead());
	Offset = 0;
}

VOID ReceiveBuffer::StartRL(UINT Bytes){
	if(Bytes < (unsigned)BytesRead)
		RLTotalRead = Bytes;
	else
		RLTotalRead = BytesRead;
	RLOldTotalRead = 0;
	RLSize = Bytes;
}

INT ReceiveBuffer::GetRLBufferSize(VOID){
	INT Bytes = BytesRead;
	if((RLSize - RLOldTotalRead) < BytesRead){
		Bytes = (RLSize - RLOldTotalRead);
	}
	return abs(Bytes);
}

BOOL ReceiveBuffer::PopRLBuffer(VOID){
	memcpy(PoppedItem, RecvBuf, GetRLBufferSize());

	if(GetRLBufferSize() < BytesRead){
		Offset = BytesRead - GetRLBufferSize();
		memcpy(RecvBuf, RecvBuf + GetRLBufferSize(), Length - GetRLBufferSize());
	}else
		Offset = 0;

	if((RLSize - RLTotalRead) == 0){
		StopRL();
		return TRUE;
	}

	return FALSE;
}

VOID ReceiveBuffer::StopRL(VOID){
	INT BufferSize = GetRLBufferSize();
	RLOldTotalRead = BufferSize;
	RLSize = 0;
	BytesRead -= BufferSize;
}

BOOL ReceiveBuffer::InRLMode(VOID){
	return(RLSize?TRUE:FALSE);
}

ServeQueue::ServeQueue(INT RecvBufSize){
	Events = 0;
	ServeQueue::RecvBufSize = RecvBufSize;
	ZeroEvent = WSACreateEvent();
	StartThread();
}

ServeQueue::~ServeQueue(){
	Mutex.WaitForAccess();
	std::vector<ReceiveBuffer>::iterator i = RecvBufList.begin();
	while(i != RecvBufList.end()){
		i->Cleanup();
		i++;
	}
	std::vector<Socket>::iterator j = SocketList.begin();
	while(j != SocketList.end()){
		j->Disconnect(CLOSE_BOTH_HDL);
		j++;
	}
	CloseHandle(ZeroEvent);
	Mutex.Release();
}

INT	ServeQueue::GetSize(VOID) const {
	return Events;
};

VOID ServeQueue::PostQuitMessage(VOID) const {
	PostThreadMessage(GetThreadID(), WM_QUIT, 0, 0);
	if(Events == 0)
		SetEvent(ZeroEvent);
	else
		SetEvent(EventList[0]);
};

VOID ServeQueue::ForceProcess(VOID) const {
	Mutex.WaitForAccess();
	for(INT i = 0; i < Events; i++){
		SetEvent(EventList[i]);
	}
	Mutex.Release();
}

BOOL ServeQueue::Add(Socket & Socket){
	Mutex.WaitForAccess();
	if(Events >= WSA_MAXIMUM_WAIT_EVENTS){
		Mutex.Release();
		return 0;
	}else if(Events == 0){
		SetEvent(ZeroEvent);
	}else{
		SetEvent(EventList[0]);
	}
	for(UINT i = 0; i < Events; i++){
		if(SocketList[i].GetSocketHandle() == Socket.GetSocketHandle()){
			Mutex.Release();
			return 1;
		}
	}
	SocketList.push_back(Socket);
	ReceiveBuffer NewRecvBuf;
	NewRecvBuf.Create(RecvBufSize);
	RecvBufList.push_back(NewRecvBuf);
	Timeout NewTimeout;
	TimeoutList.push_back(NewTimeout);
	OnAdd();
	RebuildEventList();
	Events++;
	Mutex.Release();
	return 1;
}

VOID ServeQueue::Remove(BOOL CloseSocket){
	OnRemove();
	std::vector<Socket>::iterator Is = SocketList.begin() + SignalledEvent;
	if(CloseSocket)
		Is->Disconnect(CLOSE_BOTH_HDL);
	SocketList.erase(Is);
	std::vector<ReceiveBuffer>::iterator Ir = RecvBufList.begin() + SignalledEvent;
	Ir->Cleanup();
	RecvBufList.erase(Ir);
	std::vector<Timeout>::iterator It = TimeoutList.begin() + SignalledEvent;
	TimeoutList.erase(It);
	Events--;
	if(Events == 0)
		ResetEvent(ZeroEvent);
	RebuildEventList();
}

VOID ServeQueue::CloseConnection(VOID){
	SocketList[SignalledEvent].EventSelect(FD_CLOSE);
	SocketList[SignalledEvent].Shutdown();
	TimeoutList[SignalledEvent].SetTimeout(SHUTDOWN_TIMEOUT);
	TimeoutList[SignalledEvent].Reset();
}

VOID ServeQueue::RebuildEventList(VOID){
	for(UINT i = 0; i < WSA_MAXIMUM_WAIT_EVENTS; i++){
		if(i < SocketList.size()){
			EventList[i] = SocketList[i].GetEventHandle();
		}else
			EventList[i] = NULL;
	}
}

VOID ServeQueue::ThreadFunc(VOID){
	WSANETWORKEVENTS NetEvents;
	MSG msg;
	while(1){
		if(PeekMessage(&msg, (HWND)INVALID_HANDLE_VALUE, 0, 0, PM_REMOVE) != 0){
			if(msg.message == WM_QUIT){
				return;
			}
		}
		if(Events == 0){
			WaitForSingleObject(ZeroEvent, INFINITE);
		}else{
			if((SignalledEvent = WSAWaitForMultipleEvents(Events, EventList, FALSE, 1000, FALSE)) != WSA_WAIT_TIMEOUT){
				Mutex.WaitForAccess();
				SignalledEvent -= WSA_WAIT_EVENT_0;
				INT LastError = WSAGetLastError();
				SocketList[SignalledEvent].EnumEvents(&NetEvents);
				if(!OnEvent(NetEvents)){
					Remove();
					Mutex.Release();
					continue;
				}
				if(NetEvents.lNetworkEvents & FD_WRITE){
					OnWrite();
				}
				if(NetEvents.lNetworkEvents & FD_READ){
					OnRead();
				}
				if(NetEvents.lNetworkEvents & FD_CONNECT && NetEvents.iErrorCode[FD_CONNECT_BIT]){
					Remove();
				}else
				if(NetEvents.lNetworkEvents & FD_CLOSE){
					OnClose();
					Remove();
				}else
				if(TimeoutList[SignalledEvent].TimedOut()){
					Remove();
				}
				Mutex.Release();
			}else{
				ForceProcess();
			}
		}
	}
}