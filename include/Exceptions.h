#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H
#include <exception>

namespace upnp_live {

class AlreadyInitializedException : public std::exception
{
	const char* what() { return "Resource already initialized"; }
};

class CantReadBuffer : public std::exception
{
	const char* what() { return "Buffer is empty or uninitialized"; }
};

}
#endif
