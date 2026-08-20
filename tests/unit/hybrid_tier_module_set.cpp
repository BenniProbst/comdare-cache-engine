// HY-A2 (2026-08-18) -- Hybrid-Binary-Modulquelle, ZIEL Set. Zwilling von hybrid_tier_module.cpp.
//
// Der Zweck dieser zweiten Quelle steht dort ausfuehrlich: sie belegt, dass die ZWEITE
// Ziel-Deklaration (RerouteZiel<Set>, hybrid_binary_proxy.hpp) baubar ist und nicht bloss
// dasteht. Ein Ziel aus einer ANDEREN Gattung als die erste (Set = Container-Gattung,
// SearchAlgorithm = Gattung Map) belegt zugleich, dass der Reroute nicht an einer Gattung klebt.
//
// DASS BEIDE .so DIESELBEN SECHS SYMBOLNAMEN EXPORTIEREN, ist kein Konflikt: es sind getrennte
// Objekte mit getrenntem dlopen-Handle. Genau darauf beruht die eine Ladestrecke.

#include <hybrid/hybrid_module_abi_v1.hpp>

// cppcheck kennt die COMDARE-Codegen-Emitter-Makros nicht (Definition via Include-Kette, kein -I im Lint-Lauf).
// cppcheck-suppress unknownMacro
COMDARE_DEFINE_HYBRID_MODULE(::comdare::cache_engine::anatomy::AnatomyGenus::Set)

// A-11/golden-102 (19.08.2026): STEMPEL-PFLICHT -- der Loader weist stempellose Module mit status 13
// (version_lines_symbol_missing) ab; Emission ohne Stempel faellt. Diese Fixture traegt deshalb ihre
// EIGENEN, ehrlichen Zeilen (2-arg-Form: KEINE Mess-Deklaration -- sie kompiliert kein Tooling ein).
#include <cache_engine/abi/anatomy_module_abi_v1.hpp>
COMDARE_ANATOMY_VERSION_STAMP("system_fixture=hybrid_reroute_set@1.0.0.c", "fixture_kern=reroute_ziel_set@1.0.0.c")
