#ifndef HTTPAVHANDLER_H 
#define HTTPAVHANDLER_H

#include <string>
#include "AVHandler.h"

namespace upnp_live {

/*
 * Gets AV data by reading from an HTTP connection
 */
class HttpAVHandler : public AVHandler
{
	public:
		HttpAVHandler();
		//AVHandler
		void Init();
		void Shutdown();
		bool IsInitialized();
		std::string GetMimetype();
		void SetWriteDestination(int fd);
};

}

#endif
