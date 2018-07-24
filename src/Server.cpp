#include <iostream>
#include <chrono>
#include <cstring>
#include <sstream>
#include <sys/time.h>
#include "Server.h"
#include "StreamHandler.h"
#include "Exceptions.h"
#include "Version.h"
using namespace upnp_live;

Server::Server(const struct InitOptions& options)
{
	int result;

	//Load device description in private variable
	result = ixmlLoadDocumentEx("RootDevice.xml", &description);
	if(result != IXML_SUCCESS)
	{
		std::stringstream ss;
		ss << "Error " << result << " while parsing device description";
		throw ServerInitException(ss.str().c_str());
	}

	//Check to see if there's a uuid, throw exception if there isn't
	IXML_NodeList* nodes;
	IXML_Node* node;
	nodes = ixmlDocument_getElementsByTagName(description, "UDN");
	if(nodes != nullptr && nodes->next == nullptr)
	{
		node = nodes->nodeItem;
		if(node->firstChild == nullptr)
			throw ServerInitException("UDN element has no child node");
		//FIXME - A check should probably be made to ensure that the node at least contains the uuid: label
	}
	else throw ServerInitException("Found zero or multiple UDN elements in description. This element and the uuid inside are mandatory");

	//Add dynamic stuff to description
	//URLBase
	nodes = ixmlDocument_getElementsByTagName(description, "URLBase");
	if(nodes != nullptr)
	{
		node = nodes->nodeItem;
		if(node->firstChild != nullptr)
		{
			std::stringstream ss;
			ss << "http://" << getAddress() << ":" << getPort() << "/";
			ixmlNode_setNodeValue(node->firstChild, ss.str().c_str());
		}
		else
		{
			std::stringstream ss;
			ss << "http://" << getAddress() << ":" << getPort() << "/";
			result = ixmlDocument_createTextNodeEx(description, ss.str().c_str(), &node->firstChild);
			if(result != IXML_SUCCESS)
			{
				ss.str("");
				ss << "Error " << result << " modifying URLBase element in device description\n";
				throw ServerInitException(ss.str().c_str());
			}
		}
	}
	else
	{
		//TODO - Add URLBase element (parent==root)
		std::cout << "TODO - Add URLBase element" << std::endl;
	}
	//Model Number
	nodes = ixmlDocument_getElementsByTagName(description, "modelNumber");
	if(nodes != nullptr)
	{
		node = nodes->nodeItem;
		if(node->firstChild != nullptr)
			ixmlNode_setNodeValue(node->firstChild, UPNP_LIVE_VERSION_STRING);
		else
		{
			result = ixmlDocument_createTextNodeEx(description, UPNP_LIVE_VERSION_STRING, &node->firstChild);
			if(result != IXML_SUCCESS)
			{
				std::stringstream ss;
				ss << "Error " << result << " modifying modelNumber element in device description\n";
				throw ServerInitException(ss.str().c_str());
			}
		}
	}
	else
	{
		//TODO - Add modelNumber element (parent==device)
		std::cout << "TODO - Add modelNumber element" << std::endl;
	}
	//Presentation URL
	nodes = ixmlDocument_getElementsByTagName(description, "presentationURL");
	if(nodes != nullptr)
	{
		node = nodes->nodeItem;
		if(node->firstChild != nullptr)
		{
			std::stringstream ss;
			ss << "http://" << getAddress() << ":" << getPort() << "/";
			ixmlNode_setNodeValue(node->firstChild, ss.str().c_str());
		}
		else
		{
			std::stringstream ss;
			ss << "http://" << getAddress() << ":" << getPort() << "/";
			result = ixmlDocument_createTextNodeEx(description, ss.str().c_str(), &node->firstChild);
			if(result != IXML_SUCCESS)
			{
				ss.str("");
				ss << "Error " << result << " modifying presentationURL element in device description\n";
				throw ServerInitException(ss.str().c_str());
			}
		}
	}
	else
	{
		//TODO - Add presentationURL element (parent==device)
		std::cout << "TODO - Add presentationURL element" << std::endl;
	}


	//Set web server root and virtual directories
	/*struct UpnpVirtualDirCallbacks callbacks = {
		.get_info = &StreamHandler::getInfoCallback,
		.open = &StreamHandler::openCallback,
		.close = &StreamHandler::closeCallback,
		.read = &StreamHandler::readCallback,
		.write = &StreamHandler::writeCallback,
		.seek = &StreamHandler::seekCallback
	}*/
	UpnpVirtualDir_set_GetInfoCallback(&StreamHandler::getInfoCallback);
	UpnpVirtualDir_set_OpenCallback(&StreamHandler::openCallback);
	UpnpVirtualDir_set_CloseCallback(&StreamHandler::closeCallback);
	UpnpVirtualDir_set_ReadCallback(&StreamHandler::readCallback);
	UpnpVirtualDir_set_WriteCallback(&StreamHandler::writeCallback);
	UpnpVirtualDir_set_SeekCallback(&StreamHandler::seekCallback);
	if(options.webRoot == "")
		webRoot = "www";
	else
		webRoot = options.webRoot;
	result = UpnpSetWebServerRootDir(webRoot.c_str());
	if(result != UPNP_E_SUCCESS)
	{
		std::stringstream ss;
		ss << "Error " << result << " setting web server root directory";
		throw ServerInitException(ss.str().c_str());
	}
	//result = UpnpSetVirtualDirCallbacks(&callbacks);
	/*if(result != UPNP_E_SUCCESS)
	{
		std::stringstream ss;
		ss << "Error " << result << " adding virtual directory callbacks";
		throw ServerInitException(ss.str().c_str());
	}*/
	result = UpnpAddVirtualDir("/res/", nullptr, nullptr);
	if(result != UPNP_E_SUCCESS)
	{
		std::stringstream ss;
		ss << "Error " << result << " setting up virtual directory";
		throw ServerInitException(ss.str().c_str());
	}

	//Register device
	DOMString descStr = ixmlDocumenttoString(description);
	result = UpnpRegisterRootDevice2(UPNPREG_BUF_DESC, descStr, strlen(descStr), 1, &Server::callbackWrapper, this, &libHandle);
	ixmlFreeDOMString(descStr);
	checkRegisterResult(result);

	if(UpnpSendAdvertisement(libHandle, broadcastExpiration) != UPNP_E_SUCCESS)
		throw ServerInitException("Error sending advertisement");

	cds = new ContentDirectoryService(this);
	cms = new ConnectionManagerService();

	if(options.streams.size() > 0)
		cds->addStreams(options.streams);
}
Server::~Server()
{
	delete cds;
	delete cms;
	UpnpUnRegisterRootDevice(libHandle);
}



int Server::callbackWrapper(Upnp_EventType eventType, const void* event, void* cookie)
{
	Server* s = static_cast<Server*>(cookie);
	s->incomingEvent(eventType, const_cast<void*>(event));
	return 0;
}
void Server::incomingEvent(Upnp_EventType eventType, void* event)
{
	switch(eventType)
	{
		case UPNP_CONTROL_ACTION_REQUEST:
			execActionRequest(static_cast<UpnpActionRequest*>(event));
			break;

		case UPNP_CONTROL_GET_VAR_REQUEST:
			execStateVarRequest(static_cast<UpnpStateVarRequest*>(event));
			break;

		case UPNP_EVENT_SUBSCRIPTION_REQUEST:
			execSubscriptionRequest(static_cast<UpnpSubscriptionRequest*>(event));
			break;
		default:
			std::cout << "Recieved unknown event type from pupnp library: " << eventType << std::endl;
			break;
	}
}
void Server::execActionRequest(UpnpActionRequest* request)
{
	std::string serviceid = UpnpString_get_String(UpnpActionRequest_get_ServiceID(request));

	if(serviceid.find("ConnectionManager") != std::string::npos)
	{
		cms->executeAction(request);
	}
	else if(serviceid.find("ContentDirectory") != std::string::npos)
	{
		cds->executeAction(request);
	}
	else
	{
		std::string actionName = UpnpString_get_String(UpnpActionRequest_get_ActionName(request));
		std::cout << "Received unknown action request\n\tService ID: " << serviceid << \
		"\n\tAction Name: " << actionName << std::endl;
	}
}
void Server::execStateVarRequest(UpnpStateVarRequest* request)
{
	std::string serviceid = UpnpString_get_String(UpnpStateVarRequest_get_ServiceID(request));
	std::string statevarname = UpnpString_get_String(UpnpStateVarRequest_get_StateVarName(request));

	std::cout << "Received unknown state variable request\n\tService ID: " << serviceid << \
	"\n\tState Variable name: " << statevarname << std::endl;
}
void Server::execSubscriptionRequest(UpnpSubscriptionRequest* request)
{
	std::string serviceid = UpnpString_get_String(UpnpSubscriptionRequest_get_ServiceId(request));
	std::string udn = UpnpString_get_String(UpnpSubscriptionRequest_get_UDN(request));

	std::cout << "Received unknown subscription request\n\tUniversal Device Name: " << udn << "\n\t" << \
	"Service ID: " << serviceid << std::endl;
}




int Server::getPort()
{
	return UpnpGetServerPort();
}
std::string Server::getAddress()
{
	std::string s = UpnpGetServerIpAddress();
	return s;
}
std::string Server::getBaseResURI()
{
	std::stringstream ss;
	ss << "http://" << getAddress() << ":" << getPort() << "/res/";
	return ss.str();
}
std::string Server::getFriendlyName()
{
	std::string s;
	IXML_NodeList* nodes;
	IXML_Node* node;
	nodes = ixmlDocument_getElementsByTagName(description, "friendlyName");
	if(nodes != nullptr)
	{
		node = nodes->nodeItem;
		if(node->firstChild != nullptr)
		{
			s = ixmlNode_getNodeValue(node->firstChild);
		}
	}
	return s;
}
ContentDirectoryService* Server::getCDS()
{
	return cds;
}
ConnectionManagerService* Server::getCMS()
{
	return cms;
}




void Server::checkRegisterResult(int& result)
{
	switch(result)
	{
		case UPNP_E_SUCCESS:
			break;
		case UPNP_E_INVALID_DESC:
			throw ServerInitException("Invalid device description");
			break;
		case UPNP_E_FILE_NOT_FOUND:
			throw ServerInitException("Could not find device description");
			break;
		default:
			std::string s = "Unknown error code: ";
			s += std::to_string(result);
			throw ServerInitException(s.c_str());
			break;
	}
}
