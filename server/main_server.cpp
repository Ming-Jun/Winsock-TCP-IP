/**
 ** Date: 2020/05/05
 ** Author: Ming-Jun Jiang.
 ** IDE: Win10(1909) & Visual Studio 2015 (MSVC14).
 ** Description: create a server to receive data from multiple clients and send data to all clients.
 */

#include <WinSock2.h> // Windows Sockets 2 API.
#include <ws2tcpip.h> // used by Windows Sockets 2.
#include <iostream>
#include <string>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")

class server
{
public:
	server() :bufSize(0)
	{
		// initialization of member variables.
		servSock = INVALID_SOCKET;
		maxSock = INVALID_SOCKET;
		FD_ZERO(&master_fd);//initialize the master set.
		sendBuf = nullptr;
		recvBuf = nullptr;
		sTimeout = 0;
		usTimeout = 0;
		iResult = 0;
		isWorking = false;
	}
	explicit server(const std::wstring &ip, const std::uint16_t &port, const int &_bufSize, const int& _sTimeout, const int& _usTimeout) :bufSize(_bufSize)
	{
		// initialization of member variables
		servSock = INVALID_SOCKET;
		maxSock = INVALID_SOCKET;
		FD_ZERO(&master_fd);//initialize the master set.
		sendBuf = new char[bufSize] {0};
		recvBuf = new char[bufSize] {0};
		sTimeout = _sTimeout;
		usTimeout = _usTimeout;
		iResult = 0;
		isWorking = false;

		WSADATA wsaData;//Windows Sockets API (WSA) initialization.
		iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);// Winsock Version 2.2
		if (iResult == NO_ERROR)
		{
			std::cout << "Windows Sockets API initialization succeed.\n";
			servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);// create a socket with IPv4 and TCP setting.
			if (servSock != INVALID_SOCKET)
			{
				std::cout << "server socket: " << servSock << "\n";
				sockaddr_in servAddr;
				memset(&servAddr, 0, sizeof(servAddr));
				servAddr.sin_family = AF_INET;// IPv4
				InetPton(AF_INET, ip.c_str(), &servAddr.sin_addr.s_addr);// IP
				servAddr.sin_port = htons(port);// Port
				iResult = bind(servSock, reinterpret_cast<SOCKADDR*>(&servAddr), sizeof(SOCKADDR));// bind a socket to the IP and port.
				if (iResult != SOCKET_ERROR)
				{
					iResult = listen(servSock, 30);//set to listening mode and specify maximum of 30 pending connections for the server socket.
					if (iResult == SOCKET_ERROR)
					{
						std::cerr << "Listening failed with error: " << WSAGetLastError() << "\n";
						servSock = INVALID_SOCKET;
					}
				}
				else
				{
				    std::cerr << "Connection failed with error: " << WSAGetLastError() << "\n";
					servSock = INVALID_SOCKET;
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
	server(const server&) = delete; // non construction-copyable.
	server& operator=(const server&) = delete; // non copyable.
	~server()
	{
		closesocket(servSock);// close server socket.
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

	void startRecv()
	{
		if (servSock != INVALID_SOCKET)
		{
			// initialization of FD
			timeval timeout = { sTimeout,usTimeout };// set timeout to 10 seconds.
			fd_set read_fd;//sets of file descriptors.
			FD_ZERO(&read_fd);//initialize the reading set.
			FD_SET(servSock, &master_fd);//add server socket to master set

			maxSock = servSock;// record the maximum socket.
			SOCKET clntSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			sockaddr_in clntAddr;
			int nSize = sizeof(SOCKADDR);

			isWorking = true;
			while (isWorking)
			{
				read_fd = master_fd;//refresh reading set with master set;
				iResult = select(static_cast<int>(maxSock + 1), &read_fd, NULL, NULL, &timeout);//polling all sockets in the reading set.
				if (iResult > 0)
				{
					for (unsigned int i = 0; i <= maxSock; i++)
					{
						// check sockets in the reading set.
						if (FD_ISSET(i, &read_fd))
						{
							if (i == servSock)
							{
								std::cout << "Connecting a new client";
								clntSock = accept(servSock, reinterpret_cast<SOCKADDR*>(&clntAddr), &nSize);// accept the connection from a client.
								if (clntSock != INVALID_SOCKET)
								{
									std::cout << " with the socket: " << clntSock << "\n";
									FD_SET(clntSock, &master_fd);// add a client socket to the master set.
									if (clntSock > maxSock)
									{
										maxSock = clntSock;// update the maximum socket.
									}

									strncpy_s(sendBuf, bufSize, "Welcome to your server!", _TRUNCATE);//
									iResult = send(clntSock, sendBuf, bufSize, 0);// send buffer to a client.
									if (iResult == SOCKET_ERROR)
									{
										std::cerr << "Send failed with error: " << WSAGetLastError() << "\n";
										FD_CLR(clntSock, &master_fd);// remove the client socket from the master set.
										closesocket(clntSock); // close the client socket.
									}
								}
								else
								{
									std::cerr << ".\nAccepting failed with error: " << WSAGetLastError() << "\n";
								}
							}
							else
							{
								iResult = recv(i, recvBuf, bufSize, 0);//receive data from a client.
								if (iResult > 0)
								{
									std::cout << "Client " << i << ":" << recvBuf << "\n";
								}
								else if (iResult == 0)
								{
									std::cout << "Client " << i << ": connection is closed.\n";
									FD_CLR(i, &master_fd);// remove the client socket from the master set.
									closesocket(i); // close the client socket.
								}
								else
								{
									std::cerr << "Client " << i << ":receiving failed with error: " << WSAGetLastError() << "\n";
									FD_CLR(i, &master_fd);// remove the client socket from the master set.
									closesocket(i); // close the client socket.
								}
							}
						}
					}
				}
				else if (iResult == 0)
				{
					//std::cerr << "Waiting Time Out.\n";
				}
				else 
				{
					std::cerr << "Waiting failed with error: " << WSAGetLastError() << "\n";
					break;
				}
			}

			// clean FD.
			for (unsigned int i = 0; i <= maxSock; i++)
			{
				if (FD_ISSET(i, &master_fd))
				{
					FD_CLR(i, &master_fd);// remove the client socket from the master set.
					closesocket(i); // close the client socket.
				}
			}
		}
	}
	void startSend()
	{
		if (servSock != INVALID_SOCKET)
		{
			std::string strBuffer = "";
			std::cout << "enter data (enter EXIT for leaving).\n";
			while (true)
			{
				std::cin >> strBuffer;
				if ((strBuffer == "exit") || (strBuffer == "EXIT"))
				{
					for (unsigned int i = 0; i <= maxSock; i++)
					{
						if (FD_ISSET(i, &master_fd))
						{
							shutdown(i, 2);// shutdown all sockets in the master fd set.
						}
					}
					isWorking = false;// to stop the receiving loop.
					break;
				}
				else
				{
					for (unsigned int i = 0; i <= maxSock; i++)
					{
						if ((i != servSock) && FD_ISSET(i, &master_fd))
						{
							strncpy_s(sendBuf, bufSize, strBuffer.c_str(), _TRUNCATE);// copy input data to buffer.
							iResult = send(i, sendBuf, bufSize, 0);// send buffer to a client.
							if (iResult == SOCKET_ERROR)
							{
								std::cerr << "Send failed with error: " << WSAGetLastError() << "\n";
								shutdown(i, 2);//shutdown a socket.
							}
						}
					}
				}
			}
		}
	}

private:
	SOCKET servSock;
	SOCKET maxSock;
	fd_set master_fd;
	char *sendBuf;
	char *recvBuf;
	const int bufSize;
	int iResult;
	int sTimeout;
	int usTimeout;
	volatile bool isWorking;
};

int main()
{
	// Basic setting.
	std::wstring IP = L"127.0.0.1";// IP.
	uint16_t port = 6666;// Port.
	const int data_length = 512;// maximum data length.
	int timeout_second = 1;// timeout setting in seconds.
	int timeout_usecond = 0;// timeout setting in microseconds.

	// Create a server.
	server server1(IP, port, data_length, timeout_second, timeout_usecond);
	std::thread recvThread([&server1]() {server1.startRecv(); });// creat a thread to receive data.
	std::thread sendThread([&server1]() {server1.startSend(); });// creat a thread to send data.
	recvThread.join();// wait for the receiving thread to finish.
	sendThread.join();// wait for the sending thread to finish.

	std::cout << "Press enter to exit.\n";
	getchar();
	return 0;
}