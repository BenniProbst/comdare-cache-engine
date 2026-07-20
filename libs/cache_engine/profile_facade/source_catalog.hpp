#pragma once
// STRANG A KORRIGIERT — Increment 4 / S4b (2026-06-18, profil-getrieben). source_catalog: der PROFIL-AGNOSTISCHE
// Anatomie-Quell-KATALOG (KEINE Selektions-Schicht mehr).
//
// Plan: docs/sessions/20260618-STRANG-A-KORRIGIERT-PROFIL-GETRIEBEN-PLAN.md (S4b: "SourceGen profil-getrieben";
// S5: "Code-Selektion ENTFERNEN"). Dieser Header ERSETZT die geloeschte Code-Selektions-Schicht — DIE GANZE
// „WELCHE Lebewesen/Achsen"-Selektion ist jetzt deklarativ im comdare_thesis_profile (build_axis_levels). Was HIER
// bleibt, ist KEINE Selektion, sondern die reine MATERIALISIERUNGS-DOMÄNE: der compile-time TYP→QUELLE-Katalog, der
// zu EINEM binary_id die REALE Modul-Quelle liefert (SourceGenFn). Der CacheEngineBuilder fragt den Katalog NUR
// über den binary_id ab (kein String→Typ-Dispatch, kein Code-Selektor). WELCHE binary_ids gebaut werden, bestimmt
// das Profil (tree.build + profile_select).
//
// WARUM ein compile-time Katalog architektonisch NÖTIG ist (und keine „versteckte Selektion"):
//   • Die reale, baubare, observierbare Anatomie kommt AUSSCHLIESSLICH TYP-getrieben
//     (anatomy::CompositionFromPermTuple<P> + COMDARE_DEFINE_ANATOMY_MODULE_ADHOC, BR-4). Aus einem reinen XML-String
//     lässt sich KEIN C++-Typ synthetisieren (Direktive: kein Runtime-Switch / String→Typ verboten).
//   • Auflösung (BR-2 / pilot_source_map.hpp): EINMAL host-seitig über die Achsen-Typ-Liste iterieren und je Typ den
//     serialisierten Pfad (== Baum-binary_id) → REALE Quelle in eine map ablegen. Diese map IST die SourceGenFn.
//   • Der Katalog deckt GENAU den Achsen-kartesischen Raum ab, den das Profil deklariert (search_algo × node_type ×
//     memory_layout × prefetch = 4·4·5·4 = 320, die 15 übrigen Slots je 1 Wert gepinnt). Round-Trip-Identität der
//     binary_ids ist golden-belegt (golden_fullpilot_320_binary_ids.txt + test_profile_roundtrip.cpp) → jeder
//     profil-selektierte binary_id ist im Katalog (sonst „Source-Fehler", ehrlich sichtbar).
//
// COMPILE-FEASIBILITY (Doc 27 §6): dieser EINE TU materialisiert NUR TYP-NAMEN (build_pilot_source_map → type_name<T>,
// KEINE Anatomie-Instanz) für ≤320 Permutationen; die SCHWERE Anatomie-Instanziierung passiert je separat emittiertem
// perm_<id>.cpp (eine DLL = ein TU = EINE Komposition). Lazy-Compile (1 DLL = 1 TU) bleibt damit erhalten.
//
// Umbrella-schwer (all_axes_umbrella via pilot_source_map.hpp) → gehört in die HARNESS-/Test-.cpp, NICHT in den
// engine-agnostischen Treiber-Header. C++23.

#include <builder/experiment_tree/cache_engine_builder_iterator.hpp> // run_lazy_static_then_dynamic
#include <builder/experiment_tree/pilot_source_map.hpp> // build_pilot_source_map / make_source_gen_from_map
#include <builder/experiment_tree/axis_reflect.hpp>     // push_static_axis (Katalog-Referenz-Levels)
#include <permutations/permutation_engine.hpp>          // PermutationEngine

// Die statischen Katalog-Achsen-ConfigSets (echte Wrapper):
#include <topics/traversal/topic_traversal_config_set.hpp>
// #188 per-K Increment 2b: explizite per-K-Wrapper-Typen (KArySearchAlgoK2/4/8/16) fuer den dedizierten per-K-Katalog
// (die search_algo-Achse ist NICHT vertieft -> die per-K stehen bewusst NICHT in den Basis-320-First-4). Transitiv via
// die Traversal-ConfigSet ohnehin sichtbar; hier EXPLIZIT als saubere Direkt-Abhaengigkeit.
#include <axes/lookup/axis_03a_search_algo_k_ary.hpp>
#include <topics/nodes/topic_nodes_config_set.hpp>
#include <topics/memory_layout/topic_memory_layout_config_set.hpp>
#include <topics/allocator/topic_allocator_config_set.hpp>
#include <topics/prefetch/topic_prefetch_config_set.hpp>
#include <topics/concurrency/topic_concurrency_config_set.hpp>
#include <topics/serialization/topic_serialization_config_set.hpp>
#include <topics/value_handle/topic_value_handle_config_set.hpp>
#include <topics/hardware/topic_hardware_config_set.hpp>
#include <topics/search_engine/topic_search_engine_config_set.hpp>
#include <topics/io/topic_io_config_set.hpp>
#include <topics/migration/topic_migration_config_set.hpp>
#include <topics/filter/topic_filter_config_set.hpp>
#include <topics/queuing/topic_queuing_config_set.hpp>

#include <boost/mp11.hpp>
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace comdare::cache_engine::thesis_lazy {

namespace ex   = ::comdare::cache_engine::builder::experiment;
namespace perm = ::comdare::cache_engine::permutations;
namespace ce   = ::comdare::cache_engine;
namespace mp   = boost::mp11;

// PermutationEngine erwartet je Topic ::StaticAxisVariants. Wir wickeln eine beliebige mp_list als ConfigSet.
template <class List>
struct CatalogCfg {
    using StaticAxisVariants = List;
};

// ── Die 17 AdHocComposition-Slot-Listen des Katalogs. KEINE Selektion — die VOLLE Achsen-kartesische Domäne.
//    golden_320_catalog (die alte m3v2_study.profile.xml <permute_axes>-Grundlage): variiert werden search_algo /
//    node_type / memory_layout / prefetch; die übrigen 13 sind je 1 Wert gepinnt (deckungsgleich zur Profil-
//    Deklaration → identische binary_ids = golden-320). 4·4·5·4 = 320 reale Kompositionen. FullSourceCatalog (das NEUE
//    golden ab 2026-07-18): variiert ALLE 17 Achsen je 2 = 2^17 = 131.072 (all_axes_golden.profile.xml).
//    (Die mp_take_c-Reihenfolge + die per-Slot-Wahl ist die zentrale Konvention, die die Profil-binary_ids reproduziert
//     — siehe golden_fullpilot_320_binary_ids.txt / golden_fullpilot_131072_binary_ids.txt; golden-gegated.) ──
// NEW-GOLDEN-ALL-AXES (2026-07-18, BAUPLAN-NEW-GOLDEN-ALL-AXES.md §4): CatalogAxes ist ueber ALLE 17 Kompositions-
// Achsen parametrisiert (K00..K16 = mp_take_c-Grad je Slot, kanonische L00..L16-Reihenfolge). Der golden-REFERENZ-
// Katalog FullSourceCatalog variiert ab jetzt ALLE 17 Achsen je 2 (N=2^17=131072 = Ganz-System-Regressions-Detektor);
// der messdaten-erhaltende golden_320_catalog behaelt die alte 4*4*5*4-Semantik. WICHTIG: Engine ist NUR ein Typ-Alias
// (PermutationEngine<...> wird NICHT instanziiert, solange niemand ::count()/::AllPermutations/for_each_permutation
// nennt bzw. build_pilot_source_map<Engine> aufruft) -> das mp_product-131072 wird NIE materialisiert. Zaehlung geht
// ausschliesslich ueber catalog_axis_product<> (reines ∏ mp_size, s.u.) bzw. den LAZY StaticBinaryView (view.size()).
template <std::size_t K00, std::size_t K01, std::size_t K02, std::size_t K03, std::size_t K04, std::size_t K05,
          std::size_t K06, std::size_t K07, std::size_t K08, std::size_t K09, std::size_t K10, std::size_t K11,
          std::size_t K12, std::size_t K13, std::size_t K14, std::size_t K15, std::size_t K16>
struct CatalogAxes {
    using L00 = mp::mp_take_c<ce::traversal::TopicConfigSet::StaticAxisVariants_03a, K00>; // search_algo
    using L01 = mp::mp_take_c<ce::traversal::TopicConfigSet::StaticAxisVariants_03b, K01>; // cache_traversal
    using L02 = mp::mp_take_c<ce::traversal::TopicConfigSet::StaticAxisVariants_03m, K02>; // mapping
    using L03 = mp::mp_take_c<ce::nodes::TopicConfigSet::StaticAxisVariants_02, K03>;      // path_compression
    using L04 = mp::mp_take_c<ce::nodes::TopicConfigSet::StaticAxisVariants_04, K04>;      // node_type
    using L05 = mp::mp_take_c<ce::memory_layout::TopicConfigSet::StaticAxisVariants, K05>; // memory_layout
    using L06 = mp::mp_take_c<ce::allocator::TopicConfigSet::StaticAxisVariants, K06>;     // allocator
    using L07 = mp::mp_take_c<ce::prefetch::TopicConfigSet::StaticAxisVariants, K07>;      // prefetch
    using L08 = mp::mp_take_c<ce::concurrency::TopicConfigSet::StaticAxisVariants, K08>;   // concurrency
    using L09 = mp::mp_take_c<ce::serialization::TopicConfigSet::StaticAxisVariants, K09>; // serialization
    using L10 = mp::mp_take_c<ce::value_handle::TopicConfigSet::StaticAxisVariants, K10>;  // value_handle
    using L11 = mp::mp_take_c<ce::search_engine::TopicConfigSet::StaticAxisVariants,
                              K11>;                                             // index_organization (INC-2d: isa raus)
    using L12 = mp::mp_take_c<ce::io::TopicConfigSet::StaticAxisVariants, K12>; // io_dispatch
    using L13 = mp::mp_take_c<ce::migration::TopicConfigSet::StaticAxisVariants, K13>;  // migration_policy
    using L14 = mp::mp_take_c<ce::filter::TopicConfigSet::StaticAxisVariants, K14>;     // filter
    using L15 = mp::mp_take_c<ce::queuing::TopicConfigSet::StaticAxisVariants_Q1, K15>; // queuing_q1
    using L16 = mp::mp_take_c<ce::queuing::TopicConfigSet::StaticAxisVariants_Q2, K16>; // queuing_q2

    using Engine =
        perm::PermutationEngine<CatalogCfg<L00>, CatalogCfg<L01>, CatalogCfg<L02>, CatalogCfg<L03>, CatalogCfg<L04>,
                                CatalogCfg<L05>, CatalogCfg<L06>, CatalogCfg<L07>, CatalogCfg<L08>, CatalogCfg<L09>,
                                CatalogCfg<L10>, CatalogCfg<L11>, CatalogCfg<L12>, CatalogCfg<L13>, CatalogCfg<L14>,
                                CatalogCfg<L15>, CatalogCfg<L16>>;
};

// NEW-GOLDEN-ALL-AXES: der golden-REFERENZ-Katalog variiert ALLE 17 Kompositions-Achsen je 2 (kartesisch) →
// N = 2^17 = 131.072 reale SA-Kompositionen = der Ganz-System-Regressions-Detektor. Dies ist die REFERENZ-Grundlage
// (Count-Guard + N-Zeilen-id-Datei + Roundtrip), NICHT der materialisierte Bau-/Mess-Katalog (der bleibt via
// generated_source_catalog.hpp/GeneratedFullSourceCatalog bei 320/kuratiert — s. catalog_codegen.cmake, m3v2-XML).
// mp_take_c<..., 2> ist fuer JEDE Achse gueltig (kleinstes enabled Inventar = mapping = 2). Es wird KEIN mp_product
// materialisiert (Engine ist nur ein Typ-Alias; Zaehlung via catalog_axis_product<>/view.size()).
using FullSourceCatalog = CatalogAxes<2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2>; // 2^17 = 131.072
// Messdaten-erhaltend: die ALTE 320-Semantik (search_algo×node_type×memory_layout×prefetch = 4·4·5·4; die uebrigen
// 13 Achsen je 1 Wert gepinnt) als benannter Alias. Deckungsgleich zum m3v2-Profil / GeneratedFullSourceCatalog →
// bleibt compile-verankert (static_assert unten) + test-verankert (test_limits/test_profile_roundtrip/test_smoke).
using golden_320_catalog = CatalogAxes<4, 1, 1, 1, 4, 5, 1, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1>; // 4·4·5·4 = 320
// Klein-Katalog (node_type×memory_layout je 2 = 4 Kompositionen): für den schnellen E2E-Treiber-Test
// (test_lazy_static_dynamic_driver), der real 4 DLLs baut+lädt+misst. KEINE Lauf-Selektion — nur eine kleine
// Materialisierungs-Domäne für den Test.
using SmallSourceCatalog = CatalogAxes<1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1>; // 2·2 = 4

// catalog_axis_product<Catalog>() — der EXPLOSIONSFREIE golden-Count: reines ∏ mp_size(L00..L16) (17 constexpr-
// Multiplikationen, KEINE mp_product-Materialisierung; dieselbe Kardinalitaets-Identitaet wie all_axes_matrix_count(),
// Doc 27 §4.1). Der Count der 2^17-Grundlage ist damit compile-billig belegbar, OHNE die 131.072-Element-Produkt-
// Typliste (PermutationEngine::AllPermutations) je zu bilden — das waere ein GB-Compile / g++-ICE.
template <class Catalog>
[[nodiscard]] constexpr std::size_t catalog_axis_product() noexcept {
    return mp::mp_size<typename Catalog::L00>::value * mp::mp_size<typename Catalog::L01>::value *
           mp::mp_size<typename Catalog::L02>::value * mp::mp_size<typename Catalog::L03>::value *
           mp::mp_size<typename Catalog::L04>::value * mp::mp_size<typename Catalog::L05>::value *
           mp::mp_size<typename Catalog::L06>::value * mp::mp_size<typename Catalog::L07>::value *
           mp::mp_size<typename Catalog::L08>::value * mp::mp_size<typename Catalog::L09>::value *
           mp::mp_size<typename Catalog::L10>::value * mp::mp_size<typename Catalog::L11>::value *
           mp::mp_size<typename Catalog::L12>::value * mp::mp_size<typename Catalog::L13>::value *
           mp::mp_size<typename Catalog::L14>::value * mp::mp_size<typename Catalog::L15>::value *
           mp::mp_size<typename Catalog::L16>::value;
}

// DER GEFORDERTE COMPILE-GUARD (BAUPLAN §4): das neue golden-Ziel N=2^17 + die erhaltene 320-Grundlage, beide
// compile-verankert und EXPLOSIONSFREI (∏ mp_size; NIE FullSourceCatalog::Engine::count() / mp_product bei 2^17).
static_assert(catalog_axis_product<FullSourceCatalog>() == 131072u,
              "NEW-GOLDEN-ALL-AXES: FullSourceCatalog muss alle 17 Achsen je 2 = 2^17 = 131072 abdecken.");
static_assert(catalog_axis_product<golden_320_catalog>() == 320u,
              "messdaten-erhaltend: golden_320_catalog muss die alte 4·4·5·4 = 320 Semantik behalten.");
static_assert(catalog_axis_product<SmallSourceCatalog>() == 4u, "SmallSourceCatalog = 2·2 = 4 (E2E-Treiber-Test).");

// NEW-GOLDEN CRC64-Anker (Feasibility, §0-GOAL CRC64): die CRC-64/ECMA-182 der 131.072 golden-binary_ids in
// kanonischer StaticBinaryView-Reihenfolge (je binary_id gefolgt von '\n', OHNE Kommentar-Header). ERSETZT die
// 62-MB-Datei golden_fullpilot_131072_binary_ids.txt als Test-Verankerung (die Datei kommt NICHT ins git — Repo-Bloat
// ueber die Mirrors + CI). test_limits regeneriert die ids ON-DEMAND (lazy StaticBinaryView) und prueft ihre CRC64
// gegen DIESEN committeten Wert. Neu-Ermittlung/Inspektion: `gen_golden_fullpilot --crc64` (druckt den Wert) bzw.
// `gen_golden_fullpilot <datei>` (materialisiert die Datei manuell). Aenderung nur im koordinierten ABI-Fenster.
inline constexpr std::uint64_t kNewGolden131072Crc64 = 0xF1C1F26A1232073BULL;

// ── GN-2 / §26.6 (Register-Kritik 9): NEGATIVER INSTANZIIERUNGS-GUARD an der Katalog-Naht ────────────────────
// Die golden-REFERENZ (FullSourceCatalog, 2^17) ist vom MATERIALISIERTEN Katalog (GeneratedFullSourceCatalog,
// 320/kuratiert) ENTKOPPELT -- bisher nur per Konvention/Doku (Verbots-Kommentare oben + an make_catalog_source_gen).
// Dieser Guard setzt die Entkopplung compile-time durch: die materialisierende Naht (make_catalog_source_gen ->
// build_pilot_source_map<Engine> -> mp_product) bricht ill-formed, BEVOR ein kuenftiger Codegen-/Repoint-Change
// die 2^17-Vollform je materialisieren kann (GB-TU / g++-ICE, NEW-GOLDEN-ALL-AXES.md:100). Beide Pruefungen sind
// EXPLOSIONSFREI (is_same + reines Produkt catalog_axis_product<>, KEINE Engine-/mp_product-Instanziierung).
//
// Kardinalitaets-Obergrenze der Materialisierung: verankert zwischen den BELEGTEN Punkten des Repos --
// 320 (golden_320_catalog, legitim materialisiert) < 4096 (diese Grenze, 2^12: Headroom fuer kuratierte
// Saetze) << 76.800 (belegt compile-brechend: die historische C1060-Rechnung 320*|mig|*|flt|*|vh|*|pc|,
// s. FF-Block unten) < 131.072 (2^17-Vollform = GB-TU/ICE). Eine bewusste Anhebung braucht einen Bauplan
// (Compile-Feasibility-Beleg), KEIN stilles Hochdrehen.
inline constexpr std::size_t kMaxMaterializableCatalogCardinality = 4096;

// Guard-des-Guards: die Grenze muss die 2^17-Vollform IMMER ausschliessen und die 320-Basis IMMER zulassen --
// wer FullSourceCatalog vergroessert oder die Grenze verschiebt, bricht bewusst HIER (nicht still im Codegen).
static_assert(catalog_axis_product<FullSourceCatalog>() > kMaxMaterializableCatalogCardinality,
              "GN-2/§26.6: die Materialisierungs-Grenze muss die golden-N-Vollform (2^17) ausschliessen.");
static_assert(catalog_axis_product<golden_320_catalog>() <= kMaxMaterializableCatalogCardinality,
              "GN-2/§26.6: die Materialisierungs-Grenze muss den 320-Basis-Katalog zulassen.");

/// catalog_static_levels<Catalog>() — die 17 STATISCHEN AxisLevels des Katalogs (Bau-INC-2c: telemetry / Bau-INC-2d: isa sind System-Achsen; gleiche reduzierte Listen + gleiche
/// Achsen-Namen-Reihenfolge wie der Katalog-Engine = die binary_ids, die der Katalog materialisiert). Das ist KEINE
/// Lauf-Selektion (der Lauf-Baum kommt aus build_axis_levels(Profil)) — es ist die REFERENZ-Level-Quelle, mit der die
/// committete golden_fullpilot_320_binary_ids.txt EINMAL erzeugt + im Test gegen den Profil-Pfad gegengeprüft wird.
/// DynamicDims werden hier NICHT angehängt (die binary_id nutzt nur statische Ebenen).
template <class Catalog>
[[nodiscard]] inline std::vector<ex::AxisLevel> catalog_static_levels() {
    std::vector<ex::AxisLevel> lv;
    ex::push_static_axis<typename Catalog::L00>(lv, "search_algo");
    ex::push_static_axis<typename Catalog::L01>(lv, "cache_traversal");
    ex::push_static_axis<typename Catalog::L02>(lv, "mapping");
    ex::push_static_axis<typename Catalog::L03>(lv, "path_compression");
    ex::push_static_axis<typename Catalog::L04>(lv, "node_type");
    ex::push_static_axis<typename Catalog::L05>(lv, "memory_layout");
    ex::push_static_axis<typename Catalog::L06>(lv, "allocator");
    ex::push_static_axis<typename Catalog::L07>(lv, "prefetch");
    ex::push_static_axis<typename Catalog::L08>(lv, "concurrency");
    ex::push_static_axis<typename Catalog::L09>(lv, "serialization");
    ex::push_static_axis<typename Catalog::L10>(lv, "value_handle");
    ex::push_static_axis<typename Catalog::L11>(lv, "index_organization"); // INC-2d: isa raus (war L11)
    ex::push_static_axis<typename Catalog::L12>(lv, "io_dispatch");
    ex::push_static_axis<typename Catalog::L13>(lv, "migration_policy");
    ex::push_static_axis<typename Catalog::L14>(lv, "filter");
    ex::push_static_axis<typename Catalog::L15>(lv, "queuing_q1");
    ex::push_static_axis<typename Catalog::L16>(lv, "queuing_q2");
    return lv;
}

/// make_catalog_source_gen() — die EINE profil-getriebene SourceGenFn (binary_id → reale Modul-Quelle). „Profil-
/// getrieben" heißt: der Treiber baut den Baum + die Selektion AUS DEM PROFIL (build_axis_levels / profile_select),
/// und fragt diesen Katalog NUR über den resultierenden binary_id ab. Der Katalog selbst trifft KEINE Auswahl — er
/// materialisiert (EINMAL, compile-time) den vollen deklarierten Achsenraum als Typ→Quelle-map (BR-2/BR-4). Ein
/// binary_id außerhalb des deklarierten Raums liefert eine leere Quelle (Orchestrator markiert die DLL als nicht
/// baubar — ehrlich sichtbar). Lazy-Compile (1 DLL = 1 TU) bleibt: je Quelle ein eigener perm_<id>.cpp.
// EXPLOSIONS-LEITPLANKE (NEW-GOLDEN-ALL-AXES): dies ist der MATERIALISIERENDE Pfad — build_pilot_source_map<Engine>
// KOMPLETTIERT PermutationEngine<...> und materialisiert damit AllPermutations (mp_product). Der Default-Katalog
// MUSS deshalb der kleine/kuratierte golden_320_catalog sein, NIE der 2^17-FullSourceCatalog (das waere ein GB-Compile
// / g++-ICE). Der Referenz-Count der 2^17-Grundlage laeuft ausschliesslich lazy (catalog_axis_product/view.size()).
template <class Catalog = golden_320_catalog>
[[nodiscard]] inline ex::SourceGenFn make_catalog_source_gen() {
    // GN-2 / §26.6: DER negative Instanziierungs-Guard (Definition s. kMaxMaterializableCatalogCardinality oben).
    // build_pilot_source_map<FullSourceCatalog::Engine> ist VERBOTEN -- und jede andere Katalog-Form jenseits
    // der Materialisierungs-Grenze ebenso. Beide Checks sind explosionsfrei (reines ∏ mp_size). Der heisse
    // Zweig steht in einem discarded-faehigen if-constexpr: im Fehlerfall wird das mp_product NICHT einmal
    // ANinstanziiert -- der Build bricht billig und laut mit den static_assert-Meldungen, statt im GB-TU/ICE.
    constexpr bool is_forbidden_full_form = std::is_same_v<Catalog, FullSourceCatalog>;
    constexpr bool within_materialization_bound =
        catalog_axis_product<Catalog>() <= kMaxMaterializableCatalogCardinality;
    if constexpr (!is_forbidden_full_form && within_materialization_bound) {
        return ex::make_source_gen_from_map(ex::build_pilot_source_map<typename Catalog::Engine>());
    } else {
        static_assert(!is_forbidden_full_form,
                      "GN-2/§26.6: build_pilot_source_map<FullSourceCatalog::Engine> ist VERBOTEN -- die 2^17-"
                      "golden-Referenz wird NIE materialisiert (GB-TU/ICE). Referenz-Zaehlung nur lazy via "
                      "catalog_axis_product<>/StaticBinaryView; materialisiert wird 320/kuratiert.");
        static_assert(within_materialization_bound,
                      "GN-2/§26.6: Katalog-Kardinalitaet jenseits der Materialisierungs-Grenze -- dieser Katalog "
                      "wuerde beim build_pilot_source_map-mp_product einen GB-TU/ICE ausloesen. Bewusste Anhebung "
                      "nur per Bauplan an kMaxMaterializableCatalogCardinality.");
        return {};
    }
}

/// catalog_levels<Catalog>(with_dynamic) — der GESAMT-Level-Satz fuer den E2E-Treiber-Test (Klein-Katalog): die 18
/// statischen Achsen + (optional) die 3 DynamicDims (concurrency.thread_count / prefetch.prefetch_distance /
/// repetition). Das ist KEINE Lauf-Selektion — der Produktiv-Treiber baut den Baum aus build_axis_levels(Profil);
/// diese Funktion dient NUR dem isolierten Treiber-Test (kleiner Baum ohne Profil-Parser).
template <class Catalog>
[[nodiscard]] inline std::vector<ex::AxisLevel> catalog_levels(bool with_dynamic = true, std::uint32_t n_repeats = 3) {
    std::vector<ex::AxisLevel> lv = catalog_static_levels<Catalog>();
    if (with_dynamic) {
        std::uint32_t const      reps = (n_repeats == 0) ? 1u : n_repeats;
        std::vector<std::string> rep_vals;
        rep_vals.reserve(reps);
        for (std::uint32_t r = 0; r < reps; ++r) rep_vals.push_back(std::to_string(r));
        lv.push_back(ex::AxisLevel{"concurrency", {"1", "2", "4"}, /*is_static=*/false, "thread_count", "concurrency"});
        lv.push_back(ex::AxisLevel{"prefetch", {"0", "8"}, /*is_static=*/false, "prefetch_distance", "prefetch"});
        lv.push_back(
            ex::AxisLevel{"repetition", std::move(rep_vals), /*is_static=*/false, "repetition_index", "repetition"});
    }
    return lv;
}

// ════════════════════════════════════════════════════════════════════════════════════════════════════════════
// STRANG A — Increment 7 / FF (#168, 2026-06-19): die 4 VERTIEFTEN Achsen sweep-faehig machen.
//
// PROBLEM (G5-Audit): FullSourceCatalog = CatalogAxes<4,4,5,4> PINNT path_compression(L03)/value_handle(L10)/
// migration_policy(L14)/filter(L15) je auf mp_take_c<…,1> → ihr Achsen-kartesischer Raum ist im Basis-320-Katalog
// genau 1 Wert. Das m3v2-Profil deklariert zwar <axis_sweep axis="…">, aber build_axis_levels(Profil) gibt diese
// Ebenen mit level_size==1 zurueck (im permute_axes je 1 <value> gepinnt) → profile_make_axis_sweep kann hoechstens
// die Baseline-DLL liefern, KEINE reale Achsen-Variation. FF (9-Achsen-Austauschbarkeit) fehlt damit real.
//
// LOESUNG (Plan-Direktive, KEINE Compile-Explosion): NICHT den Basis-320-Katalog auf das volle Kartesisch der 4
// vertieften Achsen freigeben (das waere 320×|mig|×|flt|×|vh|×|pc| compile-time → C1060). STATTDESSEN je vertiefte
// Achse EINE KLEINE, separate Sweep-Source-Map: die BASELINE-Komposition (alle 4 Basis-Achsen + 13 uebrige Slots
// auf Index 0 == take<…,1>) mit NUR DIESER EINEN vertieften Achse ueber ihre VOLLE Enabled-Liste variiert. Das sind
// |Achsen-Werte| Eintraege je Achse (migration 4, filter 4, value_handle 5, path_compression 3 = 16 zusaetzliche
// Typ-Materialisierungen, NICHT 320·…). Die per-Achse-Map wird mit der Basis-320-Map VEREINIGT (make_*-union); der
// axis_sweep waehlt aus der Union. binary_id = serialize_composition_path<P> (17 Achsen, voll), distinkt je
// Auspraegung (z.B. migration_policy=migration_hot_cold ≠ migration_none = ANDERE DLL). Lazy-Compile (1 DLL=1 TU)
// bleibt — die Union waehlt nur die richtige Quelle je binary_id.
//
// COMPILE-FEASIBILITY: jede Sweep-Engine materialisiert NUR |Achsen-Werte| Typ-NAMEN (build_pilot_source_map →
// type_name<T>, KEINE Anatomie-Instanz) — winzig. Die schwere Anatomie-Instanz passiert je separat emittiertem
// perm_<id>.cpp (cl), genau wie bei der Basis-320.
// ════════════════════════════════════════════════════════════════════════════════════════════════════════════

// ── Je vertiefte Achse ein Sweep-Katalog: identisch zur Baseline (CatalogAxes<1,1,1,1> → alle 4 Basis-Achsen +
//    14 uebrige Slots auf Index 0), ABER genau EINE vertiefte Achsen-Slot-Liste = die VOLLE Enabled-Liste. Die
//    mp_take_c-Konvention + Slot-Reihenfolge bleibt identisch zu CatalogAxes → der binary_id der Baseline-Auspraegung
//    (Index 0 dieser Achse) ist BYTE-IDENTISCH zur Basis-320-Baseline (= drop_tier-Baseline des Profils). ──
template <class SweepListL03, class SweepListL10, class SweepListL13, class SweepListL14>
struct AxisSweepCatalog {
    using L00 = mp::mp_take_c<ce::traversal::TopicConfigSet::StaticAxisVariants_03a, 1>; // search_algo (Baseline)
    using L01 = mp::mp_take_c<ce::traversal::TopicConfigSet::StaticAxisVariants_03b, 1>; // cache_traversal
    using L02 = mp::mp_take_c<ce::traversal::TopicConfigSet::StaticAxisVariants_03m, 1>; // mapping
    using L03 = SweepListL03;                                                       // path_compression (sweep|Baseline)
    using L04 = mp::mp_take_c<ce::nodes::TopicConfigSet::StaticAxisVariants_04, 1>; // node_type (Baseline)
    using L05 = mp::mp_take_c<ce::memory_layout::TopicConfigSet::StaticAxisVariants, 1>; // memory_layout (Baseline)
    using L06 = mp::mp_take_c<ce::allocator::TopicConfigSet::StaticAxisVariants, 1>;     // allocator
    using L07 = mp::mp_take_c<ce::prefetch::TopicConfigSet::StaticAxisVariants, 1>;      // prefetch (Baseline)
    using L08 = mp::mp_take_c<ce::concurrency::TopicConfigSet::StaticAxisVariants, 1>;   // concurrency
    using L09 = mp::mp_take_c<ce::serialization::TopicConfigSet::StaticAxisVariants, 1>; // serialization
    using L10 = SweepListL10; // value_handle (sweep|Baseline)
    using L11 = mp::mp_take_c<ce::search_engine::TopicConfigSet::StaticAxisVariants,
                              1>;                                             // index_organization (INC-2d: isa raus)
    using L12 = mp::mp_take_c<ce::io::TopicConfigSet::StaticAxisVariants, 1>; // io_dispatch
    using L13 = SweepListL13;                                                 // migration_policy (sweep|Baseline)
    using L14 = SweepListL14;                                                 // filter (sweep|Baseline)
    using L15 = mp::mp_take_c<ce::queuing::TopicConfigSet::StaticAxisVariants_Q1, 1>; // queuing_q1
    using L16 = mp::mp_take_c<ce::queuing::TopicConfigSet::StaticAxisVariants_Q2, 1>; // queuing_q2

    using Engine =
        perm::PermutationEngine<CatalogCfg<L00>, CatalogCfg<L01>, CatalogCfg<L02>, CatalogCfg<L03>, CatalogCfg<L04>,
                                CatalogCfg<L05>, CatalogCfg<L06>, CatalogCfg<L07>, CatalogCfg<L08>, CatalogCfg<L09>,
                                CatalogCfg<L10>, CatalogCfg<L11>, CatalogCfg<L12>, CatalogCfg<L13>, CatalogCfg<L14>,
                                CatalogCfg<L15>, CatalogCfg<L16>>;
};

// Die 4 Baseline-Slot-Listen (Index 0 == take<…,1>) — fuer die je 3 NICHT gesweepten der 4 vertieften Achsen.
using Pc1  = mp::mp_take_c<ce::nodes::TopicConfigSet::StaticAxisVariants_02, 1>;
using Vh1  = mp::mp_take_c<ce::value_handle::TopicConfigSet::StaticAxisVariants, 1>;
using Mg1  = mp::mp_take_c<ce::migration::TopicConfigSet::StaticAxisVariants, 1>;
using Flt1 = mp::mp_take_c<ce::filter::TopicConfigSet::StaticAxisVariants, 1>;
// Die 4 VOLLEN Enabled-Listen (alle Auspraegungen) — die jeweils GESWEEPTE Achse.
using PcAll = ce::nodes::TopicConfigSet::StaticAxisVariants_02;     // path_compression (none/patricia/byte_wise)
using VhAll = ce::value_handle::TopicConfigSet::StaticAxisVariants; // value_handle (inline/external_pool/…)
using MgAll = ce::migration::TopicConfigSet::StaticAxisVariants; // migration_policy (none/hot_cold/tier_based/adaptive)
using FltAll = ce::filter::TopicConfigSet::StaticAxisVariants;   // filter (bloom/cuckoo/range_surf/xor)

// Je 1 Sweep-Katalog: genau die eine vertiefte Achse voll, die anderen 3 vertieften + alle 4 Basis-Achsen Baseline.
using MigrationSweepCatalog       = AxisSweepCatalog<Pc1, Vh1, MgAll, Flt1>;
using FilterSweepCatalog          = AxisSweepCatalog<Pc1, Vh1, Mg1, FltAll>;
using ValueHandleSweepCatalog     = AxisSweepCatalog<Pc1, VhAll, Mg1, Flt1>;
using PathCompressionSweepCatalog = AxisSweepCatalog<PcAll, Vh1, Mg1, Flt1>;

// ── #18 Golden-Coverage: die 10 uebrigen Achsen (seit INC-2c ohne telemetry; nicht Basis-320, nicht vertieft) brauchen je einen eigenen
//    Sweep-Katalog, damit der Coverage-Test JEDE Achse beruehrt. Voll-17-Slot-parametrisiert (INC-2d: isa raus):
//    genau EIN Slot = volle Enabled-Liste, die uebrigen 16 = Baseline (Index 0) → byte-identische Baseline-DLL
//    (idempotenter Baseline-Key, disjunkter binary_id-Raum wie die 4 vertieften Sweeps). Slot-Reihenfolge == AxisSweepCatalog. ──
template <class P00, class P01, class P02, class P03, class P04, class P05, class P06, class P07, class P08, class P09,
          class P10, class P11, class P12, class P13, class P14, class P15, class P16>
struct AxisSweepCatalogFull {
    // L00..L16 als Member-Typen exponiert — catalog_static_levels<T> reflektiert T::L00..L16 (wie AxisSweepCatalog).
    using L00 = P00;
    using L01 = P01;
    using L02 = P02;
    using L03 = P03;
    using L04 = P04;
    using L05 = P05;
    using L06 = P06;
    using L07 = P07;
    using L08 = P08;
    using L09 = P09;
    using L10 = P10;
    using L11 = P11;
    using L12 = P12;
    using L13 = P13;
    using L14 = P14;
    using L15 = P15;
    using L16 = P16;
    using Engine =
        perm::PermutationEngine<CatalogCfg<L00>, CatalogCfg<L01>, CatalogCfg<L02>, CatalogCfg<L03>, CatalogCfg<L04>,
                                CatalogCfg<L05>, CatalogCfg<L06>, CatalogCfg<L07>, CatalogCfg<L08>, CatalogCfg<L09>,
                                CatalogCfg<L10>, CatalogCfg<L11>, CatalogCfg<L12>, CatalogCfg<L13>, CatalogCfg<L14>,
                                CatalogCfg<L15>, CatalogCfg<L16>>;
};

// Baseline (Index 0) je Slot — Reihenfolge identisch zu AxisSweepCatalog/CatalogAxes (INC-2d: isa raus, 17 Slots).
using B00 = mp::mp_take_c<ce::traversal::TopicConfigSet::StaticAxisVariants_03a, 1>; // search_algo
using B01 = mp::mp_take_c<ce::traversal::TopicConfigSet::StaticAxisVariants_03b, 1>; // cache_traversal
using B02 = mp::mp_take_c<ce::traversal::TopicConfigSet::StaticAxisVariants_03m, 1>; // mapping
using B03 = Pc1;                                                                     // path_compression
using B04 = mp::mp_take_c<ce::nodes::TopicConfigSet::StaticAxisVariants_04, 1>;      // node_type
using B05 = mp::mp_take_c<ce::memory_layout::TopicConfigSet::StaticAxisVariants, 1>; // memory_layout
using B06 = mp::mp_take_c<ce::allocator::TopicConfigSet::StaticAxisVariants, 1>;     // allocator
using B07 = mp::mp_take_c<ce::prefetch::TopicConfigSet::StaticAxisVariants, 1>;      // prefetch
using B08 = mp::mp_take_c<ce::concurrency::TopicConfigSet::StaticAxisVariants, 1>;   // concurrency
using B09 = mp::mp_take_c<ce::serialization::TopicConfigSet::StaticAxisVariants, 1>; // serialization
using B10 = Vh1;                                                                     // value_handle
using B11 =
    mp::mp_take_c<ce::search_engine::TopicConfigSet::StaticAxisVariants, 1>; // index_organization (INC-2d: isa raus)
using B12 = mp::mp_take_c<ce::io::TopicConfigSet::StaticAxisVariants, 1>;    // io_dispatch
using B13 = Mg1;                                                             // migration_policy
using B14 = Flt1;                                                            // filter
using B15 = mp::mp_take_c<ce::queuing::TopicConfigSet::StaticAxisVariants_Q1, 1>; // queuing_q1
using B16 = mp::mp_take_c<ce::queuing::TopicConfigSet::StaticAxisVariants_Q2, 1>; // queuing_q2

// Volle Enabled-Liste je gesweepter (nicht-Basis, nicht-vertiefter) Achse — die 9 uebrigen Achsen (INC-2d: isa raus).
using F01 = ce::traversal::TopicConfigSet::StaticAxisVariants_03b; // cache_traversal
using F02 = ce::traversal::TopicConfigSet::StaticAxisVariants_03m; // mapping
using F06 = ce::allocator::TopicConfigSet::StaticAxisVariants;     // allocator
using F08 = ce::concurrency::TopicConfigSet::StaticAxisVariants;   // concurrency
using F09 = ce::serialization::TopicConfigSet::StaticAxisVariants; // serialization
using F11 = ce::search_engine::TopicConfigSet::StaticAxisVariants; // index_organization (slot 11, INC-2d)
using F12 = ce::io::TopicConfigSet::StaticAxisVariants;            // io_dispatch (slot 12)
using F15 = ce::queuing::TopicConfigSet::StaticAxisVariants_Q1;    // queuing_q1 (slot 15)
using F16 = ce::queuing::TopicConfigSet::StaticAxisVariants_Q2;    // queuing_q2 (slot 16)

using CacheTraversalSweepCatalog =
    AxisSweepCatalogFull<B00, F01, B02, B03, B04, B05, B06, B07, B08, B09, B10, B11, B12, B13, B14, B15, B16>;
using MappingSweepCatalog =
    AxisSweepCatalogFull<B00, B01, F02, B03, B04, B05, B06, B07, B08, B09, B10, B11, B12, B13, B14, B15, B16>;
using AllocatorSweepCatalog =
    AxisSweepCatalogFull<B00, B01, B02, B03, B04, B05, F06, B07, B08, B09, B10, B11, B12, B13, B14, B15, B16>;
using ConcurrencySweepCatalog =
    AxisSweepCatalogFull<B00, B01, B02, B03, B04, B05, B06, B07, F08, B09, B10, B11, B12, B13, B14, B15, B16>;
using SerializationSweepCatalog =
    AxisSweepCatalogFull<B00, B01, B02, B03, B04, B05, B06, B07, B08, F09, B10, B11, B12, B13, B14, B15, B16>;
// (Bau-INC-2d: IsaSweepCatalog entfernt — isa ist Target-ISA-System-Achse, keine sweepbare Kompositions-Achse mehr.)
using IndexOrganizationSweepCatalog =
    AxisSweepCatalogFull<B00, B01, B02, B03, B04, B05, B06, B07, B08, B09, B10, F11, B12, B13, B14, B15, B16>;
using IoDispatchSweepCatalog =
    AxisSweepCatalogFull<B00, B01, B02, B03, B04, B05, B06, B07, B08, B09, B10, B11, F12, B13, B14, B15, B16>;
using QueuingQ1SweepCatalog =
    AxisSweepCatalogFull<B00, B01, B02, B03, B04, B05, B06, B07, B08, B09, B10, B11, B12, B13, B14, F15, B16>;
using QueuingQ2SweepCatalog =
    AxisSweepCatalogFull<B00, B01, B02, B03, B04, B05, B06, B07, B08, B09, B10, B11, B12, B13, B14, B15, F16>;

// ── #26/GO-5 (B.4.1-a, 2026-07-12): auch die 4 BASIS-Achsen (search_algo/node_type/memory_layout/prefetch)
//    bekommen je einen dedizierten Sweep-Katalog nach der #18-Schablone. GRUND: ein Smoke-/Coverage-Profil mit
//    GEPINNTER Basis (alle 17 Slots je 1 Wert, m3_smoke_coverage) kann die 4 Basis-Achsen NICHT ueber die
//    Basis-320-View sweepen (level_size==1) — ohne eigene Kataloge verloere der Smoke-Lauf genau diese 4 Achsen.
//    Jeder Katalog traegt die VOLLE Enabled-Liste im jeweiligen Slot gegen die Index-0-Baseline. Unter den
//    Default-Enables (4/4/5/4) sind die binary_ids eine TEILMENGE der golden-320 → idempotente Keys in der
//    Union (gleiche Quelle, gleiche DLL, KEIN golden-Bruch); Opt-in-Enables (z.B. per-K k_ary) erweitern sie
//    als END-Appends disjunkt. Konsum: run_profile Multi-Sweep-Durchlauf (profile_run_entry, B.4.1-b) +
//    test_smoke_coverage_profile (Binary-Zaehlungs-Gate). ──
using F00 = ce::traversal::TopicConfigSet::StaticAxisVariants_03a; // search_algo (volle Enabled-Liste)
using F04 = ce::nodes::TopicConfigSet::StaticAxisVariants_04;      // node_type
using F05 = ce::memory_layout::TopicConfigSet::StaticAxisVariants; // memory_layout
using F07 = ce::prefetch::TopicConfigSet::StaticAxisVariants;      // prefetch

using SearchAlgoSweepCatalog =
    AxisSweepCatalogFull<F00, B01, B02, B03, B04, B05, B06, B07, B08, B09, B10, B11, B12, B13, B14, B15, B16>;
using NodeTypeSweepCatalog =
    AxisSweepCatalogFull<B00, B01, B02, B03, F04, B05, B06, B07, B08, B09, B10, B11, B12, B13, B14, B15, B16>;
using MemoryLayoutSweepCatalog =
    AxisSweepCatalogFull<B00, B01, B02, B03, B04, F05, B06, B07, B08, B09, B10, B11, B12, B13, B14, B15, B16>;
using PrefetchSweepCatalog =
    AxisSweepCatalogFull<B00, B01, B02, B03, B04, B05, B06, F07, B08, B09, B10, B11, B12, B13, B14, B15, B16>;

/// axis_sweep_source_map(axis_name) — die KLEINE per-Achse Sweep-Quellen-map (binary_id → reale Modul-Quelle) der
/// gesweepten Achse. |Achsen-Werte| Eintraege; jeder ist eine reale AdHocComposition (Baseline + nur
/// diese Achse variiert). Unbekannte Achse → leere map (Caller faellt auf Basis-320 zurueck). KEINE Selektion —
/// reine Materialisierungs-Domaene EINER Achse. Seit #26/GO-5 fuer ALLE 18 Kompositions-Achsen (4 Basis +
/// 4 vertiefte + 10 uebrige; INC-2c: telemetry ist System-Achse).
[[nodiscard]] inline std::map<std::string, std::string> axis_sweep_source_map(std::string const& axis_name) {
    if (axis_name == "search_algo") return ex::build_pilot_source_map<typename SearchAlgoSweepCatalog::Engine>();
    if (axis_name == "node_type") return ex::build_pilot_source_map<typename NodeTypeSweepCatalog::Engine>();
    if (axis_name == "memory_layout") return ex::build_pilot_source_map<typename MemoryLayoutSweepCatalog::Engine>();
    if (axis_name == "prefetch") return ex::build_pilot_source_map<typename PrefetchSweepCatalog::Engine>();
    if (axis_name == "migration_policy") return ex::build_pilot_source_map<typename MigrationSweepCatalog::Engine>();
    if (axis_name == "filter") return ex::build_pilot_source_map<typename FilterSweepCatalog::Engine>();
    if (axis_name == "value_handle") return ex::build_pilot_source_map<typename ValueHandleSweepCatalog::Engine>();
    if (axis_name == "path_compression")
        return ex::build_pilot_source_map<typename PathCompressionSweepCatalog::Engine>();
    if (axis_name == "cache_traversal")
        return ex::build_pilot_source_map<typename CacheTraversalSweepCatalog::Engine>();
    if (axis_name == "mapping") return ex::build_pilot_source_map<typename MappingSweepCatalog::Engine>();
    if (axis_name == "allocator") return ex::build_pilot_source_map<typename AllocatorSweepCatalog::Engine>();
    if (axis_name == "concurrency") return ex::build_pilot_source_map<typename ConcurrencySweepCatalog::Engine>();
    if (axis_name == "serialization") return ex::build_pilot_source_map<typename SerializationSweepCatalog::Engine>();
    // (Bau-INC-2d: "isa" entfernt — Target-ISA-System-Achse, keine sweepbare Kompositions-Achse.)
    if (axis_name == "index_organization")
        return ex::build_pilot_source_map<typename IndexOrganizationSweepCatalog::Engine>();
    if (axis_name == "io_dispatch") return ex::build_pilot_source_map<typename IoDispatchSweepCatalog::Engine>();
    if (axis_name == "queuing_q1") return ex::build_pilot_source_map<typename QueuingQ1SweepCatalog::Engine>();
    if (axis_name == "queuing_q2") return ex::build_pilot_source_map<typename QueuingQ2SweepCatalog::Engine>();
    return {};
}

/// axis_sweep_levels(axis_name) — die 18 STATISCHEN AxisLevels des Sweep-Baums der gesweepten Achse:
/// 17 Baseline-Ebenen je 1 Wert (Index 0) + die gesweepte Achse mit ihrer VOLLEN Werteliste. Single-Source mit
/// axis_sweep_source_map (gleiche Slot-Listen → gleiche W::name()-Werte → die View-binary_ids treffen die map-Keys).
/// Die DynamicDims haengt der Treiber an (wie beim Basis-/SOTA-Baum). Leere Liste fuer unbekannte Achse.
[[nodiscard]] inline std::vector<ex::AxisLevel> axis_sweep_levels(std::string const& axis_name) {
    if (axis_name == "search_algo") return catalog_static_levels<SearchAlgoSweepCatalog>();
    if (axis_name == "node_type") return catalog_static_levels<NodeTypeSweepCatalog>();
    if (axis_name == "memory_layout") return catalog_static_levels<MemoryLayoutSweepCatalog>();
    if (axis_name == "prefetch") return catalog_static_levels<PrefetchSweepCatalog>();
    if (axis_name == "migration_policy") return catalog_static_levels<MigrationSweepCatalog>();
    if (axis_name == "filter") return catalog_static_levels<FilterSweepCatalog>();
    if (axis_name == "value_handle") return catalog_static_levels<ValueHandleSweepCatalog>();
    if (axis_name == "path_compression") return catalog_static_levels<PathCompressionSweepCatalog>();
    if (axis_name == "cache_traversal") return catalog_static_levels<CacheTraversalSweepCatalog>();
    if (axis_name == "mapping") return catalog_static_levels<MappingSweepCatalog>();
    if (axis_name == "allocator") return catalog_static_levels<AllocatorSweepCatalog>();
    if (axis_name == "concurrency") return catalog_static_levels<ConcurrencySweepCatalog>();
    if (axis_name == "serialization") return catalog_static_levels<SerializationSweepCatalog>();
    // (Bau-INC-2d: "isa" entfernt — Target-ISA-System-Achse.)
    if (axis_name == "index_organization") return catalog_static_levels<IndexOrganizationSweepCatalog>();
    if (axis_name == "io_dispatch") return catalog_static_levels<IoDispatchSweepCatalog>();
    if (axis_name == "queuing_q1") return catalog_static_levels<QueuingQ1SweepCatalog>();
    if (axis_name == "queuing_q2") return catalog_static_levels<QueuingQ2SweepCatalog>();
    return {};
}

/// is_deepened_axis(axis_name) — true fuer jede Achse, die eine eigene Sweep-Source-Map + eigene Sweep-Levels
/// traegt. Seit #26/GO-5 sind das ALLE 18 Kompositions-Achsen: die 4 vertieften + 10 uebrigen (historisch, #168/
/// #18) UND die 4 Basis-Achsen (search_algo/node_type/memory_layout/prefetch, B.4.1-a). Die Basis-Achsen-Sweeps
/// liefen frueher NUR ueber die Basis-320-View — das setzte eine materialisierte, VARIIERENDE Basis voraus und
/// verlor die 4 Achsen in jedem Profil mit gepinnter Basis. Unter den Default-Enables sind die Basis-Achsen-
/// Sweep-ids eine Teilmenge der golden-320 (byte-identische ids in gleicher Werte-Reihenfolge) → das bisherige
/// Einzel-Sweep-Verhalten (args.sweep_axis) bleibt id-identisch, nur die Quelle ist jetzt der eigene Katalog.
[[nodiscard]] inline bool is_deepened_axis(std::string const& axis_name) {
    return axis_name == "search_algo" || axis_name == "node_type" || axis_name == "memory_layout" ||
           axis_name == "prefetch" || axis_name == "migration_policy" || axis_name == "filter" ||
           axis_name == "value_handle" || axis_name == "path_compression" || axis_name == "cache_traversal" ||
           axis_name == "mapping" || axis_name == "allocator" || axis_name == "concurrency" ||
           axis_name == "serialization" || axis_name == "index_organization" || axis_name == "io_dispatch" ||
           axis_name == "queuing_q1" || axis_name == "queuing_q2"; // Bau-INC-2d: "isa" raus (System-Achse)
}

/// make_all_axis_sweeps_source_map() — die VEREINIGTE Sweep-Quellen-map ueber ALLE 17 Achsen-Sweeps (disjunkte
/// binary_id-Pfade, da je Achse nur sie selbst von der Baseline abweicht; die Baseline-Auspraegung jeder Achse ist
/// die GLEICHE Baseline-DLL → identischer binary_id → idempotente emplace, kein Konflikt). Eintragszahl =
/// 1 Baseline + Sum(je Achse USE-Enabled − 1); enable-/HAVE-abhaengig (ce-Default-Baum: 72 — Gate:
/// test_smoke_coverage_profile pinnt den Wert). Die Eintraege der 4 Basis-Achsen ueberlappen die
/// Basis-320-Quelle idempotent (union_gen fragt die Basis-320 zuerst).
[[nodiscard]] inline std::map<std::string, std::string> make_all_axis_sweeps_source_map() {
    std::map<std::string, std::string> all;
    for (char const* ax : {"search_algo", "node_type", "memory_layout", "prefetch", "migration_policy", "filter",
                           "value_handle", "path_compression", "cache_traversal", "mapping", "allocator", "concurrency",
                           "serialization", "index_organization", "io_dispatch", "queuing_q1", "queuing_q2"}) {
        auto m = axis_sweep_source_map(ax);
        for (auto& [k, v] : m) all.emplace(k, std::move(v)); // Baseline-Key kollidiert idempotent
    }
    return all;
}

// ══ #188 per-K Increment 2b (2026-07-01): dedizierter per-K-search_algo-Sweep-Katalog ═══════════════════════════
// search_algo war zum #188-Zeitpunkt KEINE vertiefte Achse (is_deepened_axis=false; seit #26/GO-5 hat sie einen
// eigenen SearchAlgoSweepCatalog ueber die EnabledStrategies — der per-K-Katalog hier bleibt der davon UNABHAENGIGE
// Opt-in-Kanal fuer die Default-OFF per-K-Wrapper). Historischer Kontext: die 4 Basis-Achsen liefen ueber den
// Basis-320-Katalog (L00 = mp_take_c<EnabledStrategies,4> = die First-4 [k_ary, interpolation, eytzinger, linear_scan]).
// Die per-K-Wrapper stehen aber bewusst am ENDE von AllStrategies + sind Default OFF -> NICHT in den First-4. Darum ein
// EIGENER Katalog mit EXPLIZITEM L00 = mp_list<KArySearchAlgoK2/4/8/16> (direkte Typen -> bypassen EnabledStrategies/
// mp_take_c; die per-K-Wrapper sind unabhaengig vom enable-Flag voll funktional). Alle 17 uebrigen Achsen Baseline
// (Index 0) = deckungsgleich zur 320-Baseline -> nur der search_algo-Slot variiert ueber die 4 per-K. Ergebnis: genau
// 4 reale Kompositionen (je K eine; container_traversal_t fuehrt jede ueber SEIN KAryTraversal<K>, Weg-A). Die Emission
// der 4 per-K-DLLs ist katalog-gated -> KEIN 320-Bruch (disjunkter binary_id-Raum, search_algo=k_ary_k*). Fuer den
// per-K-Mess-Lauf zusaetzlich -DCOMDARE_AXIS_03A_ENABLE_K_ARY_K{2,4,8,16}=ON (EnabledStrategies-Konsistenz +
// validate_profile) — der Katalog selbst braucht das NICHT (explizite Typen).
struct KaryPerKCatalog {
    using L00 = mp::mp_list<::comdare::cache_engine::lookup::KArySearchAlgoK2,
                            ::comdare::cache_engine::lookup::KArySearchAlgoK4,
                            ::comdare::cache_engine::lookup::KArySearchAlgoK8,
                            ::comdare::cache_engine::lookup::KArySearchAlgoK16>;         // search_algo (per-K Sweep)
    using L01 = mp::mp_take_c<ce::traversal::TopicConfigSet::StaticAxisVariants_03b, 1>; // cache_traversal (Baseline)
    using L02 = mp::mp_take_c<ce::traversal::TopicConfigSet::StaticAxisVariants_03m, 1>; // mapping
    using L03 = mp::mp_take_c<ce::nodes::TopicConfigSet::StaticAxisVariants_02, 1>;      // path_compression
    using L04 = mp::mp_take_c<ce::nodes::TopicConfigSet::StaticAxisVariants_04, 1>;      // node_type
    using L05 = mp::mp_take_c<ce::memory_layout::TopicConfigSet::StaticAxisVariants, 1>; // memory_layout
    using L06 = mp::mp_take_c<ce::allocator::TopicConfigSet::StaticAxisVariants, 1>;     // allocator
    using L07 = mp::mp_take_c<ce::prefetch::TopicConfigSet::StaticAxisVariants, 1>;      // prefetch
    using L08 = mp::mp_take_c<ce::concurrency::TopicConfigSet::StaticAxisVariants, 1>;   // concurrency
    using L09 = mp::mp_take_c<ce::serialization::TopicConfigSet::StaticAxisVariants, 1>; // serialization
    using L10 = mp::mp_take_c<ce::value_handle::TopicConfigSet::StaticAxisVariants, 1>;  // value_handle
    using L11 = mp::mp_take_c<ce::search_engine::TopicConfigSet::StaticAxisVariants,
                              1>;                                             // index_organization (INC-2d: isa raus)
    using L12 = mp::mp_take_c<ce::io::TopicConfigSet::StaticAxisVariants, 1>; // io_dispatch
    using L13 = mp::mp_take_c<ce::migration::TopicConfigSet::StaticAxisVariants, 1>;  // migration_policy
    using L14 = mp::mp_take_c<ce::filter::TopicConfigSet::StaticAxisVariants, 1>;     // filter
    using L15 = mp::mp_take_c<ce::queuing::TopicConfigSet::StaticAxisVariants_Q1, 1>; // queuing_q1
    using L16 = mp::mp_take_c<ce::queuing::TopicConfigSet::StaticAxisVariants_Q2, 1>; // queuing_q2

    using Engine =
        perm::PermutationEngine<CatalogCfg<L00>, CatalogCfg<L01>, CatalogCfg<L02>, CatalogCfg<L03>, CatalogCfg<L04>,
                                CatalogCfg<L05>, CatalogCfg<L06>, CatalogCfg<L07>, CatalogCfg<L08>, CatalogCfg<L09>,
                                CatalogCfg<L10>, CatalogCfg<L11>, CatalogCfg<L12>, CatalogCfg<L13>, CatalogCfg<L14>,
                                CatalogCfg<L15>, CatalogCfg<L16>>;
};

/// kary_perk_source_map() — die per-K Sweep-Quellen-map (binary_id → reale Modul-Quelle): genau 4 Eintraege
/// (KArySearchAlgoK2/4/8/16 × Baseline), je eine reale AdHocComposition (COMDARE_DEFINE_ANATOMY_MODULE_ADHOC).
/// Analog axis_sweep_source_map, aber fuer die NICHT-vertiefte search_algo-Achse ueber die explizite per-K-Liste.
[[nodiscard]] inline std::map<std::string, std::string> kary_perk_source_map() {
    return ex::build_pilot_source_map<typename KaryPerKCatalog::Engine>();
}

/// kary_perk_levels() — die 17 STATISCHEN AxisLevels des per-K-Sweep-Baums (17-Slot-Komposition; A11 2026-07-20:
/// gemessen 17, die alte "17+1=18"-Arithmetik war stale) — der search_algo-Slot traegt die 4 per-K-Werte
/// Slot-Listen → die View-binary_ids treffen die map-Keys). DynamicDims haengt der Treiber an.
[[nodiscard]] inline std::vector<ex::AxisLevel> kary_perk_levels() { return catalog_static_levels<KaryPerKCatalog>(); }

} // namespace comdare::cache_engine::thesis_lazy
