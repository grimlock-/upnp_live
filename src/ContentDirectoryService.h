#ifndef CONTENTDIRECTORYSERVICE_H
#define CONTENTDIRECTORYSERVICE_H

#include <string>
#include <shared_mutex>
#include <unordered_map>
#include <thread>
#include <atomic>
#include <upnp/upnp.h>
#include "Logging.h"
#include "Stream.h"

namespace upnp_live {

class ContentDirectoryService
{
	public:
		ContentDirectoryService();
		ContentDirectoryService(std::string name);
		~ContentDirectoryService();
		void Heartbeat();
		void ExecuteAction(UpnpActionRequest* request);
		bool AddStream(std::shared_ptr<Stream>& stream);
		void RemoveStream(std::string name);
		bool AddFile(std::string name, std::string mimetype, std::string path);
		void RemoveFile(std::string name);
		
		std::string GetItemXML(std::string objId);
	protected:
		struct CDResource
		{
			std::string name;
			std::string mime_type;
			bool video;
			std::string path;
		};
		//Vars
		std::string serverName;
		std::unordered_map<std::string, std::shared_ptr<Stream>> streams;
		std::shared_timed_mutex streamContainerMutex;
		std::unordered_map<std::string, CDResource> liveStreams;
		std::shared_timed_mutex liveContainerMutex;
		std::unordered_map<std::string, CDResource> files;
		std::shared_timed_mutex fileContainerMutex;
		Logger* logger;
		std::thread heartbeatThread;
		std::atomic<bool> heartbeatRunning {false};
		
		unsigned int nextId = 0;
		//Functions
		std::string browse(
			std::string objId,
			std::string browseFlag,
			std::string filter,
			int startingIndex,
			int requestedCount,
			std::string sortCriteria
		);
		std::string browseMetadata(
			std::string objectId,
			std::string filter,
			std::string sortCriteria
		);
		std::string browseDirectChildren(
			std::string objId,
			std::string filter,
			int startingIndex,
			int requestedCount,
			std::string sortCriteria
		);
		std::string GetBaseUrl();
		std::string errorXML(int, const char*);
		std::string streamContainerXML();
		std::string fileContainerXML();
		std::string getItemXML(CDResource item, std::string parent);
		void setStreamStatus(bool isLive);
};

}

#endif /* CONTENTDIRECTORYSERVICE_H */
