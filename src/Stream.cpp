#include "Stream.h"
#include "util.h"
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <limits> //max int
#include <sys/wait.h>
#include <strings.h> //strncasecmp
#include <assert.h>
using namespace upnp_live;

Stream::Stream(const char* link) : ContentContainer(), url(link)
{
	//url = link;
	stream_is_live = false;
	baseResURI = "";
	setParentID(0);
}
//Manually defined copy-constructor because atomic variables have their CCs deleted, thereby "infecting" any object that uses them as private vars
Stream::Stream(const Stream& other) : ContentContainer(other), url(other.url), baseResURI(other.baseResURI), resolutions(other.resolutions), stream_is_live(other.stream_is_live.load()) {}
Stream::~Stream() {}

void Stream::setURL(const char* link)
{
	url = link;
	if(resolutions.size() > 0)
	{
		stream_is_live = false;
		resolutions.clear();
	}
}
std::string Stream::getURL()
{
	return url;
}

void Stream::populateStreams()
{
	if(url.size() == 0)
		return;

	int pipefd[2];
	if(pipe(pipefd) == -1)
	{
		std::cout << "Failed to create data pipe" << std::endl;
		return;
	}
	pid_t pid = fork();

	if(pid == -1)
	{
		std::cout << "Failed to create child process" << std::endl;
		return;
	}
	if(pid == 0) //Child
	{
		if(dup2(pipefd[1], STDOUT_FILENO) == -1)
		{
			std::cerr << "Error: Failed to link pipe to stdout\n";
			exit(EXIT_FAILURE);
		}
		close(pipefd[0]);
		if(util::FileExists("scripts/listStreams.py"))
			execl("scripts/listStreams.py", "listStreams.py", url.c_str(), static_cast<char*>(0));
		else
			std::cerr << "Error: Script does not exist";
		exit(EXIT_FAILURE); //Exit in case execl fails or the file doesn't exist
	}
	else //Parent
	{
		close(pipefd[1]); //close the write-only file descriptor
		waitpid(pid, nullptr, 0);
		std::string output;
		char character, result;
		result = read(pipefd[0], &character, 1);
		while(result > 0)
		{
			output += character;
			result = read(pipefd[0], &character, 1);
		}
		//pipefd[0] >> output;
		if(strncasecmp(output.c_str(), "error", 5) == 0)
		{
			std::cerr << "Error getting quality levels for \"" << url << "\":\n" << output << std::endl;
			stream_is_live = false;
		}
		else if(output.find(',') != std::string::npos)
		{
			this->addResolutions(upnp_live::util::SplitString(output, ','));
			stream_is_live = true;
		}
		else
		{
			this->addResolution(output);
			stream_is_live = true;
		}
		/*size_t com1 = 0, com2 = output.find(',');
		if(com2 == std::string::npos)
		{
			std::cout << "Bad output getting stream resolutions" << endl;
			return;
		}
		while(com2 != std::string::npos)
		{
			this->addResolution(output.substr((com1 == 0 ? 0 : com1+1), com2-com1-1));
			com1 = com2;
			com2 = output.find(',', com1+1);
		}
		this->addResolution(output.substr(com1+1)); //Add final list item
		*/
	}
}

std::string Stream::getMetadataXML()
{
	std::stringstream ss;
	//FIXME - Replace childCount number with function call after heartbeat thread is implemented
	ss << "<container id=\"" << getObjectID() << "\" parentID=\"" << getParentID() << "\" childCount=\"5\" restricted=\"1\" searchable=\"0\">\n" << \
		"<dc:title>" << getURL() << "</dc:title>\n" << \
		"<upnp:class>object.container.storageFolder</upnp:class>\n" << \
	"</container>\n";
	return ss.str();
}
std::string Stream::getChildrenXML(int requestedCount, unsigned int startingIndex, int& numReturned)
{

	//TODO - After implementing the heartbeat thread, put a check here to see if the stream is live or not
	//TODO - also get rid of this call to populateStreams()
	if(resolutions.empty())
		populateStreams();

	std::stringstream xml;
	if(startingIndex >= resolutions.size())
	{
		numReturned = 0;
		std::string s = "";
		return s;
	}
	int remainingItems = requestedCount;
	if(requestedCount == 0)
		remainingItems = std::numeric_limits<int>::max();
		
	int count = 0;
	std::string s;
	while(count+startingIndex < resolutions.size() && remainingItems > 0)
	{
		/*xml << "<item id=\"" << getObjectID() << "-" << startingIndex+count+1 << "\" parentID=\"" << getObjectID() << "\" restricted=\"1\">\n" \
		"<dc:title>" << *i << "</dc:title>\n" \
		"<upnp:class>object.item.videoItem</upnp:class>\n" \
		"<res id=\"" << getObjectID() << "-" << startingIndex+count+1 << "\" protocolInfo=\"http-get:*:video/MP2T:*\">" << url << "</res>\n" \
		"</item>\n";*/
		s = getChildXML(startingIndex+count);
		if(strncasecmp(s.c_str(), "error", 5) == 0)
			return s;
		xml << s;
		count++;
		remainingItems--;
	}
	numReturned = count;
	return xml.str();
}

std::string Stream::getChildXML(unsigned int index)
{
	if(resolutions.empty())
		populateStreams();

	std::stringstream xml;
	if(index >= resolutions.size())
	{
		std::string s = "";
		return s;
	}
	xml << "<item id=\"" << getObjectID() << "-" << index+1 << "\" parentID=\"" << getObjectID() << "\" restricted=\"1\">\n" \
	"<dc:title>" << resolutions[index] << "</dc:title>\n" \
	"<upnp:class>object.item.videoItem</upnp:class>\n" \
	"<res id=\"" << getObjectID() << "-" << index+1 << "-stream\" protocolInfo=\"http-get:*:video/mpeg:*\">" << getResURI(index) << "</res>\n" \
	"</item>\n";
	return xml.str();
}
int Stream::getChildCount()
{
	if(resolutions.empty())
		populateStreams();

	return resolutions.size();
}




void Stream::addResolution(std::string res)
{
	resolutions.push_back(res);
}
void Stream::addResolutions(std::vector<std::string> res)
{
	std::vector<std::string>::iterator i = res.begin();
	while(i != res.end())
	{
		resolutions.push_back(*i);
		i++;
	}
}
void Stream::removeResolution(std::string res)
{
	std::vector<std::string>::iterator i = resolutions.begin();
	while(i != resolutions.end())
	{
		if(*i == res)
		{
			resolutions.erase(i);
			break;
		}
		i++;
	}
}
void Stream::clearResolutions()
{
	resolutions.clear();
}




void Stream::setBaseResURI(const char* newRes)
{
	baseResURI = newRes;
}
void Stream::setBaseResURI(std::string newRes)
{
	baseResURI = newRes;
}
std::string Stream::getResURI(int index)
{
	std::stringstream ss;
	ss << baseResURI << getObjectID() << "/" << resolutions[index] << "/file.mts";
	return ss.str();
}




bool Stream::isLive()
{
	/*if(!stream_is_live.test_and_set())
	{
		stream_is_live.clear();
		return false;
	}
	else
	{
		return true;
	}*/
	return stream_is_live.load();
}
