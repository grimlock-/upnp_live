#ifndef FILEAVHANDLER_H
#define FILEAVHANDLER_H

#include <string>
#include <atomic>
#include <mutex>
#include "AVSource.h"

namespace upnp_live {

/*
 * Loads AV data from file
 */
class FileAVHandler : public AVSource
{
	public:
		FileAVHandler(const std::string& path);
		~FileAVHandler();
		const std::string FilePath;
		int fd {0};
		//AVSource
		int Init();
		void Shutdown();
		bool IsInitialized();
		std::string GetMimetype();
	private:
		std::mutex fd_mutex;
};

}

#endif /* FILEAVHANDLER_H */
