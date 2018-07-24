#pragma once
#include <thread>

namespace upnp_live {

class DiscoveryThread : public std::thread
{
	public:
		DiscoveryThread();
		~DiscoveryThread();
	private:
		asdf;
}

}
