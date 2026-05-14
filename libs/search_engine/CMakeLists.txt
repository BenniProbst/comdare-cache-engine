# search_engine — Domaene 1
# fan_out_engine, Algorithm Strategies, Page/Node/Traversal/ValueHandle Concepts

add_library(comdare_search_engine INTERFACE)
target_include_directories(comdare_search_engine INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Sub-Komponenten (alle Concepts, daher header-only INTERFACE)
add_subdirectory(fan_out_engine)
add_subdirectory(algorithm_strategies)
add_subdirectory(page)
add_subdirectory(node)
add_subdirectory(value_handle)
add_subdirectory(traversal)
add_subdirectory(memory_layout)
add_subdirectory(operations)
