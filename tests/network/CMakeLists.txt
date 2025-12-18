# Network tests for ENet integration and Network Abstraction Layer

cmake_minimum_required(VERSION 3.19)

# ENet connection test (Phase 1.1)
add_executable(test_enet_connection test_enet_connection.cpp)
target_link_libraries(test_enet_connection PRIVATE enet::enet)
target_compile_features(test_enet_connection PRIVATE cxx_std_20)

# Network Abstraction Layer test (Phase 1.2)
add_executable(test_network_abstraction test_network_abstraction.cpp)
target_link_libraries(test_network_abstraction PRIVATE enet::enet)
target_compile_features(test_network_abstraction PRIVATE cxx_std_20)

# Add tests
enable_testing()
add_test(NAME ENetConnection COMMAND test_enet_connection)
set_tests_properties(ENetConnection PROPERTIES TIMEOUT 10 LABELS "network;phase1.1")

add_test(NAME NetworkAbstraction COMMAND test_network_abstraction)
set_tests_properties(NetworkAbstraction PROPERTIES TIMEOUT 10 LABELS "network;phase1.2")

message(STATUS "Network tests configured (Phase 1.1 + 1.2)")
