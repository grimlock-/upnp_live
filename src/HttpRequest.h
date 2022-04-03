#ifndef HTTPREQUEST_H 
#define HTTPREQUEST_H
/*
 * A synchronous HTTP request for a single resource using the
 * pupnp client API
 * TODO - Switch to libcurl or something and keep pupnp as a backup
 * Because of limitations with pupnp, response headers can't
 * be processed and most redirects can't be followed
 */
#include <string>
#include <vector>
#include <upnp/upnp.h>

namespace upnp_live {

class HttpRequest
{
	public:
		HttpRequest(std::string method, std::string url, int timeout);
		~HttpRequest();
		void Execute();
		void AddHeader(std::string);
		void AddHeader(const char*);
		void AddHeaders(std::vector<std::string>& newHeaders);
		void SetResponseBufferSize(int);
		int GetResponseStatus();
		std::string GetResponseHeaders();
		std::string GetResponseBody();
		bool IsResponseRedirect();
		std::string GetRedirectLocation();
	private:
		bool OpenConnection();
		void CloseConnection();
		const std::string url;
		const int timeout;

		Upnp_HttpMethod httpMethod;
		size_t responseBufferSize;
		int responseStatus{0};
		std::string responseBody, responseHeaders, responseType;
		std::vector<std::string> headers;
		bool requestMade{false}, connectionOpen{false};
		void* libHandle{nullptr};
};

}

#endif /* HTTPREQUEST_H */
