#ifndef INITOPTIONS_H
#define INITOPTIONS_H

#include <string>
#include <vector>
#include <upnp/upnp.h>

namespace upnp_live {

struct StreamInitOptions
{
	std::string name, status_handler, status_args, av_handler, av_args, transcoder, mime_type;
	int32_t buffer_size = 0;
};

struct FileOptions
{
	std::string name, mime_type, path;
};

struct DirectoryOptions
{
	std::string path;
};

struct InitOptions 
{
	std::string web_root = "www";
	std::string config_file = "upnp_live.conf";
	std::string log_file = "upnp_live.log";
	std::string device_description = "RootDevice.xml";
	std::string friendly_name = "upnp_live";
	std::string address, interface;
	std::vector<StreamInitOptions> streams;
	std::vector<FileOptions> files;
	std::vector<DirectoryOptions> directories;
	int port = 0;
	int log_level = 4;
	bool daemon = false;
	Upnp_FunPtr event_callback;
};

}

#endif /* INITOPTIONS_H */
