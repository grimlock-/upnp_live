upnp_live
=============

A UPnP MediaServer for mirroring live streams acquired via [StreamLink](https://github.com/streamlink/streamlink) (though it could theoretically work with any binary that can print a stream to stdout).

## Dependencies
[libupnp](http://sourceforge.net/projects/pupnp/files/pupnp/libUPnP%201.8.3/) v1.8.3

[streamlink](https://github.com/streamlink/streamlink) - not neded for compilation, but needed for server functionality

## Working Devices
(Only tested with streams from Twitch.tv) The server has been tested with and confirmed to work for the following media renderers:

#### Televisions
  * Sony Bravia XBR-55x850B (Software version PKG2.902AAA) 
    * only 720p and greater. Lower resolutions would not play

  * Roku TV 5305X (Insignia model NS-55DR420NA16, Software version 8.1.0 build 4140-12) - "Roku Media Player" Application
    * only 720p and lower resolutions. High framerate (60fps) streams would not play

The server will not function for both of the above devices at the same time. See bullet 1 in Known Bugs

#### Media Players
  * VLC (v2.2.4)
  * MPC-HC (64-bit v1.7.8)

## Contributing
I will accept pull requests in the future, but I would like to finish laying the ground work first (basically just the StreamHandler class). This current design is rather quick and dirty, so I want to minimize the amount of refactoring and fix that before any pull requests come in.

I would also greatly appreciate it if anyone can test upnp_live with any devices they have and provide details about which ones can successfully play streams from the server.

## TODO
* Valgrind
* Doxygen comments
* Use an autoconf configure.ac script instead of a manually edited makefile for compilation

#### Code Structure
* Implement generic singleton class to be inherited by the Server class (remove Stream::baseResURI related stuff when this one is done), the Service classes, and the StreamHandler class
* Fix the static mime types
  * Have to check and see if streamlink can return the mime type for a given URL
  * The extensions for the resource URIs will also be tied to this

###### StreamHandler
* Rework StreamHandler to be an actual class instead of just a collection of static functions
* Implement a heartbeat thread to regularly check if a stream is alive or not. If it isn't, clients should not see it when browsing the CDS
* Store stream AV data in a temporary file or buffer instead of just reading from the pipe to streamlink (and maybe do away with the virtual directory callbacks entirely)
* Also don't immediately delete said files whenever close() is called since clients tend to rapidly open and close a file if they have trouble reading them.
* Add pthread locks for the list of currently open streams to prevent race conditions
* Change execl in StreamHandler to a different function that will search the PATH environment variable for the binary

#### Features
* Centralize logging and make it abide by the log level setting
* Allow streams to be added to the Content Directory Service during runtime (must also add locks to prevent race conditions) from a web interface

#### Known Bugs
* When an open request is made for a stream that already has a OpenStreamHandle object, changing the StreamHandler::open callback to return a pointer to an already existing OpenStreamHandle (which is necessary for Roku TVs to play the stream) stops Sony Bravia TVs from being able to read any stream below 1080p60 quality. While Sony TVs can technically open the stream with this behavior, playback will stop after less than a second, and the one video frame that does get decoded is corrupted.
* Child processes executing Streamlink will intermittently throw an exception when receiving SIGTERM, which causes the shutdown process to stop until a SIGKILL is received (or two more SIGTERMs which will trigger a call to exit()).
* Occasionally, multiple pipes will be opened for a single stream (might be the cause of the video corruption for Bravia TVs)
* Eventually, Roku TVs will, without user input, just stop playing a stream and send a close request to the server
