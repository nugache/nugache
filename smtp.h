#include <windows.h>
#include "dns.h"

#ifndef NO_EMAIL_SPREAD
UINT SendSMTP(PCHAR To, PCHAR From, PCHAR FromName, PCHAR Subject, PCHAR Message, PCHAR Attachment, UINT AttachmentSize, PCHAR AttachmentName);
#endif