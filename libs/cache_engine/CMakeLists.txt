# cache_engine — Domaene 2 (KERNBEITRAG)
# CacheEngineBuilder + CacheEngine-Singleton (Heap, im Builder)

add_subdirectory(builder)
# K2 (Owner 09.08.2026): die stufen-neutrale Antriebs-Schicht. NACH builder/, weil sie dessen
# Loader-Lib linkt -- der Loader ist builder-unabhaengig und liegt dort nur physisch.
add_subdirectory(anatomy_drive)
# HY-A (F-10): der Include-/Link-Vertrag der Hybrid-Stufe als INTERFACE-Target. Header-only,
# aber mit Vertrag -- die vier Test-Bloecke haengen ab jetzt daran statt an vier Kopien.
add_subdirectory(hybrid)
add_subdirectory(profile_facade)
add_subdirectory(concurrency_manager)
add_subdirectory(subsystems)
add_subdirectory(reclamation)
