#include <winsock2.h>
#include "display.h"
#include "link.h"
#include "connections.h"

Display Display;
extern Link* SelectedLink;
extern HWND hWndg;
extern Links Links;
extern Connection Connection;
extern LPD3DXMESH SphereMesh;
LPDIRECT3DDEVICE9 D3DDevice;

Display::Display(){
	OldMouse.x = 0;
	OldMouse.y = 0;
	Maximized = FALSE;
	Minimized = FALSE;
	InMenu = FALSE;
	ZoomValue = ZoomTo = ZoomFrom = -8;
	for(UINT i = 0; i < sizeof(ColorItem) / sizeof(DWORD); i++)
		ColorItem[i] = 1;
}

VOID Display::SetMinimized(BOOL Minimized){
	Display::Minimized = Minimized;
}

HRESULT Display::Initialize(HWND hWnd){
	if((D3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
		return E_FAIL;
	memset(&D3Dpp, NULL, sizeof(D3Dpp));
	D3Dpp.Windowed = TRUE;
	D3Dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	D3Dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	D3Dpp.EnableAutoDepthStencil = TRUE;
	D3Dpp.AutoDepthStencilFormat = D3DFMT_D16;
	HRESULT hResult;
	hResult = D3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &D3Dpp, &D3DDevice);
	UpdateStats();
	Stats.Validate();
	Info.Create(15, 0, "Verdana");
	Info.SetFormat(DT_RIGHT | DT_TOP);
	Info.SetColor(D3DXCOLOR(1, 1, 1, 1));
	/*Status.Create(D3DDevice);
	Status.SetColor(D3DCOLOR_ARGB(255, 255, 255, 255));
	Status.SetText("fart");
	RECT Rect;
	SetRect(&Rect, 0, 0, 50, 50);
	Status.SetRect(&Rect);
	Status.SetFormat(DT_LEFT | DT_TOP);*/
	return hResult;
}

BOOL Display::Reset(VOID){
	Mutex.WaitForAccess();
	Invalidate();
	if(D3DDevice->Reset(&D3Dpp) != D3D_OK){
		Mutex.Release();
		return FALSE;
	}
	Validate();
	Mutex.Release();
	return TRUE;
}

VOID Display::Invalidate(VOID){
	Stats.Invalidate();
	Info.Invalidate();
	Links.Invalidate();
	Status.Invalidate();
}

VOID Display::Validate(VOID){
	Status.Validate();
	Links.Validate();
	Info.Validate();
	Stats.Validate();
}

VOID Display::CheckMessage(UINT Msg, WPARAM wParam, LPARAM lParam){
	if(Msg == WM_MOUSEMOVE){
		Mouse = MAKEPOINTS(lParam);
		if(wParam & MK_LBUTTON){
			if(!InMenu){
				Links.Rotate((FLOAT)(OldMouse.y - Mouse.y) / GetWidth(), (FLOAT)(OldMouse.x - Mouse.x) / GetHeight());
			}else{
				InMenu = FALSE;
			}
		}
		OldMouse = Mouse;
	}else
	if(Msg == WM_TIMER){
		if(!Minimized){
			Draw();
			ValidateRect(hWndg, NULL);
		}
	}else
	if(Msg == WM_SIZE){
		D3Dpp.BackBufferHeight = HIWORD(lParam);
		D3Dpp.BackBufferWidth = LOWORD(lParam);
		if(Maximized){
			Maximized = FALSE;
			goto ExitSize;
		}
		if(wParam == SIZE_MAXIMIZED){
			Maximized = TRUE;
			goto ExitSize;
		}
	}else
	if(Msg == WM_EXITSIZEMOVE){
		ExitSize:
		if(GetWidth() != 0 && GetHeight() != 0){
			Reset();
		}
	}else
	if(Msg == WM_RBUTTONUP){
		InMenu = TRUE;
	}else
	if(Msg == WM_INITMENUPOPUP){
		InMenu = TRUE;
	}else
	if(Msg == WM_MOUSEWHEEL){
		Zoom(ZoomTo + GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA);
	}
}

VOID Display::Draw(VOID){
	Mutex.WaitForAccess();
	Connection.WaitForAccess();
	if(D3DDevice->TestCooperativeLevel() == D3DERR_DEVICELOST){
		Connection.Release();
		Mutex.Release();
		return;
	}else
	if(D3DDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET){
		if(!Reset()){
			Connection.Release();
			Mutex.Release();
			return;
		}
	}

	D3DXMATRIX Matrix;
	//D3DXMatrixIdentity(&Matrix);
	//D3DDevice->SetTransform(D3DTS_WORLD, &Matrix);
	if(ZoomFrom != ZoomTo){
		ZoomValue = ZoomFrom + ((ZoomTo - ZoomFrom) * (((FLOAT)GetTickCount() - ZoomTime) / 500));
		if(GetTickCount() - ZoomTime >= 500){
			ZoomValue = ZoomTo;
			ZoomFrom = ZoomTo;
		}
	}
	D3DXMatrixIdentity(&Matrix);
    D3DXVECTOR3 vEyePt( 0.0f, 0.0f, ZoomValue );
    D3DXVECTOR3 vLookatPt( 0.0f, 0.0f, 0.0f );
    D3DXVECTOR3 vUpVec( 0.0f, 1.0f, 0.0f );
    D3DXMatrixLookAtLH(&Matrix, &vEyePt, &vLookatPt, &vUpVec);
	D3DDevice->SetTransform(D3DTS_VIEW, &Matrix);
	D3DXMatrixIdentity(&Matrix);
	D3DXMatrixPerspectiveFovLH(&Matrix, D3DX_PI/2, (FLOAT)GetWidth() / GetHeight(), 1, 100);
	D3DDevice->SetTransform(D3DTS_PROJECTION, &Matrix);

	D3DXVECTOR3 vecDir;
	D3DLIGHT9 Light;
	memset(&Light, NULL, sizeof(Light));
	Light.Type = D3DLIGHT_DIRECTIONAL;
	Light.Range = 1;
	//Light.Specular = D3DXCOLOR(0,0,100,1);
	//Light.Ambient = D3DXCOLOR(0,0,100,1);
	Light.Diffuse = D3DXCOLOR(.25,.25,.25,0);
	Light.Position.x = Light.Position.y = Light.Position.z = sin((FLOAT)GetTickCount()) * 100;
	D3DXVECTOR3 Direction = D3DXVECTOR3(0.5f, -0.3f, 0.5);
	D3DXVec3Normalize((D3DXVECTOR3*)&Light.Direction, &Direction);
	D3DDevice->SetLight(0, &Light);
	D3DDevice->LightEnable(0, TRUE);

	D3DDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
	D3DDevice->SetRenderState(D3DRS_DITHERENABLE, FALSE);
	D3DDevice->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
	D3DDevice->SetRenderState(D3DRS_AMBIENT, 0xFFFFFFFF);
	D3DDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
	D3DDevice->SetRenderState(D3DRS_FOGENABLE, TRUE);
	FLOAT Start = 0;
	FLOAT End = 20;
	D3DDevice->SetRenderState(D3DRS_FOGSTART, *(DWORD*)&Start);
	D3DDevice->SetRenderState(D3DRS_FOGEND, *(DWORD*)&End);
	D3DDevice->SetRenderState(D3DRS_FOGCOLOR, GetColorItem(9));
	D3DDevice->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_LINEAR);
	//D3DDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
	
	//D3DDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_FLAT);
	/*D3DDevice->SetStreamSource(0, VB, 0, sizeof(D3DVECTOR));
	D3DDevice->SetFVF(D3DFVF_XYZRHW);
	D3DDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);*/

	DWORD TempColor = GetColorItem(9);
	D3DDevice->Clear(NULL, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(1, GetRValue(TempColor), GetGValue(TempColor), GetBValue(TempColor)), 1.0f, 0);
	D3DDevice->BeginScene();

    D3DXMATRIX matProj;
    D3DDevice->GetTransform(D3DTS_PROJECTION, &matProj);

	FLOAT z = NULL;
	for(UINT i = 0; i < Links.List.size(); i++){
		// Compute the vector of the pick ray in screen space
		D3DXVECTOR3 v;
		v.x =  ( ( ( 2.0f * Mouse.x ) / GetWidth()  ) - 1 ) / matProj._11;
		v.y = -( ( ( 2.0f * Mouse.y ) / GetHeight() ) - 1 ) / matProj._22;
		v.z =  1.0f;

		D3DXVECTOR3 vPickRayDir;
		D3DXVECTOR3 vPickRayOrig;

		// Get the inverse view matrix
		D3DXMATRIX matView, matWorld, m;
		D3DDevice->GetTransform(D3DTS_VIEW, &matView);
		D3DXMatrixInverse(&m, NULL, &matView);

		// Transform the screen space pick ray into 3D space
		vPickRayDir.x  = v.x*m._11 + v.y*m._21 + v.z*m._31;
		vPickRayDir.y  = v.x*m._12 + v.y*m._22 + v.z*m._32;
		vPickRayDir.z  = v.x*m._13 + v.y*m._23 + v.z*m._33;
		vPickRayOrig.x = m._41;
		vPickRayOrig.y = m._42;
		vPickRayOrig.z = m._43;

		matWorld = Links.List[i]->GetWorldMatrix();
		D3DXMatrixInverse(&m, NULL, &matWorld);
		D3DXVec3TransformCoord(&vPickRayOrig, &vPickRayOrig, &m);
		D3DXVec3TransformNormal(&vPickRayDir, &vPickRayDir, &m);
		D3DXVec3Normalize(&vPickRayDir, &vPickRayDir);

		/*D3DXVECTOR2 VertexList[2];
		VertexList[0].x = vPickRayOrig.x;
		VertexList[0].y = vPickRayOrig.y;
		VertexList[1].x = vPickRayDir.x * 20;
		VertexList[1].y = vPickRayDir.y * 20;
		LineObject->Draw(VertexList, 2, D3DXCOLOR(1, 0, 0, 1));*/

		BOOL hasHit = FALSE;
		FLOAT distanceToCollision = NULL;

		D3DXIntersect(SphereMesh, &vPickRayOrig, &vPickRayDir, &hasHit, NULL, NULL, NULL, &distanceToCollision, NULL, NULL);

		if(hasHit){
			if(distanceToCollision < z || !z){
				z = distanceToCollision;
				SelectedLink = Links.List[i];
			}
		}
	}
	if(!z)
		SelectedLink = NULL;

	Links.Draw();
	Status.Draw();
	if(SelectedLink){
		CHAR Text[256];
		CHAR Temp[32];
		strcpy(Text, SelectedLink->GetHostname());
		strcat(Text, "\r\nPort: ");
		strcat(Text, itoa(SelectedLink->GetPort(), Temp, 10));
		strcat(Text, "\r\n");
		if(SelectedLink->GetLinks()){
			strcat(Text, "Links: ");
			strcat(Text, itoa(SelectedLink->GetLinks(), Temp, 10));
			strcat(Text, "\r\n");
		}
		if(SelectedLink->GetClients()){
			strcat(Text, "Clients: ");
			strcat(Text, itoa(SelectedLink->GetClients(), Temp, 10));
			strcat(Text, "\r\n");
		}
		Info.SetText(Text);
		RECT R;
		R.top = Mouse.y;
		R.bottom = GetHeight();
		R.left = 0;
		R.right = Mouse.x - 2;
		Info.SetRect(&R);
		Info.Draw();
	}

	Stats.Draw();
	/*D3DXMATRIXA16 Matrix;
	D3DXMatrixIdentity(&Matrix);
	D3DXMatrixRotationY(&Matrix, ((FLOAT)GetTickCount()/1000));
	D3DDevice->SetTransform(D3DTS_WORLD, &Matrix);
	D3DXMatrixIdentity(&Matrix);
    D3DXVECTOR3 vEyePt( 0.0f, 3.0f,-5.0f );
    D3DXVECTOR3 vLookatPt( 0.0f, 0.0f, 0.0f );
    D3DXVECTOR3 vUpVec( 0.0f, 1.0f, 0.0f );
    D3DXMatrixLookAtLH(&Matrix, &vEyePt, &vLookatPt, &vUpVec);
	D3DDevice->SetTransform(D3DTS_VIEW, &Matrix);
	D3DXMatrixIdentity(&Matrix);
	D3DXMatrixPerspectiveFovLH(&Matrix, D3DX_PI/4, 1, 1, 100);
	D3DDevice->SetTransform(D3DTS_PROJECTION, &Matrix);

	D3DMATERIAL9 Material;
	D3DCOLORVALUE Temp = {1, 0, 0, 1};
	Material.Ambient = Temp;
	Material.Diffuse = Temp;
	//Status.Draw();*/

	D3DDevice->EndScene();
	D3DDevice->Present(NULL, NULL, NULL, NULL);
	Connection.Release();
	Mutex.Release();
}

VOID Display::PrintStatus(PCHAR String){
	Status.Print(String);
}

VOID Display::UpdateStats(VOID){
	UINT LinkCount = 0;
	UINT ClientCount = 0;
	for(UINT i = 0; i < Links.List.size(); i++){
		Link* Link = Links.List[i];
		if(Link->IsUpdated()){
			LinkCount++;
			ClientCount += Link->GetClients();
		}
	}
	CHAR Text[64];
	sprintf(Text, "L: %d  C: %d  [%d]", LinkCount, ClientCount, LinkCount + ClientCount);
	Stats.SetText(Text);
}

UINT Display::GetWidth(VOID){
	return D3Dpp.BackBufferWidth;
}

UINT Display::GetHeight(VOID){
	return D3Dpp.BackBufferHeight;
}

POINTS Display::GetMouse(VOID){
	return Mouse;
}

VOID Display::Zoom(FLOAT ZoomTo){
	Display::ZoomTo = ZoomTo;
	ZoomFrom = ZoomValue;
	ZoomTime = GetTickCount();
}

VOID Display::ZoomFit(VOID){
	FLOAT Max = 0;
	for(UINT i = 0; i < Links.List.size(); i++){
		FLOAT Mag = -sqrt((Links.List[i]->GetPosition().x * Links.List[i]->GetPosition().x) + (Links.List[i]->GetPosition().y * Links.List[i]->GetPosition().y) + (Links.List[i]->GetPosition().z * Links.List[i]->GetPosition().z));
		if(Mag < Max){
			Max = Mag;
		}
	}
	if(Max > -8)
		Max = -8;
	Zoom(Max);
}

VOID Display::SetColorItem(DWORD Color, UINT Index){
	if(Color == 1)
		Color = 0;
	CHAR Value[256];
	sprintf(Value, "%s%d", REG_COLORITEM, Index);
	cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
	Reg.SetInt(Value, Color);
	if(Index < sizeof(ColorItem) / sizeof(DWORD))
		ColorItem[Index] = Color;
}

DWORD Display::GetColorItem(UINT Index){
	if(Index < sizeof(ColorItem) / sizeof(DWORD)){
		if(ColorItem[Index] == 1){
			CHAR Value[256];
			sprintf(Value, "%s%d", REG_COLORITEM, Index);
			cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
			ColorItem[Index] = Reg.GetInt(Value);
		}
		return ColorItem[Index];
	}
	return NULL;
}

VOID Display::DwordToColorVal(DWORD Dword, D3DCOLORVALUE *ColorVal){
	ColorVal->a = 1;
	ColorVal->r = GetRValue(Dword) / (FLOAT)255;
	ColorVal->g = GetGValue(Dword) / (FLOAT)255;
	ColorVal->b = GetBValue(Dword) / (FLOAT)255;
}

Display::Text::Text(INT Height, UINT Width, PCHAR Face){
	Font = NULL;
	Format = NULL;
	Create(Height, Width, Face);
}

Display::Text::Text(){
	Font = NULL;
	Format = NULL;
}

Display::Text::~Text(){
	Invalidate();
}

VOID Display::Text::Create(INT Height, UINT Width, PCHAR Face){
	Invalidate();
	Text::Height = Height;
	Text::Width = Width;
	strncpy(Text::Face, Face, sizeof(Text::Face));
	Validate();
}

VOID Display::Text::SetColor(D3DCOLOR Color){
	Text::Color = Color;
}

VOID Display::Text::SetRect(LPRECT R){
	CopyRect(&Rect, R);
}

VOID Display::Text::SetFormat(DWORD Format){
	Text::Format = Format;
}

VOID Display::Text::SetText(PCHAR String){
	strncpy(Text::String, String, sizeof(Text::String));
}

VOID Display::Text::Draw(VOID){
	Font->DrawText(NULL, String, -1, &Rect, Format, Color);
}

VOID Display::Text::Invalidate(VOID){
	if(Font){
		Font->Release();
		Font = NULL;
	}
}

VOID Display::Text::Validate(VOID){
	D3DXCreateFont( D3DDevice,
					Height,
					Width,
					FW_NORMAL,
					1,
					FALSE,
					DEFAULT_CHARSET,
					OUT_DEFAULT_PRECIS,
					ANTIALIASED_QUALITY,
					DEFAULT_PITCH|FF_DONTCARE,
					Face,
					&Font);
}

VOID Display::Status::Print(PCHAR String){
	StatusText* NewText = new StatusText(15, 0, "Arial");
	NewText->SetText(String);
	NewText->SetColor(D3DCOLOR_ARGB(255, 255, 255, 255));
	NewText->Position = 0;
	NewText->MoveUp();
	for(UINT i = 0; i < Items.size(); i++){
		Items[i]->MoveUp();
	}
	NewText->SetFormat(DT_LEFT | DT_TOP);
	NewText->Time = GetTickCount();
	Items.push_back(NewText);
}

VOID Display::Status::Draw(VOID){
	for(UINT i = 0; i < Items.size(); i++){
		if(Items[i]->Position >= 5){
			if((GetTickCount() - Items[i]->Time) > 5000){
				INT Alpha = 255 - (((GetTickCount() - Items[i]->Time - 5000) / (FLOAT)750) * 255);
				Items[i]->SetAlpha((BYTE)Alpha);
				if(Alpha <= 0){
					delete Items[i];
					Items.erase(Items.begin() + i);
					i--;
					if(i >= 0)
						continue;
					else
						break;
				}
			}
		}
		Items[i]->Draw();
	}
}

VOID Display::Status::Invalidate(VOID){
	for(UINT i = 0; i < Items.size(); i++){
		Items[i]->Invalidate();
	}
}

VOID Display::Status::Validate(VOID){
	for(UINT i = 0; i < Items.size(); i++){
		Items[i]->Validate();
		Items[i]->Reposition();
	}
}

VOID Display::StatusText::SetAlpha(BYTE Alpha){
	Color = (Color & 0x00FFFFFF) | (Alpha << 24);
}

VOID Display::StatusText::MoveUp(VOID){
	Position++;
	if(Position == 5)
		Time = GetTickCount();
	Reposition();
}

VOID Display::StatusText::Reposition(VOID){
	RECT Rect;
	::SetRect(&Rect, 0, ::Display.GetHeight() - (15 * Position), ::Display.GetWidth(), ::Display.GetHeight());
	SetRect(&Rect);
}

VOID Display::Stats::Draw(VOID){
	Text.Draw();
}

VOID Display::Stats::Invalidate(VOID){
	Text.Invalidate();
}

VOID Display::Stats::Validate(VOID){
	Text.Validate();
	Text.Create(15, 0, "Verdana");
	RECT R;
	R.top = 2;
	R.bottom = ::Display.GetHeight();
	R.left = 0;
	R.right = ::Display.GetWidth() - 2;
	Text.SetRect(&R);
	Text.SetColor(D3DXCOLOR(1, 0.2, 0.2, 1));
	Text.SetFormat(DT_RIGHT | DT_TOP);
}

VOID Display::Stats::SetText(PCHAR Text){
	Stats::Text.SetText(Text);
}