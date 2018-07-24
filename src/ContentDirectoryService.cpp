#include <iostream>
#include <sstream>
#include <string>
#include <assert.h>
#include <strings.h> //strncasecmp
#include "ContentDirectoryService.h"
#include "Server.h"
#include "Exceptions.h"
#include "util.h"
using namespace upnp_live;

ContentDirectoryService::ContentDirectoryService(Server* srv) : server(srv)
{
	/*Stream s1("twitch.tv/meleeeveryday"), s2("twitch.tv/monstercat");
	streams.push_back(s1);
	streams.push_back(s2);
	streams[0].setID(1);
	streams[1].setID(2);

	std::stringstream baseURI;
	baseURI << "http://" << server->getAddress() << ":" << server->getPort() << "/res/";
	streams[0].setBaseResURI(baseURI.str());
	streams[1].setBaseResURI(baseURI.str());*/
	/*addStream("twitch.tv/sakgamingtv");
	addStream("twitch.tv/monstercat");*/
}

ContentDirectoryService::~ContentDirectoryService()
{
	streams.clear();
}




void ContentDirectoryService::addStream(const char* url)
{
	Stream newStream(url);
	std::stringstream baseURI;
	baseURI << "http://" << server->getAddress() << ":" << server->getPort() << "/res/";
	newStream.setID(streams.size()+1);
	newStream.setBaseResURI(baseURI.str());
	streams.push_back(newStream);
}
void ContentDirectoryService::addStreams(const std::vector<std::string> &urls)
{
	for(unsigned int i = 0; i < urls.size(); i++)
		addStream(urls[i].c_str());
}
void ContentDirectoryService::executeAction(UpnpActionRequest* request)
{
	std::string ActionName = UpnpString_get_String(UpnpActionRequest_get_ActionName(request));
	DOMString requestXML = ixmlDocumenttoString(UpnpActionRequest_get_ActionRequest(request));
	if(requestXML == nullptr)
	{
		std::cerr << "[ContentDirectoryService] Error converting action to string\n";
		return;
	}
	else
	{
		std::cout << "[ContentDirectoryService] Recieved action: " << ActionName << "\n" << requestXML << std::endl;
		ixmlFreeDOMString(requestXML);
	}

	std::string responseXML;
	IXML_Document* response;
	if(ActionName == "GetSearchCapabilities")
	{
		responseXML = getSearchCapabilities();
	}
	else if(ActionName == "GetSortCapabilities")
	{
		responseXML = getSortCapabilities();
	}
	else if(ActionName == "GetFeatureList")
	{
		responseXML = getFeatureList();
	}
	else if(ActionName == "GetSystemUpdateID")
	{
		responseXML = getSystemUpdateID();
	}
	else if(ActionName == "GetServiceResetToken")
	{
		responseXML = getServiceResetToken();
	}
	else if(ActionName == "Browse")
	{
		IXML_NodeList* nodes;
		IXML_Node* node;
		std::string objID, browseFlag, filter, sortCriteria;
		int startingIndex, requestedCount;
		nodes = ixmlDocument_getElementsByTagName(UpnpActionRequest_get_ActionRequest(request), "ObjectID");
		if(nodes == nullptr || nodes->nodeItem == nullptr || nodes->nodeItem->firstChild == nullptr)
		{
			std::cout << "[ContentDirectoryService] Could not find ObjectID" << std::endl;
			return;
		}
		node = nodes->nodeItem->firstChild;
		objID = ixmlNode_getNodeValue(node);

		nodes = ixmlDocument_getElementsByTagName(UpnpActionRequest_get_ActionRequest(request), "BrowseFlag");
		if(nodes == nullptr || nodes->nodeItem == nullptr || nodes->nodeItem->firstChild == nullptr)
		{
			std::cout << "[ContentDirectoryService] Could not find BrowseFlag" << std::endl;
			return;
		}
		node = nodes->nodeItem->firstChild;
		browseFlag = ixmlNode_getNodeValue(node);

		nodes = ixmlDocument_getElementsByTagName(UpnpActionRequest_get_ActionRequest(request), "Filter");
		if(nodes == nullptr || nodes->nodeItem == nullptr || nodes->nodeItem->firstChild == nullptr)
		{
			//std::cout << "[ContentDirectoryService] Could not find Filter" << std::endl;
			//return;
			filter = "";
		}
		else
		{
			node = nodes->nodeItem->firstChild;
			filter = ixmlNode_getNodeValue(node);
		}

		nodes = ixmlDocument_getElementsByTagName(UpnpActionRequest_get_ActionRequest(request), "StartingIndex");
		if(nodes == nullptr || nodes->nodeItem == nullptr || nodes->nodeItem->firstChild == nullptr)
		{
			std::cout << "[ContentDirectoryService] Could not find StartingIndex" << std::endl;
			return;
		}
		node = nodes->nodeItem->firstChild;
		startingIndex = std::stoi(  ixmlNode_getNodeValue(node)  );

		nodes = ixmlDocument_getElementsByTagName(UpnpActionRequest_get_ActionRequest(request), "RequestedCount");
		if(nodes == nullptr || nodes->nodeItem == nullptr || nodes->nodeItem->firstChild == nullptr)
		{
			std::cout << "[ContentDirectoryService] Could not find RequestedCount" << std::endl;
			return;
		}
		node = nodes->nodeItem->firstChild;
		requestedCount = std::stoi(  ixmlNode_getNodeValue(node)  );

		nodes = ixmlDocument_getElementsByTagName(UpnpActionRequest_get_ActionRequest(request), "SortCriteria");
		if(nodes == nullptr || nodes->nodeItem == nullptr || nodes->nodeItem->firstChild == nullptr)
		{
			sortCriteria = "";
		}
		else
		{
			node = nodes->nodeItem->firstChild;
			sortCriteria = ixmlNode_getNodeValue(node);
		}

		try
		{
			responseXML = browse(objID, browseFlag, filter, startingIndex, requestedCount, sortCriteria);
		}
		catch(ActionException e)
		{
			std::cout << "Caught exception handling action request: " << ActionName << std::endl;
			UpnpActionRequest_set_ErrCode(request, e.getErrorCode());
			UpnpString* errorString = UpnpString_new();
			UpnpString_set_String(errorString, e.getMessage().c_str());
			UpnpActionRequest_set_ErrStr(request, errorString);
			std::cout << "Error code: " << e.getErrorCode() << "\nError message: " << e.getMessage() << std::endl;
		}
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
		ss << "<u:" << ActionName << "Response xmlns:u=\"urn:schemas-upnp-org:service:ContentDirectory:1\">\n" << \
		responseXML << "</u:" << ActionName << "Response>\n";
		responseXML = ss.str();
	}

	int ret = ixmlParseBufferEx(responseXML.c_str(), &response);
	if(ret != IXML_SUCCESS)
		std::cout << "[ContentDirectoryService] Error " << ret << " converting response XML into document\n" << responseXML << std::endl;
	else
	{
		UpnpActionRequest_set_ActionResult(request, response);
		DOMString finishedXML = ixmlDocumenttoString(response);
		std::cout << "Finished response:\n" << finishedXML << std::endl;
		ixmlFreeDOMString(finishedXML);
	}
}

std::string ContentDirectoryService::browse(std::string objID, std::string browseFlag, std::string filter, int startingIndex, int requestedCount, std::string sortCriteria)
{
	int numReturned = 0;
	int totalMatches = 0;
	std::string resultBody = "";

	if(browseFlag == "BrowseMetadata")
	{
		//Required by standard
		startingIndex = 0;
		numReturned = 1;
		totalMatches = 1;

		if(objID.find("-") == std::string::npos)
		{
			int id = stoi(objID);
			switch(id)
			{
				case 0:
					resultBody = \
					"<container id=\"0\" parentID=\"-1\" childCount=\"NUM\" restricted=\"1\" searchable=\"0\">\n" \
						"<dc:title>NAME</dc:title>\n" \
						"<upnp:class>object.container.storageFolder</upnp:class>\n" \
					"</container>\n";
					resultBody.replace(resultBody.find("NUM"), 3, std::to_string(streams.size()));
					resultBody.replace(resultBody.find("NAME"), 4, server->getFriendlyName());
					break;
				default:
					Stream* stream = getStream(id-1);
					if(stream != nullptr)
						resultBody = stream->getMetadataXML();
					break;
			}
		}
		else
		{
			int streamID = stoi(objID.substr(0, objID.find_first_of("-")));
			int childID = stoi(objID.substr(objID.find_first_of("-")+1));
			Stream* stream = getStream(streamID-1);
			if(stream != nullptr)
				resultBody = stream->getChildXML(childID-1);
		}
		/*else if(objID == "1")
		{
			resultBody = \
			"<container id=\"1\" parentID=\"0\" childCount=\"1\" restricted=\"1\" searchable=\"0\">\n" \
				"<dc:title>twitch.tv/sakgamingtv</dc:title>\n" \
				"<upnp:class>object.container.storageFolder</upnp:class>\n" \
			"</container>\n";
		}
		else if(objID == "2")
		{
			resultBody = \
			"<container id=\"2\" parentID=\"0\" childCount=\"1\" restricted=\"1\" searchable=\"0\">\n" \
				"<dc:title>twitch.tv/monstercat</dc:title>\n" \
				"<upnp:class>object.container.storageFolder</upnp:class>\n" \
			"</container>\n";
		}
		else if(objID.find("1-") != std::string::npos)
		{
			int child = std::stoi(objID.substr(objID.find("1-")+2));
			resultBody = streams[0].getChildXML(child-1);
		}
		else if(objID.find("2-") != std::string::npos)
		{
			int child = std::stoi(objID.substr(objID.find("2-")+2));
			resultBody = streams[1].getChildXML(child-1);
		}
		else
		{
			std::string s = "Invalid ObjectID: ";
			s += objID;
			s += "\n";
			throw ActionException(701, s.c_str());
		}*/
	}
	else if(browseFlag == "BrowseDirectChildren")
	{
		if(objID == "0")
		{
			int itemsLeft = requestedCount;
			for(unsigned int i = startingIndex; i < streams.size() && itemsLeft > 0; i++,itemsLeft--,numReturned++)
			{
				resultBody += getStream(i)->getMetadataXML();
			}
			totalMatches = streams.size();
		}
		else
		{
			Stream* stream = getStream(std::stoi(objID)-1);
			if(stream == nullptr)
			{
				std::string s = "Invalid ObjectID: ";
				s += objID;
				s += "\n";
				throw ActionException(701, s.c_str());
			}
			totalMatches = stream->getChildCount();
			int itemsLeft = requestedCount;
			for(int i = startingIndex; i < stream->getChildCount() && itemsLeft > 0; i++,itemsLeft--,numReturned++)
			{
				resultBody += stream->getChildXML(i);
			}
		}


		/*switch(std::stoi(objID))
		{
			case 0:
				if(startingIndex == 0)
				{
					resultBody = \
					"<container id=\"1\" parentID=\"0\" childCount=\"5\" restricted=\"1\">\n" \
						"<dc:title>twitch.tv/sakgamingtv</dc:title>\n" \
						"<upnp:class>object.container.storageFolder</upnp:class>\n" \
					"</container>\n";
					numReturned = 1;
					if(requestedCount != 1)
					{
						resultBody += \
						"<container id=\"2\" parentID=\"0\" childCount=\"5\" restricted=\"1\">\n" \
							"<dc:title>twitch.tv/monstercat</dc:title>\n" \
							"<upnp:class>object.container.storageFolder</upnp:class>\n" \
						"</container>\n";
						numReturned++;
					}
				}
				else if(startingIndex == 1)
				{
					resultBody = \
					"<container id=\"2\" parentID=\"0\" childCount=\"5\" restricted=\"1\">\n" \
						"<dc:title>twitch.tv/monstercat</dc:title>\n" \
						"<upnp:class>object.container.storageFolder</upnp:class>\n" \
					"</container>\n";
					numReturned = 1;
				}
				else
				{
					resultBody = "";
					numReturned = 0;
				}
				totalMatches = 2;
			break;

			case 1:
				resultBody = streams[0].getChildrenXML(requestedCount, startingIndex, numReturned);
				if(strncasecmp(resultBody.c_str(), "error", 5) == 0) throw ActionException(600, resultBody.c_str());
				totalMatches = streams[0].getChildCount();
			break;

			case 2:
				resultBody = streams[1].getChildrenXML(requestedCount, startingIndex, numReturned);
				if(strncasecmp(resultBody.c_str(), "error", 5) == 0) throw ActionException(600, resultBody.c_str());
				totalMatches = streams[1].getChildCount();
			break;

			default:
				std::string s = "Invalid ObjectID: ";
				s += objID;
				s += "\n";
				throw ActionException(701, s.c_str());
			break;
		}*/
	}
	resultBody.insert(0, "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\" xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">\n");
	resultBody += "</DIDL-Lite>\n";

	std::stringstream ss;
	ss << \
	"<Result>\n" << \
	util::EscapeXML(resultBody) << \
	"</Result>\n" \
	"<NumberReturned>" << numReturned << "</NumberReturned>\n" \
	"<TotalMatches>" << totalMatches << "</TotalMatches>\n" \
	"<UpdateID>0</UpdateID>\n";
	return ss.str();
}



//GET functions
std::string ContentDirectoryService::getSearchCapabilities()
{
	std::string s = "<SearchCaps></SearchCaps>\n";
	return s;
}
std::string ContentDirectoryService::getSortCapabilities()
{
	std::string s = "<SortCaps></SortCaps>\n";
	return s;
}
std::string ContentDirectoryService::getFeatureList()
{
	std::string s = "<Features></Features>\n";
	return s;
}
std::string ContentDirectoryService::getSystemUpdateID()
{
	std::string s = "<Id>0</Id>\n";
	return s;
}
std::string ContentDirectoryService::getServiceResetToken()
{
	std::string s = "<ResetToken>0</ResetToken>\n";
	return s;
}
//TODO - Do something to stop race conditions
int ContentDirectoryService::getStreamCount()
{
	return streams.size();
}
Stream* ContentDirectoryService::getStream(unsigned int index)
{
	if(index >= streams.size())
		return nullptr;
	else
		return &(streams[index]);
}
Stream* ContentDirectoryService::getStreamByURL(std::string url)
{
	std::vector<Stream>::iterator i = streams.begin();
	while(i != streams.end())
	{
		if(i->getURL() == url)
			return &(*i);
	}
	return nullptr;
}




std::string ContentDirectoryService::errorXML(int errorCode, const char* errorMessage)
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
