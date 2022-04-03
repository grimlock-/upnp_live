#include <iostream>
#include <sstream>
#include <mutex>
#include <strings.h> //strncasecmp
#include "ContentDirectoryService.h"
#include "util.h"
using namespace upnp_live;

ContentDirectoryService::ContentDirectoryService(std::string name) : serverName(name)
{
	if(serverName.empty())
		serverName = "upnp_live";
	logger = Logger::GetLogger();

	heartbeatRunning = true;
	heartbeatThread = std::thread(&ContentDirectoryService::Heartbeat, this);
}
ContentDirectoryService::~ContentDirectoryService()
{
	heartbeatRunning = false;
	if(heartbeatThread.joinable())
		heartbeatThread.join();
}

bool ContentDirectoryService::AddStream(std::shared_ptr<Stream>& stream)
{
	std::unique_lock<std::shared_timed_mutex> lock1(streamContainerMutex, std::defer_lock);
	std::unique_lock<std::shared_timed_mutex> lock2(liveContainerMutex, std::defer_lock);
	std::lock(lock1, lock2);
	return streams.emplace(std::make_pair(stream->Name, stream)).second;
}
void ContentDirectoryService::RemoveStream(std::string name)
{
	std::unique_lock<std::shared_timed_mutex> lock1(streamContainerMutex, std::defer_lock);
	std::unique_lock<std::shared_timed_mutex> lock2(liveContainerMutex, std::defer_lock);
	std::lock(lock1, lock2);
	streams.erase(name);
	liveStreams.erase(name);
}
bool ContentDirectoryService::AddFile(std::string name, std::string mimetype, std::string path)
{
	if(files.find(name) != files.end())
		throw std::runtime_error("File with given id already exists");

	CDResource newItem;
	newItem.name = name;
	newItem.mime_type = mimetype;
	newItem.path = path;
	while(newItem.path.back() == '/')
		newItem.path.pop_back();
	if(strncasecmp(mimetype.c_str(), "video", 5) == 0)
		newItem.video = true;
	else
		newItem.video = false;

	std::unique_lock<std::shared_timed_mutex> lock(fileContainerMutex);
	return files.emplace(std::make_pair(name, newItem)).second;
}
void ContentDirectoryService::RemoveFile(std::string name)
{
	std::unique_lock<std::shared_timed_mutex> lock(fileContainerMutex);
	files.erase(name);
}




void ContentDirectoryService::Heartbeat()
{
	std::this_thread::sleep_for(std::chrono::seconds(3));
	while(heartbeatRunning.load())
	{
		{
			std::shared_lock<std::shared_timed_mutex> lock1(streamContainerMutex, std::defer_lock);
			std::shared_lock<std::shared_timed_mutex> lock2(liveContainerMutex, std::defer_lock);
			std::lock(lock1, lock2);
			for(auto& stream : streams)
			{
				bool isLive = stream.second->IsLive();
				if(liveStreams.find(stream.second->Name) == liveStreams.end())
				{
					if(isLive)
					{
						logger->Log_fmt(info, "Adding %s to content directory\n", stream.second->Name.c_str());
						CDResource newItem;
						newItem.name = stream.second->Name;
						newItem.mime_type = stream.second->GetMimeType();
						if(strncasecmp(stream.second->GetMimeType().c_str(), "video", 5) == 0)
							newItem.video = true;
						else
							newItem.video = false;
						liveStreams.insert(std::make_pair(stream.second->Name, newItem));
					}
				}
				else
				{
					if(!isLive)
					{
						logger->Log_fmt(info, "Removing %s from content directory\n", stream.second->Name.c_str());
						liveStreams.erase(stream.second->Name);
					}
				}
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}




void ContentDirectoryService::ExecuteAction(UpnpActionRequest* request)
{
	std::string actionName = UpnpString_get_String(UpnpActionRequest_get_ActionName(request));
	DOMString requestXML = ixmlDocumenttoString(UpnpActionRequest_get_ActionRequest(request));
	if(requestXML == nullptr)
	{
		logger->Log_fmt(error, "[ContentDirectoryService] Error converting %s action to string\n", actionName.c_str());
		return;
	}
	else
	{
		logger->Log_fmt(debug, "[ContentDirectoryService] Recieved action: %s\n%s\n", actionName.c_str(), requestXML);
		ixmlFreeDOMString(requestXML);
	}
	
	std::string responseXML;
	IXML_Document* response;
	if(actionName == "GetSearchCapabilities")
	{
		responseXML = "<SearchCaps></SearchCaps>\n";
	}
	else if(actionName == "GetSortCapabilities")
	{
		responseXML = "<SortCaps></SortCaps>\n";
	}
	else if(actionName == "GetFeatureList")
	{
		responseXML = "<Features></Features>\n";
	}
	else if(actionName == "GetSystemUpdateID")
	{
		responseXML = "<Id>0</Id>\n";
	}
	else if(actionName == "GetServiceResetToken")
	{
		responseXML = "<ResetToken>0</ResetToken>\n";
	}
	else if(actionName == "Browse")
	{
		IXML_NodeList* nodes;
		IXML_Node* node;
		std::string objID, browseFlag, filter, sortCriteria;
		int startingIndex, requestedCount;
		nodes = ixmlDocument_getElementsByTagName(UpnpActionRequest_get_ActionRequest(request), "ObjectID");
		if(nodes == nullptr || nodes->nodeItem == nullptr || nodes->nodeItem->firstChild == nullptr)
		{
			logger->Log(error, "[ContentDirectoryService] Could not get ObjectID from request\n");
			return;
		}
		node = nodes->nodeItem->firstChild;
		objID = ixmlNode_getNodeValue(node);

		nodes = ixmlDocument_getElementsByTagName(UpnpActionRequest_get_ActionRequest(request), "BrowseFlag");
		if(nodes == nullptr || nodes->nodeItem == nullptr || nodes->nodeItem->firstChild == nullptr)
		{
			logger->Log(error, "[ContentDirectoryService] Could not find BrowseFlag from request\n");
			return;
		}
		node = nodes->nodeItem->firstChild;
		browseFlag = ixmlNode_getNodeValue(node);

		nodes = ixmlDocument_getElementsByTagName(UpnpActionRequest_get_ActionRequest(request), "Filter");
		if(nodes == nullptr || nodes->nodeItem == nullptr || nodes->nodeItem->firstChild == nullptr)
		{
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
			logger->Log(error, "[ContentDirectoryService] Could not find StartingIndex from request\n");
			return;
		}
		node = nodes->nodeItem->firstChild;
		startingIndex = std::stoi(  ixmlNode_getNodeValue(node)  );

		nodes = ixmlDocument_getElementsByTagName(UpnpActionRequest_get_ActionRequest(request), "RequestedCount");
		if(nodes == nullptr || nodes->nodeItem == nullptr || nodes->nodeItem->firstChild == nullptr)
		{
			logger->Log(error, "[ContentDirectoryService] Could not find RequestedCount from request\n");
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
		catch(std::pair<int, const char*> err)
		{
			logger->Log_fmt(error, "Error handling action request: %s\n", actionName.c_str());
			logger->Log_fmt(error, "Error %s: %s\n", std::to_string(err.first).c_str(), err.second);
			UpnpActionRequest_set_ErrCode(request, err.first);
			UpnpString* errorString = UpnpString_new();
			UpnpString_set_String(errorString, err.second);
			UpnpActionRequest_set_ErrStr(request, errorString);
			responseXML = errorXML(err.first, err.second);
			return;
		}
	}
	else
	{
		logger->Log_fmt(info, "Unknown action request: %s\n", actionName.c_str());
		UpnpActionRequest_set_ErrCode(request, 401);
		UpnpString* errorString = UpnpString_new();
		UpnpString_set_String(errorString, "Invalid Action");
		UpnpActionRequest_set_ErrStr(request, errorString);
		responseXML = errorXML(401, "Invalid Action");
		return;
	}

	if(UpnpActionRequest_get_ErrCode(request) == 0)
	{
		std::stringstream ss;
		ss << "<u:" << actionName << "Response xmlns:u=\"urn:schemas-upnp-org:service:ContentDirectory:1\">\n" << \
		responseXML << "</u:" << actionName << "Response>\n";
		responseXML = ss.str();
	}

	int ret = ixmlParseBufferEx(responseXML.c_str(), &response);
	if(ret != IXML_SUCCESS)
	{
		logger->Log_fmt(error, "[ContentDirectoryService] Error %s converting response XML into document\n%s\n", std::to_string(ret).c_str(), responseXML.c_str());
	}
	else
	{
		UpnpActionRequest_set_ActionResult(request, response);
		DOMString finishedXML = ixmlDocumenttoString(response);
		logger->Log_fmt(debug, "Finished response:\n%s\n", finishedXML);
		ixmlFreeDOMString(finishedXML);
	}
}

std::string ContentDirectoryService::browse(
	std::string objID,
	std::string browseFlag,
	std::string filter,
	int startingIndex,
	int requestedCount,
	std::string sortCriteria)
{
	int numReturned = 0;
	int totalMatches = 0;
	std::string resultBody;

	if(browseFlag == "BrowseMetadata")
	{
		//Required by standard
		startingIndex = 0;
		numReturned = 1;
		totalMatches = 1;

		std::shared_lock<std::shared_timed_mutex> streamLock(streamContainerMutex, std::defer_lock);
		std::shared_lock<std::shared_timed_mutex> fileLock(fileContainerMutex, std::defer_lock);
		std::lock(streamLock, fileLock);
		resultBody = browseMetadata(objID, filter, sortCriteria);
	}
	else if(browseFlag == "BrowseDirectChildren")
	{
		std::shared_lock<std::shared_timed_mutex> streamLock(streamContainerMutex, std::defer_lock);
		std::shared_lock<std::shared_timed_mutex> fileLock(fileContainerMutex, std::defer_lock);
		std::lock(streamLock, fileLock);

		if(requestedCount == 0)
			requestedCount = 999;
		
		if(objID == "0")
		{
			totalMatches = 2;
			if(startingIndex == 0 && requestedCount > 1)
				numReturned = 2;
			else if(startingIndex <= 1)
				numReturned = 1;
		}
		else if(objID == "streams")
		{
			totalMatches = liveStreams.size();
			numReturned = requestedCount;
			if(totalMatches - startingIndex < requestedCount)
				numReturned = totalMatches - startingIndex;
		}
		else if(objID == "files")
		{
			totalMatches = files.size();
			numReturned = requestedCount;
			if(totalMatches - startingIndex < requestedCount)
				numReturned = totalMatches - startingIndex;
		}
		else
		{
			throw std::make_pair(701, "Bad object ID. Must be \"0\", \"streams\", or \"files\"");
		}

		/*if(startingIndex >= totalMatches)
			throw std::make_pair(601, "Invalid starting index");*/
		
		resultBody = browseDirectChildren(objID, filter, startingIndex, requestedCount, sortCriteria);
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
std::string ContentDirectoryService::browseMetadata(
	std::string objectId, 
	std::string filter, 
	std::string sortCriteria)
{
	std::string ret;
	if(objectId == "0")
	{
		std::stringstream ss;
		ss << "<container id=\"0\" parentID=\"-1\" childCount=\"2\" restricted=\"1\" searchable=\"0\">\n" \
			"<dc:title>" << serverName << "</dc:title>\n" \
			"<upnp:class>object.container.storageFolder</upnp:class>\n" \
		"</container>\n";
		ret = ss.str();
	}
	else if(objectId == "streams")
	{
		std::stringstream ss;
		ss << "<container id=\"streams\" parentID=\"0\" childCount=\"" << liveStreams.size() << "\" restricted=\"1\" searchable=\"0\">\n" \
			"<dc:title>Streams</dc:title>\n" \
			"<upnp:class>object.container.storageFolder</upnp:class>\n" \
		"</container>\n";
		ret = ss.str();
	}
	else if(objectId == "files")
	{
		std::stringstream ss;
		ss << "<container id=\"files\" parentID=\"0\" childCount=\"" << files.size() << "\" restricted=\"1\" searchable=\"0\">\n" \
			"<dc:title>Files</dc:title>\n" \
			"<upnp:class>object.container.storageFolder</upnp:class>\n" \
		"</container>\n";
		ret = ss.str();
	}
	else
	{
		ret = GetItemXML(objectId);
	}
	
	return ret;
}

std::string ContentDirectoryService::browseDirectChildren
(
	std::string objID,
	std::string filter,
	int startingIndex,
	int requestedCount,
	std::string sortCriteria)
{
	if(objID == "0")
	{
		std::stringstream ss;
		if(startingIndex == 0)
		{
			ss << streamContainerXML();
			if(requestedCount > 1)
				ss << fileContainerXML();
		}
		else if(startingIndex == 1)
		{
			ss << fileContainerXML();
		}
		return ss.str();
	}
	else if(objID == "streams")
	{
		std::stringstream ss;
		//FIXME - use a regular for loop here
		for(auto& item : liveStreams)
		{
			if(startingIndex-- > 0)
				continue;

			ss << getItemXML(item.second, objID);

			if(--requestedCount == 0)
				break;
		}
		return ss.str();
	}
	else if(objID == "files")
	{
		std::stringstream ss;
		//FIXME - use a regular for loop here
		for(auto& item : files)
		{
			if(startingIndex-- > 0)
				continue;

			ss << getItemXML(item.second, objID);

			if(--requestedCount == 0)
				break;
		}
		return ss.str();
	}
	else
	{
		throw std::make_pair(710, "Bad container ID");
	}
}

std::string ContentDirectoryService::GetBaseUrl()
{
	std::stringstream ss;
	ss << "http://" << UpnpGetServerIpAddress() << ":" << UpnpGetServerPort() << "/";
	return ss.str();
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
std::string ContentDirectoryService::streamContainerXML()
{
	std::stringstream ss;
	ss << "<container id=\"streams\" parentID=\"0\" childCount=\"" << liveStreams.size() << "\" restricted=\"1\" searchable=\"0\">\n" << \
		"<dc:title>Streams</dc:title>\n" << \
		"<upnp:class>object.container.storageFolder</upnp:class>\n" << \
	"</container>\n";
	return ss.str();
}
std::string ContentDirectoryService::fileContainerXML()
{
	std::stringstream ss;
	ss << "<container id=\"files\" parentID=\"0\" childCount=\"" << files.size() << "\" restricted=\"1\" searchable=\"0\">\n" << \
		"<dc:title>Files</dc:title>\n" << \
		"<upnp:class>object.container.storageFolder</upnp:class>\n" << \
	"</container>";
	return ss.str();
	
}
std::string ContentDirectoryService::GetItemXML(std::string objId)
{
	std::string parent;
	
	auto it = liveStreams.find(objId);
	if(it != liveStreams.end())
		parent = "streams";
	else if((it = files.find(objId)) != files.end())
		parent = "files";
	else
		throw std::make_pair(701, "Object not found");

	return getItemXML(it->second, parent);
}
std::string ContentDirectoryService::getItemXML(CDResource item, std::string parent)
{
	std::string upnpClass = item.video ? "object.item.videoItem" : "object.item.audioItem";
	std::stringstream ss;
	ss << "<item id=\"" << item.name << "\" parentID=\"" << parent << "\" restricted=\"1\">\n" \
		"<dc:title>" << item.name << "</dc:title>\n" \
		"<upnp:class>" << upnpClass << "</upnp:class>\n" \
		"<res id=\"" << item.name << "-res\" protocolInfo=\"http-get:*:" << item.mime_type << ":*\">" << \
			GetBaseUrl();
		if(item.path.empty())
			ss << "res/" << util::EscapeURI(item.name);
		else
			ss << util::EscapeURI(item.path);
		ss << "</res>\n" \
	"</item>\n";
	return ss.str();
}
