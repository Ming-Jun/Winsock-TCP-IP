/**
 ** Date: 2020/05/05
 ** Author: Ming-Jun Jiang.
 ** IDE: Win10(1909) & Visual Studio 2015 (MSVC14).
 ** Description: create a client to sned/receive data to/from a server.
 */

#include <WinSock2.h> // Windows Sockets 2 API.
#include <ws2tcpip.h> // used by Windows Sockets 2.
#include <iostream>
#include <string>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")

class client
{
public:
	client() :bufSize(0)
	{
		// initialization of member variables.
		iResult = 0;
		clntSock = INVALID_SOCKET;
		sendBuf = nullptr;
		recvBuf = nullptr;
	}
	explicit client(const std::wstring &ip, const std::uint16_t &port, const int &_bufSize) :bufSize(_bufSize)
	{ 
		// initialization of member variables.
		iResult = 0;
		clntSock = INVALID_SOCKET;
		sendBuf = new char[bufSize] {0};
		recvBuf = new char[bufSize] {0};

		WSADATA wsaData;//Windows Sockets API (WSA) initialization.
		iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);// Winsock Version 2.2
		if (iResult == NO_ERROR)
		{
			std::cout << "Windows Sockets API initialization succeed.\n";
			clntSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);// create a socket with IPv4 and TCP setting.
			if (clntSock != INVALID_SOCKET)
			{
				std::cout << "client socket: " << clntSock << "\n";
				sockaddr_in servAddr;
				memset(&servAddr, 0, sizeof(servAddr));
				servAddr.sin_family = AF_INET;// IPv4
				InetPton(AF_INET, ip.c_str(), &servAddr.sin_addr.s_addr);// IP
				servAddr.sin_port = htons(port);// Port
				iResult = connect(clntSock, reinterpret_cast<SOCKADDR*>(&servAddr), sizeof(SOCKADDR));// connect a socket to the IP and port.
				if (iResult == SOCKET_ERROR)
				{
					std::cerr << "Connection failed with error: " << WSAGetLastError() << "\n";
					clntSock = INVALID_SOCKET;
				}
			}
			else
			{
				std::cerr << "Invalid Socket with error:" << WSAGetLastError() << "\n";
			}
		}
		else
		{
			std::cerr << "Windows Sockets API initialization failed with error:" << WSAGetLastError() << "\n";
		}
	}
	client(const client&) = delete; // non construction-copyable.
	client& operator=(const client&) = delete; // non copyable.
	~client()
	{
		closesocket(clntSock);// close client socket.
		WSACleanup();//Clean and close Windows Sockets API (WSA).

		// release memory of buffer.
		if (sendBuf != nullptr)
		{
			delete[] sendBuf;
			sendBuf = nullptr;
		}
		if (recvBuf != nullptr)
		{
			delete[] recvBuf;
			recvBuf = nullptr;
		}
	}

	void startSend()
	{
		if (clntSock != INVALID_SOCKET)
		{
			std::string strBuffer = "";
			std::cout << "enter data (enter EXIT for leaving).\n";
			while (true)
			{
				std::cin >> strBuffer;
				if ((strBuffer == "exit") || (strBuffer == "EXIT"))
				{
					break;
				}
				else
				{
					strncpy_s(sendBuf, bufSize, strBuffer.c_str(), _TRUNCATE);// copy input data to buffer.
					iResult = send(clntSock, sendBuf, bufSize, 0);// send buffer to the server.
					if (iResult == SOCKET_ERROR)
					{
						std::cerr << "Sending failed with error: " << WSAGetLastError() << "\n";
						break;
					}
				}
			}
			shutdown(clntSock, 2);//shutdown the socket.
		}
	}
	void startRecv()
	{
		if (clntSock != INVALID_SOCKET)
		{
			while (true)
			{
				iResult = recv(clntSock, recvBuf, bufSize, 0);// receive data from the server.
				if (iResult > 0)
				{
					std::cout <<"Receive: "<< recvBuf << "\n";
				}
				else if (iResult == 0)
				{
					std::cout << "connection is closed.\n";
					break;
				}
				else
				{
					std::cerr << "receiving failed with error: " << WSAGetLastError() << "\n";
					break;
				}
			}
			shutdown(clntSock, 2);//shutdown the socket.
		}
	}

private:
	SOCKET clntSock;
	char *sendBuf;
	char *recvBuf;
	const int bufSize;
	int iResult;
};

int main()
{
	// Basic setting.
	std::wstring IP = L"127.0.0.1";// IP.
	uint16_t port = 6666;// Port.
	const int data_length = 512;// maximum data length.

	// Create a client.
	client client1(IP, port, data_length);
	std::thread recvThread([&client1]() {client1.startRecv(); });// creat a thread to receive data.
	std::thread sendThread([&client1]() {client1.startSend(); });// creat a thread to send data.
	recvThread.join();// wait for the receiving thread to finish.
	sendThread.join();// wait for the sending thread to finish.

	std::cout << "Press enter to exit.\n";
	getchar();
	return 0;
}