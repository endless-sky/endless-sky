cmake_minimum_required(VERSION 3.0)

project(game4)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_CXX_STANDARD 11)

if (ENABLE_ASAN)
   set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=address")
   set (CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fno-omit-frame-pointer -fsanitize=address")
endif()

add_subdirectory(etc2comp/EtcLib)
include_directories(etc2comp/EtcLib/Etc)
include_directories(etc2comp/EtcLib/EtcCodec)

add_executable(texture_tool
   texture_tool.cpp
)

target_link_libraries(texture_tool
   EtcLib
   jpeg
   png
)