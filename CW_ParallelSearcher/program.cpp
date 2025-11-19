#include "Listener.h"
#include <iostream>

int main()
{
	try
	{
		Listener listener;
		listener.startListening();
	}
	catch (const std::exception& ex)
	{
		std::cerr << "Exception: " << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
