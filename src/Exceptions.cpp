#include <sstream>
#include "Exceptions.h"
using namespace upnp_live;

ServerInitException::ServerInitException(const char* msg) : message(msg) {}
const char* ServerInitException::what()
{
	return message.c_str();
}

ServiceInitException::ServiceInitException(const char* service) : serviceName(service) {}
void ServiceInitException::setMessage(const char* message)
{
	this->message = message;
}
void ServiceInitException::setMessage(std::string message)
{
	this->message = message;
}
const char* ServiceInitException::what()
{
	std::stringstream ss;
	ss << "Could not initialize service \"" << serviceName << "\"";
	if(message.length() > 0)
		ss << ": " << message;
	std::string s = ss.str();
	return s.c_str();
}




DeviceInitException::DeviceInitException(const char* msg) : message(msg) {}
const char* DeviceInitException::what()
{
	return message.c_str();
}




ActionException::ActionException(int err, const char* msg) : errorCode(err), message(msg) {}
int ActionException::getErrorCode()
{
	return errorCode;
}
std::string ActionException::getMessage()
{
	return message;
}
const char* ActionException::what()
{
	std::string s = "Error ";
	s += errorCode;
	s += ": ";
	s += message;
	return s.c_str();
}
