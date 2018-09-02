#pragma once

#include <windows.h>
#include <Iphlpapi.h>
#include "socketwrapper.h"
#include "debug.h"

#define DNS_TYPE_A 1
#define DNS_TYPE_MX 15

#define DNS_CLASS_IN 1

struct ResourceRecord {
	ResourceRecord* Next;
	CHAR Name[256];
	USHORT Type;
	USHORT Class;
	ULONG TTL;
	USHORT Length;
	CHAR Data[256];
};

class DNSResolver
{
public:
	DNSResolver();
	~DNSResolver();
	USHORT Query(PCHAR Name, USHORT Type);
	PCHAR DecodeDomainName(PCHAR Labels, PUINT Size, BOOL Reset = TRUE);

	ResourceRecord* Answer;
	ResourceRecord* Authority;
	ResourceRecord* Additional;

private:
	PCHAR EncodeDomainName(PCHAR DomainName);
	BOOL ParseResponse(INT Length);
	VOID DeleteResourceRecords(VOID);
	UINT AddResourceRecord(DWORD Type, PCHAR Start);

	static const enum {RTypeAnswer, RTypeAuthority, RTypeAdditional};
	Socket Socket;
	CHAR Temp[0x100];
	USHORT ID;
	CHAR Message[0x200];
	UINT MessageSize;
	struct DNSHeader {
		USHORT ID;
		union {
			USHORT ALL;
			struct {
				USHORT RCODE:4;
				USHORT Z:3;
				USHORT RA:1;
				USHORT RD:1;
				USHORT TC:1;
				USHORT AA:1;
				USHORT OPCODE:4;
				USHORT QR:1;
			};
		} FLAGS;
		USHORT QDCOUNT;
		USHORT ANCOUNT;
		USHORT NSCOUNT;
		USHORT ARCOUNT;
	};
};