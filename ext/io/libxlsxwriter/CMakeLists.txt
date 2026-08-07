# ext/io/libxlsxwriter/CMakeLists.txt -- A9-S1 (xlsx-Writer, Vendor-Scheibe).
#
# Vendored libxlsxwriter (Tag v1.2.4, siehe COMDARE-VENDOR-PROVENANCE.md) als kleines statisches
# Target comdare_vendored_xlsxwriter. Quell- und Define-Satz 1:1 aus dem Upstream-Bau abgeleitet
# (CMakeLists.txt:351-376 + src/Makefile am Tag v1.2.4); gebaut wird reines C.
#
# Consumer-only wie liburing: das Target wird NUR gezogen, wenn ein Consumer es hereinholt. Kein
# bestehendes ce-Target haengt daran, der golden-/Mess-Pfad bleibt unberuehrt.
#
# BAU-SCHALTER (Begruendung im Design-Dossier docs/architecture/20260803-a9_xlsx_writer_f3_soll_design.md,
# Abschnitt 3.2):
#   USE_STANDARD_TMPFILE -- POSIX tmpfile() statt des gebuendelten tmpfileplus. Damit wird KEIN
#                           MPL-2.0-Code uebersetzt oder gelinkt (tmpfileplus.c ist geprunt).
#   USE_DTOA_LIBRARY     -- Milo-Yip-dtoa (MIT) statt sprintf() fuer double-Ausgabe: locale-unabhaengig
#                           und byte-deterministisch, also Voraussetzung fuer den Determinismus-Hauswert.
#   NOCRYPT / NOUNCRYPT  -- minizip ohne Verschluesselungs-Zweig (Upstream-Default fuer libxlsxwriter).
# NICHT gesetzt: USE_FMEMOPEN (open_memstream) -- der tmpfile()-Weg traegt Linux und macOS gleich und
# haelt den Snapshot frei von plattformabhaengigen Zweigen.

if(NOT TARGET comdare_vendored_xlsxwriter)
    enable_language(C)

    # zlib ist die einzige externe Abhaengigkeit; sie ist unter ext/io/zlib mit-vendoriert.
    if(NOT TARGET comdare_vendored_zlib)
        add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../zlib" "${CMAKE_BINARY_DIR}/_ext_io_zlib")
    endif()

    file(GLOB COMDARE_LXW_SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/*.c")
    add_library(comdare_vendored_xlsxwriter STATIC
        ${COMDARE_LXW_SOURCES}
        "${CMAKE_CURRENT_LIST_DIR}/third_party/minizip/ioapi.c"
        "${CMAKE_CURRENT_LIST_DIR}/third_party/minizip/zip.c"
        "${CMAKE_CURRENT_LIST_DIR}/third_party/md5/md5.c"
        "${CMAKE_CURRENT_LIST_DIR}/third_party/dtoa/emyg_dtoa.c")

    # SYSTEM PUBLIC: Consumer bekommen <xlsxwriter.h> als SYSTEM-Header -> die projektweiten
    # C++-Warnungen treffen den Vendor-Header nicht (liburing-Praezedenz).
    target_include_directories(comdare_vendored_xlsxwriter SYSTEM PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/include")
    target_compile_definitions(comdare_vendored_xlsxwriter PRIVATE
        USE_STANDARD_TMPFILE USE_DTOA_LIBRARY NOCRYPT NOUNCRYPT)
    # zlib PRIVATE: <zlib.h> gehoert zum Bau von minizip/packager, nicht zur Consumer-Schnittstelle.
    target_link_libraries(comdare_vendored_xlsxwriter PRIVATE comdare::vendored_zlib)
    set_target_properties(comdare_vendored_xlsxwriter PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        FOLDER "vendor")
    add_library(comdare::vendored_xlsxwriter ALIAS comdare_vendored_xlsxwriter)
endif()
