# Build instructions

**Notice**: We'll be slowly switching over to CMake, so treat these instructions as deprecated. The instructions using CMake can be read [here](readme-cmake.md).

Most of the current development work is done on Ubuntu Linux. Building the code on any Linux variant should be relatively straightforward. Building it on Windows or Mac OS X is a bit more complicated.

**You can get a copy of the code either using "git clone," or using the repo's download button to obtain a [.ZIP archive](https://github.com/endless-sky/endless-sky/archive/master.zip).** The game's root directory, where your unzipped/`git clone`d files reside, will be your starting point for compiling the game.

How you build Endless Sky will then depend on your operating system:



## Linux

Use your favorite package manager to install the following (version numbers may vary depending on your distribution):

##### DEB-based distros:
   - g++
   - scons
   - libsdl2-dev
   - libpng-dev
   - libjpeg-dev
   - libgl1-mesa-dev
   - libglew-dev
   - libopenal-dev
   - libmad0-dev
   - uuid-dev

Additionally, if you want to build unit tests:
   - catch2

##### RPM-based distros:
   - gcc-c++
   - scons
   - SDL2-devel
   - libpng-devel
   - libjpeg-turbo-devel
   - mesa-libGL-devel
   - glew-devel
   - openal-soft-devel
   - libmad-devel
   - libuuid-devel

Additionally, if you want to build unit tests:
   - catch2-devel

Then, from the project root folder, simply type:

```
    $ scons
    $ ./endless-sky
```

The program will run, using the "data" and "images" folders that are found in the project root. For more Linux help, and to view other command-line flags, consult the `man` page (endless-sky.6), by running `man endless-sky` in the project directory.
The program also accepts a `--help` command line flag.

To compile and also run unit tests, the "test" target can be used:

```
  $ scons test
```

The produced test binary will be invoked with a reasonable set of arguments. To run any test benchmarks, you will need to invoke the test binary directly. Refer to the build pipeline definitions for the current required arguments.
More information on unit tests can be found in the project's "tests" directory, or by running the binary with the `--tests` flag.



## Windows

MinGW provides the tools used to compile the source code into the game's executable. For most cases, you should download the [MinGW-W64 Online Installer](https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/Personal%20Builds/mingw-builds/installer/mingw-w64-install.exe). When installing, use the following settings:

- Version: `8.1.0` (or greater; this document assumes 8.1.0)
- Architecture: `x86_64`
- Threads: `posix`
- Exception: `seh`

Endless Sky requires precompiled libraries to compile and play: [Download link](https://endless-sky.github.io/win64-dev.zip)

The zip can be extracted anywhere on your filesystem; to minimize additional configuration, you can move the `dev64` folder to `C:\`.


### Building with Scons

If you want to build the game from the command line via Scons, you will need [Python 3.8 or later](https://www.python.org/downloads/). When installing, be sure to select the "Add to PATH" checkbox.

Afterwards, you will need to add your MinGW installation to your path manually. To do so on Windows 10, go to **Settings** > **System** > **About** > **System info** or **Related Settings** > **Advanced system settings** > **Environment Variables**. From each, select **Path** under System variables, and click Edit.

If you used the defaults for MinGW up to this point, add "New" and enter the following in separate entries:

- `C:\Program Files\mingw-w64\x86_64-8.1.0-posix-seh-rt_v6-rev0\mingw64\bin`
- `C:\Program Files (x86)\Git\usr\bin\` or `C:\Program Files\Git\usr\bin\`, depending on which you have installed

From there, restart your command line if you had one open, and simply `cd` to your Endless Sky directory, then type `mingw32-make.exe -f .winmake`. (`make`'s standard `-j N` flag is supported, to increase the number of active parallel build tasks to N.)

**IMPORTANT:** Before the game will run the first time, you will need to skip down to the instructions at **Running the game** below.

### Building with other IDEs

Refer to the build pipeline files under `.github\` to tailor the build process to your chosen IDE.


### Running the Game

For Endless Sky to run on Windows, it needs various .dll files included in the base directory. Go to `C:\dev64\bin\`, and copy every file to your base directory.

**After** you have copied all those libraries, you will need additional libraries from your MinGW installation: libstdc++-6.dll, libwinpthread-1.dll, libgcc_s_seh-1.dll. By default, you can go to `C:\Program Files\mingw-w64\x86_64-8.1.0-posix-seh-rt_v6-rev0\mingw64\bin`, and copy those libraries to your Endless Sky directory to overwrite the current ones.

Once all the libraries in place, the game is ready to run! You have three options depending on how you built it:

- If you built the game via Scons, open a terminal to the base Endless Sky directory, then run `bin\pkgd\release\endless-sky.exe`.
- If you want to run the game manually, use one of the methods above to build the game, then copy an executable from under the `bin\` folder to your base directory, then open the game.
