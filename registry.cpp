#include "registry.h"
#include "debug.h"

cRegistry::cRegistry(){
	C_Str = NULL;
	C_MultiStr = NULL;
	hKey = NULL;
}

cRegistry::cRegistry(HKEY Key, PCHAR SubKey){
	C_Str = NULL;
	C_MultiStr = NULL;
	CreateKey(Key, SubKey);
}

cRegistry::~cRegistry(){
	CloseKey();
	if(C_Str)
		delete[] C_Str;
	if(C_MultiStr)
		delete[] C_MultiStr;
}

DWORD cRegistry::GetSize(PCHAR Value){
	DWORD Size;
	RegQueryValueEx(hKey, Value, NULL, NULL, NULL, &Size);
	return Size;
}

BOOL cRegistry::Exists(PCHAR Value){
	DWORD Size = strlen(Value) + 1;
	CHAR Temp[256];
	if(Size > sizeof(Temp))
		return FALSE;
	INT i = 0;
	LONG Return;
	do{
		DWORD iSize = Size;
		Return = RegEnumValue(hKey, i, (LPSTR)Temp, &iSize, NULL, NULL, NULL, NULL);
		if(Return != ERROR_MORE_DATA)
		if(strcmp(Temp, Value) == 0)
			return TRUE;
		i++;
	}while(Return == ERROR_SUCCESS && Return != ERROR_NO_MORE_ITEMS || Return == ERROR_MORE_DATA);
	return FALSE;
}

LONG cRegistry::DeleteValue(PCHAR Value){
	return RegDeleteValue(hKey, Value);
}

LONG cRegistry::GetBinary(PCHAR Value, PCHAR Buffer, PDWORD Size){
	return RegQueryValueEx(hKey, Value, NULL, NULL, (LPBYTE)Buffer, Size);
}

LONG cRegistry::SetBinary(PCHAR Value, PCHAR Buffer, DWORD Size){
	return RegSetValueEx(hKey, Value, NULL, REG_BINARY, (LPBYTE)Buffer, Size);
}

LONG cRegistry::SetString(PCHAR Value, PCHAR String){
	return RegSetValueEx(hKey, Value, NULL, REG_SZ, (LPBYTE)String, strlen(String) + 1);
}

PCHAR cRegistry::GetString(PCHAR Value){
	DWORD Size = GetSize(Value);
	if(C_Str)
		delete[] C_Str;
	C_Str = new CHAR[Size];
	RegQueryValueEx(hKey, Value, NULL, NULL, (LPBYTE)C_Str, &Size);
	return C_Str;
}

DWORD cRegistry::GetType(PCHAR Value){
	DWORD Type;
	RegQueryValueEx(hKey, Value, NULL, &Type, NULL, NULL);
	return Type;
}

LONG cRegistry::SetInt(PCHAR Value, DWORD Int){
	return RegSetValueEx(hKey, Value, NULL, REG_DWORD, (LPBYTE)&Int, sizeof(Int));
}

DWORD cRegistry::GetInt(PCHAR Value){
	INT Int;
	DWORD Size = sizeof(Int);
	RegQueryValueEx(hKey, Value, NULL, NULL, (LPBYTE)&Int, &Size);
	return Int;
}

INT cRegistry::GetMultiStringElements(PCHAR Value){
	DWORD Size = GetSize(Value);
	if(C_MultiStr)
		delete[] C_MultiStr;
	C_MultiStr = new CHAR[Size];
	RegQueryValueEx(hKey, Value, NULL, NULL, (LPBYTE)C_MultiStr, &Size);
	INT Elements = 0;
	for(INT i = 0; i < Size; i++){
		if(C_MultiStr[i] == 0 && i != (Size - 1))
			Elements++;
	}
	return Elements;
}

PCHAR cRegistry::GetMultiString(PCHAR Value, UINT Index){
	INT Elements = GetMultiStringElements(Value);
	if(Index > Elements)
		return NULL;
	UINT Length = 0;
	UINT CurrentString = 0;
	for(INT i = 0; i < GetSize(Value); i++){
		Length++;
		if(C_MultiStr[i] == 0){
			if(Index == CurrentString)
				return C_MultiStr + (i - Length + 1);
			Length = 0;
			CurrentString++;
		}
	}
	return NULL;
}

INT cRegistry::MultiStringExists(PCHAR Value, PCHAR String){
	for(INT i = 0; i < GetMultiStringElements(Value); i++){
		if(strcmp(GetMultiString(Value, i), String) == 0)
			return i;
	}
	return -1;
}

LONG cRegistry::InsertMultiString(PCHAR Value, PCHAR String){
	DWORD Size = GetSize(Value);
	DWORD SetSize = Size + strlen(String) + 1;
	if(!Size)
		SetSize++;
	if(C_MultiStr)
		delete[] C_MultiStr;
	C_MultiStr = new CHAR[SetSize];
	RegQueryValueEx(hKey, Value, NULL, NULL, (LPBYTE)C_MultiStr, &Size);
	strcpy(C_MultiStr + Size - (Size?1:0), String);
	C_MultiStr[SetSize - 1] = 0;
	return RegSetValueEx(hKey, Value, NULL, REG_MULTI_SZ, (LPBYTE)C_MultiStr, SetSize);
}

LONG cRegistry::RemoveMultiString(PCHAR Value, UINT Index){
	DWORD Size = GetSize(Value);
	if(!Size)
		return FALSE;
	if(C_MultiStr)
		delete[] C_MultiStr;
	C_MultiStr = new CHAR[Size];
	RegQueryValueEx(hKey, Value, NULL, NULL, (LPBYTE)C_MultiStr, &Size);
	UINT CurrentString = 0;
	UINT BytesRead = 0;
	UINT ElementSize = 0;
	for(INT i = 0; i < Size; i++, BytesRead++, ElementSize++){
		if(C_MultiStr[i] == 0){
			if(Index == CurrentString){
				memcpy(C_MultiStr + BytesRead - ElementSize + (CurrentString?1:0), C_MultiStr + BytesRead + 1, Size - BytesRead);
				RegSetValueEx(hKey, Value, NULL, REG_MULTI_SZ, (LPBYTE)C_MultiStr, Size - ElementSize - (CurrentString?0:1));
			}
			CurrentString++;
			ElementSize = 0;
		}
	}
	return FALSE;
}

LONG cRegistry::CreateKey(HKEY Key, PCHAR SubKey, REGSAM Access, LPDWORD Disposition){
	return RegCreateKeyEx(Key, SubKey, 0, NULL, REG_OPTION_NON_VOLATILE, Access, NULL, &hKey, Disposition);
}

LONG cRegistry::CreateKey(HKEY Key, PCHAR SubKey, LPDWORD Disposition){
	return CreateKey(Key, SubKey, KEY_READ | KEY_WRITE, Disposition);
}

LONG cRegistry::CreateKey(HKEY Key, PCHAR SubKey){
	DWORD Disposition;
	return CreateKey(Key, SubKey, KEY_READ | KEY_WRITE, &Disposition);
}

LONG cRegistry::OpenKey(HKEY Key, PCHAR SubKey, REGSAM Access){
	DWORD Disposition;
	return RegOpenKeyEx(Key, SubKey, 0, Access, &hKey);
}

LONG cRegistry::OpenKey(HKEY Key, PCHAR SubKey){
	return OpenKey(Key, SubKey, KEY_READ | KEY_WRITE);
}

LONG cRegistry::CloseKey(VOID){
	LONG Return = ERROR_SUCCESS;
	if(hKey){
		Return = RegCloseKey(hKey);
		hKey = NULL;
	}
	return Return;
}

LONG cRegistry::EnumKey(UINT Index, PCHAR SubKey, LPDWORD Size){
	return RegEnumKeyEx(hKey, Index, SubKey, Size, NULL, NULL, NULL, NULL);
}