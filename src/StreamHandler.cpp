/*
 *	This is the class that actually relays the stream av data
 *
 *	UpnpWebFileHandle = typedef for void*. It only holds whatever data you want it to hold
*/
#include <iostream>
#include <vector>
#include <time.h> //time_t
#include <signal.h> //SIGTERM
#include <sys/wait.h> //waitpid

#ifdef UPNP_LIVE_DEBUG
#include <errno.h>
#endif

#include "StreamHandler.h"
#include "util.h"
using namespace upnp_live;

//Static var initializations
std::vector<OpenStreamHandle> StreamHandler::openStreams = {};
Server* StreamHandler::server = nullptr;

StreamHandler::StreamHandler()
{
}
StreamHandler::~StreamHandler()
{
}

/*
 * Struct File_Info {
 * 	off_t file_length;
 * 	time_t last_modified;
 * 	int is_directory;
 * 	int is_readable;
 * 	DOMString content_type; -> starts as NULL, must be set to a dynamically allocated DOMString (aka c string)
 * }
*/
int StreamHandler::getInfoCallback(const char* filename, UpnpFileInfo* info, const void* cookie)
{
	std::cout << "[StreamHandler] getInfo() called:\n" << "\tFilename: " << filename << std::endl;

	std::string fname = filename;
	std::vector<std::string> uri = util::SplitString(fname, '/');
	if(uri[0] != "res" || uri.size() < 3)
	{
		std::cerr << "[StreamHandler] Bad URI\n";
		return -1;
	}

	time_t timestamp;
	time(&timestamp);
	DOMString ctype = ixmlCloneDOMString("video/mpeg");
	DOMString headers = ixmlCloneDOMString("");
	UpnpFileInfo_set_FileLength(info, -1);
	UpnpFileInfo_set_LastModified(info, timestamp);
	UpnpFileInfo_set_IsDirectory(info, 0);
	UpnpFileInfo_set_IsReadable(info, 1);
	UpnpFileInfo_set_ContentType(info, ctype);
	UpnpFileInfo_set_ExtraHeaders(info, headers);
	//std::cout << "[StreamHandler] Returned info for stream #" << uri[1] << " and quality level " << uri[2] << std::endl;
	return 0;
}
UpnpWebFileHandle StreamHandler::openCallback(const char* filename, enum UpnpOpenFileMode mode, const void* cookie)
{
	std::cout << "[StreamHandler] open() called:\n" << "\tFilename: " << filename << "\n\tFile Mode: " << mode << std::endl;
	if(mode == UPNP_WRITE)
	{
		std::cout << "[StreamHandler] Bad file mode" << std::endl;
		return nullptr;
	}
	std::string fname = filename;
	std::vector<std::string> uri = util::SplitString(fname, '/');
	if(uri[0] != "res" || uri.size() < 3)
	{
		std::cerr << "[StreamHandler] Bad URI\n";
		return nullptr;
	}
	std::vector<OpenStreamHandle>::iterator i = StreamHandler::openStreams.begin();
	while(i != StreamHandler::openStreams.end())
	{
		//FIXME - I would like to allow multiple clients to simultaneously access a stream from a single handle, but that won't work
		//with a fifo, so it just rejects subsequent open requests for now
		if(i->stream->getObjectID() == stoi(uri[1]) && i->quality == uri[2])
		{
			std::cout << "[StreamHandler] File handle already exists!" << std::endl;
			//Sony Bravia TVs can't read the stream unless this line is uncommented
			return nullptr;
			//Roku TVs can't read the stream unless this line is uncommented
			//return reinterpret_cast<UpnpWebFileHandle>(&(*i));
		}
		i++;
	}
	OpenStreamHandle newHandle = {0};
	newHandle.stream = server->getCDS()->getStream(stoul(uri[1])-1);
	newHandle.quality = uri[2];
	if(OpenStream(&newHandle) != 0)
	{
		std::cout << "[StreamHandler] Cannot open stream. Error creating child process!" << std::endl;
		return nullptr;
	}
	std::cout << "[StreamHandler] Successfully created child process for stream " << newHandle.stream->getObjectID() << std::endl;
	StreamHandler::openStreams.push_back(newHandle);
	//The thread has to use the handle in the vecotr so it doesn't go out of scope
	/*if(pthread_create(newHandle.readThread, nullptr, &streamReadThread, &StreamHandler::openStreams.back()) != 0)
	{
		StreamHandler::openStreams.pop_back();
		CloseStream(newHandle);
	}*/
	return reinterpret_cast<UpnpWebFileHandle>(&StreamHandler::openStreams.back());
}
//int StreamHandler::close(UpnpWebFileHandle fh)
int StreamHandler::closeCallback(UpnpWebFileHandle fh, const void* cookie)
{
	std::cout << "[StreamHandler] close() called" << std::endl;
	OpenStreamHandle* handle = reinterpret_cast<OpenStreamHandle*>(fh);
	CloseStream(handle);
	std::vector<OpenStreamHandle>::iterator i = StreamHandler::openStreams.begin();
	while(i != StreamHandler::openStreams.end())
	{
		if(&(*i) == handle)
		{
			StreamHandler::openStreams.erase(i);
			break;
		}
		i++;
	}
	return 1;
}
//int StreamHandler::read(UpnpWebFileHandle fh, char* buf, size_t len)
int StreamHandler::readCallback(UpnpWebFileHandle fh, char* buf, size_t len, const void* cookie)
{
	std::cout << "[StreamHandler] read() called:\n\tBytes to read: " << len << std::endl;
	OpenStreamHandle* handle = reinterpret_cast<OpenStreamHandle*>(fh);
	pid_t status = waitpid(handle->childProcess, nullptr, WNOHANG);
	int result = read(handle->pipe, buf, len);
	/*while(result == 0 && status == 0)
	{
		struct timespec sleep_len = {0};
		sleep_len.tv_nsec = 1000000; //1ms
		nanosleep(&sleep_len, nullptr);
		//sleep
		status = waitpid(handle->childProcess, nullptr, WNOHANG);
		result = read(handle->pipe, buf, len);
	}*/
	switch(result)
	{
		case 0:
			if(status == 0)
			{
				//Read EOF but child process is still alive (it's probably trying to shutdown)
				std::cout << "OK, we do actually reach that point" << std::endl;
			}
			else if(status < 0)
			{
				std::cout << "Error probing child process status" << std::endl;
#ifdef UPNP_LIVE_DEBUG
				std::cout << "Error: " << errno << std::endl;
#endif
			}
			else
			{
				std::cout << "[StreamHandler] Stream " << handle->stream->getObjectID() << " has been closed" << std::endl;
				std::cout << "URL: " << handle->stream->getURL() << "\n\tQuality: " << handle->quality << std::endl;
				//Process exited
			}
			break;
		case -1:
			std::cout << "Error while reading from pipe" << std::endl;
#ifdef UPNP_LIVE_DEBUG
			std::cout << "Error: " << errno << std::endl;
#endif
			break;
		default:
			std::cout << "Read " << result << " bytes from pipe" << std::endl;
			break;
	}
	
	return result;
}
int StreamHandler::seekCallback(UpnpWebFileHandle fh, long offset, int origin, const void* cookie)
{
	std::cout << "[StreamHandler] seek() called:\n" << "\tOffset: " << offset << "\n\tOrigin: " << origin << std::endl;
	return 0;
}
int StreamHandler::writeCallback(UpnpWebFileHandle fh, char* buf, size_t len, const void* cookie)
{
	//OpenStreamHandle* handle = reinterpret_cast<OpenStreamHandle*>(fh);
	std::cout << "[StreamHandler] write() called:" << std::endl;
	return 0;
}
//Handles creation of sub-process
int StreamHandler::OpenStream(OpenStreamHandle* handle)
{
	std::cout << "[StreamHandler] Opening stream:\n\tURL: " << handle->stream->getURL() << "\n\tQuality: " << handle->quality << std::endl;
	int pipefd[2];
	if(pipe(pipefd) == -1)
	{
		std::cout << "Failed to create data pipe" << std::endl;
		return -1;
	}
	pid_t pid = fork();
	if(pid == 0) //Child
	{
		if(dup2(pipefd[1], STDOUT_FILENO) == -1)
		{
			exit(EXIT_FAILURE);
		}
		close(pipefd[0]);
		execlp("streamlink", "streamlink", "--stdout", handle->stream->getURL().c_str(), handle->quality.c_str(), static_cast<char*>(0));
		exit(EXIT_FAILURE); //Exit in case execlp fails or the file doesn't exist
	}
	else //Parent
	{
		close(pipefd[1]); //close the write-only file descriptor
		handle->pipe = pipefd[0];
		handle->childProcess = pid;
		return 0;
	}
}
void StreamHandler::CloseStream(OpenStreamHandle* handle)
{
	//pthread_kill(handle->streamReadThread, SIGTERM);
	std::cout << "[StreamHandler] Stopping process for stream " << handle->stream->getObjectID() << std::endl;
	kill(handle->childProcess, SIGTERM);
	waitpid(handle->childProcess, nullptr, 0);
	std::cout << "[StreamHandler] Child process stopped" << std::endl;
	//pthread_join(handle->streamReadThread, nullptr);
}
void StreamHandler::CloseAllStreams()
{
	if(openStreams.size() <= 0)
		return;
	std::vector<OpenStreamHandle>::iterator i = openStreams.begin();
	while(i != openStreams.end())
	{
		kill(i->childProcess, SIGTERM);
	}
}
void StreamHandler::setServer(Server* srv)
{
	server = srv;
}
/*void* StreamHandler::streamReadThread(void* arg)
{
	OpenStreamHandle* handle = reinterpret_cast<OpenStreamHandle*>(arg);
	char buffer[8192];
}*/
