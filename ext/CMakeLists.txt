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

# V40.B (2026-05-24): jemalloc — versuche find_package (z.B. system-installed
# auf Linux via apt/yum), sonst Fallback ohne echtes Link. jemalloc-Permutations
# laufen dann ohne echten je_malloc-Aufruf (compile-time check).
option(COMDARE_VENDOR_JEMALLOC "Versuche jemalloc zu finden (ext/A05-jemalloc oder system)"
       ${COMDARE_BUILD_PERMUTATIONS})
set(COMDARE_HAVE_JEMALLOC OFF CACHE BOOL "" FORCE)
if(COMDARE_VENDOR_JEMALLOC)
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
        message(STATUS "V40.B jemalloc gefunden: ${JEMALLOC_LIB} (${JEMALLOC_INC})")
    else()
        message(STATUS "V40.B jemalloc NICHT gefunden — alloc=jemalloc Permutationen fallen auf std zurueck")
    endif()
endif()
