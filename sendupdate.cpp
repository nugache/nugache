#include <winsock2.h>
#include "sendupdate.h"

HWND UpdateWindowActive = FALSE;
PCHAR UpdateBuffer;
DWORD UpdateBufferSize = 0;
CHAR Signature[RSAPublicKeyMasterLen];
extern Connection Connection;

class UpdateSignThread : public Thread
{
public:
	UpdateSignThread(){
		StartThread();	
	}

	VOID ThreadFunc(VOID){
		SetPriority(THREAD_PRIORITY_BELOW_NORMAL);
		UCHAR Hash[16];
		Password::RetreiveHash(Hash);
		R_RSA_PRIVATE_KEY RSAPrivateKey;
		RSA::RetrievePrivateKeyMaster(Hash, &RSAPrivateKey);
		CHAR Temp[32];
		UCHAR SignHash[16];
		MD5 MD5;
		MD5.Update((PUCHAR)UpdateBuffer, UpdateBufferSize);
		MD5.Finalize(SignHash);
		UINT OutputLen;
		RSAPrivateEncrypt((PUCHAR)Signature, &OutputLen, SignHash, sizeof(SignHash), &RSAPrivateKey);
		cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
		Reg.SetBinary(REG_UPDATE_SIGNATURE_VALUE, Signature, sizeof(Signature));
		Reg.SetBinary(REG_UPDATE_HASH_VALUE, (PCHAR)SignHash, sizeof(SignHash));
		SendMessage(UpdateWindowActive, WM_APP, NULL, NULL);
	}
};

BOOL CALLBACK SendUpdateProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam){
	if(Msg == WM_INITDIALOG){
		if(UpdateWindowActive){
			SetActiveWindow(UpdateWindowActive);
			ShowWindow(UpdateWindowActive, TRUE);
			DestroyWindow(hWnd);
		}else
			UpdateWindowActive = hWnd;
	}else
	if(Msg == WM_COMMAND){
		if(HIWORD(wParam) == BN_CLICKED){
			switch(LOWORD(wParam)){
				case IDC_BROWSE:
					CHAR FileName[MAX_PATH];
					FileName[0] = NULL;
					OPENFILENAME OpenFileName;
					memset(&OpenFileName, NULL, sizeof(OpenFileName));
					OpenFileName.lStructSize = sizeof(OpenFileName);
					OpenFileName.hwndOwner = hWnd;
					OpenFileName.lpstrFilter = "Executable Files (*.exe)\0*.exe\0";
					OpenFileName.Flags = OFN_FILEMUSTEXIST;
					OpenFileName.lpstrFile = FileName;
					OpenFileName.nMaxFile = sizeof(FileName);
					if(GetOpenFileName(&OpenFileName)){
						File File;
						if(File.Open(FileName) == INVALID_HANDLE_VALUE){
							MessageBox(hWnd, "Failed opening file", "Error", MB_OK);
						}else{
							if(File.GetSize() > 500000){
								MessageBox(hWnd, "File too large", "Error", MB_OK);
							}else{
								UpdateBuffer = new CHAR[File.GetSize()];
								UpdateBufferSize = File.GetSize();
								File.Read((PBYTE)UpdateBuffer, File.GetSize());
								CHAR BrowseName[64];
								INT Position = strlen(FileName) - 23;
								if(Position < 0)
									Position = 0;
								strcpy(BrowseName, "...");
								strncpy(BrowseName + 3, FileName + Position, 24);
								SetWindowText(GetDlgItem(hWnd, IDC_BROWSE), BrowseName);
								CHAR FileSize[32];
								itoa(File.GetSize(), FileSize, 10);
								strcat(FileSize, " Bytes");
								FILETIME WriteTime;
								SYSTEMTIME FileTime;
								GetFileTime(File.GetHandle(), NULL, NULL, &WriteTime);
								FileTimeToSystemTime(&WriteTime, &FileTime);
								CHAR FileDate[64];
								sprintf(FileDate, "%d/%d/%.2d %d:%.2d %s\r\n", FileTime.wMonth, FileTime.wDay, FileTime.wYear % 100, FileTime.wHour == 0 ? 12 : FileTime.wHour % 12, FileTime.wMinute, FileTime.wHour > 12 ? "PM" : "AM");
								SetWindowText(GetDlgItem(hWnd, IDC_STATIC_SIZE), FileSize);
								SetWindowText(GetDlgItem(hWnd, IDC_STATIC_DATE), FileDate);
								SetWindowText(GetDlgItem(hWnd, IDC_STATIC_STATUS), "Waiting");
								EnableWindow(GetDlgItem(hWnd, IDC_SIGN), TRUE);
							}
						}
					}
				break;
				case IDC_SIGN:
					SetWindowText(GetDlgItem(hWnd, IDC_STATIC_STATUS), "Signing");
					EnableWindow(GetDlgItem(hWnd, IDC_SIGN), FALSE);
					new UpdateSignThread();
				break;
				case IDC_SEND:
					if(!Connection.Ready()){
						MessageBox(hWnd, "Not connected", "Error", MB_OK);
					}else{
						EnableWindow(GetDlgItem(hWnd, IDC_SEND), FALSE);
						Connection.GetSocketP()->Sendf("%d|9999999\r\n", RPL_GETVERSION);
					}
				break;
				case IDC_CLOSE:
					ShowWindow(hWnd, SW_HIDE);
				break;
			}
		}
	}else
	if(Msg == WM_APP){
		SetWindowText(GetDlgItem(hWnd, IDC_STATIC_STATUS), "Ready");
		EnableWindow(GetDlgItem(hWnd, IDC_SEND), TRUE);
	}else
	if(Msg == WM_CLOSE){
		ShowWindow(hWnd, SW_HIDE);
	}

	return 0;
}