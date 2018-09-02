#pragma once

#include <vector>
#include "socketwrapper.h"
#include "p2p2.h"
#include "display.h"

extern POINT pan, mouse;
extern HWND hWndg, hWndTooltip;
extern LPDIRECT3DDEVICE9 D3DDevice;

class Link
{
public:
	Link(PCHAR Name);
	~Link();
	VOID SetPosition(D3DVECTOR Position);
	D3DVECTOR GetPosition(VOID);
	VOID Draw(VOID);
	VOID Rotate(FLOAT x, FLOAT y);
	VOID Invalidate(VOID);
	VOID Validate(VOID);
	VOID SetAddr(ULONG Addr);
	VOID SetHostname(PCHAR Hostname);
	PCHAR GetHostname(VOID);
	USHORT GetPort(VOID);
	VOID SetRemotePort(USHORT Port);
	USHORT GetRemotePort();
	VOID SetClients(INT Clients);
	INT GetClients(VOID);
	INT GetLinks(VOID);
	BOOL IsUpdated(VOID);
	D3DXMATRIX GetWorldMatrix(VOID);
	Link* Add(PCHAR Name);
	std::vector<Link*> Neighbor;

private:
	D3DXVECTOR3 Position;
	D3DXVECTOR3 Translation;
	ULONG Addr;
	std::string Hostname;
	USHORT Port;
	INT Clients;
	BOOL bConnected;
	enum State {NEUTRAL, CONNECTING, UNREACHABLE, WAITING, UPDATED} State;
	D3DXMATRIX WorldMatrix;

public:
	VOID Connecting(VOID) { State = CONNECTING; };
	VOID Unreachable(VOID) { State = UNREACHABLE; };
	VOID Waiting(VOID) { State = WAITING; };
	VOID Updated(VOID) { State = UPDATED; };
	VOID Disconnecting(VOID) { bConnected = FALSE; State = NEUTRAL; };
	VOID Connected(VOID) { bConnected = TRUE; };
};

class Links
{
public:
	Links();
	Link* GetHead(VOID);
	VOID Draw(VOID);
	VOID Rotate(FLOAT x, FLOAT y);
	VOID Invalidate(VOID);
	VOID Validate(VOID);
	VOID ClearAll(VOID);
	VOID Remove(Link* Link);
	VOID RemoveNeighbors(Link* Link);
	BOOL IsLinkedTo(Link* Link);
	Link* Add(Link* From, PCHAR Name);
	Link* Head;
	std::vector<Link*> List;
};

class RingAnim
{
public:
	RingAnim();
	D3DXVECTOR3 GetRotationAxis(VOID);
	FLOAT GetRotationAngle(VOID);

private:
	D3DXVECTOR3 RotationAxis;
	Timeout Timeout;
	DWORD Time;
};

/*
#define LINK_STATUS_WAITING 1
#define LINK_STATUS_CONNECTING 2
#define LINK_STATUS_CONNECTED 4
#define LINK_STATUS_UPDATED 8
#define LINK_STATUS_CONNECTIONREFUSED 16
#define LINK_STATUS_ALL 255

class LinkEx{
public:
	LinkEx() { Addr = NULL; Clients = 0; Status = LINK_STATUS_WAITING; };
	VOID SetAddr(PCHAR Host) { Addr = inet_addr(Host); };
	VOID SetAddr(ULONG Addr) { LinkEx::Addr = Addr; };
	ULONG GetAddr(VOID) const { return Addr; };
	VOID SetStatus(BYTE Status) { LinkEx::Status |= Status; };
	VOID ClearStatus(BYTE Status) { LinkEx::Status &= ~Status; };
	BYTE GetStatus(VOID) const { return Status; };
	VOID SetNumClients(BYTE Clients) { LinkEx::Clients = Clients; };
	BYTE NumOfClients(VOID) const { return Clients; };
	VOID SetPosition(POINT Position) { LinkEx::Position = Position; };
	POINT GetPosition(VOID) const { return Position; };
	VOID Draw(HDC hDC);

	std::vector<LinkEx*> L;

private:
	BYTE Clients;
	ULONG Addr;
	BYTE Status;
	POINT Position;
};

class Links{
public:
	Links() { Head = NULL; };
	LinkEx* GetHead(VOID) { if(!Head){Head = new LinkEx; POINT P = {0,0}; Head->SetPosition(P); List.push_back(Head);} return Head; };
	LinkEx* Add(LinkEx & From, BYTE Side, ULONG Addr, BYTE Clients);
	VOID Draw(HDC hDC);
	LinkEx* GetWaiting(VOID);
	LinkEx* FindHost(ULONG Host);

private:
	std::vector<LinkEx*> List;
	LinkEx *Head;

};*/