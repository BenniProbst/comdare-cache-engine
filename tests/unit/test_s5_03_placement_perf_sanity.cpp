// A8-S5 Familie 03_placement (mapping + value_handle als Scrub-Objekte; alloc/index_org/migration mit im Bild)
// -- PERF-SANITY-HARNESS.
//
// Dossier: docs/architecture/20260803-a8_f2_benchmarking_schnitt_soll_design.md Abschn. 5-S5
// ("GATE je Familie: Grundkadenz + Konformitaets-GTest der Familie + Perf-Sanity (keine 0-/Phantom-Zeiten)").
// Zweite Haelfte des GATE-MUSTERS: waehrend test_s5_03_placement_alloc_conformance auf TYP-Ebene pinnt (und am
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
// FAMILIEN-SPEZIFISCHE FELD-WAHL (Block (2)/(3)) -- BEGRUENDET, nicht geraten:
//   * T2 mapping  -> axis_stats[T2][0] "register" + [1] "resolve" + [6] "indirect_steps"
//     (Schema-Zeile kV3AxisSchema[T2], observable_tier.hpp:75; Schreiber abi_adapter fill_observer_v3 r[0]/r[1]/r[6]).
//     Das ist EXAKT die Strecke, die der Scrub angefasst hat: register_slot fuellt die geSCRUBbte Tabelle
//     (Achsen-Allokation), resolve_offset liest sie. Die Auto-Kopplung sitzt in tier_insert (abi_adapter:941,
//     mit uint16-Kapazitaets-Guard) bzw. tier_lookup (:1010) -- also DIESELBE Op-Schleife wie die Zeit.
//   * T10 value_handle -> axis_stats[T10][0] "access" + [1] "indirect_deref"
//     (Schema-Zeile kV3AxisSchema[T10]). Der Deref-Zaehler ist die strategie-charakteristische Groesse und
//     zugleich die schaerfste Anti-Phantom-Probe: Inline = 0 (ehrlicher Nullpunkt), ExternalPool = n.
//   * DIREKTER SCRUB-BELEG obendrauf: real_slot().slot_count() -- die geSCRUBbte Pool-Struktur selbst muss
//     unter Last gefuellt sein. Ein Scrub, der das Backing versehentlich stilllegt, faellt hier auf, auch wenn
//     die aggregierten Zaehler noch liefen.
//   * T6 allocator / T11 index_organization / T13 migration_policy gehoeren zur Familie, hatten aber KEINEN
//     Code-Treffer (grep) -- sie werden auf Zeit geprueft (Block (1)), nicht auf familien-spezifische Zaehler:
//     eine Zaehler-Behauptung ueber eine nicht angefasste Achse waere Schmuck, keine Wache.
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

#include <organ_axes/mapping/axis_03m_mapping_direct_placement.hpp>
#include <organ_axes/mapping/axis_03m_mapping_pool_relative.hpp>
#include <organ_axes/persistence_target/axis_persistence_target_memory_only.hpp>
#include <organ_axes/value_handle_axis/axis_14_value_handle_external_pool.hpp>
#include <organ_axes/value_handle_axis/axis_14_value_handle_inline.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string_view>

namespace an    = ::comdare::cache_engine::anatomy;
namespace comp  = ::comdare::cache_engine::compositions;
namespace ce03a = ::comdare::cache_engine::traversal::axis_03a_search_algo;
namespace ex    = ::comdare::cache_engine::builder::experiment;
namespace mapx  = ::comdare::cache_engine::mapping;
namespace vhx   = ::comdare::cache_engine::value_handle_axis;

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
constexpr std::string_view kFamilyAxes[] = {"mapping", "allocator", "value_handle", "index_organization",
                                            "migration_policy"};

constexpr std::size_t kAxisMapping     = axis_index_of("mapping");
constexpr std::size_t kAxisValueHandle = axis_index_of("value_handle");

static_assert(kAxisMapping < an::kV3AxisCount,
              "S5-03 Perf-Sanity: Achse 'mapping' steht nicht mehr in kCompositionAxisNames -- die Wache wuerde "
              "eine falsche seg_ns-Spalte pruefen.");
static_assert(kAxisValueHandle < an::kV3AxisCount,
              "S5-03 Perf-Sanity: Achse 'value_handle' steht nicht mehr in kCompositionAxisNames -- die Wache "
              "wuerde eine falsche seg_ns-Spalte pruefen.");
static_assert(ex::kCompositionAxisNames.size() == an::kV3AxisCount,
              "S5-03 Perf-Sanity: Achsen-Namensliste und Snapshot-Achsenzahl sind auseinandergelaufen.");

// -- Store-getragene Komposition: Array256 (Byte-Domaene) + HOT-Slots, mapping/value_handle variabel.
//    Nur ueber ein REAL befuelltes Store-Backing entsteht ein echter Descent -- eine Huellen-Komposition
//    wuerde die Segment-Treiber blockieren und die Wache um ihre Aussage bringen (Pilot-Erfahrung 04).
template <class MappingStrategy, class ValueHandleStrategy>
using Family03Composition = an::AdHocComposition<
    ce03a::Array256SearchAlgo, comp::HotComposition::cache_traversal, MappingStrategy,
    comp::HotComposition::path_compression, comp::HotComposition::node_type, comp::HotComposition::memory_layout,
    comp::HotComposition::allocator, comp::HotComposition::prefetch, comp::HotComposition::concurrency,
    comp::HotComposition::serialization, ValueHandleStrategy, comp::HotComposition::index_organization,
    comp::HotComposition::io_dispatch, comp::HotComposition::migration_policy, comp::HotComposition::filter,
    comp::HotComposition::queuing_q1, comp::HotComposition::queuing_q2,
    ::comdare::cache_engine::persistence_target::MemoryOnlyTarget>;

[[nodiscard]] std::uint64_t spread_key(std::uint64_t i) noexcept {
    return (i * 2654435761ull) ^ (i << 17) ^ 0x9E3779B97F4A7C15ull;
}

/// Ergebnis eines Treiber-Laufs: der Observer-Snapshot PLUS der direkte Blick in die geSCRUBbte
/// value_handle-Slot-Struktur (die aggregierten Zaehler allein saehen ein stillgelegtes Backing nicht).
struct DriveResult {
    an::ComdareTierObserverSnapshot snap{};
    std::size_t                     vh_slot_count = 0;
    std::size_t                     vh_backing    = 0; // pool_size + chain_nodes
};

/// Treibt ein echtes Tier unter Last (insert + lookup) und zieht EINEN Observer-Snapshot.
template <class MappingStrategy, class ValueHandleStrategy>
[[nodiscard]] DriveResult drive_and_observe(std::uint64_t n_ops) {
    using Anatomy = an::SearchAlgorithmAnatomy<Family03Composition<MappingStrategy, ValueHandleStrategy>>;
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
    // Der Schluessel bleibt damit zugleich unter der uint16-Grenze des mapping-Kapazitaets-Guards
    // (abi_adapter.hpp:941) -- die T2-Auto-Kopplung feuert also bei JEDEM insert.
    for (std::uint64_t i = 0; i < n_ops; ++i) (void)drv->tier_insert(spread_key(i) & 0xFFull, i * 11u + 5u);
    std::uint64_t v = 0;
    for (std::uint64_t i = 0; i < n_ops; ++i) (void)drv->tier_lookup(spread_key(i) & 0xFFull, &v);
    obs->tier_observe(&out.snap);
    auto const& vh    = tier.value_handle_instance().real_slot();
    out.vh_slot_count = vh.slot_count();
    out.vh_backing    = vh.pool_size() + vh.chain_nodes();
    return out;
}

} // namespace

int main() {
    std::printf("== A8-S5 Familie 03_placement -- Perf-Sanity (keine 0-/Phantom-Zeiten) ==\n");
    constexpr std::uint64_t kLoad = 2000;

    // Familien-Besetzung unter Last: die geSCRUBbte Slot-Tabelle (DirectPlacement) + das geSCRUBbte
    // Pool-Backing (ExternalPool). Beide Speicher laufen seit dem Scrub ueber die Allokator-Achse.
    auto const direct_pool = drive_and_observe<mapx::DirectPlacement, vhx::ExternalPoolValueHandle>(kLoad);
    // Gegenprobe 1: dieselbe Last, aber die ANDERE mapping-Variante (Indirektions-Kontrast 1 vs. 2 Schritte).
    auto const poolrel_pool = drive_and_observe<mapx::PoolRelative, vhx::ExternalPoolValueHandle>(kLoad);
    // Gegenprobe 2: der ehrliche Nullpunkt der value_handle-Achse (Inline = M3-Pin, EmptyRealSlot).
    auto const direct_inline = drive_and_observe<mapx::DirectPlacement, vhx::InlineValueHandle>(kLoad);

    // -- (1) ZEIT -------------------------------------------------------------------------------------
    std::printf("-- (1) ZEIT: seg_ns der Familien-Achsen unter Last (n_ops=%llu je Phase) --\n",
                static_cast<unsigned long long>(kLoad));
    for (auto const axis : kFamilyAxes) {
        std::size_t const  idx = axis_index_of(axis);
        std::int64_t const ns  = direct_pool.snap.seg_ns[idx];
        std::printf("     seg_ns[T%zu %-18.*s] = %lld ns\n", idx, static_cast<int>(axis.size()), axis.data(),
                    static_cast<long long>(ns));
    }
    std::printf("     seg_run_total_ns = %lld ns   batches_measured = %llu\n",
                static_cast<long long>(direct_pool.snap.seg_run_total_ns),
                static_cast<unsigned long long>(direct_pool.snap.batches_measured));

    tr("(1) der Segment-Lauf hat ueberhaupt stattgefunden (batches_measured > 0)",
       direct_pool.snap.batches_measured > 0);
    for (auto const axis : kFamilyAxes) {
        std::size_t const idx = axis_index_of(axis);
        if (axis == std::string_view{"mapping"})
            tr("(1) seg_ns[mapping] > 0 unter Last (keine 0-Zeit)", direct_pool.snap.seg_ns[idx] > 0);
        else if (axis == std::string_view{"allocator"})
            tr("(1) seg_ns[allocator] > 0 unter Last (Versorger-Achse der Familie)", direct_pool.snap.seg_ns[idx] > 0);
        else if (axis == std::string_view{"value_handle"})
            tr("(1) seg_ns[value_handle] > 0 unter Last (keine 0-Zeit)", direct_pool.snap.seg_ns[idx] > 0);
        else if (axis == std::string_view{"index_organization"})
            tr("(1) seg_ns[index_organization] > 0 unter Last (kein Scrub, aber Familien-Mitglied)",
               direct_pool.snap.seg_ns[idx] > 0);
        else
            tr("(1) seg_ns[migration_policy] > 0 unter Last (kein Scrub, aber Familien-Mitglied)",
               direct_pool.snap.seg_ns[idx] > 0);
    }

    // -- (2) FAMILIEN-SPEZIFISCH: die Zaehler DERSELBEN Op-Schleife (Anti-Phantom) ---------------------
    std::uint64_t const map_register = direct_pool.snap.axis_stats[kAxisMapping][0];
    std::uint64_t const map_resolve  = direct_pool.snap.axis_stats[kAxisMapping][1];
    std::uint64_t const map_steps    = direct_pool.snap.axis_stats[kAxisMapping][6];
    std::uint64_t const vh_access    = direct_pool.snap.axis_stats[kAxisValueHandle][0];
    std::uint64_t const vh_deref     = direct_pool.snap.axis_stats[kAxisValueHandle][1];
    std::printf("-- (2) ZAEHLER derselben Op-Schleife (Anti-Phantom) --\n");
    std::printf("     T2  register = %llu   resolve = %llu   indirect_steps = %llu\n",
                static_cast<unsigned long long>(map_register), static_cast<unsigned long long>(map_resolve),
                static_cast<unsigned long long>(map_steps));
    std::printf("     T10 access = %llu   indirect_deref = %llu   real_slot: slots=%zu pool+chain=%zu\n",
                static_cast<unsigned long long>(vh_access), static_cast<unsigned long long>(vh_deref),
                direct_pool.vh_slot_count, direct_pool.vh_backing);

    tr("(2) T2: die gemessene Zeit ist durch reale register_slot-Ops gedeckt (register > 0)", map_register > 0);
    tr("(2) T2: und durch reale resolve_offset-Ops (resolve > 0)", map_resolve > 0);
    tr("(2) T2: die geSCRUBbte Tabelle wird auch gelesen (indirect_steps > 0)", map_steps > 0);
    tr("(2) T10: die gemessene Zeit ist durch reale Slot-Zugriffe gedeckt (access > 0)", vh_access > 0);
    tr("(2) T10: die externe Pool-Indirektion wird real durchlaufen (indirect_deref > 0)", vh_deref > 0);
    tr("(2) SCRUB-BELEG: das geSCRUBbte Pool-Backing ist unter Last gefuellt (slot_count > 0)",
       direct_pool.vh_slot_count > 0);
    tr("(2) SCRUB-BELEG: und der externe Pool traegt mindestens so viele Eintraege wie Slots",
       direct_pool.vh_backing >= direct_pool.vh_slot_count);

    // -- (3) FAMILIEN-SPEZIFISCH: ehrliche Nullpunkte + Strategie-Kontrast -----------------------------
    std::uint64_t const inl_deref  = direct_inline.snap.axis_stats[kAxisValueHandle][1];
    std::uint64_t const inl_access = direct_inline.snap.axis_stats[kAxisValueHandle][0];
    std::uint64_t const pr_steps   = poolrel_pool.snap.axis_stats[kAxisMapping][6];
    std::uint64_t const pr_resolve = poolrel_pool.snap.axis_stats[kAxisMapping][1];
    std::printf("-- (3) ehrlicher Nullpunkt + Strategie-Kontrast --\n");
    std::printf("     Inline: access = %llu   indirect_deref = %llu   real_slot slots=%zu (EmptyRealSlot)\n",
                static_cast<unsigned long long>(inl_access), static_cast<unsigned long long>(inl_deref),
                direct_inline.vh_slot_count);
    std::printf("     mapping: direct steps/resolve = %llu/%llu   pool_relative steps/resolve = %llu/%llu\n",
                static_cast<unsigned long long>(map_steps), static_cast<unsigned long long>(map_resolve),
                static_cast<unsigned long long>(pr_steps), static_cast<unsigned long long>(pr_resolve));

    tr("(3) Inline meldet 0 zusaetzliche Derefs (deklarierter Nullpunkt, kein erfundener Wert)", inl_deref == 0);
    tr("(3) Inline wird trotzdem real gescannt (access > 0) -- der Nullpunkt ist ehrlich, nicht stumm", inl_access > 0);
    tr("(3) Inline haelt weiterhin KEIN Slot-Backing (EmptyRealSlot, M3-Pin messneutral)",
       direct_inline.vh_slot_count == 0);
    tr("(3) Strategie-Kontrast T10: ExternalPool > Inline -- die Zeit haengt an der Achse, nicht am Framework",
       vh_deref > inl_deref);
    // MP01 zaehlt 1 Adress-Aufloesung je resolve, MP02 deren 2 (pool_base-Rebase) -- das ist die
    // Achsen-Charakteristik, die T2 ueberhaupt messbar macht (axis_03m_*:92 bzw. :103).
    tr("(3) Strategie-Kontrast T2: pool_relative == 2 Schritte je resolve (Rebase-Translation)",
       pr_resolve > 0 && pr_steps == 2u * pr_resolve);
    tr("(3) Strategie-Kontrast T2: direct_placement == 1 Schritt je resolve (absoluter Offset)",
       map_resolve > 0 && map_steps == map_resolve);

    std::printf("== test_s5_03_placement_perf_sanity: %s ==\n", g_fail == 0 ? "ALLE OK" : "FEHLER");
    return g_fail == 0 ? 0 : 1;
}
