#include <string>
#include <sstream>
#include <iostream>
#include <cstdarg>
#include "Logging.h"
using namespace upnp_live;

Logger* Logger::_logger;

Logger::Logger(std::ostream* o = nullptr, std::ostream* e = nullptr) : out(o), errout(e)
{
	if(!o)
		out = std::addressof(std::cout);
	if(!e)
		errout = std::addressof(std::cerr);
}

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
void Logger::Log_fmt(log_level lv, const char* message...)
{
	if(lv > currentLevel.load() || currentLevel.load() == disabled)
		return;

	std::string msg = message;
	std::stringstream full_message;
	size_t pos1 = 0;
	auto pos2 = msg.find('%');

	va_list args;
	va_start(args, message);

	while(pos2 != std::string::npos && pos2 < msg.size())
	{
		full_message << msg.substr(pos1, pos2-pos1);
		switch(msg[pos2+1])
		{
			case 'd':
				full_message << va_arg(args, int);
			break;

			case 's':
				full_message << va_arg(args, const char*);
			break;

			case '%':
				full_message << '%';
			break;

			default:
			break;
		}
		pos1 = pos2+2;
		pos2 = msg.find('%', pos1);
	}
	if(pos1 < msg.size())
		full_message << msg.substr(pos1);

	va_end(args);

	{
		std::lock_guard<std::mutex> guard(mutex);
		if(lv == error)
			*errout << full_message.str();
		else
			*out << full_message.str();
	}
}

