cmake_minimum_required(VERSION 3.5)
project(tree)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -Wall -Wno-long-long -pedantic")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(COMPILE_COMMANDS_FILE_NAME "compile_commands.json")
if(CMAKE_BUILD_TYPE MATCHES Debug)
  add_custom_target(
      link_target ALL
      COMMAND ${CMAKE_COMMAND} -E create_symlink
      "${CMAKE_BINARY_DIR}/${COMPILE_COMMANDS_FILE_NAME}"
      "${CMAKE_SOURCE_DIR}/${COMPILE_COMMANDS_FILE_NAME}")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules/")



option(TREE_ENABLE_AVX512 "Enable/Disable AVX512" OFF)
if(TREE_ENABLE_AVX512)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx512f -mavx512bw -mavx512dq -mavx512cd -DTREE_ENABLE_AVX512")
  message(STATUS "Enabling AVX512")
endif()



find_package(Threads)
#find_package(TBB COMPONENTS tbbmalloc tbbmalloc_proxy tbb_preview)
find_package(Boost COMPONENTS thread system REQUIRED)
include(lib/gtest.cmake)
include(lib/jevents.cmake)

include_directories(
  ${TBB_INCLUDE_DIR}
  include
  third_party)

### tree library
add_library(tree_lib src/tree.cpp)
target_include_directories(tree_lib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
#  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
  PRIVATE src)

### Tests
add_executable(test_all test/test_tree.cpp)
target_link_libraries(test_all tree_lib gtest gtest_main pthread tbb)

### main application
add_executable(tree src/main.cpp)
target_link_libraries(tree tree_lib tbb Boost::thread Boost::system ${CMAKE_THREAD_LIBS_INIT})

add_executable(bench src/bench.cpp)
target_include_directories(bench PRIVATE
  $<BUILD_INTERFACE:${JEVENTS_INCLUDE_DIR}>)
target_link_libraries(bench tree_lib tbb ${CMAKE_THREAD_LIBS_INIT} ${JEVENTS_LIBRARY_PATH})

add_dependencies(bench jevents)