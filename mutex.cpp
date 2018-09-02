#include "mutex.h"

Mutex::Mutex(){
	MutexHandle = CreateMutex(NULL, FALSE, NULL);
}

Mutex::~Mutex(){
	CloseHandle(MutexHandle);
}

DWORD Mutex::WaitForAccess(VOID) const {
	return WaitForAccess(INFINITE);
}

DWORD Mutex::WaitForAccess(DWORD Timeout) const {
	return WaitForSingleObject(MutexHandle, Timeout);;
}

BOOL Mutex::Release(VOID) const {
	return ReleaseMutex(MutexHandle);
}

CriticalSection::CriticalSection(){
	InitializeCriticalSection(&CriticalSectionObject);
}

CriticalSection::~CriticalSection(){
	DeleteCriticalSection(&CriticalSectionObject);
}

VOID CriticalSection::Enter(VOID){
	EnterCriticalSection(&CriticalSectionObject);
}

VOID CriticalSection::Leave(VOID){
	LeaveCriticalSection(&CriticalSectionObject);
}