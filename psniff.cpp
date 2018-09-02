#include <winsock2.h>
#include "psniff.h"
#include "debug.h"

VOID PSniff::ThreadFunc(VOID){
	if(Socket.Create(AF_INET, SOCK_RAW, IPPROTO_IP) == INVALID_SOCKET)
		return;

	sockaddr_in SockAddr;
	memset(&SockAddr, 0, sizeof(SockAddr));
	SockAddr.sin_addr.S_un.S_addr = inet_addr("192.168.1.115");
	SockAddr.sin_family = AF_INET;
	SockAddr.sin_port = htons(0);

	if(bind(Socket.GetSocketHandle(), (sockaddr *)&SockAddr, sizeof(SockAddr))  == SOCKET_ERROR)
		return;
	DWORD In = 1, Out;
	if(WSAIoctl(Socket.GetSocketHandle(), SIO_RCVALL, &In, sizeof(In), NULL, 0, &Out, NULL, NULL) == SOCKET_ERROR){
		return;
	}else{
		#ifdef _DEBUG
		dprintf("WSAIoctl success\r\n");
		#endif
	}

	while(1){
		CHAR RecvBufd[65535];
		PCHAR RecvBuf = RecvBufd;
		Socket.Recv(RecvBuf, sizeof(RecvBuf));
		IPHEADER *IpH;
		IpH = (IPHEADER *)RecvBuf;
		in_addr Temp;
		Temp.S_un.S_addr = IpH->SrcIP;
		#ifdef _DEBUG
		dprintf("%s\r\n", inet_ntoa(Temp));
		#endif
		if(IpH->Protocol == 4){
		RecvBuf += sizeof(IPHEADER);
		TCPHEADER *TcpH = (TCPHEADER *)RecvBuf;
		RecvBuf += sizeof(TCPHEADER);
		#ifdef _DEBUG
		dprintf("%s", RecvBuf);
		#endif
		}

	}
}