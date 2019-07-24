#include "System.h"

#include <iostream>

int main()
{
	System s;

	try
	{
		s.initialize();
		s.mainLoop();
		s.cleanup();
	}
	catch (const std::runtime_error& e)
	{
		std::cerr << e.what() << std::endl;
		system("PAUSE");
		return EXIT_FAILURE;
	}

	system("PAUSE");
	return EXIT_SUCCESS;
}