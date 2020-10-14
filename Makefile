EMSCRIPTEN_ENV := $(shell command -v emmake 2> /dev/null)

clean:
	rm -f endless-sky.html
	rm -f endless-sky.js
	rm -f endless-sky.worker.js
	rm -f endless-sky.data
	rm -f endless-sky.wasm
	rm -f endless-sky.wasm.map
	rm -f lib/emcc/libendless-sky.a
2.1.0.tar.gz:
	wget https://github.com/libjpeg-turbo/libjpeg-turbo/archive/refs/tags/2.1.0.tar.gz
libjpeg-turbo-2.1.0: 2.1.0.tar.gz
	tar xzvf 2.1.0.tar.gz
# | means libjpeg-turbo-2.1.0 is a "order-only prerequisite" so creating the file doesn't invalidate the dir
libjpeg-turbo-2.1.0/libturbojpeg.a: | libjpeg-turbo-2.1.0
ifndef EMSCRIPTEN_ENV
	$(error "emmake is not available, activate the emscripten env first")
endif
	cd libjpeg-turbo-2.1.0; emcmake cmake -G"Unix Makefiles" -DWITH_SIMD=0 -DCMAKE_BUILD_TYPE=Release -Wno-dev
	cd libjpeg-turbo-2.1.0; emmake make
dev: endless-sky.html
	emrun --serve_after_close --serve_after_exit --browser chrome --private_browsing endless-sky.html

COMMON_FLAGS = -O3 -flto\
		-s USE_SDL=2\
		-s USE_LIBPNG=1\
		-pthread\
		-s DISABLE_EXCEPTION_CATCHING=0

CFLAGS = $(COMMON_FLAGS)\
	-Duuid_generate_random=uuid_generate\
	-std=c++11\
	-Wall\
	-Werror\
	-Wold-style-cast\
	-DES_GLES\
	-gsource-map\
	-fno-rtti\
	-I libjpeg-turbo-2.1.0\


# Note that that libmad is not linked! It's mocked out until it works with Emscripten
LINK_FLAGS = $(COMMON_FLAGS)\
	-L libjpeg-turbo-2.1.0\
	-l jpeg\
	-lidbfs.js\
	--source-map-base http://localhost:6931/\
	-s USE_WEBGL2=1\
	-s ASSERTIONS=2\
	-s DEMANGLE_SUPPORT=1\
	-s GL_ASSERTIONS=1\
	--closure 1\
	-s ASYNCIFY\
	-s PTHREAD_POOL_SIZE=7\
	-s MIN_WEBGL_VERSION=2\
	-s MAX_WEBGL_VERSION=2\
	-s WASM_MEM_MAX=2147483648\
	-s INITIAL_MEMORY=629145600\
	-s ALLOW_MEMORY_GROWTH=1\
	--preload-file data\
	--preload-file images\
	--preload-file sounds\
	--preload-file credits.txt\
	--preload-file keys.txt\
	-s EXPORTED_RUNTIME_METHODS=['callMain']\
	--emrun

CPPS := $(shell ls source/*.cpp) $(shell ls source/text/*.cpp)
CPPS_EXCEPT_MAIN := $(shell ls source/*.cpp | grep -v main.cpp) $(shell ls source/text/*.cpp)
TEMP := $(subst source/,build/emcc/,$(CPPS))
OBJS := $(subst .cpp,.o,$(TEMP))
TEMP := $(subst source/,build/emcc/,$(CPPS_EXCEPT_MAIN))
OBJS_EXCEPT_MAIN := $(subst .cpp,.o,$(TEMP))
HEADERS := $(shell ls source/*.h*) $(shell ls source/text/*.h*)

build/emcc/%.o: source/%.cpp
	mkdir -p build/emcc
	mkdir -p build/emcc/text
	em++ $(CFLAGS) -c $< -o $@

lib/emcc/libendless-sky.a: $(OBJS_EXCEPT_MAIN)
	mkdir -p lib/emcc
	emar rcs lib/emcc/libendless-sky.a $(OBJS_EXCEPT_MAIN)

endless-sky.html: libjpeg-turbo-2.1.0/libturbojpeg.a lib/emcc/libendless-sky.a build/emcc/main.o
ifndef EMSCRIPTEN_ENV
	$(error "em++ is not available, activate the emscripten env first")
endif
	em++ -o endless-sky.html $(LINK_FLAGS) build/emcc/main.o lib/emcc/libendless-sky.a
