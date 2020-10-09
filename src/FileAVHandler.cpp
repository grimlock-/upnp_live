#include <iostream>
#include <string.h> //strerror
#include <strings.h> //strncasecmp
#include <errno.h>
#include <fcntl.h> //O_RDONLY
#include "FileAVHandler.h"
#include "Exceptions.h"
#include "util.h"

using namespace upnp_live;

FileAVHandler::FileAVHandler(const std::string& path) : FilePath(path)
{
	SourceType = file;
}
FileAVHandler::~FileAVHandler()
{
	Shutdown();
}

int FileAVHandler::Init()
{
	std::lock_guard<std::mutex> guard(fd_mutex);
	if(fd)
		throw AlreadyInitializedException();
	
	std::cout << "Initializing file handler " << FilePath << "\n";
	
	fd = open(FilePath.c_str(), O_RDONLY);
	int err = errno;
	if(fd == -1)
	{
		std::cout << "Error opening file (" << FilePath << ") " << strerror(err) << "\n";
		fd = 0;
	}
	return fd;
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
	std::string ext = FilePath.substr(FilePath.find_last_of('.'));
	if(ext.length() == 1)
		return "";
	
	ext.erase(0, 1);
	
	return util::GetMimeType(ext.c_str());
}

bool FileAVHandler::IsInitialized()
{
	std::lock_guard<std::mutex> guard(fd_mutex);
	return fd != 0;
}
