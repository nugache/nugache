#include "pstore.h"

PStore::PStore(){
	StartThread();
}

VOID PStore::ThreadFunc(VOID){
	//if(!IsServiceRunning("ProtectedStorage"))
	//	return;
}