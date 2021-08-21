#pragma once
#include <upnp/upnp.h>
#include <unistd.h>
#include <vector>
#include <pthread.h>
#include "Server.h"
#include "Stream.h"
namespace upnp_live {

typedef struct OpenStreamHandle
{
	//OpenStreamHandle() : childProcess(0), stream(nullptr), quality(""), pipe(0) {}
	pid_t childProcess;
	//pthread_t readThread;
	Stream* stream;
	std::string quality;
	int pipe;
} OpenStreamHandle;

class StreamHandler
{
	public:
		StreamHandler();
		~StreamHandler();
		static void			streamHandlerInit();
		static int			getInfoCallback(const char* filename, UpnpFileInfo* info, const void* cookie);
		static UpnpWebFileHandle	openCallback(const char* filename, enum UpnpOpenFileMode mode, const void* cookie);
		static int			closeCallback(UpnpWebFileHandle fh, const void* cookie);
		static int			readCallback(UpnpWebFileHandle fh, char* buf, size_t len, const void* cookie);
		static int			seekCallback(UpnpWebFileHandle fh, long offset, int origin, const void* cookie);
		static int			writeCallback(UpnpWebFileHandle fh, char* buf, size_t len, const void* cookie);
		static void			setServer(Server*);
		static void			CloseAllStreams();
		//static void*			streamReadThread(void*);
	private:
		//Vars
		static std::vector<OpenStreamHandle> openStreams;
		static Server* server;
		//Functions
		static int			OpenStream(OpenStreamHandle*);
		static void			CloseStream(OpenStreamHandle*);
};

}
