CFLAGS = -Wall -std=c++11 -pthread -Iinclude
LARGS = -pthread
LIBS = -lupnp -lixml -lpthread
CC = g++
OBJECTS = main.o Server.o Exceptions.o ContentDirectoryService.o ConnectionManagerService.o ContentContainer.o ContentItem.o ContentObject.o Stream.o StreamHandler.o util.o Config.o

build_bin: CFLAGS += -O3
build_bin: build_objs upnp_live
build_objs: $(OBJECTS)


upnp_live : $(OBJECTS)
	$(CC) $(LARGS) $(OBJECTS) $(LIBS) -o upnp_live

ConnectionManagerService.o : src/ConnectionManagerService.cpp include/ConnectionManagerService.h
	$(CC) -c $(CFLAGS) src/ConnectionManagerService.cpp -o ConnectionManagerService.o

ContentContainer.o : src/ContentContainer.cpp include/ContentContainer.h
	$(CC) -c $(CFLAGS) src/ContentContainer.cpp -o ContentContainer.o

ContentDirectoryService.o : src/ContentDirectoryService.cpp include/ContentDirectoryService.h
	$(CC) -c $(CFLAGS) src/ContentDirectoryService.cpp -o ContentDirectoryService.o

ContentItem.o : src/ContentItem.cpp include/ContentItem.h
	$(CC) -c $(CFLAGS) src/ContentItem.cpp -o ContentItem.o

ContentObject.o : src/ContentObject.cpp include/ContentObject.h
	$(CC) -c $(CFLAGS) src/ContentObject.cpp -o ContentObject.o

Exceptions.o : src/Exceptions.cpp include/Exceptions.h
	$(CC) -c $(CFLAGS) src/Exceptions.cpp -o Exceptions.o

Server.o : src/Server.cpp include/Server.h include/InitOptions.h
	$(CC) -c $(CFLAGS) src/Server.cpp -o Server.o

Stream.o : src/Stream.cpp include/Stream.h
	$(CC) -c $(CFLAGS) src/Stream.cpp -o Stream.o

StreamHandler.o : src/StreamHandler.cpp include/StreamHandler.h
	$(CC) -c $(CFLAGS) src/StreamHandler.cpp -o StreamHandler.o

main.o : src/main.cpp include/InitOptions.h
	$(CC) -c $(CFLAGS) src/main.cpp -o main.o

util.o : src/util.cpp include/util.h
	$(CC) -c $(CFLAGS) src/util.cpp -o util.o

Config.o : src/Config.cpp include/Config.h include/Version.h include/InitOptions.h
	$(CC) -c $(CFLAGS) src/Config.cpp -o Config.o

debug: CFLAGS += -g -DUPNP_LIVE_DEBUG
debug: LARGS += -g -rdynamic
debug: build_bin

debug-opt: CFLAGS += -Og
debug-opt: debug

.PHONY : clean
clean :
	rm *.o
	rm upnp_live
