# ext — Originalcode der Paper (Habich-Direktive)
#
# Pro Paper-Verzeichnis wird der ORIGINALCODE EXAKT KOPIERT eingebettet.
# Compiler-Layering (F-EXTRA-1):
#  - Originalcode-Bausteine werden mit IHREM Original-Compiler kompiliert
#  - Statisches Linken in den C++23-Modul-Wrapper (siehe adapters/)
#
# Pre-Build-Phase (Phase 4.B Cluster-Migration): pro ext/<paper>/ wird
# eine eigene CMakeLists.txt mit Original-Compiler-Toolchain registriert.

# Skelett — Inhalt kommt nach Repo-Einbindung in Phase 4.B
# add_subdirectory(P01-ART)   # nach Migration
# add_subdirectory(P02-HOT)
# ...

# V40.A (2026-05-24): Allokator-Achse echt — mimalloc als Vendor-Library
# Aktiviert wenn COMDARE_BUILD_PERMUTATIONS=ON (Permutationen brauchen den Allokator)
option(COMDARE_VENDOR_MIMALLOC "Build vendored mimalloc (ext/A04-mimalloc)"
       ${COMDARE_BUILD_PERMUTATIONS})
if(COMDARE_VENDOR_MIMALLOC AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/A04-mimalloc/src/static.c")
    # V40.A direkter Build via src/static.c (umgeht mimalloc's eigenes CMake-System,
    # weil cmake/-Helper-Files fehlen). static.c includiert alle .c-Sources.
    # FIX (2026-05-24): C-Compiler braucht's. Project ist LANGUAGES CXX,
    # daher hier enable_language(C) damit add_library mit .c-Sources klappt.
    enable_language(C)
    add_library(comdare_vendor_mimalloc STATIC
        A04-mimalloc/src/static.c)
    target_include_directories(comdare_vendor_mimalloc PUBLIC
        A04-mimalloc/include)
    target_compile_definitions(comdare_vendor_mimalloc PRIVATE
        MI_STATIC_LIB=1
        MI_BUILD_RELEASE=1)
    set_target_properties(comdare_vendor_mimalloc PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        FOLDER "vendor")
    if(MSVC)
        # MSVC: secure CRT warnings unterdruecken (mimalloc nutzt _* APIs heuristisch)
        target_compile_definitions(comdare_vendor_mimalloc PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()
    if(NOT TARGET comdare::vendor_mimalloc)
        add_library(comdare::vendor_mimalloc ALIAS comdare_vendor_mimalloc)
    endif()
    message(STATUS "V40.A comdare::vendor_mimalloc verfuegbar (static.c direct build)")
endif()

# V40.B + V41.A1 (2026-05-24): jemalloc — drei-stufige Detection:
#   1. find_library: system-installed (Linux apt/yum, vcpkg)
#   2. Pre-generierte jemalloc.h vorhanden (User hat autogen.sh manuell laufen lassen)
#   3. Bare-State (nur .h.in templates): klare Hinweis-Message + Fallback
option(COMDARE_VENDOR_JEMALLOC "jemalloc-Detection (system / pre-configured / Fallback)"
       ${COMDARE_BUILD_PERMUTATIONS})
set(COMDARE_HAVE_JEMALLOC OFF CACHE BOOL "" FORCE)

if(COMDARE_VENDOR_JEMALLOC)
    # Stufe 1: System-installed jemalloc
    find_library(JEMALLOC_LIB NAMES jemalloc libjemalloc)
    find_path(JEMALLOC_INC jemalloc/jemalloc.h)
    if(JEMALLOC_LIB AND JEMALLOC_INC)
        add_library(comdare_vendor_jemalloc INTERFACE)
        target_include_directories(comdare_vendor_jemalloc INTERFACE "${JEMALLOC_INC}")
        target_link_libraries(comdare_vendor_jemalloc INTERFACE "${JEMALLOC_LIB}")
        if(NOT TARGET comdare::vendor_jemalloc)
            add_library(comdare::vendor_jemalloc ALIAS comdare_vendor_jemalloc)
        endif()
        set(COMDARE_HAVE_JEMALLOC ON CACHE BOOL "" FORCE)
        message(STATUS "V41.A1 jemalloc (system): ${JEMALLOC_LIB}")
    else()
        # Stufe 2: ext/A05-jemalloc pre-configured? Headers schon generiert?
        set(_je_root "${CMAKE_CURRENT_LIST_DIR}/A05-jemalloc")
        if(EXISTS "${_je_root}/include/jemalloc/jemalloc.h"
           AND EXISTS "${_je_root}/include/jemalloc/internal/jemalloc_preamble.h")
            # Headers wurden via autogen.sh erzeugt — wir koennen direkt bauen
            file(GLOB _je_sources CONFIGURE_DEPENDS "${_je_root}/src/*.c")
            enable_language(C)
            add_library(comdare_vendor_jemalloc STATIC ${_je_sources})
            target_include_directories(comdare_vendor_jemalloc PUBLIC
                "${_je_root}/include")
            target_include_directories(comdare_vendor_jemalloc PRIVATE
                "${_je_root}/include/jemalloc/internal")
            target_compile_definitions(comdare_vendor_jemalloc PRIVATE
                JEMALLOC_NO_PRIVATE_NAMESPACE=1
                JEMALLOC_EXPORT=)
            if(MSVC)
                target_compile_definitions(comdare_vendor_jemalloc PRIVATE _CRT_SECURE_NO_WARNINGS)
                target_compile_options(comdare_vendor_jemalloc PRIVATE /wd4267 /wd4244 /wd4146)
            endif()
            set_target_properties(comdare_vendor_jemalloc PROPERTIES
                POSITION_INDEPENDENT_CODE ON
                FOLDER "vendor")
            if(NOT TARGET comdare::vendor_jemalloc)
                add_library(comdare::vendor_jemalloc ALIAS comdare_vendor_jemalloc)
            endif()
            set(COMDARE_HAVE_JEMALLOC ON CACHE BOOL "" FORCE)
            message(STATUS "V41.A1 jemalloc (ext/A05 pre-configured): direkter Build")
        else()
            # Stufe 3: Bare state — autogen.sh nicht ausgefuehrt
            message(STATUS "V41.A1 jemalloc NICHT verfuegbar. Fallback (real=std).")
            message(STATUS "  Pre-Built Headers fehlen. Setup-Optionen:")
            message(STATUS "    A) Linux: 'apt install libjemalloc-dev'")
            message(STATUS "    B) Windows + MSYS2: 'cd ext/A05-jemalloc && sh -c \"CC=cl ./autogen.sh\"' im 'x64 Native Tools Command Prompt'")
            message(STATUS "    C) Windows + vcpkg: 'vcpkg install jemalloc:x64-windows-static'")
        endif()
    endif()
endif()

# V41.A2 (2026-05-25): Weitere Allokatoren als Vendor-Build / find_library
# Analog jemalloc: System-Detection > Vendored-Build > Fallback std.
# snmalloc ist header-only-faehig; hoard/tcmalloc/scalloc brauchen separates
# Build-Setup (siehe TODO V41.A2 Sub-Tasks).

# --- snmalloc (Microsoft Research, header-only-faehig) ---
option(COMDARE_VENDOR_SNMALLOC "Snmalloc Detection (header-only-Setup)" ${COMDARE_BUILD_PERMUTATIONS})
set(COMDARE_HAVE_SNMALLOC OFF CACHE BOOL "" FORCE)
if(COMDARE_VENDOR_SNMALLOC)
    set(_sn_root "${CMAKE_CURRENT_LIST_DIR}/A07-snmalloc")
    if(EXISTS "${_sn_root}/src/snmalloc/snmalloc.h")
        add_library(comdare_vendor_snmalloc INTERFACE)
        target_include_directories(comdare_vendor_snmalloc INTERFACE "${_sn_root}/src")
        target_compile_definitions(comdare_vendor_snmalloc INTERFACE
            SNMALLOC_HEADER_ONLY_LIBRARY=1
            SNMALLOC_USE_CXX17=0
            SNMALLOC_QEMU_WORKAROUND=0)
        if(NOT TARGET comdare::vendor_snmalloc)
            add_library(comdare::vendor_snmalloc ALIAS comdare_vendor_snmalloc)
        endif()
        set(COMDARE_HAVE_SNMALLOC ON CACHE BOOL "" FORCE)
        message(STATUS "V41.A2 snmalloc (header-only): ${_sn_root}/src")
    else()
        message(STATUS "V41.A2 snmalloc NICHT gefunden. Fallback (real=std).")
    endif()
endif()

# --- tcmalloc (Google, braucht Abseil + komplexes Setup) ---
option(COMDARE_VENDOR_TCMALLOC "tcmalloc Detection (system / Fallback)" ${COMDARE_BUILD_PERMUTATIONS})
set(COMDARE_HAVE_TCMALLOC OFF CACHE BOOL "" FORCE)
if(COMDARE_VENDOR_TCMALLOC)
    find_library(TCMALLOC_LIB NAMES tcmalloc tcmalloc_minimal libtcmalloc)
    find_path(TCMALLOC_INC gperftools/tcmalloc.h)
    if(TCMALLOC_LIB AND TCMALLOC_INC)
        add_library(comdare_vendor_tcmalloc INTERFACE)
        target_include_directories(comdare_vendor_tcmalloc INTERFACE "${TCMALLOC_INC}")
        target_link_libraries(comdare_vendor_tcmalloc INTERFACE "${TCMALLOC_LIB}")
        if(NOT TARGET comdare::vendor_tcmalloc)
            add_library(comdare::vendor_tcmalloc ALIAS comdare_vendor_tcmalloc)
        endif()
        set(COMDARE_HAVE_TCMALLOC ON CACHE BOOL "" FORCE)
        message(STATUS "V41.A2 tcmalloc (system): ${TCMALLOC_LIB}")
    else()
        message(STATUS "V41.A2 tcmalloc NICHT gefunden. Fallback (real=std).")
        message(STATUS "  Setup: Linux 'apt install libgoogle-perftools-dev'; vendored-Build braucht Abseil")
    endif()
endif()

# --- hoard (Emery Berger, eigene CMakeLists vorhanden aber komplex) ---
option(COMDARE_VENDOR_HOARD "hoard Detection (system / Fallback)" ${COMDARE_BUILD_PERMUTATIONS})
set(COMDARE_HAVE_HOARD OFF CACHE BOOL "" FORCE)
if(COMDARE_VENDOR_HOARD)
    find_library(HOARD_LIB NAMES hoard libhoard)
    find_path(HOARD_INC hoard.h)
    if(HOARD_LIB)
        add_library(comdare_vendor_hoard INTERFACE)
        if(HOARD_INC)
            target_include_directories(comdare_vendor_hoard INTERFACE "${HOARD_INC}")
        endif()
        target_link_libraries(comdare_vendor_hoard INTERFACE "${HOARD_LIB}")
        if(NOT TARGET comdare::vendor_hoard)
            add_library(comdare::vendor_hoard ALIAS comdare_vendor_hoard)
        endif()
        set(COMDARE_HAVE_HOARD ON CACHE BOOL "" FORCE)
        message(STATUS "V41.A2 hoard (system): ${HOARD_LIB}")
    else()
        message(STATUS "V41.A2 hoard NICHT gefunden. Fallback (real=std).")
        message(STATUS "  Setup: vendored ext/A01-hoard Build via add_subdirectory (komplexes Linker-Setup)")
    endif()
endif()

# --- scalloc (separat, kein CMake, gyp-based; nur Detection) ---
option(COMDARE_VENDOR_SCALLOC "scalloc Detection (system / Fallback)" ${COMDARE_BUILD_PERMUTATIONS})
set(COMDARE_HAVE_SCALLOC OFF CACHE BOOL "" FORCE)
if(COMDARE_VENDOR_SCALLOC)
    find_library(SCALLOC_LIB NAMES scalloc libscalloc)
    if(SCALLOC_LIB)
        add_library(comdare_vendor_scalloc INTERFACE)
        target_link_libraries(comdare_vendor_scalloc INTERFACE "${SCALLOC_LIB}")
        if(NOT TARGET comdare::vendor_scalloc)
            add_library(comdare::vendor_scalloc ALIAS comdare_vendor_scalloc)
        endif()
        set(COMDARE_HAVE_SCALLOC ON CACHE BOOL "" FORCE)
        message(STATUS "V41.A2 scalloc (system): ${SCALLOC_LIB}")
    else()
        message(STATUS "V41.A2 scalloc NICHT gefunden. Fallback (real=std).")
        message(STATUS "  Setup: vendored ext/A08-scalloc nutzt gyp (Chrome legacy), kein CMake-Port verfuegbar")
    endif()
endif()
