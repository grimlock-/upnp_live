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

MemoryStore::MemoryStore() : heartbeatThread(&MemoryStore::Heartbeat, this)
{
	alive.test_and_set();
	logger = Logger::GetLogger();
}
MemoryStore::~MemoryStore()
{
	Shutdown();
}

void MemoryStore::Shutdown()
{	
	logger->Log(info, "Stopping MemoryStore...\n");
	alive.clear();
	if(heartbeatThread.joinable())
	{
		try
		{
			heartbeatThread.join();
		}
		catch(std::system_error& e)
		{
			logger->Log_fmt(error, "Error joining thread: %s\n", e.what());
		}
	}
}
/**
 * Assumes mutex for stream handles is locked
 */
std::shared_ptr<StreamHandle> MemoryStore::CreateStreamHandle(std::shared_ptr<Stream>& stream)
{
	std::string streamName = stream->Name;
	std::size_t bufferSize = stream->BufferSize;
	logger->Log_fmt(info, "[MemoryStore] Creating StreamHandle for %s | Buffer size: %d\n", streamName.c_str(), bufferSize);
	try
	{
		stream->Start();
	}
	catch(std::runtime_error& e)
	{
		logger->Log_fmt(error, "Error starting stream\n%s\n", e.what());
		throw std::runtime_error("Couldn't start stream");
	}

	auto newHandle = std::make_shared<StreamHandle>(bufferSize, stream);
	auto pair = streamHandles.insert(std::make_pair(streamName, newHandle));
	if(!pair.second)
		throw std::runtime_error("Couldn't add stream handle to container");
	return pair.first->second;
}
UpnpWebFileHandle MemoryStore::CreateHandle(std::shared_ptr<Stream>& stream)
{
	/*These two lock sections were originally combined with std::defer_lock to negate
	the possibility of the AV handler timing out while waiting to lock the client
	handle mutex, but with a timeout of 10 seconds and the infrequency of client
	container modification this isn't really a concern*/
	std::shared_ptr<StreamHandle> stream_handle;

	{
		std::unique_lock<std::shared_timed_mutex> guard(streamContainerMutex);
		auto streamIt = streamHandles.find(stream->Name);
		if(streamIt != streamHandles.end())
			stream_handle = streamIt->second;
		else
			stream_handle = CreateStreamHandle(stream);
	}

	auto newid = GetId();
	UpnpWebFileHandle ret = nullptr;
	ClientHandle newHandle(stream_handle, newid);

	{
		std::unique_lock<std::shared_timed_mutex> guard(clientContainerMutex);
		auto clientIt = clientHandles.insert(std::make_pair(stream_handle->stream_name, newHandle));
		ret = reinterpret_cast<UpnpWebFileHandle>(&clientIt->second);
	}

	if(ret)
		logger->Log_fmt(info, "[MemoryStore] Created client handle (%d) for %s\n", newid, stream->Name.c_str());
	else
		logger->Log_fmt(error, "[MemoryStore] Failed to create client handle for %s\n", stream->Name.c_str());
	return ret;
}

void MemoryStore::DestroyHandle(UpnpWebFileHandle hnd)
{
	auto clientHandle = reinterpret_cast<ClientHandle*>(hnd);
	logger->Log_fmt(info, "[MemoryStore] Closing client %d\n", clientHandle->id);

	std::string stream_name = clientHandle->stream_name;
	int count {0};

	{
		std::unique_lock<std::shared_timed_mutex> guard(clientContainerMutex);
		auto range = clientHandles.equal_range(clientHandle->stream_name);
		for(auto it = range.first; std::distance(it, range.second) > 0; ++it)
		{
			if(it->second.id == clientHandle->id)
			{
				clientHandles.erase(it);
			}
		}

		auto range2 = clientHandles.equal_range(clientHandle->stream_name);
		count = std::distance(range2.first, range2.second);
	}

	if(count == 0)
		RemoveStreamHandle(stream_name);
}

void MemoryStore::RemoveStreamHandle(std::string stream_name)
{
	std::unique_lock<std::shared_timed_mutex> guard(streamContainerMutex);
	streamHandles.erase(stream_name);
}

int MemoryStore::Read(UpnpWebFileHandle hnd, char* buf, std::size_t len)
{
	auto clientHandle = reinterpret_cast<ClientHandle*>(hnd);
	auto streamHandle = clientHandle->stream_handle;

	if(auto stream = streamHandle->stream_obj.lock())
		stream->UpdateReadTime();

	std::shared_lock<std::shared_timed_mutex> rw_lock(streamHandle->rw_mutex);
	if(clientHandle->head == -1)
	{
		logger->Log_fmt(verbose_debug, "[MemoryStore] Client %d: %s buffer uninitialized\n", clientHandle->id, clientHandle->stream_name.c_str());
		throw CantReadBuffer();
	}
	else if(clientHandle->head == streamHandle->tail)
	{
		if(streamHandle->finished)
		{
			logger->Log_fmt(verbose_debug, "[MemoryStore] Client %d: Finished reading stream\n", clientHandle->id, clientHandle->stream_name.c_str());
			return 0;
		}
		else
		{
			logger->Log_fmt(verbose_debug, "[MemoryStore] Client %d: %s buffer empty\n", clientHandle->id, clientHandle->stream_name.c_str());
			throw CantReadBuffer();
		}
	}

	logger->Log_fmt(verbose_debug, "[MemoryStore] Client %d requested %d bytes\n", clientHandle->id, len);
	
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

/*
 * Thread for periodically copying AV data to stream handle buffers
 */
void MemoryStore::Heartbeat()
{
	std::this_thread::sleep_for(std::chrono::milliseconds(5));
	while(alive.test_and_set())
	{
		logger->Log(verbose_debug, "[MemoryStore] Heartbeat tick\n");

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
			if(hnd.second->finished)
			{
				logger->Log_fmt(verbose_debug, "[MemoryStore] Skipping finished stream %s\n", hnd.first.c_str());
				continue;
			}
			try
			{
				GetAVData(hnd.second);
			}
			catch(const EofReached& e)
			{
				logger->Log_fmt(info, "[MemoryStore] %s: EOF reached\n", hnd.first.c_str());
				hnd.second->finished = true;
				RemoveStreamHandle(hnd.first);
			}
			catch(const std::system_error& e)
			{
				logger->Log_fmt(error, "[MemoryStore] %s: Error filling buffer\n%s\n", hnd.first.c_str(), e.what());
				hnd.second->finished = true;
				RemoveStreamHandle(hnd.first);
			}
			catch(const std::runtime_error& e)
			{
				logger->Log_fmt(error, "[MemoryStore] %s: Error filling buffer\n%s\n", hnd.first.c_str(), e.what());
				hnd.second->finished = true;
				RemoveStreamHandle(hnd.first);
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

void MemoryStore::GetAVData(std::shared_ptr<StreamHandle>& streamHandle)
{
	std::unique_lock<std::shared_timed_mutex> rw_lock(streamHandle->rw_mutex, std::defer_lock);
	std::unique_lock<std::shared_timed_mutex> container_lock(clientContainerMutex, std::defer_lock);
	std::lock(rw_lock, container_lock);

	const std::size_t len = streamHandle->len;
	const int32_t start = (streamHandle->tail+1) % len;
	const std::size_t num_bytes = len - start;

	//write to buffer end
	int bytes_written = 0;
	if(auto stream = streamHandle->stream_obj.lock())
	{
		bytes_written = stream->Read(&streamHandle->buf_start[start], num_bytes);
	}
	else
	{
		throw std::runtime_error("Could not access stream object");
	}
	
	if(!bytes_written)
	{
		logger->Log_fmt(verbose_debug, "[MemoryStore] %s: No bytes to read\n", streamHandle->stream_name.c_str());
		return;
	}
	else
	{
		logger->Log_fmt(verbose_debug, "[MemoryStore] %s: %d bytes read\n", streamHandle->stream_name.c_str(), bytes_written);
	}
	
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
