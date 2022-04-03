#ifndef AVHANDLER_H
#define AVHANDLER_H

#include <string>
#include <unistd.h>
#include "AVWriter.h"

namespace upnp_live {

class AVHandler : public AVWriter
{
	public:
		virtual void Init() = 0;
		virtual void Shutdown() = 0;
		virtual bool IsInitialized() = 0;
		virtual std::string GetMimetype() = 0;
		virtual void SetWriteDestination(int fd) = 0;
};

}

#endif
