
# Uncomment this if you're using STL in your project
# See CPLUSPLUS-SUPPORT.html in the NDK documentation for more information
# APP_STL := stlport_static 
APP_STL := c++_static
APP_ABI := armeabi-v7a
#APP_ABI := armeabi armeabi-v7a x86
APP_PLATFORM := android-19

# to set up a release build, -O2
APP_OPTIM := release
LOCAL_CFLAGS += -O2
