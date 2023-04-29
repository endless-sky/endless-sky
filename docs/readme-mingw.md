# Building with MinGW on Windows

Download the [MinGW Winlibs](https://winlibs.com/#download-release) build, which also includes various tools you'll need to build the game as well.

You'll need the MSVCRT runtime version, 64-bit. The latest version is currently gcc 13 ([direct download link](https://github.com/brechtsanders/winlibs_mingw/releases/download/13.1.0-16.0.2-11.0.0-msvcrt-r1/winlibs-x86_64-mcf-seh-gcc-13.1.0-mingw-w64msvcrt-11.0.0-r1.zip)).

Extract the zip file in a folder whose path doesn't contain a space (C:\ for example) and add the bin\ folder inside to your PATH (Press the Windows key and type "edit environment variables", then click on PATH and add it in the list).

You will also need to install [CMake](https://cmake.org) (if you don't already have them).

## Code::Blocks

If you want to use the Code::Blocks IDE, from the root of the project folder run:

```powershell
> cmake -G"CodeBlocks - Ninja" --preset mingw
```

Now there will be a Code::Blocks project inside `build\mingw`.
