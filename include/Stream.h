#pragma once
#include <atomic>
#include <vector>
#include <string>
#include <pthread.h>
#include "ContentContainer.h"

namespace upnp_live {
class Stream : public ContentContainer
{
	public:
		Stream(const char*);
		Stream(const Stream&); //Copy constructor must be manually defined (to be copied into a vector) on acount of the atomic variable
		~Stream();
		bool isLive();
		void addResolution(std::string);
		void addResolutions(std::vector<std::string>);
		void removeResolution(std::string);
		void clearResolutions();
		void populateStreams();
		void setURL(const char*);
		void setBaseResURI(const char*);
		void setBaseResURI(std::string);
		std::string getURL();
		std::string getMetadataXML();
		std::string getChildrenXML(int requestedCount, unsigned int startingIndex, int& numReturned);
		std::string getChildXML(unsigned int);
		std::string getResURI(int index);
		int getChildCount();
	private:
		//pthread_mutex_t mutex;
		//std::atomic_flag stream_is_live = ATOMIC_FLAG_INIT;
		std::string url, baseResURI;
		std::vector<std::string> resolutions;
		std::atomic<bool> stream_is_live;
};
}
