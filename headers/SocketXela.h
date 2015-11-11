#include <stdlib.h>
#include <stdio.h>

#define MAX_BUF_SIZE 60000
#define ON_WINDOWS 1   // I love windows for this so much, fucking bustards
#define ON_NORMALSYSTEM 2

#if defined (_WIN32) 
#define PLATFORM PLATFORM_WINDOWS  
#else
#define PLATFORM ON_NORMALSYSTEM
#endif

#if PLATFORM == PLATFORM_WINDOWS
	#include <winsock2.h>
	#pragma comment(lib, "Ws2_32.lib")
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif
namespace newsockets {

	const int MAXLENGTHOFNUM = 7;
	//Класс для инициалищации сокетов 
	class NetOpenClose
	{
		public:

			NetOpenClose() {};
			~NetOpenClose() {};
			
			void ShutdownSockets()  //more shit for microsoft!
			{
				#if PLATFORM == PLATFORM_WINDOWS
					WSACleanup();
				#endif
			}


			bool InitSockets()// well done microsoft!
			{	
				#if PLATFORM == PLATFORM_WINDOWS
					WSADATA WsaData;
					return WSAStartup(MAKEWORD(2, 2), &WsaData) == NO_ERROR;
				#else
					return true;
				#endif
			}
	};
	//Класс для удобного хранения адреса
	class IPAdress
	{
		private:
			unsigned int adress;
			unsigned short port;
			sockaddr_in sockAdr;
		
			bool ToIP(unsigned short a, unsigned short b, unsigned short c, unsigned short d) 
			{
				unsigned int ans = (a | b | c | d);
				if (ans < 256)
					adress = ((unsigned int)a << 24) | ((unsigned int)b << 16) | ((unsigned int)c << 8) | d;
				return ans < 256;
			}

			int TakeInt(const char* str, unsigned short& shift)
			{
				char temp[MAXLENGTHOFNUM + 1];
				for (int i = 0; str[shift] >= '0' && str[shift] <= '9' && i < MAXLENGTHOFNUM; ++i, ++shift) {
					temp[i] = str[shift];
				}
				++shift;
				temp[MAXLENGTHOFNUM] = 0;
				return atoi(temp);
			}

			void RelaxAdr()
			{
				#if PLATFORM == PLATFORM_WINDOWS
					sockAdr.sin_addr.S_un.S_addr = this->GetNetIP();
				#else
					sockAdr.sin_addr.s_addr = this->GetNetIP();
				#endif
				sockAdr.sin_family = AF_INET;
				sockAdr.sin_port = this->GetNetPort();
			}

			

		public:
			//Конструктор создает адрес 0.0.0.0:0
			IPAdress() 
			{
				adress = 0;
				port = 0;
				RelaxAdr();
			};
			//Коструктор для любого адреса на порт newPort
			IPAdress(unsigned short newPort)
			{
				adress = ntohl(INADDR_ANY);
				port = newPort;
				RelaxAdr();
			};
			//Очевидно
			IPAdress(const char* ipAdr, unsigned short newPort) 
			{
				unsigned short a, b, c, d, i = 0;
				a = TakeInt(ipAdr, i);
				b = TakeInt(ipAdr, i);
				c = TakeInt(ipAdr, i);
				d = TakeInt(ipAdr, i);
				ToIP(a, b, c, d);
				port = newPort;
				RelaxAdr();
			}
			//Очевидно
			IPAdress(unsigned short a, unsigned short b, unsigned short c, unsigned short d, unsigned short nport) 
			{
				ToIP(a, b, c, d);
				port = nport;
				RelaxAdr();
			}
			//Обновляет адрес и порт по sockAddr
			void RelaxAfterRead() 
			{
				#if PLATFORM == PLATFORM_WINDOWS
					adress = ntohl(sockAdr.sin_addr.S_un.S_addr);
				#else
					adress = ntohl(sockAdr.sin_addr.s_addr);
				#endif
				port = ntohs(sockAdr.sin_port);
			}

			unsigned int GetIP() const 
			{
				return adress;
			}

			void SetNetIP(int ipNet) 
			{
				adress = ntohl(ipNet);
				RelaxAdr();
			}

			unsigned short GetPort() const 
			{
				return port;
			}

			unsigned int GetNetIP() const 
			{
				return htonl(adress);
			}

			unsigned short GetNetPort() const
			{
				return htons(port);
			}

			sockaddr * SockAdress() 
			{
				return (sockaddr *) &sockAdr;
			}

			const sockaddr * SockAdress() const
			{
				return (sockaddr *) &sockAdr;
			}

			int SizeOfSockaddr() const
			{
				return sizeof(sockAdr);
			}

			bool operator<(const IPAdress& another)  const
			{
				if (adress < another.adress)
					return true;
				else
					return port < another.port;
			}
			
			bool operator==(const IPAdress& another)  const
			{
				return (adress == another.adress) && (port == another.port);
			}
	};
	//Класс для удобной работы с UDP сокетами
	class Socket_UDP
	{
		private:
			int id;
			bool nonblock;
		public:	
			Socket_UDP( bool block = true) 
			{
				InitEmpty();
				id = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
				if (!Exist())
				{
					printf("Socket creation error");
					return;
				}
				if (!block) MakeNonBlock();
			};

			~Socket_UDP() 
			{
				Close();
			}

			void Reset(bool block = true)
			{
				Close();
				InitEmpty();
				id = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
				if (!Exist())
				{
					printf("Socket creation error");
					return;
				}
				if (!block) MakeNonBlock();
			}

			bool Exist() 
			{
				#if PLATFORM == PLATFORM_WINDOWS
					return !(id == INVALID_SOCKET);
				#else
					return !(id <= 0);
				#endif	
			}

			bool Bind(unsigned short port = 0) 
			{
				if (port < 0 || port > 65535) 
				{
					printf("Port is not exists, will be used random port\n");
					port = 0;
				}
				IPAdress allConections(port);
				if (!(bind(id, allConections.SockAdress(), allConections.SizeOfSockaddr()) == 0))
				{
					printf("binding error on %li socket\n", id);
					Close();
					return false;
				}
				return true;
			}

			void InitEmpty() 
			{
				#if PLATFORM == PLATFORM_WINDOWS
					id = INVALID_SOCKET;
				#else
					id = -1;
				#endif
				nonblock = false;
			}
			
			void Close() 
			{
				if (Exist()) 
				{
					#if PLATFORM == PLATFORM_WINDOWS
						closesocket(id);
					#else
						close(id);
					#endif
					InitEmpty();
				}
			}
			
			bool Sendto(const char * data, int size, const IPAdress & dest, int flag = 0) 
			{
				if (size <= 0 || !Exist()) { return false; }
				return size == sendto(id, data, size, flag, dest.SockAdress(), dest.SizeOfSockaddr());
			}

			bool Recive(std::string & destdata, IPAdress & source, int flag = 0) 
			{
				char buff[MAX_BUF_SIZE];
				destdata.clear();
				if (!Exist()) { return false; }
				int sizeofSourceaddr = source.SizeOfSockaddr();
				//memset(buff, 0, MAX_BUF_SIZE);
				#if PLATFORM == PLATFORM_WINDOWS
					if  (recvfrom(id, buff, MAX_BUF_SIZE, flag, source.SockAdress(), &sizeofSourceaddr) == SOCKET_ERROR && WSAGetLastError() != WSAEMSGSIZE) 
				#else
				    if (recvfrom(id, data, size, flag, source.SockAdress(), &sizeofSourceaddr) == -1)
				#endif
					{
					//	printf("%d", WSAGetLastError());
						return false;
					}
					source.RelaxAfterRead();
					destdata.append(buff, MAX_BUF_SIZE);
					return true;
				}

			bool MakeNonBlock() 
			{
				#if PLATFORM == PLATFORM_WINDOWS
					DWORD nonBlocking = 1;
					if (ioctlsocket(id, FIONBIO, &nonBlocking) == SOCKET_ERROR)
				#else
					if (fcntl(id, F_SETFL, O_NONBLOCK, 1) == -1)
				#endif
				{
					printf("seting non-block on %li socket is down\n", id);
					Close();
					return false;
				}
				return nonblock = true;
			}

			bool IsNonBlock() const 
			{
				return nonblock;
			}

	};
}
