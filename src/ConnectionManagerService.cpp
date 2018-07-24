#include "ConnectionManagerService.h"
#include "Exceptions.h"
#include <iostream>
#include <sstream>
using namespace upnp_live;

ConnectionManagerService::ConnectionManagerService()
{
}

ConnectionManagerService::~ConnectionManagerService()
{
}

void ConnectionManagerService::executeAction(UpnpActionRequest* request)
{
	std::string ActionName = UpnpString_get_String(UpnpActionRequest_get_ActionName(request));
	DOMString requestXML = ixmlDocumenttoString(UpnpActionRequest_get_ActionRequest(request));
	if(requestXML == nullptr)
	{
		std::cerr << "[ConnectionManagerService] error converting action to string\n";
		return;
	}
	else
	{
		std::cout << "[ConnectionManagerService] recieved action: " << ActionName << "\n" << requestXML << std::endl;
		ixmlFreeDOMString(requestXML);
	}

	std::string responseXML;
	IXML_Document* response;
	if(ActionName == "GetProtocolInfo")
	{
		responseXML = getProtocolInfo();
	}
	else if(ActionName == "GetCurrentConnectionIDs")
	{
		responseXML = getCurrentConnectionIDs();
	}
	else if(ActionName == "GetCurrentConnectionInfo")
	{
		responseXML = getCurrentConnectionInfo();
	}
	else if(ActionName == "GetFeatureList")
	{
		responseXML = getFeatureList();
	}
	else
	{
		std::cout << "Unknown action request: " << ActionName << std::endl;
		UpnpActionRequest_set_ErrCode(request, 401);
		UpnpString* errorString = UpnpString_new();
		UpnpString_set_String(errorString, "Invalid Action");
		UpnpActionRequest_set_ErrStr(request, errorString);
		responseXML = errorXML(401, "Invalid Action");
		return;
	}
	if(responseXML == "")
		return;

	if(UpnpActionRequest_get_ErrCode(request) == 0)
	{
		std::stringstream ss;
		ss << "<u:" << ActionName << "Response xmlns:u=\"urn:schemas-upnp-org:service:ConnectionManager:1\">\n" << \
		responseXML << "</u:" << ActionName << "Response>\n";
		responseXML = ss.str();
	}

	int ret = ixmlParseBufferEx(responseXML.c_str(), &response);
	if(ret != IXML_SUCCESS)
		std::cout << "[ConnectionManagerService] Error " << ret << " converting response XML into document\n" << responseXML << std::endl;
	else
	{
		UpnpActionRequest_set_ActionResult(request, response);
		DOMString finishedXML = ixmlDocumenttoString(response);
		std::cout << "Finished response:\n" << finishedXML << std::endl;
		ixmlFreeDOMString(finishedXML);
	}
}


//GET functions
std::string ConnectionManagerService::getProtocolInfo()
{
	std::string response = \
		/*"<Source>http-get:*:video/mpeg:*</Source>\n"*/ \
		/*"<Source>" \
			"http-get:*:audio/mpeg:DLNA.ORG_PN=MP3;DLNA.ORG_OP=01;DLNA.ORG_FLAGS=81500000000000000000000000000000," \
			"http-get:*:image/jpeg:DLNA.ORG_PN=JPEG_SM;DLNA.ORG_OP=00;DLNA.ORG_FLAGS=00D00000000000000000000000000000," \
			"http-get:*:video/mpeg:DLNA.ORG_PN=MPEG_PS_PAL;DLNA.ORG_OP=11;DLNA.ORG_FLAGS=81500000000000000000000000000000," \
			"http-get:*:video/mp4:*," \
			"http-get:*:video/MP2T:*" \
		"</Source>"*/ \
		"<Source>" \
			"http-get:*:image/gif:*," \
			"http-get:*:image/jpeg:*," \
			"http-get:*:image/png:*," \
			"http-get:*:image/x-ms-bmp:*," \
			"http-get:*:video/mp4:*," \
			/*"http-get:*:video/mpeg:*,"*/ \
			"http-get:*:video/mpeg:DLNA.ORG_PN=MPEG_PS_PAL;DLNA.ORG_OP=11;DLNA.ORG_FLAGS=81500000000000000000000000000000," \
			"http-get:*:video/mpg:*," \
			"http-get:*:video/MP2T:*," \
			"http-get:*:application/x-mpegURL:*," \
			"http-get:*:application/vnd.apple.mpegurl" \
		"</Source>" \
		"<Sink></Sink>\n";
	return response;
}
std::string ConnectionManagerService::getCurrentConnectionIDs()
{
	std::string response = "<ConnectionIDs>0</ConnectionIDs>\n";
	return response;
}
std::string ConnectionManagerService::getCurrentConnectionInfo()
{
	std::string response = \
	"<RcsID>-1</RcsID>\n" \
	"<AVTransportID>-1</AVTransportID>\n" \
	"<ProtocolInfo>http-get:*:*:*</ProtocolInfo>\n" \
	"<PeerConnectionManager></PeerConnectionManager>\n" \
	"<PeerConnectionID>-1</PeerConnectionID>\n" \
	"<Direction>Output</Direction>\n" \
	"<Status>OK</Status>\n";
	return response;
}
std::string ConnectionManagerService::getFeatureList()
{
	std::string response = "<Features></Features>\n";
	return response;
}
std::string ConnectionManagerService::errorXML(int errorCode, const char* errorMessage)
{
	std::stringstream ss;
	ss << \
	"<s:Fault>\n" \
		"<faultcode>s:Client</faultcode>\n" \
		"<faultstring>UPnPError</faultstring>\n" \
		"<detail>\n" \
			"<UPnPError xmlns=\"urn:schemas-upnp-org:control-1-0\">\n" \
				"<errorCode>" << errorCode << "</errorCode>\n" \
				"<errorDescription>" << errorMessage << "</errorDescription>\n" \
			"</UPnPError>\n" \
		"</detail>\n" \
	"</s:Fault>\n";
	return ss.str();
}
