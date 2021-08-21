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

//Reads AV data from stdin and/or writes it to stdout
class ChildProcess
{
	public:
		ChildProcess(std::vector<std::string>& args);
		
		std::pair<int, pid_t> InitProcess();
		void ShutdownProcess();
		bool ProcessInitialized();
		
		
	protected:
		const std::string command;
		std::vector<std::string> arguments;
		std::mutex mutex;
		int inputPipe {0}; // Child process STDIN gets redirected to this
		int outputPipe {0};
		int writePipe {0}; // Used when manually writing to process fd
		pid_t processId {0};
		Logger* logger;
};

}

#endif /* CHILDPROCESS_H */
