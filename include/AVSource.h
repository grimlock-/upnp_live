#ifndef AVSOURCE_H
#define AVSOURCE_H

#include <string>
#include <unistd.h>

namespace upnp_live {

enum AVSourceType { external, file, transcoder };

/*
 * Interface for any class that has an AV buffer other classes can read
 */
class AVSource
{
	public:
		virtual int Init() = 0;
		virtual void Shutdown() = 0;
		virtual bool IsInitialized() = 0;
		virtual std::string GetMimetype() = 0;
		AVSourceType SourceType;
};

}

#endif /* AVSOURCE_H */
