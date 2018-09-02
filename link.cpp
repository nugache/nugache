#include <winsock2.h>
#include <commctrl.h>
#include "link.h"
#include "connections.h"

extern Link* SelectedLink;
extern Display Display;
extern Links Links;
extern Connection Connection;
LPD3DXMESH SphereMesh = NULL;
LPDIRECT3DVERTEXBUFFER9 RingVB = NULL;
const UINT RingVerticies = 80;
D3DXMATRIX RotationMatrix;
RingAnim RingAnim[3];

struct VERTEX{
	FLOAT x, y, z;
};

Link::Link(PCHAR Name){
	CHAR HostnameA[256];
	LinkCache::DecodeName(Name, HostnameA, sizeof(HostnameA), &Port);
	Clients = 0;
	bConnected = FALSE;
	State = NEUTRAL;
	Link::Hostname = HostnameA;
	Translation.x = 0;
	Translation.y = 0;
	Validate();
}

Link::~Link(){
	Invalidate();
}

VOID Link::SetPosition(D3DVECTOR Position){
	Link::Position = Position;
}

D3DVECTOR Link::GetPosition(VOID){
	return Position;
}

VOID Link::SetAddr(ULONG Addr){
	Link::Addr = Addr;
	SetHostname(inet_ntoa(SocketFunction::Stoin(Addr)));
}

VOID Link::SetHostname(PCHAR Hostname){
	Link::Hostname = Hostname;
}

PCHAR Link::GetHostname(VOID){
	return (PCHAR)Hostname.c_str();
}

USHORT Link::GetPort(VOID){
	return Port;
}

VOID Link::SetRemotePort(USHORT Port){
	Link::Port = Port;
}

USHORT Link::GetRemotePort(VOID){
	return Port;
}

VOID Link::SetClients(INT Clients){
	Link::Clients = Clients;
}

INT Link::GetClients(VOID){
	return Clients;
}

INT Link::GetLinks(VOID){
	return Neighbor.size();
}

BOOL Link::IsUpdated(VOID){
	return State == UPDATED;
}

VOID Link::Invalidate(VOID){
	if(SphereMesh){
		SphereMesh->Release();
		SphereMesh = NULL;
	}
	if(RingVB){
		RingVB->Release();
		RingVB = NULL;
	}
}

VOID Link::Validate(VOID){
	if(!SphereMesh){
		D3DXCreateSphere(D3DDevice, 0.5f, 20, 10, &SphereMesh, NULL);
		LPD3DXMESH NewMesh;
		SphereMesh->CloneMeshFVF(NULL, D3DFVF_XYZ | D3DFVF_NORMAL, D3DDevice, &NewMesh);
		SphereMesh->Release();
		SphereMesh = NewMesh;
		D3DXComputeNormals(SphereMesh, NULL);
	}
	if(!RingVB){
		D3DXVECTOR3 VRad(0.51f, 0, 0);
		FLOAT Width = 0.05f;
		VERTEX Verts[RingVerticies];
		for(UINT i = 0; i < RingVerticies; i += 2){
			D3DXMATRIX TransformMatrix;
			D3DXMatrixRotationZ(&TransformMatrix, ((D3DX_PI * 2) / (RingVerticies - 2)) * i);
			D3DXVECTOR4 VOut;
			D3DXVec3Transform(&VOut, &VRad, &TransformMatrix);
			D3DXMatrixTranslation(&TransformMatrix, 0, 0, -Width);
			D3DXVec4Transform(&VOut, &VOut, &TransformMatrix);
			Verts[i].x = VOut.x;
			Verts[i].y = VOut.y;
			Verts[i].z = VOut.z;
			D3DXMatrixTranslation(&TransformMatrix, 0, 0, Width * 2);
			D3DXVec4Transform(&VOut, &VOut, &TransformMatrix);
			Verts[i + 1].x = VOut.x;
			Verts[i + 1].y = VOut.y;
			Verts[i + 1].z = VOut.z;
			
		}
		D3DDevice->CreateVertexBuffer(sizeof(Verts), 0, D3DFVF_XYZ, D3DPOOL_DEFAULT, &RingVB, NULL);
		VOID* pVerts;
		RingVB->Lock(0, sizeof(Verts), (void**)&pVerts, 0);
		memcpy(pVerts, Verts, sizeof(Verts));
		RingVB->Unlock();
	}
}

D3DXMATRIX Link::GetWorldMatrix(VOID){
	return WorldMatrix;
}

VOID Link::Draw(VOID){
	D3DMATERIAL9 Material;
	D3DCOLORVALUE Temp;
	memset(&Material, NULL, sizeof(Material));
	switch(State){
		default:
		case NEUTRAL: Display.DwordToColorVal(Display.GetColorItem(4), &Temp); break;
		case CONNECTING: Display.DwordToColorVal(Display.GetColorItem(1), &Temp); break;
		case UNREACHABLE: Display.DwordToColorVal(Display.GetColorItem(5), &Temp); break;
		case WAITING: Display.DwordToColorVal(Display.GetColorItem(2), &Temp); break;
		case UPDATED: Display.DwordToColorVal(Display.GetColorItem(3), &Temp); break;
	}
	Material.Ambient = Temp;
	//Material.Diffuse = Temp;
	//Material.Specular = Temp;
	//Material.Emissive = D3DXCOLOR(0, 0, 0, 0.5f);

	D3DXMatrixIdentity(&WorldMatrix);

	if(this == SelectedLink){
		D3DXMATRIX ScaleMatrix;
		D3DXMatrixScaling(&ScaleMatrix, 1.2, 1.2, 1.2);
		D3DXMatrixMultiply(&WorldMatrix, &WorldMatrix, &ScaleMatrix);
	}

	D3DXMATRIX OldMatrix = WorldMatrix;

	if(this == Connection.GetLink()){
		for(UINT i = 0; i < 3; i++){
			D3DXVECTOR3 VAxis = RingAnim[i].GetRotationAxis();
			D3DXMATRIX RotateMatrix;
			D3DXMatrixRotationAxis(&RotateMatrix, &VAxis, RingAnim[i].GetRotationAngle());
			D3DXMatrixMultiply(&WorldMatrix, &WorldMatrix, &RotateMatrix);
			D3DDevice->SetTransform(D3DTS_WORLD, &WorldMatrix);

			D3DXMATRIX TranslateMatrix;
			D3DXMatrixTranslation(&TranslateMatrix, Position.x + Translation.x, Position.y + Translation.y, Position.z);
			D3DXMatrixMultiply(&WorldMatrix, &WorldMatrix, &TranslateMatrix);
			D3DXMatrixMultiply(&WorldMatrix, &WorldMatrix, &RotationMatrix);
			D3DDevice->SetTransform(D3DTS_WORLD, &WorldMatrix);

			D3DMATERIAL9 Material3;
			memset(&Material3, NULL, sizeof(Material3));
			D3DCOLORVALUE Temp;
			Display.DwordToColorVal(Display.GetColorItem(8), &Temp);
			Material3.Ambient = Temp;
			D3DDevice->SetMaterial(&Material3);
			D3DDevice->SetStreamSource(0, RingVB, 0, sizeof(VERTEX));
			D3DDevice->SetFVF(D3DFVF_XYZ);
			D3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
			D3DDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
			D3DDevice->SetRenderState(D3DRS_FOGENABLE, FALSE);
			D3DDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, RingVerticies - 2);
			D3DDevice->SetRenderState(D3DRS_FOGENABLE, TRUE);

			WorldMatrix = OldMatrix;
		}
	}

	D3DXMATRIX TranslateMatrix;
	D3DXMatrixTranslation(&TranslateMatrix, Position.x + Translation.x, Position.y + Translation.y, Position.z);
	D3DXMatrixMultiply(&WorldMatrix, &WorldMatrix, &TranslateMatrix);
	D3DXMatrixMultiply(&WorldMatrix, &WorldMatrix, &RotationMatrix);
	D3DDevice->SetTransform(D3DTS_WORLD, &WorldMatrix);

	D3DDevice->SetMaterial(&Material);
	SphereMesh->DrawSubset(0);

	D3DXMatrixIdentity(&WorldMatrix);
	D3DXMatrixTranslation(&TranslateMatrix, Position.x + Translation.x, Position.y + Translation.y, Position.z);
	D3DXMatrixMultiply(&WorldMatrix, &WorldMatrix, &TranslateMatrix);
	D3DXMatrixMultiply(&WorldMatrix, &WorldMatrix, &RotationMatrix);
	D3DDevice->SetTransform(D3DTS_WORLD, &WorldMatrix);


	D3DMATERIAL9 Material2;
	memset(&Material2, NULL, sizeof(Material2));
	if(this == SelectedLink)
		Display.DwordToColorVal(Display.GetColorItem(7), &Material2.Ambient);
	else
		Display.DwordToColorVal(Display.GetColorItem(6), &Material2.Ambient);
	D3DDevice->SetMaterial(&Material2);

	for(UINT i = 0; i < Neighbor.size(); i++){
		VERTEX Verts[20];

		D3DXVECTOR3 NPosition = Neighbor[i]->GetPosition();
		D3DXVECTOR3 VMag = (NPosition - Position);
		D3DXVECTOR3 VNormal;
		D3DXVec3Normalize(&VNormal, &VMag);
		D3DXVECTOR3 VPerp;
		VPerp.x = VNormal.z;
		VPerp.y = (VNormal.y * VNormal.z) / (VNormal.x - 1);
		VPerp.z = 1 + ((VNormal.z * VNormal.z) / (VNormal.x - 1));

		if(this == SelectedLink)
			VPerp *= 0.025;
		else
			VPerp *= 0.02;
	
		for(UINT i = 0; i < 20; i+=2){
			D3DXMATRIX TransformMatrix;
			D3DXMatrixRotationAxis(&TransformMatrix, &VNormal, ((D3DX_PI * 2) / 18) * i);
			D3DXVECTOR4 VOut;
			D3DXVec3Transform(&VOut, &VPerp, &TransformMatrix);
			Verts[i].x = VOut.x;
			Verts[i].y = VOut.y;
			Verts[i].z = VOut.z;
			D3DXMATRIX Matrix;
			D3DXMatrixTranslation(&Matrix, VMag.x, VMag.y, VMag.z);
			D3DXVECTOR4 VOut2;
			D3DXVec4Transform(&VOut2, &VOut, &Matrix);
			Verts[i + 1].x = VOut2.x;
			Verts[i + 1].y = VOut2.y;
			Verts[i + 1].z = VOut2.z;
		}
		LPDIRECT3DVERTEXBUFFER9 VB = NULL;
		D3DDevice->CreateVertexBuffer(sizeof(Verts), 0, D3DFVF_XYZ, D3DPOOL_DEFAULT, &VB, NULL);
		VOID* pVerts;
		VB->Lock(0, sizeof(Verts), (void**)&pVerts, 0);
		memcpy(pVerts, Verts, sizeof(Verts));
		VB->Unlock();
		D3DDevice->SetStreamSource(0, VB, 0, sizeof(VERTEX));
		D3DDevice->SetFVF(D3DFVF_XYZ);
		D3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		D3DDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
		D3DDevice->SetRenderState(D3DRS_FOGENABLE, FALSE);
		D3DDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 18);
		D3DDevice->SetRenderState(D3DRS_FOGENABLE, TRUE);
		VB->Release();
	}
	//Status.Draw();
	

/*	HBRUSH hBrush;
	HPEN hPen;
	RECT Rect;
	POINT Pos = {Position.x + pan.x, Position.y + pan.y};

	for(UINT i = 0; i < Neighbor.size(); i++){
		if(Neighbor[i]){
			hPen = CreatePen(PS_SOLID, 1, RGB(0, 255, 0));
			SelectObject(hDC, hPen);
			MoveToEx(hDC, Pos.x, Pos.y, NULL);
			POINT Pos2 = Neighbor[i]->GetPosition();
			Pos2.x += pan.x;
			Pos2.y += pan.y;
			LineTo(hDC, Pos2.x, Pos2.y);
			DeleteObject(hPen);
		}
	}

	if(bConnected){
		hBrush = CreateSolidBrush(RGB(255, 255, 10));
		Rect.top = Pos.y - (LinkSize / 2) - 1;
		Rect.left = Pos.x - (LinkSize / 2) - 1;
		Rect.right = Pos.x + (LinkSize / 2) + 1;
		Rect.bottom = Pos.y + (LinkSize / 2) + 1;
		FillRect(hDC, &Rect, hBrush);
		DeleteObject(hBrush);
	}

	if(mouse.x >= (Pos.x - (LinkSize / 2)) && mouse.y >= (Pos.y - (LinkSize / 2)) && mouse.x < (Pos.x + (LinkSize / 2)) && mouse.y < (Pos.y + (LinkSize / 2))){
		TOOLINFO TI;
		TI.cbSize = sizeof(TOOLINFO);
		TI.uFlags = TTF_SUBCLASS;
		TI.hwnd = hWndg;
		TI.uId = 0;
		TI.rect.top = Pos.y - (LinkSize / 2);
		TI.rect.left = Pos.x - (LinkSize / 2);
		TI.rect.bottom = Pos.y + (LinkSize / 2);
		TI.rect.right = Pos.x + (LinkSize / 2);
		TI.hinst = NULL;
		CHAR Temp[128];
		sprintf(Temp, "%s", Hostname.c_str());
		TI.lpszText = Temp;
		SendMessage(hWndTooltip, TTM_SETTOOLINFO, 0, (LPARAM) (LPTOOLINFO) &TI);

		hBrush = CreateSolidBrush(RGB(10, 255, 10));
		Rect.top = Pos.y - (LinkSize / 2) - 1;
		Rect.bottom = Pos.y + (LinkSize / 2) + 1;
		Rect.left = Pos.x - (LinkSize / 2) - 1;
		Rect.right = Pos.x + (LinkSize / 2) + 1;
		FillRect(hDC, &Rect, hBrush);
		DeleteObject(hBrush);

		SelectedLink = this;
	}

	COLORREF Color;
	switch(State){
		case NEUTRAL: Color = RGB(10, 220, 10); break;
		case CONNECTING: Color = RGB(220, 10, 10); break;
		case UNREACHABLE: Color = RGB(10, 10, 10); break;
		case WAITING: Color = RGB(10, 10, 220); break;
		case UPDATED: Color = RGB(10, 220, 220); break;
	}

	hBrush = CreateSolidBrush(Color);
	Rect.top = Pos.y - (LinkSize / 2);
	Rect.left = Pos.x - (LinkSize / 2);
	Rect.right = Pos.x + (LinkSize / 2);
	Rect.bottom = Pos.y + (LinkSize / 2);
	FillRect(hDC, &Rect, hBrush);
	DeleteObject(hBrush);

	HFONT hFont = CreateFont(8,6,0,0,0,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,NULL,DEFAULT_PITCH,"Lucida Console");
	SetBkColor(hDC, Color);
	SetTextColor(hDC, RGB(255 - GetRValue(Color), 255 - GetGValue(Color), 255 - GetBValue(Color)));
	SelectObject(hDC, hFont);
	char Clients[8];
	itoa(Link::Clients, Clients, 10);
	ExtTextOut(hDC, (Pos.x - (LinkSize / 2)) + 1, (Pos.y - (LinkSize / 2)) + 1, 0, &Rect, Clients, strlen(Clients), NULL);
	DeleteObject(hFont);
	*/
}

VOID Link::Rotate(FLOAT x, FLOAT y){
	D3DXMATRIX TransformMatrix, TransformMatrix2;
	D3DXMatrixRotationX(&TransformMatrix, x);
	D3DXMatrixRotationY(&TransformMatrix2, y);
	D3DXMatrixMultiply(&TransformMatrix, &TransformMatrix, &TransformMatrix2);
	D3DXMatrixMultiply(&RotationMatrix, &RotationMatrix, &TransformMatrix);
}

Link* Link::Add(PCHAR Name){
	UINT Radius = 1;
	INT Distance = 1;
	BOOL Failed;
	D3DVECTOR Position;
	do{
		Failed = FALSE;
		for(UINT i = 0; i < 3; i++){
			Position.x = (rand_r(0, 100) / (FLOAT)100);
			Position.y = (rand_r(0, 100) / (FLOAT)100);
			Position.z = sqrt(abs((FLOAT)1 - ((Position.x * Position.x) + (Position.y * Position.y))));
			
			Position.x *= rand_r(0, 1) ? -Distance : Distance;
			Position.y *= rand_r(0, 1) ? -Distance : Distance;
			Position.z *= rand_r(0, 1) ? -Distance : Distance;
			
			for(UINT i = 0; i < Links.List.size(); i++){
				D3DVECTOR TempPos = Links.List[i]->GetPosition();
				if(TempPos.x <= Position.x + Radius && TempPos.x >= Position.x - Radius){
				if(TempPos.y <= Position.y + Radius && TempPos.y >= Position.y - Radius){
				if(TempPos.z <= Position.z + Radius && TempPos.z >= Position.z - Radius){
					Failed = TRUE;
					break;
				}}}
			}
		}
		if(Failed)
			Distance += 2;
	}while(Failed);
	

	Link* NewLink = new Link(Name);
	NewLink->SetPosition(Position);
	Neighbor.push_back(NewLink);

	Display.ZoomFit();

	return NewLink;
}

Links::Links(){
	Head = NULL;
}

Link* Links::GetHead(VOID){
	if(!Head){
		Head = new Link("null");
		D3DVECTOR P = {0, 0, 0};
		Head->SetPosition(P);
		List.push_back(Head);
		/*for(UINT i = 0; i < 4; i++){
			Link* Link = Add(Head, i);
			CHAR Hostname[16];
			sprintf(Hostname, "%d", i);
			Link->SetHostname(Hostname);
		}*/
		D3DXMatrixIdentity(&RotationMatrix);
	}
	return Head;
}

VOID Links::Draw(VOID){
	for(UINT i = 0; i < List.size(); i++)
		List[i]->Draw();
}

VOID Links::Rotate(FLOAT x, FLOAT y){
	for(UINT i = 0; i < List.size(); i++)
		List[i]->Rotate(x, y);
}

VOID Links::Invalidate(VOID){
	for(UINT i = 0; i < List.size(); i++)
		List[i]->Invalidate();
}

VOID Links::Validate(VOID){
	for(UINT i = 0; i < List.size(); i++)
		List[i]->Validate();
}

VOID Links::ClearAll(VOID){
	for(UINT i = 0; i < List.size(); i++)
		delete List[i];
	List.clear();
	Head = NULL;
}

VOID Links::Remove(Link* Link){
	RemoveNeighbors(Link);
	List.erase(std::find(List.begin(), List.end(), Link));
	delete Link;
}

VOID Links::RemoveNeighbors(Link* Link){
	for(UINT i = 0; i < Link->Neighbor.size(); i++){
		//RemoveNeighbors(Link->Neighbor[i]);
		if(Link->Neighbor[i]->Neighbor.size() == 0){
			if(!IsLinkedTo(Link->Neighbor[i])){
				List.erase(std::find(List.begin(), List.end(), Link->Neighbor[i]));
				delete Link->Neighbor[i];
			}
		}
	}
	Link->Neighbor.clear();
}

BOOL Links::IsLinkedTo(Link* Link){
	for(UINT i1 = 0; i1 < List.size(); i1++)
		for(UINT i2 = 0; i2 < List[i1]->Neighbor.size(); i2++)
			if(List[i1]->Neighbor[i2] == Link)
				return TRUE;
	return FALSE;
}

Link* Links::Add(Link* From, PCHAR Name){
	CHAR Encoded[256];
	for(UINT i = 0; i < List.size(); i++){
		LinkCache::EncodeName(Encoded, sizeof(Encoded), List[i]->GetHostname(), List[i]->GetRemotePort());
		if(strcmp(Encoded, Name) == 0){
			From->Neighbor.push_back(List[i]);
			return List[i];
		}
	}
	Link* NewLink = From->Add(Name);
	List.push_back(NewLink);
	return NewLink;
}

RingAnim::RingAnim(){
	Time = 1000;
	Timeout.SetTimeout(Time);
}

D3DXVECTOR3 RingAnim::GetRotationAxis(VOID){
	if(Timeout.TimedOut()){
		Timeout.Reset();
		RotationAxis.x = ((FLOAT)rand_r(0, 100) / 100) * (rand_r(0, 1) == 1 ? 1 : -1);
		RotationAxis.y = ((FLOAT)rand_r(0, 100) / 100) * (rand_r(0, 1) == 1 ? 1 : -1);
		RotationAxis.z = ((FLOAT)rand_r(0, 100) / 100) * (rand_r(0, 1) == 1 ? 1 : -1);
	}
	return RotationAxis;
}

FLOAT RingAnim::GetRotationAngle(VOID){
	return ((FLOAT)Timeout.GetElapsedTime() / Time) * (2 * D3DX_PI);
}

/*
extern POINT mouse, pan;
extern INT Width, Height;
extern HWND hWndTooltip, hWndg;
static LinkEx *SelectedLink;

VOID LinkEx::Draw(HDC hDC){
	RECT R;
	HBRUSH hBrushTemp;
	COLORREF Color;

	COLORREF Waiting = RGB(160,60,160);
	COLORREF Connecting = RGB(160,60,60);
	COLORREF Connected = RGB(60,160,60);
	COLORREF Updated = RGB(60,60,160);
	COLORREF Refused = RGB(60, 60, 60);

	POINT Pos = {Position.x + pan.x, Position.y + pan.y};

	Color = Waiting;

	if(this->GetStatus() & LINK_STATUS_WAITING)
		Color = Waiting;
	if(this->GetStatus() & LINK_STATUS_CONNECTED)
		Color = Connected;
	if(this->GetStatus() & LINK_STATUS_UPDATED)
		Color = Updated;
	if(this->GetStatus() & LINK_STATUS_CONNECTIONREFUSED)
		Color = Refused;
	if(this->GetStatus() & LINK_STATUS_CONNECTING)
		Color = Connecting;
	

	if(mouse.x >= Pos.x && mouse.y >= Pos.y && mouse.x < Pos.x + 16 && mouse.y < Pos.y + 16){
		TOOLINFO TI;
		TI.cbSize = sizeof(TOOLINFO);
		TI.uFlags = TTF_SUBCLASS;
		TI.hwnd = hWndg;
		TI.uId = 0;
		TI.rect.top = Pos.y;
		TI.rect.left = Pos.x;
		TI.rect.bottom = Pos.y + 16;
		TI.rect.right = Pos.x + 16;
		TI.hinst = NULL;
		LPSTR Temp = new CHAR[80];
		sprintf(Temp, "%s", inet_ntoa(Stoin(this->GetAddr())));
		TI.lpszText = Temp;
		SendMessage(hWndTooltip, TTM_SETTOOLINFO, 0, (LPARAM) (LPTOOLINFO) &TI);

		hBrushTemp = CreateSolidBrush(RGB(10,255,10));
		R.top = Pos.y - 1;
		R.bottom = Pos.y + 18;
		R.left = Pos.x - 1;
		R.right = Pos.x + 18;
		FillRect(hDC, &R, hBrushTemp);
		DeleteObject(hBrushTemp);

		SelectedLink = this;
	}

	hBrushTemp = CreateSolidBrush(Color);
	R.top = Pos.y;
	R.bottom = Pos.y + 17;
	R.left = Pos.x;
	R.right = Pos.x + 17;
	FillRect(hDC, &R, hBrushTemp);
	DeleteObject(hBrushTemp);

	HFONT hFont = CreateFont(8,6,0,0,0,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,NULL,DEFAULT_PITCH,"Lucida Console");
	SetBkColor(hDC, Color);
	SetTextColor(hDC, RGB(255,255,255));
	SelectObject(hDC, hFont);
	char Clients[8];
	itoa(this->NumOfClients(), Clients, 10);
	ExtTextOut(hDC, Pos.x + 1, Pos.y + 1, 0, &R, Clients, strlen(Clients), NULL);
	DeleteObject(hFont);

	if(this->L[UP]){
		Color = RGB(128,128,128);
		hBrushTemp = CreateSolidBrush(Color);
		R.top = Pos.y - 8;
		R.bottom = Pos.y;
		R.left = Pos.x + 7;
		R.right = Pos.x + 10;
		FillRect(hDC, &R, hBrushTemp);
		DeleteObject(hBrushTemp);
	}
	if(this->L[RIGHT]){
		Color = RGB(128,128,128);
		hBrushTemp = CreateSolidBrush(Color);
		R.top = Pos.y + 7;
		R.bottom = Pos.y + 10;
		R.left = Pos.x + 17;
		R.right = Pos.x + 25;
		FillRect(hDC, &R, hBrushTemp);
		DeleteObject(hBrushTemp);
	}
	if(this->L[DOWN]){
		Color = RGB(128,128,128);
		hBrushTemp = CreateSolidBrush(Color);
		R.top = Pos.y + 17;
		R.bottom = Pos.y + 25;
		R.left = Pos.x + 7;
		R.right = Pos.x + 10;
		FillRect(hDC, &R, hBrushTemp);
		DeleteObject(hBrushTemp);
	}
	if(this->L[LEFT]){
		Color = RGB(128,128,128);
		hBrushTemp = CreateSolidBrush(Color);
		R.top = Pos.y + 7;
		R.bottom = Pos.y + 10;
		R.left = Pos.x - 8;
		R.right = Pos.x;
		FillRect(hDC, &R, hBrushTemp);
		DeleteObject(hBrushTemp);
	}
}

LinkEx* Links::Add(LinkEx & From, BYTE Side, ULONG Addr, BYTE Clients){
	if(Side >= 0 && Side <= 3){
		From.L[Side] = new LinkEx;
		From.L[Side]->SetAddr(Addr);
		From.L[Side]->SetNumClients(Clients);
		POINT NewPos = From.GetPosition();
		switch(Side){
			case 0: NewPos.y -= 33; break;
			case 1: NewPos.x += 33; break;
			case 2: NewPos.y += 33; break;
			case 3: NewPos.x -= 33; break;
		}
		From.L[Side]->SetPosition(NewPos);
		List.push_back(From.L[Side]);
		return From.L[Side];
	}
}

VOID Links::Draw(HDC hDC){
	BitBlt(hDC,0,0,Width,Height,NULL,0,0,BLACKNESS);
	for(std::vector<LinkEx*>::iterator i = List.begin(); i != List.end(); i++)
		(*i)->Draw(hDC);
}

LinkEx *Links::GetWaiting(VOID){
	LinkEx *Temp = NULL;
	for(std::vector<LinkEx*>::iterator i = List.begin(); i != List.end(); i++){
		if((*i)->GetStatus() & LINK_STATUS_WAITING){
		Temp = (*i);
		break;
		}
	}
	return Temp;
}

LinkEx *Links::FindHost(ULONG Host){
	LinkEx *Temp = NULL;
	for(std::vector<LinkEx*>::iterator i = List.begin(); i != List.end(); i++){
		if((*i)->GetAddr() == Host){
		Temp = (*i);
		break;
		}
	}
	return Temp;
}*/