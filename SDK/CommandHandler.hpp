#ifndef BRISKPKGTOOL_COMMANDHANDLER_HPP
#define BRISKPKGTOOL_COMMANDHANDLER_HPP

#include <map>
#include <string>

namespace Brisk::SDK
{
	class CommandHandler
	{
	protected:
		enum class Command
		{
			help,
			generate,
			read,
		};
		std::map<std::string, Command> commandMap;
	public:
		CommandHandler();
		void parse(const char* command[], size_t length);
		
		// Help is used by other classes, therefore it's public
		static void help();
	private:
		static void generate(const char* args[], size_t length);
		static void read(const char* args[], size_t length);
	};
}

#endif