# cmake/compiler_flags.cmake
# REV 5.3 Phase 6 - Compiler-Flags pro OS/Compiler
# Multi-OS-konforme Flag-Setzung; KEIN /OS-spezifisches Setting ohne Fallback.

# Helper: setze Compile-Optionen pro Compiler-Familie
function(COMDARE_set_default_warnings target)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${target} PRIVATE
            /W4              # Hohe Warn-Stufe (NICHT /Wall - zu viel Rauschen)
            /permissive-     # Striktes C++23-Standard-Verhalten
            /Zc:__cplusplus  # Korrekter __cplusplus Wert
            /utf-8           # UTF-8 Source-Encoding
            /EHsc            # Standard-Exception-Handling
            $<$<CONFIG:Debug>:/Od /Zi>
            $<$<CONFIG:Release>:/O2>)
        target_compile_definitions(${target} PRIVATE
            _CRT_SECURE_NO_WARNINGS
            NOMINMAX
            WIN32_LEAN_AND_MEAN)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wnon-virtual-dtor
            -Wcast-align
            -Wold-style-cast
            -Woverloaded-virtual
            -Wmissing-declarations
            $<$<CONFIG:Debug>:-O0 -g3>
            $<$<CONFIG:Release>:-O3>
            $<$<CONFIG:RelWithDebInfo>:-O2 -g>)
    endif()
endfunction()

# Helper: setze C++23 Standard (verlaesslich Compiler-uebergreifend)
function(COMDARE_set_cpp23 target)
    target_compile_features(${target} PUBLIC cxx_std_23)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${target} PUBLIC /std:c++latest)
    endif()
endfunction()

# Helper: Plattform-Defines fuer Sub-Projekte (target_type-bewusst)
function(COMDARE_set_platform_defines target)
    get_target_property(_type ${target} TYPE)
    if(_type STREQUAL "INTERFACE_LIBRARY")
        set(_scope INTERFACE)
    else()
        set(_scope PUBLIC)
    endif()

    target_compile_definitions(${target} ${_scope}
        COMDARE_OS_NAME="${COMDARE_OS}"
        COMDARE_ARCH_NAME="${COMDARE_ARCH}"
        COMDARE_BLOCK_AO_PLATFORM_NAME="${COMDARE_BLOCK_AO_PLATFORM}"
        COMDARE_CACHE_LINE_SIZE=${COMDARE_CACHE_LINE_SIZE})
    if(COMDARE_OS_WINDOWS)
        target_compile_definitions(${target} ${_scope} COMDARE_OS_WINDOWS=1)
    endif()
    if(COMDARE_OS_LINUX)
        target_compile_definitions(${target} ${_scope} COMDARE_OS_LINUX=1)
    endif()
    if(COMDARE_OS_MACOS)
        target_compile_definitions(${target} ${_scope} COMDARE_OS_MACOS=1)
    endif()
    if(COMDARE_ARCH_X86_64)
        target_compile_definitions(${target} ${_scope} COMDARE_ARCH_X86_64=1)
    endif()
    if(COMDARE_ARCH_ARM64)
        target_compile_definitions(${target} ${_scope} COMDARE_ARCH_ARM64=1)
    endif()
    if(COMDARE_ARCH_RISCV64)
        target_compile_definitions(${target} ${_scope} COMDARE_ARCH_RISCV64=1)
    endif()
endfunction()
