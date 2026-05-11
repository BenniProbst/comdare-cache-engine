# cmake/isa_features.cmake
# REV 5.3 Phase 6 - ISA-Feature-Erkennung (Compile-Time)
# Runtime-Detection via CPUID erfolgt in cache_engine/platform_probe/

include(CheckCXXSourceCompiles)

set(CMAKE_REQUIRED_QUIET ON)

# -----------------------------------------------------------------------------
# x86-64 ISA-Features
# -----------------------------------------------------------------------------
if(COMDARE_ARCH_X86_64)
    # AVX2
    if(COMDARE_ENABLE_AVX2)
        if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
            set(_avx2_flag "/arch:AVX2")
        else()
            set(_avx2_flag "-mavx2")
        endif()
        set(CMAKE_REQUIRED_FLAGS "${_avx2_flag}")
        check_cxx_source_compiles("
            #include <immintrin.h>
            int main() { __m256i a = _mm256_setzero_si256(); (void)a; return 0; }"
            COMDARE_HAS_AVX2)
        set(CMAKE_REQUIRED_FLAGS "")
    endif()

    # AVX-512 (nur auf passenden CPUs - i7-1270P hat KEIN AVX-512 mehr nach Microcode)
    if(COMDARE_ENABLE_AVX512)
        if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
            set(_avx512_flag "/arch:AVX512")
        else()
            set(_avx512_flag "-mavx512f -mavx512bw -mavx512vl")
        endif()
        set(CMAKE_REQUIRED_FLAGS "${_avx512_flag}")
        check_cxx_source_compiles("
            #include <immintrin.h>
            int main() { __m512i a = _mm512_setzero_si512(); (void)a; return 0; }"
            COMDARE_HAS_AVX512)
        set(CMAKE_REQUIRED_FLAGS "")
    endif()

    # BMI2 (PEXT/PDEP fuer HOT P02)
    if(COMDARE_ENABLE_BMI2)
        if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
            # MSVC hat keine separate /arch:BMI2 - geht ueber /arch:AVX2
            set(_bmi2_flag "")
        else()
            set(_bmi2_flag "-mbmi2")
        endif()
        set(CMAKE_REQUIRED_FLAGS "${_bmi2_flag}")
        check_cxx_source_compiles("
            #include <immintrin.h>
            int main() { unsigned long long r = _pext_u64(0xFF, 0x55); return (int)r; }"
            COMDARE_HAS_BMI2)
        set(CMAKE_REQUIRED_FLAGS "")
    endif()
endif()

# -----------------------------------------------------------------------------
# ARM ISA-Features
# -----------------------------------------------------------------------------
if(COMDARE_ARCH_ARM64)
    if(COMDARE_ENABLE_NEON OR COMDARE_OS_MACOS)
        # NEON ist auf AArch64 immer verfuegbar (auch Apple Silicon)
        set(COMDARE_HAS_NEON ON)
    endif()

    if(COMDARE_ENABLE_SVE2)
        set(CMAKE_REQUIRED_FLAGS "-march=armv8.5-a+sve2")
        check_cxx_source_compiles("
            #include <arm_sve.h>
            int main() { svuint64_t a = svdup_n_u64(0); return svcntb(); }"
            COMDARE_HAS_SVE2)
        set(CMAKE_REQUIRED_FLAGS "")
    endif()
endif()

# -----------------------------------------------------------------------------
# Reporting
# -----------------------------------------------------------------------------
set(COMDARE_ISA_SUMMARY "")
if(COMDARE_HAS_AVX2)
    string(APPEND COMDARE_ISA_SUMMARY "AVX2 ")
endif()
if(COMDARE_HAS_AVX512)
    string(APPEND COMDARE_ISA_SUMMARY "AVX512 ")
endif()
if(COMDARE_HAS_BMI2)
    string(APPEND COMDARE_ISA_SUMMARY "BMI2 ")
endif()
if(COMDARE_HAS_NEON)
    string(APPEND COMDARE_ISA_SUMMARY "NEON ")
endif()
if(COMDARE_HAS_SVE2)
    string(APPEND COMDARE_ISA_SUMMARY "SVE2 ")
endif()
if(NOT COMDARE_ISA_SUMMARY)
    set(COMDARE_ISA_SUMMARY "scalar-only")
endif()

# Helper: aktiviere fuer ein Target nur die SIMD-Flags die kompilierbar sind
function(COMDARE_apply_simd_flags target)
    if(COMDARE_HAS_AVX2 AND CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${target} PRIVATE /arch:AVX2)
    elseif(COMDARE_HAS_AVX2)
        target_compile_options(${target} PRIVATE -mavx2)
    endif()

    if(COMDARE_HAS_BMI2 AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${target} PRIVATE -mbmi2)
    endif()

    if(COMDARE_HAS_AVX512 AND CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${target} PRIVATE /arch:AVX512)
    elseif(COMDARE_HAS_AVX512)
        target_compile_options(${target} PRIVATE -mavx512f -mavx512bw -mavx512vl)
    endif()

    if(COMDARE_HAS_SVE2)
        target_compile_options(${target} PRIVATE -march=armv8.5-a+sve2)
    endif()
endfunction()
