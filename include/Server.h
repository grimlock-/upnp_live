#pragma once
#include <upnp/upnp.h>
#include <pthread.h>
#include <atomic>
#include "InitOptions.h"
#include "ContentDirectoryService.h"
#include "ConnectionManagerService.h"

namespace upnp_live {

class Server
{
	public:
		Server(const InitOptions&);
		~Server();

		int				getPort();
		std::string			getAddress();
		std::string			getBaseResURI();
		std::string			getFriendlyName();
		ContentDirectoryService*	getCDS();
		ConnectionManagerService*	getCMS();
		//static Server*	getInstance();
		static int	callbackWrapper(Upnp_EventType, const void*, void*);
	private:
		//Vars
		std::atomic_flag		shutting_down = ATOMIC_FLAG_INIT;
		int				broadcastExpiration = 150;	//100 apparently translates to 40 seconds, but 150 comes out to 45 seconds, and 65 makes it send advertisements every 2 seconds
		std::string			webRoot;
		UpnpDevice_Handle		libHandle;
		IXML_Document*			description;
		ContentDirectoryService*	cds;
		ConnectionManagerService*	cms;
		//Functions
		void incomingEvent(Upnp_EventType, void*);
		void execActionRequest(UpnpActionRequest*);
		void execStateVarRequest(UpnpStateVarRequest*);
		void execSubscriptionRequest(UpnpSubscriptionRequest*);
		void checkRegisterResult(int&);
};

}
