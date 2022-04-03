#include <iostream>
#include <sys/wait.h>
#include "ExternalStatusHandler.h"
using namespace upnp_live;

ExternalStatusHandler::ExternalStatusHandler(std::vector<std::string>& args) : ChildProcess(args) {}

bool ExternalStatusHandler::GetStatus()
{
	pid_t pid;
	try
	{
		pid = InitProcess(true).second;
	}
	catch(std::exception& e)
	{
		std::cout << "Exception initializing " << command << ": " << e.what() << "\n";
		return false;
	}
	
	int status = 0;
	waitpid(pid, &status, 0);
	if(WIFEXITED(status))
	{
		unsigned int code = WEXITSTATUS(status);
		ShutdownProcess();
		return !code;
	}
	
	return false;
}
