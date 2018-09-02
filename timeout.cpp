#include "timeout.h"

Timeout::Timeout(){
	OldTime = NULL;
	TimeoutTime = NULL;
}

VOID Timeout::SetTimeout(UINT Timeout){
	TimeoutTime = Timeout;
}

VOID Timeout::Reset(VOID){
	OldTime = GetTickCount();
}

VOID Timeout::CheckOverlap(VOID){
	if(OldTime > GetTickCount())
		OldTime = 0xFFFFFFFFFFFFFFFF - OldTime;
}

BOOL Timeout::TimedOut(VOID){
	if(GetElapsedTime() >= TimeoutTime && TimeoutTime)
		return TRUE;
	return FALSE;
}

ULONGLONG Timeout::GetElapsedTime(VOID){
	CheckOverlap();
	return (GetTickCount() - OldTime);
}