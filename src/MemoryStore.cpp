#include <iostream>
#include <shared_mutex>
#include <sstream>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h> //memcpy
#include "MemoryStore.h"
#include "util.h"
#include "Stream.h"
#include "Exceptions.h"

using namespace upnp_live;

MemoryStore::MemoryStore() :
heartbeatThread(&MemoryStore::Heartbeat, this)
{
	alive.test_and_set();
	logger = Logger::GetLogger();
}
MemoryStore::~MemoryStore()
{
	Shutdown();
}

void MemoryStore::Shutdown() try
{	
	logger->Log(info, "Stopping MemoryStore...\n");
	alive.clear();
	if(heartbeatThread.joinable())
		heartbeatThread.join();
}
catch(std::exception& e)
{
	logger->Log_cc(error, 3, "Error stopping memory store\n", e.what(), "\n");
}

UpnpWebFileHandle MemoryStore::CreateHandle(std::shared_ptr<Stream>& stream)
{
	std::string streamName = stream->Name;
	std::size_t bufferSize = stream->BufferSize;
	
	std::unique_lock<std::shared_timed_mutex> guard1(streamContainerMutex, std::defer_lock);
	std::unique_lock<std::shared_timed_mutex> guard2(clientContainerMutex, std::defer_lock);
	std::lock(guard1, guard2);
	
	auto streamIt = streamHandles.find(streamName);
	if(streamIt == streamHandles.end())
	{
		logger->Log_cc(info, 5, "[MemoryStore] Creating StreamHandle for ", streamName.c_str(), " | Buffer size: ", std::to_string(bufferSize).c_str(), "\n");
		try
		{
			int fd = stream->StartAVHandler();
			fcntl(fd, F_SETFL, O_NONBLOCK);
			auto newHandle = std::make_shared<StreamHandle>(bufferSize, fd, stream);
			auto result = streamHandles.insert(std::make_pair(streamName, newHandle));
			if(result.second)
			{
				streamIt = result.first;
			}
			else
			{
				std::string s = "Error adding ";
				s += streamName;
				s += "handle to MemoryStore container\n";
				throw std::runtime_error(s);
			}
		}
		catch(AlreadyInitializedException& e)
		{
			throw std::runtime_error("Stream already initialized, but couldn't find handle");
		}
	}

	//FIXME - emplace() and std::pair's piecewise_construct were giving me trouble, so just use insert instead
	ClientHandle newHandle(streamIt->second, GetId());
	auto clientIt = clientHandles.insert(std::make_pair(streamName, newHandle));
	/*auto clientIt = clientHandles.emplace(std::make_pair(std::piecewise_construct,
						std::forward_as_tuple(streamName),
						std::forward_as_tuple(streamIt->second, GetId())));*/
	std::stringstream ss;
	ss << "[MemoryStore] Created client handle (" << clientIt->second.id << ") for " << streamName << "\n";
	logger->Log(info, ss.str());
	return reinterpret_cast<UpnpWebFileHandle>(&clientIt->second);
}

int MemoryStore::DestroyHandle(UpnpWebFileHandle hnd)
{
	auto clientHandle = reinterpret_cast<ClientHandle*>(hnd);
	logger->Log_cc(info, 3, "[MemoryStore] Closing client handle ", std::to_string(clientHandle->id).c_str(), "\n");
	auto range = clientHandles.equal_range(clientHandle->stream_name);
	for(auto it = range.first; std::distance(it, range.second) > 0; ++it)
	{
		if(it->second.id == clientHandle->id)
		{
			clientHandles.erase(it);
			return 0;
		}
	}
	throw std::runtime_error("Could not find client handle");
}

int MemoryStore::Read(UpnpWebFileHandle hnd, char* buf, std::size_t len)
{
	auto clientHandle = reinterpret_cast<ClientHandle*>(hnd);

	std::unique_lock<std::shared_timed_mutex> lock(streamContainerMutex);
	if(auto streamHandle = clientHandle->stream_handle.lock())
	{
		lock.unlock();
		if(auto stream = streamHandle->stream_obj.lock())
			stream->UpdateReadTime();

		std::shared_lock<std::shared_timed_mutex> rw_lock(streamHandle->rw_mutex);
		if(clientHandle->head == -1)
		{
			logger->Log_cc(debig_chungus, 3, "[MemoryStore] ", clientHandle->stream_name.c_str(), " buffer uninitialized\n");
			throw CantReadBuffer();
		}
		else if(clientHandle->head == streamHandle->tail)
		{
			logger->Log_cc(debig_chungus, 5, "Client handle (", std::to_string(clientHandle->id).c_str(), "): ", clientHandle->stream_name.c_str(), " buffer empty\n");
			throw CantReadBuffer();
		}

		logger->Log_cc(debig_chungus, 5, "Client handle (", std::to_string(clientHandle->id).c_str(), ") requesting ", std::to_string(len).c_str(), " bytes\n");
		
		int bytes_written = 0;
		char* tBuf = buf;
		std::size_t head, target;
		std::size_t tail = streamHandle->tail;
		while(len > 0 && clientHandle->head != streamHandle->tail)
		{
			head = clientHandle->head;
			//Target = one past last byte to read
			//start at the lesser of all requested bytes and buffer end
			target = head+len > streamHandle->len ? streamHandle->len : head+len;
			
			//Don't read tail or past it
			if(tail > head && tail < target)
				target = tail;
			
			std::size_t num_bytes = target - head;
			memcpy(tBuf, &streamHandle->buf_start[head], num_bytes);
			tBuf += num_bytes;
			bytes_written += num_bytes;
			len -= num_bytes;
			clientHandle->head = (clientHandle->head + num_bytes) % streamHandle->len;
		}
		
		return bytes_written;
	}
	else
	{
		std::stringstream ss;
		ss << "StreamHandle for " << clientHandle->id << " has been invalidated";
		throw std::runtime_error(ss.str());
	}
}

/*
 * Thread for periodically copying AV data to stream handle buffers
 */
void MemoryStore::Heartbeat()
{
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	while(alive.test_and_set())
	{
		logger->Log(debig_chungus, "[MemoryStore] Heartbeat\n");
		try
		{
			//Copy streamhandle pointers to array
			std::unique_lock<std::shared_timed_mutex> lock(streamContainerMutex);
			std::pair<std::string, std::shared_ptr<StreamHandle>> t_streams[streamHandles.size()];
			int count = -1;
			for(auto& streamIt : streamHandles)
			{
				t_streams[++count].first = streamIt.first;
				t_streams[count].second = streamIt.second;
			}
			//release mutex since the new shared pointers will keep the structs alive even if they're removed from the container
			lock.unlock();
			for(auto& hnd : t_streams)
			{
				try
				{
					WriteToBuffer(hnd.second);
				}
				catch(std::exception& e)
				{
					logger->Log_cc(error, 5, "[MemoryStore] Error filling buffer for ", hnd.first.c_str(), "\n", e.what(), "\n");
				}
			}
		}
		catch(std::exception& e)
		{
			logger->Log_cc(error, 3, "[MemoryStore] heartbeat error: ", e.what(), "\n");
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

void MemoryStore::WriteToBuffer(std::shared_ptr<StreamHandle>& streamHandle)
{
	std::unique_lock<std::shared_timed_mutex> rw_lock(streamHandle->rw_mutex, std::defer_lock);
	std::unique_lock<std::shared_timed_mutex> container_lock(clientContainerMutex, std::defer_lock);
	std::lock(rw_lock, container_lock);

	const std::size_t len = streamHandle->len;
	const int32_t start = (streamHandle->tail+1) % len;
	const std::size_t num_bytes = len - start;

	//write to buffer end
	int bytes_written = read(streamHandle->fd, &streamHandle->buf_start[start], num_bytes);
	int err = errno;
	
	switch(bytes_written)
	{
		case -1:
		{
			if(err == EAGAIN)
				return;
			if(err == EBADF || err == EINVAL)
			{
				logger->Log_cc(error, 3, "[MemoryStore] ", streamHandle->stream_name.c_str(), ": Bad file descriptor, stopping stream...\n");
				if(auto stream = streamHandle->stream_obj.lock())
					stream->StopAVHandler();
				streamHandles.erase(streamHandle->stream_name);
			}
			throw std::system_error(err, std::generic_category());
		}
		case 0:
			return;
	}
	logger->Log_cc(debig_chungus, 5, "[MemoryStore] ", streamHandle->stream_name.c_str(), ": ", std::to_string(bytes_written).c_str(), " bytes written\n");
	
	//Move any invalidated heads
	int32_t next_tail = (streamHandle->tail + bytes_written) % len;
	auto range = clientHandles.equal_range(streamHandle->stream_name);
	for(auto it = range.first; std::distance(it, range.second) > 0; ++it)
	{
		if(it->second.head == -1)
			it->second.head = 0;
		else if(it->second.head >= start && it->second.head <= next_tail)
			it->second.head = (next_tail+1) % len;
	}

	//Move tail
	streamHandle->tail = next_tail;
}

unsigned int MemoryStore::GetId()
{
	return ++uid;
}
