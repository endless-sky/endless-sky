# Building on Windows

We recommend using the Visual Studio toolchain because it is easier for development on Windows. However if you wish to use MinGW, see [this page](readme-mingw.md).

Install the following features using the Visual Studio Installer (either for Visual Studio if you'd like to use that IDE, or just the build tools if you want to use another IDE):

- "Desktop Development with C++",
- "C++ Clang Compiler for Windows",
- "C++ clang-cl for vXXX build tools", and
- "C++ CMake tools for Windows".

You will also need Git, which you can get [here](https://gitforwindows.org/).

## Visual Studio 2022

Open the folder containing ES, that's it! If you want to use an actual VS solution, see the notes at the end.

More detailed instructions if needed:

1. Open the endless-sky folder that you cloned in the initial step.
2. Wait while Visual Studio loads everything. This may take a few minutes the first time, but should be relatively fast on subsequent loads.
3. On your toolbar there should be a pulldown menu that says "debug" or "release." Select the version you want to build.
4. Hit the "build" button, or find the "Build" menu and select "Build All"
5. In the status window it will give a scrolling list of actions being completed. Wait until it states "Build Complete"
6. At this point you should be able to launch the recently completed build by hitting the F5 key or the run button. The recently build executable and essential libraries can be found in your Endless-Sky folder, nested within a subfolder created by Visual Studio during the build process

### Notes

#### Earlier versions of Visual Studio

If you are on an earlier version of Visual Studio, or would like to use an actual VS solution, you will need to generate the solution manually as follows:

```powershell
> cmake --preset clang-cl -G"Visual Studio 17 2022"
```

This will create a Visual Studio 2022 solution. If you are using an older version of VS, you will need to adjust the version. Now you will find a complete solution in the `build/` folder. Find the solution and open it and you're good to go!

#### Using the Microsoft Visual C++ compiler

Using MSVC (instead of Clang on Windows) to compile Endless Sky is not supported. If you try, you will get a ton of warnings (and maybe even a couple of errors) during compilation.
