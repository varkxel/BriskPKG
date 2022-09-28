#include "CommandHandler.hpp"

#include <iostream>
#include "../BPKG.hpp"

static constexpr const char* helpString = "\
Usage: BPKGTool <command> <args>\n\
Commands:\n\
- help - Shows this screen.\n\
- generate <Source Directory> <Output Directory> - Generates a BPK from a source directory.\n\
- read <BPKG> <Resource ID> - Reads the file with a given ID from the given BPKG.\
";

namespace Brisk::SDK
{
	using index_t = BriskPKG::index_t;
	
	CommandHandler::CommandHandler()
	{
		commandMap["help"] = Command::help;
		commandMap["generate"] = Command::generate;
		commandMap["read"] = Command::read;
	}
	
	void CommandHandler::parse(const char* command[], size_t length)
	{
		const char* args[length - 1];
		for(size_t i = 1; i < length; i++) args[i - 1] = command[i];
		
		switch(commandMap[command[0]])
		{
			default:
			case Command::help:
				help();
				return;
			case Command::generate:
				generate(args, length - 1);
				return;
			case Command::read:
				read(args, length - 1);
				return;
		}
	}
	
	void CommandHandler::help()
	{
		using namespace std;
		cout << helpString << endl;
	}
	
	void CommandHandler::generate(const char* args[], size_t length)
	{
		using namespace std;
		if(length < 2)
		{
			cout << "Usage: generate <Source Directory> <Output Directory>" << endl;
			return;
		}
		
		BriskPKG::generate(args[0], args[1]);
	}
	
	void CommandHandler::read(const char* args[], size_t length)
	{
		using namespace std;
		if(length < 2)
		{
			cout << "Usage: read <BPKG> <Resource ID>" << endl;
			return;
		}
		
		std::string id;
		for(size_t i = 1; i < length; i++) id += args[i];
		
		BriskPKG bpkg(args[0]);
		const BriskPKG::ObjectEntry* file = bpkg.load(id);
		if(file == nullptr) throw runtime_error("Failed to read BPKG file.\nMake sure you have got your BPKG path right.");
		
		for(index_t i = 0; i < file->size; i++) cout << file->data[i];
		
		bpkg.clear();
	}
}