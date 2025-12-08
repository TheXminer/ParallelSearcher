#include "Listener.h"
#include <iostream>
#include <csignal>
bool stopFlag = false;

class Server {
private:
	std::unique_ptr<Listener> listener;

public:
	Server() : listener(std::make_unique<Listener>()) {}

	void run() {
		std::signal(SIGINT, Server::handleSignal);

		listener->startListening();

		while (!stopFlag) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		std::cout << "Shutting down server..." << std::endl;
		listener->stopListening();
		std::cout << "Server stopped." << std::endl;	
	}

	static void handleSignal(int signum) {
		std::cout << "Interrupt (" << signum << ") received." << std::endl;
		stopFlag = true;
	}
};

int main()
{
	try
	{
		Server server;
		server.run();
	}
	catch (const std::exception& ex)
	{
		std::cerr << "Exception: " << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
