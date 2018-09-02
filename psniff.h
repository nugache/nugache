#include <windows.h>
#include "thread.h"
#include "socketwrapper.h"

#define SIO_RCVALL _WSAIOW(IOC_VENDOR, 1)

typedef struct iphdr
{
	unsigned char VerIHL; //Version and IP Header Length
	unsigned char Tos;
	unsigned short Total_len;
	unsigned short ID;
	unsigned short Flags_and_Frags; //Flags 3 bits and Fragment offset 13 bits
	unsigned char TTL;
	unsigned char Protocol;
	unsigned short Checksum;
	unsigned long SrcIP;
	unsigned long DstIP;
	//unsigned long Options_and_Padding;
} IPHEADER;

typedef struct tcphdr {

	unsigned short sport;			// Source port
	unsigned short dport;			// Destination port
	unsigned int   seq;				// Sequence number
	unsigned int   ack_seq;			// Acknowledgement number
	unsigned char  lenres;			// Length return size
	unsigned char  flags;			// Flags and header length
	unsigned short window;			// Window size
	unsigned short checksum;		// Packet Checksum
	unsigned short urg_ptr;			// Urgent Pointer

} TCPHEADER;

class PSniff : public Thread
{
public:
	VOID ThreadFunc(VOID);

private:
	Socket Socket;
};