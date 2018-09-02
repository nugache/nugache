#include "linkcache.h"

namespace LinkCache
{
	Link GetLink(PCHAR Name){
		Link Link;
		CHAR Hostname[256];
		USHORT Port = 0;
		DecodeName(Name, Hostname, sizeof(Hostname), &Port);
		if(strcmp(Hostname, "null") == 0){
			strcpy(Link.Hostname, Hostname);
			Link.Port = Port;
			return Link;
		}
		memset(&Link, NULL, sizeof(Link));
		CHAR KeyName[256];
		strcpy(KeyName, REG_LINKS);
		strcat(KeyName, "\\");
		strncat(KeyName, Name, sizeof(KeyName) - 1 - strlen(KeyName));
		DWORD Disposition;
		cRegistry Reg;
		if(Reg.OpenKey(HKEY_CURRENT_USER, KeyName) == ERROR_SUCCESS){
			strcpy(Link.Hostname, Hostname);
			Link.Port = Port;
			if(Reg.Exists(REG_SUCCESSFUL_CONNECTIONS_VALUE))
				Link.SuccessfulConnections = Reg.GetInt(REG_SUCCESSFUL_CONNECTIONS_VALUE);
			else
				Link.SuccessfulConnections = 0;
			if(Reg.Exists(REG_FAILED_CONNECTIONS_VALUE))
				Link.FailedConnections = Reg.GetInt(REG_FAILED_CONNECTIONS_VALUE);
			else
				Link.FailedConnections = 0;
			if(Reg.Exists(REG_PERMANENT_VALUE))
				Link.Permanent = Reg.GetInt(REG_PERMANENT_VALUE);
			else
				Link.Permanent = FALSE;
			if(Reg.Exists(REG_LAST_CONNECTION_ATTEMPT_VALUE)){
				DWORD Size = sizeof(Link.LastConnectionAttempt);
				Reg.GetBinary(REG_LAST_CONNECTION_ATTEMPT_VALUE, (PCHAR)&Link.LastConnectionAttempt, &Size); 
			}else
				memset(&Link.LastConnectionAttempt, 0, sizeof(Link.LastConnectionAttempt));
		}
		return Link;
	}

	Link GetNextLink(PUINT Index){
		CHAR Hostname[256];
		cRegistry Reg(HKEY_CURRENT_USER, REG_LINKS);
		LONG Return;
		do{
			DWORD Size = sizeof(Hostname);
			Return = Reg.EnumKey(*Index, Hostname, &Size);
			if((Return == ERROR_NO_MORE_ITEMS && *Index == 0) || Return == ERROR_MORE_DATA){
				if(Return == ERROR_MORE_DATA)
					*Index += 1;
				strcpy(Hostname, "null");
				break;
			}
			if(Return == ERROR_NO_MORE_ITEMS){
				*Index = 0;
			}else{
				*Index += 1;
			}
		}while(Return != ERROR_SUCCESS);
		return GetLink(Hostname);
	}

	Link GetRandomLink(VOID){
		INT Max = GetLinkCount() - 1;
		if(Max < 0) Max = 0;
		UINT Index = rand_r(0, Max);
		return GetNextLink(&Index);
	}

	VOID AddLink(PCHAR Name){
		if(LinkExists(Name))
			return;
		UINT AntiLock = 0;
		CHAR Hostname[256];
		USHORT Port;
		DecodeName(Name, Hostname, sizeof(Hostname), &Port);
		while(GetLinkCount() >= MAX_LINKS){
			// remove links with the same port because they are probably dynamic ips
			UINT i = 0;
			for(UINT j = 0; j < GetLinkCount(); j++){
				Link TempLink = GetNextLink(&i);
				if(TempLink.Port == Port){
					CHAR Encoded[256];
					EncodeName(Encoded, sizeof(Encoded), TempLink.Hostname, TempLink.Port);
					RemoveLink(Encoded);
					if(j > 0)
						j--;
				}
			}
			if(GetLinkCount() >= MAX_LINKS){
				Link Link = GetRandomLink();
				for(UINT i = 0; i < 100 && Link.Permanent == TRUE; i++)
					Link = GetRandomLink();
				CHAR Encoded[256];
				EncodeName(Encoded, sizeof(Encoded), Link.Hostname, Link.Port);
				RemoveLink(Encoded);
			}
			AntiLock++;
			if(AntiLock > 1000)
				break;
		}
		CHAR Encoded[256];
		EncodeName(Encoded, sizeof(Encoded), Hostname, Port);
		CHAR KeyName[256];
		strcpy(KeyName, REG_LINKS);
		strcat(KeyName, "\\");
		strncat(KeyName, Encoded, sizeof(KeyName) - 1 - strlen(KeyName));
		DWORD Disposition;
		cRegistry Reg;
		Reg.CreateKey(HKEY_CURRENT_USER, KeyName, &Disposition);
		Reg.CloseKey();
		if(Disposition == REG_CREATED_NEW_KEY){
			Link Link;
			Link.FailedConnections = 0;
			Link.SuccessfulConnections = 0;
			Link.Permanent = FALSE;
			memset(&Link.LastConnectionAttempt, 0, sizeof(Link.LastConnectionAttempt));
			
			Reg.OpenKey(HKEY_CURRENT_USER, KeyName);
			Reg.SetInt(REG_SUCCESSFUL_CONNECTIONS_VALUE, Link.SuccessfulConnections);
			Reg.SetInt(REG_FAILED_CONNECTIONS_VALUE, Link.FailedConnections);
			Reg.SetInt(REG_PERMANENT_VALUE, Link.Permanent);
			Reg.SetBinary(REG_LAST_CONNECTION_ATTEMPT_VALUE, (PCHAR)&Link.LastConnectionAttempt, sizeof(Link.LastConnectionAttempt));
		}
	}

	VOID RemoveLink(PCHAR Name){
		CHAR KeyName[256];
		strcpy(KeyName, REG_LINKS);
		strcat(KeyName, "\\");
		strncat(KeyName, Name, sizeof(KeyName) - 1 - strlen(KeyName));
		DWORD Disposition;
		RegDeleteKey(HKEY_CURRENT_USER, KeyName);
	}

	VOID RemoveAll(VOID){
		cRegistry Reg(HKEY_CURRENT_USER, REG_LINKS);
		CHAR Key[256];
		CHAR KeyName[512];
		do{
			DWORD Size = sizeof(Key);
			Reg.EnumKey(0, Key, &Size);
			strcpy(KeyName, REG_LINKS);
			strcat(KeyName, "\\");
			strcat(KeyName, Key);
		}while(RegDeleteKey(HKEY_CURRENT_USER, KeyName) == ERROR_SUCCESS);
	}

	VOID UpdateLink(Link Link){
		CHAR KeyName[256];
		CHAR Name[256];
		LinkCache::EncodeName(Name, sizeof(Name), Link.Hostname, Link.Port);
		strcpy(KeyName, REG_LINKS);
		strcat(KeyName, "\\");
		strncat(KeyName, Name, sizeof(KeyName) - 1 - strlen(KeyName));
		DWORD Disposition;
		cRegistry Reg;
		if(Reg.OpenKey(HKEY_CURRENT_USER, KeyName) == ERROR_SUCCESS){
			Reg.SetInt(REG_SUCCESSFUL_CONNECTIONS_VALUE, Link.SuccessfulConnections);
			Reg.SetInt(REG_FAILED_CONNECTIONS_VALUE, Link.FailedConnections);
			Reg.SetInt(REG_PERMANENT_VALUE, Link.Permanent);
			Reg.SetBinary(REG_LAST_CONNECTION_ATTEMPT_VALUE, (PCHAR)&Link.LastConnectionAttempt, sizeof(Link.LastConnectionAttempt));
		}
	}

	UINT GetLinkCount(VOID){
		UINT Index = 0;
		cRegistry Reg(HKEY_CURRENT_USER, REG_LINKS);
		CHAR Temp[1];
		DWORD Size = 0;
		LONG Return;
		while(Reg.EnumKey(Index, Temp, &Size) != ERROR_NO_MORE_ITEMS){
			Index++;
		}
		return Index;
	}

	BOOL LinkExists(PCHAR Name){
		CHAR KeyName[256];
		strcpy(KeyName, REG_LINKS);
		strcat(KeyName, "\\");
		strncat(KeyName, Name, sizeof(KeyName) - 1 - strlen(KeyName));
		cRegistry Reg;
		if(Reg.OpenKey(HKEY_CURRENT_USER, KeyName) == ERROR_SUCCESS)
			return TRUE;
		return FALSE;
	}

	VOID UpdateLastConnectionAttemptTime(Link* Link){
		FILETIME FileTime;
		GetSystemTimeAsFileTime(&FileTime);
		Link->LastConnectionAttempt.LowPart = FileTime.dwLowDateTime;
		Link->LastConnectionAttempt.HighPart = FileTime.dwHighDateTime;
	}

	VOID DecodeName(PCHAR Encoded, PCHAR Hostname, UINT Length, PUSHORT Port){
		CHAR EncodedCopy[256 + 8];
		*Port = 8;
		strncpy(EncodedCopy, Encoded, sizeof(EncodedCopy) - 1);
		PCHAR Host = EncodedCopy;
		INT Position = strchr(EncodedCopy, ':') - EncodedCopy;
		if(Position > 0){
			if(Position < strlen(EncodedCopy))
				*Port = atoi(&EncodedCopy[Position + 1]);
			EncodedCopy[Position] = NULL;
		}
		if(Host)
			strncpy(Hostname, Host, Length - 1);
	}

	PCHAR EncodeName(PCHAR Encoded, UINT Length, PCHAR Hostname, USHORT Port){
		CHAR PortA[8];
		memset((PVOID)Encoded, NULL, Length);
		strncpy(Encoded, Hostname, Length - 1);
		strncat(Encoded, ":", Length - 1 - strlen(Encoded));
		strncat(Encoded, itoa(Port, PortA, 10), Length - 1 - strlen(Encoded));
		return Encoded;
	}
}