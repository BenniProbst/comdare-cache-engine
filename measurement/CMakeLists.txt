# measurement — Domaene 3 (Mikrobenchmarks-Bibliothek)

add_library(comdare_measurement INTERFACE)
target_include_directories(comdare_measurement INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(perf_wrapper)
add_subdirectory(papi_wrapper)
add_subdirectory(advisor_wrapper)
add_subdirectory(hdr_histogram_wrapper)
add_subdirectory(dataset_loader)
add_subdirectory(platform_probe)
add_subdirectory(run_recorder)
