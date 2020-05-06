/**
 ** Date: 2020/05/06
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
	server() :kBufSize(0) {
		// initialization of member variables.
		serv_sock_ = INVALID_SOCKET;
		max_sock_ = INVALID_SOCKET;
		FD_ZERO(&master_fd_);//initialize the master set.
		send_buf_ = nullptr;
		recv_buf_ = nullptr;
		timeout_s_ = 0;
		timeout_us_ = 0;
		result_ = 0;
		is_working_ = false;
	}
	explicit server(const std::wstring &ip, const std::uint16_t &port, const int &buf_size, const int& timeout_s, const int& timeout_us) :kBufSize(buf_size) {
		// initialization of member variables
		serv_sock_ = INVALID_SOCKET;
		max_sock_ = INVALID_SOCKET;
		FD_ZERO(&master_fd_);//initialize the master set.
		send_buf_ = new char[kBufSize]();
		recv_buf_ = new char[kBufSize]();
		timeout_s_ = timeout_s;
		timeout_us_ = timeout_us;
		result_ = 0;
		is_working_ = false;

		WSADATA wsa_data;//Windows Sockets API (WSA) initialization.
		result_ = WSAStartup(MAKEWORD(2, 2), &wsa_data);// Winsock Version 2.2
		if (result_ == NO_ERROR) {
			std::cout << "Windows Sockets API initialization succeed.\n";
			serv_sock_ = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);// create a socket with IPv4 and TCP setting.
			if (serv_sock_ != INVALID_SOCKET) {
				std::cout << "server socket: " << serv_sock_ << "\n";
				sockaddr_in serv_addr;
				memset(&serv_addr, 0, sizeof(serv_addr));
				serv_addr.sin_family = AF_INET;// IPv4
				InetPton(AF_INET, ip.c_str(), &serv_addr.sin_addr.s_addr);// IP
				serv_addr.sin_port = htons(port);// Port
				result_ = bind(serv_sock_, reinterpret_cast<SOCKADDR*>(&serv_addr), sizeof(SOCKADDR));// bind a socket to the IP and port.
				if (result_ != SOCKET_ERROR) {
					result_ = listen(serv_sock_, 30);//set to listening mode and specify maximum of 30 pending connections for the server socket.
					if (result_ == SOCKET_ERROR) {
						std::cerr << "Listening failed with error: " << WSAGetLastError() << "\n";
						serv_sock_ = INVALID_SOCKET;
					}
				}
				else {
					std::cerr << "Connection failed with error: " << WSAGetLastError() << "\n";
					serv_sock_ = INVALID_SOCKET;
				}
			}
			else {
				std::cerr << "Invalid Socket with error:" << WSAGetLastError() << "\n";
			}
		}
		else {
			std::cerr << "Windows Sockets API initialization failed with error:" << WSAGetLastError() << "\n";
		}
	}
	server(const server&) = delete; // non construction-copyable.
	server& operator=(const server&) = delete; // non copyable.
	~server() {
		closesocket(serv_sock_);// close server socket.
		WSACleanup();//Clean and close Windows Sockets API (WSA).

		// release memory of buffer.
		if (send_buf_ != nullptr)
		{
			delete[] send_buf_;
			send_buf_ = nullptr;
		}
		if (recv_buf_ != nullptr)
		{
			delete[] recv_buf_;
			recv_buf_ = nullptr;
		}
	}

	void StartRecv() {
		if (serv_sock_ != INVALID_SOCKET) {
			// initialization of FD
			timeval timeout = { timeout_s_, timeout_us_ };// set timeout to 10 seconds.
			fd_set read_fd;//sets of file descriptors.
			FD_ZERO(&read_fd);//initialize the reading set.
			FD_SET(serv_sock_, &master_fd_);//add server socket to master set

			max_sock_ = serv_sock_;// record the maximum socket.
			SOCKET clnt_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			sockaddr_in clnt_addr;
			int addr_size = sizeof(SOCKADDR);

			is_working_ = true;
			while (IsWorking()) {
				read_fd = master_fd_;//refresh reading set with master set;
				result_ = select(static_cast<int>(max_sock_ + 1), &read_fd, NULL, NULL, &timeout);//polling all sockets in the reading set.
				if (result_ > 0) {
					for (unsigned int i = 0; i <= max_sock_; ++i) {
						// check sockets in the reading set.
						if (FD_ISSET(i, &read_fd)) {
							if (i == serv_sock_) {
								std::cout << "Connecting a new client";
								clnt_sock = accept(serv_sock_, reinterpret_cast<SOCKADDR*>(&clnt_addr), &addr_size);// accept the connection from a client.
								if (clnt_sock != INVALID_SOCKET) {
									std::cout << " with the socket: " << clnt_sock << "\n";
									FD_SET(clnt_sock, &master_fd_);// add a client socket to the master set.
									if (clnt_sock > max_sock_) {
										max_sock_ = clnt_sock;// update the maximum socket.
									}

									strncpy_s(send_buf_, kBufSize, "Welcome to your server!", _TRUNCATE);//
									result_ = send(clnt_sock, send_buf_, kBufSize, 0);// send buffer to a client.
									if (result_ == SOCKET_ERROR) {
										std::cerr << "Send failed with error: " << WSAGetLastError() << "\n";
										FD_CLR(clnt_sock, &master_fd_);// remove the client socket from the master set.
										closesocket(clnt_sock); // close the client socket.
									}
								}
								else {
									std::cerr << ".\nAccepting failed with error: " << WSAGetLastError() << "\n";
								}
							}
							else {
								result_ = recv(i, recv_buf_, kBufSize, 0);//receive data from a client.
								if (result_ > 0) {
									std::cout << "Client " << i << ":" << recv_buf_ << "\n";
								}
								else if (result_ == 0) {
									std::cout << "Client " << i << ": connection is closed.\n";
									FD_CLR(i, &master_fd_);// remove the client socket from the master set.
									closesocket(i); // close the client socket.
								}
								else {
									std::cerr << "Client " << i << ":receiving failed with error: " << WSAGetLastError() << "\n";
									FD_CLR(i, &master_fd_);// remove the client socket from the master set.
									closesocket(i); // close the client socket.
								}
							}
						}
					}
				}
				else if (result_ == 0) {
					//std::cerr << "Waiting Time Out.\n";
				}
				else {
					std::cerr << "Waiting failed with error: " << WSAGetLastError() << "\n";
					break;
				}
			}

			// clean FD.
			for (unsigned int i = 0; i <= max_sock_; ++i) {
				if (FD_ISSET(i, &master_fd_)) {
					FD_CLR(i, &master_fd_);// remove the client socket from the master set.
					closesocket(i); // close the client socket.
				}
			}
		}
	}
	void StartSend() {
		if (serv_sock_ != INVALID_SOCKET) {
			std::string str_buf = "";
			std::cout << "enter data (enter EXIT for leaving).\n";
			while (true) {
				std::cin >> str_buf;
				if ((str_buf == "exit") || (str_buf == "EXIT")) {
					for (unsigned int i = 0; i <= max_sock_; ++i) {
						if (FD_ISSET(i, &master_fd_)) {
							shutdown(i, 2);// shutdown all sockets in the master fd set.
						}
					}
					is_working_ = false;// to stop the receiving loop.
					break;
				}
				else {
					for (unsigned int i = 0; i <= max_sock_; ++i) {
						if ((i != serv_sock_) && FD_ISSET(i, &master_fd_)) {
							strncpy_s(send_buf_, kBufSize, str_buf.c_str(), _TRUNCATE);// copy input data to buffer.
							result_ = send(i, send_buf_, kBufSize, 0);// send buffer to a client.
							if (result_ == SOCKET_ERROR) {
								std::cerr << "Send failed with error: " << WSAGetLastError() << "\n";
								shutdown(i, 2);//shutdown a socket.
							}
						}
					}
				}
			}
		}
	}
	bool IsWorking() const { return is_working_; }

private:
	SOCKET serv_sock_;
	SOCKET max_sock_;
	fd_set master_fd_;
	char *send_buf_;
	char *recv_buf_;
	const int kBufSize;
	int result_;
	int timeout_s_;
	int timeout_us_;
	volatile bool is_working_;
};

int main() {
	// Basic settings.
	std::wstring ip = L"127.0.0.1";// IP.
	uint16_t port = 6666;// Port.
	const int kDataLength = 512;// maximum data length.
	int timeout_s = 1;// timeout setting in seconds.
	int timeout_us = 0;// timeout setting in microseconds.


	server server1(ip, port, kDataLength, timeout_s, timeout_us);// Create a server.
	std::thread recv_thread([&server1]() {server1.StartRecv(); });// creat a thread to receive data.
	std::thread send_thread([&server1]() {server1.StartSend(); });// creat a thread to send data.
	recv_thread.join();// wait for the receiving thread to finish.
	send_thread.join();// wait for the sending thread to finish.

	std::cout << "Press enter to exit.\n";
	getchar();
	return 0;
}