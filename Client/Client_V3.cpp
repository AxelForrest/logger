#include <thread>
#include <iostream>
#include <set>
#include "Logger.h"
//#include "SocketXela.h"
#define BUF_LEN 10
#define PORT 52341

using namespace newsockets;
using namespace logger;


set<string> listOfCommands;
void AddCommands() 
{
	listOfCommands.insert("warn");
	listOfCommands.insert("info");
	listOfCommands.insert("error");
	listOfCommands.insert("clear");
	listOfCommands.insert("request_warn");
	listOfCommands.insert("request_info");
	listOfCommands.insert("request_error");
	listOfCommands.insert("request_since");
}

void LowwerCase(string & str)
{
	for (int i = 0; i < str.length(); ++i)
	{
		if (str[i] >= 'A' && str[i] <= 'Z')
		{
			str[i] = str[i] - 'A' + 'a';
		}
	}
}

bool IsExist(const string& str)
{
	return !(listOfCommands.find(str) == listOfCommands.end());
}

void ReadLN(string& str)
{
	char ch;
	str = "";
	while (scanf_s("%c", &ch) && ch != '\n')
	{
		str += ch;
	}
}

bool IsGood(int h, int m, int s) 
{
	return !(h < 0 || h>23 || m < 0 || m > 59 || s < 0 || s > 59);
}

bool GetRequestTimeBorders(string &Time)
{
	int h1,h2, m1,m2, s1, s2, count = 0;
	string temp;
	ReadLN(temp);
	int pos=0;
	while (temp[pos] == ' ') { ++pos; }
	while (count++ < 3) 
	{
		if (sscanf_s(&temp[pos], "!%d:%d:%d", &h1, &m1, &s1) == 3 && IsGood(h1, m1, s1) ) 
		{
			Time.clear();
			char BuFF[30];
			sprintf_s(BuFF, "%d:%d:%d-NOW", h1, m1, s1);
			Time += BuFF;
			return true;
		}
		else if (sscanf_s(&temp[0], "%d:%d:%d-%d:%d:%d", &h1, &m1, &s1, &h2, &m2, &s2) == 6 && IsGood(h1, m1, s1) && IsGood(h2, m2, s2)) 
		{
			Time.clear();
			char BuFF[30];
			sprintf_s(BuFF, "%d:%d:%d-%d:%d:%d", h1, m1, s1, h2, m2, s2);
			Time += BuFF;
			return true;

		}
		cout << "Bad format of time. Please try again use only !HH:MM:SS or HH:MM:SS-HH:MM:SS format" << endl;
		ReadLN(temp);
		pos = 0;
	}
	return false;
}

void SendCommand(string command, EasyInterface& NetPlace, int dest)
{
	NetPlace.UntilSendMessage(command, dest);
}

string SendRequest(string command, EasyInterface& NetPlace, int  dest)
{
	NetPlace.UntilSendMessage(command, dest);
	string result = "";
	NetPlace.NextReciveFrom(result, dest);
	if (result.length() == 0)
	{
		result = "SORRY - NO DATA FOR YOUR REQUEST";
	}
	return result;
}

void Work()
{
	NetOpenClose Net;
	Net.InitSockets();
	EasyInterface netInterface;	
	string strmsg;
	srand(time(0));
	IPAdress serverAdr(127, 0, 0, 1, PORT + (rand() % 3));
	netInterface.Bind(0);
	int serverIP = netInterface.MakeNewConnection(serverAdr);
	if (serverIP < 0)
	{
		cout << "Just shit happend. No Server." << endl;
		return;
	}
	string s(""), msg("");
	while (s != "exit")
	{
		cout << "Type command: ";
		cin >> s;
		LowwerCase(s);
		if (IsExist(s)) 
		{
			if (s == "request_since" && GetRequestTimeBorders(msg))
			{
				s = s + ' ' + msg;
				cout << "Server answer is:\n" << SendRequest(s, netInterface, serverIP) << endl;
			}
			else 
			{
				if (s == "warn" || s == "info" || s == "error")
				{
					ReadLN(msg);
					s = s + msg;
				}
				//cout << serverIP.GetIP();
				if (s == "request_warn" || s == "request_error" || s == "request_info") 
				{
					ReadLN(msg);
					cout << "Server answer is:\n" << SendRequest(s, netInterface, serverIP) << endl;
				}
				else
				{
					SendCommand(s, netInterface, serverIP);
				}
			}
			
		}
		else if (s != "exit")
		{
			ReadLN(msg);
			cout << "Bad command, please try again.\n";
		}
	}
	Net.ShutdownSockets();
}


int main()
{
	AddCommands();
	Work();
}
