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
#include "AVSource.h"
#include "AVStore.h"
#include "ContentDirectoryService.h"
#include "ConnectionManagerService.h"
#include "Stream.h"
#include "StatusHandler.h"
#include "Logging.h"

namespace upnp_live {

enum vdirs { api, resources };
enum vfilehandletype { avstore, fd };

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
		int IncomingEvent(Upnp_EventType eventType, const void* event, void* cookie);

		int GetPort();
		std::string GetAddress();
		std::string GetBaseResURI();
		std::string GetFriendlyName();

		int GetInfo(const char* filename, UpnpFileInfo* info, const void* cookie);
		UpnpWebFileHandle Open(const char* filename, enum UpnpOpenFileMode mode, const void* cookie);
		int Close(UpnpWebFileHandle hnd, const void* cookie);
		int Read(UpnpWebFileHandle hnd, char* buf, size_t len, const void* cookie);
		int Seek(UpnpWebFileHandle hnd, long offset, int origin, const void* cookie);
		int Write(UpnpWebFileHandle hnd, char* buf, size_t len, const void* cookie);
		//static int CallbackWrapper(Upnp_EventType, const void*, void*);
	
	protected:
		struct VirtualFileHandle
		{
			vfilehandletype type;
			std::string stream_name;
			int fd {0};
			UpnpWebFileHandle data {0};
		};
	
		//Vars
		std::unordered_map<std::string, std::shared_ptr<Stream>> streams;
		std::mutex streamContainerMutex;
		std::multimap<std::string, VirtualFileHandle> handles;
		std::mutex handleMutex;
		int broadcastExpiration {0}; //0 makes lib default to 30 second advertisements
		std::string webRoot;
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
		//UpnpWebFileHandle = void* typedef. It's a handle for our end to use, not the lib
		void execActionRequest(UpnpActionRequest*);
		void execStateVarRequest(UpnpStateVarRequest*);
		void execSubscriptionRequest(UpnpSubscriptionRequest*);
		
		int createPlainFileHandle(Stream& stream);
		std::unique_ptr<AVSource> createAVHandler(std::string& handlerType, std::string& argstr);
		std::unique_ptr<StatusHandler> createStatusHandler(std::string& handlerType, std::string& argstr);
};

}

#endif /* SERVER_H */
