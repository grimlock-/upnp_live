#include <iostream>
#include <chrono>
#include <atomic>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <upnp/upnp.h>

#ifdef UPNP_LIVE_DEBUG
#include <sys/time.h>
#include <sys/resource.h>
#endif

#include "Server.h"
#include "Exceptions.h"
#include "StreamHandler.h"
#include "Config.h"


using namespace upnp_live;

static std::atomic_flag shutdown_flag(ATOMIC_FLAG_INIT);
static std::atomic<char> shutdown_count;

//pthread_t signalThreadVar;
void shutdown(int signal)
{
	shutdown_count++;
	if(shutdown_count >= 3)
		exit(1);
	shutdown_flag.clear();
	StreamHandler::CloseAllStreams();
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
	struct sigaction signal_struct = {0};
	signal_struct.sa_handler = &shutdown;
	sigaction(SIGINT, &signal_struct, nullptr);
	sigaction(SIGTERM, &signal_struct, nullptr);

	//Process arguments and config file
	InitOptions options;
	ParseArguments(argc, argv, options);
	if(options.configFile != "")
		ParseConfigFile(options);
	//Daemonize if needed
	if(options.daemon)
	{
		std::cout << "Daemon flag set. Forking now" << std::endl;
		if(daemon(1, 0) == -1)
			std::cout << "Error forking process for daemon mode" << std::endl;
	}

	//Library init
	int result;
	if(options.address == "")
		result = UpnpInit(nullptr, options.port);
	else
		result = UpnpInit(options.address.c_str(), options.port);
	if(result != UPNP_E_SUCCESS)
	{
		std::cout << "Library init returned error code " << result << std::endl;
		return 1;
	}
	std::cout << "mupnp library initialized\nUsing IP Address " << UpnpGetServerIpAddress() << ":" << UpnpGetServerPort() << std::endl;

	//Server init
	Server* server;
	try
	{
		server = new Server(options);
	}
	catch (ServerInitException& e)
	{
		std::cerr << "Server initialization failed\n" << e.what() << std::endl;
		return 1;
	}
	catch (std::exception& e)
	{
		std::cerr << "Unknown exception\nException message: " << e.what() << std::endl;
		return 1;
	}
	std::cout << "Server object created successfully" << std::endl;
	StreamHandler::setServer(server);

	//Main sleep loop
	if(!options.daemon)
		std::cout << "No daemon flag set. Imma' just chill and let you know what's what" << std::endl;;
	struct timespec sleep_len = {0};
	sleep_len.tv_nsec = 100000000; //100ms
	shutdown_flag.test_and_set();
	while(shutdown_flag.test_and_set())
	{
		nanosleep(&sleep_len, nullptr);
	}

	std::cout << "Stopping server\n";
	delete server;
	UpnpFinish();
	return 0;
}
