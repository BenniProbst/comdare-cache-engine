# cache_engine/builder/algorithm_visitor — Sub-Komponente (LEERER PLATZHALTER)
# Skelett (Phase 4.B) — keine Implementation.
#
# (Audit K10 / Pattern-Integritaet, 2026-06-13): Es gibt KEIN GoF-Visitor-Muster im Code (kein accept()/
# visit()/Double-Dispatch). Der fruehere "hybrider Visitor"-Anspruch (Task #44 / V5-I6) ist ein Planungs-
# Artefakt und NICHT umgesetzt. Der reale Mess-Pfad nutzt direkte tier_*-Aufrufe (abi_adapter) + das Pull-
# Observer-Interface (IObservableTier::tier_observe), KEINE Visitor-Traversierung. Dieser INTERFACE-Platzhalter
# ist bewusst leer; bei kuenftiger Visitor-Einfuehrung hier ein echtes accept/visit-Protokoll anlegen oder den
# Ordner entfernen. Disposition: docs/sessions/20260613-K10-pattern-integritaet-disposition.md.

add_library(comdare_algorithm_visitor INTERFACE)
target_include_directories(comdare_algorithm_visitor INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
