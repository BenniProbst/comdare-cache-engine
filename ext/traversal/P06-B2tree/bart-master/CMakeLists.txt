cmake_minimum_required(VERSION 3.5)
project(tree)

set(CMAKE_CXX_STANDARD 17)

# Compiler flags for the different targets
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fPIC -Wall -Wextra -fno-omit-frame-pointer -march=native -Wno-unknown-pragmas")
# Only use address sanitizer in debug builds
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")

# Make sure we use O3 optimization level instead of O2
string(REPLACE "-O2" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE} -O3")
string(REPLACE "-O2" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O3")

include(lib/jevents.cmake)

include_directories(
  third_party)

add_executable(treeception pbtreestatic_trie.cpp)
target_include_directories(treeception PRIVATE $<BUILD_INTERFACE:${JEVENTS_INCLUDE_DIR}>)
target_link_libraries(treeception ${JEVENTS_LIBRARY_PATH})
add_dependencies(treeception jevents)

add_executable(b2tree_int b2tree_int.cpp)
target_include_directories(b2tree_int PRIVATE $<BUILD_INTERFACE:${JEVENTS_INCLUDE_DIR}>)
target_link_libraries(b2tree_int ${JEVENTS_LIBRARY_PATH})
add_dependencies(b2tree_int jevents)

add_executable(pbtreestatic pbtreestatic.cpp)
target_include_directories(pbtreestatic PRIVATE $<BUILD_INTERFACE:${JEVENTS_INCLUDE_DIR}>)
target_link_libraries(pbtreestatic ${JEVENTS_LIBRARY_PATH})
add_dependencies(pbtreestatic jevents)

add_executable(bart bart.cpp)
target_include_directories(bart PRIVATE $<BUILD_INTERFACE:${JEVENTS_INCLUDE_DIR}>)
target_link_libraries(bart ${JEVENTS_LIBRARY_PATH})
add_dependencies(bart jevents)

ADD_CUSTOM_TARGET(link_target ALL COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_SOURCE_DIR}/bench.sh" "${CMAKE_CURRENT_BINARY_DIR}/bench.sh")
