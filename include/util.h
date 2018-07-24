#pragma once
#include <vector>
#include <string>

namespace upnp_live {
namespace util {
	std::vector<std::string> SplitString(std::string, char);
	std::string EscapeXML(std::string);
	bool FileExists(const char*);
}
}
