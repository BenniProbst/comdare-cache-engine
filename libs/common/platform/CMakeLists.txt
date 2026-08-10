# hardware_isa — Domaene 4 (Querschnitt: ISA-Dispatch + Hybrid-Core + Memory)

add_library(comdare_hardware_isa INTERFACE)
target_include_directories(comdare_hardware_isa INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# #265 (Reuse P2, Default E-A): vendored foundation-Zellen comdare::platform + comdare::simd
# (Provenienz + Doktrin: vendor/REPOS_OVERVIEW.md). REIHENFOLGE ZWINGEND platform VOR simd —
# comdare-simd linkt comdare::platform nur if(TARGET ...) = Silent-Skip bei falscher Reihenfolge.
add_subdirectory(vendor/comdare-platform)
add_subdirectory(vendor/comdare-simd)

# WARN-VENDOR-AUSNAHME (10.08.2026, -Wall/-Wextra/-Wpedantic-Behebung): die Zellen sind byte-identische
# Kopien (Update-Prozedur "byte-treu neu kopieren", vendor/REPOS_OVERVIEW.md) -- Quell-Fixes gehoeren ins
# Upstream-Repo, nicht in die Kopie. Ihre Header werden darum als SYSTEM-Includes konsumiert (analog ext/):
# Consumer-TUs sehen keine Warnungen aus dem Vendor-Baum. Beleg: 2x -Wsign-compare SIMDDetect.hpp:340/348.
foreach(_comdare_vendored_cell comdare-platform comdare-simd)
    set_target_properties(${_comdare_vendored_cell} PROPERTIES
        INTERFACE_SYSTEM_INCLUDE_DIRECTORIES
        $<TARGET_PROPERTY:${_comdare_vendored_cell},INTERFACE_INCLUDE_DIRECTORIES>)
endforeach()

target_link_libraries(comdare_hardware_isa INTERFACE comdare::platform comdare::simd)

add_subdirectory(isa_dispatch)
add_subdirectory(hybrid_core_pinning)
add_subdirectory(memory_type_detector)
add_subdirectory(hugepage_manager)
