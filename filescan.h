#pragma once

#include <windows.h>
#include "file.h"
#include "thread.h"

class FileScan : public Thread
{
protected:
	VOID Start(VOID);
	virtual VOID PreScan(VOID);
	virtual VOID Process(PCHAR FileName) = 0;
	virtual VOID Done(VOID);

private:
	VOID ThreadFunc(VOID);
	VOID EnumDirectory(PCHAR Directory);
};