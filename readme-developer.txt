To build Endless Sky you probably want the latest XCode version (I used 5.1.1). You also need to install three libraries:

libpng

http://sourceforge.net/projects/libpng/files/libpng14/

Get version 1.4.13. (The version number doesn't seem to match -- XCode links to libpng14.14.dylib -- but the file name will be right).

On my system, it was installed with the name libpng14.14..dylib (extra ".") and I had to rename it and fix the symlinks.

libturbojpeg

http://www.libjpeg-turbo.org/Documentation/OfficialBinaries

SDL2

Just downloading the SDL binary won't work, because XCode 5 checks that the framework is signed, and it isn't. Instead, build it from sourse:

hg clone https://hg.libsdl.org/SDL
open SDL/Xcode/SDL/SDL.xcodeproj

Build the framework, then copy it into /Library/Frameworks. When I did this, for some reason it built to some obscure directory that I had to look up in the logs, rather than into the Release directory where you 

Hopefully these will all install to the locations referenced in the project file; if not you can correct the paths.
