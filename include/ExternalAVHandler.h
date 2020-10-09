#ifndef EXTERNALAVHANDLER_H
#define EXTERNALAVHANDLER_H

#include <string>
#include <vector>
#include "AVSource.h"
#include "ChildProcess.h"

namespace upnp_live {

/*
 * Offloads retrieval of AV data to a sub-process
 */
class ExternalAVHandler : public AVSource, ChildProcess
{
	public:
		ExternalAVHandler(std::vector<std::string>& args);
		~ExternalAVHandler();
		//AVSource
		int Init();
		void Shutdown() noexcept;
		std::string GetMimetype();
		bool IsInitialized();
};

}

#endif /* EXTERNALAVHANDLER_H */
