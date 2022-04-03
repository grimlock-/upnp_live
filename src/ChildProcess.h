#ifndef CHILDPROCESS_H
#define CHILDPROCESS_H

#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <utility>
#include <unistd.h>
#include "Logging.h"

namespace upnp_live {

class ChildProcess
{
	public:
		ChildProcess(std::vector<std::string>& args);
		
		std::pair<int, pid_t> InitProcess(bool blocking_out);
		void ShutdownProcess();
		bool ProcessInitialized();
		size_t Write(const char* buf, const size_t len);
		void SetInput(int fd);
		
	protected:
		const std::string command;
		std::vector<std::string> arguments;
		std::mutex mutex;
		int inputPipe {0}; // Child process STDIN gets redirected to this
		int outputPipe {0};
		int writePipe {0};
		pid_t processId {0};
		Logger* logger;
};

}

#endif
