#pragma once
#include <exception>
#include <string>

namespace upnp_live {

class ServerInitException : public std::exception
{
	private:
		std::string message;
	public:
		ServerInitException(const char*);
		void setMessage(const char*);
		virtual const char* what();
};

class ServiceInitException : public std::exception
{
	private:
		std::string serviceName, message;
	public:
		ServiceInitException(const char* name);
		void setMessage(const char*);
		void setMessage(std::string);
		virtual const char* what();
};

class DeviceInitException : public std::exception
{
	private:
		std::string message;

	public:
		DeviceInitException(const char*);
		virtual const char* what();
};

class ActionException : public std::exception
{
	private:
		int errorCode;
		std::string message;
	public:
		ActionException(int, const char*);
		virtual const char* what();
		int getErrorCode();
		std::string getMessage();
};
}
