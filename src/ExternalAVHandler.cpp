#include "ExternalAVHandler.h"
#include "Exceptions.h"
#include <system_error>
#include <string.h> //strerror
//#include <time.h> //time_t
#include <unistd.h> //read
//#include <signal.h> //SIGTERM
//#include <sys/wait.h> //waitpid
#include <errno.h>

using namespace upnp_live;

ExternalAVHandler::ExternalAVHandler(std::vector<std::string>& args) : ChildProcess(args) { }
ExternalAVHandler::~ExternalAVHandler()
{
	ShutdownProcess();
}

void ExternalAVHandler::Init()
{
	InitProcess(blocking_output);
}

void ExternalAVHandler::Shutdown()
{
	ShutdownProcess();
}

bool ExternalAVHandler::IsInitialized()
{
	return ProcessInitialized();
}

void ExternalAVHandler::SetWriteDestination(std::shared_ptr<Transcoder>& transcoder)
{
	throw std::runtime_error("Use Transcoder::SetInput to connect to External AV Handler");
}

std::string ExternalAVHandler::GetMimetype()
{
	throw std::runtime_error("ExternalAVHandler::GetMimetype() can't be used");
}

int ExternalAVHandler::GetOutputFd()
{
	std::lock_guard<std::mutex> guard(mutex);
	return outputPipe;
}

int ExternalAVHandler::Read(char* buf, size_t len)
{
	std::lock_guard<std::mutex> guard(mutex);
	if(outputPipe <= 0)
		throw std::runtime_error("AV Handler not initialized: ");
	start:
	auto ret = read(outputPipe, (void*) buf, len);
	if(ret > 0)
		return ret;
	if(ret == 0)
	{
		Shutdown();
		throw EofReached();
	}
	auto err = errno;
	switch(ret)
	{
		//case EAGAIN:
		case EWOULDBLOCK:
			//Nothing to read
			return 0;
		break;
		case EINTR:
			//Interrupted read, try again
			goto start;
		break;

		//EBADF, EFAULT, EINVAL, etc.
		default:
			Shutdown();
			std::error_code ec(err, std::system_category());
			throw std::system_error(ec);
		break;
	}
}

