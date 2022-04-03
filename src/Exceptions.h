#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H
#include <exception>

namespace upnp_live {

/*class AlreadyInitializedException : public std::exception
{
	const char* what() { return "Resource already initialized"; }
};*/

class CantReadBuffer : public std::exception
{
	const char* what() { return "Buffer is empty or uninitialized"; }
};

class InvalidUrl : public std::exception
{
	const char* what() { return "Invalid URI"; }
};

class EofReached
{
	const char* what() { return "Reading no longer possible"; }
};

}
#endif
