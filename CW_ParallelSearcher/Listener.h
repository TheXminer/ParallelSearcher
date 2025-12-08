#pragma once
#include "Controller.h"
#include "ThreadPool.h"

class Listener
{
private:
	Controller* controller;
	std::shared_ptr<ThreadPool> threadPool;
	static Listener* instance;
	std::atomic<bool> listening{ true };
public:
	Listener();
	~Listener();
	void startListening();
	void handleClient(int serverSocket, std::atomic<uint32_t>& client_counter);
	int processListening();
	int startSocket();

	void stopListening();
};