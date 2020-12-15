#include <iostream>
#include <cstdarg>
#include "Logging.h"
using namespace upnp_live;

Logger* Logger::_logger;

Logger* Logger::GetLogger()
{
	return Logger::_logger;
}
void Logger::SetLogger(Logger* l)
{
	Logger::_logger = l;
}

void Logger::SetLogLevel(log_level lv)
{
	currentLevel = lv;
}
log_level Logger::GetLogLevel()
{
	return currentLevel;
}

void Logger::Log(std::string& message)
{
	Log(always, message.c_str());
}
void Logger::Log(log_level lv, std::string& message)
{
	Log(lv, message.c_str());
}
void Logger::Log(log_level lv, std::string&& message)
{
	Log(lv, message.c_str());
}
void Logger::Log(log_level lv, const char* message)
{
	if(lv > currentLevel.load() || currentLevel.load() == disabled)
		return;

	std::ostream* stream = std::addressof(std::cout);
	if(lv == error)
		stream = std::addressof(std::cerr);

	std::lock_guard<std::mutex> guard(mutex);
	*stream << message;
}
void Logger::Log_cc(log_level lv, std::size_t len...)
{
	if(lv > currentLevel.load() || currentLevel.load() == disabled)
		return;

	std::ostream* stream = std::addressof(std::cout);
	if(lv == error)
		stream = std::addressof(std::cerr);

	va_list args;
	va_start(args, len);

	{
		std::string message;
		std::lock_guard<std::mutex> guard(mutex);
		for(std::size_t i = 0; i < len; ++i)
			message += va_arg(args, const char*);
		*stream << message.c_str();
	}

	va_end(args);
}

