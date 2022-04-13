# Android Build
These instructions have only been tested on linux based hosts. 

1. Run the download_build_dependencies.sh script to download and unpack the SDL2, turbojpeg, and png dependencies.
2. Set the ANDROID_SDK_ROOT environment variable.
3. Run `gradlew build` from the android directory.
4. If gradle notifies you that you are missing the required ndk version, then install it via the sdk manager, and try again. 


