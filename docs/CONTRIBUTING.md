# Contributing to Endless Sky

## Build instructions

**You can get a copy of the code either using "git clone," or using the repo's download button to obtain a [.ZIP archive](https://github.com/endless-sky/endless-sky/archive/master.zip).** The game's root directory, where your unzipped/`git clone`d files reside, will be your starting point for compiling the game.

How you build Endless Sky will then depend on your operating system: [Windows](readme-windows.md], [Linux](readme-linux.md), [macOS](readme-macos.md). Popular IDEs are supported through their CMake integration, for example: Visual Studio, Visual Studio Code, XCode, CodeBlocks, CLion, and many more.

<details>
<summary>Command line build instructions</summary>

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
- macOS: `macos` or `macos-arm` (builds with the default compiler)
- Linux: `linux` (builds with the default compiler)

</details>

<details>
<summary>Using Visual Studio Code</summary>

Install the [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools), [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools), [CMake Test explorer](https://marketplace.visualstudio.com/items?itemName=fredericbonnet.cmake-test-adapter) extensions and open the project folder under File -> Open Folder.

You'll be asked to select a preset. Select the one you want (see the table above). If you get asked to configure the project, click on Yes. You can use the bar at the very bottom to select between different configurations (Debug/Release), build, start the game, and execute the unit tests. On the left you can click on the test icon to run individual integration tests.

</details>

## Posting issues

The [issues page](https://github.com/endless-sky/endless-sky/issues) on GitHub is for tracking bugs and feature requests. When posting a new issue, please:

* Check to make sure it's not a duplicate of an existing issue.
* Create a separate "issue" for each bug you are reporting and each feature you are requesting.
* Do not use the issues page for things other than bug reports and feature requests. Use the [discussions page](https://github.com/endless-sky/endless-sky/discussions) instead.

If requesting a new feature, first ask yourself: will this make the game more fun or interesting? Remember that this is a game, not a simulator. Changes will not be made purely for the sake of realism, especially if they introduce needless complexity or aggravation.

## Posting pull requests

If you are posting a pull request, please:

* Do not combine multiple unrelated changes.
* Check the diff and make sure the pull request does not contain unintended changes.
* If changing the C++ code, follow the [coding standard](https://endless-sky.github.io/styleguide/styleguide.xml).

If proposing a major pull request, start by posting an issue and discussing the best way to implement it. Often the first strategy that occurs to you will not be the cleanest or most effective way to implement a new feature.

## Closing issues

If you believe your issue has been resolved, you can close the issue yourself. If your issue gets closed because a PR was merged, and you are not satisfied, please open a new issue.
