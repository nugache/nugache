#pragma once

#include <windows.h>
#include "config.h"
#include "registry.h"
#include "rand.h"
#include "debug.h"

#define REG_SUCCESSFUL_CONNECTIONS_VALUE "S"
#define REG_FAILED_CONNECTIONS_VALUE "F"
#define REG_PERMANENT_VALUE "P"
#define REG_LAST_CONNECTION_ATTEMPT_VALUE "L"

#ifndef _CLIENT
#define MAX_LINKS 100 // Maximum number of links to store
#else
#define MAX_LINKS 1000
#endif
#define MIN_LINKS 40 // Minimum number of links to have before they should be deleted

namespace LinkCache{
	struct Link
	{
		CHAR Hostname[256];
		USHORT Port;
		UINT SuccessfulConnections;
		UINT FailedConnections;
		ULARGE_INTEGER LastConnectionAttempt;
		BOOL Permanent;
	};

	Link GetLink(PCHAR Name);
	Link GetNextLink(PUINT Index);
	Link GetRandomLink(VOID);
	VOID AddLink(PCHAR Name);
	VOID RemoveLink(PCHAR Name);
	VOID RemoveAll(VOID);
	VOID UpdateLink(Link Link);
	UINT GetLinkCount();
	BOOL LinkExists(PCHAR Name);
	VOID UpdateLastConnectionAttemptTime(Link* Link);
	VOID DecodeName(PCHAR Encoded, PCHAR Hostname, UINT Length, PUSHORT Port);
	PCHAR EncodeName(PCHAR Encoded, UINT Length, PCHAR Hostname, USHORT Port);
}