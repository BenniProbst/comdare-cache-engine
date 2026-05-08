# engine_choice — Domaene 5 (Compile-Time Template-Wahl: StaticEngine vs CacheEngine)

add_library(comdare_engine_choice INTERFACE)
target_include_directories(comdare_engine_choice INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# template-only: static_engine.hpp + cache_engine.hpp werden in Phase 5 angelegt
