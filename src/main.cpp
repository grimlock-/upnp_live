#include <iostream>
#include <string>
#include <chrono>
#include <atomic>
#include <thread>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <csignal>
//#include <signal.h>
#include <unistd.h>
#include <upnp/upnp.h>

#include "InitOptions.h"
#include "Config.h"
#include "Server.h"
#include "util.h"
#include "Logging.h"
#include "DirectoryMonitor.h"

#ifdef UPNP_LIVE_DEBUG
#include <sys/time.h>
#include <sys/resource.h>
#endif

using namespace upnp_live;

static std::atomic_flag shutdown_flag(ATOMIC_FLAG_INIT);
static std::atomic<char> shutdown_count;
static Server* server;
static Logger* logger;
static DirectoryMonitor* dirMonitor;

extern "C" void handle_signal(int signal)
{
	if(++shutdown_count >= 3)
		std::abort();
	shutdown_flag.clear();
}




#if UPNP_VERSION_MINOR <= 8
int getInfoCallback(const char* filename, UpnpFileInfo* info, const void* cookie) try
#else
int getInfoCallback(const char* filename, UpnpFileInfo* info, const void* cookie, const void** request_cookie) try
#endif
{
	return server->GetInfo(filename, info);
}
catch(std::exception& e)
{
	logger->Log_fmt(error, "Error getting virtual file info: %s\n%s\n", filename, e.what());
	return -1;
}
#if UPNP_VERSION_MINOR <= 8
UpnpWebFileHandle openCallback(const char* filename, enum UpnpOpenFileMode mode, const void* cookie) try
#else
UpnpWebFileHandle openCallback(const char* filename, enum UpnpOpenFileMode mode, const void* cookie, const void* request_cookie) try
#endif
{
	return server->Open(filename, mode);
}
catch(const std::runtime_error& e)
{
	logger->Log_fmt(error, "Error opening virtual file: %s\n%s\n", filename, e.what());
	return nullptr;
}
catch(const std::bad_alloc& e)
{
	logger->Log_fmt(error, "Error opening virtual file: %s\n%s\n", filename, e.what());
	return nullptr;
}
#if UPNP_VERSION_MINOR <= 8
int closeCallback(UpnpWebFileHandle fh, const void* cookie) try
#else
int closeCallback(UpnpWebFileHandle fh, const void* cookie, const void* request_cookie) try
#endif
{
	return server->Close(fh);
}
catch(const std::bad_alloc& e)
{
	logger->Log_fmt(error, "Error closing virtual file: %s\n", e.what());
	return -1;
}
#if UPNP_VERSION_MINOR <= 8
int readCallback(UpnpWebFileHandle handle, char* buf, std::size_t len, const void* cookie) try
#else
int readCallback(UpnpWebFileHandle handle, char* buf, std::size_t len, const void* cookie, const void* request_cookie) try
#endif
{
	return server->Read(handle, buf, len);
}
catch(const std::bad_alloc& e)
{
	logger->Log_fmt(error, "Error reading virtual file: %s\n", e.what());
	return -1;
}
#if UPNP_VERSION_MINOR <= 8
int seekCallback(UpnpWebFileHandle fh, long offset, int origin, const void* cookie)
#else
int seekCallback(UpnpWebFileHandle fh, long offset, int origin, const void* cookie, const void* request_cookie)
#endif
{
	return server->Seek(fh, offset, origin);
}
#if UPNP_VERSION_MINOR <= 8
int writeCallback(UpnpWebFileHandle fh, char* buf, std::size_t len, const void* cookie)
#else
int writeCallback(UpnpWebFileHandle fh, char* buf, std::size_t len, const void* cookie, const void* request_cookie)
#endif
{
	return server->Write(fh, buf, len);
}
int IncomingEvent(Upnp_EventType eventType, const void* event, void* cookie) try
{
	return server->IncomingEvent(eventType, event);
}
catch(std::exception& e)
{
	std::string err = "Error executing";
	switch(eventType)
	{
		case UPNP_CONTROL_ACTION_REQUEST:
			err += " control action request";
			break;
		case UPNP_CONTROL_GET_VAR_REQUEST:
			err += " service variable request";
			break;
		case UPNP_EVENT_SUBSCRIPTION_REQUEST:
			err += " subscription request";
			break;
		case UPNP_EVENT_AUTORENEWAL_FAILED:
			err += " client auto-renewal";
			break;
		case UPNP_EVENT_SUBSCRIPTION_EXPIRED:
			err += " client subscription expiration";
			break;
		default:
			break;
	}
	err += " event: ";
	err += e.what();
	err += "\n";
	logger->Log(error, err);
	return 0;
}


int main(int argc, char* argv[])
{
#ifdef UPNP_LIVE_DEBUG
	//Force enable core dumps
	struct rlimit core;
	core.rlim_cur = core.rlim_max = RLIM_INFINITY;
	setrlimit(RLIMIT_CORE, &core);
#endif
	//FIXME - signal()'s behaviour is undefined in multi-threaded environments, look and see if this holds true for sigaction() as well
	/*struct sigaction signal_struct {0};
	signal_struct.sa_handler = &handle_signal;
	sigaction(SIGINT, &signal_struct, nullptr);
	sigaction(SIGTERM, &signal_struct, nullptr);*/
	auto sigres = std::signal(SIGINT, handle_signal);
	if(sigres == SIG_ERR)
	{
		std::cout << "Error setting SIG_ERR handler" << std::endl;
		std::abort();
	}
	sigres = std::signal(SIGTERM, handle_signal);
	{
		std::cout << "Error setting SIGTERM handler" << std::endl;
		std::abort();
	}


	//Process arguments and config file
	InitOptions options;
	options.event_callback = &IncomingEvent;
	ParseArguments(argc, argv, options);
	if(!options.config_file.empty())
		ParseConfigFile(options);


	//Start logger
	logger = new Logger(&std::cout, &std::cerr);
	Logger::SetLogger(logger);
	logger->SetLogLevel(static_cast<log_level>(options.log_level));
	const char* lv;
	switch(options.log_level)
	{
		case 0:
			lv = "disabled";
			break;
		case 1:
			lv = "always";
			break;
		case 2:
			lv = "error";
			break;
		case 3:
			lv = "warning";
			break;
		case 4:
			lv = "info";
			break;
		case 5:
			lv = "verbose";
			break;
		case 6:
			lv = "debug";
			break;
		case 7:
			lv = "verbose debug";
			break;
		default:
			lv = "bad value";
			break;
	}
	logger->Log_fmt(always, "Logger set to %s\n", lv);

	logger->Log_fmt(debug, "Web root: %s\nconfig file: %s\nlog file: %s\nlog level: %d\naddress: %s\ninterface: %s\nrun in background: %s\nstreams:\n", \
	options.web_root.c_str(), \
	options.config_file.c_str(), \
	options.log_file.c_str(), \
	options.log_level, \
	options.address.c_str(), \
	options.interface.c_str(), \
	std::to_string(options.background).c_str());
	for(auto& str : options.streams)
	{
		logger->Log_fmt(debug, "  name: %s\n  status handler: %s %s\n  av handler: %s %s\n  transcoder: %s\n  mime type: %s\n  buffer size: %d\n\n", str.name.c_str(), \
		str.status_handler.c_str(), \
		str.status_args.c_str(), \
		str.av_handler.c_str(), \
		str.av_args.c_str(), \
		str.transcoder.c_str(), \
		str.mime_type.c_str(), \
		str.buffer_size);
	}
	logger->Log(debug, "files:\n");
	for(auto& f : options.files)
	{
		logger->Log_fmt(debug, "  Name: %s\n  path: %s\n  mimetype: %s\n", f.name.c_str(), f.path.c_str(), f.mime_type.c_str());
	}
	logger->Log(debug, "directories:\n");
	for(auto& d: options.directories)
	{
		logger->Log_fmt(debug, "  %s\n", d.path.c_str());
	}

	//Daemonize if needed
	if(options.background)
	{
		logger->Log(info, "Daemon flag set. Forking now\n");
		if(daemon(1, 0) == -1)
			logger->Log(error, "Error forking process to run in background\n");
	}

	//Library init
	int result;
	if(!options.interface.empty())
		result = UpnpInit2(options.interface.c_str(), options.port);
	else if(options.address.empty())
		result = UpnpInit2(nullptr, options.port);
	else
		result = UpnpInit(options.address.c_str(), options.port);
	if(result != UPNP_E_SUCCESS)
	{
		std::string err = "Error initializing UPnP library: ";
		switch(result)
		{
			case UPNP_E_INVALID_INTERFACE:
				err += "Invalid interface\n";
				break;
			case UPNP_E_SOCKET_BIND:
				err += "Could not bind to socket\n";
				break;
			case UPNP_E_OUTOF_MEMORY:
				err += "Out of memory\n";
				break;
			case UPNP_E_LISTEN:
				err += "Error listening to socket\n";
				break;
			case UPNP_E_OUTOF_SOCKET:
				err += "Error creating socket\n";
				break;
			case UPNP_E_INTERNAL_ERROR:
				err += "Internal error\n";
				break;
			default:
				err += "Reason unknown\n";
				break;
		}
		std::cerr << err;
		return 1;
	}

	auto timet = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::stringstream ss;
	ss << "UPnP library initialized  " << std::put_time(std::localtime(&timet), "%F %T") << "\nIP Address: " << UpnpGetServerIpAddress() << "\nPort: " << UpnpGetServerPort() << "\n";
	logger->Log(info, ss.str());

	//Server init
	auto serverOne = std::chrono::steady_clock::now();
	try
	{
		server = new Server(options);
	}
	catch (std::exception& e)
	{
		logger->Log_fmt(error, "Server initialization failed\n%s\n", e.what());
		return 1;
	}

	{
		auto serverTwo = std::chrono::steady_clock::now();
		auto milli = std::chrono::duration_cast<std::chrono::milliseconds>(serverTwo-serverOne).count();
		ss.str("");
		ss << "Server initialized in " << milli << "ms\n";
		logger->Log(info, ss.str());
	}


	//Search directories for files
	if(options.directories.size() > 0)
	{
		try
		{
			dirMonitor = new DirectoryMonitor();
		}
		catch(std::system_error& e)
		{
			logger->Log_fmt(error, "Error creating Directory Monitor object, the server will not react to filesystem changes\nError: %s\n", e.what());
		}

		std::size_t watch_count = 0;

		//TODO - ensure each specified directory is a child of the web root
		for(auto& dir : options.directories)
		{
			std::string path = options.web_root + "/" + dir.path;
			if(logger->GetLogLevel() < verbose)
				logger->Log_fmt(info, "Monitoring directory: %s\n", path.c_str());
			try
			{
				dirMonitor->AddDirectory(path);
				++watch_count;
			}
			catch(std::system_error& e)
			{
				logger->Log_fmt(error, "Error creating watch for %s, the server will not react to filesystem changes in this directory\nError: %s\n", path.c_str(), e.what());
			}
			auto contents = util::GetDirectoryContents(path);
			for(auto& file : contents.first)
			{
				//files
				FileOptions f;
				std::string fname;
				f.name = file.substr(file.find_last_of("/")+1);
				f.path = file.substr(options.web_root.size() + 1);
				options.files.push_back(f);
			}
			if(dirMonitor != nullptr)
			{
				for(auto& d : contents.second)
				{
					//directories
					try
					{
						dirMonitor->AddDirectory(d);
						++watch_count;
					}
					catch(std::invalid_argument& e) {logger->Log_fmt(error, "Directory does not exist: %s\n", d.c_str()); continue;}
					catch(std::system_error& e) {logger->Log_fmt(error, "Error creating inotify watch: %s\n", e.what()); continue;}
				}
			}
		}

		if(!watch_count)
		{
			delete dirMonitor;
			dirMonitor = nullptr;
		}
		else
		{
			logger->Log(info, "Directory Monitor created\n");
		}
	}

	logger->Log(debug, "files after parsing directories:\n");
	for(auto& f : options.files)
	{
		logger->Log_fmt(debug, "  Friendly name: %s\n  path: %s\n  mimetype: %s\n", f.name.c_str(), f.path.c_str(), f.mime_type.c_str());
	}

	//Add streams from options
	if(options.streams.size() > 0)
		server->AddStreams(options.streams);
	if(options.files.size() > 0)
		server->AddFiles(options.files);

	//Add callbacks
	if(UpnpVirtualDir_set_GetInfoCallback(&getInfoCallback) != UPNP_E_SUCCESS)
		logger->Log(error, "Error registering GetInfo callback\n");
	if(UpnpVirtualDir_set_OpenCallback(&openCallback) != UPNP_E_SUCCESS)
		logger->Log(error, "Error registering Open callback\n");
	if(UpnpVirtualDir_set_CloseCallback(&closeCallback) != UPNP_E_SUCCESS)
		logger->Log(error, "Error registering Close callback\n");
	if(UpnpVirtualDir_set_ReadCallback(&readCallback) != UPNP_E_SUCCESS)
		logger->Log(error, "Error registering Read callback\n");
	if(UpnpVirtualDir_set_WriteCallback(&writeCallback) != UPNP_E_SUCCESS)
		logger->Log(error, "Error registering Write callback\n");
	if(UpnpVirtualDir_set_SeekCallback(&seekCallback) != UPNP_E_SUCCESS)
		logger->Log(error, "Error registering Seek callback\n");

	//Main loop
	if(!options.background)
		logger->Log(info, "No background flag set. Imma' just chill and let you know what's what\n");
	shutdown_flag.test_and_set();
	while(shutdown_flag.test_and_set())
	{
		if(dirMonitor != nullptr)
		{
			std::vector<DirectoryMonitorEvent> changes {dirMonitor->Update()};
			for(auto& e : changes)
			{
				switch(e.type)
				{
					case file_create:
					{
						logger->Log_fmt(info, "Adding %s to content directory\n", e.path.c_str());
						FileOptions f;
						f.name = e.path.substr(e.path.find_last_of("/")+1);
						f.path = e.path.substr(options.web_root.size()+1);
						try
						{
							server->AddFile(f);
						}
						catch(std::exception& e) {logger->Log_fmt(error, "Error adding newly created file %s: %s\n", f.path.c_str(), e.what());}
					}
					break;

					case file_delete:
					{
						logger->Log_fmt(info, "Removing %s from content directory\n", e.path.c_str());
						server->RemoveFile(e.path.substr(e.path.find_last_of("/")+1));
					}
					break;

					default:
						//Don't really care about any other events
					break;
				}
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	logger->Log(info, "Stopping server...\n");
	if(dirMonitor != nullptr)
		delete dirMonitor;
	delete server;
	UpnpFinish();
	delete logger;
	return 0;
}

