#pragma once
#include <upnp/upnp.h>
#include "Stream.h"

namespace upnp_live {

class Server;

class ContentDirectoryService
{
	public:
		ContentDirectoryService(Server*);
		~ContentDirectoryService();
		void executeAction(UpnpActionRequest*);
		void addStream(const char*);
		void addStreams(const std::vector<std::string> &);
		void removeStream(Stream&);
		int getStreamCount();
		Stream* getStream(unsigned int);
		Stream* getStreamByURL(std::string);
	private:
		//Vars
		std::vector<Stream> streams;
		Server* server;
		//Server serverInstance;
		//Functions
		//Browse(objectID, browseFlag, filter, startingIndex, requestedCount, sortCriteria)
		std::string browse(std::string, std::string, std::string, int, int, std::string);
		std::string getSearchCapabilities();
		std::string getSortCapabilities();
		std::string getFeatureList();
		std::string getSystemUpdateID();
		std::string getServiceResetToken();
		std::string errorXML(int, const char*);
};

}
