# Network tests for ENet integration

cmake_minimum_required(VERSION 3.19)

# ENet connection test
add_executable(test_enet_connection test_enet_connection.cpp)
target_link_libraries(test_enet_connection PRIVATE enet::enet)

# Enable C++20
target_compile_features(test_enet_connection PRIVATE cxx_std_20)

# Add test
enable_testing()
add_test(NAME ENetConnection COMMAND test_enet_connection)
set_tests_properties(ENetConnection PROPERTIES TIMEOUT 10)

message(STATUS "ENet network tests configured")
