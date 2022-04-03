upnp_live
=============
A UPnP Media Server for relaying internet live streams. Also has basic file serving functionality.

## Building
* C++14 compliant compiler required
* Autoconf eventually, for now just run `make`
#### Dependencies
* [libupnp](https://github.com/pupnp/pupnp) v1.8+
  * Only tested with versions 1.8.3 and 1.12.1
* A program that prints AV data to stdout. Not neded for compilation, but required for core functionality until other handlers are implemented.
  * Output from this program should either originate from a real-time source or be throttled so as to be no faster than real-time. The primary use case is programs like [streamlink](https://github.com/streamlink/streamlink) that retrieve remote live streams.
* \[optional\] Transcoding requires a program like [ffmpeg](https://ffmpeg.org/) or [flac](https://xiph.org/flac/documentation_tools_flac.html) that can read media from stdin and print it to stdout.

## Working Devices
The server has been confirmed to work with the following media renderers:

#### Televisions
  * Roku TV 5305X (Insignia model NS-55DR420NA16, Software version 9.3.0 build 4194) - "Roku Media Player" v5.3
    * 60fps streams had to be transcoded to 30fps. Audio and video desynced nearly instantly at 60fps
  * Roku TV 8116X (TCL model 32S327, Software version 9.3.2 build 4217-48) - "Roku Media Player" v5.3 build 5
    * This one *was* able to handle 60fps streams

#### Media Players
  * VLC (v3.0.7.1 Vetinari)
  * MPC-HC (64-bit v1.7.13)
    * Couldn't render video when stream was being transcoded, just audio

#### Other
  * Playstation 4 (non-pro) OS v7.55 - Media Player application v4.01
    * Played video and audio files, couldn't play streams (only tested with Twitch)

## Contributing
I make no guarantees of any kind with this project. I don't expect to respond to or engage with pull requests, issues/tickets, or pretty much anything, at least not for a while. It's possible I won't touch this codebase again until my next sabbatical.

## Known Bugs
* The program can be a bit brittle. It tends to segfault when frequently recompiling (hopefully that's just me being bad with makefiles) and may still do it when shutting down thanks to Server::Read().
* The file AV handler is probably busted, I haven't actually tested it since, even if it technically works, it won't really work as long as MemoryStore overwrites old data as soon as it can. 
* Will frequently hang while shutting down

## Code Structure
The three classes that form the largest chunks of functionality are Server, Stream and MemoryStore. Basically everything else is directly dependent on one of these three classes and they house most of the code that determines control flow in the program, does error handling, etc.

## TODO (big ticket items only. see TODO.md for more)
* Use autoconf or something instead of a manually edited makefile for compilation
* Fix the UPnP stuff (events like subscriptions and state var requests, the ConnectionManagerService, etc.)
* Add web interface (with compiler flag)
* Tests
* Unicode for (at least) stream names
* Valgrind?
* Doxygen comments?
