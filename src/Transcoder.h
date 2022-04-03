#ifndef TRANSCODER_H
#define TRANSCODER_H

#include <string>
#include <vector>
#include "AVHandler.h"
#include "AVWriter.h"
#include "ChildProcess.h"

namespace upnp_live {

class Transcoder : public AVWriter, ChildProcess
{
	public:
		Transcoder(std::vector<std::string>& args);
		~Transcoder();
		
		/*void Init();
		void Shutdown();
		bool IsInitialized();*/
		std::string GetMimetype();
		void SetInput(std::shared_ptr<AVHandler>& handler);
		//AVWriter
		int Read(char* buf, size_t len);
	private:
		std::string mimetype;
};

}

#endif /* TRANSCODER_H */
