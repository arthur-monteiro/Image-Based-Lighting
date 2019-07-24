#include <iostream>

int main()
{
	try
	{
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