#include <winsock2.h>
#include "preferences.h"

extern HINSTANCE hInst;
HWND hWndt = NULL;
HWND hWndGK = NULL;
HWND hWndP = NULL;
HTREEITEM LogHarvestTreeItem = NULL;
HTREEITEM AboutTreeItem = NULL;
extern HWND hWndPrefs;
extern Display Display;

struct {
	UINT BytesNeeded;
	UINT Read;
	PCHAR Data;
	UINT Bits;
	DOUBLE Bar;
	GenerateKeyThread *Thread;
} Random;

HTREEITEM TV_AddItem(HWND hWnd, HTREEITEM hTI, PCHAR String, BOOL Children, LPARAM Data){
	TV_INSERTSTRUCT is={hTI,TVI_LAST,{TVIF_PARAM|TVIF_TEXT|TVIF_CHILDREN,0,0,0,String,strlen(String),0,0,Children,Data}};
	return TreeView_InsertItem(hWnd, &is);
}

VOID GenerateKeyThread::ThreadFunc(VOID){
	SetPriority(THREAD_PRIORITY_BELOW_NORMAL);
	R_RANDOM_STRUCT RandomStruct;
	R_RSA_PROTO_KEY RSAProtoKey;
	R_RSA_PRIVATE_KEY RSAPrivateKey;
	R_RSA_PUBLIC_KEY RSAPublicKey;
	RSAPublicKey.bits = Random.Bits;
	RSAProtoKey.bits = Random.Bits;
	RSAProtoKey.useFermat4 = TRUE;
	RSAPrivateKey.bits = Random.Bits;
	R_RandomInit(&RandomStruct);
	PUCHAR block = new UCHAR[Random.BytesNeeded];
	for(UINT i = 0; i < Random.BytesNeeded; i++)
		block[i] = (UCHAR)Random.Data[i];
	R_RandomUpdate(&RandomStruct, block, RandomStruct.bytesNeeded);
	delete[] block;
	R_GeneratePEMKeys(&RSAPublicKey, &RSAPrivateKey, &RSAProtoKey, &RandomStruct);
	UINT Bits = Random.Bits;

	UCHAR Hash[16];
	Password::RetreiveHash(Hash);
	RSA::StorePrivateKey(Hash, RSAPrivateKey);

	SendMessage(hWndGK, (WM_USER + 2), 0, 0);
}

BOOL CALLBACK WINAPI GenerateKeyProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam){
	if(Msg == WM_INITDIALOG){
		if(hWndGK){
			SetActiveWindow(hWndGK);
			DestroyWindow(hWnd);
			return 0;
		}
		hWndGK = hWnd;
		SendMessage(GetDlgItem(hWnd, IDC_BITS), EM_LIMITTEXT, 4, 0);
		SetWindowText(GetDlgItem(hWnd, IDC_BITS), "1024");
		SetWindowText(GetDlgItem(hWnd, IDC_STATUS), "Enter key length");
		Random.BytesNeeded = 0;
		Random.Read = 0;
		Random.Bar = 0;
		Random.Data = NULL;
	}else
	if(Msg == WM_USER + 1){
		hWndP = (HWND)wParam;
	}else
	if(Msg == WM_COMMAND){
		if(HIWORD(wParam) == BN_CLICKED){
			if(LOWORD(wParam) == IDC_START){
				CHAR Status[256];
				GetWindowText(GetDlgItem(hWnd, IDC_BITS), Status, sizeof(Status));
				UINT Bits = atoi(Status);
				if(Bits < MIN_RSA_MODULUS_BITS)
					MessageBox(NULL, "The key is too small", "Error", MB_OK | MB_ICONERROR);
				else if(Bits > MAX_RSA_MODULUS_BITS)
					MessageBox(NULL, "The key is too large", "Error", MB_OK | MB_ICONERROR);
				else{
					EnableWindow(GetDlgItem(hWnd, IDC_BITS), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_START), FALSE);
					SetWindowText(GetDlgItem(hWnd, IDC_STATUS), "Please move the mouse around in this box to create entropy.");
					if(Random.Data)
						delete[] Random.Data;
					Random.Read = 0;
					R_RANDOM_STRUCT RandomStruct;
					R_RandomInit(&RandomStruct);
					Random.Data = new CHAR[RandomStruct.bytesNeeded];
					Random.BytesNeeded = RandomStruct.bytesNeeded;
					Random.Bits = Bits;
					SendMessage(GetDlgItem(hWnd, IDC_PROGRESS), PBM_SETRANGE, 0, MAKELPARAM(0, Random.BytesNeeded));
				}
			}else
			if(LOWORD(wParam) == IDCANCEL){
				delete[] Random.Data;
				if(Random.Thread){
					HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, Random.Thread->GetThreadID());
					TerminateThread(hThread, 0);
				}
				hWndGK = NULL;
				DestroyWindow(hWnd);
			}
		}else
		if(HIWORD(wParam) == EN_CHANGE){
			if(LOWORD(wParam) == IDC_BITS){
				CHAR Status[256];
				GetWindowText(GetDlgItem(hWnd, IDC_BITS), Status, sizeof(Status));
				UINT Bits = atoi(Status);

				if(Bits < MIN_RSA_MODULUS_BITS)
					sprintf(Status, "Key too small.");
				else if(Bits > MAX_RSA_MODULUS_BITS)
					sprintf(Status, "Key too large.");
				else{
					UINT Minutes = 1;
					if(Bits <= 768)
						Minutes = 1;
					else if(Bits > 768 && Bits <= 1536)
						Minutes = 3;
					else if(Bits > 1536 && Bits <= 2048)
						Minutes = 10;
					else if(Bits > 2048 && Bits <= 4096)
						Minutes = 20;
					sprintf(Status, "May take up to %d minute(s) to generate.", Minutes);
				}
				SetWindowText(GetDlgItem(hWnd, IDC_STATUS), Status);
			}
		}
	}else
	if(Msg == WM_MOUSEMOVE){
		if(Random.BytesNeeded > Random.Read){
			POINTS Mouse = MAKEPOINTS(lParam);
			Random.Data[Random.Read] = rand_r(0, 0xff);
			Random.Read++;
			SendMessage(GetDlgItem(hWnd, IDC_PROGRESS), PBM_SETPOS, Random.Read, 0);
			if(Random.BytesNeeded == Random.Read){
				SetWindowText(GetDlgItem(hWnd, IDC_STATUS), "Now generating key...");
				SetTimer(hWnd, 1, 100, NULL);
				Random.Thread = new GenerateKeyThread;
			}
		}
	}else
	if(Msg == WM_TIMER){
		Random.Bar += (20000000/float(Random.Bits*Random.Bits));
		if(Random.Bar >= Random.BytesNeeded)
			Random.Bar = 0;
		SendMessage(GetDlgItem(hWnd, IDC_PROGRESS), PBM_SETPOS, (UINT)Random.Bar, 0);
	}else
	if(Msg == (WM_USER + 2)){
		SetWindowText(GetDlgItem(hWnd, IDC_STATUS), "Done generating Key.");
		SendMessage(GetDlgItem(hWnd, IDC_PROGRESS), PBM_SETPOS, Random.BytesNeeded, 0);
		SetWindowText(GetDlgItem(hWnd, IDCANCEL), "Close");
		KillTimer(hWnd, 1);
		if(hWndP)
			SendMessage(hWndP, WM_INITDIALOG, 0, 0);
	}
	return 0;
}

BOOL CALLBACK WINAPI SettingsProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam){
	if(Msg == WM_INITDIALOG){
		CHAR Text[256];
		if(RSA::PrivateKeyStored()){
			UCHAR Hash[16];
			Password::RetreiveHash(Hash);
			RSA::RetrievePrivateKey(Hash, &RSAPrivateKey);
			RSA::MakePublicKey(&RSAPublicKey, RSAPrivateKey);
			sprintf(Text, "%d-bit key loaded", RSAPrivateKey.bits);
			EnableWindow(GetDlgItem(hWnd, IDC_EXPORTPRIVATE), TRUE);
			EnableWindow(GetDlgItem(hWnd, IDC_EXPORTPUBLIC), TRUE);
		}else{
			strcpy(Text, "No key loaded");
			EnableWindow(GetDlgItem(hWnd, IDC_EXPORTPRIVATE), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_EXPORTPUBLIC), FALSE);
		}
		SetWindowText(GetDlgItem(hWnd, IDC_STATUS), Text);
		if(RSA::PrivateKeyMasterStored()){
			R_RSA_PRIVATE_KEY RSAPrivateKeyMaster;
			UCHAR Hash[16];
			Password::RetreiveHash(Hash);
			RSA::RetrievePrivateKeyMaster(Hash, &RSAPrivateKeyMaster);
			sprintf(Text, "%d-bit key loaded", RSAPrivateKeyMaster.bits);
		}else{
			strcpy(Text, "No key loaded");
		}
		SetWindowText(GetDlgItem(hWnd, IDC_STATUS2), Text);
	}else
	if(Msg == WM_COMMAND){
		if(HIWORD(wParam) == BN_CLICKED){
			if(LOWORD(wParam) == IDC_GENERATE){
				UINT Ret = IDYES;
				if(RSA::PrivateKeyStored())
					Ret = MessageBox(NULL, "A key already exists, generating a new one will replace it, are you sure?", "Warning", MB_YESNO | MB_ICONWARNING);
				if(Ret == IDYES){
					HWND hWndD = CreateDialog(hInst, MAKEINTRESOURCE(IDD_GENERATEKEY), NULL, GenerateKeyProc);
					SendMessage(hWndD, WM_USER + 1, (WPARAM)hWnd, 0);
				}
			}else
			if(LOWORD(wParam) == IDC_EXPORTPRIVATE){
				CHAR FileName[MAX_PATH];
				FileName[0] = '\0';
				OPENFILENAME ofn;
				memset(&ofn, 0, sizeof(ofn));
				ofn.lStructSize = sizeof(OPENFILENAME);
				ofn.hwndOwner = hWnd;
				ofn.lpstrFilter = "Private Key Files (*.prv)\0*.prv\0All files (*.*)\0*.*\0";
				ofn.lpstrDefExt = "prv";
				ofn.lpstrTitle = "Export Private Key";
				ofn.lpstrFile = FileName;
				ofn.nMaxFile = sizeof(FileName) - 1;
				ofn.Flags = OFN_OVERWRITEPROMPT;
				if(GetSaveFileName(&ofn) != ERROR){
					UCHAR Hash[16];
					Password::RetreiveHash(Hash);
					R_RSA_PRIVATE_KEY PrivateKey;
					RSA::RetrievePrivateKey(Hash, &PrivateKey);
					RSA::ExportPrivateKey(Hash, FileName, PrivateKey);
				}
			}else
			if(LOWORD(wParam) == IDC_EXPORTPUBLIC){
				CHAR FileName[MAX_PATH];
				FileName[0] = '\0';
				OPENFILENAME ofn;
				memset(&ofn, 0, sizeof(ofn));
				ofn.lStructSize = sizeof(OPENFILENAME);
				ofn.hwndOwner = hWnd;
				ofn.lpstrFilter = "Public Key Files (*.pub)\0*.pub\0All files (*.*)\0*.*\0";
				ofn.lpstrDefExt = "pub";
				ofn.lpstrTitle = "Export Public Key";
				ofn.lpstrFile = FileName;
				ofn.nMaxFile = sizeof(FileName) - 1;
				ofn.Flags = OFN_OVERWRITEPROMPT;
				if(GetSaveFileName(&ofn) != ERROR){
					UCHAR Hash[16];
					Password::RetreiveHash(Hash);
					R_RSA_PRIVATE_KEY PrivateKey;
					RSA::RetrievePrivateKey(Hash, &PrivateKey);
					RSA::ExportPublicKey(Hash, FileName, RSA::MakePublicKey(PrivateKey));
				}
			}else
			if(LOWORD(wParam) == IDC_IMPORT || LOWORD(wParam) == IDC_IMPORT2){
				UINT Return = IDOK;
				if(LOWORD(wParam) == IDC_IMPORT?RSA::PrivateKeyStored():RSA::PrivateKeyMasterStored())
					Return = MessageBox(NULL, "This will overwrite your current key", "Warning", MB_OKCANCEL|MB_ICONWARNING);
				if(Return == IDOK){
					CHAR FileName[MAX_PATH];
					FileName[0] = '\0';
					OPENFILENAME ofn;
					memset(&ofn, 0, sizeof(ofn));
					ofn.lStructSize = sizeof(OPENFILENAME);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFilter = "Private Key Files (*.prv)\0*.prv\0All files (*.*)\0*.*\0";
					ofn.lpstrDefExt = "prv";
					ofn.lpstrTitle = "Import Private Key";
					ofn.lpstrFile = FileName;
					ofn.nMaxFile = sizeof(FileName) - 1;
					ofn.Flags = NULL;
					if(GetOpenFileName(&ofn) != ERROR){
						UCHAR Hash[16];
						Password::RetreiveHash(Hash);
						R_RSA_PRIVATE_KEY PrivateKey;
						if(!RSA::ImportPrivateKey(Hash, FileName, &PrivateKey)){
							MessageBox(NULL, "This private key appears to be either invalid or was exported using a different hash", "Error", MB_OK|MB_ICONERROR);
						}else{
							LOWORD(wParam) == IDC_IMPORT?RSA::StorePrivateKey(Hash, PrivateKey):RSA::StorePrivateKeyMaster(Hash, PrivateKey);
							MessageBox(NULL, "Private key successfully imported", "Imported successfully", MB_OK|MB_ICONINFORMATION);
							SendMessage(hWnd, WM_INITDIALOG, 0, 0);
						}
					}
				}
			}
		}
	}
	return 0;
}

BOOL CALLBACK WINAPI AboutProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam){
	if(Msg == WM_INITDIALOG){
		
	}
	return 0;
}

BOOL AddEdit = FALSE;
CHAR AddEditAddress[256];
BOOL AddEditPermanent = FALSE;
UINT AddEditSelection = 0;

BOOL CALLBACK WINAPI AddEditLinkProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam){
	if(Msg == WM_INITDIALOG){
		if(lParam){
			SetWindowText(hWnd, "Edit Link");
			SetWindowText(GetDlgItem(hWnd, IDC_ADDRESS), AddEditAddress);
			BOOL Checked = (LinkCache::GetLink(AddEditAddress).Permanent ? BST_CHECKED : BST_UNCHECKED);
			SendMessage(GetDlgItem(hWnd, IDC_PERMANENT), BM_SETCHECK, (WPARAM)Checked, NULL);
			AddEdit = TRUE;
		}else{
			SetWindowText(hWnd, "Add Link");
			AddEdit = FALSE;
		}
		SetFocus(GetDlgItem(hWnd, IDC_ADDRESS));
	}else
	if(Msg == WM_COMMAND){
		if(HIWORD(wParam) == BN_CLICKED){
			if(LOWORD(wParam) == IDOK){
				if(AddEdit)
					LinkCache::RemoveLink(AddEditAddress);
				GetWindowText(GetDlgItem(hWnd, IDC_ADDRESS), AddEditAddress, sizeof(AddEditAddress));
				CHAR Hostname[256];
				USHORT Port;
				LinkCache::DecodeName(AddEditAddress, Hostname, sizeof(Hostname), &Port);
				CHAR Encoded[256];
				LinkCache::EncodeName(Encoded, sizeof(Encoded), Hostname, Port);
				LinkCache::AddLink(Encoded);
				LinkCache::Link Link = LinkCache::GetLink(Encoded);
				Link.Permanent = SendMessage(GetDlgItem(hWnd, IDC_PERMANENT), BM_GETCHECK, NULL, NULL) == BST_CHECKED ? 1 : 0;
				LinkCache::UpdateLink(Link);
				EndDialog(hWnd, 0);
			}else
			if(LOWORD(wParam) == IDCANCEL){
				EndDialog(hWnd, 0);
			}
		}
	}
	return 0;
}

BOOL CALLBACK WINAPI LinksProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam){
	if(Msg == WM_INITDIALOG){
		cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
		if(!Reg.Exists(REG_AUTOADDLINKS))
			Reg.SetInt(REG_AUTOADDLINKS, 100);
		if(!Reg.Exists(REG_AUTOREMOVELINKS))
			Reg.SetInt(REG_AUTOREMOVELINKS, 30);
		DWORD Links = Reg.GetInt(REG_AUTOADDLINKS);
		SendMessage(GetDlgItem(hWnd, IDC_MAXLINKS), EM_LIMITTEXT, 5, 0);
		if(Links){
			CheckDlgButton(hWnd, IDC_AUTOADDLINKS, BST_CHECKED);
			CHAR Temp[16];
			SetWindowText(GetDlgItem(hWnd, IDC_MAXLINKS), itoa(Links, Temp, 10));
		}else{
			EnableWindow(GetDlgItem(hWnd, IDC_MAXLINKS), FALSE);
		}
		Links = Reg.GetInt(REG_AUTOREMOVELINKS);
		SendMessage(GetDlgItem(hWnd, IDC_MINLINKS), EM_LIMITTEXT, 5, 0);
		if(Links){
			CheckDlgButton(hWnd, IDC_AUTOREMOVELINKS, BST_CHECKED);
			CHAR Temp[16];
			SetWindowText(GetDlgItem(hWnd, IDC_MINLINKS), itoa(Links, Temp, 10));
		}else{
			EnableWindow(GetDlgItem(hWnd, IDC_MINLINKS), FALSE);
		}
		SendMessage(hWnd, WM_USER, NULL, NULL);
	}else
	if(Msg == WM_USER){
		while(SendMessage(GetDlgItem(hWnd, IDC_LIST), LB_DELETESTRING, 0, NULL) > 0);
		UINT LinkIndex = 0;
		for(INT i = 0; i < LinkCache::GetLinkCount(); i++){
			CHAR Encoded[256];
			LinkCache::Link TempLink = LinkCache::GetNextLink(&LinkIndex);
			LinkCache::EncodeName(Encoded, sizeof(Encoded), TempLink.Hostname, TempLink.Port);
			SendMessage(GetDlgItem(hWnd, IDC_LIST), LB_INSERTSTRING, i, (LPARAM)Encoded);
		}
		CHAR Text[16];
		SetWindowText(GetDlgItem(hWnd, IDC_COUNT), itoa(SendMessage(GetDlgItem(hWnd, IDC_LIST), LB_GETCOUNT, 0, 0), Text, 10));
		if(LinkCache::GetLinkCount() > 0){
			EnableWindow(GetDlgItem(hWnd, IDC_EXPORT), TRUE);
			EnableWindow(GetDlgItem(hWnd, IDC_CLEAR), TRUE);
		}else{
			EnableWindow(GetDlgItem(hWnd, IDC_EXPORT), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_CLEAR), FALSE);
		}
		if(SendMessage(GetDlgItem(hWnd, IDC_LIST), LB_SETCURSEL, (WPARAM)AddEditSelection, NULL) == LB_ERR){
			EnableWindow(GetDlgItem(hWnd, IDC_EDIT), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_REMOVE), FALSE);
		}else{
			EnableWindow(GetDlgItem(hWnd, IDC_EDIT), TRUE);
			EnableWindow(GetDlgItem(hWnd, IDC_REMOVE), TRUE);
		}
	}else
	if(Msg == WM_COMMAND){
		if(HIWORD(wParam) == LBN_SELCHANGE){
			EnableWindow(GetDlgItem(hWnd, IDC_EDIT), TRUE);
			EnableWindow(GetDlgItem(hWnd, IDC_REMOVE), TRUE);
			AddEditSelection = SendMessage(GetDlgItem(hWnd, IDC_LIST), LB_GETCURSEL, NULL, NULL);
			if(AddEditSelection < 0)
				AddEditSelection = 0;
		}else
		if(HIWORD(wParam) == BN_CLICKED){
			if(LOWORD(wParam) == IDC_AUTOADDLINKS){
				if(IsDlgButtonChecked(hWnd, IDC_AUTOADDLINKS) == BST_UNCHECKED){
					cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
					EnableWindow(GetDlgItem(hWnd, IDC_MAXLINKS), FALSE);
					Reg.SetInt(REG_AUTOADDLINKS, 0);
				}else
				if(IsDlgButtonChecked(hWnd, IDC_AUTOADDLINKS) == BST_CHECKED){
					EnableWindow(GetDlgItem(hWnd, IDC_MAXLINKS), TRUE);
					SetWindowText(GetDlgItem(hWnd, IDC_MAXLINKS), "100");
				}
			}else
			if(LOWORD(wParam) == IDC_AUTOREMOVELINKS){
				if(IsDlgButtonChecked(hWnd, IDC_AUTOREMOVELINKS) == BST_UNCHECKED){
					cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
					EnableWindow(GetDlgItem(hWnd, IDC_MINLINKS), FALSE);
					Reg.SetInt(REG_AUTOREMOVELINKS, 0);
				}else
				if(IsDlgButtonChecked(hWnd, IDC_AUTOREMOVELINKS) == BST_CHECKED){
					EnableWindow(GetDlgItem(hWnd, IDC_MINLINKS), TRUE);
					SetWindowText(GetDlgItem(hWnd, IDC_MINLINKS), "30");
				}
			}else
			if(LOWORD(wParam) == IDC_ADD){
				DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_ADDEDITLINK), hWnd, AddEditLinkProc, FALSE);
				PostMessage(hWnd, WM_USER, NULL, NULL);
			}else
			if(LOWORD(wParam) == IDC_EDIT){
				INT Item = SendMessage(GetDlgItem(hWnd, IDC_LIST), LB_GETCURSEL, 0, 0);
				SendMessage(GetDlgItem(hWnd, IDC_LIST), LB_GETTEXT, (WPARAM)Item, (LPARAM)&AddEditAddress);
				AddEditPermanent = LinkCache::GetLink(AddEditAddress).Permanent;
				DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_ADDEDITLINK), hWnd, AddEditLinkProc, TRUE);
				PostMessage(hWnd, WM_USER, NULL, NULL);
			}else
			if(LOWORD(wParam) == IDC_REMOVE){
				INT Item = SendMessage(GetDlgItem(hWnd, IDC_LIST), LB_GETCURSEL, 0, 0);
				CHAR Address[256];
				SendMessage(GetDlgItem(hWnd, IDC_LIST), LB_GETTEXT, (WPARAM)Item, (LPARAM)&Address);
				LinkCache::RemoveLink(Address);
				PostMessage(hWnd, WM_USER, NULL, NULL);
			}else
			if(LOWORD(wParam) == IDC_IMPORT){
				MessageBox(hWnd, "Not Implemented", "Sorry", MB_OK);
				PostMessage(hWnd, WM_USER, NULL, NULL);
			}else
			if(LOWORD(wParam) == IDC_EXPORT){
				CHAR FileName[MAX_PATH];
				FileName[0] = '\0';
				OPENFILENAME ofn;
				memset(&ofn, 0, sizeof(ofn));
				ofn.lStructSize = sizeof(OPENFILENAME);
				ofn.hwndOwner = hWnd;
				ofn.lpstrFilter = "Text (*.txt)\0*.txt\0C Source (*.c)\0*.c\0";
				ofn.lpstrDefExt = "txt";
				ofn.lpstrTitle = "Export Link List";
				ofn.lpstrFile = FileName;
				ofn.nMaxFile = sizeof(FileName) - 1;
				ofn.Flags = OFN_OVERWRITEPROMPT;
				if(GetSaveFileName(&ofn) != ERROR){
					File File(FileName);
					BOOL C = FALSE;
					if(strcmp(strrchr(FileName, '.'), ".c") == 0)
						C = TRUE;
					if(File.GetHandle() != INVALID_HANDLE_VALUE){
						File.SetEOF();
						UINT Index = 0;
						UINT OldIndex = 0;
						if(C){
							File.Writef("cRegistry Reg;\r\nCHAR Prefix[64];\r\nstrcpy(Prefix, REG_LINKS);\r\nstrcat(Prefix, \"\\\\\");\r\nCHAR Name[256];\r\n");
						}
						while(1){
							OldIndex = Index;
							LinkCache::Link Link = LinkCache::GetNextLink(&Index);
							if(OldIndex > Index){
								break;
							}
							if(C){
								File.Writef("strcpy(Name, Prefix); strcat(Name, \"%s:%d\"); Reg.CreateKey(HKEY_CURRENT_USER, Name); Reg.CloseKey();\r\n", Link.Hostname, Link.Port);
							}else{
								File.Writef("%s:%d\r\n", Link.Hostname, Link.Port);
							}
							//File.Writef("    WriteRegDWORD HKEY_CURRENT_USER \"SOFTWARE\\GNU\\Data\\%s:%d\" \"F\" 0\r\n", Link.Hostname, Link.Port);
						}
					}
				}
				PostMessage(hWnd, WM_USER, NULL, NULL);
			}else
			if(LOWORD(wParam) == IDC_CLEAR){
				if(MessageBox(hWnd, "Are you sure you want to clear the list?", "Warning", MB_YESNO) == IDYES){
					UINT Index = 0;
					LinkCache::RemoveAll();
					PostMessage(hWnd, WM_USER, NULL, NULL);
				}
			}
		}else
		if(HIWORD(wParam) == EN_CHANGE){
			if(LOWORD(wParam) == IDC_MAXLINKS){
				cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
				CHAR Temp[16];
				GetWindowText(GetDlgItem(hWnd, IDC_MAXLINKS), Temp, sizeof(Temp));
				UINT MaxLinks = atoi(Temp);
				if(MaxLinks > MAX_LINKS)
					MaxLinks = MAX_LINKS;
				if(MaxLinks < 0)
					MaxLinks = 0;
				Reg.SetInt(REG_AUTOADDLINKS, MaxLinks);
			}else
			if(LOWORD(wParam) == IDC_MINLINKS){
				cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
				CHAR Temp[16];
				GetWindowText(GetDlgItem(hWnd, IDC_MINLINKS), Temp, sizeof(Temp));
				UINT MinLinks = atoi(Temp);
				if(MinLinks > 99999)
					MinLinks = 99999;
				if(MinLinks < 0)
					MinLinks = 0;
				Reg.SetInt(REG_AUTOREMOVELINKS, MinLinks);
			}
		}
	}
	return 0;
}

BOOL CALLBACK WINAPI LogProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam){
	if(Msg == WM_INITDIALOG){
		cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
		if(!Reg.Exists(REG_LOGHARVESTDIR))
			Reg.SetString(REG_LOGHARVESTDIR, "");
		SetWindowText(GetDlgItem(hWnd, IDC_DIRECTORY), Reg.GetString(REG_LOGHARVESTDIR));
	}else
	if(Msg == WM_COMMAND){
		if(HIWORD(wParam) == BN_CLICKED){
			if(LOWORD(wParam) == IDC_BROWSE){
				CHAR FileName[MAX_PATH];
				LPITEMIDLIST ItemID;
				BROWSEINFO BI;
				BI.hwndOwner = hWnd;
				BI.pidlRoot = NULL;
				BI.pszDisplayName = FileName;
				BI.lpszTitle = "Select a folder to save log files to";
				BI.ulFlags = BIF_DONTGOBELOWDOMAIN | BIF_RETURNONLYFSDIRS;
				BI.lpfn = NULL;
				BI.lParam = NULL;
				BI.iImage = NULL;
				ItemID = SHBrowseForFolder(&BI);
				if(ItemID){
					if(SHGetPathFromIDList(ItemID, FileName)){
						cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
						Reg.SetString(REG_LOGHARVESTDIR, FileName);
						SetWindowText(GetDlgItem(hWnd, IDC_DIRECTORY), FileName);
					}
				}
			}
		}
	}
	return 0;
}

BOOL CALLBACK WINAPI DisplayProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam){
	if(Msg == WM_INITDIALOG){
		cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
		if(!Reg.Exists(REG_MINIMIZETOTRAY))
			Reg.SetInt(REG_MINIMIZETOTRAY, 0);
		CheckDlgButton(hWnd, IDC_MINIMIZETOTRAY, Reg.GetInt(REG_MINIMIZETOTRAY) ? BST_CHECKED : BST_UNCHECKED);
	}else
	if(Msg == WM_COMMAND){
		if(HIWORD(wParam) == BN_CLICKED){
			UINT Index = NULL;
			switch(LOWORD(wParam)){
				case IDC_COLORBTN1: Index = 1; break;
				case IDC_COLORBTN2: Index = 2; break;
				case IDC_COLORBTN3: Index = 3; break;
				case IDC_COLORBTN4: Index = 4; break;
				case IDC_COLORBTN5: Index = 5; break;
				case IDC_COLORBTN6: Index = 6; break;
				case IDC_COLORBTN7: Index = 7; break;
				case IDC_COLORBTN8: Index = 8; break;
				case IDC_COLORBTN9: Index = 9; break;
			}
			if(Index){
				CHOOSECOLOR Color;
				static COLORREF CustomColors[16];
				ZeroMemory(&Color, sizeof(Color));
				Color.lStructSize = sizeof(Color);
				Color.lpCustColors = CustomColors;
				Color.hwndOwner = hWnd;
				Color.rgbResult = Display.GetColorItem(Index);
				Color.Flags = CC_FULLOPEN | CC_RGBINIT;
				if(ChooseColor(&Color))
					Display.SetColorItem(Color.rgbResult, Index);
				InvalidateRect(hWnd, NULL, FALSE);
			}else
			if(LOWORD(wParam) == IDC_MINIMIZETOTRAY){
				cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);;
				if(IsDlgButtonChecked(hWnd, IDC_MINIMIZETOTRAY) == BST_UNCHECKED){
					Reg.SetInt(REG_MINIMIZETOTRAY, 0);
				}else
				if(IsDlgButtonChecked(hWnd, IDC_MINIMIZETOTRAY) == BST_CHECKED){
					Reg.SetInt(REG_MINIMIZETOTRAY, 1);
				}
			}
		}
	}else
	if(Msg == WM_DRAWITEM){
		LPDRAWITEMSTRUCT DrawItem = (LPDRAWITEMSTRUCT)lParam;
		UINT Index = 1;
		switch(wParam){
			case IDC_COLORBTN1: Index = 1; break;
			case IDC_COLORBTN2: Index = 2; break;
			case IDC_COLORBTN3: Index = 3; break;
			case IDC_COLORBTN4: Index = 4; break;
			case IDC_COLORBTN5: Index = 5; break;
			case IDC_COLORBTN6: Index = 6; break;
			case IDC_COLORBTN7: Index = 7; break;
			case IDC_COLORBTN8: Index = 8; break;
			case IDC_COLORBTN9: Index = 9; break;
		}
		HBRUSH Brush = CreateSolidBrush(Display.GetColorItem(Index));
		SelectObject(DrawItem->hDC, Brush);
		Rectangle(DrawItem->hDC, DrawItem->rcItem.left, DrawItem->rcItem.top, DrawItem->rcItem.right, DrawItem->rcItem.bottom);
		DeleteObject(Brush);
	}
	return 0;
}

BOOL CALLBACK WINAPI PreferencesProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam){
	if(Msg == WM_INITDIALOG){
		SetWindowText(hWnd, " Preferences");
		HWND hWndTree = GetDlgItem(hWnd, IDC_TREE);
		HTREEITEM LinksItem = TV_AddItem(hWndTree, TVI_ROOT, "Links", 0, 5);
		//TV_AddItem(hWndTree, TVI_ROOT, "Proxy", 0, 6);
		HTREEITEM h = TV_AddItem(hWndTree, TVI_ROOT, "Settings", 1, 2);
			TV_AddItem(hWndTree, h, "Message Queue", 0, 7);
		LogHarvestTreeItem = TV_AddItem(hWndTree, h, "Log Harvester", 0, 8);
		SendMessage(hWndTree, TVM_EXPAND, TVE_EXPAND, (LPARAM)h);
		TV_AddItem(hWndTree, TVI_ROOT, "Display", 0, 3);
		AboutTreeItem = TV_AddItem(hWndTree, TVI_ROOT, "About", 0, 4);
		TreeView_SelectItem(hWndTree, LinksItem);
	}else
	if(Msg == WM_CLOSE){
		hWndPrefs = NULL;
		DestroyWindow(hWnd);
	}else
	if(Msg == WM_COMMAND){
		if(HIWORD(wParam) == BN_CLICKED){
			if(LOWORD(wParam) == IDC_CLOSE){
				hWndPrefs = NULL;
				DestroyWindow(hWnd);
			}
		}
	}else
	if(Msg == WM_NOTIFY){
		LPNMHDR Hdr = (LPNMHDR)lParam;
		if(Hdr->code == TVN_SELCHANGED){
			HTREEITEM hTitem = TreeView_GetSelection(GetDlgItem(hWnd, IDC_TREE));
			TVITEM tItem;
			tItem.mask = TVIF_HANDLE;
			tItem.hItem = hTitem;
			TreeView_GetItem(GetDlgItem(hWnd, IDC_TREE),&tItem);
			INT Res = -1;
			DLGPROC Proc = NULL;
			switch(tItem.lParam){
				case 2: Res = IDD_PREFS_SETTINGS; Proc = SettingsProc; break;
				case 3: Res = IDD_PREFS_DISPLAY; Proc = DisplayProc; break;
				case 4: Res = IDD_PREFS_ABOUT; Proc = AboutProc; break;
				case 5: Res = IDD_PREFS_LINKS; Proc = LinksProc; break;
				case 7: Res = IDD_PREFS_MESSAGEQUEUE; Proc = LinksProc; break;
				case 8: Res = IDD_PREFS_LOGHARVESTER; Proc = LogProc; break;
			}
			if(Res != -1){
				HWND oldhWndt = hWndt;
				hWndt = CreateDialog(hInst, MAKEINTRESOURCE(Res), hWnd, Proc);
				RECT Rect;
				GetWindowRect(GetDlgItem(hWnd, IDC_REGION), &Rect);
				ScreenToClient(hWnd, (LPPOINT)&Rect);
				SetWindowPos(hWndt, NULL, Rect.left, Rect.top, 0, 0, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOZORDER);
				ShowWindow(hWndt, SW_SHOWNA);
				if(oldhWndt)
					DestroyWindow(oldhWndt);
			}
		}
	}
	return 0;
}