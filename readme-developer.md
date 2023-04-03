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

Two methods of building the game on Windows are supported. The official build is done on the command-line with the Scons program, but the game may also be built using the Code::Blocks IDE. Both require a MinGW toolchain and the precompiled libraries linked below.

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


### Building with Code::Blocks

If you instead want to build the game with a visual IDE, then you can use [Code::Blocks](https://codeblocks.org/downloads/26), the main supported IDE for Endless Sky on Windows. As you have already installed a compiler via MinGW, you should select a version without one.

After you install it, Code::Blocks will likely detect a "GNU GCC Compiler" from MinGW. Select this as your compiler, then take these steps to configure it:

- Go to Settings > Compiler > Global Compiler Settings > Toolchain Executables. Set the **Compiler's installation directory** to `C:\Program Files\mingw-w64\x86_64-8.1.0-posix-seh-rt_v6-rev0\mingw64\bin`, if you followed the default MinGW installation process above.
- Modify **Linker for static libs** from `ar.exe` to `gcc-ar.exe`.

After this, the game should be ready to compile. Use Code::Blocks to open the "EndlessSky.workspace" (**not EndlessSky.cbp**) file in the directory where you've cloned the game's code. This workspace consists of three projects: one for the majority of source code, one for the game binary, and one for the unit tests. As with most IDE-based projects, you must explicitly add new files to the respective project for them to be compiled.

With the workspace open, you can double-click a project in the left-hand menu to activate that project, binding the toolbar & keyboard shortcuts for "Build," "Run,", "Build & Run," etc. to it, no matter which file is being edited.

For example, when making lots of changes to the game, you will generally have the "endless-sky-lib" project active, so that you can ensure changes compile without needing to fully link things together into the actual game binary. After making changes, you would then activate the "EndlessSky" project and use the "Build & Run" option with the "Release" target, which will fully link the compiled code into the small, performant executable.

You can continue making tweaks to files that belong to the "endless-sky-lib" project without reactivating it, so that "Build & Run" will still launch the game with your latest tweaks. Should you experience some show-stopper bug in your modified game, you would then change to the "Debug" build target to help determine the source(s) of the issue.

**IMPORTANT:** Before the game will run the first time, you will need to skip down to the instructions at **Running the game** below.


### Building with other IDEs

Refer to the build pipeline files under `.github\` to tailor the build process to your chosen IDE.


### Running the Game

For Endless Sky to run on Windows, it needs various .dll files included in the base directory. Go to `C:\dev64\bin\`, and copy every file to your base directory.

**After** you have copied all those libraries, you will need additional libraries from your MinGW installation: libstdc++-6.dll, libwinpthread-1.dll, libgcc_s_seh-1.dll. By default, you can go to `C:\Program Files\mingw-w64\x86_64-8.1.0-posix-seh-rt_v6-rev0\mingw64\bin`, and copy those libraries to your Endless Sky directory to overwrite the current ones.

Once all the libraries in place, the game is ready to run! You have three options depending on how you built it:

- If you built the game via Scons, open a terminal to the base Endless Sky directory, then run `bin\pkgd\release\endless-sky.exe`.
- If you built the game via Code::Blocks, double click *EndlessSky* in the file panel on the left, then either click the green arrow on top, or hit Control+F10.
- If you want to run the game manually, use one of the methods above to build the game, then copy an executable from under the `bin\` folder to your base directory, then open the game.
