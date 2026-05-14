# hardware_isa — Domaene 4 (Querschnitt: ISA-Dispatch + Hybrid-Core + Memory)

add_library(comdare_hardware_isa INTERFACE)
target_include_directories(comdare_hardware_isa INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(isa_dispatch)
add_subdirectory(hybrid_core_pinning)
add_subdirectory(memory_type_detector)
add_subdirectory(hugepage_manager)
