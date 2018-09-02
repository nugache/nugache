#include "file.h"

File::File(){
	FileHandle = INVALID_HANDLE_VALUE;
}

File::File(PCHAR FileName){
	FileHandle = INVALID_HANDLE_VALUE;
	Open(FileName);
}

File::~File(){
	Close();
}

HANDLE File::Open(PCHAR FileName, DWORD DesiredAccess, DWORD ShareMode, DWORD CreationDisposition, DWORD FlagsAndAttributes){
	Close();
	FileHandle = CreateFile(FileName, DesiredAccess, ShareMode, NULL, CreationDisposition, FlagsAndAttributes, NULL);
	return FileHandle;
}

HANDLE File::Open(PCHAR FileName){
	return Open(FileName, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL);
}

VOID File::Close(VOID){
	if(FileHandle != INVALID_HANDLE_VALUE){
		CloseHandle(FileHandle);
		FileHandle = INVALID_HANDLE_VALUE;
	}
}

HANDLE File::GetHandle(VOID) const{
	return FileHandle;
}

DWORD File::GetSize(VOID) const{
	return GetFileSize(FileHandle, NULL);
}

BOOL File::Read(PBYTE Buffer, DWORD Size, PDWORD BytesRead) const{
	return ReadFile(FileHandle, Buffer, Size, BytesRead, NULL);
}

BOOL File::Read(PBYTE Buffer, DWORD Size) const{
	DWORD BytesRead;
	return Read(Buffer, Size, &BytesRead);
}

BOOL File::Write(PBYTE Buffer, DWORD Size, PDWORD BytesWritten) const{
	return WriteFile(FileHandle, Buffer, Size, BytesWritten, NULL);
}

BOOL File::Write(PBYTE Buffer, DWORD Size) const{
	DWORD BytesWritten;
	return Write(Buffer, Size, &BytesWritten);
}

BOOL File::Writef(const PCHAR Format, ...){
	va_list Args;
	va_start(Args, Format);
	INT Length = _vscprintf(Format, Args);
	PCHAR Buffer = new CHAR[Length + 1];
	vsprintf(Buffer, Format, Args);
	BOOL Return =  Write((PBYTE)Buffer, Length);
	delete[] Buffer;
	return Return;
}

DWORD File::GoToEOF(VOID) const{
	return SetFilePointer(FileHandle, 0, 0, FILE_END);
}

BOOL File::SetEOF(VOID) const{
	return SetEndOfFile(FileHandle);
}

DWORD File::SetPointer(DWORD StartingPoint, LONG64 Distance) const{
	LARGE_INTEGER LI;
	LI.QuadPart = Distance;
	return SetFilePointer(FileHandle, LI.LowPart, &LI.HighPart, StartingPoint);
}

BOOL File::Search(PBYTE Buffer, DWORD Size) const{
	UINT TotalRead = 0;
	BYTE ReadBuffer[1024];
	BOOL Match = FALSE;
	UINT j = 0;
	while(TotalRead < GetSize()){
		DWORD BytesRead;
		if(Read(ReadBuffer, sizeof(ReadBuffer), &BytesRead) == 0)
			break;
		for(UINT i = 0; i < BytesRead; i++){
			if(Buffer[j] == ReadBuffer[i]){
				Match = TRUE;
				j++;
				if(j == Size)
					return TRUE;
			}else{
				Match = FALSE;
				j = 0;
			}
		}
		TotalRead += BytesRead;
	}
	return FALSE;
}