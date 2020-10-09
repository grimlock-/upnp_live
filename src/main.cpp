#include <iostream>
#include <string>
#include <chrono>
#include <atomic>
#include <thread>
#include <sstream>
#include <iomanip>
#include <signal.h>
#include <unistd.h>
#include <upnp/upnp.h>

#include "InitOptions.h"
#include "Config.h"
#include "Server.h"
#include "util.h"
#include "Logging.h"

#ifdef UPNP_LIVE_DEBUG
#include <sys/time.h>
#include <sys/resource.h>
#endif

using namespace upnp_live;

static std::atomic_flag shutdown_flag(ATOMIC_FLAG_INIT);
static std::atomic<char> shutdown_count;
static Server* server;
static Logger* logger;

void shutdown(int signal)
{
	if(++shutdown_count >= 3)
		exit(1);
	shutdown_flag.clear();
}




#ifdef OLD_UPNP
int getInfoCallback(const char* filename, UpnpFileInfo* info, const void* cookie) try
#else
int getInfoCallback(const char* filename, UpnpFileInfo* info, const void* cookie, const void** request_cookie) try
#endif
{
	return server->GetInfo(filename, info, cookie);
}
catch(std::exception& e)
{
	logger->Log_cc(error, 5, "Error getting virtual file info: ", filename, "\n", e.what(), "\n");
	return -1;
}
#ifdef OLD_UPNP
UpnpWebFileHandle openCallback(const char* filename, enum UpnpOpenFileMode mode, const void* cookie) try
#else
UpnpWebFileHandle openCallback(const char* filename, enum UpnpOpenFileMode mode, const void* cookie, const void* request_cookie) try
#endif
{
	return server->Open(filename, mode, cookie);
}
catch(std::exception& e)
{
	logger->Log_cc(error, 5, "Error opening virtual file: ", filename, "\n", e.what(), "\n");
	return nullptr;
}
#ifdef OLD_UPNP
int closeCallback(UpnpWebFileHandle fh, const void* cookie) try
#else
int closeCallback(UpnpWebFileHandle fh, const void* cookie, const void* request_cookie) try
#endif
{
	return server->Close(fh, cookie);
}
catch(std::exception& e)
{
	logger->Log_cc(error, 3, "Error closing virtual file: ", e.what(), "\n");
	return -1;
}
#ifdef OLD_UPNP
int readCallback(UpnpWebFileHandle handle, char* buf, std::size_t len, const void* cookie) try
#else
int readCallback(UpnpWebFileHandle handle, char* buf, std::size_t len, const void* cookie, const void* request_cookie) try
#endif
{
	return server->Read(handle, buf, len, cookie);
}
catch(std::exception& e)
{
	logger->Log_cc(error, 3, "Error reading virtual file: ", e.what(), "\n");
	return 0;
}
#ifdef OLD_UPNP
int seekCallback(UpnpWebFileHandle fh, long offset, int origin, const void* cookie) try
#else
int seekCallback(UpnpWebFileHandle fh, long offset, int origin, const void* cookie, const void* request_cookie) try
#endif
{
	return server->Seek(fh, offset, origin, cookie);
}
catch(std::exception& e)
{
	logger->Log_cc(error, 3, "Error seeking virtual file: ", e.what(), "\n");
	return -1;
}
#ifdef OLD_UPNP
int writeCallback(UpnpWebFileHandle fh, char* buf, std::size_t len, const void* cookie) try
#else
int writeCallback(UpnpWebFileHandle fh, char* buf, std::size_t len, const void* cookie, const void* request_cookie) try
#endif
{
	return server->Write(fh, buf, len, cookie);
}
catch(std::exception& e)
{
	logger->Log_cc(error, 3, "Error writing virtual file: ", e.what(), "\n");
	return 0;
}
int IncomingEvent(Upnp_EventType eventType, const void* event, void* cookie) try
{
	return server->IncomingEvent(eventType, event, cookie);
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
	struct sigaction signal_struct {0};
	signal_struct.sa_handler = &shutdown;
	sigaction(SIGINT, &signal_struct, nullptr);
	sigaction(SIGTERM, &signal_struct, nullptr);


	//Process arguments and config file
	InitOptions options;
	options.event_callback = &IncomingEvent;
	ParseArguments(argc, argv, options);
	if(options.config_file != "")
		ParseConfigFile(options);


	//Start logger
	logger = new Logger();
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
	logger->Log_cc(always, 3, "Logger set to ", lv, "\n");

	logger->Log_cc(debug, 23, "InitOptions struct:\n", \
	"web root: ", options.web_root.c_str(), "\n", \
	"config file: ", options.config_file.c_str(), "\n", \
	"log file: ", options.log_file.c_str(), "\n", \
	"log level: ", std::to_string(options.log_level).c_str(), "\n", \
	"address: ", options.address.c_str(), "\n", \
	"interface: ", options.interface.c_str(), "\n", \
	"daemonize: ", std::to_string(options.daemon).c_str(), "\n\n", \
	"streams:\n");
	for(auto& str : options.streams)
	{
		logger->Log_cc(debug, 23, str.name.c_str(), "\n", \
		"status handler: ", str.status_handler.c_str(), "\n", \
		"arguments: ", str.status_args.c_str(), "\n", \
		"av handler: ", str.av_handler.c_str(), "\n", \
		"arguments: ", str.av_args.c_str(), "\n", \
		"transcoder: ", str.transcoder.c_str(), "\n", \
		"mime type: ", str.mime_type.c_str(), "\n", \
		"buffer size: ", std::to_string(str.buffer_size).c_str(), "\n\n");
	}
	logger->Log(debug, "files:\n");
	for(auto& f : options.files)
	{
		logger->Log_cc(debug, 9, "Friendly name: ", f.name.c_str(), "\n", \
		"path: ", f.path.c_str(), "\n", \
		"mimetype: ", f.mime_type.c_str(), "\n");
	}

	//Daemonize if needed
	if(options.daemon)
	{
		
		logger->Log(info, "Daemon flag set. Forking now\n");
		if(daemon(1, 0) == -1)
			logger->Log(error, "Error forking process for daemon mode\n");
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
	//logger->Log_cc(always, 7, "UPnP library initialized  ", std::put_time(std::localtime(&timet), "%F %T"), "\nIP Address: ", UpnpGetServerIpAddress(), "\nPort: ", UpnpGetServerPort(), "\n");
	std::cout << "UPnP library initialized  " << std::put_time(std::localtime(&timet), "%F %T") << "\nIP Address: " << UpnpGetServerIpAddress() << "\nPort: " << UpnpGetServerPort() << "\n";

	//Server init
	auto serverOne = std::chrono::steady_clock::now();
	try
	{
		server = new Server(options);
	}
	catch (std::exception& e)
	{
		logger->Log_cc(error, 3, "Server initialization failed\n", e.what(), "\n");
		return 1;
	}

	auto serverTwo = std::chrono::steady_clock::now();
	{
		auto milli = std::chrono::duration_cast<std::chrono::milliseconds>(serverTwo-serverOne).count();
		std::stringstream ss;
		ss << "Server initialized in " << milli << "ms\n";
		logger->Log(info, ss.str());
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

	//Main sleep loop
	if(!options.daemon)
		logger->Log(info, "No daemon flag set. Imma' just chill and let you know what's what\n");
	shutdown_flag.test_and_set();
	while(shutdown_flag.test_and_set())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	logger->Log(info, "Stopping server...\n");
	delete server;
	UpnpFinish();
	delete logger;
	return 0;
}

