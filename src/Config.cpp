#include <iostream>
#include <fstream>
#include <string>
#include <exception>
#include "Config.h"
#include "Version.h"

void upnp_live::ParseArguments(int argc, char* argv[], InitOptions& options)
{
	std::cout << "Parsing command line arguments" << std::endl;
	for(int i = 1; i < argc; i++)
	{
		std::string arg = argv[i];
		std::cout << "Argument: " << arg << std::endl;
		if(arg == "-v" || arg == "--version")
		{
			std::cout << "Matched: version" << std::endl;
			std::cout << "upnp_live v" << UPNP_LIVE_VERSION_STRING << std::endl;
			exit(0);
		}
		else if(arg == "-c" || arg == "--config")
		{
			std::cout << "Matched: config" << std::endl;
			options.configFile = argv[++i];
		}
		else if(arg == "-d" || arg == "--daemon")
		{
			std::cout << "Matched: daemon" << std::endl;
			options.daemon = true;
		}
		else if(arg == "-f" || arg == "--logfile")
		{
			std::cout << "Matched: log file" << std::endl;
			options.logFile = argv[++i];
		}
		else if(arg == "-F" || arg == "--logfile-only")
		{
			std::cout << "Matched: log file only" << std::endl;
			options.logFile = argv[++i];
			//TODO - set disabled standard output
		}
		else if(arg == "--loglevel")
		{
			std::cout << "Matched: log level" << std::endl;
			arg = argv[++i];
			options.logLevel = stoi(arg);
		}
		else if(arg == "-ip" || arg == "--address")
		{
			std::cout << "Matched: IP address" << std::endl;
			options.address = argv[++i];
		}
		else if(arg == "-p" || arg == "--port")
		{
			std::cout << "Matched: port" << std::endl;
			arg = argv[++i];
			options.port = stoi(arg);
		}
		else if(arg == "--url")
		{
			std::cout << "Matched: url" << std::endl;
			options.streams.push_back(argv[++i]);
		}
		else if(arg == "--verbose")
		{
			std::cout << "Matched: verbose" << std::endl;
			options.logLevel = 4;
		}
		else if(arg == "--help")
		{
			std::cout << "Matched: help" << std::endl;
			std::cout << \
			"upnp_live is a DLNA media server that mirrors internet live streams so\n" << \
			"other devices can access them via UPnP.\n\n" << \
			\
			"Some useful command line parameters:\n\n" << \
			\
			"  --config <filepath>    configuration file to load\n" << \
			"  --url <url>            stream to mirror\n" << \
			"  --daemon               run as a background process\n" << \
			"  --logfile              print program output to the specified file\n" << \
			"  --logfile-only         same as above while also disabling standard output\n" << \
			"                         (identical to above for daemonized version)\n" << \
			\
			"See the man page (not yet made) for all parameters plus their short versions" << std::endl;
			exit(0);
		}
	}
}
void upnp_live::ParseConfigFile(InitOptions& options)
{
	if(options.configFile == "")
		return;

	std::fstream configFile;
	try
	{
		std::cout << "Opening configuration file: " << options.configFile << std::endl;
		configFile.open(options.configFile.c_str(), std::ios_base::in);
		std::cout << "Successfully opened file" << std::endl;
		int count = 0;
		while(configFile.good())
		{
			++count;
			std::string line;
			std::string::size_type c;
			std::getline(configFile, line);
			while( (c = line.find_first_of("\t")) != std::string::npos)
			{
				line.erase(c, 1);
			}
			while(line[0] == ' ')
				line.erase(0, 1);
			if(line[0] == '#')
				continue;
			c = line.find_first_of(" ");
			std::string name = line.substr(0, c);
			std::string value = line.substr(c+1);
			std::cout << "Line " << count << " first word: " << name << std::endl;
			if(name == "address")
			{
				std::cout << "Matched: address" << std::endl;
				options.address = value;
			}
			else if(name == "daemon")
			{
				std::cout << "Matched: daemon" << std::endl;
				options.daemon = true;
			}
			else if(name == "logfile")
			{
				std::cout << "Matched: log file" << std::endl;
				options.logFile = value;
			}
			else if(name == "logfile-only")
			{
				std::cout << "Matched: log file only" << std::endl;
				options.logFile = value;
				//TODO - set disabled standard output
			}
			else if(name == "loglevel")
			{
				std::cout << "Matched: log level" << std::endl;
				options.logLevel = stoi(value);
			}
			else if(name == "port")
			{
				std::cout << "Matched: port" << std::endl;
				options.port = stoi(value);
			}
			else if(name == "url")
			{
				std::cout << "Matched: url" << std::endl;
				options.streams.push_back(value);
			}
		}
	}
	catch(std::ios_base::failure& e)
	{
		std::cerr << "Caught failure exception parsing config file: " << e.what() << std::endl;
		return;
	}
	catch(std::exception& e)
	{
		std::cerr << "Caught generic exception parsing config file: " << e.what() << std::endl;
		return;
	}
}
