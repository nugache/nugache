#define WIN32_LEAN_AND_MEAN
#define _WIN32_IE 0x0500
#define WM_SYSTRAYNOTIFY (WM_USER + 1)
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <winsock2.h>
#include <vector>
#include "p2p2.h"
#include "config.h"
#include "crypt.h"
#include "debug.h"
#include "connections.h"
#include "preferences.h"
#include "logdialog.h"
#include "messagedialog.h"
#include "sendupdate.h"
#include "registry.h"
#include "Resource.h"
#include "display.h"

HINSTANCE hInst;
HWND hWndg;//, hWndTooltip;
HWND hWndPrefs = NULL;
POINT pan, mouse;
BOOL WindowActive = TRUE;
BOOL SendToLinks = TRUE, SendToClients = TRUE;
Links Links;
Link* SelectedLink = NULL;
Link* ConnectLink = NULL;
UINT CurrentLink = 0;
ULONG ResolvedAddr;
USHORT ResolvedPort;
BOOL DNSActive = FALSE;
extern HTREEITEM AboutTreeItem;
extern Display Display;
extern HWND ListLinksWindow;

Connection Connection;

ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

BOOL CALLBACK WINAPI SetPasswordProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam){
	if(Msg == WM_INITDIALOG){
		SetFocus(GetDlgItem(hWnd, IDC_PASSWORD1));
	}else
	if(Msg == WM_COMMAND){
		if(HIWORD(wParam) == BN_CLICKED){
			if(LOWORD(wParam) == IDCANCEL){
				EndDialog(hWnd, FALSE);
			}else
			if(LOWORD(wParam) == IDOK){
				CHAR Password1[129];
				CHAR Password2[129];
				GetWindowText(GetDlgItem(hWnd, IDC_PASSWORD1), Password1, sizeof(Password1));
				GetWindowText(GetDlgItem(hWnd, IDC_PASSWORD2), Password2, sizeof(Password2));

				if(strcmp(Password1, Password2) != 0){
					MessageBox(NULL, "Passwords do not match", "Error", MB_OK|MB_ICONERROR);
				}else
				if(strlen(Password1) < 8){
					MessageBox(NULL, "Password must be at least 8 digits", "Error", MB_OK|MB_ICONERROR);
				}else
				if(strlen(Password1) > 128){
					MessageBox(NULL, "Password cannot be larger than 128 digits", "Error", MB_OK|MB_ICONERROR);
				}else{
					MD5 MD5;
					MD5.Update((PUCHAR)Password1, strlen(Password1));
					UCHAR Hash[16];
					MD5.Finalize(Hash);
					Password::SetHash(Hash);
					Password::RememberHash(Hash);
					memset(Password1, 0, sizeof(Password1));
					memset(Password2, 0, sizeof(Password2));
					RSA::DeletePrivateKey();
					EndDialog(hWnd, TRUE);
				}
			}
		}
	}else
	if(Msg == WM_CLOSE){
		EndDialog(hWnd, FALSE);
	}

	return 0;
}

BOOL CALLBACK WINAPI EnterPasswordProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam){
	if(Msg == WM_INITDIALOG){
		SetFocus(GetDlgItem(hWnd, IDC_PASSWORD));
	}else
	if(Msg == WM_COMMAND){
		if(HIWORD(wParam) == BN_CLICKED){
			if(LOWORD(wParam) == IDCANCEL){
				EndDialog(hWnd, FALSE);
			}else
			if(LOWORD(wParam) == IDOK){
				CHAR Password[129];

				GetWindowText(GetDlgItem(hWnd, IDC_PASSWORD), Password, sizeof(Password));
				MD5 MD5;
				MD5.Update((PUCHAR)Password, strlen(Password));
				UCHAR Hash[16];
				MD5.Finalize(Hash);
				Password::RememberHash(Hash);
				memset(Password, 0, sizeof(Password));
				if(!Password::TestHash(Hash)){
					EndDialog(hWnd, FALSE);
				}else{
					EndDialog(hWnd, TRUE);
				}
			}
		}
	}else
	if(Msg == WM_CLOSE){
		EndDialog(hWnd, FALSE);
	}

	return 0;
}

VOID Connect2(Link* Link){
	Connection.WaitForAccess();
	PCHAR Address = inet_ntoa(SocketFunction::Stoin(ResolvedAddr));
	Connection.SetLink(Link);
	Connection.GetLink()->SetHostname(Address);
	Connection.GetLink()->SetRemotePort(ResolvedPort);
	CHAR Text[256];
	sprintf(Text, "Connecting to %s:%d...", Address, ResolvedPort);
	Display.PrintStatus(Text);
	Connection.Connect(Address, ResolvedPort);
	Connection.Release();
}

VOID Connect(PCHAR Address, USHORT Port, Link* Link){
	ResolvedPort = Port;
	if((ResolvedAddr = inet_addr(Address)) == INADDR_NONE){
		CHAR Text[256];
		sprintf(Text, "Looking up %s...", Address);
		Display.PrintStatus(Text);
		DNSActive = TRUE;
		ConnectLink = Link;
		new AsyncDNS(GetCurrentThreadId(), Address, &ResolvedAddr);
	}else{
		Connect2(Link);
	}

}

VOID InsertRecentLink(PCHAR Address){
	cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
	while(Reg.GetMultiStringElements(REG_RECENT_LINKS) >= 10){
		Reg.RemoveMultiString(REG_RECENT_LINKS, 0);
	}
	Reg.InsertMultiString(REG_RECENT_LINKS, Address);
}

BOOL CALLBACK WINAPI EnterAddressProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam){
	if(Msg == WM_INITDIALOG){
		SetFocus(GetDlgItem(hWnd, IDC_ADDRESS));
	}else
	if(Msg == WM_COMMAND){
		if(HIWORD(wParam) == BN_CLICKED){
			if(LOWORD(wParam) == IDCANCEL){
				EndDialog(hWnd, FALSE);
			}else
			if(LOWORD(wParam) == IDOK){
				CHAR Address[256];
				GetWindowText(GetDlgItem(hWnd, IDC_ADDRESS), Address, sizeof(Address));
				CHAR Hostname[256];
				USHORT Port;
				LinkCache::DecodeName(Address, Hostname, sizeof(Hostname), &Port);
				Links.ClearAll();
				Connect(Hostname, Port, Links.GetHead());
				InsertRecentLink(Address);
				EndDialog(hWnd, TRUE);
			}
		}
	}else
	if(Msg == WM_CLOSE){
		EndDialog(hWnd, FALSE);
	}

	return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	#ifdef _DEBUG
	init_debug(80,20);
	dprintf("SNU3 DEBUG\r\n");
	#endif
	MSG msg;
	PeekMessage(&msg, hWndg, NULL, NULL, PM_NOREMOVE);
	HACCEL hAccelTable;

	if(!Password::Exists()){
		MessageBox(NULL, "A password has not been set\r\nYou will be able to set one when you click OK\r\n\r\nMake sure your password is nice and long and something\r\nnot easy to guess, such as random letters and numbers", "No password set", MB_OK|MB_ICONINFORMATION|MB_TOPMOST);
		if(DialogBox(hInstance, MAKEINTRESOURCE(IDD_SETPASSWORD), NULL, SetPasswordProc)){

		}else{
			return FALSE;
		}
	}else{
		if(DialogBox(hInstance, MAKEINTRESOURCE(IDD_ENTERPASSWORD), NULL, EnterPasswordProc)){

		}else{
			return FALSE;
		}
	}

	MyRegisterClass(hInstance);

	if (!InitInstance(hInstance, nCmdShow)) 
	{
		return FALSE;
	}

	new P2P2(FALSE);

	if(RSA::PrivateKeyStored()){
		UCHAR Hash[16];
		Password::RetreiveHash(Hash);
		RSA::RetrievePrivateKey(Hash, &RSAPrivateKey);
		RSA::MakePublicKey(&RSAPublicKey, RSAPrivateKey);
	}

	InitCommonControlsEx(NULL);

	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDR_ACCELERATOR);
	DWORD Time = GetTickCount();

	SetTimer(hWndg, 1, 20, NULL);

	while(GetMessage(&msg, NULL, 0, 0) > 0){
		if(!IsDialogMessage(hWndg, &msg)){
			if(msg.message == WM_USER_GETHOSTBYNAME){
				if(DNSActive){
					Connect2(ConnectLink);
					DNSActive = FALSE;
				}
			}
			if(!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	HBRUSH hBrush = CreateSolidBrush(RGB(0,0,0));

	wcex.style			= CS_SAVEBITS;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_SNU3);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= hBrush;
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDR_MENU1);
	wcex.lpszClassName	= "main";
	wcex.hIconSm		= NULL;

	return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;

	hInst = hInstance;

	RECT Rect;
	GetWindowRect(GetDesktopWindow(), &Rect);
	INT MainX, MainY, MainW, MainH;
	cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
	if(!Reg.Exists(REG_MAINX) || !Reg.Exists(REG_MAINY) || !Reg.Exists(REG_MAINW) || !Reg.Exists(REG_MAINH) || Reg.GetInt(REG_MAINW) <= 0 || Reg.GetInt(REG_MAINH) <= 0){
		RECT Adjusted;
		Adjusted.bottom = 480;
		Adjusted.right = 640;
		Adjusted.top = 0;
		Adjusted.left = 0;
		AdjustWindowRect(&Adjusted, WS_OVERLAPPEDWINDOW, TRUE);
		MainX = (Rect.right/2) - ((Adjusted.right - Adjusted.left)/2);
		MainY = (Rect.bottom/2) - ((Adjusted.bottom - Adjusted.top)/2);
		MainW = (Adjusted.right - Adjusted.left);
		MainH = (Adjusted.bottom - Adjusted.top);
	}else{
		MainX = Reg.GetInt(REG_MAINX);
		MainY = Reg.GetInt(REG_MAINY);
		MainW = Reg.GetInt(REG_MAINW);
		MainH = Reg.GetInt(REG_MAINH);
	}
	hWnd = CreateWindow("main", "SNU 3", WS_OVERLAPPEDWINDOW, MainX, MainY, MainW, MainH, 0, 0, hInstance, NULL);
	SetWindowPos(hWnd, HWND_BOTTOM, NULL, NULL, NULL, NULL, SWP_NOSIZE | SWP_NOMOVE);
	hWndg = hWnd;

	if(Display.Initialize(hWndg) == E_FAIL){
		MessageBox(NULL, "Display failed to initialize", "Error", MB_OK);
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);

	if(Reg.Exists(REG_LOGHARVESTA) && Reg.GetInt(REG_LOGHARVESTA))
		ShowWindow(CreateDialog(hInst, MAKEINTRESOURCE(IDD_LOGHARVESTER), hWndg, (DLGPROC)logdialog::LogProc), SW_SHOW);
	
	if(Reg.Exists(REG_MSGQUEUEA) && Reg.GetInt(REG_MSGQUEUEA))
		ShowWindow(CreateDialog(hInst, MAKEINTRESOURCE(IDD_MESSAGEQUEUE), hWndg, (DLGPROC)messagequeue::MessageQueueProc), SW_SHOW);
	//hWndStatus = CreateWindow(STATUSCLASSNAME, "Ready", WS_CHILD, 0, 0, 0, 0, hWnd, NULL, hInstance, NULL);
	//ShowWindow(hWndStatus, SW_SHOW);

	/*hWndTooltip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, 0, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, 0, 0, 0, 0, hWndg, NULL, hInstance, NULL);

    SetWindowPos(hWndTooltip,
        HWND_TOPMOST,
        0,
        0,
        0,
        0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

	TOOLINFO TI;
	TI.cbSize = sizeof(TOOLINFO);
	TI.uFlags = TTF_SUBCLASS;
	TI.hwnd = hWndg;
	TI.uId = 0;
	TI.rect.top = TI.rect.left = 0;
	TI.rect.bottom = TI.rect.right = 500;
	TI.hinst = hInstance;
	TI.lpszText = NULL;
	SendMessage(hWndTooltip, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &TI);
	SendMessage(hWndTooltip, TTM_ACTIVATE, TRUE, 0);*/

	pan.x = 0;
	pan.y = 0;

	WSADATA wsaData;
	WSAStartup(MAKEWORD( 1, 1 ), &wsaData);

	if (!hWnd)
	{
		return FALSE;
	}

	UpdateWindow(hWnd);

	Display.PrintStatus("SNU 3");

	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	HWND hWndDlg = NULL;
	LinkCache::Link RandomLink;

	Display.CheckMessage(message, wParam, lParam);

	switch (message) 
	{
	case WM_RBUTTONUP:
		HMENU hMenu, hSubMenu;
		RECT Rect;
		hMenu = CreatePopupMenu();
		hSubMenu = CreatePopupMenu();
		InsertMenu(hSubMenu, 0, MF_BYPOSITION|MF_STRING, 7, "Refresh Info");
		InsertMenu(hSubMenu, 1, MF_BYPOSITION|MF_STRING, 8, "List Links");
		if(SelectedLink){
			if(Connection.GetLink() == SelectedLink && Connection.Active() && Connection.Ready())
				InsertMenu(hMenu, 0, MF_BYPOSITION|MF_STRING|MF_POPUP, (UINT)hSubMenu, "Commands");
			else
				InsertMenu(hMenu, 0, MF_BYPOSITION|MF_STRING, 1, "Connect");
			InsertMenu(hMenu, 1, MF_BYPOSITION|MF_SEPARATOR, NULL, NULL);
			CHAR Encoded[256];
			LinkCache::EncodeName(Encoded, sizeof(Encoded), SelectedLink->GetHostname(), SelectedLink->GetPort());
			if(!LinkCache::LinkExists(Encoded))
				InsertMenu(hMenu, 2, MF_BYPOSITION|MF_STRING, 2, "Add to list");
			else
				InsertMenu(hMenu, 3, MF_BYPOSITION|MF_STRING, 3, "Remove from list");
			InsertMenu(hMenu, 4, MF_BYPOSITION|MF_STRING, 4, "Copy to clipboard");
		}
		GetWindowRect(hWnd, &Rect);
		INT Return;
		Return = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RETURNCMD,
		LOWORD(lParam) + Rect.left + (2*GetSystemMetrics(SM_CXEDGE)),
		HIWORD(lParam) + Rect.top + GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYMENU) + (2*GetSystemMetrics(SM_CYEDGE)),
		0, hWnd, NULL);
		if(Return == 1){
			//Connection.SetLink(SelectedLink);
			//Connection.Connect(SelectedLink->GetHostname());
			//Connect(Host);
			if(SelectedLink)
				Connect(SelectedLink->GetHostname(), SelectedLink->GetRemotePort(), SelectedLink);
		}else
		if(Return == 2){
			CHAR Encoded[256];
			LinkCache::EncodeName(Encoded, sizeof(Encoded), SelectedLink->GetHostname(), SelectedLink->GetPort());
			LinkCache::AddLink(Encoded);
		}else
		if(Return == 3){
			CHAR Encoded[256];
			LinkCache::EncodeName(Encoded, sizeof(Encoded), SelectedLink->GetHostname(), SelectedLink->GetPort());
			LinkCache::RemoveLink(Encoded);
		}else
		if(Return == 4){
			Clipboard Clipboard;
			Clipboard.Empty();
			Clipboard.SetData(CF_TEXT, (PBYTE)SelectedLink->GetHostname(), strlen(SelectedLink->GetHostname()) + 1);
		}else
		if(Return == 7){
			Connection.GetInfo();
		}else
		if(Return == 8){
			if(ListLinksWindow)
				SendMessage(GetDlgItem(ListLinksWindow, IDC_LIST), LB_RESETCONTENT, NULL, wParam);
			Connection.ListLinks();
		}
		break;
	case WM_INITMENUPOPUP:
		//dprintf("%d %X %d\r\n", HIWORD(lParam), wParam, GetMenuItemID((HMENU)wParam, 0));
		if(GetMenuItemID((HMENU)wParam, 1) == IDM_FILE_DISCONNECT){
			MENUITEMINFO mInfo;
			memset(&mInfo, NULL, sizeof(mInfo));
			mInfo.cbSize = sizeof(mInfo);
			mInfo.fMask = MIIM_STATE;
			if(Connection.Active() || DNSActive)
				mInfo.fState = MFS_ENABLED;
			else
				mInfo.fState = MFS_DISABLED;
			SetMenuItemInfo(HMENU(wParam), IDM_FILE_DISCONNECT, FALSE, &mInfo);
		}else
		if(GetMenuItemID((HMENU)wParam, 0) == IDM_CONNECT_ADDRESS){
			MENUITEMINFO mInfo;
			memset(&mInfo, NULL, sizeof(mInfo));
			mInfo.cbSize = sizeof(mInfo);
			mInfo.fMask = MIIM_STATE;
			if(LinkCache::GetLinkCount() > 0)
				mInfo.fState = MFS_ENABLED;
			else
				mInfo.fState = MFS_DISABLED;
			SetMenuItemInfo(HMENU(wParam), IDM_CONNECT_RANDOMLINK, FALSE, &mInfo);
			cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
			while(DeleteMenu((HMENU)(wParam), 2, MF_BYPOSITION));
			if(Reg.GetMultiStringElements(REG_RECENT_LINKS) > 0){
				mInfo.fMask = MIIM_TYPE;
				mInfo.fType = MFT_SEPARATOR;
				InsertMenuItem((HMENU)(wParam), 2, TRUE, &mInfo);
				mInfo.fMask = MIIM_STRING | MIIM_ID;
				mInfo.fType = MFT_STRING;
				UINT Total = Reg.GetMultiStringElements(REG_RECENT_LINKS);
				if(Total > 10)
					Total = 10;
				UINT i = 0;
				for(i = 0; i < Total; i++){
					mInfo.wID = WM_USER + 10 + i;
					mInfo.dwTypeData = Reg.GetMultiString(REG_RECENT_LINKS, Total - (i + 1));
					mInfo.cch = strlen(mInfo.dwTypeData);
					InsertMenuItem((HMENU)(wParam), 3 + i, TRUE, &mInfo);
				}
				mInfo.fMask = MIIM_TYPE;
				mInfo.fType = MFT_SEPARATOR;
				InsertMenuItem((HMENU)(wParam), 3 + i + 1, TRUE, &mInfo);
				mInfo.fMask = MIIM_STRING | MIIM_ID;
				mInfo.fType = MFT_STRING;
				mInfo.wID = IDM_CLEARHISTORY;
				mInfo.dwTypeData = "Clear History";
				mInfo.cch = strlen(mInfo.dwTypeData);
				InsertMenuItem((HMENU)(wParam), 3 + i + 2, TRUE, &mInfo);
			}
		}
		break;
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		if(wmId >= WM_USER + 10 && wmId < WM_USER + 20){
			MENUITEMINFO MenuItemInfo;
			memset(&MenuItemInfo, 0, sizeof(MenuItemInfo));
			MenuItemInfo.cbSize = sizeof(MenuItemInfo);
			MenuItemInfo.fMask = MIIM_SUBMENU;
			GetMenuItemInfo(GetMenu(hWndg), 0, TRUE, &MenuItemInfo);
			GetMenuItemInfo(MenuItemInfo.hSubMenu, 0, TRUE, &MenuItemInfo);;
			MenuItemInfo.fMask = MIIM_STRING;
			CHAR Text[256];
			MenuItemInfo.dwTypeData = Text;
			MenuItemInfo.cch = sizeof(Text);
			GetMenuItemInfo(MenuItemInfo.hSubMenu, 3 + wmId - (WM_USER + 10), TRUE, &MenuItemInfo);
			CHAR Hostname[256];
			USHORT Port;
			LinkCache::DecodeName(MenuItemInfo.dwTypeData, Hostname, sizeof(Hostname), &Port);
			Connect(Hostname, Port, Links.GetHead());
		}
		switch (wmId)
		{
		case IDM_CONNECT_ADDRESS:
			DialogBox(hInst, (LPCTSTR)IDD_ENTERADDRESS, hWnd, (DLGPROC)EnterAddressProc);
			break;
		case IDM_CONNECT_RANDOMLINK:
			Links.ClearAll();
			RandomLink = LinkCache::GetRandomLink();
			Connect(RandomLink.Hostname, RandomLink.Port, Links.GetHead());
			break;
		case IDM_CLEARHISTORY:{
			cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
			Reg.DeleteValue(REG_RECENT_LINKS);}
			break;
		case IDM_FILE_DISCONNECT:
			Connection.Disconnect();
			Display.PrintStatus("Disconnecting");
			Connection.GetLink()->Disconnecting();
			DNSActive = FALSE;
			break;
		case IDM_HELP_ABOUT:
			if(!hWndPrefs)
				hWndPrefs = CreateDialog(hInst, MAKEINTRESOURCE(IDD_PREFERENCES), hWndg, (DLGPROC)PreferencesProc);
			SetActiveWindow(hWndPrefs);
			ShowWindow(hWndPrefs, SW_SHOW);
			TreeView_SelectItem(GetDlgItem(hWndPrefs, IDC_TREE), AboutTreeItem);
			break;
		case IDM_FILE_EXIT:
			DestroyWindow(hWnd);
			break;
		case IDM_TOOLS_PREFERENCES:
			if(hWndPrefs)
				SetActiveWindow(hWndPrefs);
			else
				hWndPrefs = CreateDialog(hInst, MAKEINTRESOURCE(IDD_PREFERENCES), hWndg, (DLGPROC)PreferencesProc);
			ShowWindow(hWndPrefs, SW_SHOW);
			break;
		case IDM_TOOLS_LOGHARVESTER:
			hWndDlg = CreateDialog(hInst, MAKEINTRESOURCE(IDD_LOGHARVESTER), hWndg, (DLGPROC)logdialog::LogProc);
			ShowWindow(hWndDlg, SW_SHOW);
			break;
		case IDM_TOOLS_MESSAGEQUEUE:
			hWndDlg = CreateDialog(hInst, MAKEINTRESOURCE(IDD_MESSAGEQUEUE), hWndg, (DLGPROC)messagequeue::MessageQueueProc);
			ShowWindow(hWndDlg, SW_SHOW);
			break;
		case IDM_TOOLS_SENDUPDATE:
			hWndDlg = CreateDialog(hInst, MAKEINTRESOURCE(IDD_SENDUPDATE), hWndg, (DLGPROC)SendUpdateProc);
			ShowWindow(hWndDlg, SW_SHOW);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_CLOSE:
		{
		cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
		if(logdialog::IsActive())
			Reg.SetInt(REG_LOGHARVESTA, TRUE);
		else
			Reg.SetInt(REG_LOGHARVESTA, FALSE);
		if(messagequeue::IsActive())
			Reg.SetInt(REG_MSGQUEUEA, TRUE);
		else
			Reg.SetInt(REG_MSGQUEUEA, FALSE);
		}
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		NOTIFYICONDATA NotifyIconData;
		NotifyIconData.cbSize = sizeof(NotifyIconData);
		NotifyIconData.hWnd = hWndg;
		NotifyIconData.uID = 1;
		Shell_NotifyIcon(NIM_DELETE, &NotifyIconData);
		PostQuitMessage(0);
		break;
	case WM_EXITSIZEMOVE:
		{
		RECT Rect;
		GetWindowRect(hWnd, &Rect);
		cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
		Reg.SetInt(REG_MAINX, Rect.left);
		Reg.SetInt(REG_MAINY, Rect.top);
		Reg.SetInt(REG_MAINW, Rect.right - Rect.left);
		Reg.SetInt(REG_MAINH, Rect.bottom - Rect.top);
		}
		/*SurfaceWidth = LOWORD(lParam);
		SurfaceHeight = HIWORD(lParam);*/
		/*if(!pan.x){
			pan.x = SurfaceWidth / 2;
			pan.y = SurfaceHeight / 2;
		}*/
		//SendMessage(hWndStatus, WM_SIZE, wParam, lParam);
		//Redraw();


		break;
	case WM_SIZE:
		if(wParam == SIZE_MINIMIZED){
			cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
			if(Reg.GetInt(REG_MINIMIZETOTRAY)){
				Display.SetMinimized(TRUE);
				NOTIFYICONDATA NotifyIconData;
				NotifyIconData.cbSize = sizeof(NotifyIconData);
				NotifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
				NotifyIconData.hIcon = (HICON)LoadImage(hInst, (LPCTSTR)IDI_SNU3, IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
				NotifyIconData.uVersion = NOTIFYICON_VERSION;
				NotifyIconData.hWnd = hWnd;
				NotifyIconData.uCallbackMessage = WM_SYSTRAYNOTIFY;
				NotifyIconData.uID = 1;
				strcpy(NotifyIconData.szTip, "SNU 3");
				Shell_NotifyIcon(NIM_ADD, &NotifyIconData);
				ShowWindow(hWndg, SW_HIDE);
			}
		}else
		if(wParam == SIZE_RESTORED){
			Display.SetMinimized(FALSE);
			NOTIFYICONDATA NotifyIconData;
			NotifyIconData.cbSize = sizeof(NotifyIconData);
			NotifyIconData.hWnd = hWndg;
			NotifyIconData.uID = 1;
			Shell_NotifyIcon(NIM_DELETE, &NotifyIconData);
		}
		break;
	case WM_SYSTRAYNOTIFY:
		if(lParam == WM_LBUTTONDBLCLK){
			ShowWindow(hWndg, SW_SHOW);
			ShowWindow(hWndg, SW_RESTORE);
		}else
		if(lParam == WM_RBUTTONUP){
			hMenu = CreatePopupMenu();
			InsertMenu(hMenu, 0, MF_BYPOSITION|MF_STRING, 1, "Exit");
			SetForegroundWindow(hWnd);
			POINT P;
			GetCursorPos(&P);
			INT Return = TrackPopupMenu(hMenu, TPM_NONOTIFY|TPM_LEFTBUTTON|TPM_VERNEGANIMATION|TPM_RETURNCMD, P.x, P.y, 0, hWnd, NULL);
			if(Return == 1){
				DestroyWindow(hWnd);
			}
			PostMessage(hWnd, WM_NULL, 0, 0);
			DestroyMenu(hMenu);
		}
		break;
	case WM_APP + 1:
		if(!ListLinksWindow)
			CreateDialog(hInst, MAKEINTRESOURCE(IDD_LISTLINKS), hWndg, (DLGPROC)ListLinksProc);
		SendMessage(GetDlgItem(ListLinksWindow, IDC_LIST), LB_ADDSTRING, NULL, wParam);
		if(Connection.GetLink()){
			CHAR Text[256];
			CHAR Encoded[256];
			DWORD Count = SendMessage(GetDlgItem(ListLinksWindow, IDC_LIST), LB_GETCOUNT, NULL, wParam);
			LinkCache::EncodeName(Encoded, sizeof(Encoded), Connection.GetLink()->GetHostname(), Connection.GetLink()->GetPort());
			sprintf(Text, "%d Links on %s\r\n", Count, Encoded);
			SetWindowText(ListLinksWindow, Text);
		}
		break;
	case WM_PAINT:

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}