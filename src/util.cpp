#include "util.h"
#include <sys/stat.h>
#include <strings.h> //strncasecmp
using namespace upnp_live;

/*
 * Splits the input string into a vector of smaller strings using the provided character delimiter
 * If the input is less than 3 characters or the delimiter isn't found, the input string is the vector's only element
**/
std::vector<std::string> util::SplitString(std::string str, char delimiter)
{
	//Trim
	while(str.front() == delimiter)
		str.erase(0, 1);
	while(str.back() == delimiter)
		str.erase(str.size()-1, 1);

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
//Replace ampersand and angle bracket characters with their escaped versions
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
bool util::FileExists(const char* filename)
{
	struct stat info;
	return (stat(filename, &info) == 0);
}
std::string util::GetMimeType(const char* ext)
{
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
