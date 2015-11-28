Build instructions:



Linux:

Use your favorite package manager to install the following (version numbers may vary depending on your distribution):

  * g++
  * scons
  * libsdl2-dev
  * libpng12-dev
  * libjpeg-turbo8-dev
  * libgl1-mesa-dev (or some other equivalent)
  * libglew-dev
  * libopenal-dev

You can then just navigate to the source code folder in a terminal and type:

  $ scons
  $ ./endless-sky

The program will run using the "data" and "images" folders that are found in the source code folder itself. For more Linux help, consult the man page (endless-sky.6).



Windows:

The Windows build has been tested on 64-bit Windows 7, only. You will need the Code::Blocks IDE and g++ 4.8 or higher. Code::Blocks is available here:

  http://sourceforge.net/projects/codeblocks/files/Binaries/13.12/Windows/codeblocks-13.12-setup.exe/download

You can install g++ separately through mingw-w64:

  http://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win64/Personal%20Builds/mingw-builds/4.8.5/threads-posix/seh/

*** Be sure to install the "pthread" version of MinGW. The "win32-thread" one does not come with support for C++11 threading. If you are using 32-bit Windows, install the compiler for "dwarf" exceptions, not "sjlj." ***

If you are on 64-bit Windows, a full set of development libraries are available here:

  http://endless-sky.github.io/win64-dev.zip

If you don't want to have to edit the paths in the Code::Blocks file, unpack the "dev64" folder directly into C:\.

If you are using 32-bit Windows, a full set of development libraries are available here:

  http://endless-sky.github.io/win32-dev.zip

You will probably need to adjust the paths to your compiler binaries, and you should also switch to the "Win32" build instead of the "Debug" or "Release" build.

You will also need libmingw32.a and libopengl32.a. Those should be included in the MinGW g++ install. If they are not in C:\Program Files\mingw64\x86_64-w64-mingw32\lib\ you will have to adjust the paths in the Code::Blocks file.



Mac OS X:

To build Endless Sky you probably want the latest XCode version (I used 5.1.1). You also need to install three libraries:

libpng

http://sourceforge.net/projects/libpng/files/libpng14/

Get version 1.4.13. (The version number doesn't seem to match -- XCode links to libpng14.14.dylib -- but the file name will be right).

On my system, it was installed with the name libpng14.14..dylib (extra ".") and I had to rename it and fix the symlinks.

libturbojpeg

http://www.libjpeg-turbo.org/Documentation/OfficialBinaries

SDL2

Just downloading the SDL binary won't work, because XCode 5 checks that the framework is signed, and it isn't. Instead, build it from source:

hg clone https://hg.libsdl.org/SDL; open SDL/Xcode/SDL/SDL.xcodeproj

Build the framework, then copy it into /Library/Frameworks. When I did this, for some reason it built to some obscure directory that I had to look up in the logs.

Hopefully these will all install to the locations referenced in the project file; if not you can correct the paths.

Library paths

To create a Mac OS X binary that will work on systems other than your own, you may also need to use install_name_tool to modify the libraries so that their location is relative to the @rpath.
