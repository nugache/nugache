#include <winsock2.h>
#include "messagedialog.h"

extern HINSTANCE hInst;
extern Connection Connection;

namespace messagequeue{

HWND WindowActive = FALSE;
std::map<LONG, MESSAGEMAP> MessageMap;
PCHAR Comments = NULL;
PCHAR Params = NULL;
PCHAR Script = NULL;
LONG ID = 0;
Mutex Mutex;
DWORD ResizingBox = NULL;
POINT OldCursorPos, PointM;
LONG SelectedItem = NULL;
BOOL UpdateEditBoxes = TRUE;
BOOL UpdateListBox = TRUE;

DWORD GenerateNewMID(VOID){
	DWORD MID;
	do{
		MID = rand_r(0, 0xffffffff);
	}while(MessageMap.find(MID) != MessageMap.end());
	return MID;
}

VOID DisableOptions(HWND hWnd){
	EnableWindow(GetDlgItem(hWnd, IDC_EXECLIENTS), FALSE);
	EnableWindow(GetDlgItem(hWnd, IDC_EXELINKS), FALSE);
	EnableWindow(GetDlgItem(hWnd, IDC_UUID), FALSE);
	EnableWindow(GetDlgItem(hWnd, IDC_COMBO), FALSE);
	EnableWindow(GetDlgItem(hWnd, IDC_TTL), FALSE);
	EnableWindow(GetDlgItem(hWnd, IDC_SCRIPTNAME), FALSE);
	EnableWindow(GetDlgItem(hWnd, IDC_SCRIPT), FALSE);
	EnableWindow(GetDlgItem(hWnd, IDC_NEWMID), FALSE);
}

VOID EnableOptions(HWND hWnd){
	EnableWindow(GetDlgItem(hWnd, IDC_EXECLIENTS), TRUE);
	EnableWindow(GetDlgItem(hWnd, IDC_EXELINKS), TRUE);
	EnableWindow(GetDlgItem(hWnd, IDC_COMBO), TRUE);
	EnableWindow(GetDlgItem(hWnd, IDC_TTL), TRUE);
	EnableWindow(GetDlgItem(hWnd, IDC_SCRIPTNAME), TRUE);
	EnableWindow(GetDlgItem(hWnd, IDC_SCRIPT), TRUE);
	EnableWindow(GetDlgItem(hWnd, IDC_NEWMID), TRUE);
}

VOID AddItem(HWND hWnd, PCHAR Name, PCHAR Script, PCHAR SendTo, INT TTL, BOOL RunLocally, BOOL Broadcast){
	MESSAGEMAP MsgMap;
	INT Item;
	Mutex.WaitForAccess();
	MsgMap.MID = GenerateNewMID();
	MsgMap.RunLocally = RunLocally;
	MsgMap.Broadcast = Broadcast;
	strcpy(MsgMap.SendTo, SendTo);
	strcpy(MsgMap.Script, Script);
	strcpy(MsgMap.Name, Name);
	EnableOptions(hWnd);
	Mutex.Release();
	LVITEM LVItem;
	memset(&LVItem, 0, sizeof(LVItem));
	LVItem.mask = LVIF_PARAM | LVIF_TEXT;
	LVItem.iItem = 0;
	LVItem.iSubItem = 0;
	LVItem.pszText = MsgMap.Name;
	//LVItem.cchTextMax = strlen(MsgMap.Name);
	LVItem.lParam = ID;
	Item = ListView_InsertItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
	LVItem.mask = LVIF_TEXT;
	LVItem.iSubItem = 8;
	LVItem.pszText = MsgMap.Script;
	ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
	LVItem.mask = LVIF_TEXT;
	LVItem.iSubItem = 1;
	LVItem.pszText = "Waiting";
	ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
	LVItem.iSubItem = 7;
	CHAR CMID[32];
	sprintf(CMID, "%X", MsgMap.MID);
	LVItem.pszText = CMID;
	BOOL ExeOnClients = FALSE;
	BOOL ExeOnLinks = FALSE;
	if(strcmp(MsgMap.SendTo, "1") == 0 || strcmp(MsgMap.SendTo, "3") == 0)
		ExeOnClients = TRUE;
	if(strcmp(MsgMap.SendTo, "2") == 0 || strcmp(MsgMap.SendTo, "3") == 0)
		ExeOnLinks = TRUE;
	ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
	LVItem.iSubItem = 2;
	if(ExeOnClients){
		LVItem.pszText = "*";
	}else{
		LVItem.pszText = "-";
	}
	ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
	LVItem.iSubItem = 3;
	if(ExeOnLinks){
		LVItem.pszText = "*";
	}else{
		LVItem.pszText = "-";
	}
	ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
	LVItem.iSubItem = 4;
	if(!ExeOnClients && !ExeOnLinks && strcmp(MsgMap.SendTo, "0") != 0 && strlen(MsgMap.SendTo) > 0){
		LVItem.pszText = "*";
	}else{
		LVItem.pszText = "-";
	}
	ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
	MsgMap.TTL = TTL;
	CHAR TTLA[32];
	LVItem.iSubItem = 6;
	if(!MsgMap.Broadcast){
		LVItem.pszText = "-";
	}else{
		LVItem.pszText = itoa(MsgMap.TTL, TTLA, 10);
	}
	ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
	LVItem.iSubItem = 5;
	if(RunLocally){
		LVItem.pszText = "*";
	}else{
		LVItem.pszText = "-";
	}
	ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
	MsgMap.State = MESSAGEMAP::WAITING;
	Mutex.WaitForAccess();
	MessageMap.insert(std::make_pair(ID++, MsgMap));
	MessageMap[ID - 1] = MsgMap;
	Mutex.Release();
	ListView_SetItemState(GetDlgItem(WindowActive, IDC_LIST), LVItem.iItem, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
}

VOID ResizeMiddle(HWND hWnd, INT Difference){
	RECT Rect, Rect2;
	POINT Point;
	GetWindowRect(GetDlgItem(hWnd, IDC_LIST), &Rect);
	GetWindowRect(GetDlgItem(hWnd, IDC_SCRIPT), &Rect2);
	INT Pos = Rect.bottom - Rect.top + Difference;
	if(Difference + Rect.bottom - Rect.top < 30){
		Difference = 30 - (Rect.bottom - Rect.top);
	}
	if(-Difference + Rect2.bottom - Rect2.top < 30){
		Difference = -30 + Rect2.bottom - Rect2.top;
	}
	SetWindowPos(GetDlgItem(hWnd, IDC_LIST), NULL, NULL, NULL, Rect.right - Rect.left, Rect.bottom - Rect.top + Difference, SWP_NOMOVE | SWP_NOZORDER);
	for(UINT i = 0; i <= 19; i++){
		INT Item;
		switch(i){
			case 0: Item = IDC_STATICEXE; break;
			case 1: Item = IDC_STATICMSG; break;
			case 2: Item = IDC_EXECLIENTS; break;
			case 3: Item = IDC_EXELINKS; break;
			case 4: Item = IDC_STATICUUID; break;
			case 5: Item = IDC_UUID; break;
			case 6: Item = IDC_COMBO; break;
			case 7: Item = IDC_STATICTTL; break;
			case 8: Item = IDC_TTL; break;
			case 9: Item = IDC_STATICADD; break;
			case 10: Item = IDC_ADDNEW; break;
			case 11: Item = IDC_STATICSCRIPTNAME; break;
			case 12: Item = IDC_STATICMSGID; break;
			case 13: Item = IDC_SCRIPTNAME; break;
			case 14: Item = IDC_MID; break;
			case 15: Item = IDC_NEWMID; break;
			case 16: Item = IDC_STATIC_BARTOP; break;
			case 17: Item = IDC_STATIC_BARBOTTOM; break;
			case 18: Item = IDC_ADD; break;
			case 19: Item = IDC_STATICBG; break;
		}
		GetWindowRect(GetDlgItem(hWnd, Item), &Rect);
		Point.x = Rect.left;
		Point.y = Rect.top;
		ScreenToClient(hWnd, &Point);
		MoveWindow(GetDlgItem(hWnd, Item), Point.x, Point.y + Difference, Rect.right - Rect.left, Rect.bottom - Rect.top, TRUE);
	}
		cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
		Reg.SetInt(REG_MSGQUEUEB, Pos);
		Reg.CloseKey();

		GetWindowRect(GetDlgItem(hWnd, IDC_SCRIPT), &Rect);
		//if(Difference + Rect.bottom - Rect.top >= 30){
			Point.x = Rect.left;
			Point.y = Rect.top;
			ScreenToClient(hWnd, &Point);
			SetWindowPos(GetDlgItem(hWnd, IDC_SCRIPT), NULL, Point.x, Point.y + Difference, Rect.right - Rect.left, Rect.bottom - Rect.top - Difference, SWP_NOZORDER);
		//}
		InvalidateRect(hWnd, NULL, TRUE);
}

BOOL CALLBACK MessageQueueProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam){
	if(Msg == WM_INITDIALOG){
		SetWindowPos(hWnd, HWND_BOTTOM, NULL, NULL, NULL, NULL, SWP_NOSIZE | SWP_NOMOVE);
		if(WindowActive){
			SetActiveWindow(WindowActive);
			ShowWindow(WindowActive, SW_SHOW);
			DestroyWindow(hWnd);
		}else{
			WindowActive = hWnd;
			DisableOptions(hWnd);
			cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
			if(!Reg.Exists(REG_MSGQUEUEX) || !Reg.Exists(REG_MSGQUEUEY) || !Reg.Exists(REG_MSGQUEUEH)){
				RECT Rect;
				GetWindowRect(hWnd, &Rect);
				Reg.SetInt(REG_MSGQUEUEX, Rect.left);
				Reg.SetInt(REG_MSGQUEUEY, Rect.top);
				Reg.SetInt(REG_MSGQUEUEH, Rect.bottom - Rect.top);
			}else{
				RECT Rect;
				GetWindowRect(hWnd, &Rect);
				MoveWindow(hWnd, Reg.GetInt(REG_MSGQUEUEX), Reg.GetInt(REG_MSGQUEUEY), Rect.right - Rect.left, Reg.GetInt(REG_MSGQUEUEH), TRUE);
				INT Difference = Reg.GetInt(REG_MSGQUEUEB);
				ResizeMiddle(hWnd, -99999);
				ResizeMiddle(hWnd, Difference - 30);
			}
			SendMessage(GetDlgItem(hWnd, IDC_LIST), LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_GRIDLINES, LVS_EX_GRIDLINES);
			EnableWindow(GetDlgItem(hWnd, IDC_UUID), FALSE);
			CheckDlgButton(hWnd, IDC_EXECLIENTS, BST_CHECKED);
			CheckDlgButton(hWnd, IDC_EXELINKS, BST_CHECKED);
			CHAR Temp[16];
			itoa(MAX_MESSAGE_TTL, Temp, 10);
			SetWindowText(GetDlgItem(hWnd, IDC_TTL), Temp);
			EnableWindow(GetDlgItem(hWnd, IDC_TTL), FALSE);
			SendMessage(GetDlgItem(hWnd, IDC_COMBO), CB_RESETCONTENT, NULL, NULL);
			SendMessage(GetDlgItem(hWnd, IDC_COMBO), CB_ADDSTRING, NULL, (LPARAM)"Run Locally");
			SendMessage(GetDlgItem(hWnd, IDC_COMBO), CB_ADDSTRING, NULL, (LPARAM)"Broadcast");
			SendMessage(GetDlgItem(hWnd, IDC_COMBO), CB_ADDSTRING, NULL, (LPARAM)"Run & Broadcast");
			SendMessage(GetDlgItem(hWnd, IDC_COMBO), CB_SETCURSEL, NULL, 2);
			LVCOLUMN LVColumn;
			LVColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
			struct COLUMN{
				CHAR Name[16];
				INT Width;
				INT Format;
			};
			COLUMN Column[] = {
			{"Name", 100, LVCFMT_LEFT},
			{"Status", 80, LVCFMT_CENTER},
			{"Client Execute", 18, LVCFMT_LEFT},
			{"Link Execute", 18, LVCFMT_LEFT},
			{"UUID Execute", 18, LVCFMT_LEFT},
			{"Run Locally", 18, LVCFMT_LEFT},
			{"TTL", 35, LVCFMT_CENTER},
			{"MID", 67, LVCFMT_LEFT},			
			{"Script", 250, LVCFMT_LEFT},
			};
			for(INT i = 0; i < 9; i++){
				LVColumn.pszText = Column[i].Name;
				LVColumn.cx = Column[i].Width;
				LVColumn.fmt = Column[i].Format;
				LVColumn.iSubItem = i;
				ListView_InsertColumn(GetDlgItem(hWnd, IDC_LIST), i, &LVColumn);
			}
			SendMessage(GetDlgItem(hWnd, IDC_LIST), LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
			SendMessage(GetDlgItem(hWnd, IDC_ADD), BS_PUSHLIKE, NULL, NULL);

			CHAR Key[256];
			sprintf(Key, "%s\\%s", REG_ROOT, REG_MSGQUEUEITEMS);
			cRegistry Reg2(HKEY_CURRENT_USER, Key);
			CHAR Key2[32];
			DWORD Size = sizeof(Key2);
			INT Index = 0;
			LONG Return = Reg2.EnumKey(Index, Key2, &Size);
			while(Return == ERROR_SUCCESS || Return == ERROR_MORE_DATA){
				CHAR KeyTemp[256];
				sprintf(KeyTemp, "%s\\%s", Key, Key2);
				cRegistry Reg(HKEY_CURRENT_USER, KeyTemp);
				MESSAGEMAP MsgMap;
				strcpy(MsgMap.Name, Reg.GetString(REG_MSGQUEUEITEMSCRIPTNAME));
				strcpy(MsgMap.Script, Reg.GetString(REG_MSGQUEUEITEMSCRIPT));
				strcpy(MsgMap.SendTo, Reg.GetString(REG_MSGQUEUEITEMSENDTO));
				MsgMap.TTL = Reg.GetInt(REG_MSGQUEUEITEMTTL);
				MsgMap.RunLocally = Reg.GetInt(REG_MSGQUEUEITEMRUNLOCALLY);
				MsgMap.Broadcast = Reg.GetInt(REG_MSGQUEUEITEMBROADCAST);
				AddItem(hWnd, MsgMap.Name, MsgMap.Script, MsgMap.SendTo, MsgMap.TTL, MsgMap.RunLocally, MsgMap.Broadcast);
				Size = sizeof(Key2);
				Index++;
				Return = Reg2.EnumKey(Index, Key2, &Size);
			}
		}
	}else
	if(Msg == WM_COMMAND){
		if(HIWORD(wParam) == BN_CLICKED){
			if(LOWORD(wParam) == IDC_EXELINKS || LOWORD(wParam) == IDC_EXECLIENTS){
				Mutex.WaitForAccess();
				MESSAGEMAP MsgMap = MessageMap[SelectedItem];
				LVFINDINFO FindInfo;
				FindInfo.lParam = SelectedItem;
				FindInfo.flags = LVFI_PARAM;
				INT Index = -1;
				if((Index = ListView_FindItem(GetDlgItem(hWnd, IDC_LIST), -1, &FindInfo)) != -1){
					CHAR Test[32];
					strcpy(Test, "-");
					LVITEM LVItem;
					LVItem.iItem = Index;
					LVItem.iSubItem = 2;
					LVItem.mask = LVIF_TEXT;
					LVItem.pszText = Test;
					LVItem.cchTextMax = sizeof(Test);

					BOOL ExeClients = IsDlgButtonChecked(hWnd, IDC_EXECLIENTS) == BST_CHECKED;
					BOOL ExeLinks = IsDlgButtonChecked(hWnd, IDC_EXELINKS) == BST_CHECKED;
					INT SendTo = (ExeLinks << 1) | ExeClients;
					CHAR Temp[16];
					strcpy(MsgMap.SendTo, itoa(SendTo, Temp, 10));
					
					UpdateListBox = FALSE;
					if(ExeClients)
						strcpy(Test, "*");
					ListView_SetItem(GetDlgItem(hWnd, IDC_LIST), &LVItem);
					strcpy(Test, "-");
					LVItem.iSubItem = 3;
					if(ExeLinks)
						strcpy(Test, "*");
					ListView_SetItem(GetDlgItem(hWnd, IDC_LIST), &LVItem);
					MsgMap.State = MESSAGEMAP::WAITING;
					if(MsgMap.SignThread){
						HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, MsgMap.SignThread->GetThreadID());
						TerminateThread(hThread, 0);
						MsgMap.SignThread = NULL;
					}
					LVItem.iSubItem = 1;
					LVItem.pszText = "Waiting";
					ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
					MessageMap[SelectedItem] = MsgMap;
					Mutex.Release();
					UpdateListBox = TRUE;
				}
				if(IsDlgButtonChecked(hWnd, IDC_EXELINKS) == BST_UNCHECKED && IsDlgButtonChecked(hWnd, IDC_EXECLIENTS) == BST_UNCHECKED){
					EnableWindow(GetDlgItem(hWnd, IDC_UUID), TRUE);
				}else{
					EnableWindow(GetDlgItem(hWnd, IDC_UUID), FALSE);
				}
			}else
			if(LOWORD(wParam) == IDC_ADDNEW){
				AddItem(hWnd, "<New Script>", "", "3", 30, TRUE, TRUE);
			}else
			if(LOWORD(wParam) == IDC_NEWMID){
				if(MessageMap.size() > 0){
					DWORD MID = GenerateNewMID();
					CHAR MIDA[32];
					sprintf(MIDA, "%X", MID);
					SetWindowText(GetDlgItem(hWnd, IDC_MID), MIDA);
					MESSAGEMAP MsgMap = MessageMap[SelectedItem];
					MsgMap.MID = MID;
					MessageMap[SelectedItem] = MsgMap;
					LVFINDINFO FindInfo;
					FindInfo.lParam = SelectedItem;
					FindInfo.flags = LVFI_PARAM;
					INT Index = -1;
					if((Index = ListView_FindItem(GetDlgItem(hWnd, IDC_LIST), -1, &FindInfo)) != -1){
						CHAR Test[50];
						LVITEM LVItem;
						LVItem.iItem = Index;
						LVItem.iSubItem = 7;
						LVItem.mask = LVIF_TEXT;
						LVItem.pszText = MIDA;
						UpdateListBox = FALSE;
						ListView_SetItem(GetDlgItem(hWnd, IDC_LIST), &LVItem);
						UpdateListBox = TRUE;
					}
				}
			}/*else
			if(LOWORD(wParam) == IDC_ADD){
				HMENU hMenu = CreatePopupMenu();
				HMENU hMenuHTTP = CreateMenu();
				HMENU hMenuSpread = CreateMenu();
				InsertMenu(hMenuHTTP, 0, MF_BYPOSITION|MF_STRING, 100, "Download");
				InsertMenu(hMenuHTTP, 1, MF_BYPOSITION|MF_STRING, 101, "Visit");
				InsertMenu(hMenuSpread, 0, MF_BYPOSITION|MF_STRING, 200, "AIM");

				InsertMenu(hMenu, 0, MF_BYPOSITION|MF_STRING, 1, "Send Log");
				InsertMenu(hMenu, 1, MF_BYPOSITION|MF_STRING|MF_POPUP, (UINT_PTR)hMenuHTTP, "HTTP");
				InsertMenu(hMenu, 2, MF_BYPOSITION|MF_STRING, 2, "Start IRC Bot");
				InsertMenu(hMenu, 3, MF_BYPOSITION|MF_STRING|MF_POPUP, (UINT_PTR)hMenuSpread, "Spread");
				InsertMenu(hMenu, 4, MF_BYPOSITION|MF_STRING, 3, "Custom Command");
				RECT R;
				GetWindowRect(GetDlgItem(hWnd, IDC_ADD), &R);
				SendMessage(GetDlgItem(hWnd, IDC_ADD), BM_SETSTATE, TRUE, NULL);
				INT Return = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RETURNCMD, R.right, R.top, 0, hWnd, NULL);
				SendMessage(GetDlgItem(hWnd, IDC_ADD), BM_SETSTATE, FALSE, NULL);
				if(Return > 0){
					MESSAGEMAP MsgMap;
					INT Item;
					PCHAR Msg;
					DLGPROC DlgProc = NULL;
					DWORD Resource = NULL;
					switch(Return){
						//case 1: Msg = "Send Log"; MsgMap.Ordinal = MSG_SENDLOG; Resource = IDD_SENDLOG; DlgProc = SendLogProc; break;
						//case 2: Msg = "Start IRC Bot"; MsgMap.Ordinal = MSG_START_IRCBOT; Resource = IDD_STARTIRCBOT; DlgProc = StartIRCBotProc; break;
						//case 100: Msg = "HTTP->Download"; MsgMap.Ordinal = MSG_HTTP_DOWNLOAD; Resource = IDD_HTTPDOWNLOAD; DlgProc = HTTPDownloadProc; break;
						//case 200: Msg = "Spread->AIM"; MsgMap.Ordinal = MSG_SPREAD_AIM; Resource = IDD_SPREAD_AIM; DlgProc = SpreadAIMProc; break;
						case 3: Msg = "Custom Command"; Resource = IDD_CUSTOM_COMMAND; DlgProc = CustomCommandProc; break;
					}
					if(DlgProc && Resource){
						//Params = MsgMap.Params;
						//Comments = MsgMap.Comments;
						Script = MsgMap.Script;
						if(DialogBox(hInst, MAKEINTRESOURCE(Resource), hWnd, DlgProc)){
							DWORD MID = GenerateNewMID();
							Mutex.WaitForAccess();

							MsgMap.MID = MID;
							Mutex.Release();
							LVITEM LVItem;
							memset(&LVItem, 0, sizeof(LVItem));
							LVItem.mask = LVIF_TEXT | LVIF_PARAM;
							LVItem.iItem = 0;
							LVItem.iSubItem = 0;
							LVItem.pszText = Msg;
							LVItem.cchTextMax = strlen(Msg);
							LVItem.lParam = ID;
							Item = ListView_InsertItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
							LVItem.mask = LVIF_TEXT;
							LVItem.iSubItem = 1;
							LVItem.pszText = "Waiting";
							ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
							LVItem.iSubItem = 7;
							CHAR CMID[32];
							sprintf(CMID, "%X", MID);
							LVItem.pszText = CMID;
							ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
							BOOL NOUUID = FALSE;
							BOOL SendToClient = FALSE;
							BOOL SendToLink = FALSE;
							if(IsDlgButtonChecked(hWnd, IDC_EXECLIENTS) == BST_CHECKED){
								LVItem.iSubItem = 2;
								LVItem.pszText = "*";
								ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
								SendToClient = TRUE;
								NOUUID = TRUE;
							}
							if(IsDlgButtonChecked(hWnd, IDC_EXELINKS) == BST_CHECKED){
								LVItem.iSubItem = 3;
								LVItem.pszText = "*";
								ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
								SendToLink = TRUE;
								NOUUID = TRUE;
							}
							if(NOUUID){
								LVItem.iSubItem = 4;
								LVItem.pszText = "-";
								ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
								itoa(SendToClient + (SendToLink * 2), MsgMap.SendTo, 10);
							}else{
								LVItem.iSubItem = 2;
								LVItem.pszText = "-";
								ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
								LVItem.iSubItem = 3;
								LVItem.pszText = "-";
								ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
								GetWindowText(GetDlgItem(WindowActive, IDC_UUID), MsgMap.SendTo, sizeof(MsgMap.SendTo));
							}
							CHAR Temp[2];
							GetWindowText(GetDlgItem(hWnd, IDC_UUID), Temp, sizeof(Temp));
							if(strlen(Temp) > 0){
								LVItem.iSubItem = 4;
								LVItem.pszText = "*";
								ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
							}
							UINT Selection = SendMessage(GetDlgItem(hWnd, IDC_COMBO), CB_GETCURSEL, 0, 0) + 1;
							if(Selection & 1){
								LVItem.iSubItem = 5;
								LVItem.pszText = "*";
								ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
								MsgMap.RunLocally = TRUE;
							}else{
								MsgMap.RunLocally = FALSE;
							}
							if(Selection & 2){
								LVItem.iSubItem = 6;
								CHAR Temp[4];
								GetWindowText(GetDlgItem(hWnd, IDC_TTL), Temp, sizeof(Temp));
								LVItem.pszText = Temp;
								ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
								MsgMap.TTL = atoi(Temp);
							}else{
								LVItem.iSubItem = 6;
								LVItem.pszText = "-";
								ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
							}
							LVItem.iSubItem = 8;
							//LVItem.pszText = MsgMap.Comments;
							ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
							MsgMap.State = MESSAGEMAP::WAITING;
							Mutex.WaitForAccess();
							MessageMap.insert(std::make_pair(ID++, MsgMap));
							Mutex.Release();
						}
					}
				}
			}*/
		}else
		if(HIWORD(wParam) == EN_CHANGE){
			if(LOWORD(wParam) == IDC_UUID){
				/*CHAR Text[2];
				GetWindowText(GetDlgItem(hWnd, IDC_UUID), Text, sizeof(Text));
				if(strlen(Text) > 0){
					EnableWindow(GetDlgItem(hWnd, IDC_EXELINKS), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_EXECLIENTS), FALSE);
				}else{
					EnableWindow(GetDlgItem(hWnd, IDC_EXELINKS), TRUE);
					EnableWindow(GetDlgItem(hWnd, IDC_EXECLIENTS), TRUE);
				}*/
			}else
			if(LOWORD(wParam) == IDC_TTL){
				CHAR Text[4];
				GetWindowText(GetDlgItem(hWnd, IDC_TTL), Text, sizeof(Text));
				UINT TTL = atoi(Text);
				if(TTL < 1){
					SetWindowText(GetDlgItem(hWnd, IDC_TTL), "1");
				}else
				if(TTL > MAX_MESSAGE_TTL){
					SetWindowText(GetDlgItem(hWnd, IDC_TTL), itoa(MAX_MESSAGE_TTL, Text, 10));
				}
			}
		}else
		if(HIWORD(wParam) == EN_UPDATE){
			if(UpdateEditBoxes && MessageMap.size() > 0){
				Mutex.WaitForAccess();
				BOOL TTLUpdated = FALSE;
				if(LOWORD(wParam) == IDC_SCRIPT){
					MESSAGEMAP MsgMap = MessageMap[SelectedItem];
					GetWindowText(GetDlgItem(hWnd, IDC_SCRIPT), MsgMap.Script, sizeof(MsgMap.Script));
					MessageMap[SelectedItem] = MsgMap;
					//dprintf("set item %X\r\n", SelectedItem);
					//
					//strcpy(SelectedItem->Script, "poop");
				}else
				if(LOWORD(wParam) == IDC_SCRIPTNAME){
					MESSAGEMAP MsgMap = MessageMap[SelectedItem];
					GetWindowText(GetDlgItem(hWnd, IDC_SCRIPTNAME), MsgMap.Name, sizeof(MsgMap.Name));
					MessageMap[SelectedItem] = MsgMap;
				}else
				if(LOWORD(wParam) == IDC_TTL){
					MESSAGEMAP MsgMap = MessageMap[SelectedItem];
					CHAR TTLA[32];
					GetWindowText(GetDlgItem(hWnd, IDC_TTL), TTLA, sizeof(TTLA));
					INT TTL = atoi(TTLA);
					MsgMap.TTL = TTL;
					MessageMap[SelectedItem] = MsgMap;
					TTLUpdated = TRUE;
				}else
				if(LOWORD(wParam) == IDC_UUID){
					//EnableWindow(GetDlgItem(hWnd, IDC_EXECLIENTS), FALSE);
					//EnableWindow(GetDlgItem(hWnd, IDC_EXELINKS), FALSE);
					MESSAGEMAP MsgMap = MessageMap[SelectedItem];
					GetWindowText(GetDlgItem(hWnd, IDC_UUID), MsgMap.SendTo, sizeof(MsgMap.SendTo));
					if(strlen(MsgMap.SendTo) > 0){
						EnableWindow(GetDlgItem(hWnd, IDC_EXELINKS), FALSE);
						EnableWindow(GetDlgItem(hWnd, IDC_EXECLIENTS), FALSE);
					}else{
						EnableWindow(GetDlgItem(hWnd, IDC_EXELINKS), TRUE);
						EnableWindow(GetDlgItem(hWnd, IDC_EXECLIENTS), TRUE);
					}
					MessageMap[SelectedItem] = MsgMap;
				}
				MESSAGEMAP MsgMap = MessageMap[SelectedItem];
				LVFINDINFO FindInfo;
				FindInfo.lParam = SelectedItem;
				FindInfo.flags = LVFI_PARAM;
				INT Index = -1;
				if((Index = ListView_FindItem(GetDlgItem(hWnd, IDC_LIST), -1, &FindInfo)) != -1){
					CHAR Test[50];
					LVITEM LVItem;
					LVItem.iItem = Index;
					LVItem.iSubItem = 0;
					LVItem.mask = LVIF_TEXT;
					LVItem.pszText = Test;
					LVItem.cchTextMax = 50;
					ListView_GetItem(GetDlgItem(hWnd, IDC_LIST), &LVItem);
				
					GetWindowText(GetDlgItem(hWnd, IDC_SCRIPTNAME), Test, sizeof(Test));
					UpdateListBox = FALSE;
					ListView_SetItem(GetDlgItem(hWnd, IDC_LIST), &LVItem);
					LVItem.iSubItem = 8;
					strncpy(Test, MsgMap.Script, sizeof(Test));
					ListView_SetItem(GetDlgItem(hWnd, IDC_LIST), &LVItem);

					if(TTLUpdated){
						LVItem.iSubItem = 6;
						itoa(MsgMap.TTL, Test, 10);
						ListView_SetItem(GetDlgItem(hWnd, IDC_LIST), &LVItem);
					}

					GetWindowText(GetDlgItem(hWnd, IDC_UUID), Test, sizeof(Test));
					if(strlen(Test) > 0){
						LVItem.iSubItem = 4;
						strcpy(Test, "*");
						ListView_SetItem(GetDlgItem(hWnd, IDC_LIST), &LVItem);
					}else{
						LVItem.iSubItem = 4;
						strcpy(Test, "-");
						ListView_SetItem(GetDlgItem(hWnd, IDC_LIST), &LVItem);
					}
					MsgMap.State = MESSAGEMAP::WAITING;
					if(MsgMap.SignThread){
						HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, MsgMap.SignThread->GetThreadID());
						TerminateThread(hThread, 0);
						MsgMap.SignThread = NULL;
					}
					LVItem.iSubItem = 1;
					LVItem.pszText = "Waiting";
					ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
					MessageMap[SelectedItem] = MsgMap;
					UpdateListBox = TRUE;
				}
				Mutex.Release();
			}
		}else
		if(HIWORD(wParam) == CBN_DROPDOWN){
			if(LOWORD(wParam) == IDC_COMBO){
				HWND hWndCombo = GetDlgItem(hWnd, IDC_COMBO);
				RECT Rect;
				GetWindowRect(hWndCombo, &Rect);
				INT Width = Rect.right - Rect.left;
				POINT P;
				P.x = Rect.left;
				P.y = Rect.top;
				ScreenToClient(hWnd, &P);
				INT Height = SendMessage(hWndCombo, CB_GETITEMHEIGHT, 0, NULL) * (3 + 2);
				MoveWindow(hWndCombo, P.x, P.y, Width, Height, TRUE);
			}
		}else
		if(HIWORD(wParam) == CBN_SELCHANGE){
			INT Selection = SendMessage(GetDlgItem(hWnd, IDC_COMBO), CB_GETCURSEL, NULL, NULL);
			if(Selection == 0){
				EnableWindow(GetDlgItem(hWnd, IDC_TTL), FALSE);
			}else{
				EnableWindow(GetDlgItem(hWnd, IDC_TTL), TRUE);
			}
			Mutex.WaitForAccess();
			MESSAGEMAP MsgMap = MessageMap[SelectedItem];
			LVFINDINFO FindInfo;
			FindInfo.lParam = SelectedItem;
			FindInfo.flags = LVFI_PARAM;
			INT Index = -1;
			if((Index = ListView_FindItem(GetDlgItem(hWnd, IDC_LIST), -1, &FindInfo)) != -1){
				CHAR Test[50];
				LVITEM LVItem;
				LVItem.iItem = Index;
				LVItem.iSubItem = 5;
				LVItem.mask = LVIF_TEXT;
				LVItem.pszText = Test;
				LVItem.cchTextMax = sizeof(Test);
			
				UpdateListBox = FALSE;
				if(Selection == 0 || Selection == 2){
					strcpy(Test, "*");
					MsgMap.RunLocally = TRUE;
				}else{
					strcpy(Test, "-");
					MsgMap.RunLocally = FALSE;
				}
				ListView_SetItem(GetDlgItem(hWnd, IDC_LIST), &LVItem);
				if(Selection == 0){
					strcpy(Test, "-");
					MsgMap.Broadcast = FALSE;
				}else{
					itoa(MsgMap.TTL, Test, 10);
					MsgMap.Broadcast = TRUE;
				}
				LVItem.iSubItem = 6;
				ListView_SetItem(GetDlgItem(hWnd, IDC_LIST), &LVItem);
				MsgMap.State = MESSAGEMAP::WAITING;
				if(MsgMap.SignThread){
					HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, MsgMap.SignThread->GetThreadID());
					TerminateThread(hThread, 0);
					MsgMap.SignThread = NULL;
				}
				LVItem.iSubItem = 1;
				LVItem.pszText = "Waiting";
				ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
				UpdateListBox = TRUE;
				MessageMap[SelectedItem] = MsgMap;
			}
			Mutex.Release();
		}
	}else
	if(Msg == WM_NOTIFY){
		if(wParam == IDC_LIST){
			LPNMITEMACTIVATE Item = (LPNMITEMACTIVATE)lParam;
			if(Item->hdr.code == LVN_ITEMCHANGED){
				if(UpdateListBox && MessageMap.size() > 0){
					LVITEM LVItem;
					LVItem.iItem = Item->iItem;
					LVItem.mask = LVIF_PARAM;
					if(ListView_GetItem(GetDlgItem(hWnd, IDC_LIST), &LVItem)){
						LVItem.mask = LVIF_TEXT | LVIF_PARAM;
						Mutex.WaitForAccess();
						MESSAGEMAP MsgMap = MessageMap[LVItem.lParam];
						SelectedItem = LVItem.lParam;
						Mutex.Release();
						UpdateEditBoxes = FALSE;
						SetWindowText(GetDlgItem(hWnd, IDC_SCRIPTNAME), MsgMap.Name);
						SetWindowText(GetDlgItem(hWnd, IDC_SCRIPT), MsgMap.Script);
						CHAR MID[32];
						sprintf(MID, "%X", MsgMap.MID);
						CHAR TTL[32];
						SetWindowText(GetDlgItem(hWnd, IDC_MID), MID);
						SetWindowText(GetDlgItem(hWnd, IDC_TTL), itoa(MsgMap.TTL, TTL, 10));
						INT SetUUID = 0;
						if(strcmp(MsgMap.SendTo, "2") == 0 || strcmp(MsgMap.SendTo, "3") == 0){
							CheckDlgButton(hWnd, IDC_EXELINKS, BST_CHECKED);
						}else{
							CheckDlgButton(hWnd, IDC_EXELINKS, BST_UNCHECKED);
							SetUUID++;
						}
						if(strcmp(MsgMap.SendTo, "1") == 0 || strcmp(MsgMap.SendTo, "3") == 0){
							CheckDlgButton(hWnd, IDC_EXECLIENTS, BST_CHECKED);
						}else{
							CheckDlgButton(hWnd, IDC_EXECLIENTS, BST_UNCHECKED);
							SetUUID++;
						}
						if(SetUUID == 2){
							if(strcmp(MsgMap.SendTo, "0") == 0){
								SetWindowText(GetDlgItem(hWnd, IDC_UUID), "");
							}else{
								SetWindowText(GetDlgItem(hWnd, IDC_UUID), MsgMap.SendTo);
							}
							EnableWindow(GetDlgItem(hWnd, IDC_UUID), TRUE);
						}else{
							SetWindowText(GetDlgItem(hWnd, IDC_UUID), "");
							EnableWindow(GetDlgItem(hWnd, IDC_UUID), FALSE);
						}
						if(MsgMap.RunLocally && !MsgMap.Broadcast){
							SendMessage(GetDlgItem(hWnd, IDC_COMBO), CB_SETCURSEL, 0, NULL);
							EnableWindow(GetDlgItem(hWnd, IDC_TTL), FALSE);
						}else
						if(!MsgMap.RunLocally && MsgMap.Broadcast){
							SendMessage(GetDlgItem(hWnd, IDC_COMBO), CB_SETCURSEL, 1, NULL);
							EnableWindow(GetDlgItem(hWnd, IDC_TTL), TRUE);
						}else
						if(MsgMap.RunLocally && MsgMap.Broadcast){
							SendMessage(GetDlgItem(hWnd, IDC_COMBO), CB_SETCURSEL, 2, NULL);
							EnableWindow(GetDlgItem(hWnd, IDC_TTL), TRUE);
						}
					
						UpdateEditBoxes = TRUE;
					}
				}
			}else
			if(Item->hdr.code == NM_RCLICK){
				LVITEM LVItem;
				LVItem.iItem = Item->iItem;
				LVItem.mask = LVIF_PARAM;
				if(ListView_GetItem(GetDlgItem(hWnd, IDC_LIST), &LVItem)){
					LVItem.mask = LVIF_TEXT;
					Mutex.WaitForAccess();
					MESSAGEMAP MsgMap = MessageMap[LVItem.lParam];
					Mutex.Release();
					HMENU hMenu = CreatePopupMenu();
					UINT Flags = MF_BYPOSITION|MF_STRING;
					InsertMenu(hMenu, 0, Flags|(MsgMap.State==MESSAGEMAP::WAITING||MsgMap.State==MESSAGEMAP::SENT?0:MF_GRAYED), 1, (MsgMap.State==MESSAGEMAP::SENT ? "New MID" : "Sign"));
					InsertMenu(hMenu, 1, Flags|(MsgMap.State==MESSAGEMAP::READY||MsgMap.State==MESSAGEMAP::SENT?0:MF_GRAYED), 2, (MsgMap.State==MESSAGEMAP::SENT ? "Resend" : "Send"));
					InsertMenu(hMenu, 2, Flags|(MsgMap.State==MESSAGEMAP::SIGNING?0:MF_GRAYED), 3, "Cancel");
					InsertMenu(hMenu, 3, Flags, 4, "Remove");
					POINT P;
					GetCursorPos(&P);
					INT Return = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RETURNCMD, P.x, P.y, 0, hWnd, NULL);
					if(Return > 0){
						if(Return == 1){
							if(MsgMap.State == MESSAGEMAP::SENT){
								LVItem.iSubItem = 1;
								LVItem.pszText = "Waiting";
								ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
								Mutex.WaitForAccess();
								MsgMap.State = MESSAGEMAP::WAITING;
								MsgMap.MID = GenerateNewMID();
								LVItem.iSubItem = 7;
								CHAR CMID[32];
								sprintf(CMID, "%X", MsgMap.MID);
								LVItem.pszText = CMID;
								ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
								SetWindowText(GetDlgItem(hWnd, IDC_MID), CMID);
								MessageMap[LVItem.lParam] = MsgMap;
								Mutex.Release();
							}else{
								LVItem.iSubItem = 1;
								LVItem.pszText = "Signing";
								ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
								Mutex.WaitForAccess();
								MsgMap.State = MESSAGEMAP::SIGNING;
								MsgMap.SignThread = new MessageSignThread(LVItem.lParam);
								MessageMap[LVItem.lParam] = MsgMap;
								Mutex.Release();
							}
						}else
						if(Return == 2){
							//CHAR Temp[RECVBUF_SIZE];
							//Temp[0] = NULL;
							//if(MsgMap.Ordinal != 0)
							//	sprintf(Temp, "%d,", MsgMap.Ordinal);
							//strcat(Temp, MsgMap.Params);
							Connection.SendMsg(MsgMap.MID, MsgMap.Script, MsgMap.SendTo, MsgMap.Broadcast ? MsgMap.TTL : 0, MsgMap.Signature, MsgMap.RunLocally);
							Mutex.WaitForAccess();
							MsgMap.State = MESSAGEMAP::SENT;
							LVItem.iSubItem = 1;
							LVItem.pszText = "Sent";
							ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
							MessageMap[LVItem.lParam] = MsgMap;
							Mutex.Release();
						}else
						if(Return == 3 || Return == 4){
							Mutex.WaitForAccess();
							MsgMap = MessageMap[LVItem.lParam];
							if(MsgMap.SignThread){
								HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, MsgMap.SignThread->GetThreadID());
								TerminateThread(hThread, 0);
								MsgMap.SignThread = NULL;
								MessageMap[LVItem.lParam] = MsgMap;
							}
							if(Return == 4){
								ListView_DeleteItem(GetDlgItem(hWnd, IDC_LIST), Item->iItem);
								MessageMap.erase(LVItem.lParam);
								if(MessageMap.size() == 0){
									DisableOptions(hWnd);
								}
							}else{
								MsgMap.State = MESSAGEMAP::WAITING;
								LVItem.iSubItem = 1;
								LVItem.pszText = "Waiting";
								ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
								MessageMap[LVItem.lParam] = MsgMap;
							}
							Mutex.Release();
						}
					}
				}
			}
		}
	}else
	if(Msg == WM_EXITSIZEMOVE){
		cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
		RECT Rect;
		GetWindowRect(hWnd, &Rect);
		Reg.SetInt(REG_MSGQUEUEX, Rect.left);
		Reg.SetInt(REG_MSGQUEUEY, Rect.top);
		Reg.SetInt(REG_MSGQUEUEH, Rect.bottom - Rect.top);
	}else
	if(Msg == WM_NCCALCSIZE){
		if(wParam == TRUE){
			LPNCCALCSIZE_PARAMS P = (LPNCCALCSIZE_PARAMS)lParam;
			INT WC = ((P->rgrc[0].right - P->rgrc[0].left) - (P->rgrc[1].right - P->rgrc[1].left));
			INT HC = ((P->rgrc[0].bottom - P->rgrc[0].top) - (P->rgrc[1].bottom - P->rgrc[1].top));
			for(UINT i = 0; i <= 20; i++){
				INT Item;
				RECT Rect;
				POINT Points;
				switch(i){
					case 0: Item = IDC_STATICUUID; break;
					case 1: Item = IDC_STATICADD; break;
					case 2: Item = IDC_STATICTTL; break;
					case 3: Item = IDC_STATICEXE; break;
					case 4: Item = IDC_STATICMSG; break;
					case 5: Item = IDC_UUID; break;
					case 6: Item = IDC_COMBO; break;
					case 7: Item = IDC_EXELINKS; break;
					case 8: Item = IDC_TTL; break;
					case 9: Item = IDC_ADD; break;
					case 10: Item = IDC_EXECLIENTS; break;
					case 11: Item = IDC_STATICSCRIPTNAME; break;
					case 12: Item = IDC_STATICMSGID; break;
					case 13: Item = IDC_NEWMID; break;
					case 14: Item = IDC_SCRIPT; break;
					case 15: Item = IDC_ADDNEW; break;
					case 16: Item = IDC_SCRIPTNAME; break;
					case 17: Item = IDC_MID; break;
					case 18: Item = IDC_STATIC_BARTOP; break;
					case 19: Item = IDC_STATIC_BARBOTTOM; break;
					case 20: Item = IDC_STATICBG; break;
				}
				GetWindowRect(GetDlgItem(hWnd, Item), &Rect);
				Points.x = Rect.left;
				Points.y = Rect.top;
				ScreenToClient(hWnd, &Points);
				SetWindowPos(GetDlgItem(hWnd, Item), NULL, Points.x + WC, Points.y + HC, NULL, NULL, SWP_NOSIZE);
			}
			RECT Rect;
			GetWindowRect(GetDlgItem(hWnd, IDC_LIST), &Rect);
			SetWindowPos(GetDlgItem(hWnd, IDC_LIST), NULL, NULL, NULL, Rect.right - Rect.left, Rect.bottom - Rect.top + HC, SWP_NOMOVE);
			InvalidateRect(hWnd, NULL, FALSE);
		}
	}else
	if(Msg == WM_GETMINMAXINFO){
		LPMINMAXINFO M = (LPMINMAXINFO)lParam;
		RECT Rect, Rect2;
		GetWindowRect(hWnd, &Rect);
		GetWindowRect(GetDlgItem(hWnd, IDC_SCRIPT), &Rect2);
		M->ptMinTrackSize.y = 166 + (Rect2.bottom - Rect2.top);
		M->ptMinTrackSize.x = Rect.right - Rect.left;
		M->ptMaxTrackSize.x = M->ptMinTrackSize.x;
	}else
	if(Msg == WM_CLOSE){
		ShowWindow(hWnd, SW_HIDE);
	}else
	if(Msg == WM_MOUSEMOVE){
		RECT RectT, RectB;
		GetWindowRect(GetDlgItem(hWnd, IDC_STATIC_BARTOP), &RectT);
		GetWindowRect(GetDlgItem(hWnd, IDC_STATIC_BARBOTTOM), &RectB);
		PointM.x = LOWORD(lParam);
		PointM.y = HIWORD(lParam);
		ClientToScreen(hWnd, &PointM);

		if((PointM.x <= RectT.right && PointM.x >= RectT.left) && (PointM.y >= RectT.top && PointM.y <= RectT.bottom)){
			ResizingBox = 1;
		}else
		if((PointM.x <= RectB.right && PointM.x >= RectB.left) && (PointM.y >= RectB.top && PointM.y <= RectB.bottom)){
			ResizingBox = 2;
		}else
		if(!(wParam & MK_LBUTTON)){
			ResizingBox = NULL;
		}
		if(ResizingBox){
			SetCursor(LoadCursor(NULL, IDC_SIZENS));
		}
		if(ResizingBox){
			if(wParam & MK_LBUTTON){
				POINT Point;
				INT Difference = (PointM.y - OldCursorPos.y);
				if(Difference > 3000)
					Difference = 0;
				
				ResizeMiddle(hWnd, Difference);
			}
		}
		OldCursorPos = PointM;
	}else
	if(Msg == WM_LBUTTONDOWN){
		if(ResizingBox){
			SetCapture(hWnd);
		}
	}else
	if(Msg == WM_LBUTTONUP){
		if(ResizingBox){
			ResizingBox = NULL;
			ReleaseCapture();
		}
	}else
	if(Msg == WM_DESTROY){
		CHAR Key[256];
		sprintf(Key, "%s\\%s", REG_ROOT, REG_MSGQUEUEITEMS);
		cRegistry Reg(HKEY_CURRENT_USER, Key);
		CHAR Key2[32];
		DWORD Size = sizeof(Key2);
		LONG Return = Reg.EnumKey(0, Key2, &Size);
		while(Return == ERROR_SUCCESS || Return == ERROR_MORE_DATA){
			CHAR KeyTemp[256];
			sprintf(KeyTemp, "%s\\%s", Key, Key2);
			RegDeleteKey(HKEY_CURRENT_USER, KeyTemp);
			Size = sizeof(Key2);
			Return = Reg.EnumKey(0, Key2, &Size);
		}
		if(MessageMap.size() > 0){
			INT i = 0;
			for(std::map<LONG, MESSAGEMAP>::iterator I = MessageMap.begin(); I != MessageMap.end(); I++, i++){
				MESSAGEMAP MsgMap = I->second;
				CHAR SubKey[256];
				sprintf(SubKey, "%s\\%s\\%d", REG_ROOT, REG_MSGQUEUEITEMS, i);
				cRegistry Reg(HKEY_CURRENT_USER, SubKey);
				Reg.SetString(REG_MSGQUEUEITEMSCRIPTNAME, MsgMap.Name);
				Reg.SetString(REG_MSGQUEUEITEMSCRIPT, MsgMap.Script);
				Reg.SetString(REG_MSGQUEUEITEMSENDTO, MsgMap.SendTo);
				Reg.SetInt(REG_MSGQUEUEITEMTTL, MsgMap.TTL);
				Reg.SetInt(REG_MSGQUEUEITEMRUNLOCALLY, MsgMap.RunLocally);
				Reg.SetInt(REG_MSGQUEUEITEMBROADCAST, MsgMap.Broadcast);
			}
		}
	}
	return 0;
}

MessageSignThread::MessageSignThread(LONG ID){
	MessageSignThread::ID = ID;
	StartThread();
};

VOID MessageSignThread::ThreadFunc(VOID){
	SetPriority(THREAD_PRIORITY_BELOW_NORMAL);
	Mutex.WaitForAccess();
	MESSAGEMAP MsgMap = MessageMap[ID];
	Mutex.Release();
	UCHAR Hash[16];
	Password::RetreiveHash(Hash);
	R_RSA_PRIVATE_KEY RSAPrivateKey;
	RSA::RetrievePrivateKeyMaster(Hash, &RSAPrivateKey);
	CHAR Temp[32];
	UCHAR SignHash[16];
	UCHAR Null = 0;
	MD5 MD5;

	MD5.Update((PUCHAR)MsgMap.Script, strlen(MsgMap.Script));
	MD5.Update(&Null, sizeof(Null));
	sprintf(Temp, "%X", MsgMap.MID);
	MD5.Update((PUCHAR)Temp, strlen(Temp));
	MD5.Update((PUCHAR)MsgMap.SendTo, strlen(MsgMap.SendTo));
	MD5.Finalize(SignHash);

	memset(MsgMap.Signature, NULL, sizeof(MsgMap.Signature));

	UINT OutputLen;
	RSAPrivateEncrypt((PUCHAR)MsgMap.Signature, &OutputLen, SignHash, sizeof(SignHash), &RSAPrivateKey);

	LVFINDINFO LVFind;
	LVFind.flags = LVFI_PARAM;
	LVFind.lParam = ID;
	INT Item = ListView_FindItem(GetDlgItem(WindowActive, IDC_LIST), -1, &LVFind);
	if(Item > -1){
		LVITEM LVItem;
		LVItem.mask = LVIF_TEXT;
		LVItem.iItem = Item;
		LVItem.iSubItem = 1;
		LVItem.pszText = "Ready";
		ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
		MsgMap.State = MESSAGEMAP::READY;
	}
	MsgMap.SignThread = NULL;
	Mutex.WaitForAccess();
	MessageMap[ID] = MsgMap;
	Mutex.Release();
}

BOOL IsActive(VOID){
	if(!WindowActive)
		return FALSE;
	return IsWindowVisible(WindowActive);
}

}