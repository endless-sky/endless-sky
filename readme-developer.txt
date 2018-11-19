Build instructions:



Linux:

Use your favorite package manager to install the following (version numbers may vary depending on your distribution):

DEB-based distros:
   g++ \
   scons \
   libsdl2-dev \
   libpng-dev \
   libjpeg-dev \
   libgl1-mesa-dev \
   libglew-dev \
   libopenal-dev \
   libmad0-dev

RPM-based distros:
   gcc-c++ \
   scons \
   SDL2-devel \
   libpng-devel \
   libjpeg-turbo-devel \
   mesa-libGL-devel \
   glew-devel \
   openal-soft-devel \
   libmad-devel

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

To build Endless Sky, you will first need to download Xcode from the App Store.

Next, install Homebrew (from http://brew.sh).

Once Homebrew is installed, use it to install the libraries you will need:
$ brew install libpng
$ brew install libjpeg-turbo
$ brew install libmad
$ brew install sdl2

If the versions of those libraries are different from the ones that the Xcode project is set up for, you will need to modify the file paths in the “Frameworks” section in Xcode.

It is possible that you will also need to modify the “Header Search Paths” and “Library Search Paths” in “Build Settings” to point to wherever Homebrew installed those libraries.

Library paths

To create a Mac OS X binary that will work on systems other than your own, you may also need to use install_name_tool to modify the libraries so that their location is relative to the @rpath.

$ sudo install_name_tool -id "@rpath/libpng16.16.dylib" /usr/local/lib/libpng16.16.dylib
$ sudo install_name_tool -id "@rpath/libmad.0.2.1.dylib" /usr/local/lib/libmad.0.2.1.dylib
$ sudo install_name_tool -id "@rpath/libturbojpeg.0.dylib" /usr/local/opt/libjpeg-turbo/lib/libturbojpeg.0.dylib
$ sudo install_name_tool -id "@rpath/libSDL2-2.0.0.dylib" /usr/local/lib/libSDL2-2.0.0.dylib
