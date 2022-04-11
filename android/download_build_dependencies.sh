mkdir -p dependencies
cd dependencies 

echo "Downloading libjpeg-turbo"
wget -q https://downloads.sourceforge.net/project/libjpeg-turbo/2.1.3/libjpeg-turbo-2.1.3.tar.gz
echo "Downloading libpng"
wget -q https://downloads.sourceforge.net/project/libpng/libpng16/1.6.37/libpng-1.6.37.tar.gz
echo "Downloading SDL2"
wget -q https://www.libsdl.org/release/SDL2-2.0.20.tar.gz

cd ../jni/src

echo "Decompressing archives"
tar -xf ../../dependencies/libjpeg-turbo-2.1.3.tar.gz
tar -xf ../../dependencies/libpng-1.6.37.tar.gz
tar -xf ../../dependencies/SDL2-2.0.20.tar.gz
