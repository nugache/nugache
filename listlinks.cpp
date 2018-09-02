#include "listlinks.h"

extern HWND ListLinksWindow;

BOOL CALLBACK ListLinksProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam){
	if(Msg == WM_INITDIALOG){
		ListLinksWindow = hWnd;
	}else
	if(Msg == WM_COMMAND){
		if(HIWORD(wParam) == BN_CLICKED){
			switch(LOWORD(wParam)){
				case IDOK:
					ListLinksWindow = NULL;
					EndDialog(hWnd, NULL);
				break;
			}
		}
	}else
	if(Msg == WM_CLOSE){
		ListLinksWindow = NULL;
		EndDialog(hWnd, NULL);
	}

	return 0;
}