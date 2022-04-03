#include "Transcoder.h"
#include "Exceptions.h"

using namespace upnp_live;

Transcoder::Transcoder(std::vector<std::string>& args) : ChildProcess(args) { }
Transcoder::~Transcoder()
{
	ShutdownProcess();
}

/*void Transcoder::Init()
{
	InitProcess(false);
}

void Transcoder::Shutdown()
{
	ShutdownProcess();
}

bool Transcoder::IsInitialized()
{
	return ProcessInitialized();
}*/

std::string Transcoder::GetMimetype()
{
	return mimetype;
}

void Transcoder::SetInput(ExternalAVHandler* ext)
{
	inputPipe = ext->GetOutputFd();
	if(!inputPipe)
		throw std::runtime_error("Couldn't get AV Handler output");
}

int Transcoder::Read(char* buf, size_t len)
{
	std::lock_guard<std::mutex> guard(mutex);
	if(outputPipe <= 0)
		throw std::runtime_error("Transcoder not initialized: ");
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
		case EAGAIN:
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
			throw std::system_error(err, strerror(err));
		break;
	}
}

