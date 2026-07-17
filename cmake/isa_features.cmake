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

# ─────────────────────────────────────────────────────────────────────────────
# GO-3 A1 (Task #5 Hebel-A-Rest, 2026-07-12): comdare_apply_simd_extension_flags(<target> <EXT>)
# Bildet GENAU EINE axis_09b-simd_extension (09b-Vokabular: NO_EXTENSION/SSE2/AVX2/AVX512/NEON/SVE2/RVV/
# CUDA_GH200) auf ihre ECHTEN Compiler-ISA-Flags ab — Deklarations-Wahrheit der Build-Varianten-Achse: eine
# Binary, die Avx2SimdExtension deklariert (POD simd_width_bits=256), MUSS auch mit AVX2-Codegen bauen, sonst
# faehrt der T12-Kernel (Amd64Isa::simd_field_sum-Dispatch) real den SSE2-Pfad (Mess-Etikett != Mess-
# Gegenstand). Compile-time-Gegenstueck: consteval-Guard axis_09b_build_coherence.hpp + additives Makro
# COMDARE_DEFINE_BUILD_VARIANT_INSPECTION_CHECKED (static_assert bei Inkohaerenz).
# ABGRENZUNG: COMDARE_apply_simd_flags (oben) appliziert ALLE detektierten Stufen auf einmal (Detection-
# orientiert) und bleibt unveraendert daneben bestehen; DIESE Funktion waehlt je-Extension (09b-orientiert) —
# die Flag-Kaskade ist damit EINMAL zentral (keine Dreifach-Spiegelung in Test-/Produkt-CMakeLists).
# Der Legacy-V36.B-Permutations-Codegen (_p_simd-Kanal, tools/permutation_codegen/codegen.cmake) und sein
# C++23-Backend stehen unter dem #25-B-Byte-Identitaets-Vertrag — bewusst NICHT beruehrt (GO-2-Sequenz).
# ─────────────────────────────────────────────────────────────────────────────
function(comdare_apply_simd_extension_flags target ext)
    if(ext MATCHES "^(SSE2|AVX2|AVX512)$" AND NOT COMDARE_ARCH_X86_64)
        message(FATAL_ERROR "comdare_apply_simd_extension_flags(${target} ${ext}): x86-Extension auf "
            "Nicht-x86-Build (COMDARE_ARCH_X86_64=OFF) — das Etikett waere unwahr.")
    endif()
    if(ext STREQUAL "NO_EXTENSION" OR ext STREQUAL "SSE2")
        # x86-64-ABI-Baseline: SSE2 ist Pflicht-Teil der ABI — keine zusaetzliche Flag noetig.
        # (NO_EXTENSION deklariert Nicht-NUTZUNG, nicht Nicht-Existenz — Exemption auch im consteval-Guard.)
    elseif(ext STREQUAL "AVX2")
        if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
            target_compile_options(${target} PRIVATE /arch:AVX2)
        else()
            target_compile_options(${target} PRIVATE -mavx2)
        endif()
    elseif(ext STREQUAL "AVX512")
        # F-Stufe wie der Kernel-Dispatch (__AVX512F__); MSVC /arch:AVX512 definiert __AVX512F__ (+BW/DQ/VL —
        # der Guard prueft nur die F-Stufe, wie der Kernel; Dossier-GO3-Risiko 2).
        if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
            target_compile_options(${target} PRIVATE /arch:AVX512)
        else()
            target_compile_options(${target} PRIVATE -mavx512f)
        endif()
    elseif(ext STREQUAL "NEON")
        # aarch64-Baseline: NEON ist immer verfuegbar — keine Flag; auf Nicht-ARM ist die Deklaration unwahr.
        if(NOT COMDARE_ARCH_ARM64)
            message(FATAL_ERROR "comdare_apply_simd_extension_flags(${target} NEON): NEON-Deklaration auf "
                "Nicht-ARM-Build (COMDARE_ARCH_ARM64=OFF) — Etikett != Maschinencode.")
        endif()
    elseif(ext STREQUAL "SVE2")
        if(COMDARE_HAS_SVE2)
            target_compile_options(${target} PRIVATE -march=armv8.5-a+sve2)
        else()
            message(FATAL_ERROR "comdare_apply_simd_extension_flags(${target} SVE2): COMDARE_HAS_SVE2 nicht "
                "detektiert (Detection oben in dieser Datei) — die SVE2-Deklaration waere unwahr.")
        endif()
    elseif(ext STREQUAL "RVV")
        message(FATAL_ERROR "comdare_apply_simd_extension_flags(${target} RVV): RVV-Flag-Kopplung ist "
            "Folge-Slice R3 (Doc 21 §F: NEON/RVV HW-/INFRA-gated) — noch nicht verdrahtet.")
    elseif(ext STREQUAL "CUDA_GH200")
        message(FATAL_ERROR "comdare_apply_simd_extension_flags(${target} CUDA_GH200): CUDA ist keine "
            "Host-CPU-ISA-Flag — GPU-Offload-Bau gehoert in die #276-Build-Matrix (8er-Docker/ISA-Doktrin).")
    else()
        message(FATAL_ERROR "comdare_apply_simd_extension_flags(${target} ${ext}): unbekannte axis_09b-"
            "Extension (erwartet: NO_EXTENSION/SSE2/AVX2/AVX512/NEON/SVE2/RVV/CUDA_GH200).")
    endif()
endfunction()

# ── comdare_apply_optimization_level_flags(target level) — Bau-INC-2c.opt Naht #2 ────────────────────
# Wendet die Optimierungsstufe der Compiler-System-Achse (opt_level-Unterachse, siehe
# include/cache_engine/measurement/optimization_level_sub_axis.hpp) auf ein Target an. Vorbild:
# comdare_apply_simd_extension_flags (oben). Zweck HIER: mess-charakteristische Verifikations-Test-Targets
# (Cache-Line-/Layout-Effekte sind nur UNTER Optimierung sichtbar; im unoptimierten Default-Build maskiert der
# Instrumentierungs-Overhead den Effekt → Layout-Sensitivitaets-Check faellt physikalisch unter die 5%-Schwelle).
# Level-Semantik deckungsgleich zur opt_level-Achse: O0/O1/O2/O3 = IEEE-754-treu + Run-to-Run-deterministisch;
# Ofast = aggressiv (CEB-Default fuer Mess-Binaries, OF-2), aber determinismus-brechend → NICHT fuer die
# deterministischen Verifikations-Tests. Der volle XML-permutierte opt_level-Raum kommt mit dem Planer (2c.opt-g/h).
function(comdare_apply_optimization_level_flags target level)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        if(level STREQUAL "O0")
            target_compile_options(${target} PRIVATE /Od)
        else()
            target_compile_options(${target} PRIVATE /O2)  # MSVC-naechstliegend fuer O1..Ofast
        endif()
    elseif(level MATCHES "^(O0|O1|O2|O3|Ofast)$")
        target_compile_options(${target} PRIVATE "-${level}")
    else()
        message(FATAL_ERROR "comdare_apply_optimization_level_flags(${target} ${level}): unbekannte "
            "opt_level-Stufe (erwartet: O0/O1/O2/O3/Ofast).")
    endif()
endfunction()
