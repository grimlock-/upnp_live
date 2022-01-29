CFLAGS = -Wall -std=c++14 -pthread -D_FILE_OFFSET_BITS=64
LARGS = -pthread
LIBS = -lupnp -lixml -lpthread
CC = g++
OBJECTS = ChildProcess.o Config.o ConnectionManagerService.o ContentDirectoryService.o DirectoryMonitor.o ExternalAVHandler.o ExternalStatusHandler.o FileAVHandler.o HttpRequest.o HttpStatusHandler.o Logging.o main.o MemoryStore.o Server.o Stream.o Transcoder.o util.o

build_bin: CFLAGS += -O3
build_bin: build_objs upnp_live
build_objs: $(OBJECTS)

test : src/test.cpp src/util.cpp src/util.h src/HttpRequest.cpp src/HttpRequest.h src/HttpStatusHandler.cpp src/HttpStatusHandler.h src/StatusHandler.h
	g++ -std=c++14 -g src/util.cpp src/HttpStatusHandler.cpp src/HttpRequest.cpp src/test.cpp -lupnp -lixml -g -rdynamic -o test

upnp_live : $(OBJECTS)
	$(CC) $(LARGS) $(OBJECTS) $(LIBS) -o upnp_live

ChildProcess.o : src/ChildProcess.cpp src/ChildProcess.h
	$(CC) -c $(CFLAGS) src/ChildProcess.cpp -o ChildProcess.o

Config.o : src/Config.cpp src/Config.h src/Version.h src/InitOptions.h
	$(CC) -c $(CFLAGS) src/Config.cpp -o Config.o

ConnectionManagerService.o : src/ConnectionManagerService.cpp src/ConnectionManagerService.h
	$(CC) -c $(CFLAGS) src/ConnectionManagerService.cpp -o ConnectionManagerService.o

ContentDirectoryService.o : src/ContentDirectoryService.cpp src/ContentDirectoryService.h
	$(CC) -c $(CFLAGS) src/ContentDirectoryService.cpp -o ContentDirectoryService.o

DirectoryMonitor.o : src/DirectoryMonitor.cpp src/DirectoryMonitor.h
	$(CC) -c $(CFLAGS) src/DirectoryMonitor.cpp -o DirectoryMonitor.o
	
ExternalAVHandler.o : src/ExternalAVHandler.cpp src/ExternalAVHandler.h
	$(CC) -c $(CFLAGS) src/ExternalAVHandler.cpp -o ExternalAVHandler.o

ExternalStatusHandler.o : src/ExternalStatusHandler.cpp src/ExternalStatusHandler.h
	$(CC) -c $(CFLAGS) src/ExternalStatusHandler.cpp -o ExternalStatusHandler.o

FileAVHandler.o : src/FileAVHandler.cpp src/FileAVHandler.h
	$(CC) -c $(CFLAGS) src/FileAVHandler.cpp -o FileAVHandler.o

HttpRequest.o : src/HttpRequest.cpp src/HttpRequest.h
	$(CC) -c $(CFLAGS) src/HttpRequest.cpp -o HttpRequest.o

HttpStatusHandler.o : src/HttpStatusHandler.cpp src/HttpStatusHandler.h
	$(CC) -c $(CFLAGS) src/HttpStatusHandler.cpp -o HttpStatusHandler.o

Logging.o : src/Logging.cpp src/Logging.h
	$(CC) -c $(CFLAGS) src/Logging.cpp -o Logging.o

main.o : src/main.cpp src/InitOptions.h
	$(CC) -c $(CFLAGS) src/main.cpp -o main.o

MemoryStore.o : src/MemoryStore.cpp src/MemoryStore.h
	$(CC) -c $(CFLAGS) src/MemoryStore.cpp -o MemoryStore.o

Server.o : src/Server.cpp src/Server.h
	$(CC) -c $(CFLAGS) src/Server.cpp -o Server.o

Stream.o : src/Stream.cpp src/Stream.h
	$(CC) -c $(CFLAGS) src/Stream.cpp -o Stream.o

Transcoder.o : src/Transcoder.cpp src/Transcoder.h
	$(CC) -c $(CFLAGS) src/Transcoder.cpp -o Transcoder.o

util.o : src/util.cpp src/util.h
	$(CC) -c $(CFLAGS) src/util.cpp -o util.o


debug: CFLAGS += -g -DUPNP_LIVE_DEBUG
debug: LARGS += -g -rdynamic
debug: build_objs upnp_live

.PHONY : clean
clean :
	rm -f *.o
	rm -f upnp_live
	rm -f test
