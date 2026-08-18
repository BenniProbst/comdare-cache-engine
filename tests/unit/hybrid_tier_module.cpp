// HY-A2 (2026-08-18) -- Hybrid-Binary-Modulquelle, ZIEL SearchAlgorithm.
//
// Die MODUL-AUTOR-Seite der Gattung HeuristikAdapter: erste .so des Baums, deren
// comdare_create_anatomy() KEIN plain Tier liefert, sondern einen Reroute-Proxy. Sie materialisiert
// via COMDARE_DEFINE_HYBRID_MODULE die SECHS extern-"C"-ABI-Pflicht-Symbole
// (comdare_anatomy_abi_version / comdare_anatomy_abi_magic / comdare_create_anatomy /
// comdare_destroy_anatomy + seit Q2/V-06 comdare_anatomy_gattung / comdare_anatomy_genus) --
// buchstabengleich zu denen der plain Tiere, damit derselbe gattungs-agnostische
// AnatomyModuleLoader sie ohne Aenderung laedt (Begruendung im Makro-Header).
//
// WARUM ES ZWEI HYBRID-MODULQUELLEN GIBT (diese + hybrid_tier_module_set.cpp): der Header deklariert
// ZWEI zulaessige Reroute-Ziele. Waere nur eines als .so gebaut, bliebe die zweite Deklaration eine
// BEHAUPTUNG -- Text, den nie ein Compiler gesehen hat. Erst die zweite Quelle macht aus "waere
// zulaessig" ein "ist gebaut". Beide sind bewusst winzig: die Belegung ist die Aussage, nicht der
// Inhalt.
//
// WAS SIE NICHT TUT: sie bindet kein Ziel. Ein frisch geladener Proxy hat sein Standard-Dock, aber
// noch keinen Antrieb daran -- das Binden gehoert dem Aufrufer, der die Ziel-.so ebenfalls geladen
// hat und beide Lebensdauern kennt (destroy-vor-dlclose). Eine Factory, die selbst ein Ziel suchte,
// muesste dafuer einen Lader IM Modul mitfuehren und haette damit eine zweite Ladestrecke geoeffnet.

#include <hybrid/hybrid_module_abi_v1.hpp>

COMDARE_DEFINE_HYBRID_MODULE(::comdare::cache_engine::anatomy::AnatomyGenus::SearchAlgorithm)
