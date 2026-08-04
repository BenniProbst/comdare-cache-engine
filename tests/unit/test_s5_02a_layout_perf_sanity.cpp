// A8-S5 Familie 02_layout / Sub-Scheibe 02a (node_type + memory_layout + path_compression + serialization)
// -- PERF-SANITY-HARNESS.
//
// Dossier: docs/architecture/20260803-a8_f2_benchmarking_schnitt_soll_design.md Abschn. 5-S5
// ("GATE je Familie: Grundkadenz + Konformitaets-GTest der Familie + Perf-Sanity (keine 0-/Phantom-Zeiten)").
// Zweite Haelfte des GATE-MUSTERS: waehrend test_s5_02a_layout_alloc_conformance auf TYP-Ebene pinnt (und am
// Objekt belegt), dass kein Familien-Organ am Allokator-Achsen-Interface vorbei alloziert, belegt DIESE TU zur
// LAUFZEIT, dass der Scrub die Messbarkeit der Familie nicht beschaedigt hat.
//
// WARUM DIESE WACHE NOETIG IST: ein Scrub kann eine Achse still stumm schalten -- der Bau bleibt gruen, die
// Tests bleiben gruen, und die Mess-Spalte der Achse steht ab da auf 0. Genau diese Klasse war der A8-S1-/
// A8-S3-Befund (stiller Messwert-Verlust bzw. strukturell-0). Die Wache verlangt deshalb ZWEI Dinge ZUGLEICH:
//   (1) ZEIT:    seg_ns der Familien-Achsen > 0 unter Last.
//   (2) ZAEHLER: die Observer-Zaehler DERSELBEN Op-Schleife > 0.
// Zeit ohne Zaehler waere eine Phantom-Zeit (der Timer misst eine leere Schleife); Zaehler ohne Zeit waere ein
// toter Timer. Dazu (3) der EHRLICHE NULLPUNKT als Gegenprobe.
//
// FAMILIEN-SPEZIFISCHE FELD-WAHL (Block (2)/(3)) -- BEGRUENDET, nicht geraten. Die Auswahl folgt der Regel
// "pruefe die Spalte, die der Scrub angefasst hat", nicht "pruefe alles, was vorhanden ist":
//   * T4 node_type -> axis_stats[T4][0] "find" + [1] "keys_stored" + [2] "queries"
//     (Schema-Zeile kV3AxisSchema[T4], observable_tier.hpp; Schreiber abi_adapter fill_observer_v3).
//     BEGRUENDUNG: die node-Zeile wird ueber store_observe_node_type aus dem CHUNK-BACKING gespeist -- also
//     genau ueber die Struktur, deren Chunk-INDEX diese Scheibe auf den Kompositions-Allokator gehoben hat
//     (HERZ). Ein stillgelegter Chunk-Speicher wuerde hier zuerst als 0 auffallen. keys_stored ist zusaetzlich
//     die node-KAPAZITAETS-getriebene Groesse (Node4 vs. Node256), also die Achsen-Charakteristik selbst.
//   * T3 path_compression -> axis_stats[T3][0] "compress" + [1] "prefix_len" + [3] "cuts"
//     (Schema-Zeile kV3AxisSchema[T3]). BEGRUENDUNG: compress() ist im HOT-Path AUTO-gekoppelt (tier_insert/
//     tier_lookup) und ist der Treiber, an dem der geSCRUBbte Patricia-Trie haengt. DIREKTER SCRUB-BELEG
//     obendrauf: real_trie().key_count()/node_count() -- die geSCRUBbte Struktur selbst muss unter Last
//     gefuellt sein. Ein Scrub, der das Backing versehentlich stilllegt, faellt hier auf, auch wenn die
//     aggregierten Zaehler noch liefen (das war die A8-S3-Lehre in Reinform).
//   * T9 serialization -> axis_stats[T9][0] "serialize" + [1] "records" + [2] "bytes"
//     (Schema-Zeile kV3AxisSchema[T9]). BEGRUENDUNG: die serialization-Zeile wird ueber
//     store_observe_serialization aus DEMSELBEN Chunk-Backing gespeist -- sie ist damit die zweite,
//     unabhaengige Sonde auf den HERZ-Schnitt, ueber eine andere Achse und einen anderen Zaehler-Satz.
//   * T5 memory_layout gehoert zur Sub-Familie, hatte aber KEINEN Code-Treffer (grep) -- es wird auf Zeit
//     geprueft (Block (1)) und ueber die CLU-Zeile [3] "cache_lines" mitgelesen, aber es traegt keine
//     familien-spezifische Zaehler-BEHAUPTUNG: eine Zaehler-Aussage ueber eine nicht angefasste Achse waere
//     Schmuck, keine Wache.
//   * T6 allocator ist der VERSORGER der Scheibe: nur Zeit (Block (1)). Seine Zaehler sind Gegenstand des
//     Mess-Schnitt-Fensters (Doppelzaehlungs-Regel, Owner-KERN 04.08. abend-11), nicht dieser Wache.
//
// ACHSEN-INDIZES werden aus kCompositionAxisNames ABGELEITET (Name -> Position), nie als Literal geschrieben.
// Die Namensliste selbst wird nur GELESEN (golden-Tabu).
//
// Standalone (plain int main, KEIN gtest), COMDARE_MEASUREMENT_ON/-STATISTICS kommen global aus dem
// Haupt-CMakeLists (COMDARE_MEASUREMENT_MODE=ON).

#include <anatomy/abi_adapter.hpp>
#include <anatomy/composition_factory.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>

#include <builder/experiment_tree/axis_path_serialization.hpp> // kCompositionAxisNames (NUR gelesen)
#include <compositions/hot_reference.hpp>
#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_array256.hpp>

#include <axes/layout/axis_05_memory_layout_aos_strict.hpp>
#include <axes/layout/axis_05_memory_layout_cache_line_aligned.hpp>
#include <axes/layout/axis_05_memory_layout_observable.hpp>
#include <axes/node/axis_04_node_type_node256.hpp>
#include <axes/node/axis_04_node_type_node4.hpp>
#include <axes/node/axis_04_node_type_observable.hpp>
#include <axes/path_compression/axis_02_path_compression_none.hpp>
#include <axes/path_compression/axis_02_path_compression_patricia.hpp>
#include <axes/persistence_target/axis_persistence_target_memory_only.hpp>
#include <axes/serialization_axis/axis_10_serialization_observable.hpp>
#include <axes/serialization_axis/axis_10_serialization_var_len.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string_view>

namespace an    = ::comdare::cache_engine::anatomy;
namespace comp  = ::comdare::cache_engine::compositions;
namespace ce03a = ::comdare::cache_engine::traversal::axis_03a_search_algo;
namespace ex    = ::comdare::cache_engine::builder::experiment;
namespace ndx   = ::comdare::cache_engine::node;
namespace mlx   = ::comdare::cache_engine::layout;
namespace pcx   = ::comdare::cache_engine::path_compression;
namespace serx  = ::comdare::cache_engine::serialization_axis;

namespace {

int  g_fail = 0;
void tr(char const* what, bool ok) {
    std::printf("  [%s] %s\n", ok ? " ok " : "FAIL", what);
    if (!ok) ++g_fail;
}

// -- Achsen-Index AUS DEM NAMEN (nie Literal) ------------------------------------------------------
[[nodiscard]] constexpr std::size_t axis_index_of(std::string_view axis) noexcept {
    for (std::size_t i = 0; i < ex::kCompositionAxisNames.size(); ++i)
        if (ex::kCompositionAxisNames[i] == axis) return i;
    return ex::kCompositionAxisNames.size(); // nicht gefunden -> vom static_assert unten gefangen
}

// -- Familien-Definition: DAS ist die Zeile, die Folge-Familien austauschen ------------------------
// 02a = die Sub-Scheibe dieser Welle. "filter" gehoert zur Organ-Gruppe 02_layout, aber zur Sub-Scheibe
// 02b (eigener Scrub) und steht deshalb bewusst NICHT hier -- sonst behauptete die Wache Gruen fuer eine
// noch ungeschnittene Achse.
constexpr std::string_view kFamilyAxes[] = {"node_type", "memory_layout", "path_compression", "serialization",
                                            "allocator"};

constexpr std::size_t kAxisNode   = axis_index_of("node_type");
constexpr std::size_t kAxisLayout = axis_index_of("memory_layout");
constexpr std::size_t kAxisPc     = axis_index_of("path_compression");
constexpr std::size_t kAxisSer    = axis_index_of("serialization");

static_assert(kAxisNode < an::kV3AxisCount,
              "S5-02a Perf-Sanity: Achse 'node_type' steht nicht mehr in kCompositionAxisNames -- die Wache "
              "wuerde eine falsche seg_ns-Spalte pruefen.");
static_assert(kAxisLayout < an::kV3AxisCount,
              "S5-02a Perf-Sanity: Achse 'memory_layout' steht nicht mehr in kCompositionAxisNames.");
static_assert(kAxisPc < an::kV3AxisCount,
              "S5-02a Perf-Sanity: Achse 'path_compression' steht nicht mehr in kCompositionAxisNames.");
static_assert(kAxisSer < an::kV3AxisCount,
              "S5-02a Perf-Sanity: Achse 'serialization' steht nicht mehr in kCompositionAxisNames.");
static_assert(ex::kCompositionAxisNames.size() == an::kV3AxisCount,
              "S5-02a Perf-Sanity: Achsen-Namensliste und Snapshot-Achsenzahl sind auseinandergelaufen.");

// -- Store-getragene Komposition: Array256 (Byte-Domaene) + HOT-Slots, die 02a-Achsen variabel.
//    Nur ueber ein REAL befuelltes Store-Backing entsteht ein echter Descent -- eine Huellen-Komposition
//    wuerde die Segment-Treiber blockieren und die Wache um ihre Aussage bringen (Pilot-Erfahrung 04).
// WICHTIG (am Objekt verifiziert, nicht geraten): die Achsen node_type/memory_layout/serialization gehen als
// OBSERVABLE HUELLE in die Komposition -- der ABI-Adapter haelt sie als `typename Composition::memory_layout` /
// `::serialization` DIREKT als Member (abi_adapter.hpp:2295-2296) und die node-/layout-/ser-Zeilen des Snapshots
// werden aus deren statistics() gespeist. Eine ROHE Strategie an dieser Stelle traegt keine statistics() -> die
// drei Spalten stuenden auf honest-0 und die Wache pruefte NICHTS. Exakt so macht es auch HotComposition
// (hot_reference.hpp:46-54). Genau diese Falle hat die erste Fassung dieser TU gestellt -- sie ist hier
// dokumentiert, damit die naechste Familie sie nicht erneut stellt.
template <class NodeStrategy, class LayoutStrategy, class PcStrategy, class SerStrategy>
using Family02aComposition = an::AdHocComposition<
    ce03a::Array256SearchAlgo, comp::HotComposition::cache_traversal, comp::HotComposition::mapping, PcStrategy,
    ndx::ObservableNodeType<NodeStrategy>, mlx::ObservableMemoryLayout<LayoutStrategy>, comp::HotComposition::allocator,
    comp::HotComposition::prefetch, comp::HotComposition::concurrency, serx::ObservableSerialization<SerStrategy>,
    comp::HotComposition::value_handle, comp::HotComposition::index_organization, comp::HotComposition::io_dispatch,
    comp::HotComposition::migration_policy, comp::HotComposition::filter, comp::HotComposition::queuing_q1,
    comp::HotComposition::queuing_q2, ::comdare::cache_engine::persistence_target::MemoryOnlyTarget>;

[[nodiscard]] std::uint64_t spread_key(std::uint64_t i) noexcept {
    return (i * 2654435761ull) ^ (i << 17) ^ 0x9E3779B97F4A7C15ull;
}

/// Ergebnis eines Treiber-Laufs: der Observer-Snapshot PLUS der direkte Blick in die geSCRUBbte
/// path_compression-Trie-Struktur (die aggregierten Zaehler allein saehen ein stillgelegtes Backing nicht).
struct DriveResult {
    an::ComdareTierObserverSnapshot snap{};
    std::size_t                     trie_keys  = 0;
    std::size_t                     trie_nodes = 0;
};

/// Treibt ein echtes Tier unter Last (insert + lookup) und zieht EINEN Observer-Snapshot.
template <class NodeStrategy, class LayoutStrategy, class PcStrategy, class SerStrategy>
[[nodiscard]] DriveResult drive_and_observe(std::uint64_t n_ops) {
    using Anatomy =
        an::SearchAlgorithmAnatomy<Family02aComposition<NodeStrategy, LayoutStrategy, PcStrategy, SerStrategy>>;
    an::SearchAlgorithmAbiAdapter<Anatomy> tier;
    auto*                                  base = static_cast<an::IAnatomyBase*>(&tier);
    auto*                                  drv  = dynamic_cast<an::IDriveableTier*>(base);
    auto*                                  obs  = dynamic_cast<an::IObservableTier*>(base);
    DriveResult                            out{};
    if (drv == nullptr || obs == nullptr) {
        tr("IDriveableTier + IObservableTier vorhanden (Messung-AN kompiliert)", false);
        return out;
    }
    // Byte-Domaene: Array256 ist DirectAddress-treu; ungeklemmte u64-Keys wuerden abgelehnt und das
    // Store-Backing bliebe leer (slot_count()==0 -> kein Descent -> die Wache saehe eine leere Achse).
    for (std::uint64_t i = 0; i < n_ops; ++i) (void)drv->tier_insert(spread_key(i) & 0xFFull, i * 11u + 5u);
    std::uint64_t v = 0;
    for (std::uint64_t i = 0; i < n_ops; ++i) (void)drv->tier_lookup(spread_key(i) & 0xFFull, &v);
    obs->tier_observe(&out.snap);
    auto const& trie = tier.path_compression_instance().real_trie();
    out.trie_keys    = trie.key_count();
    out.trie_nodes   = trie.node_count();
    return out;
}

} // namespace

int main() {
    std::printf("== A8-S5 Familie 02a (node/layout/path_compression/serialization) -- Perf-Sanity ==\n");
    constexpr std::uint64_t kLoad = 2000;

    using CLA      = mlx::CacheLineAlignedMemoryLayout;
    using AoS      = mlx::AoSStrictMemoryLayout;
    using VarLen   = serx::VarLenSerialization;
    using Patricia = pcx::PatriciaPathCompression;
    using PcNone   = pcx::PathCompressionNone;

    // Familien-Besetzung unter Last: der geSCRUBbte Chunk-Speicher (Node4/CLA) + der geSCRUBbte Trie
    // (Patricia). Beide Speicher laufen seit dem Scrub ueber die Allokator-Achse.
    auto const n4_patricia = drive_and_observe<ndx::Node4NodeType, CLA, Patricia, VarLen>(kLoad);
    // Gegenprobe 1: dieselbe Last, aber der ANDERE Node-Typ (Chunk-Kapazitaet 4 vs. 256 -- die
    // node_type-Charakteristik, die der Chunk-Speicher ueberhaupt erst messbar macht).
    auto const n256_patricia = drive_and_observe<ndx::Node256NodeType, CLA, Patricia, VarLen>(kLoad);
    // Gegenprobe 2: dasselbe mit dem ANDEREN Layout (aos_strict -- 16-B-Stride statt 64-B-Padding).
    auto const n4_aos = drive_and_observe<ndx::Node4NodeType, AoS, Patricia, VarLen>(kLoad);
    // Gegenprobe 3: der ehrliche Nullpunkt der path_compression-Achse (None = M3-Pin, EmptyPatriciaTrie).
    auto const n4_none = drive_and_observe<ndx::Node4NodeType, CLA, PcNone, VarLen>(kLoad);

    // -- (1) ZEIT -------------------------------------------------------------------------------------
    std::printf("-- (1) ZEIT: seg_ns der Familien-Achsen unter Last (n_ops=%llu je Phase) --\n",
                static_cast<unsigned long long>(kLoad));
    for (auto const axis : kFamilyAxes) {
        std::size_t const  idx = axis_index_of(axis);
        std::int64_t const ns  = n4_patricia.snap.seg_ns[idx];
        std::printf("     seg_ns[T%zu %-18.*s] = %lld ns\n", idx, static_cast<int>(axis.size()), axis.data(),
                    static_cast<long long>(ns));
    }
    std::printf("     seg_run_total_ns = %lld ns   batches_measured = %llu\n",
                static_cast<long long>(n4_patricia.snap.seg_run_total_ns),
                static_cast<unsigned long long>(n4_patricia.snap.batches_measured));

    tr("(1) der Segment-Lauf hat ueberhaupt stattgefunden (batches_measured > 0)",
       n4_patricia.snap.batches_measured > 0);
    for (auto const axis : kFamilyAxes) {
        std::size_t const idx = axis_index_of(axis);
        if (axis == std::string_view{"node_type"})
            tr("(1) seg_ns[node_type] > 0 unter Last (keine 0-Zeit)", n4_patricia.snap.seg_ns[idx] > 0);
        else if (axis == std::string_view{"memory_layout"})
            tr("(1) seg_ns[memory_layout] > 0 unter Last (kein Scrub, aber Familien-Mitglied)",
               n4_patricia.snap.seg_ns[idx] > 0);
        else if (axis == std::string_view{"path_compression"})
            tr("(1) seg_ns[path_compression] > 0 unter Last (keine 0-Zeit)", n4_patricia.snap.seg_ns[idx] > 0);
        else if (axis == std::string_view{"serialization"})
            tr("(1) seg_ns[serialization] > 0 unter Last (keine 0-Zeit)", n4_patricia.snap.seg_ns[idx] > 0);
        else
            tr("(1) seg_ns[allocator] > 0 unter Last (Versorger-Achse der Scheibe)", n4_patricia.snap.seg_ns[idx] > 0);
    }

    // -- (2) FAMILIEN-SPEZIFISCH: die Zaehler DERSELBEN Op-Schleife (Anti-Phantom) ---------------------
    std::uint64_t const nd_find   = n4_patricia.snap.axis_stats[kAxisNode][0];
    std::uint64_t const nd_keys   = n4_patricia.snap.axis_stats[kAxisNode][1];
    std::uint64_t const nd_query  = n4_patricia.snap.axis_stats[kAxisNode][2];
    std::uint64_t const pc_calls  = n4_patricia.snap.axis_stats[kAxisPc][0];
    std::uint64_t const pc_prefix = n4_patricia.snap.axis_stats[kAxisPc][1];
    std::uint64_t const pc_cuts   = n4_patricia.snap.axis_stats[kAxisPc][3];
    std::uint64_t const sr_calls  = n4_patricia.snap.axis_stats[kAxisSer][0];
    std::uint64_t const sr_recs   = n4_patricia.snap.axis_stats[kAxisSer][1];
    std::uint64_t const sr_bytes  = n4_patricia.snap.axis_stats[kAxisSer][2];
    std::uint64_t const ml_lines  = n4_patricia.snap.axis_stats[kAxisLayout][3];
    std::printf("-- (2) ZAEHLER derselben Op-Schleife (Anti-Phantom) --\n");
    std::printf("     T4  find = %llu   keys_stored = %llu   queries = %llu\n",
                static_cast<unsigned long long>(nd_find), static_cast<unsigned long long>(nd_keys),
                static_cast<unsigned long long>(nd_query));
    std::printf("     T3  compress = %llu   prefix_len = %llu   cuts = %llu   real_trie: keys=%zu nodes=%zu\n",
                static_cast<unsigned long long>(pc_calls), static_cast<unsigned long long>(pc_prefix),
                static_cast<unsigned long long>(pc_cuts), n4_patricia.trie_keys, n4_patricia.trie_nodes);
    std::printf("     T9  serialize = %llu   records = %llu   bytes = %llu     T5 cache_lines = %llu\n",
                static_cast<unsigned long long>(sr_calls), static_cast<unsigned long long>(sr_recs),
                static_cast<unsigned long long>(sr_bytes), static_cast<unsigned long long>(ml_lines));

    tr("(2) T4: die gemessene Zeit ist durch reale node-Finds gedeckt (find > 0)", nd_find > 0);
    tr("(2) T4: das geSCRUBbte Chunk-Backing traegt die Keys wirklich (keys_stored > 0)", nd_keys > 0);
    tr("(2) T4: und es wird auch abgefragt (queries > 0)", nd_query > 0);
    tr("(2) T3: die gemessene Zeit ist durch reale compress-Ops gedeckt (compress > 0)", pc_calls > 0);
    tr("(2) T3: die Kompression tut auch etwas (prefix_len > 0 und cuts > 0)", pc_prefix > 0 && pc_cuts > 0);
    tr("(2) SCRUB-BELEG: der geSCRUBbte Patricia-Trie ist unter Last gefuellt (key_count > 0)",
       n4_patricia.trie_keys > 0);
    tr("(2) SCRUB-BELEG: und strukturell konsistent (node_count == 2*key_count - 1, crit-bit-Trie)",
       n4_patricia.trie_nodes == 2u * n4_patricia.trie_keys - 1u);
    tr("(2) T9: die gemessene Zeit ist durch reale serialize-Ops ueber DASSELBE Chunk-Backing gedeckt "
       "(serialize > 0, records > 0, bytes > 0)",
       sr_calls > 0 && sr_recs > 0 && sr_bytes > 0);
    tr("(2) T5: die CLU-Zeile wird aus dem realen Chunk-Backing gefuellt (cache_lines > 0)", ml_lines > 0);

    // -- (3) FAMILIEN-SPEZIFISCH: ehrliche Nullpunkte + Strategie-Kontrast -----------------------------
    std::uint64_t const none_calls  = n4_none.snap.axis_stats[kAxisPc][0];
    std::uint64_t const n256_keys   = n256_patricia.snap.axis_stats[kAxisNode][1];
    std::uint64_t const aos_lines   = n4_aos.snap.axis_stats[kAxisLayout][3];
    std::uint64_t const aos_srbytes = n4_aos.snap.axis_stats[kAxisSer][2];
    std::printf("-- (3) ehrlicher Nullpunkt + Strategie-Kontrast --\n");
    std::printf("     None: compress = %llu   real_trie keys = %zu (EmptyPatriciaTrie)\n",
                static_cast<unsigned long long>(none_calls), n4_none.trie_keys);
    std::printf("     node_type: Node4 keys_stored = %llu   Node256 keys_stored = %llu\n",
                static_cast<unsigned long long>(nd_keys), static_cast<unsigned long long>(n256_keys));
    std::printf("     memory_layout: CLA cache_lines = %llu   aos_strict cache_lines = %llu   "
                "T9 bytes CLA/aos = %llu/%llu\n",
                static_cast<unsigned long long>(ml_lines), static_cast<unsigned long long>(aos_lines),
                static_cast<unsigned long long>(sr_bytes), static_cast<unsigned long long>(aos_srbytes));

    tr("(3) None haelt weiterhin KEINEN Trie (key_count == 0, M3-Pin messneutral, deklarierter Nullpunkt)",
       n4_none.trie_keys == 0);
    tr("(3) None wird trotzdem real getrieben (compress > 0) -- der Nullpunkt ist ehrlich, nicht stumm",
       none_calls > 0);
    tr("(3) Strategie-Kontrast T3: Patricia baut eine Struktur, None nicht -- die Zeit haengt an der Achse",
       n4_patricia.trie_keys > n4_none.trie_keys);
    // Node4 speichert je Chunk 4 Keys, Node256 bis zu 256 -- die node-Zeile zaehlt min(n, cap) je Chunk-Scan,
    // also ist die Node256-Zahl ECHT groesser. Genau das ist die Achsen-Charakteristik, die ueber den
    // geSCRUBbten Chunk-Speicher laeuft: waere der stillgelegt, waeren beide Werte gleich (oder 0).
    tr("(3) Strategie-Kontrast T4: Node256 speichert je Chunk mehr Keys als Node4 (Kapazitaets-Charakteristik "
       "ueber den geSCRUBbten Chunk-Speicher)",
       n256_keys > nd_keys);
    // CLA padded auf Cache-Line-Stride, aos_strict packt dicht -> derselbe Key-Scan beruehrt bei CLA MEHR
    // Linien. Der Kontrast beweist, dass die layout-Zeile aus dem REALEN Byte-Layout des Chunk-Backings
    // kommt und nicht aus einem entkoppelten Deskriptor (P-MD1-Erdung, vom Scrub unberuehrt).
    tr("(3) Strategie-Kontrast T5: CLA (padded) beruehrt mehr Cache-Linien als aos_strict (dicht)",
       ml_lines > aos_lines);
    tr("(3) und die serialization-Zeile folgt demselben realen Backing (aos_strict bytes > 0, != CLA)",
       aos_srbytes > 0 && aos_srbytes != sr_bytes);

    std::printf("== test_s5_02a_layout_perf_sanity: %s ==\n", g_fail == 0 ? "ALLE OK" : "FEHLER");
    return g_fail == 0 ? 0 : 1;
}
