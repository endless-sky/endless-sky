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

Then, from the project root folder, simply type:

  $ scons
  $ ./endless-sky

The program will run, using the "data" and "images" folders that are found in the project root. For more Linux help, and to view other command-line flags, consult the `man` page (endless-sky.6), by running `man endless-sky` in the project directory.
The program also accepts a `--help` command line flag.

To compile and also run unit tests, the "test" target can be used:

  $ scons test

The produced test binary will be invoked with a reasonable set of arguments. To run any test benchmarks, you will need to invoke the test binary directly. Refer to the build pipeline definitions for the current required arguments.
More information on unit tests can be found in the project's "tests" directory.



Windows:

Two methods of building the game on Windows are available: via a Code::Blocks workspace, and via Scons. Both options require a MinGW toolchain. To build via Scons will also require Python 3, while building with Code::Blocks will naturally require the Code::Blocks IDE.
The Scons method is preferred, and a makefile wrapper is provided to enable integration with a wide variety of IDEs. The Windows build pipeline definitions can be used as a reference to configure arbitrary IDEs or build environments, such as VS Code, CLion, or Visual Studio.

You can install g++ through mingw-w64. Any version of the MinGW toolchain that supports the full C++11 standard can be used, meaning any version of g++ >= 4.8.5 is sufficient. One such toolchain is available here:

  http://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win64/Personal%20Builds/mingw-builds/4.8.5/threads-posix/seh/

*** Be sure to install the "pthread" version of MinGW. The "win32-thread" one does not come with support for C++11 threading. If you are using 32-bit Windows, install the compiler for "dwarf" exceptions, not "sjlj." ***

MinGW and g++ are often available bundled in other development tools, such as Qt or Code::Blocks. These bundled installations will also generally work, but are untested.

No matter the method you choose to build the game, you will need external dependencies to link with the compiled source code. Precompiled dependencies are available for the MinGW toolchain. If you wish to use a different compiler, you will likely need to compile your own versions of these dependencies.
If you are on 64-bit Windows, a full set of development libraries are available here:

  http://endless-sky.github.io/win64-dev.zip

If you don't want to have to edit the paths in the Code::Blocks project files, unpack the "dev64" folder directly into C:\.

If you are still using 32-bit Windows, a full set of development libraries are available here:

  http://endless-sky.github.io/win32-dev.zip

You will probably need to adjust the paths to your compiler binaries, and you should also switch to the "Win32" build instead of the "Debug" or "Release" build. No additional support is provided for the 32-bit build; you are strongly recommended to upgrade to a 64-bit environment.

You will also need libmingw32.a and libopengl32.a. Those should be included in the MinGW g++ install. If they are not in C:\Program Files\mingw64\x86_64-w64-mingw32\lib\ you will have to adjust the include directory and library (linker) directory search paths in the Code::Blocks project files.

The Code::Blocks workspace consists of three projects: one for the majority of source code, one for the game binary, and one for the unit tests. As with most IDE-based projects, you must explicitly add new files to the respective project for them to be compiled.
To get started, open the "EndlessSky.workspace" file, which will load the three linked projects. Double-clicking a project in the left-hand menu will activate that project, binding the toolbar & keyboard shortcuts for "Build," "Run,", "Build & Run," etc. to that project, no matter which file is being edited.
For example, when making lots of changes to the game, you will generally have the "endless-sky-lib" project active, so that you can ensure changes compile without needing to fully link things together into the actual game binary. After making changes, you would then activate the "EndlessSky" project and use the "Build & Run" option with the "Release" target, which will fully link the compiled code into the small, performant executable. You can continue making tweaks to files that belong to the "endless-sky-lib" project without reactivating it, so that "Build & Run" will still launch the game with your latest tweaks. Should you experience some show-stopper bug in your modified game, you would then change to the "Debug" build target to help determine the source(s) of the issue.



Mac OS X:

To build Endless Sky with native tools, you will first need to download Xcode from the App Store.

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
  $ sudo install_name_tool -id "@rpath/libmad.0.dylib" /usr/local/lib/libmad.0.dylib
  $ sudo install_name_tool -id "@rpath/libturbojpeg.0.dylib" /usr/local/opt/libjpeg-turbo/lib/libturbojpeg.0.dylib
  $ sudo install_name_tool -id "@rpath/libSDL2-2.0.0.dylib" /usr/local/lib/libSDL2-2.0.0.dylib

*** Note: there is extremely limited development support for macOS, and no intent to support macOS's new ARM architecture. ***



Link-Time Optimization (LTO):

For both the Linux and Windows "release" targets, "link-time optimization" is used. This generally will work without issue with newer versions of g++ / MinGW, but may require an explicit usage of the `gcc-ar` and `gcc-ranlib` binaries in your development environment.
For the Code::Blocks project, the archive program can be configured in Code::Block's global compiler settings menu, accessed via "Settings -> Compiler..." in the application menu bar. On the "Toolchain executables" tab, change "linker for static libs" from "ar.exe" to "gcc-ar.exe"

The Scons builds can be controlled by setting the appropriate environment variable(s), either directly in the environment or just for the lifetime of the command:

  $ AR=gcc-ar RANLIB=gcc-ranlib scons 
