#include "sysinfo.h"
#include "debug.h"

OSVERSIONINFO OsInfo;

PCHAR GetWindowsVersionName(VOID){
	OsInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&OsInfo);
	if(OsInfo.dwMajorVersion == 4){
		if(OsInfo.dwMinorVersion == 0 && OsInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
			return "95";
		if(OsInfo.dwMinorVersion == 0 && OsInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
			return "NT4";
		if(OsInfo.dwMinorVersion == 10)
			return "98";
		if(OsInfo.dwMinorVersion == 90)
			return "ME";
	}else
	if(OsInfo.dwMajorVersion == 5){
		if(OsInfo.dwMinorVersion == 0)
			return "2000";
		if(OsInfo.dwMinorVersion == 1)
			return "XP";
		if(OsInfo.dwMinorVersion == 2)
			return "2K3/XP64";
	}else
	if(OsInfo.dwMajorVersion == 6){
		if(OsInfo.dwMinorVersion == 0)
			return "Vista";
	}
	return "Unknown";
}

PCHAR GetWindowsVersionServicePackName(VOID){
	OsInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&OsInfo);
	if(!OsInfo.szCSDVersion)
		return "";
	return OsInfo.szCSDVersion;
}

DWORD GetTotalPhysicalMemory(VOID){
	MEMORYSTATUS MemStatus;
	MemStatus.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&MemStatus);
	return MemStatus.dwTotalPhys;
}

DWORD GetAvailablePhysicalMemory(VOID){
	MEMORYSTATUS MemStatus;
	MemStatus.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&MemStatus);
	return MemStatus.dwAvailPhys;
}

ULONG CycleCount(VOID)
{
	_asm rdtsc;
}

UINT GetCPUClock(VOID){
	ULONG StartCycles = CycleCount();
	Sleep(100);
	ULONG EndCycles = CycleCount();
	ULONG Cycles = (EndCycles - StartCycles) / 100000;
	UINT Round = Cycles % 100;
	UINT Round2 = 100;
	if(Round < 80)
		Round2 = 66;
	if(Round < 60)
		Round2 = 33;
	if(Round < 25)
		Round2 = 0;
	return (UINT)(Cycles - Round) + Round2;
}