#ifndef DIRECTORYMONITOR_H
#define DIRECTORYMONITOR_H
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include "Logging.h"

namespace upnp_live {

enum directory_monitor_event {file_create = 0, file_delete, dir_create, dir_delete};

struct DirectoryMonitorEvent
{
	directory_monitor_event type;
	std::string path;
};

class DirectoryMonitor
{
        public:
                DirectoryMonitor();
                ~DirectoryMonitor();
		void AddDirectory(std::string dir);
		void RemoveDirectory(int watch_id);
		std::vector<DirectoryMonitorEvent> Update();
		void FilterEvents(std::vector<DirectoryMonitorEvent>& events);

	protected:
		int inHandle = 0;
		std::unordered_map<int, std::string> watches;
		Logger* logger;
};

}

#endif
