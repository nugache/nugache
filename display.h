#pragma once

#include <windows.h>
#include <D3DX9.h>
#include <vector>
#include "mutex.h"
#include "debug.h"

class Display
{
public:
	Display();
	HRESULT Initialize(HWND hWnd);
	BOOL Reset(VOID);
	VOID Invalidate(VOID);
	VOID Validate(VOID);
	VOID CheckMessage(UINT Msg, WPARAM wParam, LPARAM lParam);
	VOID ExitSize(VOID);
	VOID Draw(VOID);
	VOID PrintStatus(PCHAR String);
	VOID UpdateStats(VOID);
	UINT GetWidth(VOID);
	UINT GetHeight(VOID);
	POINTS GetMouse(VOID);
	VOID Zoom(FLOAT ZoomTo);
	VOID ZoomFit(VOID);
	VOID SetColorItem(DWORD Color, UINT Index);
	DWORD GetColorItem(UINT Index);
	VOID DwordToColorVal(DWORD Dword, D3DCOLORVALUE *ColorVal);
	VOID SetMinimized(BOOL Minimized);

	class Text
	{
	public:
		Text();
		Text(INT Height, UINT Width, PCHAR Face);
		~Text();
		VOID Create(INT Height, UINT Width, PCHAR Face);
		VOID Draw(VOID);
		VOID SetColor(D3DCOLOR Color);
		VOID SetRect(LPRECT R);
		VOID SetFormat(DWORD Format);
		VOID SetText(PCHAR String);
		VOID Invalidate(VOID);
		VOID Validate(VOID);

	protected:
		INT Height;
		UINT Width;
		CHAR Face[128];
		LPD3DXFONT Font;
		RECT Rect;
		D3DCOLOR Color;
		DWORD Format;
		CHAR String[256];
	};

private:
	class StatusText : public Text
	{
	public:
		StatusText(INT Height, UINT Width, PCHAR Face) : Text(Height, Width, Face) {};
		VOID SetAlpha(BYTE Alpha);
		VOID MoveUp(VOID);
		VOID Reposition(VOID);
		DWORD Time;
		UINT Position;
	};

	class Status
	{
	public:
		VOID Print(PCHAR String);
		VOID Draw(VOID);
		VOID Invalidate(VOID);
		VOID Validate(VOID);

	private:
		std::vector<StatusText*> Items;
	};

	class Stats
	{
	public:
		VOID Draw(VOID);
		VOID Invalidate(VOID);
		VOID Validate(VOID);
		VOID SetText(PCHAR Text);

	private:
		Text Text;
	};

	LPDIRECT3D9 D3D;
	LPDIRECT3DVERTEXBUFFER9 VB;
	D3DPRESENT_PARAMETERS D3Dpp;
	POINTS Mouse, OldMouse;
	FLOAT ZoomValue, ZoomTo, ZoomFrom;
	DWORD ColorItem[9];
	DWORD ZoomTime;
	BOOL Maximized;
	BOOL InMenu;
	BOOL Minimized;
	Mutex Mutex;
	Status Status;
	Text Info;
	Stats Stats;
};