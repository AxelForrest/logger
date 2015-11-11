#include "SocketXela.h"
#include <string>
#include <time.h>
#include <map>
#define MAX_LENGTH_OF_MSG 60000
#define WAIT_FOR_APROOV 1
#define WAIT_FOR_DATA 2
#define WAIT_FOR_COMMAND 3
#define WAIT_FOR_HELLO 4
#define RECIVE_SUCCESFUL 5
#define LENGTH_OF_COMMAND 9
#define LIVE_TIME 0.5
#define COUNT_OF_TRY 10
#define SIZE_OF_MSG_MAX (MAX_BUF_SIZE - LENGTH_OF_COMMAND - 4) 
using namespace newsockets;
using namespace std;


namespace logger
{
	class Connection
	{
	private:
		IPAdress dest;
		int id;
		Socket_UDP* pSock;
		std::string allData;
		char cMsg[MAX_BUF_SIZE], temp[MAX_BUF_SIZE];
		int status, numberof, tsize, reciveSuc, tryies;
		time_t lastSucces;

		void IntToFourChar(char* p, const int i)
		{
			memcpy(p, &i, 4);
		}

		unsigned int IntFromStr(const char* first)
		{
			int ans = 0;
			memcpy(&ans, first, 4);
			return ans;
		}

		void MakeHeader(char* strdest, char ch, int size /* without length of command*/, int connection_id)
		{
			strdest[0] = ch;
			IntToFourChar(&strdest[1], size);
			IntToFourChar(&strdest[5], connection_id); // for nat;
		}

		void SendPart(int num)
		{
			printf("Send data number %d\n", num);
			if ((num)* (SIZE_OF_MSG_MAX) < allData.length())
			{
				MakeHeader(temp, 'D', 4 + min(SIZE_OF_MSG_MAX, allData.length() - (num)* (SIZE_OF_MSG_MAX)), id);
				IntToFourChar(&temp[LENGTH_OF_COMMAND], num);
				memcpy(&temp[LENGTH_OF_COMMAND + 4], &allData[num*(SIZE_OF_MSG_MAX)], min((SIZE_OF_MSG_MAX), allData.length() - (num * (SIZE_OF_MSG_MAX))));
				tsize = min(SIZE_OF_MSG_MAX, allData.length() - (num)* (SIZE_OF_MSG_MAX)) + 4 + LENGTH_OF_COMMAND;
				printf("Sending data...\n");
			//	for (int i = 0; i < tsize; ++i)
			//		printf("%c\n", temp[i]);
				pSock->Sendto(temp, tsize, dest);
			}
			else
			{
				MakeHeader(temp, 'S', numberof, id);
				tsize = LENGTH_OF_COMMAND;
				pSock->Sendto(temp, LENGTH_OF_COMMAND, dest);
			}
			status = WAIT_FOR_APROOV;
			printf("Status = %d\n", status);
		}

		void ReciveAproove()
		{
			printf("Recive Aproove\n");
			if (numberof * (SIZE_OF_MSG_MAX) >= allData.size())
			{
				SendSuccsesfull();
			}
			else
			{
				++numberof;
				SendPart(numberof);
			}
		}

		void SendSuccsesfull()
		{
			allData.clear();
			numberof = 0;
			status = WAIT_FOR_COMMAND;
		}

		void ReciveHello(int newID)
		{
			if (id < 0)
			{
				id = newID;
			}
			printf("Connection with %s set. New id = %d\n", data(IP()), id);
			status = WAIT_FOR_COMMAND;
			if (reciveSuc == WAIT_FOR_HELLO)
			{
				reciveSuc = 0;
				time(&lastSucces);
				tryies = 0;
				numberof = 0;
				SendPart(numberof);
			}
		}

		void SendAproove()
		{
			printf("Send Aproove\n");
			MakeHeader(temp, 'A', numberof, id);
			tsize = LENGTH_OF_COMMAND;
			pSock->Sendto(temp, LENGTH_OF_COMMAND, dest);
		}

		void SendAproove(int n)
		{
			printf("Send Aproove\n");
			MakeHeader(temp, 'A', n, id);
			tsize = LENGTH_OF_COMMAND;
			pSock->Sendto(temp, LENGTH_OF_COMMAND, dest);
		}

		void RecivePart(const char* data)
		{
			allData += data;
			SendAproove();
			numberof++;
		}

	public:

		string IP() 
		{
			char ip[4];
			char res[30];
			IntToFourChar(ip, dest.GetIP());
			sprintf_s(res, "%u.%u.%u.%u:%u", ip[3], ip[2], ip[1], ip[0], dest.GetPort());
			return res;
		}

		void GetFromHeader(const char* strsource, char& command, int& size, int& id)
		{
			command = strsource[0];
			size = IntFromStr(&strsource[1]);
			id = IntFromStr(&strsource[5]);
		}

		void SendHello() 
		{
			MakeHeader(temp, 'H', id, id);
			tsize = LENGTH_OF_COMMAND;
			pSock->Sendto(temp, tsize, dest);
		}

		void SendHello(int forConnectID)
		{
			MakeHeader(temp, 'H', id, forConnectID);
			tsize = LENGTH_OF_COMMAND;
			pSock->Sendto(temp, tsize, dest);
		}

		Connection() { time(&lastSucces); tryies = 0; }

		Connection(IPAdress newDest, Socket_UDP* newpSock, int newID)
		{
			dest = newDest;
			id = newID;
			pSock = newpSock;
			allData.clear();
			status = WAIT_FOR_HELLO;
			reciveSuc = 0;
			time(&lastSucces);
			tryies = 0;
		}

		int Status()
		{
			return status;
		}

		void WorkOn(string & data)
		{
			char action;
			int size, idinfo;
			GetFromHeader(&data[0], action, size, idinfo);
			pSock->Recive(data, dest);
			if (action == 'H')
			{
				if (status == WAIT_FOR_HELLO)
				{
					ReciveHello(size);
				}
			}
			else
			{
				size += LENGTH_OF_COMMAND;
				printf("Reciving data\n");
			//	for (int i = 0; i < size; ++i)
			//		printf("%c\n", data[i]);
				if (status == WAIT_FOR_APROOV && action == 'A')
				{
					//printf("%c%c%c%c%c\n", action, data[1], data[2], data[3], data[4]);
					//printf("%d\n", IntFromStr(&data[1]));
					if ((numberof) == IntFromStr(&data[1]))
					{
						ReciveAproove();
						time(&lastSucces);
						tryies = 0;
					}
				}
				else if (status == WAIT_FOR_DATA)
				{
					//printf("Work with data %c\n", action);
					if (action == 'D')
					{
						data[size] = 0;
						if (numberof == IntFromStr(&data[LENGTH_OF_COMMAND])) {
							RecivePart(&data[LENGTH_OF_COMMAND + 4]);
							time(&lastSucces);
							tryies = 0;
						}
						else if (numberof > IntFromStr(&data[LENGTH_OF_COMMAND]))
						{
							SendAproove(IntFromStr(&data[LENGTH_OF_COMMAND]));
							time(&lastSucces);
							tryies = 0;

						}
					}
					else if (action == 'S')
					{
						time(&lastSucces);
						SendAproove();
						reciveSuc = RECIVE_SUCCESFUL;
						tryies = 0;
					}
				}
				else if (status == WAIT_FOR_COMMAND)
				{
					if (action == 'D')
					{
						status = WAIT_FOR_DATA;
						data[size] = 0;
						numberof = 0;
						if (numberof == IntFromStr(&data[LENGTH_OF_COMMAND]))
						{
							RecivePart(&data[LENGTH_OF_COMMAND + 4]);
							time(&lastSucces);
							tryies = 0;
						}
					}
					else if (action == 'S')
					{
						numberof == 0;
						allData.clear();
						SendAproove();
						reciveSuc = RECIVE_SUCCESFUL;
						time(&lastSucces);
						tryies = 0;
					}
				}
			}
		}

		bool IsReciveSuccesfull()
		{
			return reciveSuc == RECIVE_SUCCESFUL;
		}

		void RelaxRecive()
		{
			status = WAIT_FOR_COMMAND;
			reciveSuc = 0;
			allData.clear();
		}

		string Message()
		{
			return allData;
		}

		bool SendMSG(const char * data)
		{
			if (status == WAIT_FOR_COMMAND)
			{
				time(&lastSucces);
				tryies = 0;
				numberof = 0;
				allData = data;
				SendPart(numberof);
				return true;
			}
			else if (status == WAIT_FOR_HELLO && reciveSuc != WAIT_FOR_HELLO)
			{
				time(&lastSucces);
				tryies = 0;
				numberof = 0;
				reciveSuc = WAIT_FOR_HELLO;
				allData = data;
				return true;
			}
			else
				return false;
		}

		void RepeatLastMSG()
		{
			tryies++;
			pSock->Sendto(temp, tsize, dest);
		}

		bool NoChances(time_t &now)
		{
			if (status != WAIT_FOR_COMMAND && difftime(now, lastSucces) > LIVE_TIME)
			{
				if (tryies < COUNT_OF_TRY)
					RepeatLastMSG();
				else
					return true;
			}
			else if (status == WAIT_FOR_COMMAND && difftime(now, lastSucces) > LIVE_TIME * 100) 
			{
				return true;
			}
			return false;
		}

	};

	class EasyInterface
	{
	private:
		Socket_UDP sock;
		IPAdress ClientAdress;
		map<int, Connection> connections;
		string cData;
		map<int, bool> forDelete;
		time_t now, last;
		//			Connection currentConnection;
	public:

		EasyInterface()
		{
			sock.MakeNonBlock();
			srand(time(&last));
		}

		void Bind(unsigned short port)
		{
			sock.Bind(port);
		}

		~EasyInterface()
		{
			connections.clear();
			sock.Close();
		}

		void Reset() 
		{
			sock.Reset(false);
		}

		bool NextReciveMSG(string &message, int& source)
		{
			while (ISReciveMSG(message, source))
			{
				if (connections.size() == 0)
				{
					return false;
				}
			}
			return true;
		}

		int AskNewConnection(const IPAdress& dest)
		{
			int connectID = -(rand() + 1);
			connections[connectID] = Connection(dest, &sock, connectID);
			connections[connectID].SendHello();
			return connectID;
		}

		int MakeNewConnection(const IPAdress& dest)
		{
			int result = AskNewConnection(dest);
			if (WaitForConnect(result))
			{
				return result;
			}
			return -1;
		}

		bool WaitForConnect(int & connectionID)
		{
			for (int i = 0; i < 100000; ++i)
			{
				if (sock.Recive(cData, ClientAdress, MSG_PEEK))
				{
					char comm; int size, id;
					Connection().GetFromHeader(&cData[0], comm, size, id);
					if (comm == 'H')
					{
						if (connections.find(id) == connections.end())
						{
							id = RecivedNewConnection(ClientAdress, id);
						}
						else
						{
							connections[size] = connections[id];
							connections.erase(id);
							if (connectionID == id)
							{
								connectionID = size;
							}
							id = size;
						}
					}
					if (connections.find(id) != connections.end())
					{
						connections[id].WorkOn(cData);
						if (id == connectionID)
						{
							return true;
						}
					}
					else { sock.Recive(cData, ClientAdress); }
				}
			}
			return false;
		}

		int RecivedNewConnection(const IPAdress& dest, int connectID)
		{
			int legalID;
			do {
				legalID = rand();
			} while (connections.find(legalID) != connections.end());
			connections[legalID] = Connection(dest, &sock, legalID);
			connections[legalID].SendHello(connectID);
			return legalID;
		}

		bool ISReciveMSG(string &message, int& source)
		{
			if (sock.Recive(cData, ClientAdress, MSG_PEEK))
			{
				char comm; int size, id;
				Connection().GetFromHeader(&cData[0], comm, size, id);
				if (comm == 'H')
				{
					if (connections.find(id) == connections.end())
					{
						id = RecivedNewConnection(ClientAdress, id);
					}
					else
					{
						connections[size] = connections[id];
						connections.erase(id);
						id = size;
						source = id;
					}
				}
				if (connections.find(id) != connections.end())
				{
					connections[id].WorkOn(cData);
					if (connections.begin() != connections.end() && connections[id].IsReciveSuccesfull())
					{
						message = connections[id].Message();
						source = id;
						connections[id].RelaxRecive();
						return true;
					}
				}
				else { sock.Recive(cData, ClientAdress); }
			}
			CheckConnection();
			return false;
		}

		bool SendMessageW(const string &msg, const int & id, bool NO_WARN = false)
		{
			if (connections.find(id) == connections.end())
			{
				printf("Error could start sending message:Bad id = %d\n", id);
				return false;
			} if (!NO_WARN && id < 0)
			{
				printf("Error not initiated yet:Bad id = %d, message = %s\n", id, data(msg));
				return false;
			}
			connections[id].SendMSG(&msg[0]);
			return true;
		}

		bool NextReciveFrom(string &msg, int & id, int timelimitsec = 100)
		{
			int temp;
			time(&last);
			for (; difftime(now, last) < timelimitsec && NextReciveMSG(msg, temp) && temp != id; time(&now));
			return (temp == id);
		}


		bool UntilSendMessage(const string &msg, const int & dest)
		{
			if (connections.find(dest) == connections.end())
			{
				printf("Error could start sending message:Bad id = %d\n", dest);
				return false;
			}
			time(&now);
			if (connections[dest].NoChances(now)) 
			{
				printf("Session is closed for time limit unactivity: in id = %d\n", dest);
				connections.erase(dest);
				return false;
			}
			SendMessageW(msg, dest, true);
			while ((connections[dest].Status() == WAIT_FOR_APROOV || connections[dest].Status() == WAIT_FOR_HELLO) && !(connections[dest].NoChances(now)))
			{
				if (sock.Recive(cData, ClientAdress, MSG_PEEK))
				{
					char comm; int size, id;
					Connection().GetFromHeader(&cData[0], comm, size, id);
					if (comm == 'H')
					{
						if (connections.find(id) == connections.end())
						{
							id = RecivedNewConnection(ClientAdress, id);
					    }
						else
						{
							connections[size] = connections[id];
							connections.erase(id);
							id = size;
						}
					}
					if (connections.find(id) != connections.end())
					{
						connections[id].WorkOn(cData);
						if (connections[id].IsReciveSuccesfull())
						{
							connections[id].RelaxRecive();
						}
					}
					else { sock.Recive(cData, ClientAdress); }
				}
				time(&now);
			}
			if (connections[dest].Status() != WAIT_FOR_COMMAND)
			{
				CheckConnection();
				return false;
			}
			return true;
		}

		void CloseConnection(const int & dest)
		{
			connections.erase(dest);
		}

		string InfoString(int id) 
		{
			if (connections.find(id) == connections.end()) 
			{
				return "error";
			}
			return connections[id].IP();
		}

		void CheckConnection()
		{
			time(&now);
			if (difftime(now, last) > LIVE_TIME/3)
			{
				for (map <int, Connection> ::iterator it = connections.begin(); it != connections.end(); ++it)
				{
					if (it->second.NoChances(now))
					{
						forDelete[it->first] = true;
					}
				}
				for (map <int, bool> ::iterator it = forDelete.begin(); it != forDelete.end(); ++it)
				{
					printf("Connection with %s lost\n", data(connections[it->first].IP()));
					connections.erase(it->first); 
				}
				forDelete.clear();
				last = now;
			}
		}

	};

}