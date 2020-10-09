#pragma once
namespace upnp_live {

/*
 * Interface for handling the retrieval of remote stream information
 */
class StatusHandler
{
	public:
		virtual bool GetStatus() = 0;
};

}
