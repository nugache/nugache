#pragma once

#include <vector>
#include "thread.h"
#include "mutex.h"
#include "timeout.h"

#define CRYPT_ENABLED

#ifdef CRYPT_ENABLED
#include "crypt.h"
#endif

#define CLOSE_NO_HDL 0
#define CLOSE_SOCKET_HDL 1
#define CLOSE_EVENT_HDL 2
#define CLOSE_BOTH_HDL 3

#define BLOCKING 0
#define NONBLOCKING 1

#define WM_USER_GETHOSTBYNAME WM_USER + 1

#define SHUTDOWN_TIMEOUT 5000

PBYTE WordToArray(WORD Word, PBYTE Array);
WORD ArrayToWord(PBYTE Array);
INT TokenizeStr(PCHAR Buffer, PCHAR Item[], UINT Size, PCHAR Delimiter);

namespace SocketFunction
{
	ULONG GetAddr( PCHAR Host );
	in_addr Stoin(ULONG Addr);
	USHORT Checksum(PUSHORT Buffer, UINT Size);
}

struct IPHeader {
	UCHAR HeaderLength:4, Version:4;
	UCHAR TOS;
	USHORT Length;
	USHORT ID;
	USHORT Offset;
	UCHAR TTL;
	UCHAR Protocol;
	USHORT Checksum;
	UINT Src;
	UINT Dst;
};

struct ICMPHeader {
	UCHAR Type;
	UCHAR Code;
	USHORT Checksum;
	USHORT ID;
	USHORT Sequence;
};

struct UDPHeader {
	USHORT SrcPort;
	USHORT DstPort;
	USHORT Length;
	USHORT Checksum;
};

struct TCPHeader {
	USHORT SrcPort;
	USHORT DstPort;
	UINT Sequence;
	UINT ACK;
	UCHAR X2:4, Offset:4;
	UCHAR Flags;
	USHORT Window;
	USHORT Checksum;
	USHORT UrgentP;
};

class Socket
{
public:
	Socket();
	INT			Accept( Socket & Socket );
	INT			AsyncSelect( HWND hWnd, UINT Msg, LONG Events );
	INT			Bind( USHORT Port );
	INT			BlockingMode( BOOL Mode );
	INT			Connect( const PCHAR Host, USHORT Port );
	INT			Create( INT Type );
	INT			Create( INT Family, INT Type, INT Protocol );
	WSAEVENT	CreateEvent( VOID );
	VOID		Disconnect( INT Close );
	INT			EnumEvents( LPWSANETWORKEVENTS NetEvents ) const;
	INT			EventSelect( LONG Events );
	PCHAR		GetConnectHost( VOID ) const;
	USHORT		GetConnectPort( VOID ) const;
	WSAEVENT	GetEventHandle( VOID ) const;
	ULONG		GetPeerAddr( VOID );
	INT			GetPeerName( sockaddr* SockAddr, PINT Size );
	VOID		GetPeerName( PCHAR Name, UINT Size );
	USHORT		GetPeerPort( VOID );
	ULONG		GetAddr( VOID );
	INT			GetName( sockaddr* SockAddr, PINT Size );
	USHORT		GetPort( VOID );
	LONG		GetSelectedEvents( VOID ) const;
	SOCKET		GetSocketHandle( VOID ) const;
	INT			GetProtocol( VOID ) const;
	INT			Listen( INT Backlog );
	INT			Recv( PCHAR Buffer, INT Length );
	INT			RecvFrom( PCHAR Buffer, INT Length, sockaddr* SockAddr, PINT Size );
	BOOL		ResetEvent( VOID ) const;
	INT			Send( const PCHAR Buffer, INT Length );
	INT			Sendf( const PCHAR Format, ... );
	INT			SendTo( const PCHAR Buffer, INT Length, sockaddr* SockAddr, INT Size );
	INT			Shutdown( VOID );
	INT			Shutdown( INT How );
	VOID		FillAddrStruct( sockaddr_in *SockAddr, PCHAR Host, USHORT Port = 0 );
#ifdef CRYPT_ENABLED
	VOID		Crypt(::CFB CFB);
	VOID		StopCrypt(VOID);
	BOOL		IsCrypted(VOID);
#endif

private:
#ifdef CRYPT_ENABLED
	CFB			CFBS, CFBR;
#endif
	SOCKET		hSocket;
	INT			Family, Type, Protocol;
	USHORT		Port;
	WSAEVENT	hEvent;
	LONG		Events;
	CHAR		ConnectHost[256];
};

class AsyncDNS : public Thread
{
public:
	AsyncDNS(DWORD ThreadID, PCHAR Hostname, ULONG* Addr);
	~AsyncDNS();

private:
	VOID ThreadFunc(VOID);

	DWORD ThreadID;
	PCHAR Hostname;
	ULONG* Addr;
};

class ReceiveBuffer
{
public:
	ReceiveBuffer();
	VOID		Create( INT Length );
	VOID		Cleanup( VOID );
	INT			GetBytesRead( VOID ) const;
	BOOL		PopItem( const PCHAR Delimiter, INT DSize );
	INT			Read( Socket & Socket );
	VOID		Reset( VOID );
	VOID		PopBuffer( VOID );
	VOID		StartRL( UINT Bytes );
	INT			GetRLBufferSize( VOID );
	BOOL		PopRLBuffer( VOID );
	VOID		StopRL( VOID );
	BOOL		InRLMode( VOID );

	PCHAR		PoppedItem;
	
private:
	INT			BytesRead;
	INT			Length;
	INT			Offset;
	BOOL		OverFlow;
	PCHAR		RecvBuf;
	INT			RLSize;
	INT			RLTotalRead;
	INT			RLOldTotalRead;
};

template <size_t s>
struct RLBuffer {
	VOID Read(PCHAR Buffer, UINT Size){
		for(UINT i = BytesRead; i < BytesRead + Size; i++)
			Data[i] = Buffer[i - BytesRead];
		BytesRead += Size;
	};
	UINT BytesRead;
	CHAR Data[s];
};

//#define RLBuffer(x) struct { VOID Read(PCHAR Buffer, UINT Size){ for(UINT i = BytesRead; i < BytesRead + Size; i++) Data[i] = Buffer[i - BytesRead]; BytesRead += Size; }; UINT BytesRead; CHAR Data[x]; }

class ServeQueue : public Thread
{
public:
	ServeQueue( INT RecvBufSize );
	virtual ~ServeQueue();
	BOOL	Add( Socket & Socket );
	VOID	Remove( BOOL CloseSocket = TRUE );
	INT		GetSize( VOID ) const;
	VOID	PostQuitMessage( VOID ) const;
	VOID	ForceProcess( VOID ) const;

	Mutex	Mutex;

protected:
	VOID			CloseConnection( VOID );
	virtual BOOL	OnEvent( WSANETWORKEVENTS NetEvents ) { return TRUE; };
	virtual VOID	OnWrite( VOID ) {};
	virtual VOID	OnRead( VOID ) {};
	virtual VOID	OnClose( VOID ) {};
	virtual VOID	OnAdd( VOID ) {};
	virtual VOID	OnRemove( VOID ) {};

	std::vector<Socket>	SocketList;
	std::vector<ReceiveBuffer> RecvBufList;
	INT				SignalledEvent;

private:
	VOID			ThreadFunc( VOID );
	VOID			RebuildEventList( VOID );

	std::vector<Timeout> TimeoutList;
	INT				Events;
	WSAEVENT		EventList[WSA_MAXIMUM_WAIT_EVENTS];
	WSAEVENT		ZeroEvent;
	INT				RecvBufSize;
};

template <class SQ>
class ServeQueueList
{
public:
	ServeQueueList<SQ>() { SQ *NewSQ = new SQ; SQVector.push_back(NewSQ); };
	SQ*		Add( Socket & Socket );
	INT		GetSize( VOID ) const;
	SQ*		GetItem( UINT Index ) const;
	VOID	ForceProcess( VOID );

	Mutex	Mutex;

private:
	std::vector<SQ*>	SQVector;
};

template <class SQ>
SQ* ServeQueueList<SQ>::Add(Socket & Socket){
	Mutex.WaitForAccess();
	std::vector<SQ*>::iterator i = SQVector.begin();
	while(i != SQVector.end() && SQVector.size() > 1){
		if((*i)->GetSize() <= 0){
			(*i)->PostQuitMessage();
			SQVector.erase(i);
			i = SQVector.begin();
		}else
		i++;
	}
	i = SQVector.begin();
	while(i != SQVector.end()){
		if((*i)->Add(Socket)){
			Mutex.Release();
			return (*i);
		}else if(++i == SQVector.end()){
			SQ* Temp = new SQ;
			SQVector.push_back(Temp);
			SQVector.back()->Add(Socket);
			Mutex.Release();
			return SQVector.back();
		}
	}
}

template <class SQ>
INT ServeQueueList<SQ>::GetSize(VOID) const {
	return SQVector.size();
}

template <class SQ>
SQ* ServeQueueList<SQ>::GetItem(UINT Index) const {
	return SQVector[Index];
}

template <class SQ>
VOID ServeQueueList<SQ>::ForceProcess(VOID){
	std::vector<SQ*>::iterator i = SQVector.begin();
	while(i != SQVector.end()){
		(*i)->ForceProcess();
		i++;
	}
}