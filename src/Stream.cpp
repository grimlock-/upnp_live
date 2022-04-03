#include <iostream>
#include <stdexcept>
#include <sstream>
#include <typeinfo>
#include <strings.h> //strncasecmp
#include "Stream.h"
#include "FileAVHandler.h"
using namespace upnp_live;


//{ Inits
Stream::Stream(
	std::string name,
	std::string mt,
	std::unique_ptr<AVHandler>&& avh,
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
	try
	{
		if(dynamic_cast<ExternalAVHandler*>(*avHandler) && mimeType.empty())
			throw std::invalid_argument("Mimetype required for external AV handler");
	}
	catch(std::bad_cast& e) { }
	try
	{
		if(dynamic_cast<FileAVHandler*>(*avHandler) && transcoder == nullptr)
			throw std::invalid_argument("Transcoder required for File AV handler");
	}
	catch(std::bad_cast& e) { }
	if(!mimeType.empty() && strncasecmp(mimeType.c_str(), "video", 5) != 0 && strncasecmp(mimeType.c_str(), "audio", 5) != 0)
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
		logger->Log_fmt(verbose, "No status handler for %s\n", Name.c_str());
		streamIsLive = true;
	}
	if(transcoder == nullptr)
		logger->Log_fmt(verbose, "No transcoder for %s\n", Name.c_str());

}
void Stream::Start()
{
	logger->Log_fmt(info, "Starting stream: %s\n", Name.c_str());
	UpdateReadTime();
	if(transcoder != nullptr)
	{
		ExternalAVHandler* ext = nullptr;
		try
		{
			ext = dynamic_cast<ExternalAVHandler>(*avHandler);
		}
		catch(std::bad_cast& e) { }

		if(ext)
		{
			//For ext, the handler process stdout can be duped
			//to the transcoder process' stdin, but then it needs
			//to be started first
			ext->Init();
			transcoder->SetInput(ext);
			transcoder->InitProcess(false);
		}
		else
		{
			transcoder->InitProcess(false);
			avHandler->SetWriteDestination(transcoder);
			avHandler->Init();
		}
	}
	else
	{
		avHandler->Init();
	}
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
	logger->Log_fmt(info, "Destroying stream: %s\n", Name.c_str());
	if(IsLive())
		Stop();
	try
	{
		StopHeartbeat();
	}
	catch(std::system_error& e)
	{
		logger->Log_fmt(error, "Error stopping heartbeat for %s: %s\n", Name.c_str(), e.what());
	}
}
void Stream::Stop()
{
	logger->Log_fmt(info, "Stopping stream: %s\n", Name.c_str());
	if(transcoder != nullptr)
		transcoder->ShutdownProcess();
	avHandler->Shutdown();
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
				logger->Log_fmt(info, "[%s] timeout reached\n", Name.c_str());
				Stop();
			}
		}
		std::this_thread::sleep_for(chr::milliseconds(100));
	}
}
void Stream::UpdateStatus()
{
	logger->Log_fmt(debug, "[%s] Getting status\n", Name.c_str());
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




int Stream::Read(char* buf, size_t len)
{
	if(transcoder == nullptr)
		return avHandler->Read(buf, len);
	else
		return transcoder->Read(buf, len);
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
std::string Stream::GetFilePath()
{
	if(avHandler->HandlerType != file)
		throw std::runtime_error("Not a file stream");
	
	auto fh = static_cast<FileAVHandler*>(avHandler.get());
	return fh->FilePath;
}
void Stream::UpdateReadTime()
{
	logger->Log_fmt(verbose_debug, "[%s] Updating read time\n", Name.c_str());
	std::lock_guard<std::mutex> guard(readTimeMutex);
	lastRead = std::chrono::steady_clock::now();
}
//} Simple accessors/mutators

