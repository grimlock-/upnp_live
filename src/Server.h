#ifndef SERVER_H
#define SERVER_H

#include <vector>
#include <mutex>
#include <map>
#include <unordered_map>
#include <memory>
#include <utility>
#include <upnp/upnp.h>
#include "InitOptions.h"
#include "AVHandler.h"
#include "AVStore.h"
#include "ContentDirectoryService.h"
#include "ConnectionManagerService.h"
#include "Stream.h"
#include "StatusHandler.h"
#include "Logging.h"

namespace upnp_live {

enum vdirs { api, resources };

class Server
{
	public:
		Server(InitOptions&);
		virtual ~Server();
		void Shutdown();
		
		AddStreamResult AddStream(StreamInitOptions&);
		void AddStreams(std::vector<StreamInitOptions>& streams);
		void AddFile(FileOptions& options);
		void AddFiles(std::vector<FileOptions>& files);
		void RemoveFile(std::string filename);
		void RemoveFiles(std::vector<std::string>& files);
		int IncomingEvent(Upnp_EventType eventType, const void* event);

		int GetPort();
		std::string GetAddress();
		std::string GetBaseResURI();
		std::string GetFriendlyName();

		//UpnpWebFileHandle = void* typedef. It's a handle for our end to use, not the lib
		int GetInfo(const char* filename, UpnpFileInfo* info);
		UpnpWebFileHandle Open(const char* filename, enum UpnpOpenFileMode mode);
		int Close(UpnpWebFileHandle hnd);
		int Read(UpnpWebFileHandle hnd, char* buf, size_t len);
		int Seek(UpnpWebFileHandle hnd, long offset, int origin);
		int Write(UpnpWebFileHandle hnd, char* buf, size_t len);
	
	protected:
		struct VirtualFileHandle
		{
			std::string stream_name;
			//Opaque identifier used by AV Store
			UpnpWebFileHandle data {0};
		};
	
		//Vars
		std::unordered_map<std::string, std::shared_ptr<Stream>> streams;
		std::mutex streamContainerMutex;
		std::multimap<std::string, VirtualFileHandle> handles;
		std::mutex handleMutex;
		int broadcastExpiration {0}; //0 makes lib default to 30 second advertisements
		const std::string webRoot;
		UpnpDevice_Handle libHandle;
		IXML_Document* description;
		std::unique_ptr<ContentDirectoryService> cds;
		std::unique_ptr<ConnectionManagerService> cms;
		std::unique_ptr<AVStore> avStore;
		const vdirs API {api};
		const vdirs RES {resources};
		Logger* logger;
		std::atomic<bool> shuttingDown {false};
		
		//Functions
		void loadXml(InitOptions&);
		void execActionRequest(UpnpActionRequest*);
		void execStateVarRequest(UpnpStateVarRequest*);
		void execSubscriptionRequest(UpnpSubscriptionRequest*);
		
		int createPlainFileHandle(Stream& stream);
		std::unique_ptr<AVHandler> createAVHandler(std::string& handlerType, std::string& argstr);
		std::unique_ptr<StatusHandler> createStatusHandler(std::string& handlerType, std::string& argstr);
};

}

#endif
