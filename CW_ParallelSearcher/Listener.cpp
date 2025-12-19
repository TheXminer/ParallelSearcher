#include "Listener.h"
#include <iostream>
#include <WinSock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define PORT 8000
#define BUFFER_SIZE 4096
#define MAX_CLIENTS 1000

Listener::Listener()
{
	threadPool.reset(new ThreadPool(12));
	controller = new Controller(threadPool);
}

Listener::~Listener()
{
	delete controller;
	threadPool->stopPool();
}

void Listener::startListening()
{
	std::cout << "Server is running. Press Ctrl+C to stop." << std::endl;
	threadPool->enqueue(&Listener::processListening, this);
}

void Listener::handleClient(int serverSocket, std::atomic<uint32_t>& client_counter)
{
	struct sockaddr_in clientAddr;
	int clientAddrLen = sizeof(clientAddr);
	SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr,
		&clientAddrLen);
	if (clientSocket == INVALID_SOCKET) {
		return;
	}
	char client_addr[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(clientAddr.sin_addr), client_addr, INET_ADDRSTRLEN);
	std::cout << "Connection accepted from " << client_addr << ":" <<
		ntohs(clientAddr.sin_port) << std::endl;
	if (client_counter.load() <= MAX_CLIENTS)
	{
		client_counter++;
		threadPool->enqueue([this, clientSocket, &client_counter]() {
			this->controller->handleClient(clientSocket);
			client_counter--;
			});
	}
}

int Listener::processListening()
{
	int serverSocket;
	std::atomic<uint32_t> client_counter{ 0 };
	try {
		serverSocket = startSocket();
	}
	catch (const std::exception& ex) {
		std::cerr << "Exception in starting socket: " << ex.what() << std::endl;
		return -1;
	}

	while (listening.load()) {
		try{
			handleClient(serverSocket, client_counter);
		}
		catch (const std::exception& ex) {
			std::cerr << "Exception in handling client: " << ex.what() << std::endl;
		}
	}
	std::cout << "No longer accepting new connections. Waiting for existing clients to finish..." << std::endl;
	closesocket(serverSocket);
	WSACleanup();
	return 0;
}

int Listener::startSocket()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	SOCKET serverSocket;
	struct sockaddr_in serverAddr;
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == INVALID_SOCKET) {
		std::cerr << "Socket creation failed\n";
		WSACleanup();
		throw std::runtime_error("Socket creation failed");
	}
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;

	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(PORT);
	if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) ==
		SOCKET_ERROR) {
		std::cerr << "Bind failed\n";
		closesocket(serverSocket);
		WSACleanup();
		throw std::runtime_error("Bind failed");
	}
	if (listen(serverSocket, 5) == SOCKET_ERROR) {
		std::cerr << "Listen failed\n";
		closesocket(serverSocket);
		WSACleanup();
		throw std::runtime_error("Listen failed");
	}
	std::cout << "Server listening on port " << PORT << "...\n";
	return serverSocket;
}

void Listener::stopListening()
{
	listening.store(false);
	std::cout << "Server stopped listening." << std::endl;
	controller->stopSearcher();
	std::cout << "Searcher stopped." << std::endl;
	threadPool->stopPool();
	std::cout << "Thread pool stopped." << std::endl;
}
