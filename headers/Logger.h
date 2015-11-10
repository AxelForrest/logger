#include "SocketXela.h"
#include <string>
#include <time.h>
#include <map>
#define MAX_BUF_SIZE 60000
#define MAX_LENGTH_OF_MSG 60000
#define WAIT_FOR_APROOV 1
#define WAIT_FOR_DATA 2
#define WAIT_FOR_COMMAND 3
#define WAIT_FOR_HELLO 4
#define LENGTH_OF_COMMAND 9
#define LIVE_TIME 1
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
			strdest[0] = "D";
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
				pSock->Sendto(temp, tsize, dest);
			}
			else
			{
				MakeHeader(temp, 'S', 0, id);
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
			printf("Connection with %d:%d set. New id = %d\n", dest.GetIP(), dest.GetPort(), newID);
			status = WAIT_FOR_COMMAND;
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

		void GetFromHeader(const char* strsource, char& command, int& size, int& id) 
		{
			command = strsource[0];
			size = IntFromStr(strsource[1]);
			id = IntFromStr(strsource[5]);
		}

		void SendHello(int forConnectID = id) 
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

		void WorkOn(char* data)
		{
			char action;
			int size, id;
			GetFromHeader(data, action, size, id);
			if (action == 'H')
			{
				pSock->Recive(data, LENGTH_OF_COMMAND, dest);
				if (status == WAIT_FOR_HELLO)
				{
					ReciveHello(size);
				}
			}
			else
			{
				pSock->Recive(data, size, dest);
				if (status == WAIT_FOR_APROOV && action == 'A')
				{
					printf("%c%c%c%c%c\n", action, data[1], data[2], data[3], data[4]);
					printf("%d\n", IntFromStr(&data[1]));
					if ((numberof) == IntFromStr(&data[1]))
					{
						ReciveAproove();
						time(&lastSucces);
						tryies = 0;
					}
				}
				else if (status == WAIT_FOR_DATA)
				{
					printf("Work with data %c\n", action);
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
					else if (data[0] == 'S')
					{
						time(&lastSucces);
						SendAproove();
						reciveSuc = 1;
						tryies = 0;
					}
				}
				else if (status == WAIT_FOR_COMMAND)
				{
					if (data[0] == 'D')
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
					else if (data[0] == 'S')
					{
						numberof == 0;
						allData.clear();
						SendAproove();
						reciveSuc = 1;
						time(&lastSucces);
						tryies = 0;
					}
				}
			}
		}

		bool IsReciveSuccesfull()
		{
			return reciveSuc;
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
				numberof = 0;
				allData = data;
				SendPart(numberof);
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
			return false;
		}

	};

	class EasyInterface
	{
	private:
		Socket_UDP sock;
		IPAdress ClientAdress;
		map<int, Connection> connections;
		map<int, IPAdress> idtoIP;
		char cData[MAX_LENGTH_OF_MSG];
		map<IPAdress, bool> forDelete;
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

		int SendNewConnection(const IPAdress& dest)
		{
			int connectID = -(rand() + 1);
			iptoID[connectID] = dest;
			connections[connectID] = Connection(dest, &sock, connectID);
			connections[connectID].SendHello();
			return connectID;
		}

		int RecivedNewConnection(const IPAdress& dest, int connectID) 
		{
			int legalID;
			do {
				legalID = rand();
			} while (connections.find(legalID) != connections.end());
			connections[legalID] = Connection(dest, &sock, connectID);
			connections[legalID].SendHello(connectID);
			return legalID;
		}

		bool ISReciveMSG(string &message, int& source)
		{
			if (sock.Recive(&cData[0], LENGTH_OF_COMMAND, ClientAdress, MSG_PEEK)) 
			{
				char comm; int size, id;
				Connection().GetFromHeader(&cData[0], comm, size, id);
				if (comm == 'H')
				{
					if (connections.find(id) == connections.end()) 
					{
						id = RecivedNewConnection(ClientAdress, id);
						idtoIP[id] = ClientAdress;
					}
					else 
					{
						connections[size] = connnections[id];
						idtoIP[size] = ClientAdress;
						connections.erase(id);
						idtoIP.erase(id);
						id = size;
					}
				} 
				connections[id].WorkOn(&cData[0], LENGTH_OF_COMMAND);
				if (connections.begin() != connections.end() && connections[id].IsReciveSuccesfull())
				{
					message = connections[id].Message();
					source = id;
					connections[id].RelaxRecive();
					return true;
				}
			}
			CheckConnection();
			return false;
		}

		bool SendMessageW(const string &msg, const int & id)
		{
			if (connections.find(id) == connections.end())
			{
				printf("Bad id = %d\n", id);
				return false
			}
			else if (id < 0) 
			{
				printf("Sorry not yet connected id = %d\n", id);
				return false;
			}
			connections[id].SendMSG(&msg[0]);
		}

		bool NextReciveFrom(string &msg, IPAdress & source)
		{
			while (NextReciveMSG(msg, ClientAdress) && !(ClientAdress == source));
			return (ClientAdress == source);
		}


		bool UntilSendMessage(const string &msg, IPAdress & dest)
		{
			SendMessageW(msg, dest);
			time(&now);
			while ((connections[dest].Status() == WAIT_FOR_APROOV) && !(connections[dest].NoChances(now)))
			{
				if (sock.Recive(&cData[0], LENGTH_OF_COMMAND, ClientAdress, MSG_PEEK))
				{
					if (connections.find(ClientAdress) == connections.end())
						connections[ClientAdress] = Connection(ClientAdress, &sock);
					connections[ClientAdress].WorkOn(&cData[0], LENGTH_OF_COMMAND);
					if (connections[ClientAdress].IsReciveSuccesfull())
					{
						connections[ClientAdress].RelaxRecive();
					}
					time(&now);
				}
			}
			if (connections[dest].Status() != WAIT_FOR_COMMAND)
			{
				CheckConnection();
				return false;
			}
			return true;
		}

		void CloseConnection(const IPAdress & dest)
		{
			connections.erase(dest);
		}

		void CheckConnection()
		{
			time(&now);
			if (difftime(now, last) > LIVE_TIME)
			{
				for (map <IPAdress, Connection> ::iterator it = connections.begin(); it != connections.end(); ++it)
				{
					if (it->second.NoChances(now))
					{
						forDelete[it->first] = true;
					}
				}
				for (map <IPAdress, bool> ::iterator it = forDelete.begin(); it != forDelete.end(); ++it)
				{
					printf("Connection with %d lost\n", it->first.GetIP());
					connections.erase(it->first);
				}
				forDelete.clear();
				last = now;
			}
		}

	};

}