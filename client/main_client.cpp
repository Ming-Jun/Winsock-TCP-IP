/**
 ** Date: 2020/05/06
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
	client() :kBufSize(0) {
		// initialization of member variables.
		result_ = 0;
		clnt_sock_ = INVALID_SOCKET;
		send_buf_ = nullptr;
		recv_buf_ = nullptr;
	}
	explicit client(const std::wstring &ip, const std::uint16_t &port, const int &bufSize) :kBufSize(bufSize) { 
		// initialization of member variables.
		result_ = 0;
		clnt_sock_ = INVALID_SOCKET;
		send_buf_ = new char[kBufSize]();
		recv_buf_ = new char[kBufSize]();

		WSADATA wsa_data;//Windows Sockets API (WSA) initialization.
		result_ = WSAStartup(MAKEWORD(2, 2), &wsa_data);// Winsock Version 2.2
		if (result_ == NO_ERROR) {
			std::cout << "Windows Sockets API initialization succeed.\n";
			clnt_sock_ = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);// create a socket with IPv4 and TCP setting.
			if (clnt_sock_ != INVALID_SOCKET) {
				std::cout << "client socket: " << clnt_sock_ << "\n";
				sockaddr_in serv_addr;
				memset(&serv_addr, 0, sizeof(serv_addr));
				serv_addr.sin_family = AF_INET;// IPv4
				InetPton(AF_INET, ip.c_str(), &serv_addr.sin_addr.s_addr);// IP
				serv_addr.sin_port = htons(port);// Port
				result_ = connect(clnt_sock_, reinterpret_cast<SOCKADDR*>(&serv_addr), sizeof(SOCKADDR));// connect a socket to the IP and port.
				if (result_ == SOCKET_ERROR) {
					std::cerr << "Connection failed with error: " << WSAGetLastError() << "\n";
					clnt_sock_ = INVALID_SOCKET;
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
	client(const client&) = delete; // non construction-copyable.
	client& operator=(const client&) = delete; // non copyable.
	~client() {
		closesocket(clnt_sock_);// close client socket.
		WSACleanup();//Clean and close Windows Sockets API (WSA).

		// release memory of buffer.
		if (send_buf_ != nullptr) {
			delete[] send_buf_;
			send_buf_ = nullptr;
		}
		if (recv_buf_ != nullptr) {
			delete[] recv_buf_;
			recv_buf_ = nullptr;
		}
	}

	void StartSend() {
		if (clnt_sock_ != INVALID_SOCKET) {
			std::string str_buf = "";
			std::cout << "enter data (enter EXIT for leaving).\n";
			while (true) {
				std::cin >> str_buf;
				if ((str_buf == "exit") || (str_buf == "EXIT")) {
					break;
				}
				else {
					strncpy_s(send_buf_, kBufSize, str_buf.c_str(), _TRUNCATE);// copy input data to buffer.
					result_ = send(clnt_sock_, send_buf_, kBufSize, 0);// send buffer to the server.
					if (result_ == SOCKET_ERROR) {
						std::cerr << "Sending failed with error: " << WSAGetLastError() << "\n";
						break;
					}
				}
			}
			shutdown(clnt_sock_, 2);//shutdown the socket.
		}
	}
	void StartRecv() {
		if (clnt_sock_ != INVALID_SOCKET) {
			while (true) {
				result_ = recv(clnt_sock_, recv_buf_, kBufSize, 0);// receive data from the server.
				if (result_ > 0) {
					std::cout <<"Receive: "<< recv_buf_ << "\n";
				}
				else if (result_ == 0) {
					std::cout << "connection is closed.\n";
					break;
				}
				else {
					std::cerr << "receiving failed with error: " << WSAGetLastError() << "\n";
					break;
				}
			}
			shutdown(clnt_sock_, 2);//shutdown the socket.
		}
	}

private:
	SOCKET clnt_sock_;
	char *send_buf_;
	char *recv_buf_;
	const int kBufSize;
	int result_;
};

int main()
{
	// Basic setting.
	std::wstring ip = L"127.0.0.1";// IP.
	uint16_t port = 6666;// Port.
	const int kDataLength = 512;// maximum data length.

	// Create a client.
	client client1(ip, port, kDataLength);
	std::thread recv_thread([&client1]() {client1.StartRecv(); });// creat a thread to receive data.
	std::thread send_thread([&client1]() {client1.StartSend(); });// creat a thread to send data.
	recv_thread.join();// wait for the receiving thread to finish.
	send_thread.join();// wait for the sending thread to finish.

	std::cout << "Press enter to exit.\n";
	getchar();
	return 0;
}