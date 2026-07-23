# ext/io/liburing/CMakeLists.txt -- G3 / #46b Lager-Gate, Scheibe B7.
#
# Vendored liburing (Tag liburing-2.6, siehe COMDARE-VENDOR-PROVENANCE.md) als kleines statisches
# Target comdare_vendored_liburing. Baut die fuenf Kern-Quellen im STANDARD-libc-Modus (Mode B:
# ohne config-host.h/CONFIG_NOLIBC -> Standard memset/malloc, keine nolibc.c/-nostdlib-Sonderfaelle).
# Nur gezogen, wenn ein Consumer es braucht (COMDARE_WRITER_BACKEND=io_uring bzw. der io_uring-Test).
#
# Der NOT-TARGET-Guard macht das add_subdirectory idempotent (Test-Pfad zieht es on-demand herein;
# eine spaetere Produktions-Verdrahtung darf es erneut adden, ohne Doppel-Definition).

if(NOT TARGET comdare_vendored_liburing)
    enable_language(C)
    add_library(comdare_vendored_liburing STATIC
        "${CMAKE_CURRENT_LIST_DIR}/src/setup.c"
        "${CMAKE_CURRENT_LIST_DIR}/src/queue.c"
        "${CMAKE_CURRENT_LIST_DIR}/src/register.c"
        "${CMAKE_CURRENT_LIST_DIR}/src/syscall.c"
        "${CMAKE_CURRENT_LIST_DIR}/src/version.c")
    # SYSTEM PUBLIC: Consumer bekommen <liburing.h> als SYSTEM-Header -> die projektweiten C++-Warnungen
    # (-Wpedantic: zero-size arrays / flexible array members im UAPI-Header) treffen den Vendor-Header
    # NICHT. PRIVATE: interne Header (lib.h/syscall.h) nur beim Lib-Bau.
    target_include_directories(comdare_vendored_liburing SYSTEM PUBLIC "${CMAKE_CURRENT_LIST_DIR}/src/include")
    target_include_directories(comdare_vendored_liburing PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src")
    target_compile_definitions(comdare_vendored_liburing PRIVATE
        _GNU_SOURCE _LARGEFILE_SOURCE _FILE_OFFSET_BITS=64)
    set_target_properties(comdare_vendored_liburing PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        FOLDER "vendor")
    add_library(comdare::vendored_liburing ALIAS comdare_vendored_liburing)
endif()
