#include "idletrack.h"

class IdleTrack IdleTrack;

IdleTrack::IdleTrack(){
	Timeout.Reset();
	StartThread();
}

ULONGLONG IdleTrack::GetIdleTime(VOID){
	return Timeout.GetElapsedTime();
}

VOID IdleTrack::ThreadFunc(VOID){
	POINT CursorPos;
	POINT OldCursorPos = {0, 0};
	while(1){
		for(UINT i = 0; i < 256; i++){
			if(GetAsyncKeyState(i) & 1){
				Timeout.Reset();
				break;
			}
		}
		GetCursorPos(&CursorPos);
		if(CursorPos.x != OldCursorPos.x && CursorPos.y != OldCursorPos.y)
			Timeout.Reset();
		OldCursorPos = CursorPos;
		Sleep(1000);
	}
}