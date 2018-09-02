#include "hostexempt.h"

HostExempts::~HostExempts(){
	Mutex.WaitForAccess();
	for(UINT i = 0; i < List.size(); i++)
		delete List[i];
	Mutex.Release();
}

VOID HostExempts::Add(PCHAR Host){
	if(Exists(Host))
		return;
	Mutex.WaitForAccess();
	PCHAR lpHost = new CHAR[strlen(Host)];
	strcpy(lpHost, Host);
	List.push_back(lpHost);
	Mutex.Release();
}

BOOL HostExempts::Matches(PCHAR Host){
	if(List.size() == 0)
		return FALSE;
	Mutex.WaitForAccess();
	for(UINT i = 0; i < List.size(); i++){
		if(WildcardCompare(List[i], Host)){
			Mutex.Release();
			return TRUE;
		}
	}
	Mutex.Release();
	return FALSE;
}

BOOL HostExempts::Exists(PCHAR Host){
	Mutex.WaitForAccess();
	for(UINT i = 0; i < List.size(); i++){
		if(strcmp(List[i], Host) == 0){
			Mutex.Release();
			return TRUE;
		}
	}
	Mutex.Release();
	return FALSE;
}