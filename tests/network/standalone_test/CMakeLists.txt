# Minimal standalone ENet test
cmake_minimum_required(VERSION 3.19)
project(ENetStandaloneTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use vcpkg if available
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../../vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

find_package(enet CONFIG REQUIRED)

add_executable(enet_standalone_test ../test_enet_connection.cpp)
target_link_libraries(enet_standalone_test PRIVATE enet::enet)

message(STATUS "ENet standalone test configured successfully")
