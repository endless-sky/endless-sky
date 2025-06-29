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
