#include "RootDevice.h"
#include "ConnectionManagerService.h"
#include "ContentDirectoryService.h"
#include "util.h"
#include <cstring>
using namespace upnp_live;

RootDevice::RootDevice(std::string xmlDoc) : Device(xmlDoc)
{
	mUPnP::ServiceList* sl = getServiceList();
	std::vector<mUPnP::Service*>::iterator i = sl->begin();
	while(i != sl->end())
	{
		//Double dereference because the service list is actually a vector of Service* instead of just Service
		std::string type = (*i)->getServiceType();
		if(type.find("ContentDirectory") != nullptr)
		{
			mupnp_shared_ptr<uXML::Node> node = (*i)->getServiceNode();
			contentDirectoryService = new ContentDirectoryService(node);
			(*i)->setActionListener(contentDirectoryService);
		}
		else if(type.find("ConnectionManager") != nullptr)
		{
			try {
				mupnp_shared_ptr<uXML::Node> node = (*i)->getServiceNode();
				connectionManagerService = new ConnectionManagerService(node);
				(*i)->setActionListener(connectionManagerService);
			}
			catch (ServiceInitException& e) {
				delete contentDirectoryService;
				throw e;
			}
		}
		i++;
	}
}

RootDevice::~RootDevice()
{
	delete contentDirectoryService;
	delete connectionManagerService;
}

/*uHTTP::HTTP::StatusCode RootDevice::httpRequestRecieved(uHTTP::HTTPRequest* request)
{
	if(request->isGetRequest() || request->isHeadRequest())
		return getRequestRecieved(request);
	if(request->isPostRequest() && request->isSOAPAction())
		return controlRequestRecieved(request);
	return request->returnBadRequest();
}*/

uHTTP::HTTP::StatusCode RootDevice::getRequestRecieved(uHTTP::HTTPRequest* request)
{
	const char* response = "";
	uHTTP::URI uri;
	request->getURI(uri);
	mUPnP::Service* service = getServiceBySCPDURL(uri.getSting());
	std::vector<std::string> path = util::SplitString(static_cast<std::string>(uri.getPath()), '/');

	if(isDescriptionURI(uri.getSting()))
	{
		response = getDeviceDescription();
	}
	else if(service)
	{
		std::string str;
		response = service->getSCPDData(str);
	}
	else if(path.size() == 4 && path[0] == "stream" && path[2] == "quality")
	{
		std::string streamID = path[1], quality = path[3];
		std::cout << "Client requested stream with id (" << streamID << ") and quality setting (" << quality << ")\n";
		return request->returnResponse(501);
	}
	else
	{
		return request->returnBadRequest();
	}

	uHTTP::HTTPResponse responseObj;
	if(uHTTP::File::isXMLFileName(uri.getSting()))
		responseObj.setContentType(uXML::XML::CONTENT_TYPE);
	responseObj.setStatusCode(uHTTP::HTTP::OK_REQUEST);
	responseObj.setContent(response);
	return request->post(&responseObj);
}

uHTTP::HTTP::StatusCode RootDevice::controlRequestRecieved(uHTTP::HTTPRequest* request)
{
	uHTTP::URI uri;
	request->getURI(uri);
	mUPnP::Service* service = getServiceByControlURL(uri.getSting());
	if(!service)
		return badRequest(request);

	mUPnP::ControlRequest controlRequest({request});
	if(controlRequest.isQueryControl())
	{
		mUPnP::QueryRequest queryRequest({request});
		if(!service->hasStateVariable(queryRequest.getVarName()))
			return badRequest(request);
		mUPnP::StateVariable* sVar = getStateVariable(queryRequest.getVarName());
		if(!sVar->performQueryListener(&queryRequest))
			return badRequest(request);
		return uHTTP::HTTP::OK_REQUEST;
	}
	std::string buf;
	mUPnP::ActionRequest actionRequest({request});
	mUPnP::Action* action = service->getAction(actionRequest.getActionName(buf));
	if(!action)
		return badRequest(request);
	mUPnP::ArgumentList* actionArgs = action->getArgumentList();
	actionArgs->set(actionRequest.getArgumentList());
	if(!action->performActionListener(&actionRequest))
		return badRequest(request);
	return uHTTP::HTTP::OK_REQUEST;
}

uHTTP::HTTP::StatusCode RootDevice::badRequest(uHTTP::HTTPRequest* request)
{
	mUPnP::ControlResponse response;
	response.setFaultResponse(mUPnP::UPnP::INVALID_ACTION);
	return request->post(&response);
}

const char* RootDevice::getDeviceDescription()
{
	std::string buffer = "";
	lock();
		mupnp_shared_ptr<uXML::Node> rootNode = getRootNode();
		if(rootNode)
		{
			std::string nodeBuffer;
			buffer = mUPnP::UPnP::XML_DECLARATION;
			buffer += "\n";
			buffer += rootNode->toString(nodeBuffer);
		}
	unlock();
	return buffer.c_str();
}
