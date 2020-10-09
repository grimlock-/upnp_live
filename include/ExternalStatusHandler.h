#ifndef EXTERNALSTATUSHANDLER_H
#define EXTERNALSTATUSHANDLER_H

#include <string>
#include <vector>
#include "StatusHandler.h"
#include "ChildProcess.h"

namespace upnp_live {
	
/*
 * Use external program to get stream status.
 */
class ExternalStatusHandler : public StatusHandler, ChildProcess
{
	public:
		ExternalStatusHandler(std::vector<std::string>& args);
		bool GetStatus();
};

}

#endif /* EXTERNALSTATUSHANDLER_H */
