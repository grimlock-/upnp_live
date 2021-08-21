#ifndef UTIL_H
#define UTIL_H

#include <vector>
#include <string>

namespace upnp_live {
namespace util {
	std::vector<std::string> SplitString(std::string, char);
	std::string TrimString(std::string);
	std::string TrimString(std::string, char);
	std::string EscapeXML(std::string);
	std::string EscapeURI(std::string);
	std::string GetMimeType(const char*);
	bool FileExists(const char*);
	bool IsDirectory(const char*);
	bool IsRegularFile(const char*);
	bool IsChildDirectory(std::string, std::string);
	bool IsSymbolicLink(const char* filename);
	std::pair<std::vector<std::string>, std::vector<std::string>> GetDirectoryContents(std::string);
}
}

#endif /* UTIL_H */
