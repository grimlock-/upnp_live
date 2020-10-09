#ifndef CONNECTIONMANAGERSERVICE_H
#define CONNECTIONMANAGERSERVICE_H
#include <string>
#include <upnp/upnp.h>
#include "Logging.h"

namespace upnp_live {

class ConnectionManagerService
{
	public:
		ConnectionManagerService();
		void ExecuteAction(UpnpActionRequest* request);
	private:
		std::string getProtocolInfo();
		std::string getCurrentConnectionIDs();
		std::string getCurrentConnectionInfo();
		std::string getFeatureList();
		std::string errorXML(int errorCode, const char* errorMessage);
		Logger* logger;
};

}
#endif /* CONTENTDIRECTORYSERVICE_H */
