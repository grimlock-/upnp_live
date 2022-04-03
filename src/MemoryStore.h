#ifndef MEMORYSTORE_H
#define MEMORYSTORE_H

#include <string>
#include <map>
#include <unordered_map>
#include <atomic>
#include <thread>
#include <memory>
#include <utility>
#include "AVStore.h"
#include "Stream.h"
#include "Logging.h"

namespace upnp_live {

struct StreamHandle
{
	StreamHandle(std::size_t size, std::shared_ptr<Stream>& s) : stream_name(s->Name), len(size), buf_start(new char[size]), stream_obj(s)
	{
		if(len == 0)
			throw std::invalid_argument("invalid buffer size");
		/*if(auto str = stream_obj.lock())
			stream_name = str->Name;
		else
			throw std::runtime_error("error  stream object");*/
	}
	~StreamHandle()
	{
		delete[] buf_start;
	}
	const std::string stream_name;
	const std::size_t len;
	char * const buf_start;
	int32_t tail = -1;
	std::shared_timed_mutex rw_mutex;
	std::weak_ptr<Stream> stream_obj;
	bool finished {false};
};

struct ClientHandle
{
	ClientHandle(std::shared_ptr<StreamHandle>& hnd, unsigned int _id) : id(_id), stream_name(hnd->stream_name), stream_handle(hnd)
	{
		std::shared_lock<std::shared_timed_mutex> rw_lock(hnd->rw_mutex);
		head = hnd->tail;
		//stream_name = hnd->stream_name;
	}
	bool operator ==(const ClientHandle& other)
	{
		return other.id == id;
	}
	const unsigned int id;
	const std::string stream_name;
	int32_t head;
	std::shared_ptr<StreamHandle> stream_handle;
	bool IsValid {true};
};

/*
 * Keeps AV data stored in constant sized ring buffers shared among clients
 * FIXME - Assumes sources generate data no faster than real-time, so writing
 * is not blocked with a full buffer, old data is just overwritten and
 * all invalidated heads are moved to the new front of the buffer
 */
class MemoryStore : public AVStore
{
	public:
		MemoryStore();
		~MemoryStore();
		void Heartbeat();
		void GetAVData(std::shared_ptr<StreamHandle>& streamHandle);
		unsigned int GetId();
		//AVStore
		void Shutdown();
		UpnpWebFileHandle CreateHandle(std::shared_ptr<Stream>& stream);
		void DestroyHandle(UpnpWebFileHandle hnd);
		int Read(UpnpWebFileHandle hnd, char* buf, std::size_t len);
	private:
		std::thread heartbeatThread;
		std::atomic_flag alive = ATOMIC_FLAG_INIT;
		std::unordered_map<std::string, std::shared_ptr<StreamHandle>> streamHandles;
		std::shared_timed_mutex streamContainerMutex;
		std::multimap<std::string, ClientHandle> clientHandles;
		std::shared_timed_mutex clientContainerMutex;
		unsigned int uid = 0;
		Logger* logger;
		//Functions
		std::shared_ptr<StreamHandle> CreateStreamHandle(std::shared_ptr<Stream>& stream);
		void DestroyStreamHandle(std::string stream_name);
};

}

#endif
