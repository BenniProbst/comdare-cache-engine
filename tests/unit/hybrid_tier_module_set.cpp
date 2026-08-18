// HY-A2 (2026-08-18) -- Hybrid-Binary-Modulquelle, ZIEL Set. Zwilling von hybrid_tier_module.cpp.
//
// Der Zweck dieser zweiten Quelle steht dort ausfuehrlich: sie belegt, dass die ZWEITE
// Ziel-Deklaration (RerouteZiel<Set>, hybrid_binary_proxy.hpp) baubar ist und nicht bloss
// dasteht. Ein Ziel aus einer ANDEREN Gattung als die erste (Set = Container-Gattung,
// SearchAlgorithm = Gattung Map) belegt zugleich, dass der Reroute nicht an einer Gattung klebt.
//
// DASS BEIDE .so DIESELBEN VIER SYMBOLNAMEN EXPORTIEREN, ist kein Konflikt: es sind getrennte
// Objekte mit getrenntem dlopen-Handle. Genau darauf beruht die eine Ladestrecke.

#include <hybrid/hybrid_module_abi_v1.hpp>

COMDARE_DEFINE_HYBRID_MODULE(::comdare::cache_engine::anatomy::AnatomyGenus::Set)
