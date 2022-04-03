#ifndef LOGGING_H
#define LOGGING_H
#include <string>
#include <mutex>
#include <atomic>
#include <iostream>
#include <fstream>

namespace upnp_live {

enum log_level { disabled = 0, always, error, warning, info, verbose, debug, verbose_debug };

class Logger
{
	public:
		Logger(std::ostream*, std::ostream*);
		static Logger* GetLogger();
		static void SetLogger(Logger* l);
		void SetLogLevel(log_level lv);
		log_level GetLogLevel();
		void Log(std::string& message);
		void Log(log_level lv, std::string& message);
		void Log(log_level lv, std::string&& message);
		void Log(log_level lv, const char* message);
		void Log_fmt(log_level lv, const char* message...);
	protected:
		std::mutex mutex;
		static Logger* _logger;
		std::atomic<log_level> currentLevel {error};
		std::ostream *out, *errout;
		//std::ofstream file;
};

}

#endif
