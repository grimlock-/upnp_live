#ifndef HTTPAVHANDLER_H 
#define HTTPAVHANDLER_H

#include <string>
#include "AVSource.h"

namespace upnp_live {

/*
 * Gets AV data by reading from an HTTP connection
 */
class HttpAVHandler : public AVSource
{
	public:
		HttpAVHandler();
		//AVSource
		int Init();
		void Shutdown();
		bool IsInitialized();
		std::string GetMimetype();
};

}

#endif /* HTTPAVHANDLER_H */
