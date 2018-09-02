#include "firewall.h"
#include "debug.h"

FireWall::FireWall(){
    INetFwMgr* FwMgr = NULL;
    INetFwPolicy* FwPolicy = NULL;
	FwProfile = NULL;
	Initialized = FALSE;
	if(SUCCEEDED(CoInitialize(NULL))){
		Initialized = TRUE;
		if(SUCCEEDED(CoCreateInstance(__uuidof(NetFwMgr), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwMgr), (void**)&FwMgr))){
			if(SUCCEEDED(FwMgr->get_LocalPolicy(&FwPolicy))){
				FwPolicy->get_CurrentProfile(&FwProfile);
			}
		}
	}
	if(FwMgr)
		FwMgr->Release();
	if(FwPolicy)
		FwPolicy->Release();
}

FireWall::~FireWall(){
	if(FwProfile)
		FwProfile->Release();
	if(Initialized)
		CoUninitialize();
}

BOOL FireWall::Enabled(VOID){
	if(FwProfile){
		VARIANT_BOOL FwEnabled;
		FwProfile->get_FirewallEnabled(&FwEnabled);
		return FwEnabled == VARIANT_TRUE ? TRUE : FALSE;
	}else
		return FALSE;
}

BOOL FireWall::Enable(BOOL Enable){
	if(FwProfile){
		VARIANT_BOOL FwEnabled VARIANT_FALSE;
		if(Enable)
			FwEnabled = VARIANT_TRUE;
		if(SUCCEEDED(FwProfile->put_FirewallEnabled(FwEnabled)))
			return TRUE;
		return FALSE;
	}else
		return FALSE;
}

BOOL FireWall::OpenPort(USHORT Port, NET_FW_IP_PROTOCOL Protocol, const OLECHAR* Name){
	if(FwProfile){
		INetFwOpenPort* FwOpenPort = NULL;
		INetFwOpenPorts* FwOpenPorts = NULL;
		BSTR FwBstrName = NULL;
		BOOL Success = FALSE;

		if(SUCCEEDED(FwProfile->get_GloballyOpenPorts(&FwOpenPorts))){
			if(SUCCEEDED(CoCreateInstance(__uuidof(NetFwOpenPort), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwOpenPort), (void**)&FwOpenPort))){
				if((FwBstrName = SysAllocString(Name))){
					if(SUCCEEDED(FwOpenPort->put_Port(Port)) && SUCCEEDED(FwOpenPort->put_Protocol(Protocol)) && SUCCEEDED(FwOpenPort->put_Name(FwBstrName))){
						if(SUCCEEDED(FwOpenPorts->Add(FwOpenPort)))
							Success = TRUE;
					}
				}
			}
		}

		if(FwBstrName)
			SysFreeString(FwBstrName);
		if(FwOpenPort)
			FwOpenPort->Release();
		if(FwOpenPorts)
			FwOpenPorts->Release();

		return Success;
	}else
		return FALSE;
}

BOOL FireWall::PortOpened(USHORT Port, NET_FW_IP_PROTOCOL Protocol){
	if(FwProfile){
		INetFwOpenPort* FwOpenPort = NULL;
		INetFwOpenPorts* FwOpenPorts = NULL;
		VARIANT_BOOL FwEnabled = VARIANT_FALSE;
		if(SUCCEEDED(FwProfile->get_GloballyOpenPorts(&FwOpenPorts))){
			if(SUCCEEDED(FwOpenPorts->Item(Port, Protocol, &FwOpenPort))){
				FwOpenPort->get_Enabled(&FwEnabled);
			}
		}
		
		if(FwOpenPort)
			FwOpenPort->Release();
		if(FwOpenPorts)
			FwOpenPorts->Release();

		return FwEnabled == VARIANT_TRUE ? TRUE : FALSE;
	}else
		return FALSE;
}