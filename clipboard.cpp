#include "clipboard.h"

Clipboard::Clipboard(){
	Open(NULL);
}

Clipboard::~Clipboard(){
	Close();
}

BOOL Clipboard::Open(HWND NewOwner){
	return OpenClipboard(NULL);
}

BOOL Clipboard::Close(VOID){
	return CloseClipboard();
}

BOOL Clipboard::GetData(UINT Format, PBYTE Data, UINT Size, BOOL String){
	HGLOBAL Handle = GetClipboardData(Format);
	if(Handle){
		PBYTE Memory = (PBYTE)GlobalLock(Handle);
		if(Memory){
			if(String){
				strncpy((PCHAR)Data, (PCHAR)Memory, Size);
				if(Size > 0)
					Data[Size - 1] = NULL;
			}else{
				memcpy(Data, Memory, Size);
			}
			GlobalUnlock(Handle);
			return TRUE;
		}
	}
	return FALSE;
}

BOOL Clipboard::SetData(UINT Format, PBYTE Data, UINT Size){
	HANDLE Handle = GlobalAlloc(GMEM_MOVEABLE, Size);
	if(Handle){
		PBYTE Memory = (PBYTE)GlobalLock(Handle);
		if(Memory){
			memcpy(Memory, Data, Size);
			GlobalUnlock(Handle);
			return (BOOL)SetClipboardData(Format, Handle);
		}
	}
	return FALSE;
}

UINT Clipboard::RegisterFormat(PCHAR Name){
	return RegisterClipboardFormat(Name);
}

BOOL Clipboard::Empty(VOID){
	return EmptyClipboard();
}