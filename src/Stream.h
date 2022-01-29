#ifndef STREAM_H
#define STREAM_H

#include <atomic>
#include <string>
#include <thread>
#include <memory>
#include <utility>
#include <chrono>
#include "AVSource.h"
#include "StatusHandler.h"
#include "Transcoder.h"
#include "Logging.h"

namespace upnp_live {

enum AddStreamResult { success, fail, heartbeatFail };

class Stream
{
	public:
		Stream(
			std::string name,
			std::string mt,
			std::unique_ptr<AVSource>&& avh,
			std::unique_ptr<StatusHandler>&& sh,
			std::unique_ptr<Transcoder>&& tr
		);
		virtual ~Stream();
		
		void Heartbeat();
		void UpdateStatus();
		
		void StartHeartbeat();
		int StartAVHandler();
		void StopAVHandler();
		void StopHeartbeat();
		void SetStatusInterval(int);
		
		bool IsLive();
		int GetChildCount();
		int GetStatusInterval();
		bool HasTranscoder();
		//Other classes can simplify stuff for non-transcoded file sources
		std::string GetFilePath();
		void UpdateReadTime();
		std::string GetMimeType();
		
		//Vars
		const std::string Name;
		int32_t BufferSize = 14400000; //Buffer size for avstore
	
	protected:
		const std::string mimeType;
		//std::vector<std::string> resolutions;
		std::atomic<bool> streamIsLive {false};
		std::unique_ptr<AVSource> avHandler;
		std::unique_ptr<StatusHandler> statusHandler;
		std::unique_ptr<Transcoder> transcoder;
		std::thread heartbeatThread;
		std::atomic<bool> heartbeatRunning {false};
		//std::atomic<int> statusInterval {300}; //in seconds
		std::atomic<int> statusInterval {30}; //in seconds
		//AV Handler is shutdown after the avstore stops reading data
		std::atomic<int> handlerTimeout {10}; //in seconds 0=no timeout
		std::chrono::time_point<std::chrono::steady_clock> lastRead;
		std::mutex readTimeMutex;
		std::chrono::time_point<std::chrono::steady_clock> lastStatus;
		Logger* logger;
};

}

#endif /* STREAM_H */
