TODO
=============
## Big ticket items (in approximate order of priority)
* Use autoconf or something instead of a manually edited makefile for compilation
* Fix the UPnP stuff (events like subscriptions and state var requests, the ConnectionManagerService, etc.)
* Web interface (with compiler flag)
* Tests
* Unicode
* Valgrind?
* Doxygen comments?
* Checklist for v1.0
  * autoconf or whatever allows the basic ./configure && make && make install process
  * log file for running as daemon
  * Web interface
  * Add directory contents, copying the directory layout in the CDS

## AV Handlers
* Listener sources so clients like OBS or ffmpeg can stream to the server
* DASHHandler
  *Allow changing quality mid-stream and constant quality like streamlink
* HLSHandler
  *Allow changing quality mid-stream and constant quality like streamlink
* HttpHandler (could this be done with wget or curl via ext?)
* RTP/RTSP/RTMP Handlers?

## AV Stores
* Add FileStore for these use cases
  * Archive streams
  * Enable seeking while a stream is live?
* MemoryStore
  * Better handling for a source that generates AV data faster than real-time (i.e. transcoding a file)

## Code Thoughts
* strerror apparently [isn't thread-safe](https://en.wikipedia.org/wiki/C_standard_library#Threading_problems,_vulnerability_to_race_conditions), so consider using system_error exceptions instead
* Allow increasing pipe size for transcoding? Might fix the constant buffering while transcoding (assuming that isn't just my hardware, which it shouldn't be considering x264 is on ultrafast)
* I sprinkled const in some places, but there's room for improvement, same for noexcept and constexpr
* There's a number of inconsistencies regarding code style that should be fixed. Some functions have only one return statement while others have multiple, treating mimetype as both one word and two, single letter variable names in some places but purposefuly not in other similar ones, copy initialization with = in some places and brace initialization elsewhere, abbreviating all letters in an acronym vs just the first, etc.
* Should probably just use libfmt for logging stuff instead of variadic args. Even stringstream would be better since it's much less likely to segfault
* The Server class is quite the monolith. It should probably be broken up. Maybe start by not putting all the XML document validation in the constructor
* The process of updating the CDS to reflect the status of the streams needs to be cleaned up. It's the last thing I did in this commit, so it's a bit sloppy. The CDS originally had no dependencies on other classes and no heartbeat, but since I'm just trying to wrap everything up I ended up adding both. The heartbeat makes the status update in the stream heartbeat seem redundant, and if I take the status update out of the Stream heartbeat I feel like I ought to remove the AVHandler timeout checking as well, maybe stick that in the MemoryStore so that it **completely** controls their lifetime (it would certainly scale a shit ton better than having a thread for every Stream object).
* Thread pool for the stream statuses?

## Other
* Client handle timeout for clients that don't close them (i.e. MPC-HC, VLC, web browser)
* Initial wait times for stream heartbeats should be randomized so the processes don't all get started together
* Stream-specific handler timeout and status interval config options
* Output to log file for running as daemon
* Allow art/image for content items
* Add config option for specifying how full a stream buffer needs to be before allowing clients to read
* Add code to create required XML elements not in the description document
* Remove VirtualFileHandle struct since I just let pupnp handle plain file serving
* Add config option for upnp broadcast interval
* Allow (only?) words when specifying log level
* Allow customization of content directory structure
* Clean up Server::loadXml
* When stopping the process, if a status handler is running then it will recieve the signal and python will barf all over the console
