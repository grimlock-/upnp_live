#pragma once
#include <string>
#include "InitOptions.h"

namespace upnp_live {
	void ParseArguments(int, char**, InitOptions&);
	void ParseConfigFile(InitOptions&);
}
