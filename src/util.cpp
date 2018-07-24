#include "util.h"
#include <sys/stat.h>
using namespace upnp_live;

/** Splits the input string into a vector of smaller strings using the provided character delimiter
  *
  * If the input has less than 3 characters or the delimiter isn't found, the vector has the input string as the only element
**/
std::vector<std::string> util::SplitString(std::string str, char delimiter)
{
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
	while(input.find("<") != std::string::npos)
		input.replace(  input.find("<"), 1, "&lt;"  );
	while(input.find(">") != std::string::npos)
		input.replace(  input.find(">"), 1, "&gt;"  );
	while(input.find("\"") != std::string::npos)
		input.replace(  input.find("\""), 1, "&quot;"  );
	return input;
}
bool util::FileExists(const char* filename)
{
	struct stat info;
	return (stat(filename, &info) == 0);
}
