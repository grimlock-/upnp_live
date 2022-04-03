#ifndef UTIL_H
#define UTIL_H

#include <vector>
#include <string>
#include "Exceptions.h"

namespace upnp_live {
namespace util {
	struct url_t {
		url_t(const std::string& in)
		{
			auto i = in.find("://");
			if(i == std::string::npos)
				throw InvalidUrl();
			protocol = in.substr(0, i);
			i += 3;
			auto j = in.find(':', i);
			auto k = in.find('/', i);
			if(j != std::string::npos)
			{
				hostname = in.substr(i, (j-i));
				if(k != std::string::npos)
				{
					port = std::stoi( in.substr(j+1, ( k-(j+1) )) );
					path = in.substr(k);
				}
				else
				{
					port = std::stoi(in.substr(j+1));
					path = '/';
				}
			}
			else if(k != std::string::npos)
			{
				hostname = in.substr(i, (k-i));
				port = 0;
				path = in.substr(k);
			}
			else
			{
				hostname = in.substr(i);
				port = 0;
				path = '/';
			}
		}
		std::string protocol, hostname, path;
		unsigned int port;
	};
	std::vector<std::string> SplitString(std::string, char);
	std::vector<std::string> SplitArgString(std::string, char);
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
