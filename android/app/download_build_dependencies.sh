mkdir -p dependencies
cd dependencies 

echo "Downloading libjpeg-turbo"
curl -Lo libjpeg-turbo.tar.gz https://downloads.sourceforge.net/project/libjpeg-turbo/2.1.3/libjpeg-turbo-2.1.3.tar.gz
echo "467b310903832b033fe56cd37720d1b73a6a3bd0171dbf6ff0b620385f4f76d0  libjpeg-turbo.tar.gz" | sha256sum -c - || exit
tar xzf libjpeg-turbo.tar.gz -C ../jni/src

echo "Downloading libpng"
curl -Lo libpng.tar.gz https://downloads.sourceforge.net/project/libpng/libpng16/1.6.37/libpng-1.6.37.tar.gz
echo "daeb2620d829575513e35fecc83f0d3791a620b9b93d800b763542ece9390fb4  libpng.tar.gz" | sha256sum -c - || exit
tar xzf libpng.tar.gz -C ../jni/src

echo "Downloading SDL2"
curl -Lo SDL2.tar.gz https://www.libsdl.org/release/SDL2-2.0.20.tar.gz
echo "c56aba1d7b5b0e7e999e4a7698c70b63a3394ff9704b5f6e1c57e0c16f04dd06  SDL2.tar.gz" | sha256sum -c - || exit
tar xzf SDL2.tar.gz -C ../jni/src

echo "Downloading OpenAL-Soft"
curl -kLo openal-soft.tar.bz2  https://openal-soft.org/openal-releases/openal-soft-1.22.0.tar.bz2
echo "ce0f9300de3de7bc737b0be2a995619446e493521d070950eea53eddd533fc9b  openal-soft.tar.bz2" | sha256sum -c - || exit
tar xjf openal-soft.tar.bz2 -C ../jni/src

# Unusable without av1 decoder backend
# echo "Downloading libavif"
# curl -Lo libavif.tar.gz https://github.com/AOMediaCodec/libavif/archive/refs/tags/v0.11.1.tar.gz
# echo "0eb49965562a0e5e5de58389650d434cff32af84c34185b6c9b7b2fccae06d4e  libavif.tar.gz" | sha256sum -c - || exit
# tar xzf libavif.tar.gz -C ../jni/src
# # It claims that cmake 3.13 is required, but it mysteriously works fine with
# # 3.10 (which is what the ndk provides)
# sed -i '/cmake_minimum_required/d' ../jni/src/libavif-0.11.1/CMakeLists.txt

# Uhg... This changes the timestamps in the tarball every time, so the checksum never matches. 
# echo "Downloading libgav1"
# curl -Lo libgav1.tar.gz https://chromium.googlesource.com/codecs/libgav1/+archive/c05bf9be660cf170d7c26bd06bb42b3322180e58.tar.gz


echo "Downloading dav1d"
curl -Lo dav1d.tar.xz http://downloads.videolan.org/videolan/dav1d/1.5.1/dav1d-1.5.1.tar.xz
echo "401813f1f89fa8fd4295805aa5284d9aed9bc7fc1fdbe554af4292f64cbabe21  dav1d.tar.xz" | sha256sum -c - || exit
tar xf dav1d.tar.xz -C ../jni/src/dav1d/

