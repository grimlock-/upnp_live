#ifndef STREAM_H
#define STREAM_H

#include <atomic>
#include <string>
#include <thread>
#include <memory>
#include <utility>
#include <chrono>
#include "AVHandler.h"
#include "StatusHandler.h"
#include "Transcoder.h"
#include "Logging.h"

namespace upnp_live {

enum AddStreamResult { success, fail, heartbeatFail };

class Stream : public AVWriter
{
	public:
		Stream(
			std::string name,
			std::string mt,
			std::unique_ptr<AVHandler>&& avh,
			std::unique_ptr<StatusHandler>&& sh,
			std::unique_ptr<Transcoder>&& tr
		);
		virtual ~Stream();
		
		void Heartbeat();
		void UpdateStatus();
		
		void StartHeartbeat();
		void Start();
		void Stop();
		void StopHeartbeat();
		void SetStatusInterval(int);
		
		bool IsLive();
		int GetChildCount();
		int GetStatusInterval();
		std::string GetFilePath();
		void UpdateReadTime();
		std::string GetMimeType();
		//AVWriter
		int Read(char* buf, size_t len);
		
		//Vars
		const std::string Name;
		int32_t BufferSize = 14400000; //Buffer size for avstore
	
	protected:
		const std::string mimeType;
		std::atomic<bool> streamIsLive {false};
		std::unique_ptr<AVHandler> avHandler;
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

#endif
