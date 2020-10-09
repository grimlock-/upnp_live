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
	StreamHandle(std::size_t size, int filedescriptor, std::shared_ptr<Stream>& s) : fd(filedescriptor), len(size), buf_start(new char[size]), stream_obj(s)
	{
		if(fd == 0)
			throw std::invalid_argument("invalid file descriptor");
		if(len == 0)
			throw std::invalid_argument("invalid buffer size");
		if(auto str = stream_obj.lock())
			stream_name = str->Name;
		else
			throw std::invalid_argument("invalid stream object");
	}
	~StreamHandle()
	{
		delete[] buf_start;
	}
	std::string stream_name;
	const int fd;
	const std::size_t len;
	char * const buf_start;
	int32_t tail = -1;
	std::shared_timed_mutex rw_mutex;
	std::weak_ptr<Stream> stream_obj;
};

struct ClientHandle
{
	ClientHandle(std::shared_ptr<StreamHandle>& hnd, unsigned int _id) : stream_handle(hnd), id(_id)
	{
		std::shared_lock<std::shared_timed_mutex> rw_lock(hnd->rw_mutex);
		head = hnd->tail;
		stream_name = hnd->stream_name;
	}
	bool operator ==(const ClientHandle& other)
	{
		return other.id == id;
	}
	std::weak_ptr<StreamHandle> stream_handle;
	int32_t head;
	unsigned int id;
	std::string stream_name;
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
		//AVStore
		void Shutdown();
		UpnpWebFileHandle CreateHandle(std::shared_ptr<Stream>& stream);
		int DestroyHandle(UpnpWebFileHandle hnd);
		int Read(UpnpWebFileHandle hnd, char* buf, std::size_t len);
		void WriteToBuffer(std::shared_ptr<StreamHandle>& streamHandle);
		unsigned int GetId();
	private:
		std::thread heartbeatThread;
		std::atomic_flag alive = ATOMIC_FLAG_INIT;
		std::unordered_map<std::string, std::shared_ptr<StreamHandle>> streamHandles;
		std::shared_timed_mutex streamContainerMutex;
		std::multimap<std::string, ClientHandle> clientHandles;
		std::shared_timed_mutex clientContainerMutex;
		unsigned int uid = 0;
		Logger* logger;
};

}

#endif /* MEMORYSTORE_H */
