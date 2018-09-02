#include <winsock2.h>
#include "thread.h"

Thread::Thread(){
	ThreadStarted = FALSE;
	ThreadHandle = (HANDLE)-1;
}

Thread::~Thread(){
	if(ThreadHandle != (HANDLE)-1){
		CloseHandle(ThreadHandle);
		ThreadHandle = (HANDLE)-1;
	}
}

VOID Thread::StartThread(VOID){
	ThreadHandle = (HANDLE)_beginthreadex(NULL, 0, ThreadCall, this, 0, &ThreadID);
	//ThreadHandle = (HANDLE)CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadCall, this, 0, (LPDWORD)&ThreadID);
}

UINT __stdcall Thread::ThreadCall(LPVOID ThisP){
	((Thread*)ThisP)->ThreadFuncD();
	return 0;
}

VOID Thread::ThreadFuncD(VOID){
	ThreadStarted = TRUE;
	ThreadFunc();
	OnEnd();
	delete this;
	return;
}

VOID Thread::ThreadFunc(VOID){
	return;
}

VOID Thread::OnEnd(VOID){

}

BOOL Thread::IsThreadStarted(VOID) const {
	return ThreadStarted;
}

BOOL Thread::SetPriority(INT Priority) const {
	return SetThreadPriority(ThreadHandle, Priority);
}

UINT Thread::GetThreadID(VOID) const {
	return ThreadID;
}

HANDLE Thread::GetThreadHandle(VOID) const {
	return ThreadHandle;
}