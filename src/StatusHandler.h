#ifndef STATUSHANDLER_H
#define STATUSHANDLER_H
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
#endif /* STATUSHANDLER_H */
