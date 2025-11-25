cmake_minimum_required(VERSION 3.0)

# Strip the debug flags, as there isn't a great way to debug on a device,
# and we have the option to build and debug on the host machine anyways. The
# only benefit of debugging symbols in the android build was that it bloated
# the binary by 70 MB per so file. I don't know why I said this was a benefit.
# For the record, overriding these flags is normally a very bad idea.
STRING(REPLACE "-g" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
STRING(REPLACE "-g" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

# enable link time optimization. Also, override the optimization flags here,
# otherwise -Oz gets passed to the linker, which does not understand it.
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -O3 -flto")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -flto")

add_subdirectory(src)
