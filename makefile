CFLAGS = -Wall -std=c++14 -pthread -Iinclude -D_FILE_OFFSET_BITS=64
LARGS = -pthread
LIBS = -lupnp -lixml -lpthread
CC = g++
OBJECTS = ChildProcess.o Config.o ConnectionManagerService.o ContentDirectoryService.o DirectoryMonitor.o ExternalAVHandler.o ExternalStatusHandler.o FileAVHandler.o Logging.o main.o MemoryStore.o Server.o Stream.o Transcoder.o util.o

build_bin: CFLAGS += -O3
build_bin: build_objs upnp_live
build_objs: $(OBJECTS)


upnp_live : $(OBJECTS)
	$(CC) $(LARGS) $(OBJECTS) $(LIBS) -o upnp_live

ChildProcess.o : src/ChildProcess.cpp include/ChildProcess.h
	$(CC) -c $(CFLAGS) src/ChildProcess.cpp -o ChildProcess.o

Config.o : src/Config.cpp include/Config.h include/Version.h include/InitOptions.h
	$(CC) -c $(CFLAGS) src/Config.cpp -o Config.o

ConnectionManagerService.o : src/ConnectionManagerService.cpp include/ConnectionManagerService.h
	$(CC) -c $(CFLAGS) src/ConnectionManagerService.cpp -o ConnectionManagerService.o

ContentDirectoryService.o : src/ContentDirectoryService.cpp include/ContentDirectoryService.h
	$(CC) -c $(CFLAGS) src/ContentDirectoryService.cpp -o ContentDirectoryService.o

DirectoryMonitor.o : src/DirectoryMonitor.cpp include/DirectoryMonitor.h
	$(CC) -c $(CFLAGS) src/DirectoryMonitor.cpp -o DirectoryMonitor.o
	
ExternalAVHandler.o : src/ExternalAVHandler.cpp include/ExternalAVHandler.h
	$(CC) -c $(CFLAGS) src/ExternalAVHandler.cpp -o ExternalAVHandler.o

ExternalStatusHandler.o : src/ExternalStatusHandler.cpp include/ExternalStatusHandler.h
	$(CC) -c $(CFLAGS) src/ExternalStatusHandler.cpp -o ExternalStatusHandler.o

FileAVHandler.o : src/FileAVHandler.cpp include/FileAVHandler.h
	$(CC) -c $(CFLAGS) src/FileAVHandler.cpp -o FileAVHandler.o

Logging.o : src/Logging.cpp include/Logging.h
	$(CC) -c $(CFLAGS) src/Logging.cpp -o Logging.o

main.o : src/main.cpp include/InitOptions.h
	$(CC) -c $(CFLAGS) src/main.cpp -o main.o

MemoryStore.o : src/MemoryStore.cpp include/MemoryStore.h
	$(CC) -c $(CFLAGS) src/MemoryStore.cpp -o MemoryStore.o

Server.o : src/Server.cpp include/Server.h
	$(CC) -c $(CFLAGS) src/Server.cpp -o Server.o

Stream.o : src/Stream.cpp include/Stream.h
	$(CC) -c $(CFLAGS) src/Stream.cpp -o Stream.o

Transcoder.o : src/Transcoder.cpp include/Transcoder.h
	$(CC) -c $(CFLAGS) src/Transcoder.cpp -o Transcoder.o

util.o : src/util.cpp include/util.h
	$(CC) -c $(CFLAGS) src/util.cpp -o util.o


debug: CFLAGS += -g -DUPNP_LIVE_DEBUG
debug: LARGS += -g -rdynamic
debug: build_objs upnp_live

old_upnp: CFLAGS += -DOLD_UPNP
old_upnp: build_objs upnp_live

.PHONY : clean
clean :
	rm -f *.o
	rm -f upnp_live
