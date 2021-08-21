#ifndef TRANSCODER_H
#define TRANSCODER_H

#include <string>
#include <vector>
#include "AVSource.h"
#include "ChildProcess.h"

namespace upnp_live {

class Transcoder : public AVSource, ChildProcess
{
	public:
		Transcoder(std::vector<std::string>& args);
		~Transcoder();
		
		void SetSource(int fd);
		//AVSource
		int Init();
		void Shutdown();
		std::string GetMimetype();
		bool IsInitialized();
		
	protected:
		int source_fd = 0;
};

}

#endif /* TRANSCODER_H */
