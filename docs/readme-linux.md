# Building on Linux

You will need at least CMake 3.21. You can get the latest version from the [offical website](https://cmake.org/download/). You will also need a C++ compiler (usually gcc) and [Ninja](https://ninja-build.org/).

**Note**: If your distro does not provide up-to-date versions of the needed libraries, you will need to tell CMake to build the libraries from source by passing `-DES_USE_SYSTEM_LIBRARIES=OFF` to CMake when configuring.

If you use a reasonably up-to-date distro, then you can use your favorite package manager to install the needed dependencies:

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
