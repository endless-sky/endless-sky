First you need a copy of the code (if you intend on working on the game use your fork's URL here):

```powershell
> git clone https://github.com/endless-sky/endless-sky
```

The game's root directory, where your `git clone`d files reside, will be your starting point for compiling the game.

Next you will need to install a couple of dependencies to build the game.

## Build Environment

There are several different ways to build Endless Sky, depending on your operating system and preference.
- [Windows](#Windows)
- [MacOS](#MacOS)
- [Linux](#Linux)

## Windows (Visual Studio) <Windows>
We are currently switching to using VS with cmake. If you wish the older MingW build instructions, they are located at the end of the Windows section.

Download Visual Studio, and make sure to install the following features: 
- "Desktop Development with C++", 
- "C++ Clang Compiler for Windows", 
- "C++ clang-cl for vXXX build tools", and 
- "C++ CMake tools for Windows".

Please note that it is recommened to use VS 2022 (or higher) for its better CMake integration.

### Windows Build Process

1. Open the endless-sky folder that you cloned in the initial step.
2. Wait while Visual Studio loads everything. This may take a few minutes the first time, but should be relatively fast on subsequent loads.
3. On your toolbar there should be a pulldown menu that says "debug" or "release." Select the version you want to build.
4. Hit the "build" button, or find the "Build" menu and select "Build All"
5. In the status window it will give a scrolling list of actions being completed. Wait until it states "Build Complete"
6. At this point you should be able to launch the recently completed build by hitting the F5 key or the run button. The recently build executable and essential libraries can be found in your Endless-Sky folder, nested within a subfolder created by Visual Studio during the build process

#### Notes

##### Earlier versions of Visual Studio

If you are on an earlier version of Visual Studio, or would like to use an actual VS solution, you will need to generate the solution manually as follows:

```powershell
> cmake --preset clang-cl -G"Visual Studio 17 2022"
```

This will create a Visual Studio 2022 solution. If you are using an older version of VS, you will need to adjust the version. Now you will find a complete solution in the `build/` folder. Find the solution and open it and you're good to go!

#### Using Microsoft Visual C++

Using MSVC to compile Endless Sky is not supported. If you try, you will get a ton of warnings (and maybe even a couple of errors) during compilation.

<details>

<summary>Building on Windows using MinGW</summary>

  
You can download the [MinGW Winlibs](https://winlibs.com/#download-release) build, which also includes various tools you'll need to build the game as well. It is possible to use other MinGW builds as well.

You'll need the MSVCRT runtime version, 64-bit. The latest version is currently gcc 12 ([direct download link](https://github.com/brechtsanders/winlibs_mingw/releases/download/12.1.0-14.0.4-10.0.0-msvcrt-r2/winlibs-x86_64-posix-seh-gcc-12.1.0-mingw-w64msvcrt-10.0.0-r2.zip)).

Extract the zip file in a folder whose path doesn't contain a space (C:\ for example) and add the bin\ folder inside to your PATH (Press the Windows key and type "edit environment variables", then click on PATH and add it in the list).

You will also need to install [CMake](https://cmake.org) (if you don't already have it).

</details>

## MacOS <MacOS>

Install [Homebrew](https://brew.sh). Once it is installed, use it to install the tools and libraries you will need:

```
$ brew install cmake ninja mad libpng jpeg-turbo sdl2 openal-soft
```

**Note**: If you are on Apple Silicon (and want to compile for ARM), make sure that you are using ARM Homebrew!

If you want to build the libraries from source instead of using Homebrew, you can pass `-DES_USE_SYSTEM_LIBRARIES=OFF` to CMake when configuring.

## Linux <Linux>

You will need at least CMake 3.21. You can get the latest version from the [offical website](https://cmake.org/download/).

**Note**: If your distro does not provide up-to-date version of the needed libraries, you will need to tell CMake to build the libraries from source by passing `-DES_USE_SYSTEM_LIBRARIES=OFF` to the first cmake command under the command line build instructions.

If you use a reasonably up-to-date distro, then you can use your favorite package manager to install the needed dependencies.

<details>
<summary>DEB-based distros</summary>

```
g++ cmake ninja-build libsdl2-dev libpng-dev libjpeg-dev libgl1-mesa-dev libglew-dev libopenal-dev libmad0-dev uuid-dev
```

</details>

<details>
<summary>RPM-based distros</summary>

```
gcc-c++ cmake ninja-build SDL2-devel libpng-devel libjpeg-turbo-devel mesa-libGL-devel glew-devel openal-soft-devel libmad-devel libuuid-devel
```

</details>

# Building from the command line

Here's a summary of every command you will need for development:

```bash
$ cmake --preset <preset>                     # configure project (only needs to be done once)
$ cmake --build --preset <preset>-debug       # actually build Endless Sky (as well as any tests)
$ ./build/<preset>/Debug/endless-sky          # run the game
$ ctest --preset <preset>-test                # run the unit tests
$ ctest --preset <preset>-benchmark           # run the benchmarks
$ ctest --preset <preset>-integration         # run the integration tests (Linux only)
```

If you'd like to debug a specific integration test (on any OS), you can do so as follows:

```bash
$ ctest --preset <preset>-integration-debug -R <name>
```

You can get a list of integration tests with `ctest --preset <preset>-integration-debug -N`.

(You can also use the `<preset>-release` preset for a release build, and the output will be in the Release folder).

Replace `<preset>` with one of the following presets:

- Windows: `clang-cl` (builds with Clang for Windows), `mingw` (builds with MinGW)
- MacOS: `macos` or `macos-arm` (builds with the default compiler), `xcode` or `xcode-arm` (builds using the XCode toolchain)
- Linux: `linux` (builds with the default compiler)

# Building with other IDEs

## Building with Visual Studio Code

Install the [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools), [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools), [CMake Test explorer](https://marketplace.visualstudio.com/items?itemName=fredericbonnet.cmake-test-adapter) extensions and open the project folder under File -> Open Folder.

You'll be asked to select a preset. Select the one you want (see the table above). If you get asked to configure the project, click on Yes. You can use the bar at the very bottom to select between different configurations (Debug/Release), build, start the game, and execute the unit tests. On the left you can click on the test icon to run individual integration tests.

## Building with Code::Blocks

If you want to use the Code::Blocks IDE, from the root of the project folder execute:

```powershell
> cmake -G"CodeBlocks - Ninja" --preset <preset>
```

With `<preset>` being one of the available presets (see above for a list). For Windows for example you'd want `mingw`. Now there will be a Code::Blocks project inside `build\mingw`.



## Building with XCode

If you want to use the XCode IDE, from the root of the project folder execute:

```bash
$ cmake --preset macos -G Xcode # macos-arm for Apple Silicon
```

The XCode project is located in the `build/` directory.

