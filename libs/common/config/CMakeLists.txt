# V30.D.3 (2026-05-14): engine_choice/ -> libs/common/config/
# Compile-Time Template-Wahl: StaticEngine vs CacheEngine
# (frueher: engine_choice/ Top-Level)

add_library(comdare_engine_choice INTERFACE)
add_library(comdare::common::config ALIAS comdare_engine_choice)
target_include_directories(comdare_engine_choice INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# template-only: static_engine.hpp + cache_engine.hpp werden in Phase 5 angelegt
