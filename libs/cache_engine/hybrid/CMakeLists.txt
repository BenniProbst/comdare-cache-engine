# HY-A (A2.5-Fix F-10) -- der CMake-TRAEGER der Hybrid-Stufe.
#
# WARUM ES IHN JETZT GIBT, obwohl hybrid/ header-only ist: VIER Test-Bloecke haben die immer
# gleichen fuenf Include-Wurzeln handgefuehrt dupliziert (hy_a1_dock_contract, hy_a3_config_parser,
# hy_a1_contract_token_negativ, k2_stufen_neutraler_loader). Vier Kopien derselben Liste sind vier
# Stellen, an denen eine kuenftige Wurzel vergessen wird -- und das faellt dann nur in dem einen
# Test auf, den gerade jemand anfasst.
#
# Der frueher hier stehende Verzicht ("header-only, kein add_subdirectory noetig") verwechselte
# zwei Dinge: ob eine Stufe KOMPILIERTE Objekte hat (nein) und ob sie einen Include-/Link-VERTRAG
# hat (ja -- sie zieht anatomy/, den stufen-neutralen Drive und Boost.mp11). Das INTERFACE-Target
# traegt den Vertrag, ohne eine einzige .cpp zu erzeugen.
#
# K2-KANTE: hybrid/ spricht seine Ziele ueber die STUFEN-NEUTRALE Schicht an, nicht ueber den
# Builder. Diese Zeile ist die maschinelle Fassung genau dieser Aussage -- wer hier den Builder
# ergaenzt, macht die K2-Entscheidung rueckgaengig und sieht es an dieser Stelle.

add_library(comdare_ce_hybrid INTERFACE)
add_library(comdare::ce_hybrid ALIAS comdare_ce_hybrid)

target_include_directories(comdare_ce_hybrid INTERFACE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    # G3/A-03: die Stempel-Kette (hybrid_stempel_kette.hpp) zieht anatomy_fingerprint.hpp, und der
    # traegt seit E-A/B-6 (18.08.2026) den consteval-SHA-256 (<sha256/ctsha.hpp>) -- dessen Wurzel
    # ist libs/cache_engine/src. Sie steht HIER im Vertrag statt in den Konsumenten: vier
    # handgefuehrte Kopien derselben Liste waren genau der F-10-Anlass dieses Targets.
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated")

target_compile_features(comdare_ce_hybrid INTERFACE cxx_std_23)

# HART, nicht if(TARGET) -- siehe F-2-Begruendung in den Nachbar-Listen.
target_link_libraries(comdare_ce_hybrid INTERFACE
    comdare::ce_anatomy_drive
    Boost::mp11)
