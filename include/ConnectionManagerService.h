#pragma once
#include <upnp/upnp.h>
#include <string>

namespace upnp_live {
class ConnectionManagerService
{
	public:
		ConnectionManagerService();
		~ConnectionManagerService();
		void executeAction(UpnpActionRequest*);
	private:
		std::string getProtocolInfo();
		std::string getCurrentConnectionIDs();
		std::string getCurrentConnectionInfo();
		std::string getFeatureList();
		std::string errorXML(int, const char*);
};
}
