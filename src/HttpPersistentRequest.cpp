#include "HttpPersistentRequest.h"
#include "Version.h"

using namespace upnp_live;

HttpPersistentRequest::HttpPersistentRequest(std::string method, std::string Url, int t) : url(Url), timeout(t)
{
	std::string s{"User-Agent: upnp_live/"};
	s += upnp_live_version;
	s += "\r\n";
	headers.push_back(s);
}

HttpPersistentRequest::~HttpPersistentRequest()
{
	if(connectionOpen)
		Close();
}

void HttpPersistentRequest::AddHeader(std::string header)
{
}

void HttpPersistentRequest::AddHeaders(std::string newHeaders)
{
}
