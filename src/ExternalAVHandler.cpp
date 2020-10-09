#include "ExternalAVHandler.h"
#include "util.h"
#include <vector>
#include <time.h> //time_t
#include <signal.h> //SIGTERM
#include <sys/wait.h> //waitpid
#include <errno.h>

using namespace upnp_live;

ExternalAVHandler::ExternalAVHandler(std::vector<std::string>& args): ChildProcess(args)
{
	SourceType = external;
}
ExternalAVHandler::~ExternalAVHandler()
{
	ShutdownProcess();
}

int ExternalAVHandler::Init()
{
	return InitProcess().first;
}

void ExternalAVHandler::Shutdown() noexcept
{
	ShutdownProcess();
}

std::string ExternalAVHandler::GetMimetype()
{
	return "";
}

bool ExternalAVHandler::IsInitialized()
{
	return ProcessInitialized();
}
