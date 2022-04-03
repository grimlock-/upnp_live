#include <sstream>
#include <exception>
#include <unistd.h>
#include <errno.h>
#include <cstdlib>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include "DirectoryMonitor.h"
#include "util.h"
using namespace upnp_live;

DirectoryMonitor::DirectoryMonitor()
{
	logger = Logger::GetLogger();
	inHandle = inotify_init1(IN_NONBLOCK);
	if(inHandle == -1)
		throw std::system_error(errno, std::generic_category());
}
DirectoryMonitor::~DirectoryMonitor()
{
	for(auto& watch : watches)
	{
		inotify_rm_watch(inHandle, watch.first);
	}
	if(inHandle)
		close(inHandle);
}

void DirectoryMonitor::AddDirectory(std::string dir)
{
	int watch_id = inotify_add_watch(inHandle, dir.c_str(), IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_MOVE_SELF | IN_MOVED_FROM | IN_MOVED_TO | IN_DONT_FOLLOW | IN_ONLYDIR);
	if(watch_id == -1)
		throw std::system_error(errno, std::generic_category());

	if(logger->GetLogLevel() >= debug)
	{
		std::string str = "[DM] New watch (";
		str += watch_id;
		str += "): ";
		str += dir;
		str += "\n";
		logger->Log(str);
	}
	else if(logger->GetLogLevel() == verbose)
	{
		std::string str = "Monitoring directory: ";
		str += dir;
		str += "\n";
		logger->Log(str);
	}
	watches.insert(std::make_pair(watch_id, dir));
}

void DirectoryMonitor::RemoveDirectory(int watch_id)
{
	auto it = watches.find(watch_id);
	if(it != watches.end())
	{
		logger->Log_fmt(debug, "[DirectoryMonitor] Removed watch (%d): %s\n", watch_id, it->second.c_str());
		watches.erase(it);
	}
}

std::vector<DirectoryMonitorEvent> DirectoryMonitor::Update()
{
	std::vector<DirectoryMonitorEvent> ret;

	std::size_t available = 0;
	ioctl(inHandle, FIONREAD, &available);
	if(!available)
		return ret;

	char buffer[available];
	read(inHandle, buffer, available);

	//Read inofity events
	std::size_t offset = 0;
	while(offset < available)
	{
		struct inotify_event* event = reinterpret_cast<inotify_event*>(buffer+offset);

		//Sanity checks
		if(event->mask & IN_Q_OVERFLOW)
		{
			std::stringstream ss;
			ss << "inotify event queue overflow\n" << \
			 "{\n" << \
			"\twatch ID: " << event->wd << "\n" << \
			"\tmask:\n" << \
			"\t  CREATE: " << (event->mask & IN_CREATE) << "\n" << \
			"\t  DELETE: " << (event->mask & IN_DELETE) << "\n" << \
			"\t  DELETE_SELF: " << (event->mask & IN_DELETE_SELF) << "\n" << \
			"\t  MOVE_SELF: " << (event->mask & IN_MOVE_SELF) << "\n" << \
			"\t  MOVED_FROM: " << (event->mask & IN_MOVED_FROM) << "\n" << \
			"\t  MOVED_TO: " << (event->mask & IN_MOVED_TO) << "\n" << \
			"\t  IGNORED: " << (event->mask & IN_IGNORED) << "\n" << \
			"\t  ISDIR: " << (event->mask & IN_ISDIR) << "\n" << \
			"\tcookie: " << event->cookie << "\n" << \
			"\tlen: " << event->len << "\n" << \
			"\tname: " << (event->mask & (IN_DELETE_SELF & IN_MOVE_SELF) ? "N/A" : event->name) << "\n" << \
			"}\n";
			logger->Log(debug, ss.str());
			offset += sizeof(struct inotify_event) + event->len;
			continue;
		}
		auto val = watches.find(event->wd);
		if(val == watches.end())
		{
			std::stringstream ss;
			ss << "Invalid watch ID, skipping this event\n" << \
			 "{\n" << \
			"\twatch ID: " << event->wd << "\n" << \
			"\tmask:\n" << \
			"\t  CREATE: " << (event->mask & IN_CREATE) << "\n" << \
			"\t  DELETE: " << (event->mask & IN_DELETE) << "\n" << \
			"\t  DELETE_SELF: " << (event->mask & IN_DELETE_SELF) << "\n" << \
			"\t  MOVE_SELF: " << (event->mask & IN_MOVE_SELF) << "\n" << \
			"\t  MOVED_FROM: " << (event->mask & IN_MOVED_FROM) << "\n" << \
			"\t  MOVED_TO: " << (event->mask & IN_MOVED_TO) << "\n" << \
			"\t  IGNORED: " << (event->mask & IN_IGNORED) << "\n" << \
			"\t  ISDIR: " << (event->mask & IN_ISDIR) << "\n" << \
			"\tcookie: " << event->cookie << "\n" << \
			"\tlen: " << event->len << "\n" << \
			"\tname: " << (event->mask & (IN_DELETE_SELF & IN_MOVE_SELF) ? "N/A" : event->name) << "\n" << \
			"}\n";
			//silently ignore ignored messages, print anything else
			if(!(event->mask & IN_IGNORED))
				logger->Log(debug, ss.str());
			offset += sizeof(struct inotify_event) + event->len;
			continue;
		}

		//calc vars
		std::string filename, filepath;
		bool isDirectory = (event->mask & IN_ISDIR) > 0;
		bool creation = ((event->mask & (IN_CREATE | IN_MOVED_TO)) != 0);
		bool deletion = ((event->mask & (IN_DELETE | IN_MOVED_FROM)) != 0);
		if(event->len)
		{
			filename = event->name;
			filepath = val->second;
			filepath += "/";
			filepath += filename;
		}
		else
		{
			filepath = val->second;
			filename = filepath.substr(filepath.find_last_of("/")+1);
		}

		if(event->mask & IN_UNMOUNT)
		{
			logger->Log(debug, "[DM] File system unmounted\n");
			if(isDirectory)
			{
				RemoveDirectory(event->wd);
				DirectoryMonitorEvent e;
				e.type = dir_delete;
				e.path = filepath;
				ret.push_back(e);
			}
			else
			{
				DirectoryMonitorEvent e;
				e.type = file_delete;
				e.path = filepath;
				ret.push_back(e);
			}
		}
		else if(event->mask & (IN_DELETE_SELF | IN_MOVE_SELF))
		{
			//A dir_delete event is made in the parent's delete event instead of here
			//to account for instances where a directory is created and instantly deleted
			logger->Log_fmt(debug, "[DirectoryMonitor] watch invalidated for %s\n", filepath.c_str());
			RemoveDirectory(event->wd);
		}
		else if(creation)
		{

			if(isDirectory)
			{
				logger->Log_fmt(debug, "[DirectoryMonitor] new directory: %s\n", filepath.c_str());
				DirectoryMonitorEvent e;
				e.type = dir_create;
				e.path = filepath;
				ret.push_back(e);
			}
			else
			{
				logger->Log_fmt(debug, "[DirectoryMonitor] File created: %s\n", filepath.c_str());
				DirectoryMonitorEvent e;
				e.type = file_create;
				e.path = filepath;
				ret.push_back(e);
			}
		}
		else if(deletion)
		{
			if(isDirectory)
			{
				//This here will be the only way of making a delete event for a dir that's created and instantly deleted
				logger->Log_fmt(debug, "[DirectoryMonitor] directory deleted/renamed: %s\n", filepath.c_str());
				DirectoryMonitorEvent e;
				e.type = dir_delete;
				e.path = filepath;
				ret.push_back(e);
			}
			else
			{
				logger->Log_fmt(debug, "[DirectoryMonitor] file deleted/renamed: %s\n", filepath.c_str());
				DirectoryMonitorEvent e;
				e.type = file_delete;
				e.path = filepath;
				ret.push_back(e);
			}
		}

		offset += sizeof(struct inotify_event) + event->len;
	}

	FilterEvents(ret);

	//Add watches for new directories
	for(auto it = ret.begin(); it != ret.end(); ++it)
	{
		if(it->type == dir_create)
		{
			try
			{
				AddDirectory(it->path);
			}
			catch(std::system_error& e)
			{
				logger->Log_fmt(error, "Error creating watch for %s, the server will not react to filesystem chagnes in this directory\nError: %s\n", it->path.c_str(), e.what());
			}
		}
	}
	return ret;
}

//Remove files that were created and instantly deleted or vice versa
void DirectoryMonitor::FilterEvents(std::vector<DirectoryMonitorEvent>& events)
{
	std::vector<DirectoryMonitorEvent> filteredEvents;
	filterstart:
	for(auto it = events.begin(); it != events.end(); ++it)
	{
		if(it->type != file_create && it->type != file_delete)
			continue;
		for(auto it2 = it+1; it2 != events.end(); ++it2)
		{
			if(it->path != it2->path)
				continue;
			if(it->type == file_create && it2->type == file_delete ||
				it->type == file_delete && it2->type == file_create)
			{
				events.erase(it2);
				events.erase(it);
				goto filterstart;
			}
		}
	}
	return;
}
