cmake_minimum_required(VERSION 3.16)
project(nicsim VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(NICSIM_BUILD_TESTS "Build unit tests" ON)

add_library(nicsim_core
    src/mac_address.cpp
    src/ethernet_frame.cpp
    src/packet_buffer.cpp
    src/nic_device.cpp
    src/nic_stats.cpp
    src/network_medium.cpp
)

target_include_directories(nicsim_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(nicsim src/main.cpp)
target_link_libraries(nicsim PRIVATE nicsim_core)

if(NICSIM_BUILD_TESTS)
    enable_testing()
    add_executable(nicsim_tests tests/test_main.cpp)
    target_link_libraries(nicsim_tests PRIVATE nicsim_core)
    add_test(NAME nicsim_tests COMMAND nicsim_tests)
endif()
