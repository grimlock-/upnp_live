#ifndef UTIL_H
#define UTIL_H

#include <vector>
#include <string>

namespace upnp_live {
namespace util {
	std::vector<std::string> SplitString(std::string, char);
	std::string EscapeXML(std::string);
	std::string EscapeURI(std::string);
	bool FileExists(const char*);
	std::string GetMimeType(const char* extension);
}
}

#endif /* UTIL_H */
