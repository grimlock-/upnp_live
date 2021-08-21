#ifndef LOGGING_H
#define LOGGING_H
#include <string>
#include <mutex>
#include <atomic>
#include <fstream>

namespace upnp_live {

enum log_level { disabled = 0, always, error, warning, info, verbose, debug, debig_chungus };

class Logger
{
	public:
		//Logger(std::string filepath);
		static Logger* GetLogger();
		static void SetLogger(Logger* l);
		void SetLogLevel(log_level lv);
		log_level GetLogLevel();
		void Log(std::string& message);
		void Log(log_level lv, std::string& message);
		void Log(log_level lv, std::string&& message);
		void Log(log_level lv, const char* message);
		void Log_cc(log_level lv, std::size_t len...);
	protected:
		std::mutex mutex;
		static Logger* _logger;
		std::atomic<log_level> currentLevel {error};
		//std::ofstream file;
};

}

#endif
