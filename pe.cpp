#include "pe.h"

DWORD RvaToVa(IMAGE_NT_HEADERS* NtHeaders, DWORD Rva){
	DWORD Va = NULL;
	PCHAR End = (PCHAR)NtHeaders + sizeof(IMAGE_NT_HEADERS);
	IMAGE_SECTION_HEADER* Section = (IMAGE_SECTION_HEADER*)(End);
	for(UINT i = 0; i < NtHeaders->FileHeader.NumberOfSections; i++){
		if(Rva >= Section->VirtualAddress && Rva < (Section->VirtualAddress + Section->SizeOfRawData)){
			Va = Section->PointerToRawData + (Rva - Section->VirtualAddress);
			break;
		}
		Section++;
	}
	return Va;
}