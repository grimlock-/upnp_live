#ifndef EXTERNALAVHANDLER_H
#define EXTERNALAVHANDLER_H

#include <string>
#include <vector>
#include <memory>
#include "AVHandler.h"
#include "AVWriter.h"
#include "ChildProcess.h"
#include "Transcoder.h"

namespace upnp_live {

/*
 * Offloads retrieval of AV data to a sub-process
 */
class ExternalAVHandler : public AVHandler, ChildProcess
{
	public:
		ExternalAVHandler(std::vector<std::string>& args);
		~ExternalAVHandler();
		int GetOutputFd();
		//AVHandler
		void Init();
		void Shutdown();
		bool IsInitialized();
		std::string GetMimetype();
		void SetWriteDestination(std::shared_ptr<Transcoder>& transcoder);
		//AVWriter
		int Read(char* buf, size_t len);
	private:
		bool blocking_output {false};
};

}

#endif
