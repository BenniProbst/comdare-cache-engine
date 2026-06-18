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

#include <builder/experiment_tree/cache_engine_builder_iterator.hpp>  // run_lazy_static_then_dynamic
#include <builder/experiment_tree/pilot_source_map.hpp>               // build_pilot_source_map / make_source_gen_from_map
#include <builder/experiment_tree/axis_reflect.hpp>                   // push_static_axis (Katalog-Referenz-Levels)
#include <permutations/permutation_engine.hpp>                        // PermutationEngine

// Die statischen Katalog-Achsen-ConfigSets (echte Wrapper):
#include <topics/traversal/topic_traversal_config_set.hpp>
#include <topics/nodes/topic_nodes_config_set.hpp>
#include <topics/memory_layout/topic_memory_layout_config_set.hpp>
#include <topics/allocator/topic_allocator_config_set.hpp>
#include <topics/prefetch/topic_prefetch_config_set.hpp>
#include <topics/concurrency/topic_concurrency_config_set.hpp>
#include <topics/serialization/topic_serialization_config_set.hpp>
#include <topics/telemetry/topic_telemetry_config_set.hpp>
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
#include <utility>
#include <vector>

namespace comdare::cache_engine::thesis_lazy {

namespace ex   = ::comdare::cache_engine::builder::experiment;
namespace perm = ::comdare::cache_engine::permutations;
namespace ce   = ::comdare::cache_engine;
namespace mp   = boost::mp11;

// PermutationEngine erwartet je Topic ::StaticAxisVariants. Wir wickeln eine beliebige mp_list als ConfigSet.
template <class List> struct CatalogCfg { using StaticAxisVariants = List; };

// ── Die 19 AdHocComposition-Slot-Listen des Katalogs. KEINE Selektion — die VOLLE Achsen-kartesische Domäne, die
//    das comdare_thesis_profile (m3v2_study.profile.xml <permute_axes>) deklariert: variiert werden search_algo /
//    node_type / memory_layout / prefetch; die übrigen 15 sind je 1 Wert gepinnt (deckungsgleich zur Profil-
//    Deklaration → identische binary_ids = golden). 4·4·5·4 = 320 reale Kompositionen.
//    (Die mp_take_c-Reihenfolge + die per-Slot-Wahl ist die zentrale Konvention, die die Profil-binary_ids reproduziert
//     — siehe golden_fullpilot_320_binary_ids.txt; eine Änderung hier bräche Resume #139, daher golden-gegated.) ──
template <std::size_t KSearch, std::size_t KNode, std::size_t KLayout, std::size_t KPrefetch>
struct CatalogAxes {
    using L00 = mp::mp_take_c<ce::traversal::TopicConfigSet::StaticAxisVariants_03a, KSearch>;     // search_algo
    using L01 = mp::mp_take_c<ce::traversal::TopicConfigSet::StaticAxisVariants_03b, 1>;           // cache_traversal
    using L02 = mp::mp_take_c<ce::traversal::TopicConfigSet::StaticAxisVariants_03m, 1>;           // mapping
    using L03 = mp::mp_take_c<ce::nodes::TopicConfigSet::StaticAxisVariants_02, 1>;                // path_compression
    using L04 = mp::mp_take_c<ce::nodes::TopicConfigSet::StaticAxisVariants_04, KNode>;            // node_type
    using L05 = mp::mp_take_c<ce::memory_layout::TopicConfigSet::StaticAxisVariants, KLayout>;     // memory_layout
    using L06 = mp::mp_take_c<ce::allocator::TopicConfigSet::StaticAxisVariants, 1>;               // allocator
    using L07 = mp::mp_take_c<ce::prefetch::TopicConfigSet::StaticAxisVariants, KPrefetch>;        // prefetch
    using L08 = mp::mp_take_c<ce::concurrency::TopicConfigSet::StaticAxisVariants, 1>;             // concurrency
    using L09 = mp::mp_take_c<ce::serialization::TopicConfigSet::StaticAxisVariants, 1>;           // serialization
    using L10 = mp::mp_take_c<ce::telemetry::TopicConfigSet::StaticAxisVariants, 1>;               // telemetry
    using L11 = mp::mp_take_c<ce::value_handle::TopicConfigSet::StaticAxisVariants, 1>;            // value_handle
    using L12 = mp::mp_take_c<ce::hardware::TopicConfigSet::StaticAxisVariants_09, 1>;             // isa
    using L13 = mp::mp_take_c<ce::search_engine::TopicConfigSet::StaticAxisVariants, 1>;           // index_organization
    using L14 = mp::mp_take_c<ce::io::TopicConfigSet::StaticAxisVariants, 1>;                      // io_dispatch
    using L15 = mp::mp_take_c<ce::migration::TopicConfigSet::StaticAxisVariants, 1>;               // migration_policy
    using L16 = mp::mp_take_c<ce::filter::TopicConfigSet::StaticAxisVariants, 1>;                  // filter
    using L17 = mp::mp_take_c<ce::queuing::TopicConfigSet::StaticAxisVariants_Q1, 1>;              // queuing_q1
    using L18 = mp::mp_take_c<ce::queuing::TopicConfigSet::StaticAxisVariants_Q2, 1>;              // queuing_q2

    using Engine = perm::PermutationEngine<
        CatalogCfg<L00>, CatalogCfg<L01>, CatalogCfg<L02>, CatalogCfg<L03>, CatalogCfg<L04>, CatalogCfg<L05>,
        CatalogCfg<L06>, CatalogCfg<L07>, CatalogCfg<L08>, CatalogCfg<L09>, CatalogCfg<L10>, CatalogCfg<L11>,
        CatalogCfg<L12>, CatalogCfg<L13>, CatalogCfg<L14>, CatalogCfg<L15>, CatalogCfg<L16>, CatalogCfg<L17>,
        CatalogCfg<L18>>;
};

// Voller Katalog: 4·4·5·4 = 320 reale SA-Kompositionen = der vom m3v2-Profil deklarierte Achsen-kartesische Raum.
using FullSourceCatalog = CatalogAxes<4, 4, 5, 4>;
// Klein-Katalog (1·2·2·1 = 4 Kompositionen): für den schnellen E2E-Treiber-Test (test_lazy_static_dynamic_driver),
// der real 4 DLLs baut+lädt+misst. KEINE Lauf-Selektion — nur eine kleine Materialisierungs-Domäne für den Test.
using SmallSourceCatalog = CatalogAxes<1, 2, 2, 1>;

/// catalog_static_levels<Catalog>() — die 19 STATISCHEN AxisLevels des Katalogs (gleiche reduzierte Listen + gleiche
/// Achsen-Namen-Reihenfolge wie der Katalog-Engine = die binary_ids, die der Katalog materialisiert). Das ist KEINE
/// Lauf-Selektion (der Lauf-Baum kommt aus build_axis_levels(Profil)) — es ist die REFERENZ-Level-Quelle, mit der die
/// committete golden_fullpilot_320_binary_ids.txt EINMAL erzeugt + im Test gegen den Profil-Pfad gegengeprüft wird.
/// DynamicDims werden hier NICHT angehängt (die binary_id nutzt nur statische Ebenen).
template <class Catalog>
[[nodiscard]] inline std::vector<ex::AxisLevel> catalog_static_levels() {
    std::vector<ex::AxisLevel> lv;
    ex::push_static_axis<typename Catalog::L00>(lv, "search_algo");        ex::push_static_axis<typename Catalog::L01>(lv, "cache_traversal");
    ex::push_static_axis<typename Catalog::L02>(lv, "mapping");            ex::push_static_axis<typename Catalog::L03>(lv, "path_compression");
    ex::push_static_axis<typename Catalog::L04>(lv, "node_type");          ex::push_static_axis<typename Catalog::L05>(lv, "memory_layout");
    ex::push_static_axis<typename Catalog::L06>(lv, "allocator");          ex::push_static_axis<typename Catalog::L07>(lv, "prefetch");
    ex::push_static_axis<typename Catalog::L08>(lv, "concurrency");        ex::push_static_axis<typename Catalog::L09>(lv, "serialization");
    ex::push_static_axis<typename Catalog::L10>(lv, "telemetry");          ex::push_static_axis<typename Catalog::L11>(lv, "value_handle");
    ex::push_static_axis<typename Catalog::L12>(lv, "isa");                ex::push_static_axis<typename Catalog::L13>(lv, "index_organization");
    ex::push_static_axis<typename Catalog::L14>(lv, "io_dispatch");        ex::push_static_axis<typename Catalog::L15>(lv, "migration_policy");
    ex::push_static_axis<typename Catalog::L16>(lv, "filter");
    ex::push_static_axis<typename Catalog::L17>(lv, "queuing_q1");         ex::push_static_axis<typename Catalog::L18>(lv, "queuing_q2");
    return lv;
}

/// make_catalog_source_gen() — die EINE profil-getriebene SourceGenFn (binary_id → reale Modul-Quelle). „Profil-
/// getrieben" heißt: der Treiber baut den Baum + die Selektion AUS DEM PROFIL (build_axis_levels / profile_select),
/// und fragt diesen Katalog NUR über den resultierenden binary_id ab. Der Katalog selbst trifft KEINE Auswahl — er
/// materialisiert (EINMAL, compile-time) den vollen deklarierten Achsenraum als Typ→Quelle-map (BR-2/BR-4). Ein
/// binary_id außerhalb des deklarierten Raums liefert eine leere Quelle (Orchestrator markiert die DLL als nicht
/// baubar — ehrlich sichtbar). Lazy-Compile (1 DLL = 1 TU) bleibt: je Quelle ein eigener perm_<id>.cpp.
template <class Catalog = FullSourceCatalog>
[[nodiscard]] inline ex::SourceGenFn make_catalog_source_gen() {
    return ex::make_source_gen_from_map(ex::build_pilot_source_map<typename Catalog::Engine>());
}

/// catalog_levels<Catalog>(with_dynamic) — der GESAMT-Level-Satz fuer den E2E-Treiber-Test (Klein-Katalog): die 19
/// statischen Achsen + (optional) die 3 DynamicDims (concurrency.thread_count / prefetch.prefetch_distance /
/// repetition). Das ist KEINE Lauf-Selektion — der Produktiv-Treiber baut den Baum aus build_axis_levels(Profil);
/// diese Funktion dient NUR dem isolierten Treiber-Test (kleiner Baum ohne Profil-Parser).
template <class Catalog>
[[nodiscard]] inline std::vector<ex::AxisLevel> catalog_levels(bool with_dynamic = true,
                                                               std::uint32_t n_repeats = 3) {
    std::vector<ex::AxisLevel> lv = catalog_static_levels<Catalog>();
    if (with_dynamic) {
        std::uint32_t const reps = (n_repeats == 0) ? 1u : n_repeats;
        std::vector<std::string> rep_vals; rep_vals.reserve(reps);
        for (std::uint32_t r = 0; r < reps; ++r) rep_vals.push_back(std::to_string(r));
        lv.push_back(ex::AxisLevel{"concurrency", {"1", "2", "4"}, /*is_static=*/false, "thread_count",      "concurrency"});
        lv.push_back(ex::AxisLevel{"prefetch",    {"0", "8"},      /*is_static=*/false, "prefetch_distance", "prefetch"});
        lv.push_back(ex::AxisLevel{"repetition",  std::move(rep_vals), /*is_static=*/false, "repetition_index", "repetition"});
    }
    return lv;
}

}  // namespace comdare::cache_engine::thesis_lazy
