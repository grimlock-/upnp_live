#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include "InitOptions.h"

namespace upnp_live {
	void PrintHelp();
	void PrintVersion();
	void ParseArgument(std::string name, std::string value, InitOptions& options);
	void ParseArguments(int argc, char** argv, InitOptions& options);
	void ParseConfigFile(InitOptions& options);
}

#endif /* CONFIG_H */
