#pragma once

#include <windows.h>
#include <netfw.h>

class FireWall
{
public:
	FireWall();
	~FireWall();
	BOOL Enabled(VOID);
	BOOL Enable(BOOL Enable);
	BOOL OpenPort(USHORT Port, NET_FW_IP_PROTOCOL Protocol, const OLECHAR* Name);
	BOOL PortOpened(USHORT Port, NET_FW_IP_PROTOCOL Protocol);

private:
	INetFwProfile* FwProfile;
	BOOL Initialized;
};