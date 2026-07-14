# V26.B/V31.K4 (2026-05-14) — A07-snmalloc Adapter
option(COMDARE_HAVE_SNMALLOC "Activate ext/allocator/A07-snmalloc/ (MIT)" OFF)
add_library(comdare_adapter_a07_snmalloc INTERFACE)
add_library(comdare::adapter::a07_snmalloc ALIAS comdare_adapter_a07_snmalloc)
target_include_directories(comdare_adapter_a07_snmalloc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(comdare_adapter_a07_snmalloc INTERFACE cxx_std_23)
if(COMDARE_HAVE_SNMALLOC)
    target_compile_definitions(comdare_adapter_a07_snmalloc INTERFACE COMDARE_HAVE_SNMALLOC=1)
    target_include_directories(comdare_adapter_a07_snmalloc INTERFACE
        "${PROJECT_SOURCE_DIR}/ext/allocator/A07-snmalloc/src")
    # M-CE-21-Fix (#21): die zwingenden snmalloc-Compile-Belange haengen self-contained DIREKT am Adapter,
    # sodass sie an JEDEN Konsumenten propagieren — UNABHAENGIG davon, ob das ext/-Vendor-Target
    # comdare::vendor_snmalloc existiert (das entsteht nur unter COMDARE_VENDOR_SNMALLOC/BUILD_PERMUTATIONS).
    # Im Voll-Bau (messung_driver, COMDARE_HAVE_SNMALLOC=ON ohne den Vendor-Pfad) fehlten diese Flags sonst und
    # der header-only-snmalloc brach: aba.h „You must compile with -mcx16", combininglock.h „SNMALLOC_USE_WAIT_ON_ADDRESS
    # was not declared". Werte sind stringgleich zu ext/CMakeLists.txt -> CMake dedupliziert bei Doppel-Link, kein Konflikt.
    target_compile_definitions(comdare_adapter_a07_snmalloc INTERFACE
        SNMALLOC_HEADER_ONLY_LIBRARY=1
        SNMALLOC_USE_WAIT_ON_ADDRESS=0)
    # snmalloc verlangt auf x86-64 16-Byte-CAS (cmpxchg16b) -> -mcx16 (aba.h). ISA-gegatet analog Vendor-Target
    # und Voll-Plattform-Doktrin: auf ARM/RISC-V liefert die AAL ihren eigenen Atomic-Pfad (dortige Enablement =
    # Multi-Plattform-Matrix), daher -mcx16 NUR auf x86_64/AMD64.
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|amd64")
        target_compile_options(comdare_adapter_a07_snmalloc INTERFACE -mcx16)
    endif()
    # Falls das ext/-Vendor-Target existiert, zusaetzlich verlinken (traegt dieselben Belange stringgleich +
    # Threads/atomic etc.) — kein Duplikat/Konflikt dank CMake-Deduplizierung.
    if(TARGET comdare::vendor_snmalloc)
        target_link_libraries(comdare_adapter_a07_snmalloc INTERFACE comdare::vendor_snmalloc)
    endif()
endif()
