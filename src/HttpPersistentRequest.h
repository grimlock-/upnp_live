#ifndef HTTPPERSISTENTREQUEST_H 
#define HTTPPERSISTENTREQUEST_H

#include <string>
#include <vector>

namespace upnp_live {

class HttpPersistentRequest
{
	public:
		HttpPersistentRequest(std::string method, std::string url, int timeout);
		~HttpPersistentRequest();
		void AddHeader(std::string);
		void AddHeaders(std::vector<std::string> newHeaders);
		void Open();
		void Close();
	private:
		std::string url;
		int timeout, status{0};
		std::vector<std::string> headers;
		bool connectionOpen{false};
		void* connHandle{nullptr};
};

}

#endif /* HTTPPERSISTENTREQUEST_H */
