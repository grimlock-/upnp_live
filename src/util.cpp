#include <queue>
#include <system_error>
#include <strings.h> //strncasecmp
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "util.h"
using namespace upnp_live;

/*
 * Splits the input string into a vector of smaller strings using the provided character delimiter
 * If the input is less than 3 characters or the delimiter isn't found, the input string is the vector's only element
**/
std::vector<std::string> util::SplitString(std::string str, char delimiter)
{
	TrimString(str, delimiter);

	std::vector<std::string> v;
	size_t pos1 = 0, pos2 = str.find(delimiter);

	if(str.length() < 3 || pos2 == std::string::npos)
	{
		v.push_back(str);
		return v;
	}
	//If str starts with the delimiter, skip past it
	while(pos1 == pos2)
	{
		pos1++;
		pos2 = str.find(delimiter, pos1);
	}

	while(pos2 != std::string::npos)
	{
		if(pos1 != pos2)
			v.push_back(str.substr(pos1, pos2-pos1));
		pos1 = pos2+1;
		pos2 = str.find(delimiter, pos1);
	}
	//If they are equal it means the last char is the delimiter
	if(pos1 != pos2)
		v.push_back(str.substr(pos1));
	return v;
}
/*
 * Returns a copy of the given string with whitespace removed from the beginning and end
 */
std::string util::TrimString(std::string input)
{
	if(input.empty())
		return input;
	while(std::isspace(input.front()))
		input.erase(0, 1);
	while(std::isspace(input.back()))
		input.pop_back();
	return input;
}
/*
 * Returns a copy of the given string with the given character removed from the beginning and end
 */
std::string util::TrimString(std::string input, char c)
{
	if(input.empty())
		return input;
	while(input.front() == c)
		input.erase(0, 1);
	while(input.back() == c)
		input.pop_back();
	return input;
}
/*
 * Escape ampersand, angle brackets and quotation marks for use in XML
 */
std::string util::EscapeXML(std::string input)
{
	while(input.find("&") != std::string::npos)
		input.replace(  input.find("&"), 1, "&amp;"  );
	while(input.find("<") != std::string::npos)
		input.replace(  input.find("<"), 1, "&lt;"  );
	while(input.find(">") != std::string::npos)
		input.replace(  input.find(">"), 1, "&gt;"  );
	while(input.find("\"") != std::string::npos)
		input.replace(  input.find("\""), 1, "&quot;"  );
	return input;
}
/*
 * Escape characters for use in URIs
 */
std::string util::EscapeURI(std::string input)
{
	while(input.find("%") != std::string::npos)
		input.replace(  input.find("%"), 1, "%25"  );
	while(input.find(":") != std::string::npos)
		input.replace(  input.find(":"), 1, "%3A"  );
	//while(input.find("/") != std::string::npos)
		//input.replace(  input.find("/"), 1, "%2F"  );
	while(input.find("?") != std::string::npos)
		input.replace(  input.find("?"), 1, "%3F"  );
	while(input.find("#") != std::string::npos)
		input.replace(  input.find("#"), 1, "%23"  );
	while(input.find("[") != std::string::npos)
		input.replace(  input.find("["), 1, "%5B"  );
	while(input.find("]") != std::string::npos)
		input.replace(  input.find("]"), 1, "%5D"  );
	while(input.find("@") != std::string::npos)
		input.replace(  input.find("@"), 1, "%40"  );
	while(input.find("!") != std::string::npos)
		input.replace(  input.find("!"), 1, "%21"  );
	while(input.find("$") != std::string::npos)
		input.replace(  input.find("$"), 1, "%24"  );
	while(input.find("&") != std::string::npos)
		input.replace(  input.find("&"), 1, "%26"  );
	while(input.find("'") != std::string::npos)
		input.replace(  input.find("'"), 1, "%27"  );
	while(input.find("(") != std::string::npos)
		input.replace(  input.find("("), 1, "%28"  );
	while(input.find(")") != std::string::npos)
		input.replace(  input.find(")"), 1, "%29"  );
	while(input.find("*") != std::string::npos)
		input.replace(  input.find("*"), 1, "%2A"  );
	while(input.find("+") != std::string::npos)
		input.replace(  input.find("+"), 1, "%2B"  );
	while(input.find(",") != std::string::npos)
		input.replace(  input.find(","), 1, "%2C"  );
	while(input.find(";") != std::string::npos)
		input.replace(  input.find(";"), 1, "%3B"  );
	while(input.find("=") != std::string::npos)
		input.replace(  input.find("="), 1, "%3D"  );
	while(input.find(" ") != std::string::npos)
		input.replace(  input.find(" "), 1, "%20"  );
	while(input.find("\"") != std::string::npos)
		input.replace(  input.find("\""), 1, "%22"  );

	return input;
}
std::string util::GetMimeType(const char* ext)
{
	//Audio
	if(strncasecmp(ext, "ogg", 3) == 0 || strncasecmp(ext, "oga", 3) == 0)
		return std::string("audio/ogg");
	else if(strncasecmp(ext, "mp3", 3) == 0)
		return std::string("audio/mpeg");
	else if(strncasecmp(ext, "flac", 4) == 0)
		return std::string("audio/flac");
	else if(strncasecmp(ext, "aac", 3) == 0)
		return std::string("audio/aac");
	else if(strncasecmp(ext, "opus", 4) == 0)
		return std::string("audio/opus");
	else if(strncasecmp(ext, "wav", 3) == 0)
		return std::string("audio/wav");
	else if(strncasecmp(ext, "weba", 4) == 0)
		return std::string("audio/webm");
	//Video
	else if(strncasecmp(ext, "mkv", 3) == 0)
		return std::string("video/x-matroska");
	else if(strncasecmp(ext, "mp4", 3) == 0)
		return std::string("video/mp4");
	else if(strncasecmp(ext, "webm", 4) == 0)
		return std::string("video/webm");
	else if(strncasecmp(ext, "avi", 3) == 0)
		return std::string("video/x-msvideo");
	else if(strncasecmp(ext, "mpeg", 4) == 0)
		return std::string("video/mpeg");
	else if(strncasecmp(ext, "ogv", 3) == 0)
		return std::string("video/ogg");
	else if(strncasecmp(ext, "ts", 2) == 0)
		return std::string("video/mp2t");
	else
		return std::string();
}

bool util::FileExists(const char* filename)
{
	struct stat info;
	return (stat(filename, &info) == 0);
}
bool util::IsDirectory(const char* filename)
{
	struct stat info;
	if(stat(filename, &info) == -1)
		throw std::system_error(errno, std::generic_category());
	return S_ISDIR(info.st_mode);
}
bool util::IsRegularFile(const char* filename)
{
	struct stat info;
	if(stat(filename, &info) == -1)
		throw std::system_error(errno, std::generic_category());
	return S_ISREG(info.st_mode);
}
bool util::IsSymbolicLink(const char* filename)
{
	struct stat info;
	if(stat(filename, &info) == -1)
		throw std::system_error(errno, std::generic_category());
	return S_ISLNK(info.st_mode);
}
bool util::IsChildDirectory(std::string parent, std::string child)
{
	return false;
}
std::pair<std::vector<std::string>, std::vector<std::string>> util::GetDirectoryContents(std::string dir)
{
	std::vector<std::string> ret_dirs, ret_files;
	std::queue<std::string> directories;
	directories.push(dir);

	while(directories.size())
	{
		DIR* dir = opendir(directories.front().c_str());
		if(dir == nullptr)
			continue;
		struct dirent* ent;
		while((ent = readdir(dir)) != nullptr)
		{
			std::string filename {ent->d_name};
			std::string filepath {directories.front()};
			filepath += "/";
			filepath += filename;
			if(IsDirectory(filepath.c_str()))
			{
				if(filename == "." || filename == ".." || IsSymbolicLink(filepath.c_str()))
					continue;

				directories.push(filepath);
				ret_dirs.push_back(filepath);
			}
			else if(IsRegularFile(filepath.c_str()))
			{
				ret_files.push_back(filepath);
			}
		}
		closedir(dir);
		directories.pop();
	}

	return std::make_pair(ret_files, ret_dirs);
}
