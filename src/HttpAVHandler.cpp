#include "HttpAVHandler.h"

using namespace upnp_live;

HttpAVHandler::HttpAVHandler() : SourceType{http}
{
}

int HttpAVHandler::Init()
{
	return 0;
}

void HttpAVHandler::Shutdown()
{
}

bool HttpAVHandler::IsInitialized()
{
	return false;
}

std::string HttpAVHandler::GetMimetype()
{
	return std::string("");
}
