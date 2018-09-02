#pragma once

#include <windows.h>

class cRegistry
{
public:
	cRegistry();
	cRegistry(HKEY Key, PCHAR SubKey);
	~cRegistry();
	BOOL	Exists(PCHAR Value);
	LONG	DeleteValue(PCHAR Value);
	LONG	GetBinary(PCHAR Value, PCHAR Buffer, PDWORD Size);
	LONG	SetBinary(PCHAR Value, PCHAR Buffer, DWORD Size);
	LONG	SetString(PCHAR Value, PCHAR String);
	PCHAR	GetString(PCHAR Value);
	DWORD	GetSize(PCHAR Value);
	DWORD	GetType(PCHAR Valie);
	LONG	SetInt(PCHAR Value, DWORD Int);
	DWORD	GetInt(PCHAR Value);
	INT		GetMultiStringElements(PCHAR Value);
	PCHAR	GetMultiString(PCHAR Value, UINT Index);
	INT		MultiStringExists(PCHAR Value, PCHAR String);
	LONG	InsertMultiString(PCHAR Value, PCHAR String);
	LONG	RemoveMultiString(PCHAR Value, UINT Index);
	LONG	CreateKey(HKEY Key, PCHAR SubKey, REGSAM Access, LPDWORD Disposition);
	LONG	CreateKey(HKEY Key, PCHAR SubKey, LPDWORD Disposition);
	LONG	CreateKey(HKEY Key, PCHAR SubKey);
	LONG	OpenKey(HKEY Key, PCHAR SubKey, REGSAM Access);
	LONG	OpenKey(HKEY Key, PCHAR SubKey);
	LONG	CloseKey(VOID);
	LONG	EnumKey(UINT Index, PCHAR SubKey, LPDWORD Size);

private:
	HKEY	hKey;
	PCHAR	C_Str;
	PCHAR	C_MultiStr;
};