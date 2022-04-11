# Android Build
The android build is supported using the ndk-build tool provided with the android ndk. These instructions have only been tested on linux based hosts. 

1. Run the download_build_dependencies.sh script to download and unpack the SDL2, turbojpeg, and png dependencies.
2. Set the JAVA_HOME and ANDROID_HOME environment variables, and add the android ndk to the PATH.
3. Run ndk-build from the android directory
4. Run ant from android directory. 

I usally just create a build.sh file with the following contents (you will need to adjust the paths to match your environment):
```
export JAVA_HOME="/usr/lib/jvm/java-8-openjdk-amd64"
export PATH=~/Android/android-ndk-r20b:$PATH
export ANDROID_HOME=~/Android/adt-bundle-linux-x86_64-20140321/sdk

ndk-build -j3 && ant -v debug  && adb install -r bin/EndlessMobile-debug.apk
```

