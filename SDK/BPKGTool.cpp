#include "CommandHandler.hpp"

#include <string>

int main(int argc, char* argv[])
{
	using namespace Brisk::SDK;
	
	if(argc <= 1)
	{
		// Show help
		CommandHandler::help();
		return EXIT_SUCCESS;
	}
	
	constexpr size_t commandIndex = 1;
	
	const char* command[argc - commandIndex];
	for(size_t i = commandIndex; i < argc; i++) command[i - commandIndex] = argv[i];
	
	CommandHandler handler;
	handler.parse(command, argc - commandIndex);
}
