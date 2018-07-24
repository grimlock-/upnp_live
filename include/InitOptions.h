#pragma once
#include <string>
#include <vector>

namespace upnp_live {

struct InitOptions 
{
	InitOptions() : webRoot("www"), configFile("upnp_live.conf"), logFile("upnp_live.log"), logLevel(3), daemon(false)
	{
		/*config = "";
		description = "";
		descriptionType = DESCTYPE_FILE;
		webRoot = "";*/
	}
	std::string webRoot, configFile, address, logFile;
	std::vector<std::string> streams;
	int port, logLevel;
	bool daemon;
};

}
