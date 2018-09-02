#include <winsock2.h>
#include "filescan.h"

VOID FileScan::Start(VOID){
	StartThread();
}

VOID FileScan::ThreadFunc(VOID){
	SetPriority(THREAD_PRIORITY_BELOW_NORMAL);
	SetErrorMode(SEM_FAILCRITICALERRORS);
	PreScan();
	CHAR Drive[5] = "A:\\";
	for(UINT i = 'A'; i <= 'Z'; i++){
		Drive[0] = i;
		UINT DriveType = GetDriveType(Drive);
		if(DriveType != DRIVE_UNKNOWN && DriveType != DRIVE_NO_ROOT_DIR){
			Drive[3] = '*';
			EnumDirectory(Drive);
			Drive[3] = NULL;
		}
	}
	Done();
}

VOID FileScan::EnumDirectory(PCHAR Directory){
	WIN32_FIND_DATA FindData;
	HANDLE Handle = FindFirstFile(Directory, &FindData);
	if(Handle != INVALID_HANDLE_VALUE){
		do{
			if(strcmp(FindData.cFileName, ".") && strcmp(FindData.cFileName, "..")){
				CHAR Path[MAX_PATH];
				strcpy(Path, Directory);
				if(Path[strlen(Path) - 1] == '*')
					Path[strlen(Path) - 1] = NULL;
				strcat(Path, FindData.cFileName);
				if(FindData.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY){
					strcat(Path, "\\*");
					EnumDirectory(Path);
				}else{
					Process(Path);
				}
			}
		}while(FindNextFile(Handle, &FindData));
		FindClose(Handle);
	}
}

VOID FileScan::PreScan(VOID){

}

VOID FileScan::Done(VOID){

}