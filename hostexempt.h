#pragma once

#include <windows.h>
#include <vector>
#include "mutex.h"
#include "wildcard.h"

class HostExempts
{
public:
	~HostExempts();
	VOID Add(PCHAR Host);
	BOOL Matches(PCHAR Host);
	BOOL Exists(PCHAR Host);

private:
	std::vector<PCHAR> List;
	Mutex Mutex;
};