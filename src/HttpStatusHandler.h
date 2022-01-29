#ifndef HTTPSTATUSHANDLER_H
#define HTTPSTATUSHANDLER_H

#include <string>
#include <vector>
#include "StatusHandler.h"
#include "HttpRequest.h"
namespace upnp_live {

struct HttpStatusOptions
{
	HttpStatusOptions(std::string);
	std::string url;
	std::string method{"HEAD"};
	int timeout{15};
	bool follow_redirect{false};
	std::vector<std::string> headers;
	std::string success_codes{"200-299"};
};

class HttpStatusHandler : public StatusHandler
{
	public:
		HttpStatusHandler(HttpStatusOptions&);
		void AddHeaders(std::vector<std::string>& newHeaders);
		bool IsSuccessfulRequest(HttpRequest& request);
		void SetSuccessCodes(std::string success_codes);
		//StatusHandler
		bool GetStatus();
	private:
		std::string url, method;
		bool follow_redirect;
		std::vector<std::string> success_codes;
		const int timeout;
		std::vector<std::string> headers;
};

}
#endif /* HTTPSTATUSHANDLER_H */
