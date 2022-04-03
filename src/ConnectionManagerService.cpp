#include "ConnectionManagerService.h"
#include <iostream>
#include <sstream>
using namespace upnp_live;

ConnectionManagerService::ConnectionManagerService()
{
	logger = Logger::GetLogger();
}

void ConnectionManagerService::ExecuteAction(UpnpActionRequest* request)
{
	std::string ActionName = UpnpString_get_String(UpnpActionRequest_get_ActionName(request));
	DOMString requestXML = ixmlDocumenttoString(UpnpActionRequest_get_ActionRequest(request));
	if(requestXML == nullptr)
	{
		logger->Log_fmt(error, "[ConnectionManagerService] error converting action %s to string\n", ActionName.c_str());
		return;
	}
	else
	{
		logger->Log_fmt(debug, "[ConnectionManagerService] recieved action: %s\n%s\n", ActionName.c_str(), requestXML);
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
		logger->Log_fmt(error, "Unknown action request: %s\n", ActionName.c_str());
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
		logger->Log_fmt(error, "[ConnectionManagerService] Error %d converting response XML into document\n%s\n", ret, responseXML.c_str());
	else
	{
		UpnpActionRequest_set_ActionResult(request, response);
		DOMString finishedXML = ixmlDocumenttoString(response);
		logger->Log_fmt(debug, "Finished response:\n%s\n", finishedXML);
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
			"http-get:*:audio/ogg:*," \
			"http-get:*:audio/mpeg:*," \
			"http-get:*:audio/flac:*," \
			"http-get:*:audio/aac:*," \
			"http-get:*:audio/opus:*," \
			"http-get:*:audio/wav:*," \
			"http-get:*:audio/webm:*," \
			"http-get:*:video/x-matroska:*," \
			"http-get:*:video/mp4:*," \
			"http-get:*:video/webm:*," \
			"http-get:*:video/x-msvideo:*," \
			"http-get:*:video/ogg:*," \
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
