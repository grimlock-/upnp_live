#include <iostream>
#include <fstream>
#include <string>
#include <exception>
#include <cctype> //isspace
#include "Config.h"
#include "Version.h"
#include "util.h"
#include "Logging.h"

void upnp_live::PrintHelp()
{
	std::cout << "upnp_live v" << upnp_live_version << "\n\n" \
\
        "upnp_live is a DLNA media server that mirrors internet live streams for other\n" \
        "devices to access via UPnP. It also has basic file serving and can use other\n" \
	"programs for transcoding.\n\n" \
\
        "Some useful command line parameters:\n\n" \
\
        "  -c, --config <filepath>   configuration file to load\n" \
        "  -d, --daemon              run as a background process\n" \
	"  -i, --interface <name>    set network interface (has priority over -ip)\n" \
	"  -ip, --address <ip>       set IP address\n" \
	"  -p, --port <port>         set port\n" \
	"  --loglevel <level>        accepts 0 (disable) to 7 (verbose debug)\n" \
	"                            defaults to 4 (info)\n" \
	"  -w, --web-root <path>     set root directory for web server\n" \
	"                            defaults to 'www' alongside the executable\n";
}

void upnp_live::PrintVersion()
{
	std::cout << "upnp_live v" << upnp_live_version << " pupnp v" << UPNP_VERSION_STRING << "\n";
}

void upnp_live::ParseArgument(std::string name, std::string value, InitOptions& options)
{
#ifdef UPNP_LIVE_DEBUG
	std::cout << "ParseArgument(): " << name << " | " << value << "\n";
#endif
	if(name.empty())
		return;

	if(name == "daemon" || name == "background")
	{
#ifdef UPNP_LIVE_DEBUG
		std::cout << "background\n";
#endif
		options.background = true;
		return;
	}

	if(value.empty())
	{
		std::cout << "Ignoring argument \"" << name << "\" with no value\n";
		return;
	}

	if(name == "address")
	{
#ifdef UPNP_LIVE_DEBUG
		std::cout << "address\n";
#endif
		options.address = value;
	}
	else if(name == "interface")
	{
#ifdef UPNP_LIVE_DEBUG
		std::cout << "interface\n";
#endif
		options.interface = value;
	}
	else if(name == "logfile")
	{
#ifdef UPNP_LIVE_DEBUG
		std::cout << "log file\n";
#endif
		options.log_file = value;
	}
	else if(name == "loglevel")
	{
#ifdef UPNP_LIVE_DEBUG
		std::cout << "log level\n";
#endif
		options.log_level = stoi(value);
	}
	else if(name == "port")
	{
#ifdef UPNP_LIVE_DEBUG
		std::cout << "port\n";
#endif
		options.port = stoi(value);
	}
	else if(name == "webroot")
	{
#ifdef UPNP_LIVE_DEBUG
		std::cout << "web root\n";
#endif
		options.web_root = value;
		while(options.web_root.back() == '/' || options.web_root.back() == '\\')
			options.web_root.pop_back();
	}
	else if(name == "stream" || name == "name")
	{
#ifdef UPNP_LIVE_DEBUG
		std::cout << "stream block start\n";
#endif
		options.streams.emplace(options.streams.end());
		options.streams.back().name = value;
	}
	else if(name == "status")
	{
#ifdef UPNP_LIVE_DEBUG
		std::cout << "status handler\n";
#endif
		if(options.streams.size() == 0)
			return;
		
		auto i = value.find_first_of(" ");
		options.streams.back().status_handler = value.substr(0, i);
		options.streams.back().status_args = value.substr(i+1);
	}
	else if(name == "media")
	{
#ifdef UPNP_LIVE_DEBUG
		std::cout << "media handler\n";
#endif
		if(options.streams.size() == 0)
			return;
		
		auto i = value.find_first_of(" ");
		options.streams.back().av_handler = value.substr(0, i);
		options.streams.back().av_args = value.substr(i+1);
	}
	//FIXME - make a way for this to refer to a file block
	else if(name == "mimetype")
	{
#ifdef UPNP_LIVE_DEBUG
		std::cout << "mimetype\n";
#endif
		if(options.streams.size() == 0)
			return;
		
		options.streams.back().mime_type = value;
	}
	else if(name == "transcode")
	{
#ifdef UPNP_LIVE_DEBUG
		std::cout << "transcoder\n";
#endif
		if(options.streams.size() == 0)
			return;
		
		options.streams.back().transcoder = value;
	}
	else if(name == "buffer")
	{
#ifdef UPNP_LIVE_DEBUG
		std::cout << "buffer\n";
#endif
		if(options.streams.size() == 0)
			return;
		
		options.streams.back().buffer_size = std::stoll(value);
		if(options.streams.back().buffer_size < 4096)
		{
			std::cout << "Buffer size must be at least 4096\n";
			options.streams.back().buffer_size = 0;
		}
	}
	else if(name == "file")
	{
#ifdef UPNP_LIVE_DEBUG
		std::cout << "file block start\n";
#endif
		options.files.emplace(options.files.end());
		options.files.back().name = value;
	}
	else if(name == "path")
	{
#ifdef UPNP_LIVE_DEBUG
		std::cout << "file path\n";
#endif
		if(options.files.size() == 0)
			return;

		options.files.back().path = value;
	}
	else if(name == "directory" || name == "dir")
	{
#ifdef UPNP_LIVE_DEBUG
		std::cout << "directory\n";
#endif
		if(value.front() == '/' || value.front() == '\\')
		{
			std::cout << "Directory config option cannot be an absolute path: " << value << "\n";
			return;
		}
		while(value.back() == '/' || value.back() == '\\')
			value.pop_back();
		options.directories.emplace(options.directories.end());
		options.directories.back().path = value;
	}
}

void upnp_live::ParseArguments(int argc, char* argv[], InitOptions& options)
{
#ifdef UPNP_LIVE_DEBUG
	std::cout << "ParseArguments()\n";
#endif
	for(int i = 1; i < argc; i++)
	{
		std::string arg = argv[i];
#ifdef UPNP_LIVE_DEBUG
		std::cout << "Command line arg: " << arg << "\n";
#endif
		
		if(arg == "-c" || arg == "--config")
		{
			options.config_file = argv[++i];
		}
		else if(arg == "-d" || arg == "--daemon")
		{
			arg = "daemon";
			ParseArgument(arg, "", options);
		}
		else if(arg == "-f" || arg == "--logfile")
		{
			arg = "logfile";
			ParseArgument(arg, argv[++i], options);
		}
		else if(arg == "--loglevel")
		{
			arg = "loglevel";
			ParseArgument(arg, argv[++i], options);
		}
		else if(arg == "-i" || arg == "--interface")
		{
			arg = "interface";
			ParseArgument(arg, argv[++i], options);
		}
		else if(arg == "-ip" || arg == "--address")
		{
			arg = "address";
			ParseArgument(arg, argv[++i], options);
		}
		else if(arg == "-p" || arg == "--port")
		{
			arg = "port";
			ParseArgument(arg, argv[++i], options);
		}
		else if(arg == "-v" || arg == "--verbose")
		{
			arg = "loglevel";
			std::string ll = "4";
			ParseArgument(arg, ll, options);
		}
		else if(arg == "--help")
		{
			PrintHelp();
			exit(0);
		}
		else if(arg == "--version")
		{
			PrintVersion();
			exit(0);
		}
		else
		{
			std::cout << "Unknown argument: " << arg << "\n";
		}
	}
}
void upnp_live::ParseConfigFile(InitOptions& options)
{
	if(options.config_file.empty())
		return;

#ifdef UPNP_LIVE_DEBUG
	std::cout << "ParseConfigFile(): " << options.config_file << "\n";
#endif
	std::fstream configFile;
	try
	{
		StreamInitOptions tStream;
		configFile.open(options.config_file.c_str(), std::ios_base::in);
		int count = 0;
		while(configFile.good())
		{
			++count;
			std::string line;
			std::string::size_type c;
			std::getline(configFile, line);
			line = util::TrimString(line);
			if(line.empty() || line.front() == '#')
				continue;
			c = line.find_first_of(" ");
			//TODO - to lowercase
			std::string name = util::TrimString(line.substr(0, c));
			std::string value = util::TrimString(line.substr(c+1));
			ParseArgument(name, value, options);
		}
		if(!tStream.name.empty())
			options.streams.push_back(tStream);
	}
	catch(std::exception& e)
	{
		std::cerr << "Caught exception parsing config file: " << e.what() << "\n";
		return;
	}
}
