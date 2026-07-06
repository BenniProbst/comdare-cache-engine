# hardware_isa — Domaene 4 (Querschnitt: ISA-Dispatch + Hybrid-Core + Memory)

add_library(comdare_hardware_isa INTERFACE)
target_include_directories(comdare_hardware_isa INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# #265 (Reuse P2, Default E-A): vendored foundation-Zellen comdare::platform + comdare::simd
# (Provenienz + Doktrin: vendor/REPOS_OVERVIEW.md). REIHENFOLGE ZWINGEND platform VOR simd —
# comdare-simd linkt comdare::platform nur if(TARGET ...) = Silent-Skip bei falscher Reihenfolge.
add_subdirectory(vendor/comdare-platform)
add_subdirectory(vendor/comdare-simd)
target_link_libraries(comdare_hardware_isa INTERFACE comdare::platform comdare::simd)

add_subdirectory(isa_dispatch)
add_subdirectory(hybrid_core_pinning)
add_subdirectory(memory_type_detector)
add_subdirectory(hugepage_manager)
