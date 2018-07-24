#pragma once
#include <mupnp/mUPnP.h>
#include "Exceptions.h"

namespace upnp_live {

class ContentDirectoryService;
class ConnectionManagerService;

class RootDevice : public mUPnP::Device
{
	public:
		RootDevice(std::string);
		virtual ~RootDevice();
		//uHTTP::HTTP::StatusCode httpRequestRecieved(uHTTP::HTTPRequest*);
	private:
		ContentDirectoryService* contentDirectoryService;
		ConnectionManagerService* connectionManagerService;
		uHTTP::HTTP::StatusCode getRequestRecieved(uHTTP::HTTPRequest*);
		uHTTP::HTTP::StatusCode controlRequestRecieved(uHTTP::HTTPRequest*);
		uHTTP::HTTP::StatusCode badRequest(uHTTP::HTTPRequest*);
		const char* getDeviceDescription();
};

}
