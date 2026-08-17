# K2 (Owner-Entscheid 09.08.2026) -- die STUFEN-NEUTRALE Antriebs-Schicht.
#
# Owner woertlich: "der Loader wandert in eine stufen-neutrale Bibliothek, weil das Pruefdock der
# CEB und das Pruefdock der Hybrid-Tier-Binary jeweils technisch identisch bei Konfiguration sein
# muessen."
#
# WAS DIESE LIB IST: die ABI-Kante zum geladenen Modul -- Status-Codes + Antriebs-Buendel.
# WAS SIE NICHT IST: ein Dock. IPruefDock, das Konformitaets-Gate, die Registry und der Sequencer
# bleiben in comdare::ce_pruef_dock; die Doktrin-Zeile "IPruefDock lebt nur im Builder-Binary"
# bleibt damit woertlich wahr.
#
# INTERFACE-Target, weil beide Header header-only sind. Die einzige kompilierte Abhaengigkeit ist
# der Loader (AnatomyModuleHandle::unload() lebt in dessen .cpp) -- und der ist bereits eine eigene,
# builder-unabhaengige Lib; er liegt nur physisch unter builder/. Damit zieht ein Konsument dieser
# Lib KEINEN Builder-Code.

add_library(comdare_ce_anatomy_drive INTERFACE)
add_library(comdare::ce_anatomy_drive ALIAS comdare_ce_anatomy_drive)

target_include_directories(comdare_ce_anatomy_drive INTERFACE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")

target_compile_features(comdare_ce_anatomy_drive INTERFACE cxx_std_23)

# Der Drive konstruiert/liest AnatomyModuleHandle -> Loader-Lib linken (unload() im .cpp).
# HART, NICHT if(TARGET) -- A2.5-Fix F-2: dieselbe reihenfolge-fragile Bauform wie im Pruefdock.
# Sie war hier heute ZUFAELLIG wahr (builder/ wird vorher eingebunden); "zufaellig wahr" ist kein
# Zustand, auf den sich eine Pflicht-Kante stuetzen darf. Eine fehlende Loader-Kante faellt sonst
# erst am Linker auf -- und zwar bei einem beliebigen KONSUMENTEN, nicht hier.
target_link_libraries(comdare_ce_anatomy_drive INTERFACE
    comdare::anatomy_module_loader
    Boost::mp11)
