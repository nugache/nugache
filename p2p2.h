#pragma once

#include <windows.h>
#include <vector>
#include <algorithm>
#include "thread.h"
#include "mutex.h"
#include "socketwrapper.h"
#include "rand.h"
#include "wildcard.h"
#include "timeout.h"
#include "hostexempt.h"
#include "linkcache.h"
#include "firewall.h"
#include "sel.h"
#include "update.h"
extern "C" {
	#include "rsa/rsa.h"
	#include "rsa/rsaref.h"
}

extern R_RSA_PRIVATE_KEY RSAPrivateKey;
extern R_RSA_PUBLIC_KEY RSAPublicKey;
extern R_RSA_PUBLIC_KEY RSAPublicKeyMaster;

#define RECVBUF_SIZE 2048 // Delimited messages cannot exceed this length
//#define LISTENING_PORT 8
#define NEGOTIATION_TIMEOUT 15000 // Timeout of in-process negotiation
//#define VERBOSE_NEGOTIATION // Display extra informatin about negotiations if defined
#define PING_TIMEOUT 160000 // Timeout of a ready connection
#define PING_INTERVAL PING_TIMEOUT / 2 // Send ping after this amount of time
#define NETWORK_ID "Test4" // Negotiations won't connect if their network id doesn't match
#define RSA_MODULUS_BITS 512 // RSA key length for negotiations
#define MAX_CONTROL_CONNECTIONS 100
#define MAX_CLIENT_CONNECTIONS 50
#define MAX_CLIENT_CONNECTIONS_BEFORE_DISCONNECT 30 // Start disconnecting connected clients after sending link list so they can reconnect to another link
#define MAX_LINK_CONNECTIONS 15
#define MIN_LINK_CONNECTIONS 5
#define MAX_LINK_CONNECTIONS_BEFORE_DISCONNECT 10 // Same as MAX_CLIENT_CONNECTIONS_BEFORE_DISCONNECT but with links
#define MAX_NEWLINK_TTL 2
#define MAX_MESSAGE_TTL 30
#define MAX_MIDS_STORE 200 // How many message IDs to remember
#define MODE_DECISION_RATIO 0.65f // Ratio of successful/unsuccessful connectbacks to stay in link mode
#define OUTBOUND_CONNECTION_INTERVAL 20000 // Attempt an outbound connection when this amount of time passes

// Ordinals
#define RPL_CONNECTION_OKAY 1

#define CMD_CONNECTBACK 1
#define RPL_CONNECTBACK 2
#define CMD_TRANSFORM 3
#define RPL_TRANSFORM 4
#define CMD_GETINFO 5
#define RPL_GETINFO 6
//#define CMD_SENDMSG 7
#define CMD_LISTLINKS 8
#define RPL_LINKITEM 9
#define MSG_MESSAGE 10
#define CMD_PING 11
#define RPL_PING 12
#define CMD_GETVERSION 13
#define RPL_GETVERSION 14
#define CMD_REQUEST_UPGRADE 15
#define RPL_UPGRADE_UNAVAILABLE 16
#define RPL_UPGRADE_INFORMATION 17

#define CLIENT 0
#define HOST 1
#define MODE_CLIENT 0
#define MODE_LINK 1

class P2P2 : public Thread
{
public:
	P2P2(BOOL Run = TRUE);
	~P2P2();
	VOID GenerateKeys(VOID);
	UINT GetClientCount(VOID);
	UINT GetControlCount(VOID);
	UINT GetLinkCount(VOID);
	BOOL GetMode(VOID);
	ULONG GetLinkedIP(VOID);

	class Negotiation
	{
	public:
		enum ErrorCodes{ERR_CONNECTIONCLOSED, ERR_CONTIMEDOUT, ERR_UNREACHABLE, ERR_CRYPT, ERR_CONNECTION_CLASS_FULL, ERR_INCORRECT, ERR_TYPE, ERR_NETWORK, ERR_KEYLENGTH, ERR_TIMEOUT, ERR_CBFIRST};
		enum ConnectionTypes{TYPE_UNKNOWN, TYPE_CONTROL, TYPE_CLIENT, TYPE_LINK};

	public:
		Negotiation(UCHAR ConnectionType);
		~Negotiation(); // Making this virtual causes errors on delete when connectbackthread is active
		VOID Connect(PCHAR Host, USHORT Port, UCHAR ConnectionType);
		VOID Disconnect(VOID);
		VOID Attach(::Socket & Socket, ReceiveBuffer & RecvBuf, BOOL Position);
		BOOL Ready(VOID);
		UCHAR GetConnectionType(VOID);
		BOOL GetPosition(VOID);
		BOOL Connected(VOID);
		Socket* GetSocketP(VOID) { return &Socket; };
		BOOL Process(WSANETWORKEVENTS NetEvents);
		VOID Close(UINT ErrorCode);
		virtual VOID OnFail(UINT ErrorCode) = 0;
		virtual VOID OnReady(VOID) = 0;
		virtual VOID ProcessSpecial(WSANETWORKEVENTS NetEvents) = 0;

	protected:
		BOOL TransformConnection(UCHAR ConnectionType);
		ReceiveBuffer RecvBuf;
		Socket Socket;

	private:
		UINT ErrorCode;
		enum State{CONNECT, READ_PUBLIC_KEY_LENGTH, READ_PUBLIC_KEY, READ_SYMMETRICAL_KEY, GET_CONNECTION_TYPE, GET_STATUS, READY} State;
		struct RSABlock : public RLBuffer<MAX_RSA_MODULUS_LEN> {
			RLBuffer<sizeof(WORD)> Length;
		} RSABlock;
		UCHAR ConnectionType;
		BOOL Position;
		BOOL Attached;
		BOOL bConnected;
		BOOL SentShutdown;
		Timeout Timeout;
	};

	template <class N>
	class NegotiationQueue : public ServeQueue
	{
	public:
		NegotiationQueue() : ServeQueue(RECVBUF_SIZE) {};
		N* GetLastAdded(VOID);
		N* GetItem(UINT Index);

	private:
		BOOL OnEvent(WSANETWORKEVENTS NetEvents);
		VOID OnAdd(VOID);
		VOID OnRemove(VOID);

		std::vector<N*> Negotiation;
	};

private:
	VOID ThreadFunc(VOID);

	class Connection; // Forward

	class ConnectBackThread : public Thread
	{
	public:
		ConnectBackThread(Connection* Connection, USHORT Port);
		VOID ConnectionClosed(VOID);

	private:
		VOID ThreadFunc(VOID);

		Connection* Connection;
		CHAR RemoteHost[256];
		USHORT RemotePort;
	};

	class Connection : public Negotiation
	{
	public:
		Connection();
		VOID ConnectBackDone(BOOL Success);
		VOID SetLinkCacheLink(LinkCache::Link Link);
		USHORT GetPort(VOID);
		Mutex Mutex;

	private:
		VOID ProcessSpecial(WSANETWORKEVENTS NetEvents);
		VOID OnReady(VOID);
		VOID OnFail(UINT ErrorCode);
		VOID OnClientConnected(VOID);
		VOID PingReplied(VOID);
		enum State{SEND_CONNECTBACK, WAIT_FOR_CONNECTBACK, TRANSFORM, GET_LINK_LIST, REQUESTING_UPGRADE, UPGRADE} State;
		struct Update {
			RLBuffer<RSAPublicKeyMasterLen> Signature;
			PCHAR Buffer;
			DWORD Size;
			DWORD Read;
			MD5 MD5;
		} Update;
		ConnectBackThread* ConnectBackThread;
		UCHAR TransformType;
		LinkCache::Link Link;
		USHORT Port;
		::Timeout Timeout;
		BOOL Pinging;
		struct CurrentMsg {
			enum State{INACTIVE, READ_SIGNATURE, READ_BODY} State;
			PCHAR Body;
			UINT BytesRead;
			RLBuffer<RSAPublicKeyMasterLen> Signature;
			DWORD MID;
			UINT Length;
			CHAR SendTo[32];
			UINT TTL;
			BOOL NewLinkMessage;
			BOOL RunLocally;
		} CurrentMsg;
	};

	VOID Initialize(VOID);
	VOID ExecuteMessage(PCHAR Message);
	//VOID BroadcastMessage(Connection* Exempt, PCHAR SendTo, UINT TTL, PCHAR Message, PCHAR Signature, DWORD MID);
	VOID BroadcastMessage(Connection* Exempt, PCHAR SendTo, UINT TTL, PCHAR Message, PCHAR Signature, DWORD MID, BOOL NewLinkMessage);
	Socket ListeningSocket;
	ServeQueueList<NegotiationQueue<Connection> > ConnectionList;
};

extern P2P2* P2P2Instance;