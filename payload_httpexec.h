#pragma once

#include <windows.h>
#include "scan.h"

class PayloadHTTPExec : public Payload
{
public:
	PayloadHTTPExec(PCHAR URL);
};