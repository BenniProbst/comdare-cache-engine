# measurement/dataset_loader — Dataset-Loader-Slot (AP-CE2)
# Header-only INTERFACE-Bibliothek: DatasetLoaderStrategy + DatasetLoaderRegistry +
# load_or_generate_ycsb(). Bildet Nicht-YCSB-Datensaetze (SOSD/TPC/SPEC/CloudSuite/gem5/
# Allokator-Suiten) auf das comdare Operation[uint64]-Modell ab.
#
# Konkrete Framework-Loader sind separate Header (include/.../loaders/) und registrieren sich
# via inline-Variable. Verdrahtung in Phase 5 (phase5_run_workload): siehe README.md.

add_library(comdare_dataset_loader INTERFACE)
target_include_directories(comdare_dataset_loader INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(comdare_dataset_loader INTERFACE comdare_workload_generator)
target_compile_features(comdare_dataset_loader INTERFACE cxx_std_23)
