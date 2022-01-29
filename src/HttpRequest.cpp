/*
 * A single HTTP request made using pupnp client API. 
 * Might switch to libcurl and keep pupnp as a backup option.
 * Because of limitations with pupnp, response headers can't
 * be processed and redirects can't be followed
 */

#include <iostream>
#include <exception>
#include <stdexcept>
#include <upnp/upnptools.h>
#include "HttpRequest.h"
#include "Version.h"

using namespace upnp_live;

HttpRequest::HttpRequest(std::string method, std::string Url, int t) : url(Url), timeout(t)
{
	AddHeader("Connection: close");
	std::string s{"User-Agent: upnp_live/"};
	s += upnp_live_version;
	s += " pupnp/";
	s += UPNP_VERSION_STRING;
	AddHeader(s);

	if(method == "PUT")
		httpMethod = UPNP_HTTPMETHOD_PUT;
	else if(method == "DELETE")
		httpMethod = UPNP_HTTPMETHOD_DELETE;
	else if(method == "GET")
		httpMethod = UPNP_HTTPMETHOD_GET;
	else if(method == "HEAD")
		httpMethod = UPNP_HTTPMETHOD_HEAD;
	else if(method == "POST")
		httpMethod = UPNP_HTTPMETHOD_POST;
	else
		throw std::invalid_argument("Invalid HTTP method " + method);

	SetResponseBufferSize(4096);
}

HttpRequest::~HttpRequest()
{
	if(connectionOpen)
	{
		UpnpCancelHttpGet(libHandle);
		CloseConnection();
	}
}

void HttpRequest::AddHeader(const char* header)
{
	headers.emplace_back(header);
	std::string& newH = headers.back();
	if(newH.substr(newH.size()-2, 2) != "\r\n")
		newH += "\r\n";
}
void HttpRequest::AddHeader(std::string header)
{
	if(header.substr(header.size()-2, 2) != "\r\n")
		header += "\r\n";
	headers.push_back(header);
}

void HttpRequest::AddHeaders(std::vector<std::string>& newHeaders)
{
	for(auto& header : newHeaders)
	{
		AddHeader(header);
	}
}

void HttpRequest::Execute()
{
	if(this->requestMade)
		throw std::runtime_error("Request already made");

	if(!OpenConnection())
		return;

	//Make and end HTTP request
	std::string allHeaders;
	bool hasHost{false};
	for(auto& str : headers)
	{
		allHeaders += str;
		if(str.find("HOST:") != std::string::npos)
			hasHost = true;
	}
	if(!hasHost)
	{
		std::string hostHeader = "HOST: ";
		int second = url.find("/", url.find("/")+1)+1;
		hostHeader += url.substr(second, url.rfind("/")-second);
		hostHeader += "\r\n";
		allHeaders += hostHeader;
	}
	allHeaders += "\r\n";
	UpnpString* header_string = UpnpString_new();
	UpnpString_set_String(header_string, allHeaders.c_str());
	std::cout << "Making HTTP request: " << url << "\nHeaders:\n" << UpnpString_get_String(header_string) << "\n";
	int result = UpnpMakeHttpRequest(httpMethod, url.c_str(), libHandle, header_string, nullptr, 0, timeout);
	if(result != UPNP_E_SUCCESS)
	{
		UpnpString_delete(header_string);
		CloseConnection();
		std::cout << "MakeHttpRequest(): " << UpnpGetErrorMessage(result) << "\n";
		return;
	}
	UpnpString_delete(header_string);
	result = UpnpEndHttpRequest(libHandle, timeout);
	if(result != UPNP_E_SUCCESS)
	{
		CloseConnection();
		std::cout << "EndHttpRequest(): " << UpnpGetErrorMessage(result) << "\n";
		return;
	}
	std::cout << "Request complete\n\n";

	//Get response info
	char* contentType;
	int responseLength;
	UpnpString* respHeaders = UpnpString_new();
	result = UpnpGetHttpResponse(libHandle, respHeaders, &contentType, &responseLength, &responseStatus, timeout);
	if(result != UPNP_E_SUCCESS)
	{
		responseType = "";
		UpnpString_delete(respHeaders);
		std::cout << "GetHttpResponse(" << url << "): " << UpnpGetErrorMessage(result) << "\n";
		return;
	}
	responseHeaders = UpnpString_get_String(respHeaders);
	UpnpString_delete(respHeaders);
	if(contentType)
		responseType = contentType;
	else
		responseType = "NULL";
	std::cout << "status code: " << responseStatus << "\n";
	std::cout << "content type: " << responseType << "\n";
	if(responseLength == UPNP_UNTIL_CLOSE)
		std::cout << "content length: Until conn close\n";
	else
		std::cout << "content length: " << responseLength << "\n";

	//Get response
	if(responseLength > 0 && responseBufferSize)
	{
		size_t sz = responseBufferSize;
		char buffer[responseBufferSize];
		result = UpnpReadHttpResponse(libHandle, buffer, &sz, timeout);
		if(result != UPNP_E_SUCCESS)
		{
			std::cout << "ReadHttpResponse(" << url << "): " << result << "\n";
			UpnpCloseHttpConnection(libHandle);
			return;
		}
		if(sz)
			responseBody = buffer;
	}

	CloseConnection();
}
bool HttpRequest::OpenConnection()
{
	requestMade = true;
	connectionOpen = true;
	int result = UpnpOpenHttpConnection(url.c_str(), &libHandle, timeout);
	if(result != UPNP_E_SUCCESS)
	{
		connectionOpen = false;
		std::cout << "OpenHttpConnection (" << url << "): " << UpnpGetErrorMessage(result) << "\n";
		return false;
	}
	return true;
}
void HttpRequest::CloseConnection()
{
	if(!connectionOpen)
		return;
	int result = UpnpCloseHttpConnection(libHandle);
	if(result != UPNP_E_SUCCESS)
	{
		std::cout << "CloseHttpConnection(" << url << "): " << UpnpGetErrorMessage(result) << "\n";
		return;
	}
	connectionOpen = false;
}

bool HttpRequest::IsResponseRedirect()
{
	if(!requestMade)
		return false;
	if(responseStatus >= 300 && responseStatus <= 399)
		return true;
	if((responseType == "text/html" || responseType == "application/xhtml+xml") && responseBody.size())
	{
		if(responseBody.find("http-equiv=\"REFRESH\"") != std::string::npos || responseBody.find("http-equiv='REFRESH'") != std::string::npos)
			return true;
		if(responseBody.find("http-equiv=\"LOCATION\"") != std::string::npos || responseBody.find("http-equiv='LOCATION'") != std::string::npos)
			return true;
		if(responseBody.find("http-equiv=\"CONTENT-LOCATION\"") != std::string::npos || responseBody.find("http-equiv='CONTENT-LOCATION'") != std::string::npos)
			return true;
	}
	return false;
}
std::string HttpRequest::GetRedirectLocation()
{
	std::string ret;
	if(responseBody.size())
	{
		size_t pos;
		//FIXME - redo if a regex lib is ever added
		pos = responseBody.find("http-equiv=\"REFRESH\"");
		if(pos == std::string::npos)
			pos = responseBody.find("http-equiv=\"Refresh\"");
		if(pos == std::string::npos)
			pos = responseBody.find("http-equiv='REFRESH'");
		if(pos == std::string::npos)
			pos = responseBody.find("http-equiv='Refresh'");
		if(pos == std::string::npos)
			pos = responseBody.find("http-equiv=\"LOCATION\"");
		if(pos == std::string::npos)
			pos = responseBody.find("http-equiv=\"Location\"");
		if(pos == std::string::npos)
			pos = responseBody.find("http-equiv='LOCATION'");
		if(pos == std::string::npos)
			pos = responseBody.find("http-equiv='Location'");
		if(pos == std::string::npos)
			pos = responseBody.find("http-equiv=\"CONTENT-LOCATION\"");
		if(pos == std::string::npos)
			pos = responseBody.find("http-equiv=\"Content-Location\"");
		if(pos == std::string::npos)
			pos = responseBody.find("http-equiv='CONTENT-LOCATION'");
		if(pos == std::string::npos)
			pos = responseBody.find("http-equiv='Content-Location'");
		if(pos == std::string::npos)
			return ret;

		size_t pos2 = responseBody.find("url=", pos);
		if(pos2 == std::string::npos)
			return ret;
		size_t sq = responseBody.find('\'', pos2);
		size_t dq = responseBody.find('"', pos2);
		size_t sc = responseBody.find(';', pos2);
		size_t t = 0;
		if(sq == std::string::npos)
			t = (dq < sc) ? dq : sc;
		else if(dq == std::string::npos)
			t = (sq < sc) ? sq : sc;
		else if(sc == std::string::npos)
			t = (sq < dq) ? sq : dq;
		else
		{
			t = (sq < sc) ? sq : sc;
			t = (t < dq) ? t : dq;
		}
		ret = responseBody.substr(pos2+4, t-(pos2+4));
	}
	return ret;
}

void HttpRequest::SetResponseBufferSize(int size)
{
	responseBufferSize = size;
}

int HttpRequest::GetResponseStatus()
{
	return responseStatus;
}

std::string HttpRequest::GetResponseHeaders()
{
	return responseHeaders;
}

std::string HttpRequest::GetResponseBody()
{
	return responseBody;
}
