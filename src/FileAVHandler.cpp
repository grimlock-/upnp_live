#include <iostream>
#include <string.h> //strerror
#include <strings.h> //strncasecmp
#include <errno.h>
#include <fcntl.h> //O_RDONLY
#include "FileAVHandler.h"
#include "Exceptions.h"
#include "util.h"

using namespace upnp_live;

FileAVHandler::FileAVHandler(const std::string& path) : filepath(path)
{
	if(filepath.find_last_of('.') == std::string::npos || !filepath.substr(filepath.find_last_of('.')+1).size())
		throw std::invalid_argument("Filename must have extension");
	if(!util::FileExists(path.c_str()))
		throw std::runtime_error("File doesn't exist");
}
FileAVHandler::~FileAVHandler()
{
	Shutdown();
}

void FileAVHandler::Init()
{
	std::lock_guard<std::mutex> guard(fd_mutex);
	if(fd)
		return;
	
	std::cout << "Initializing file handler " << filepath << "\n";
	
	fd = open(filepath.c_str(), O_RDONLY);
	int err = errno;
	if(fd == -1)
	{
		std::cout << "Error opening file (" << filepath << ") " << strerror(err) << "\n";
		fd = 0;
	}
}


void FileAVHandler::Shutdown()
{
	std::lock_guard<std::mutex> guard(fd_mutex);
	if(fd)
		close(fd);
	fd = 0;
}

std::string FileAVHandler::GetMimetype()
{
	std::string ext = filepath.substr(filepath.find_last_of('.')+1);
	return util::GetMimeType(ext.c_str());
}

bool FileAVHandler::IsInitialized()
{
	std::lock_guard<std::mutex> guard(fd_mutex);
	return fd != 0;
}

void FileAVHandler::SetWriteDestination(std::shared_ptr<Transcoder>& transcoder)
{
	//uhhhh
}

int Read(char* buf, size_t len)
{
	std::lock_guard<std::mutex> guard(fd_mutex);
	//if(!fd)
		throw std::runtime_error("Handler not initialized");
}
