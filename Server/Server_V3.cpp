#include <thread>
#include <conio.h>
#include <mutex>
#include <iostream>
#include "Logger.h"
#define BUF_LEN 10
#define PORT 52341
#define TIME_LENGTH 30

using namespace newsockets;
using namespace logger;

FILE *fWarn, *fInfo, *fError;
bool needClose = false;
mutex mutexWarn, mutexInfo, mutexError, mutexNeedClose;

string Understand(string& str) 
{
	string ans("");
	for (int i = 0; i < str.length() && str[i] != ' '; ans += str[i++]);
	return ans;
}

void ReadTime(const char *str, time_t & t1, time_t & t2) 
{
	int h1, h2, m1, m2, s1, s2;
	time_t t = time(0);
	tm* temp = localtime(&t);
	if (sscanf_s(str, "request_since %d:%d:%d-%d:%d:%d", &h1, &m1, &s1, &h2, &m2, &s2) == 6) 
	{
		temp->tm_hour = h2;
		temp->tm_min = m2;
		temp->tm_sec = s2;
		t2 = mktime(temp);
	}
	temp->tm_hour = h1;
	temp->tm_min = m1;
	temp->tm_sec = s1;
	t1 = mktime(temp);
}

void ReadSavedMSGFromString(const string & source, int &pos, string &ans, tm * res)
{
	ans.clear();
	for (; pos < source.length() && source[pos] != '\n'; ++pos)
	{
		ans += source[pos];
	}
	++pos;
	sscanf_s(&ans[0], "%d:%d:%d", &res->tm_hour, &res->tm_min, &res->tm_sec);
}

string TakeFrom(const string & source, time_t t1, time_t t2) 
{
	string ans(""), msg;
	time_t t = time(0);
	tm* msgTime = localtime(&t);
	int i = 0;
	while (i < source.length())
	{
		ReadSavedMSGFromString(source, i, msg, msgTime);
		if (difftime(mktime(msgTime), t1) > 0 && difftime(t2, mktime(msgTime)) > 0)
		{
			ans += msg + '\n';
		}
	}
	return ans;
}

string Ipadrtostring(int ip, int port) 
{
	string ans("");
	char buff[4];
	memcpy(buff, &ip, 4); //4 - length of int
	sprintf(&ans[0], "%u.%u.%u.%u", buff[3], buff[2], buff[1], buff[0]);
	if (port > 0) 
	{
		ans += ':';
		ans += itoa(port, &ans[ans.length()], 10);
	}
	return ans;
}

void AddMessage(FILE * filestream, mutex & mutexFile, string warning, const time_t actime, string IPadr)
{
	mutexFile.lock();
		char time[TIME_LENGTH];
		tm* actual = localtime(&actime);
		strftime(time, TIME_LENGTH, "%X-%F", actual);
		fprintf(filestream, "%s %s %s\n", time, data(IPadr), data(warning));
	mutexFile.unlock();
}

string SendWarnings()
{
	FILE* fRead;
	mutexWarn.lock();
		fclose(fWarn);
		fRead = fopen("warnings", "r");
		fWarn = fopen("warnings", "a");
		string answer("");
		long long ch;
		while ((ch = fgetc(fRead)) != EOF)
		{
			answer += (char)ch;
		}
		fclose(fRead);
	mutexWarn.unlock();
	return answer;
}

string SendInfos()
{
	FILE* fRead;
	mutexInfo.lock();
		fclose(fInfo);
		fRead = fopen("infos", "r");
		fInfo = fopen("infos", "a");
		string answer("");
		long long ch;
		while ((ch = fgetc(fRead)) != EOF)
		{
			answer += (char)ch;
		}
		fclose(fRead);
	mutexInfo.unlock();
	return answer;
}

string SendErrors()// Very lazy to merge this three
{
	FILE* fRead;
	mutexError.lock();
		fclose(fError);
		fRead = fopen("errors", "r");
		fError = fopen("errors", "a");
		string answer("");
		long long ch;
		while ((ch = fgetc(fRead)) != EOF)
		{
			answer += (char)ch;
		}
		fclose(fRead);
	mutexError.unlock();
	return answer;
}

void ClearLog() 
{
	bool clearWarn = false, clearInf = false, clearErr = false;
	FILE* fRead;
	while (!(clearWarn && clearInf && clearErr))
	{
		if (!clearWarn && mutexWarn.try_lock())
		{
			fclose(fWarn);
			fRead = fopen("warnings", "w");
			fclose(fRead);
			fWarn = fopen("warnings", "a");
			clearWarn = true;
			mutexWarn.unlock();
		}
		if (!clearInf && mutexInfo.try_lock())
		{
			fclose(fInfo);
			fRead = fopen("infos", "w");
			fclose(fRead);
			fInfo = fopen("infos", "a");
			clearInf = true;
			mutexInfo.unlock();
		}
		if (!clearErr && mutexError.try_lock())
		{
			fclose(fError);
			fRead = fopen("errors", "w");
			fclose(fRead);
			fError = fopen("errors", "a");
			clearErr = true;
			mutexError.unlock();
		}
	}
}



string ConstructAnswer(time_t down, time_t up)
{
	return TakeFrom(SendWarnings(), down, up) + TakeFrom(SendInfos(), down, up) + TakeFrom(SendErrors(), down, up);

}

bool AlreadyClosed()
{
	bool ans = false;
	mutexNeedClose.lock();
		ans = needClose;
	mutexNeedClose.unlock();
	return ans;
}

void CloseServer()
{
	mutexNeedClose.lock();
		needClose = true;
	mutexNeedClose.unlock();
}

void CloseControl()
{
	while (true)
	{
		if (kbhit() != 0 && getch() == 'e')
		{
			CloseServer();
			return;
		}
		Sleep(500);
	}
}

void Work(int myport = PORT) 
{
	EasyInterface netInterface;
	string strmsg, command;
	int ipClient;
	netInterface.Bind(myport);
	while (!AlreadyClosed())
	{
		if (netInterface.ISReciveMSG(strmsg, ipClient))
		{
			command = Understand(strmsg);
			if (command == "request_since") 
			{
				time_t tstart = time(0), tend = time(0);
				ReadTime(&strmsg[0], tstart, tend);
		    	netInterface.SendMessageW(ConstructAnswer(tstart, tend), ipClient);
			} else if (command == "clear") 
			{
				ClearLog();
			}
			else if (command == "request_warn")
			{	
				netInterface.SendMessageW(SendWarnings(), ipClient);
			}
			else if (command == "request_info")
			{
				netInterface.SendMessageW(SendInfos(), ipClient);
			}
			else if (command == "request_error")
			{
				netInterface.SendMessageW(SendErrors(), ipClient);
			}
			else if (command == "warn")
			{
				AddMessage(fWarn, mutexWarn, strmsg, time(0), netInterface.InfoString(ipClient));
			}
			else if (command == "info")
			{
				AddMessage(fInfo, mutexInfo, strmsg, time(0), netInterface.InfoString(ipClient));
			}
			else if (command == "error")
			{
				AddMessage(fError, mutexError, strmsg, time(0), netInterface.InfoString(ipClient));
			}
		}
		netInterface.CheckConnection();
	}
}


int main()
{
	NetOpenClose Net;
	Net.InitSockets();
	fWarn = fopen("warnings", "a");
	fInfo = fopen("infos", "a");
	fError = fopen("errors", "a");
//	ClearLog();
	thread first(Work, PORT+1), second(Work, PORT + 2), last(Work, PORT);
	CloseControl();
	last.join();
	first.join();
	second.join();
	Net.ShutdownSockets();
}