#include <windows.h>
#include "debug.h"
#include "rand.h"

class Target
{
public:
	Target(PCHAR Format);
	ULONG GetNext(VOID);

private:
	UCHAR Min[4], Max[4];
	UCHAR Pos[4];
	BOOL Rand[4];
	UINT Octet;
	BOOL Final;
	UINT Random;
};