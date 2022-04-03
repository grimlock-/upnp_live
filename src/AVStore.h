#ifndef AVSTORE_H
#define AVSTORE_H

#include <memory>
#include <upnp/upnp.h>

namespace upnp_live {

class Stream;

/*
 * Interface for any class that manages local AV data
 */
class AVStore
{
	public:
		virtual void Shutdown() = 0;
		virtual UpnpWebFileHandle CreateHandle(std::shared_ptr<Stream>& stream) = 0;
		virtual void DestroyHandle(UpnpWebFileHandle handle) = 0;
		virtual int Read(UpnpWebFileHandle handle, char* buf, std::size_t len) = 0;
};

}

#endif /* AVSTORE_H */
