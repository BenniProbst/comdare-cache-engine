# ext/io/zlib/CMakeLists.txt -- A9-S1 (xlsx-Writer, Vendor-Scheibe).
#
# Vendored zlib (Tag v1.3.2, siehe COMDARE-VENDOR-PROVENANCE.md) als kleines statisches Target
# comdare_vendored_zlib. Quellliste 1:1 aus dem Upstream-CMake (ZLIB_SRCS, CMakeLists.txt:135-150
# am Tag v1.3.2) uebernommen -- 15 Kern-Quellen, keine contrib-/Beispiel-/Test-Anteile.
#
# Warum mit-vendorn statt find_package(ZLIB): self-contained ueber die 8er-Docker-Matrix, keine
# zlib-dev-Header-Annahme auf Hosts und neuen Plattformen (RISC-V/macOS). Umkehrbar -- ein Consumer,
# der System-zlib will, zieht dieses Verzeichnis schlicht nicht herein.
#
# Der NOT-TARGET-Guard macht das add_subdirectory idempotent (mehrere Consumer duerfen es on-demand
# hereinziehen, ohne Doppel-Definition) -- exakt das liburing-Muster (ext/io/liburing/CMakeLists.txt).
#
# KEINE Praeprozessor-Schalter fuer HAVE_UNISTD_H / _LARGEFILE64_SOURCE: diese Makros wirken in
# zconf.h, also im OEFFENTLICHEN Header. Wuerde die Bibliothek mit anderen Werten uebersetzt als ihre
# Consumer, driftete z_off_t zwischen beiden Seiten auseinander. Der Default-Satz (keiner gesetzt) ist
# auf beiden Seiten identisch und traegt deflate/inflate vollstaendig; die gz*-Grossdatei-API wird von
# libxlsxwriter nicht benutzt.

if(NOT TARGET comdare_vendored_zlib)
    enable_language(C)
    add_library(comdare_vendored_zlib STATIC
        "${CMAKE_CURRENT_LIST_DIR}/adler32.c"
        "${CMAKE_CURRENT_LIST_DIR}/compress.c"
        "${CMAKE_CURRENT_LIST_DIR}/crc32.c"
        "${CMAKE_CURRENT_LIST_DIR}/deflate.c"
        "${CMAKE_CURRENT_LIST_DIR}/gzclose.c"
        "${CMAKE_CURRENT_LIST_DIR}/gzlib.c"
        "${CMAKE_CURRENT_LIST_DIR}/gzread.c"
        "${CMAKE_CURRENT_LIST_DIR}/gzwrite.c"
        "${CMAKE_CURRENT_LIST_DIR}/inflate.c"
        "${CMAKE_CURRENT_LIST_DIR}/infback.c"
        "${CMAKE_CURRENT_LIST_DIR}/inftrees.c"
        "${CMAKE_CURRENT_LIST_DIR}/inffast.c"
        "${CMAKE_CURRENT_LIST_DIR}/trees.c"
        "${CMAKE_CURRENT_LIST_DIR}/uncompr.c"
        "${CMAKE_CURRENT_LIST_DIR}/zutil.c")
    # SYSTEM PUBLIC: Consumer bekommen <zlib.h> als SYSTEM-Header, damit die projektweiten
    # C/C++-Warnungen den Vendor-Header nicht treffen (liburing-Praezedenz).
    target_include_directories(comdare_vendored_zlib SYSTEM PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
    set_target_properties(comdare_vendored_zlib PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        FOLDER "vendor")
    add_library(comdare::vendored_zlib ALIAS comdare_vendored_zlib)
endif()
