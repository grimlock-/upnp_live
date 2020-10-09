#include <iostream>
#include <chrono>
#include <sstream>
#include <exception>
#include <typeindex>
#include <string.h> //strerror
#include <strings.h> //strncasecmp
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <fcntl.h>
#include "Server.h"
#include "MemoryStore.h"
#include "ExternalAVHandler.h"
#include "FileAVHandler.h"
#include "ExternalStatusHandler.h"
#include "Transcoder.h"
#include "util.h"
#include "Version.h"
#include "Exceptions.h"
using namespace upnp_live;

Server::Server(struct InitOptions& options)
{
	logger = Logger::GetLogger();

	loadXml(options);
	cds = std::make_unique<ContentDirectoryService>(options.friendly_name);
	cms = std::make_unique<ConnectionManagerService>();

	//Initialize AVStore
	avStore = std::unique_ptr<AVStore>(new MemoryStore());
	
	//Web server stuff
	if(options.web_root == "")
	{
		webRoot = "www";
	}
	else
	{
		webRoot = options.web_root;
		while(webRoot.back() == '/')
			webRoot.pop_back();
	}
	if(UpnpSetWebServerRootDir(webRoot.c_str()) != UPNP_E_SUCCESS)
		throw std::runtime_error("Error setting web server root directory");
	
	if(UpnpAddVirtualDir("/res", reinterpret_cast<const void*>(&RES), nullptr) != UPNP_E_SUCCESS)
		throw std::runtime_error("Error setting up resources virtual directory");
	
	//TODO - add API stuff here

	//Register device and callbacks
	DOMString descStr = ixmlDocumenttoString(description);
	int result = UpnpRegisterRootDevice2(UPNPREG_BUF_DESC, descStr, strlen(descStr), 1, options.event_callback, nullptr, &libHandle);
	ixmlFreeDOMString(descStr);
	switch(result)
	{
		case UPNP_E_SUCCESS:
			break;
		case UPNP_E_INVALID_DESC:
			throw std::runtime_error("Invalid device description");
			break;
		case UPNP_E_FILE_NOT_FOUND:
			throw std::runtime_error("Could not find device description");
			break;
		default:
			std::string s = "Unknown pupnp error code: ";
			s += std::to_string(result);
			throw std::runtime_error(s.c_str());
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
	logger->Log_cc(verbose, 3, "Creating stream: ", options.name.c_str(), "\n");
	
	//Initialize handlers
	std::unique_ptr<AVSource> avh {createAVHandler(options.av_handler, options.av_args)};
	std::unique_ptr<StatusHandler> sh {createStatusHandler(options.status_handler, options.status_args)};
	std::unique_ptr<Transcoder> transcoder = nullptr;
	if(!options.transcoder.empty())
	{
		std::vector<std::string> v {std::move(util::SplitString(options.transcoder, ' '))};
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
					logger->Log_cc(info, 3, "Stream ", stream.name.c_str(), " created\n");
					break;
				case heartbeatFail:
					logger->Log_cc(warning, 3, "Stream ", stream.name.c_str(), " created. Error starting heartbeat\n");
					break;
				case fail:
					logger->Log_cc(error, 3, "Stream ", stream.name.c_str(), " already exists or couldn't be created\n");
					break;
			}
		}
		catch(std::exception& e)
		{
			logger->Log_cc(error, 5, "Error adding stream ", stream.name.c_str(), "\n", e.what(), "\n");
		}
	}
}
std::unique_ptr<AVSource> Server::createAVHandler(std::string& handlerType, std::string& argstr)
{
	if(handlerType.empty())
		throw std::invalid_argument("AV handler type required");

	std::unique_ptr<AVSource> ret;
	if(strncasecmp(handlerType.c_str(), "ext", 3) == 0 ||
	strncasecmp(handlerType.c_str(), "cmd", 3) == 0)
	{
		logger->Log(debug, "Creating external AV handler\n");
		std::vector<std::string> v {std::move(util::SplitString(argstr, ' '))};
		ret = std::unique_ptr<AVSource>(new ExternalAVHandler(v));
	}
	else if(strncasecmp(handlerType.c_str(), "file", 4) == 0)
	{
		logger->Log(debug, "Creating file AV handler\n");
		ret = std::unique_ptr<AVSource>(new FileAVHandler(argstr));
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
	else
	{
		logger->Log_cc(error, 3, "Invalid StatusHandler type: ", handlerType.c_str(), "\n");
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

	if(!util::FileExists(options.path.c_str()))
		throw std::invalid_argument("File does not exist");

	if(options.mime_type.empty())
	{
		if(options.path.find(".") == std::string::npos)
			throw std::invalid_argument("File must have an extension");

		auto i = options.path.find_last_of('.');
		auto ext = options.path.substr(i+1);
		if(ext.empty())
			throw std::invalid_argument("No extension found");
		options.mime_type = util::GetMimeType(ext.c_str());
	}

	if(!cds->AddFile(options.name, options.mime_type, options.path))
		logger->Log_cc(error, 3, "Error adding file to CDS: ", options.path.c_str(), "\n");
}
void Server::AddFiles(std::vector<FileOptions>& files)
{
	for(auto& file : files)
	{
		try
		{
			AddFile(file);
		}
		catch(std::exception& e)
		{
			logger->Log_cc(error, 5, "Error adding file ", file.name.c_str(), "\n", e.what(), "\n");
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
int Server::GetInfo(const char* filename, UpnpFileInfo* info, const void* cookie)
{
	const vdirs* requestType = reinterpret_cast<const vdirs*>(cookie);
	switch(*requestType)
	{
		case resources:
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
		break;
		
		default:
			throw std::runtime_error("GetInfo() only supported with resource URLs");
			break;
	}
}

UpnpWebFileHandle Server::Open(const char* filename, enum UpnpOpenFileMode mode, const void* cookie)
{
	const vdirs* requestType = reinterpret_cast<const vdirs*>(cookie);
	switch(*requestType)
	{
		case resources:
		{
			if(mode == UPNP_WRITE)
				throw std::invalid_argument("UPNP_WRITE mode not accepted");
			
			VirtualFileHandle newHandle;
			std::string fname(filename);
			std::vector<std::string> uri = util::SplitString(fname, '/');
			if(uri[0] != "res" || uri.size() < 2)
			{
				std::string s = "Bad URI: ";
				s += fname;
				throw std::runtime_error(s);
			}
			
			{
				std::unique_lock<std::mutex> streamLock(streamContainerMutex, std::defer_lock);
				std::unique_lock<std::mutex> handleLock(handleMutex, std::defer_lock);
				std::lock(streamLock, handleLock);
				auto it = streams.find(uri[1]);
				if(it == streams.end())
				{
					std::string s = "No stream named ";
					s += uri[1];
					throw std::runtime_error(s);
				}
				
				newHandle.stream_name = it->second->Name;
				newHandle.type = avstore;
				newHandle.data = avStore->CreateHandle(it->second);
				auto handleIt = handles.insert(std::make_pair(it->second->Name, newHandle));

				std::stringstream ss;
				ss << "Client handle (" << static_cast<const void*>(&handleIt->second) << ") created for " << handleIt->first << "\n";
				logger->Log(info, ss.str());
				return reinterpret_cast<UpnpWebFileHandle>(&handleIt->second);
			}
		}
		break;
		
		default:
			throw std::runtime_error("Open() only supported for resource URLs");
			break;
	}
}


int Server::Close(UpnpWebFileHandle hnd, const void* cookie)
{
	const vdirs* requestType = reinterpret_cast<const vdirs*>(cookie);
	switch(*requestType)
	{
		case resources:
		{
			std::stringstream ss;
			ss << "Closing client handle " << static_cast<const void*>(hnd) << "\n";
			logger->Log(info, ss.str());
			auto handle = static_cast<VirtualFileHandle*>(hnd);
			if(handle->type == fd)
			{
				std::lock_guard<std::mutex> guard(handleMutex);
				close(fd);
				auto range = handles.equal_range(handle->stream_name);
				for(auto it = range.first; std::distance(it, range.second) > 0; ++it)
				{
					if(it->second.type == handle->type && it->second.fd == handle->fd)
						handles.erase(it);
				}
				return (handle->fd);
			}
			else
			{
				return avStore->DestroyHandle(handle->data);
			}
		}
		break;
		
		default:
			throw std::runtime_error("Close() only supported with resource URLs");
			break;
	}
	return 0;
}


int Server::Read(UpnpWebFileHandle hnd, char* buf, std::size_t len, const void* cookie)
{
	const vdirs* requestType = reinterpret_cast<const vdirs*>(cookie);
	switch(*requestType)
	{
		case resources:
		{
			auto handle = static_cast<VirtualFileHandle*>(hnd);
			//FIXME - I expect this may still cause a null pointer dereference when shutting down
			//so I'll need a better way of shutting down this function
			while(!shuttingDown.load())
			{
				try
				{
					int ret = avStore->Read(handle->data, buf, len);
					return ret;
				}
				catch(CantReadBuffer& e)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
				}
			}
			return 0;
		}
		break;
		
		default:
			throw std::runtime_error("Read() only supported with resource URLs");
			break;
	}
}
int Server::Seek(UpnpWebFileHandle hnd, long offset, int origin, const void* cookie)
{
	return 0;
}
int Server::Write(UpnpWebFileHandle hnd, char* buf, std::size_t len, const void* cookie)
{
	return 0;
}
int Server::IncomingEvent(Upnp_EventType eventType, const void* event, void* cookie)
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

	logger->Log_cc(info, 5, "Received state variable request\nService ID: ", serviceid.c_str(), "State Variable name: ", statevarname.c_str(), "\n");
}
void Server::execSubscriptionRequest(UpnpSubscriptionRequest* request)
{
	std::string serviceid = UpnpString_get_String(UpnpSubscriptionRequest_get_ServiceId(request));
	std::string udn = UpnpString_get_String(UpnpSubscriptionRequest_get_UDN(request));

	logger->Log_cc(info, 4, "Received subscription request\nUniversal Device Name: ", udn.c_str(), "\nService ID: ", serviceid.c_str());
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
