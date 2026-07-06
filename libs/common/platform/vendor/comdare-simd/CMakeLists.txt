cmake_minimum_required(VERSION 3.20)
project(comdare-simd VERSION 0.1.0 LANGUAGES CXX)
if(TARGET comdare-simd)
    message(STATUS "comdare-simd already defined; skipping wrapper target.")
    return()
endif()


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find comdare-platform
find_package(comdare-platform CONFIG QUIET)

# Library
add_library(comdare-simd INTERFACE)
target_include_directories(comdare-simd INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(TARGET comdare::platform)
    target_link_libraries(comdare-simd INTERFACE comdare::platform)
endif()

add_library(comdare::simd ALIAS comdare-simd)

# Tests
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    enable_testing()
    add_subdirectory(tests)
endif()

# Install
install(TARGETS comdare-simd EXPORT comdare-simd-targets)
install(DIRECTORY include/ DESTINATION include)
install(EXPORT comdare-simd-targets
    FILE comdare-simd-config.cmake
    NAMESPACE comdare::
    DESTINATION lib/cmake/comdare-simd
)
