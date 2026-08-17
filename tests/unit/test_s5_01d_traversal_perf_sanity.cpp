// A8-S5 Familie 01d cache_traversal (Achse 03b) -- PERF-SANITY-HARNESS.
//
// Dossier: docs/architecture/20260803-a8_f2_benchmarking_schnitt_soll_design.md Abschn. 5-S5
// ("GATE je Familie: Grundkadenz + Konformitaets-GTest der Familie + Perf-Sanity (keine 0-/Phantom-Zeiten)").
// Zweite Haelfte des GATE-MUSTERS (Pilot 04_execution): waehrend
// test_s5_01d_traversal_alloc_conformance auf TYP-Ebene pinnt, dass kein Familien-Organ am
// Allokator-Achsen-Interface vorbei alloziert, belegt DIESE TU zur LAUFZEIT, dass der Scrub die
// Messbarkeit der Familie nicht beschaedigt hat.
//
// WARUM DIESE WACHE NOETIG IST: ein Scrub kann eine Achse still stumm schalten -- der Bau bleibt gruen,
// die Tests bleiben gruen, und die Mess-Spalte der Achse steht ab da auf 0. Genau diese Klasse war der
// A8-S1-/A8-S3-Befund. Die Wache verlangt deshalb ZWEI Dinge ZUGLEICH, nie nur eines:
//   (1) ZEIT:    seg_ns der Familien-Achse > 0 unter Last.
//   (2) ZAEHLER: die Observer-Zaehler DERSELBEN Op-Schleife > 0.
// Zeit ohne Zaehler waere eine Phantom-Zeit (der Timer misst eine leere Schleife); Zaehler ohne Zeit
// waere ein toter Timer. Dazu (3) der EHRLICHE NULLPUNKT als Gegenprobe.
//
// ACHSEN-INDIZES werden aus kCompositionAxisNames ABGELEITET (Name -> Position), nie als Literal
// geschrieben. Die Namensliste selbst wird nur GELESEN (golden-Tabu).
//
// -- FAMILIEN-SPEZIFISCH: DIE SPALTEN-WAHL, UND WARUM GENAU DIESE (Auflage des Gate-Musters) --------
// Schema-Zeile der Achse (observable_tier.hpp kV3AxisSchema[T1], Schreib-Reihenfolge = Vertrag
// zwischen DLL und CSV):
//   [0] resolve   [1] resolve_hit   [2] resolve_miss   [3] register   [4] unregister
//   [5] peak_tracked   [6] batch_size   [7] batch_visited
// GEWAEHLT: [3] register UND [0] resolve.
//   BEGRUENDUNG: der Scrub hat den EINTRAGS-SPEICHER angefasst -- also muss die Wache genau die beiden
//   Ops decken, die diesen Speicher beruehren bzw. lesen. [3] register ist die schreibende Op (die
//   allozierende, tier_insert -> ct_organ_.register_entry, abi_adapter.hpp:927), [0] resolve die
//   lesende (tier_lookup -> ct_organ_.resolve, abi_adapter.hpp:1002). Beide entstehen in DERSELBEN
//   Op-Schleife wie die gemessene Zeit.
//   NICHT gewaehlt und warum: [5] peak_tracked ist ein Maximum-Nebenprodukt von [3] (kein eigener
//   Beweis); [6]/[7] batch_* sind an dieser Achse nicht befuellt (Batch-Pfad, ausserhalb dieser
//   Op-Schleife) -- sie zu pruefen ergaebe eine Wache, die dauerhaft 0 saehe und damit entweder
//   falsch waere oder still auf ">= 0" verwaessert werden muesste.
//
// -- FAMILIEN-SPEZIFISCH: DER EHRLICHE NULLPUNKT ---------------------------------------------------
// Die Achse 03b hat KEINE None-Strategie (Registry: LinearFanout / HashLookup / BinarySearchFanout,
// alle drei fuehren real Buch). Der Nullpunkt kann hier also nicht wie im Pilot aus einer
// 0-Overhead-Strategie kommen. Ehrlich sind stattdessen ZWEI Gegenproben:
//   (a) [4] unregister MUSS 0 bleiben -- der Treiber ruft nie unregister. Ein Zaehler, der auch ohne
//       die Op steigt, waere ein pauschal hochgezaehlter Zaehler; genau das schliesst (a) aus.
//   (b) [1]/[2] resolve_hit/resolve_miss MUESSEN sich zu [0] resolve summieren UND beide Seiten
//       muessen real vorkommen (Treffer aus dem eingefuegten Bereich, Fehlschlaege aus einem nie
//       eingefuegten Bereich). Ein konstant mitlaufender Zaehler kann das nicht erfuellen.
// Dazu (4) der LAUFZEIT-VERDRAHTUNGS-ANKER der Form B am Scrub-Gegenstand selbst:
// traversal_allocator_statistics() MUSS am frisch konstruierten LinearFanout/BinarySearchFanout
// GENAU 0 Allokationen melden (deklarierter Nullpunkt: leerer Vektor alloziert nicht) und unter Last
// > 0 -- und bei HashLookup muss ein Rehash die Zahl ERHOEHEN. Das ist der Beleg, dass allocator_type
// nicht nur deklariert, sondern real benutzt wird (Grenze der Form B, s5_family_alloc_conformance.hpp).
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

#include <organ_axes/cache_traversal/axis_03b_cache_traversal_registry.hpp>
#include <organ_axes/persistence_target/axis_persistence_target_memory_only.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string_view>

namespace an    = ::comdare::cache_engine::anatomy;
namespace comp  = ::comdare::cache_engine::compositions;
namespace ce03a = ::comdare::cache_engine::traversal::axis_03a_search_algo;
namespace ex    = ::comdare::cache_engine::builder::experiment;
namespace ct    = ::comdare::cache_engine::cache_traversal;

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
constexpr std::string_view kFamilyAxes[] = {"cache_traversal"};

constexpr std::size_t kAxisTraversal = axis_index_of("cache_traversal");

// Feld-Indizes der Schema-Zeile kV3AxisSchema[T1] -- als benannte Konstanten, damit die Wahl im
// Quelltext lesbar bleibt (die Reihenfolge selbst ist der Wire-Vertrag und wird nur GELESEN).
constexpr std::size_t kFieldResolve     = 0;
constexpr std::size_t kFieldResolveHit  = 1;
constexpr std::size_t kFieldResolveMiss = 2;
constexpr std::size_t kFieldRegister    = 3;
constexpr std::size_t kFieldUnregister  = 4;

static_assert(kAxisTraversal < an::kV3AxisCount,
              "S5-01d Perf-Sanity: Achse 'cache_traversal' steht nicht mehr in kCompositionAxisNames -- die "
              "Wache wuerde eine falsche seg_ns-Spalte pruefen.");
static_assert(ex::kCompositionAxisNames.size() == an::kV3AxisCount,
              "S5-01d Perf-Sanity: Achsen-Namensliste und Snapshot-Achsenzahl sind auseinandergelaufen.");

// -- Store-getragene Komposition: Array256 (Byte-Domaene) + Hot-Kette, cache_traversal variabel. ----
//    Nur ueber ein REAL befuelltes Store-Backing entsteht ein echter Descent -- eine Huellen-
//    Komposition brachte die Wache um ihre Aussage (Pilot-Lehre, dort dokumentiert).
template <class CTStrategy>
using Family01dComposition = an::AdHocComposition<
    ce03a::Array256SearchAlgo, CTStrategy, comp::HotComposition::mapping, comp::HotComposition::path_compression,
    comp::HotComposition::node_type, comp::HotComposition::memory_layout, comp::HotComposition::allocator,
    comp::HotComposition::prefetch, comp::HotComposition::concurrency, comp::HotComposition::serialization,
    comp::HotComposition::value_handle, comp::HotComposition::index_organization, comp::HotComposition::io_dispatch,
    comp::HotComposition::migration_policy, comp::HotComposition::filter, comp::HotComposition::queuing_q1,
    comp::HotComposition::queuing_q2, ::comdare::cache_engine::persistence_target::MemoryOnlyTarget>;

[[nodiscard]] std::uint64_t spread_key(std::uint64_t i) noexcept {
    return (i * 2654435761ull) ^ (i << 17) ^ 0x9E3779B97F4A7C15ull;
}

/// Treibt ein echtes Tier unter Last (insert + lookup-Treffer + lookup-Fehlschlag) und zieht EINEN
/// Observer-Snapshot. Die Fehlschlag-Phase ist noetig, damit resolve_hit UND resolve_miss real
/// vorkommen (Gegenprobe (b) des ehrlichen Nullpunkts).
template <class CTStrategy>
[[nodiscard]] an::ComdareTierObserverSnapshot drive_and_observe(std::uint64_t n_ops) {
    using Anatomy = an::SearchAlgorithmAnatomy<Family01dComposition<CTStrategy>>;
    an::SearchAlgorithmAbiAdapter<Anatomy> tier;
    auto*                                  base = static_cast<an::IAnatomyBase*>(&tier);
    auto*                                  drv  = dynamic_cast<an::IDriveableTier*>(base);
    auto*                                  obs  = dynamic_cast<an::IObservableTier*>(base);
    an::ComdareTierObserverSnapshot        snap{};
    if (drv == nullptr || obs == nullptr) {
        tr("IDriveableTier + IObservableTier vorhanden (Messung-AN kompiliert)", false);
        return snap;
    }
    // Byte-Domaene: Array256 ist DirectAddress-treu; ungeklemmte u64-Keys wuerden abgelehnt und das
    // Store-Backing bliebe leer (kein Descent -> die Wache saehe eine leere Achse).
    for (std::uint64_t i = 0; i < n_ops; ++i) (void)drv->tier_insert(spread_key(i) & 0x7Full, i * 11u + 5u);
    std::uint64_t v = 0;
    for (std::uint64_t i = 0; i < n_ops; ++i) (void)drv->tier_lookup(spread_key(i) & 0x7Full, &v);
    // Fehlschlag-Phase: der obere Byte-Halbraum wurde NIE eingefuegt (Maske 0x7F oben).
    for (std::uint64_t i = 0; i < n_ops; ++i) (void)drv->tier_lookup(0x80ull | (i & 0x7Full), &v);
    obs->tier_observe(&snap);
    return snap;
}

#ifdef COMDARE_CE_ENABLE_STATISTICS
/// (4) LAUFZEIT-VERDRAHTUNGS-ANKER der Form B, direkt am Scrub-Gegenstand.
template <class Strategy>
void check_allocator_axis_wiring(char const* label) {
    Strategy       organ{};
    auto const     fresh          = organ.traversal_allocator_statistics();
    constexpr bool is_hash_lookup = requires(Strategy const& s) { s.bucket_count(); };

    char buf[256];
    if constexpr (is_hash_lookup) {
        // HashLookup alloziert schon in der Konstruktion (Bucket-Vektor der Startkapazitaet) --
        // deklarierter Unterschied, keine Ausnahme von der Regel.
        std::snprintf(buf, sizeof buf,
                      "(4) %s: Konstruktion alloziert ueber die Achse (alloc_cnt=%llu > 0, Bucket-Vektor)", label,
                      static_cast<unsigned long long>(fresh.allocation_count));
        tr(buf, fresh.allocation_count > 0);
    } else {
        std::snprintf(buf, sizeof buf,
                      "(4) %s: frisch konstruiert = deklarierter Nullpunkt (alloc_cnt=%llu == 0, leerer Vektor)", label,
                      static_cast<unsigned long long>(fresh.allocation_count));
        tr(buf, fresh.allocation_count == 0);
    }

    for (std::uint64_t i = 0; i < 4096; ++i) organ.register_entry(i, i * 3u + 1u);
    auto const loaded = organ.traversal_allocator_statistics();
    std::snprintf(buf, sizeof buf,
                  "(4) %s: unter Last laeuft der Speicher REAL ueber die Achse (alloc_cnt %llu -> %llu, "
                  "bytes_allocated=%llu)",
                  label, static_cast<unsigned long long>(fresh.allocation_count),
                  static_cast<unsigned long long>(loaded.allocation_count),
                  static_cast<unsigned long long>(loaded.total_bytes_allocated));
    tr(buf, loaded.allocation_count > fresh.allocation_count && loaded.total_bytes_allocated > 0);
}
#endif

} // namespace

int main() {
    std::printf("== A8-S5 Familie 01d cache_traversal -- Perf-Sanity (keine 0-/Phantom-Zeiten) ==\n");
    constexpr std::uint64_t kLoad = 2000;

    // Familien-Besetzung unter Last: die drei geSCRUBbten Strategien der Achse.
    auto const linear = drive_and_observe<ct::LinearFanout>(kLoad);
    auto const hashed = drive_and_observe<ct::HashLookup>(kLoad);
    auto const binary = drive_and_observe<ct::BinarySearchFanout>(kLoad);

    std::printf("-- (1) ZEIT: seg_ns der Familien-Achse unter Last (n_ops=%llu je Phase, 3 Phasen) --\n",
                static_cast<unsigned long long>(kLoad));
    for (auto const axis : kFamilyAxes) {
        std::size_t const idx = axis_index_of(axis);
        std::printf("     seg_ns[T%zu %-16.*s] linear=%lld  hash=%lld  binary=%lld ns\n", idx,
                    static_cast<int>(axis.size()), axis.data(), static_cast<long long>(linear.seg_ns[idx]),
                    static_cast<long long>(hashed.seg_ns[idx]), static_cast<long long>(binary.seg_ns[idx]));
    }
    std::printf("     seg_run_total_ns (linear) = %lld ns   batches_measured = %llu\n",
                static_cast<long long>(linear.seg_run_total_ns),
                static_cast<unsigned long long>(linear.batches_measured));

    tr("(1) der Segment-Lauf hat ueberhaupt stattgefunden (batches_measured > 0)", linear.batches_measured > 0);
    tr("(1) seg_ns[cache_traversal] > 0 unter Last -- LinearFanout (keine 0-Zeit)", linear.seg_ns[kAxisTraversal] > 0);
    tr("(1) seg_ns[cache_traversal] > 0 unter Last -- HashLookup (keine 0-Zeit)", hashed.seg_ns[kAxisTraversal] > 0);
    tr("(1) seg_ns[cache_traversal] > 0 unter Last -- BinarySearchFanout (keine 0-Zeit)",
       binary.seg_ns[kAxisTraversal] > 0);
    tr("(1) Kommensurabel: das Familien-Segment <= seg_run_total_ns (kein Zeit-Ueberlauf)",
       linear.seg_ns[kAxisTraversal] <= linear.seg_run_total_ns);

    // -- (2) FAMILIEN-SPEZIFISCH: die Zaehler DERSELBEN Op-Schleife (Anti-Phantom) -------------------
    std::uint64_t const reg_cnt  = linear.axis_stats[kAxisTraversal][kFieldRegister];
    std::uint64_t const res_cnt  = linear.axis_stats[kAxisTraversal][kFieldResolve];
    std::uint64_t const res_hit  = linear.axis_stats[kAxisTraversal][kFieldResolveHit];
    std::uint64_t const res_miss = linear.axis_stats[kAxisTraversal][kFieldResolveMiss];
    std::uint64_t const unreg    = linear.axis_stats[kAxisTraversal][kFieldUnregister];
    std::printf("-- (2) ZAEHLER derselben Op-Schleife (Anti-Phantom; kV3AxisSchema[T1]) --\n");
    std::printf("     T1 register=%llu  resolve=%llu  resolve_hit=%llu  resolve_miss=%llu  unregister=%llu\n",
                static_cast<unsigned long long>(reg_cnt), static_cast<unsigned long long>(res_cnt),
                static_cast<unsigned long long>(res_hit), static_cast<unsigned long long>(res_miss),
                static_cast<unsigned long long>(unreg));
    tr("(2) T1 [3] register > 0 -- die gemessene Zeit ist durch reale, ALLOZIERENDE Ops gedeckt", reg_cnt > 0);
    tr("(2) T1 [0] resolve > 0 -- die lesende Op derselben Schleife ist ebenfalls real", res_cnt > 0);

    // -- (3) FAMILIEN-SPEZIFISCH: der ehrliche Nullpunkt als Gegenprobe -----------------------------
    std::printf("-- (3) ehrlicher Nullpunkt (die Achse hat KEINE None-Strategie -- zwei Gegenproben) --\n");
    tr("(3a) T1 [4] unregister == 0: der Treiber ruft nie unregister -- kein pauschal hochgezaehlter Zaehler",
       unreg == 0);
    tr("(3b) T1 [1]+[2] == [0]: hit/miss summieren sich exakt auf resolve", res_hit + res_miss == res_cnt);
    tr("(3b) T1 beide Seiten kommen real vor (hit > 0 UND miss > 0) -- ein konstanter Zaehler kann das nicht",
       res_hit > 0 && res_miss > 0);

    // -- (4) LAUFZEIT-VERDRAHTUNGS-ANKER der Form B (Scrub-Gegenstand direkt) -----------------------
    std::printf("-- (4) Verdrahtungs-Anker: laeuft der Speicher REAL ueber die Allokator-Achse? --\n");
#ifdef COMDARE_CE_ENABLE_STATISTICS
    check_allocator_axis_wiring<ct::LinearFanout>("LinearFanout");
    check_allocator_axis_wiring<ct::BinarySearchFanout>("BinarySearchFanout");
    check_allocator_axis_wiring<ct::HashLookup>("HashLookup");
    {
        // Rehash-Pfad: der Neuaufbau MUSS ueber dieselbe Achse laufen (Auftrag (b) explizit).
        ct::HashLookup h{16};
        auto const     before = h.traversal_allocator_statistics();
        h.set_iterable_aspect(1024);
        auto const after = h.traversal_allocator_statistics();
        std::printf("     HashLookup rehash 16 -> 1024: alloc_cnt %llu -> %llu, bucket_count=%zu\n",
                    static_cast<unsigned long long>(before.allocation_count),
                    static_cast<unsigned long long>(after.allocation_count), h.bucket_count());
        tr("(4) HashLookup: der REHASH-Neuaufbau alloziert ueber die Achse (alloc_cnt gestiegen)",
           after.allocation_count > before.allocation_count);
        tr("(4) HashLookup: der Rehash hat die Kapazitaet real gedreht (bucket_count == 1024)",
           h.bucket_count() == 1024u);
    }
#else
    std::printf("     UEBERSPRUNGEN: ohne COMDARE_CE_ENABLE_STATISTICS gibt es keine Allokator-Statistik.\n");
    std::printf("     (Die compile-harte Haelfte des Ankers laeuft unabhaengig davon in der Konformitaets-TU.)\n");
#endif

    std::printf("== test_s5_01d_traversal_perf_sanity: %s ==\n", g_fail == 0 ? "ALLE OK" : "FEHLER");
    return g_fail == 0 ? 0 : 1;
}
