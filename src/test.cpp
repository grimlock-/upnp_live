#include <iostream>
#include <string>
#include <cstring>
#include <sys/time.h>
#include <sys/resource.h>
#include <upnp/upnp.h>
#include "HttpStatusHandler.h"

using namespace std;

bool request_use_get{false};
std::string URL;

void test_request()
{
	std::cout << "================================\n";
	std::cout << "HttpRequest()\n";
	string method{"HEAD"};
	if(request_use_get)
		method = "GET";
	upnp_live::HttpRequest req(method, URL, 15);

	req.Execute();

	cout << "\nResponse Headers:\n" << req.GetResponseHeaders() << "\nResponse body:\n" << req.GetResponseBody() << "\n";
}

void test_status()
{
	std::cout << "================================\n";
	std::cout << "HttpStatusHandler()\n";
	upnp_live::HttpStatusOptions opt{URL};
	if(request_use_get)
		opt.method = "GET";
	opt.follow_redirect = false;
	upnp_live::HttpStatusHandler handler(opt);
	handler.GetStatus();
}

int main(int argc, char* argv[])
{
	//Force enable core dumps
	struct rlimit core;
	core.rlim_cur = core.rlim_max = RLIM_INFINITY;
	setrlimit(RLIMIT_CORE, &core);

	if(argc == 1)
	{
		cout << "No URL\n";
		return 0;
	}

	for(int i = 1; i < argc; ++i)
	{
		if(strcmp(argv[i], "-g") == 0 || strcmp(argv[i], "--get") == 0)
			request_use_get = true;
	}

	string destination{argv[argc-1]};
	if(!destination.size())
	{
		cout << "Empty URL\n";
		return 0;
	}

	if(destination.find("http://") == string::npos && destination.find("https://") == string::npos || destination.rfind("/") <= destination.find(":")+2)
	{
		cout << "Bad URL\n";
		return 0;
	}

	URL = destination;
	std::cout << "url: " << URL << "\n";
	std::cout << "method: " << (request_use_get ? "GET" : "HEAD") << "\n";
	test_request();
	test_status();

	return 0;
}

