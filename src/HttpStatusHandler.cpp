#include <iostream>
#include <exception>
#include <cstdlib>
#include <upnp/upnp.h>
#include "HttpStatusHandler.h"
#include "HttpRequest.h"
#include "util.h"
using namespace upnp_live;

HttpStatusOptions::HttpStatusOptions(std::string argstr)
{
	std::vector<std::string> args {util::SplitArgString(argstr, ' ')};
	if(args.size() == 1)
		url = args[0];
	else
	{
		for(int i = 0; i < args.size(); ++i)
		{
			if(args[i] == "--header")
				headers.push_back(args[++i]);
			else if(args[i] == "--success")
				success_codes = args[++i];
			else if(args[i] == "--no-redirect")
				follow_redirect = false;
			else if(args[i] == "--method")
				method = args[++i];
			else
				url = args[i];
		}
	}
}

HttpStatusHandler::HttpStatusHandler(HttpStatusOptions& o) : url(o.url), timeout(o.timeout), method(o.method), follow_redirect(o.follow_redirect)
{
	if(!url.size())
		throw std::invalid_argument("No URL");
	if(url.find("http://") == std::string::npos)
		url = "http://" + url;
	if(url.rfind("/") <= 8)
		url += "/";
	if(timeout <= 0)
		throw std::invalid_argument("Timeout required");
	if(o.headers.size())
		headers.swap(o.headers);
	SetSuccessCodes(o.success_codes);
}

void HttpStatusHandler::SetSuccessCodes(std::string new_codes)
{
	auto codes = util::SplitString(new_codes, ',');
	success_codes.swap(codes);
}

bool HttpStatusHandler::GetStatus()
{
	//Some servers put cookie-less requests in an infinite redirect/refresh loop
	//so put a hard cap at 10 redirects
	char redirect_max{10}, redirect{-1};
	std::string reqUrl = url;

	while(++redirect < redirect_max)
	{
		HttpRequest req(method, reqUrl, timeout);

		if(headers.size())
			req.AddHeaders(headers);

		try
		{
			req.Execute();
		}
		catch(std::system_error& e)
		{
			std::cout << "Error getting status: " << e.what() << "\n";
			return false;
		}

		bool red = req.IsResponseRedirect();
		if(red)
		{
			std::cout << "Got redirect to " << req.GetRedirectLocation() << "\n";
			if(follow_redirect)
			{
				reqUrl = req.GetRedirectLocation();
				if(reqUrl.size())
					continue;
			}
		}

		bool success = IsSuccessfulRequest(req);
		std::cout << "Successful request? " << std::boolalpha << success << "\n";
		return success;
	}

	return false;
}

bool HttpStatusHandler::IsSuccessfulRequest(HttpRequest& request)
{
	std::cout << "IsSuccessfulRequest()\n";
	int status_code = request.GetResponseStatus();
	if(!status_code)
		return false;
	for(auto& target : success_codes)
	{
		std::cout << "case: " << target << " ";
		size_t dash = target.find('-');
		if(dash == std::string::npos)
		{
			if(stoi(target) == status_code)
			{
				std::cout << "match\n";
				return true;
			}
		}
		else
		{
			int first = stoi(target.substr(0, dash));
			int second = stoi(target.substr(dash+1));
			if(status_code >= first && status_code <= second)
			{
				std::cout << "match\n";
				return true;
			}
		}
	}
	std::cout << "no match\n";
	return false;
}

void HttpStatusHandler::AddHeaders(std::vector<std::string>& newHeaders)
{
	for(auto header : newHeaders)
	{
		headers.push_back(header);
	}
}
