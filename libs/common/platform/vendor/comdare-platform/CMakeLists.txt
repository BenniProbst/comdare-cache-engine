cmake_minimum_required(VERSION 3.20)
project(comdare-platform VERSION 0.1.0 LANGUAGES CXX)
if(TARGET comdare-platform)
    message(STATUS "comdare-platform already defined; skipping wrapper target.")
    return()
endif()


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Library
add_library(comdare-platform INTERFACE)
target_include_directories(comdare-platform INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

add_library(comdare::platform ALIAS comdare-platform)

# Tests
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    enable_testing()
    add_subdirectory(tests)
endif()

# Install
install(TARGETS comdare-platform EXPORT comdare-platform-targets)
install(DIRECTORY include/ DESTINATION include)
install(EXPORT comdare-platform-targets
    FILE comdare-platform-config.cmake
    NAMESPACE comdare::
    DESTINATION lib/cmake/comdare-platform
)
