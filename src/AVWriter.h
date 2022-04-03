#ifndef AVWRITER_H
#define AVWRITER_H

#include <string>
#include <unistd.h>

namespace upnp_live {

class AVWriter
{
	public:
		/*virtual void Init() = 0;
		virtual void Shutdown() = 0;
		virtual bool IsInitialized() = 0;*/

		//Must throw EofReached when reading is no longer possible
		virtual int Read(char* buf, size_t len) = 0;

		//virtual void SetWriteDestination(int fd) = 0;
		//const AVWriterType WriterType;
};

}

#endif
