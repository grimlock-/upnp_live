#ifndef FILEAVHANDLER_H
#define FILEAVHANDLER_H

#include <string>
#include <atomic>
#include <mutex>
#include "AVHandler.h"

namespace upnp_live {

/*
 * Loads AV data from file
 */
class FileAVHandler : public AVHandler
{
	public:
		FileAVHandler(const std::string& path);
		~FileAVHandler();
		//AVHandler
		void Init();
		void Shutdown();
		bool IsInitialized();
		std::string GetMimetype();
		void SetWriteDestination(std::shared_ptr<Transcoder>& transcoder);
		//AVWriter
		int Read(char* buf, size_t len);
	private:
		int fd {0};
		std::mutex fd_mutex;
		const std::string filepath;
};

}

#endif /* FILEAVHANDLER_H */
