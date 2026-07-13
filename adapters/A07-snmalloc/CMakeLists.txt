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
    # M-CE-21-Vervollstaendigung: das ext/-Vendor-Target traegt die zwingenden snmalloc-Compile-Belange
    # (SNMALLOC_HEADER_ONLY_LIBRARY, SNMALLOC_USE_WAIT_ON_ADDRESS=0, -mcx16 auf x86). Ohne dieses Link scheiterte
    # der Adapter beim erstmals-aktivierten snmalloc (aba.h „-mcx16", combininglock „WAIT_ON_ADDRESS"). Single-Source
    # der Config; Guard falls das Vendor-Target (COMDARE_VENDOR_SNMALLOC/BUILD_PERMUTATIONS) nicht existiert.
    if(TARGET comdare::vendor_snmalloc)
        target_link_libraries(comdare_adapter_a07_snmalloc INTERFACE comdare::vendor_snmalloc)
    endif()
endif()
