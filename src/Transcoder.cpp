#include "Transcoder.h"

using namespace upnp_live;

Transcoder::Transcoder(std::vector<std::string>& args) : ChildProcess(args)
{
	SourceType = transcoder;
}
Transcoder::~Transcoder()
{
	ShutdownProcess();
}

//IsInitialized() from ChildProcess

void Transcoder::SetSource(int fd)
{
	//I don't think the guard here is strictly necessary since it's only ever
	//called from Stream.cpp after init()ing an av handler (which itself is only
	//called from MemoryStore which has it's own guards),thereby ensuring only
	//one thread will ever be calling this function, but I'm sure I'll forget
	//this while making some change in the future, so better safe than sorry
	std::lock_guard<std::mutex> guard(mutex);
	inputPipe = fd;
}

int Transcoder::Init()
{
	return InitProcess().first;
}
void Transcoder::Shutdown()
{
	ShutdownProcess();
}
std::string Transcoder::GetMimetype()
{
	return "";
}
bool Transcoder::IsInitialized()
{
	return ProcessInitialized();
}
