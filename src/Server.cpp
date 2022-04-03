#include <iostream>
#include <chrono>
#include <sstream>
#include <exception>
#include <typeindex>
#include <string.h> //strlen
#include <strings.h> //strncasecmp
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <fcntl.h>
#include <upnp/upnptools.h>
#include "Server.h"
#include "MemoryStore.h"
#include "ExternalStatusHandler.h"
#include "ExternalAVHandler.h"
#include "FileAVHandler.h"
#include "HttpStatusHandler.h"
#include "HttpAVHandler.h"
#include "Transcoder.h"
#include "util.h"
#include "Version.h"
#include "Exceptions.h"
using namespace upnp_live;

Server::Server(struct InitOptions& options) : webRoot(options.web_root)
{
	logger = Logger::GetLogger();

	//Web server stuff
	if(webRoot.empty())
		throw std::invalid_argument("Web root cannot be empty");
	if(webRoot.back() == '/' || webRoot.back() == '\\')
		throw std::invalid_argument("Web root cannot end with a slash");
	if(UpnpSetWebServerRootDir(webRoot.c_str()) != UPNP_E_SUCCESS)
		throw std::runtime_error("Error setting web server root directory");
	
	if(UpnpAddVirtualDir("/res", reinterpret_cast<const void*>(&RES), nullptr) != UPNP_E_SUCCESS)
		throw std::runtime_error("Error setting up resources virtual directory");

	loadXml(options);

	//Initialize components
	cds = std::make_unique<ContentDirectoryService>(options.friendly_name);
	cms = std::make_unique<ConnectionManagerService>();
	avStore = std::unique_ptr<AVStore>(new MemoryStore());

	//Register device
	DOMString descStr = ixmlDocumenttoString(description);
	int result = UpnpRegisterRootDevice2(UPNPREG_BUF_DESC, descStr, strlen(descStr), 1, options.event_callback, nullptr, &libHandle);
	ixmlFreeDOMString(descStr);
	switch(result)
	{
		case UPNP_E_SUCCESS:
			break;
		default:
			throw std::runtime_error(UpnpGetErrorMessage(result));
			break;
	}
	
	if(UpnpSendAdvertisement(libHandle, broadcastExpiration) != UPNP_E_SUCCESS)
		throw std::runtime_error("Error sending advertisement");
}
Server::~Server()
{
	Shutdown();
}
void Server::Shutdown()
{
	shuttingDown = true;
	UpnpRemoveAllVirtualDirs();
	UpnpUnRegisterRootDevice(libHandle);
	ixmlDocument_free(description);
}




void Server::loadXml(InitOptions& options)
{
	//Load device description in private variable
	int result = ixmlLoadDocumentEx(options.device_description.c_str(), &description);
	if(result != IXML_SUCCESS)
	{
		std::stringstream ss;
		ss << "Error " << result << " while parsing device description";
		throw std::runtime_error(ss.str().c_str());
	}

	//Check to see if there's a uuid, throw exception if there isn't
	IXML_NodeList* nodes;
	IXML_Node* node;
	nodes = ixmlDocument_getElementsByTagName(description, "UDN");
	if(nodes != nullptr && nodes->next == nullptr)
	{
		node = nodes->nodeItem;
		if(node->firstChild == nullptr)
			throw std::runtime_error("UDN element has no child node");
		//FIXME - A check should probably be made to ensure that the node at least contains the uuid: label
	}
	else throw std::runtime_error("Found zero or multiple UDN elements in description. This element and the uuid inside are mandatory");

	//Get friendly name
	nodes = ixmlDocument_getElementsByTagName(description, "friendlyName");
	if(nodes != nullptr)
	{
		node = nodes->nodeItem;
		if(node->firstChild == nullptr)
		{
			result = ixmlDocument_createTextNodeEx(description, options.friendly_name.c_str(), &node->firstChild);
			if(result != IXML_SUCCESS)
			{
				std::stringstream ss;
				ss << "Error " << result << " adding friendly name to XML document\n";
				throw std::runtime_error(ss.str());
			}
		}
		else
		{
			options.friendly_name = ixmlNode_getNodeValue(node->firstChild);
		}
	}
	else
	{
		//TODO - Add friendlyName element
		logger->Log(always, "TODO - Add friendlyName element\n");
	}

	//Add dynamic stuff to description
	//URLBase
	nodes = ixmlDocument_getElementsByTagName(description, "URLBase");
	if(nodes != nullptr)
	{
		node = nodes->nodeItem;
		if(node->firstChild != nullptr)
		{
			std::stringstream ss;
			ss << "http://" << GetAddress() << ":" << GetPort() << "/";
			ixmlNode_setNodeValue(node->firstChild, ss.str().c_str());
		}
		else
		{
			std::stringstream ss;
			ss << "http://" << GetAddress() << ":" << GetPort() << "/";
			result = ixmlDocument_createTextNodeEx(description, ss.str().c_str(), &node->firstChild);
			if(result != IXML_SUCCESS)
			{
				ss.str("");
				ss << "Error " << result << " modifying URLBase element in device description\n";
				throw std::runtime_error(ss.str());
			}
		}
	}
	else
	{
		//TODO - Add URLBase element (parent==root)
		logger->Log(always, "TODO - Add URLBase element\n");
	}
	//Model Number
	nodes = ixmlDocument_getElementsByTagName(description, "modelNumber");
	if(nodes != nullptr)
	{
		node = nodes->nodeItem;
		if(node->firstChild != nullptr)
			ixmlNode_setNodeValue(node->firstChild, upnp_live_version);
		else
		{
			result = ixmlDocument_createTextNodeEx(description, upnp_live_version, &node->firstChild);
			if(result != IXML_SUCCESS)
			{
				std::stringstream ss;
				ss << "Error " << result << " modifying modelNumber element in device description\n";
				throw std::runtime_error(ss.str().c_str());
			}
		}
	}
	else
	{
		//TODO - Add modelNumber element (parent==device)
		logger->Log(always, "TODO - Add modelNumber element\n");
	}
	//Presentation URL
	nodes = ixmlDocument_getElementsByTagName(description, "presentationURL");
	if(nodes != nullptr)
	{
		node = nodes->nodeItem;
		if(node->firstChild != nullptr)
		{
			std::stringstream ss;
			ss << "http://" << GetAddress() << ":" << GetPort() << "/";
			ixmlNode_setNodeValue(node->firstChild, ss.str().c_str());
		}
		else
		{
			std::stringstream ss;
			ss << "http://" << GetAddress() << ":" << GetPort() << "/";
			result = ixmlDocument_createTextNodeEx(description, ss.str().c_str(), &node->firstChild);
			if(result != IXML_SUCCESS)
			{
				ss.str("");
				ss << "Error " << result << " modifying presentationURL element in device description\n";
				throw std::runtime_error(ss.str().c_str());
			}
		}
	}
	else
	{
		//TODO - Add presentationURL element (parent==device)
		logger->Log(always, "TODO - Add presentationURL element\n");
	}
}




AddStreamResult Server::AddStream(StreamInitOptions& options)
{
	logger->Log_fmt(verbose, "Creating stream: %s\n", options.name.c_str());
	
	//Initialize handlers
	std::unique_ptr<AVHandler> avh {createAVHandler(options.av_handler, options.av_args)};
	std::unique_ptr<StatusHandler> sh {createStatusHandler(options.status_handler, options.status_args)};
	std::unique_ptr<Transcoder> transcoder = nullptr;
	if(!options.transcoder.empty())
	{
		std::vector<std::string> v {std::move(util::SplitArgString(options.transcoder, ' '))};
		transcoder = std::make_unique<Transcoder>(v);
	}
	std::shared_ptr<Stream> newStream = std::make_shared<Stream>(options.name, options.mime_type, std::move(avh), std::move(sh), std::move(transcoder));
	
	{
		std::lock_guard<std::mutex> guard(streamContainerMutex);
		if(streams.find(options.name) != streams.end())
			throw std::runtime_error("Stream already exists");
		auto result = streams.insert(std::make_pair(options.name, newStream));
		if(!result.second)
			return fail;
		auto it = result.first;
		if(options.buffer_size)
			it->second->BufferSize = options.buffer_size;
		else if(strncasecmp(options.mime_type.c_str(), "audio", 5) == 0)
			it->second->BufferSize = 1048576;
	}

	cds->AddStream(newStream);
	return success;
}
void Server::AddStreams(std::vector<StreamInitOptions>& streams)
{
	for(auto& stream : streams)
	{
		try
		{
			switch(AddStream(stream))
			{
				case success:
					logger->Log_fmt(info, "Stream %s created\n", stream.name.c_str());
					break;
				case heartbeatFail:
					logger->Log_fmt(warning, "Stream %s created. Error starting heartbeat\n", stream.name.c_str());
					break;
				case fail:
					logger->Log_fmt(error, "Stream %s already exists or couldn't be created\n", stream.name.c_str());
					break;
			}
		}
		catch(std::exception& e)
		{
			logger->Log_fmt(error, "Error adding stream %s\n%s\n", stream.name.c_str(), e.what());
		}
	}
}
std::unique_ptr<AVHandler> Server::createAVHandler(std::string& handlerType, std::string& argstr)
{
	if(handlerType.empty())
		throw std::invalid_argument("AV handler type required");

	std::unique_ptr<AVHandler> ret;
	if(strncasecmp(handlerType.c_str(), "ext", 3) == 0 ||
	strncasecmp(handlerType.c_str(), "cmd", 3) == 0)
	{
		logger->Log(debug, "Creating external AV handler\n");
		std::vector<std::string> v {std::move(util::SplitString(argstr, ' '))};
		ret = std::unique_ptr<AVHandler>(new ExternalAVHandler(v));
	}
	else if(strncasecmp(handlerType.c_str(), "file", 4) == 0)
	{
		logger->Log(debug, "Creating file AV handler\n");
		ret = std::unique_ptr<AVHandler>(new FileAVHandler(argstr));
	}
	else if(strncasecmp(handlerType.c_str(), "http", 4) == 0)
	{
		logger->Log(debug, "Creating http AV handler\n");
		throw std::runtime_error("http AV handler not implemented");
		//ret = std::unique_ptr<AVHandler>(new HttpAVHandler());
	}
	else
	{
		std::string err("Invalid AVHandler type: ");
		err += handlerType;
		throw std::invalid_argument(err.c_str());
	}
	return ret;
}
std::unique_ptr<StatusHandler> Server::createStatusHandler(std::string& handlerType, std::string& argstr)
{
	std::unique_ptr<StatusHandler> ret;
	if(handlerType.empty())
	{
		ret = nullptr;
	}
	else if(strncasecmp(handlerType.c_str(), "ext", 3) == 0 ||
	strncasecmp(handlerType.c_str(), "cmd", 3) == 0)
	{
		logger->Log(debug, "Creating external status handler\n");
		std::vector<std::string> v {std::move(util::SplitString(argstr, ' '))};
		ret = std::unique_ptr<StatusHandler>(new ExternalStatusHandler(v));
	}
	else if(strncasecmp(handlerType.c_str(), "http", 4) == 0)
	{
		logger->Log(debug, "Creating http status handler\n");
		struct HttpStatusOptions options{argstr};
		ret = std::unique_ptr<StatusHandler>(new HttpStatusHandler(options));
	}
	else
	{
		logger->Log_fmt(error, "Invalid StatusHandler type: %s\n", handlerType.c_str());
		ret = nullptr;
	}
	return ret;
}
void Server::AddFile(FileOptions& options)
{
	if(options.path.empty())
		throw std::invalid_argument("File not specified");
	if(options.path[0] == '/')
		throw std::invalid_argument("Must be relative path");

	//TODO - Add a check here to not include text files and the like

	std::string relPath = webRoot + "/" + options.path;
	if(!util::FileExists(relPath.c_str()))
		throw std::invalid_argument("File does not exist");

	if(options.mime_type.empty())
	{
		if(options.path.find(".") == std::string::npos || options.path.find_last_of(".") == options.path.size()-1)
			throw std::invalid_argument("File must have an extension");

		auto i = options.path.find_last_of('.');
		auto ext = options.path.substr(i+1);
		options.mime_type = util::GetMimeType(ext.c_str());
	}

	if(!cds->AddFile(options.name, options.mime_type, options.path))
		logger->Log_fmt(error, "Error adding file to CDS: %s\n", options.path.c_str());
}
void Server::AddFiles(std::vector<FileOptions>& files)
{
	for(auto& file : files)
	{
		try
		{
			AddFile(file);
			logger->Log_fmt(info, "Added %s to content directory\n", file.path.c_str());
		}
		catch(std::exception& e)
		{
			logger->Log_fmt(error, "Error adding file %s\n%s\n", file.name.c_str(), e.what());
		}
	}
}
void Server::RemoveFile(std::string filepath)
{
	cds->RemoveFile(filepath);
}
void Server::RemoveFiles(std::vector<std::string>& files)
{
	for(auto& file : files)
	{
		try
		{
			RemoveFile(file);
			logger->Log_fmt(info, "Removed %s from content directory\n", file.c_str());
		}
		catch(std::exception& e)
		{
			logger->Log_fmt(error, "Error removing file %s\n%s\n", file.c_str(), e.what());
		}
	}
}




//{ Callbacks
/*
 * Struct File_Info {
 * 	off_t file_length;
 * 	time_t last_modified;
 * 	int is_directory;
 * 	int is_readable;
 * 	DOMString content_type; -> starts as NULL, must be set to a dynamically allocated DOMString (aka c string)
 * }
*/
int Server::GetInfo(const char* filename, UpnpFileInfo* info)
{
	std::string fname = filename;
	std::vector<std::string> uri = util::SplitString(fname, '/');
	if(uri[0] != "res")
	{
		std::string s = "Bad URI: ";
		s += fname;
		throw std::runtime_error(s);
	}
	
	std::string mimetype;
	{
		std::lock_guard<std::mutex> guard(streamContainerMutex);
		auto it = streams.find(uri[1]);
		if(it == streams.end())
			throw std::runtime_error("stream not found");
		mimetype = it->second->GetMimeType();
	}
	
	time_t timestamp;
	time(&timestamp);
	UpnpFileInfo_set_LastModified(info, timestamp);
	UpnpFileInfo_set_FileLength(info, -1);
	UpnpFileInfo_set_IsDirectory(info, 0);
	UpnpFileInfo_set_IsReadable(info, 1);
	/*DOMString headers = ixmlCloneDOMString("");
	UpnpFileInfo_set_ExtraHeaders(info, headers);*/
	DOMString ctype = ixmlCloneDOMString(mimetype.c_str());
	UpnpFileInfo_set_ContentType(info, ctype);
	return 0;
}

UpnpWebFileHandle Server::Open(const char* filename, enum UpnpOpenFileMode mode)
{
	if(mode == UPNP_WRITE)
	{
		logger->Log(error, "UPNP_WRITE mode not accepted");
		return nullptr;
	}
	
	std::string fname(filename);
	std::vector<std::string> uri = util::SplitString(fname, '/');
	if(uri[0] != "res" || uri.size() < 2)
	{
		logger->Log_fmt(error, "Open(): Bad URI - %s\n", fname.c_str());
		return nullptr;
	}
	
	std::shared_ptr<Stream> stream_obj;

	{
		std::unique_lock<std::mutex> streamLock(streamContainerMutex);
		auto it = streams.find(uri[1]);
		if(it == streams.end())
		{
			logger->Log(error, "Open(): No stream named %s\n", uri[1]);
			return nullptr;
		}
		stream_obj = *it;
	}
		
	{
		std::unique_lock<std::mutex> handleLock(handleMutex);
		VirtualFileHandle newHandle;
		newHandle.stream_name = stream_obj->Name;
		newHandle.data = avStore->CreateHandle(stream_obj);
		auto handleIt = handles.insert(std::make_pair(stream_obj->Name, newHandle));
		return reinterpret_cast<UpnpWebFileHandle>(&handleIt->second);
	}
}


int Server::Close(UpnpWebFileHandle hnd)
{
	auto handle = reinterpret_cast<VirtualFileHandle*>(hnd);
	if(!handle)
	{
		logger->Log(error, "Error casting handle");
		return -1;
	}
	avStore->DestroyHandle(handle->data);

	auto range = handles.equal_range(handle->stream_name);
	for(auto it = range.first; std::distance(it, range.second) > 0; ++it)
	{
		if(&it->second == handle)
		{
			handles.erase(it);
			return 0;
		}
	}
	logger->Log(error, "Close(): Handle not in container\n");
	return -1;
}


int Server::Read(UpnpWebFileHandle hnd, char* buf, std::size_t len)
{
	auto handle = reinterpret_cast<VirtualFileHandle*>(hnd);
	if(!handle)
	{
		logger->Log(error, "Error casting handle");
		return -1;
	}
	//I think pupnp expects read calls to be blocking,
	//so loop in case we can't read from memory store
	while(!shuttingDown.load())
	{
		try
		{
			return avStore->Read(handle->data, buf, len);
		}
		catch(const CantReadBuffer& e)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
		}
	}
	return 0;
}
int Server::Seek(UpnpWebFileHandle hnd, long offset, int origin)
{
	return 0;
}
int Server::Write(UpnpWebFileHandle hnd, char* buf, std::size_t len)
{
	return 0;
}
int Server::IncomingEvent(Upnp_EventType eventType, const void* event)
{
	switch(eventType)
	{
		case UPNP_CONTROL_ACTION_REQUEST:
			execActionRequest(static_cast<UpnpActionRequest*>(const_cast<void*>(event)));
			break;

		case UPNP_CONTROL_GET_VAR_REQUEST:
			execStateVarRequest(static_cast<UpnpStateVarRequest*>(const_cast<void*>(event)));
			break;

		case UPNP_EVENT_SUBSCRIPTION_REQUEST:
			execSubscriptionRequest(static_cast<UpnpSubscriptionRequest*>(const_cast<void*>(event)));
			break;
		default:
			throw std::invalid_argument("Unknown event type");
			break;
	}
	//FIXME - lib ignores return value, check if that's still true for versions >1.8
	return 0;
}
void Server::execActionRequest(UpnpActionRequest* request)
{
	std::string serviceid = UpnpString_get_String(UpnpActionRequest_get_ServiceID(request));

	if(serviceid.find("ConnectionManager") != std::string::npos)
	{
		try
		{
			cms->ExecuteAction(request);
		}
		catch(std::exception& e)
		{
			std::string s = "ConnectionManager error: ";
			s += e.what();
			throw std::runtime_error(s);
		}
	}
	else if(serviceid.find("ContentDirectory") != std::string::npos)
	{
		try
		{
			cds->ExecuteAction(request);
		}
		catch(std::exception& e)
		{
			std::string s = "ContentDirectory error: ";
			s += e.what();
			throw std::runtime_error(s);
		}
	}
	else
	{
		std::string actionName = UpnpString_get_String(UpnpActionRequest_get_ActionName(request));
		std::string s = "Unknown action request. Service ID ";
		s += serviceid;
		s += ". Action name: ";
		s += actionName;
		throw std::runtime_error(s);
	}
}
void Server::execStateVarRequest(UpnpStateVarRequest* request)
{
	std::string serviceid = UpnpString_get_String(UpnpStateVarRequest_get_ServiceID(request));
	std::string statevarname = UpnpString_get_String(UpnpStateVarRequest_get_StateVarName(request));

	logger->Log_fmt(info, "Received state variable request\nService ID: %s\nState Variable name: %s\n", serviceid.c_str(), statevarname.c_str());
}
void Server::execSubscriptionRequest(UpnpSubscriptionRequest* request)
{
	std::string serviceid = UpnpString_get_String(UpnpSubscriptionRequest_get_ServiceId(request));
	std::string udn = UpnpString_get_String(UpnpSubscriptionRequest_get_UDN(request));

	logger->Log_fmt(info, "Received subscription request\nUniversal Device Name: %s\nService ID: %s\n", udn.c_str(), serviceid.c_str());
}
//}




//{ Simple accessors/mutators
int Server::GetPort()
{
	return UpnpGetServerPort();
}
std::string Server::GetAddress()
{
	return std::string(UpnpGetServerIpAddress());
}
std::string Server::GetBaseResURI()
{
	std::stringstream ss;
	ss << "http://" << GetAddress() << ":" << GetPort() << "/res/";
	return ss.str();
}
std::string Server::GetFriendlyName()
{
	std::string s;
	IXML_NodeList* nodes;
	IXML_Node* node;
	nodes = ixmlDocument_getElementsByTagName(description, "friendlyName");
	if(nodes != nullptr)
	{
		node = nodes->nodeItem;
		if(node->firstChild != nullptr)
			s = ixmlNode_getNodeValue(node->firstChild);
	}
	return s;
}
//}
