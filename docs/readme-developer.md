# Build instructions

**Notice**: We have switched from Scons to CMake in release 0.9.16. Scons is no longer available as build system after release 0.10.12.

First you need a copy of the code (if you intend on working on the game, use your fork's URL here):

```powershell
> git clone https://github.com/endless-sky/endless-sky
```
**You can get a copy of the code either using "git clone," or using the repo's download button to obtain a [.ZIP archive](https://github.com/endless-sky/endless-sky/archive/master.zip).**

The game's root directory, where your unzipped/`git clone`d files reside, will be your starting point for compiling the game. You can use `cd endless-sky` to enter the game's directory.

Next, you will need to install a couple of dependencies to build the game. There are several different ways to build Endless Sky, depending on your operating system and preference.

- [Windows](#windows)
- [Windows (MinGW)](#windows-mingw)
- [MacOS](#macos)
- [Linux](#linux)

## Installing build dependencies

### Windows

We recommend using the toolchain from Visual Studio to build the game (regardless of the IDE you wish to use).

Download [Visual Studio](https://visualstudio.microsoft.com/downloads/#visual-studio-community-2022) (if you do not want to install Visual Studio, you can alternatively download the [VS Build Tools](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022)), and make sure to install the following components:

- "Desktop Development with C++",
- "C++ Clang Compiler for Windows",
- "C++ CMake tools for Windows".

We recommend using Visual Studio 2022 or newer. If you are unsure of which edition to use, choose Visual Studio Community.

### Windows (MinGW)
  
We recommend the [MinGW Winlibs](https://winlibs.com/#download-release) distribution, which also includes various tools you'll need to build the game as well. It is possible to use other MinGW distributions too (like Msys2 for example), but you'll need to make sure to install [CMake](https://cmake.org/) (3.21 or later) and [Ninja](https://ninja-build.org/).

When installing MinGW, use the following settings:
- Version: `8.1.0` (or greater; this document assumes 8.1.0)
- Architecture: `x86_64`
- Threads: `posix`
- Exception: `seh`

You'll need the POSIX version of MinGW. If you want your builds to have the same runtime library requirements as the official releases of Endless Sky, choose a version that links to the UCRT. For the Winlibs distribution mentioned above, the latest version is currently gcc 14 ([direct download link](https://github.com/brechtsanders/winlibs_mingw/releases/download/14.2.0posix-18.1.8-12.0.0-ucrt-r1/winlibs-x86_64-posix-seh-gcc-14.2.0-mingw-w64ucrt-12.0.0-r1.zip)). Download and extract the zip file in a folder whose path doesn't contain a space (C:\ for example) and add the bin\ folder inside to your PATH (Press the Windows key and type "edit environment variables", then click on PATH and add it to the list).

Endless Sky requires precompiled libraries to compile and play: [Download link](https://endless-sky.github.io/win64-dev.zip)

### MacOS

Install [Homebrew](https://brew.sh). Once it is installed, use it to install the tools and libraries you will need:

```bash
$ brew install cmake ninja mad libpng jpeg-turbo sdl2 minizip
```

**Note**: If you are on Apple Silicon (and want to compile for ARM), make sure that you are using ARM Homebrew!

If you want to build the libraries from source instead of using Homebrew, you can pass `-DES_USE_SYSTEM_LIBRARIES=OFF` to CMake when configuring.

### Linux

You can use your favorite package manager to install the needed dependencies. If you're using a slower moving distro like Ubuntu or Debian (or any derivatives thereof), make sure to use at least Ubuntu 22.04 LTS or Debian 12.
If your distro does not provide up-to-date version of these libraries, you can use vcpkg to build the necessary libraries from source by passing `-DES_USE_VCPKG=ON`. Older versions of Ubuntu and Debian, for example, will need this. Additional dependencies will likely need to be installed to build the libraries from source as well.

In addition to the below dependencies, you will also need CMake 3.19 or newer, however 3.21 or newer is strongly recommended. You can get the latest version from the [official website](https://cmake.org/download/). If you are often switching branches, then you can also consider installing [ccache](https://ccache.dev/) to speed up rebuilds after switching branches.


<details>
<summary>DEB-based distros</summary>

```
g++ cmake ninja-build curl libsdl2-dev libpng-dev libjpeg-dev libavif-dev libgl1-mesa-dev libglew-dev libminizip-dev libopenal-dev libmad0-dev libflac++-dev uuid-dev
```
If your CMake version is less than 3.31, you will also need
```
pkgconf
```
Additionally, if you want to build unit tests:
```
catch2
```
While sufficient versions of other dependencies are available, Ubuntu 22.04 does not provide an up to date version of catch2 (3.0 or newer is required), so this will need to be built from source if unit tests are desired.


</details>

<details>
<summary>RPM-based distros</summary>

```
gcc-c++ cmake ninja-build SDL2-devel libpng-devel libjpeg-turbo-devel libavif-devel mesa-libGL-devel glew-devel minizip-devel openal-soft-devel libmad-devel flac-devel libuuid-devel
```
If your CMake version is less than 3.31, you will also need
```
pkgconf
```
Additionally, if you want to build unit tests:
```
catch2-devel
```

</details>

## Building the game

### Building from the command line

(Note: This commands require CMake 3.21+. If you are using a lower version of CMake you will not be able to use the presets mentioned in this section.)

Here's a summary of every command you will need for development:

```bash
$ cmake --preset <preset>                                       # configure project (only needs to be done once)
$ cmake --build --preset <preset>-debug                         # build Endless Sky and all tests
$ cmake --build --preset <preset>-debug --target EndlessSky     # build only the game
$ ctest --preset <preset>-test                                  # run the unit tests
$ ctest --preset <preset>-benchmark                             # run the benchmarks
$ ctest --preset <preset>-integration                           # run the integration tests (Linux only)
```

The executable will be located in `build/<preset>/Debug/`. If you'd like to debug a specific integration test (on any OS), you can do so as follows:

```bash
$ ctest --preset <preset>-integration-debug -R <name>
```

You can get a list of integration tests with `ctest --preset <preset>-integration-debug -N`.

(You can also use the `<preset>-release` preset for a release build, and the output will be in the Release folder).

Replace `<preset>` with one of the following presets:

- Windows: `clang-cl` (builds with Clang for Windows), `mingw` (builds with MinGW), `mingw32` (builds with x86 MinGW)
- MacOS: `macos` or `macos-arm` (builds with the default compiler, for x64 and ARM64 respectively)
- Linux: `linux` (builds with the default compiler), `linux-gles` (compiles with GLES instead of OpenGL support), `linux-armv7` (provides better support for 32-bit ARM systems)

You can list all of available presets with `cmake --list-presets`.

<details>
<summary>Alternative linkers</summary>

The default linker on many systems is slow and may take multiple seconds to link the final executable, and has to be run every time any source files are changed. For faster iteration while developing, you can use an alternative linker for your platform:

- Windows: lld (supports clang-cl and mingw)
- MacOS: lld
- Linux: mold

Above are the generally best performing linkers on each platform, for all options see the full list in the [cmake docs](https://cmake.org/cmake/help/latest/variable/CMAKE_LINKER_TYPE.html).

This feature is available in cmake versions 3.29 and above through adding the `CMAKE_LINKER_TYPE` variable while performing the project configuration step (the build folder has to be deleted for changes to apply properly):

```sh
cmake --preset <preset> -DCMAKE_LINKER_TYPE=<LLD,MOLD>
```

</details>

### Using an IDE

Most IDEs have CMake support, and can be used to build the game. We recommend using [Visual Studio Code](#visual-studio-code).

#### Visual Studio Code

After installing VS Code, install the [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) and [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools) extensions, and open the project folder under File -> Open Folder.

<details>
<summary>Other recommended extensions</summary>

- [EditorConfig](https://marketplace.visualstudio.com/items?itemName=EditorConfig.EditorConfig)

</details>

You'll be asked to select a preset. Select the one you want (see the list above in the previous section for help). If you get asked to configure the project, click on Yes. You can use the bar at the very bottom to select between different configurations (Debug/Release), build, start the game, and execute the unit tests. On the left you can click on the test icon to run individual integration tests.

#### Visual Studio

We recommend using [Visual Studio 2022](https://visualstudio.microsoft.com/downloads/#visual-studio-community-2022) or newer, because of its better CMake integration. Once you have installed Visual Studio, you can simply open the root folder.

<details>
<summary>Step-by-step instructions for Visual Studio 2022 or later</summary>

1. Open the repository's root folder using Visual Studio ("Open Folder")
2. Wait while Visual Studio loads everything. This may take a few minutes the first time, but should be relatively fast on subsequent loads.
3. On the toolbar you're able to choose between Debug and Release.
4. You might need to select the target to launch in the dropdown menu of the Run button (it's the one with the green arrow). Select "Endless Sky (build/.../)" (not the one with install).
5. Hit the Run button (F5) to build and run the game.
6. In the status window it will give a scrolling list of actions being completed. Wait until it states "Build Complete"
7. You'll find the executables and libraries located inside the build directory in the root folder.

</details>

If you are on an earlier version of Visual Studio, or would like to use an actual VS solution, you will need to generate the solution manually as follows:

```powershell
> cmake --preset clang-cl -G"Visual Studio 17 2022"
```

This will create a Visual Studio 2022 solution. If you are using an older version of VS, you will need to adjust the version. Now you will find a complete solution in the `build/` folder. Find the solution and open it and you're good to go!

#### Code::Blocks

If you want to use the [Code::Blocks](https://www.codeblocks.org/downloads/) IDE, from the root of the project folder execute:

```powershell
> cmake -G"CodeBlocks - Ninja" --preset <preset>
```

With `<preset>` being one of the available presets (see above for a list). For Windows for example you'd want `clang-cl` or `mingw`. Now there will be a Code::Blocks project inside `build\`.

#### XCode

If you want to use the XCode IDE, from the root of the project folder execute:

```bash
$ cmake -G Xcode --preset macos # macos-arm for Apple Silicon
```

The XCode project is located in the `build/` directory.
