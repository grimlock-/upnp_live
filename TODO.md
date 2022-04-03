TODO
=============
## Big ticket items (in approximate order of priority)
* Use autoconf or something instead of a manually edited makefile for compilation
* Fix the UPnP stuff (events like subscriptions and state var requests, the ConnectionManagerService, ContentDirectory updates, etc.)
* Web interface (with compiler flag)
* Actual tests
* Unicode
* Valgrind?
* Doxygen comments?
* Checklist for v1.0
  * autoconf or whatever allows the basic ./configure && make && make install process
  * log file for running as daemon
  * Web interface
  * Customizable CDS layout?

## AV Handlers
* Listener sources so clients like OBS or ffmpeg can stream to the server
  * Youtube's docs for [ingestion protocols](https://developers.google.com/youtube/v3/live/guides/ingestion-protocol-comparison) might be of interest
* DASH
  * Allow adaptive and constant quality like streamlink
* HLS
  * Allow adaptive and constant quality like streamlink
* Http
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
* The Server class is quite the monolith. It should probably be broken up. Maybe start by not putting all the XML document validation in the constructor
* The process of updating the CDS to reflect the status of the streams needs to be cleaned up, it's a bit sloppy. The CDS originally had no dependencies on other classes and no heartbeat, but I got tired of working on v0.2 and ended up adding both. The heartbeat makes the status update in the stream heartbeat seem redundant, and if I take the status update out of the Stream heartbeat I feel like I ought to remove the AVHandler timeout checking as well, maybe stick that in the MemoryStore so that it **completely** controls their lifetime (it would certainly scale a shit ton better than having a thread for every Stream object).
* I want reading AV data into MemoryStore to not be confined to one thread so that way blocking reads on one stream don't hold up another, but the program is already going pretty crazy with the threads, especially considering pupnp has its own thread pool
* The Directory Monitor is rather limited in scope right now. Currently, when creating event objects, it reduces state changes down to either deletion or creation and disregards editing and renaming. This limits the Filter function since that operates on a collection of custom event objects, so if it is to be expanded later on e.g. to disregard files that were renamed and instantly given the old name back, the Update function will need to be changed.

## Other
* Client handle timeout for clients that don't close them (i.e. MPC-HC, VLC, web browser)
* Initial wait times for stream heartbeats should be randomized so the processes don't all get started together
* Stream-specific handler timeout and status interval config options
* Output to log file for running as daemon
* Allow art/image for content items
* Add config option for specifying how full a stream buffer needs to be before allowing clients to read
* Add code to create required XML elements not in the description document
* Remove VirtualFileHandle struct since I just let pupnp handle plain file serving
* Allow (only?) words when specifying log level
* Allow customization of content directory structure
  * Copy filesystem directory structure to CDS for directories specified via config options
* Clean up Server::loadXml
* When stopping the process, if a status handler is running then it will recieve the signal and python will barf all over the console
* Use a library for inet stuff. [Here's](https://stackoverflow.com/questions/118945/best-c-c-network-library) a good list
