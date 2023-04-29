# Building on MacOS

Install [Homebrew](https://brew.sh). Once it is installed, use it to install the tools and libraries you will need:

```bash
$ brew install cmake ninja mad libpng jpeg-turbo sdl2 openal-soft
```

**Note**: If you are on Apple Silicon (and want to compile for ARM), make sure that you are using ARM Homebrew!

If you want to build the libraries from source instead of using Homebrew, you can pass `-DES_USE_SYSTEM_LIBRARIES=OFF` to CMake when configuring.

## XCode

If you want to use the XCode IDE, from the root of the project folder run:

```bash
$ cmake --preset macos -G Xcode # macos-arm for Apple Silicon
```

The XCode project is located in the `build/` directory.
