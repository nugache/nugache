#include <winsock2.h>
#include "dns.h"

DNSResolver::DNSResolver(){
	Socket.Create(SOCK_DGRAM);
	ID = 100;
	Answer = NULL;
	Authority = NULL;
	Additional = NULL;
}

DNSResolver::~DNSResolver(){
	Socket.Disconnect(CLOSE_BOTH_HDL);
	DeleteResourceRecords();
}

PCHAR DNSResolver::EncodeDomainName(PCHAR DomainName){
	if(!strstr(DomainName, ".")){
		Temp[0] = strlen(DomainName) > 0xFF ? 0xFF : strlen(DomainName);
		strncpy(&Temp[1], DomainName, 0xFE);
	}else{
		UINT Position = 0;
		CHAR DomainNameL[256];
		strncpy(DomainNameL, DomainName, sizeof(DomainNameL));
		PCHAR C = strtok(DomainNameL, ".");
		do{
			Temp[Position] = strlen(C);
			Position++;
			strncpy(&Temp[Position], C, 0xFF - Position);
			Position += strlen(C);
			if(Position >= 0xFF){
				Temp[Position - strlen(C)] = 0xFF - (Position - strlen(C) + 1);
				C = NULL;
			}else
				C = strtok(NULL, ".");
		}while(C);
	}
	return Temp;
}

PCHAR DNSResolver::DecodeDomainName(PCHAR Labels, PUINT Size, BOOL Reset){
	if(Reset)
		Temp[0] = NULL;
	UINT Position = 0;
	while(Labels[Position]){
		if((Labels[Position] & 0xC0) == 0xC0){
			USHORT Offset = (BYTE)Labels[Position + 1] | (USHORT)((Labels[Position] & 0x3F) << 8);
			//dprintf("%d  %d/%d\r\n", Offset, Position, *Size);
			UINT Size2;
			DecodeDomainName(Message + Offset, &Size2, FALSE);
			Position += 2;
			*Size = Position;
			return Temp;
		}else{
			strncat(Temp, &Labels[Position + 1], Labels[Position]);
			strncat(Temp, ".", 1);
			Position += (1 + Labels[Position]);
		}
	}
	if(strlen(Temp)){
		Temp[strlen(Temp) - 1] = NULL;
		Position++;
	}
	*Size = Position;
	return Temp;
}

BOOL DNSResolver::ParseResponse(INT Length){
	DNSHeader* DNSHeaderP = (DNSHeader*)Message;
	DNSHeaderP->ANCOUNT = ntohs(DNSHeaderP->ANCOUNT);
	DNSHeaderP->ARCOUNT = ntohs(DNSHeaderP->ARCOUNT);
	DNSHeaderP->FLAGS.ALL = ntohs(DNSHeaderP->FLAGS.ALL);
	DNSHeaderP->NSCOUNT = ntohs(DNSHeaderP->NSCOUNT);
	DNSHeaderP->QDCOUNT = ntohs(DNSHeaderP->QDCOUNT);
	if(DNSHeaderP->FLAGS.TC || DNSHeaderP->FLAGS.QR == 0 || DNSHeaderP->FLAGS.RCODE || DNSHeaderP->QDCOUNT != 1 || DNSHeaderP->ID != ID)
		return FALSE;
	UINT Size;
	UINT Position = sizeof(DNSHeader);
	PCHAR DomainName = DecodeDomainName(Message + Position, &Size);
	Position += Size + sizeof(USHORT) + sizeof(USHORT);

	for(UINT i = 0; i < DNSHeaderP->ANCOUNT; i++){
		Position += AddResourceRecord(RTypeAnswer, Message + Position);
	}
	for(UINT i = 0; i < DNSHeaderP->NSCOUNT; i++){
		Position += AddResourceRecord(RTypeAuthority, Message + Position);
	}
	for(UINT i = 0; i < DNSHeaderP->ARCOUNT; i++){
		Position += AddResourceRecord(RTypeAdditional, Message + Position);
	}

	return TRUE;
}

typedef DWORD (__stdcall *GetNetworkParamsP)(PFIXED_INFO, PULONG);

USHORT DNSResolver::Query(PCHAR Name, USHORT Type){
	//dprintf("Querying %s\r\n", Name);
	DeleteResourceRecords();
	HMODULE Dll = LoadLibrary("iphlpapi");
	GetNetworkParamsP GetNetworkParamsF = (GetNetworkParamsP)GetProcAddress(Dll, "GetNetworkParams");
	if(!GetNetworkParamsF)
		return NULL;
	FIXED_INFO FixedInfo;
	ULONG Size = sizeof(FixedInfo);
	DWORD Return = GetNetworkParamsF(&FixedInfo, &Size);
	if(Return != ERROR_SUCCESS)
		return NULL;
	sockaddr_in SockAddr;
	SockAddr.sin_family = AF_INET;
	SockAddr.sin_port = htons(53);

	SockAddr.sin_addr.S_un.S_addr = inet_addr(FixedInfo.DnsServerList.IpAddress.String);
	USHORT Class = htons(DNS_CLASS_IN);
	Type = htons(Type);

	ID++;
	if(!ID) ID = 100;
	
	PCHAR NameEncoded = EncodeDomainName(Name);
	CHAR Buf[0x200];
	DNSHeader DNSHeader;
	
	DNSHeader.ID = ID;
	DNSHeader.FLAGS.ALL = htons(0x0100);
	DNSHeader.QDCOUNT = htons(1);
	DNSHeader.ANCOUNT = 0;
	DNSHeader.NSCOUNT = 0;
	DNSHeader.ARCOUNT = 0;
	

	memcpy(Buf, &DNSHeader, sizeof(DNSHeader));
	
	memcpy(Buf + sizeof(DNSHeader), NameEncoded, strlen(NameEncoded) + 1);
	memcpy(Buf + sizeof(DNSHeader) + strlen(NameEncoded) + 1, &Type, sizeof(Type));
	memcpy(Buf + sizeof(DNSHeader) + strlen(NameEncoded) + 1 + sizeof(Type), &Class, sizeof(Class));
	Socket.SendTo((PCHAR)Buf, sizeof(DNSHeader) + strlen(NameEncoded) + 1 + sizeof(Type) + sizeof(Class), (sockaddr *)&SockAddr, sizeof(SockAddr));
	USHORT Port = Socket.GetPort();
	
	Socket.Disconnect(CLOSE_BOTH_HDL);
	Socket.Create(SOCK_DGRAM);
	Socket.Bind(Port);

	INT AddrSize = sizeof(SockAddr);
	INT BytesRead = Socket.RecvFrom(Message, sizeof(Message), (sockaddr *)&SockAddr, &AddrSize);
	if(BytesRead == SOCKET_ERROR)
		return NULL;
	MessageSize = BytesRead;
		
	ParseResponse(MessageSize);
	Socket.Disconnect(CLOSE_BOTH_HDL);
	Socket.Create(SOCK_DGRAM);
	return ID;
}

VOID DNSResolver::DeleteResourceRecords(VOID){
	ResourceRecord* TempRecord;
	TempRecord = Answer;
	while(TempRecord){
		ResourceRecord* OldRecord = TempRecord;
		TempRecord = TempRecord->Next;
		delete OldRecord;
	}
	TempRecord = Authority;
	while(TempRecord){
		ResourceRecord* OldRecord = TempRecord;
		TempRecord = TempRecord->Next;
		delete OldRecord;
	}
	TempRecord = Additional;
	while(TempRecord){
		ResourceRecord* OldRecord = TempRecord;
		TempRecord = TempRecord->Next;
		delete OldRecord;
	}
}

UINT DNSResolver::AddResourceRecord(DWORD Type, PCHAR Start){
	ResourceRecord** TempRecord;
	switch(Type){
		case RTypeAnswer: TempRecord = &Answer; break;
		case RTypeAuthority: TempRecord = &Authority; break;
		case RTypeAdditional: TempRecord = &Additional; break;
	}
	while(*TempRecord) TempRecord = &(*TempRecord)->Next;
	*TempRecord = new ResourceRecord;
	UINT Size;
	strncpy((*TempRecord)->Name, DecodeDomainName(Start, &Size), sizeof((*TempRecord)->Name));
	(*TempRecord)->Type = ntohs(*(USHORT*)(Start + Size));
	(*TempRecord)->Class = ntohs(*(USHORT*)(Start + Size + sizeof(USHORT)));
	(*TempRecord)->TTL = ntohs(*(ULONG*)(Start + Size + sizeof(USHORT) + sizeof(USHORT)));
	(*TempRecord)->Length = ntohs(*(USHORT*)(Start + Size + sizeof(USHORT) + sizeof(USHORT) + sizeof(ULONG)));
	memcpy((*TempRecord)->Data, Start + Size + sizeof(USHORT) + sizeof(USHORT) + sizeof(ULONG) + sizeof(USHORT), (*TempRecord)->Length);
	(*TempRecord)->Next = NULL;
	return Size + sizeof(USHORT) + sizeof(USHORT) + sizeof(ULONG) + sizeof(USHORT) + (*TempRecord)->Length;
}