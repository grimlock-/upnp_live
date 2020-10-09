#include <iostream>
#include <stdexcept>
#include <sstream>
#include <strings.h> //strncasecmp
#include "Stream.h"
#include "FileAVHandler.h"
using namespace upnp_live;


//{ Inits
Stream::Stream(
	std::string name,
	std::string mt,
	std::unique_ptr<AVSource>&& avh,
	std::unique_ptr<StatusHandler>&& sh,
	std::unique_ptr<Transcoder>&& tr) :
Name{name},
mimeType{mt},
avHandler{std::move(avh)},
statusHandler{std::move(sh)},
transcoder{std::move(tr)}
{
	logger = Logger::GetLogger();

	if(avHandler == nullptr)
		throw std::invalid_argument("No AV handler");
	if(avHandler->SourceType == external && mimeType.empty())
		throw std::invalid_argument("Mimetype required for external AV handler");
	if(avHandler->SourceType == file && transcoder == nullptr)
		throw std::invalid_argument("File AV handler requires transcoder");
	if(strncasecmp(mimeType.c_str(), "video", 5) != 0 && strncasecmp(mimeType.c_str(), "audio", 5) != 0)
		throw std::invalid_argument("Bad Mimetype");

	try
	{
		StartHeartbeat();
	}
	catch(std::system_error& e)
	{
		std::string s {"Error creating heartbeat thread for "};
		s += Name;
		s += ": ";
		s += e.what();
		throw std::runtime_error(s);
	}

	if(statusHandler == nullptr)
	{
		logger->Log_cc(verbose, 3, "No status handler for ", Name.c_str(), "\n");
		streamIsLive = true;
	}
	if(transcoder == nullptr)
		logger->Log_cc(verbose, 3, "No transcoder for ", Name.c_str(), "\n");

}
int Stream::StartAVHandler()
{
	logger->Log_cc(info, 3, "Starting stream: ", Name.c_str(), "\n");
	UpdateReadTime();
	int fd = avHandler->Init();
	if(transcoder != nullptr)
	{
		try
		{
			transcoder->SetSource(fd);
			fd = transcoder->Init();
		}
		catch(std::exception& e)
		{
			avHandler->Shutdown();
			throw e;
		}
	}
	

	return fd;
}
void Stream::StartHeartbeat()
{
	heartbeatRunning = true;
	heartbeatThread = std::thread(&Stream::Heartbeat, this);
}
//} Inits




//{ Shutdowns
Stream::~Stream()
{
	logger->Log_cc(info, 3, "Destroying stream: ", Name.c_str(), "\n");
	try
	{
		StopHeartbeat();
	}
	catch(std::system_error& e)
	{
		logger->Log_cc(error, 5, "Error stopping heartbeat for ", Name.c_str(), ": ", e.what(), "\n");
	}
}
void Stream::StopAVHandler() try
{
	logger->Log_cc(info, 3, "Stopping stream: ", Name.c_str(), "\n");
	if(transcoder != nullptr)
		transcoder->Shutdown();
	avHandler->Shutdown();
}
catch(std::exception& e)
{
	logger->Log_cc(error, 5, "Error stopping stream ", Name.c_str(), "\n", e.what(), "\n");
}
void Stream::StopHeartbeat()
{
	heartbeatRunning = false;
	if(heartbeatThread.joinable())
		heartbeatThread.join();
}
//} Shutdowns




//Thread to periodically check stream status and shutdown
//the av handler after data stops being read
void Stream::Heartbeat()
{
	namespace chr = std::chrono;
	
	lastStatus = chr::steady_clock::now();
	lastStatus -= chr::seconds(statusInterval.load()+1);
	std::this_thread::sleep_for(chr::milliseconds(100));
	while(heartbeatRunning.load())
	{
		auto now = chr::steady_clock::now();
		auto delta = chr::duration_cast<chr::seconds>(now-lastStatus);
		//status
		if(statusHandler != nullptr && delta.count() >= chr::seconds(statusInterval.load()).count())
		{
			UpdateStatus();
			lastStatus = chr::steady_clock::now();
		}
		//handler timeout
		if(avHandler->IsInitialized() && handlerTimeout.load() > 0)
		{
			now = chr::steady_clock::now();
			{
				std::lock_guard<std::mutex> guard(readTimeMutex);
				delta = chr::duration_cast<chr::seconds>(now-lastRead);
			}
			if(delta.count() >= chr::seconds(handlerTimeout.load()).count())
			{
				logger->Log_cc(info, 3, "[", Name.c_str(), "] timeout reached\n");
				StopAVHandler();
			}
		}
		std::this_thread::sleep_for(chr::milliseconds(100));
	}
}
void Stream::UpdateStatus()
{
	logger->Log_cc(debug, 5, "[", Name.c_str(), "] Getting status for ", Name.c_str(), "\n");
	bool live;

	auto one = std::chrono::steady_clock::now();
	if(avHandler->IsInitialized())
		live = true;
	else
		live = statusHandler->GetStatus();
	auto two = std::chrono::steady_clock::now();
	std::stringstream ss;
	ss << "[" << Name << "] status: " << (live ? "live" : "not live") << " | took " << std::chrono::duration_cast<std::chrono::milliseconds>(two-one).count() << "ms" << "\n";
	logger->Log(verbose, ss.str());
	streamIsLive = live;
}




//{ Simple accessors/mutators
void Stream::SetStatusInterval(int intv)
{
	if(intv > 0)
		statusInterval = intv;
}
int Stream::GetStatusInterval()
{
	return statusInterval.load();
}
std::string Stream::GetMimeType()
{
	if(!mimeType.empty())
		return mimeType;
	
	if(transcoder != nullptr)
		return transcoder->GetMimetype();
	
	return avHandler->GetMimetype();
}
bool Stream::IsLive()
{
	return streamIsLive.load();
}
bool Stream::HasTranscoder()
{
	return transcoder == nullptr;
}
std::string Stream::GetFilePath()
{
	if(avHandler->SourceType != file)
		throw std::runtime_error("Not a file stream");
	
	auto fh = static_cast<FileAVHandler*>(avHandler.get());
	return fh->FilePath;
}
void Stream::UpdateReadTime()
{
	logger->Log_cc(debig_chungus, 3, "[", Name.c_str(), "] Updating read time\n");
	std::lock_guard<std::mutex> guard(readTimeMutex);
	lastRead = std::chrono::steady_clock::now();
}
//} Simple accessors/mutators

