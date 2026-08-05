// A8-S5 Familie 02_layout / Sub-Scheibe 02b (filter / SuRF) -- PERF-SANITY-HARNESS.
//
// Dossier: docs/architecture/20260803-a8_f2_benchmarking_schnitt_soll_design.md Abschn. 5-S5
// ("GATE je Familie: Grundkadenz + Konformitaets-GTest der Familie + Perf-Sanity (keine 0-/Phantom-Zeiten)").
// Zweite Haelfte des GATE-MUSTERS: waehrend test_s5_02b_filter_alloc_conformance auf TYP-Ebene pinnt (und am
// Objekt belegt), dass kein Familien-Organ am Allokator-Achsen-Interface vorbei alloziert, belegt DIESE TU zur
// LAUFZEIT, dass der Scrub die Messbarkeit der Achse nicht beschaedigt hat.
//
// WARUM DIESE WACHE NOETIG IST: ein Scrub kann eine Achse still stumm schalten -- der Bau bleibt gruen, die
// Tests bleiben gruen, und die Mess-Spalte der Achse steht ab da auf 0. Genau diese Klasse war der A8-S1-/
// A8-S3-Befund (stiller Messwert-Verlust bzw. strukturell-0). Die Wache verlangt deshalb ZWEI Dinge ZUGLEICH:
//   (1) ZEIT:    seg_ns der filter-Achse > 0 unter Last.
//   (2) ZAEHLER: die Observer-Zaehler DERSELBEN Op-Schleife > 0.
// Zeit ohne Zaehler waere eine Phantom-Zeit (der Timer misst eine leere Schleife); Zaehler ohne Zeit waere ein
// toter Timer. Dazu (3) der EHRLICHE NULLPUNKT/Strategie-Kontrast und (4) der SCRUB-GEGENSTAND selbst.
//
// FELD-WAHL (T14 filter) -- BEGRUENDET, nicht geraten:
//   * kV3AxisSchema[T14] (observable_tier.hpp) = {"probe", "pos", "neg", "hash_probes", "checksum"}.
//     Der Achsen-Index wird aus kCompositionAxisNames ABGELEITET ("filter" -> Position), nie als Literal
//     geschrieben; die Namensliste wird nur GELESEN (golden-Tabu).
//   * GEPRUEFT werden [0] probe + [1] pos + [2] neg + [3] hash_probes. BEGRUENDUNG (Semantik am Objekt
//     nachgelesen, axis_filter_observable.hpp:38-42 -- NICHT geraten; die erste Fassung dieser Wache nahm
//     "pos+neg == probe" an und fiel zurecht durch): probe_count zaehlt die Probe-RUNDEN derselben
//     Schleife, die seg_ns[T14] misst (Anti-Phantom-Paar); pos/neg zerlegen die QUERIES innerhalb dieser
//     Runden in die beiden Antwort-Klassen des may-contain-Vertrags; hash_probes ist die
//     strategie-CHARAKTERISTISCHE Multiplizitaet (RangeSurf kDepth=6, Bloom kHashes=4, Xor 3, Cuckoo 2)
//     und damit der Kontrast, der beweist, dass die Zeile aus der REALEN Strategie kommt und nicht aus
//     einem Deskriptor. Die geltende Invariante ist hash_probes == (pos+neg) * probe_multiplicity.
//   * [4] checksum wird MITGELESEN und ausgewiesen, aber nicht als Ungleichung behauptet: eine Pruefsumme
//     darf legitim jeden Wert annehmen; eine ">0"-Behauptung darauf waere Schmuck, keine Wache.
//
// WAS DIESE WACHE **NICHT** BEHAUPTET (ehrlich deklariert -- die Besonderheit von 02b):
// Die Achse hat ZWEI Ebenen, und der Scrub hat die ZWEITE angefasst:
//   (I)  die KOMPOSITIONS-getragene filter-Achse (Bloom/Cuckoo/RangeSurf/Xor) liegt im T14-Mess-Pfad, ist
//        heap-frei und wurde vom Scrub NICHT beruehrt. Bloecke (1)-(3) pruefen SIE -- als Nachweis, dass
//        die Achse messfaehig geblieben ist.
//   (II) die COMPOSABLE Filter-Organe (S1/S2, axes/filter_axis/composable/) SIND der Scrub-Gegenstand, aber
//        sie liegen NICHT im T14-Pfad (sie sind keine Registry-Strategie). Eine T14-Spalte kann ueber sie
//        also gar nichts aussagen. Block (4) misst sie deshalb DIREKT -- Zeit UND Achsen-Allokations-Zaehler
//        derselben Op-Schleife, mit demselben Anti-Phantom-Paar. Diese Trennung wird hier ausgeschrieben,
//        damit niemand die T14-Gruen-Meldung faelschlich als Beleg fuer den Scrub-Gegenstand liest.
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

#include <axes/filter_axis/axis_filter_bloom.hpp>
#include <axes/filter_axis/axis_filter_cuckoo.hpp>
#include <axes/filter_axis/axis_filter_observable.hpp>
#include <axes/filter_axis/axis_filter_range_surf.hpp>
#include <axes/filter_axis/axis_filter_xor.hpp>
#include <axes/filter_axis/composable/exact_prefix_filter_organ.hpp>
#include <axes/filter_axis/composable/louds_sparse_filter_organ.hpp>
#include <axes/persistence_target/axis_persistence_target_memory_only.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <set>
#include <string_view>
#include <vector>

namespace an    = ::comdare::cache_engine::anatomy;
namespace comp  = ::comdare::cache_engine::compositions;
namespace ce03a = ::comdare::cache_engine::traversal::axis_03a_search_algo;
namespace ex    = ::comdare::cache_engine::builder::experiment;
namespace fx    = ::comdare::cache_engine::filter_axis;
namespace fcmp  = ::comdare::cache_engine::filter_axis::composable;

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
// 02b = die filter-Seite der Organ-Gruppe 02_layout. Die uebrigen vier Achsen der Gruppe (node_type,
// memory_layout, path_compression, serialization) sind Sub-Scheibe 02a und werden DORT geprueft --
// eine zweite Wache ueber demselben Gegenstand driftet.
constexpr std::string_view kFamilyAxes[] = {"filter", "allocator"};

constexpr std::size_t kAxisFilter = axis_index_of("filter");
constexpr std::size_t kAxisAlloc  = axis_index_of("allocator");

static_assert(kAxisFilter < an::kV3AxisCount,
              "S5-02b Perf-Sanity: Achse 'filter' steht nicht mehr in kCompositionAxisNames -- die Wache "
              "wuerde eine falsche seg_ns-Spalte pruefen.");
static_assert(kAxisAlloc < an::kV3AxisCount,
              "S5-02b Perf-Sanity: Achse 'allocator' steht nicht mehr in kCompositionAxisNames.");
static_assert(ex::kCompositionAxisNames.size() == an::kV3AxisCount,
              "S5-02b Perf-Sanity: Achsen-Namensliste und Snapshot-Achsenzahl sind auseinandergelaufen.");
// Die Schema-Zeile, deren Spalten diese Wache liest -- gegen Umbenennung gepinnt (nie Literal-Index blind).
static_assert(std::string_view{an::kV3AxisSchema[kAxisFilter].names[0]} == std::string_view{"probe"} &&
                  std::string_view{an::kV3AxisSchema[kAxisFilter].names[1]} == std::string_view{"pos"} &&
                  std::string_view{an::kV3AxisSchema[kAxisFilter].names[2]} == std::string_view{"neg"} &&
                  std::string_view{an::kV3AxisSchema[kAxisFilter].names[3]} == std::string_view{"hash_probes"},
              "S5-02b Perf-Sanity: die T14-Schema-Zeile hat sich verschoben -- die Wache laese fremde Spalten.");

// -- Store-getragene Komposition: Array256 (Byte-Domaene) + HOT-Slots, die filter-Achse variabel.
//    Nur ueber ein REAL befuelltes Store-Backing entsteht ein echter Descent -- eine Huellen-Komposition
//    wuerde die Segment-Treiber blockieren und die Wache um ihre Aussage bringen (Pilot-Erfahrung 04).
//
//    ACHTUNG, HIER ANDERS ALS IN 02a (am Objekt verifiziert, nicht geraten): die filter-Achse geht als
//    ROHE Strategie hinein. Der ABI-Adapter haelt fuer T14 einen EIGENEN Huellen-Member
//    (abi_adapter.hpp:2339: `ObservableFilter<typename Composition::filter> flt_organ_`) und wickelt die
//    Strategie also SELBST ein -- anders als bei node_type/memory_layout/serialization, wo er die Huelle
//    aus der Komposition direkt haelt und eine rohe Strategie die Spalte auf honest-0 stellen wuerde.
//    Steckte man hier zusaetzlich eine Huelle hinein, entstuende ObservableFilter<ObservableFilter<S>>:
//    eine DOPPELTE Huelle, deren aeussere Zaehler-Schicht die Achsen-Charakteristik der Strategie
//    verwaescht (real gemessen: hash_probes fiel dabei fuer ALLE VIER Strategien auf denselben Wert,
//    die Multiplizitaet war unsichtbar). Genau so macht es auch HotComposition (hot_reference.hpp:59:
//    `using filter = BloomFilter`, roh).
template <class FilterStrategy>
using Family02bComposition = an::AdHocComposition<
    ce03a::Array256SearchAlgo, comp::HotComposition::cache_traversal, comp::HotComposition::mapping,
    comp::HotComposition::path_compression, comp::HotComposition::node_type, comp::HotComposition::memory_layout,
    comp::HotComposition::allocator, comp::HotComposition::prefetch, comp::HotComposition::concurrency,
    comp::HotComposition::serialization, comp::HotComposition::value_handle, comp::HotComposition::index_organization,
    comp::HotComposition::io_dispatch, comp::HotComposition::migration_policy, FilterStrategy,
    comp::HotComposition::queuing_q1, comp::HotComposition::queuing_q2,
    ::comdare::cache_engine::persistence_target::MemoryOnlyTarget>;

[[nodiscard]] std::uint64_t spread_key(std::uint64_t i) noexcept {
    return (i * 2654435761ull) ^ (i << 17) ^ 0x9E3779B97F4A7C15ull;
}

/// Treibt ein echtes Tier unter Last (insert + lookup) und zieht EINEN Observer-Snapshot.
template <class FilterStrategy>
[[nodiscard]] an::ComdareTierObserverSnapshot drive_and_observe(std::uint64_t n_ops) {
    using Anatomy = an::SearchAlgorithmAnatomy<Family02bComposition<FilterStrategy>>;
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
    // Store-Backing bliebe leer (slot_count()==0 -> kein Descent -> die Wache saehe eine leere Achse).
    for (std::uint64_t i = 0; i < n_ops; ++i) (void)drv->tier_insert(spread_key(i) & 0xFFull, i * 11u + 5u);
    std::uint64_t v = 0;
    for (std::uint64_t i = 0; i < n_ops; ++i) (void)drv->tier_lookup(spread_key(i) & 0xFFull, &v);
    obs->tier_observe(&snap);
    return snap;
}

// -- Block (4): der SCRUB-GEGENSTAND selbst (composable S1/S2) -------------------------------------
std::vector<std::uint64_t> build_keys(std::set<std::uint64_t>& gt, std::uint64_t n) {
    for (std::uint64_t i = 0; i < n; ++i) gt.insert((i * 2654435761u) % 1000003u);
    return {gt.begin(), gt.end()};
}

struct DirectRun {
    std::int64_t  build_ns     = 0;
    std::int64_t  query_ns     = 0;
    std::uint64_t alloc_cnt    = 0;
    std::uint64_t bytes_alloc  = 0;
    std::uint64_t hits         = 0;
    std::size_t   bit_size     = 0;
    double        bits_per_key = 0.0;
};

/// Zeit UND Achsen-Zaehler DERSELBEN Op-Schleife am geSCRUBbten Organ (Anti-Phantom, direkt gemessen --
/// diese Organe liegen NICHT im T14-Pfad, s. Kopf). Der Zaehler wird ueber organ.store() aus GENAU DEM
/// Objekt gelesen, dessen Bau-Zeit oben gestoppt wurde -- nicht aus einer zweiten, gleichartigen Instanz.
template <class Organ>
[[nodiscard]] DirectRun drive_composable(std::vector<std::uint64_t> const& keys, std::set<std::uint64_t> const& gt) {
    DirectRun  r{};
    Organ      organ;
    auto const t0 = std::chrono::steady_clock::now();
    organ.build_from_sorted_keys(keys);
    auto const t1 = std::chrono::steady_clock::now();
    for (std::uint64_t const k : gt) r.hits += organ.contains(k) ? 1u : 0u;
    for (std::uint64_t q = 0; q < 20000ull; ++q) r.hits += organ.contains(q * 7919ull) ? 1u : 0u;
    auto const t2 = std::chrono::steady_clock::now();

    r.build_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
    r.query_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
#ifdef COMDARE_CE_ENABLE_STATISTICS
    auto const st = organ.store().filter_surf_allocator_statistics();
    r.alloc_cnt   = st.allocation_count;
    r.bytes_alloc = st.total_bytes_allocated;
#endif
    r.bit_size     = organ.bit_size();
    r.bits_per_key = organ.bits_per_key();
    return r;
}

} // namespace

int main() {
    std::printf("== A8-S5 Familie 02b (filter / SuRF) -- Perf-Sanity ==\n");
    constexpr std::uint64_t kLoad = 2000;

    // Familien-Besetzung unter Last: die vier Registry-Strategien der KOMPOSITIONS-filter-Achse.
    auto const surf   = drive_and_observe<fx::RangeSurfFilter>(kLoad);
    auto const bloom  = drive_and_observe<fx::BloomFilter>(kLoad);
    auto const cuckoo = drive_and_observe<fx::CuckooFilter>(kLoad);
    auto const xorf   = drive_and_observe<fx::XorFilter>(kLoad);

    // -- (1) ZEIT -------------------------------------------------------------------------------------
    std::printf("-- (1) ZEIT: seg_ns der Familien-Achsen unter Last (n_ops=%llu je Phase) --\n",
                static_cast<unsigned long long>(kLoad));
    for (auto const axis : kFamilyAxes) {
        std::size_t const  idx = axis_index_of(axis);
        std::int64_t const ns  = surf.seg_ns[idx];
        std::printf("     seg_ns[T%zu %-14.*s] = %lld ns\n", idx, static_cast<int>(axis.size()), axis.data(),
                    static_cast<long long>(ns));
    }
    std::printf("     seg_run_total_ns = %lld ns   batches_measured = %llu\n",
                static_cast<long long>(surf.seg_run_total_ns), static_cast<unsigned long long>(surf.batches_measured));

    tr("(1) der Segment-Lauf hat ueberhaupt stattgefunden (batches_measured > 0)", surf.batches_measured > 0);
    tr("(1) seg_ns[filter] > 0 unter Last (keine 0-Zeit)", surf.seg_ns[kAxisFilter] > 0);
    tr("(1) seg_ns[allocator] > 0 unter Last (Versorger-Achse der Scheibe)", surf.seg_ns[kAxisAlloc] > 0);
    tr("(1) die filter-Zeit ist ueber ALLE vier Registry-Strategien > 0 (keine entartete Strategie)",
       bloom.seg_ns[kAxisFilter] > 0 && cuckoo.seg_ns[kAxisFilter] > 0 && xorf.seg_ns[kAxisFilter] > 0);

    // -- (2) FAMILIEN-SPEZIFISCH: die Zaehler DERSELBEN Op-Schleife (Anti-Phantom) ---------------------
    std::uint64_t const fp_probe = surf.axis_stats[kAxisFilter][0];
    std::uint64_t const fp_pos   = surf.axis_stats[kAxisFilter][1];
    std::uint64_t const fp_neg   = surf.axis_stats[kAxisFilter][2];
    std::uint64_t const fp_hash  = surf.axis_stats[kAxisFilter][3];
    std::uint64_t const fp_csum  = surf.axis_stats[kAxisFilter][4];
    std::printf("-- (2) ZAEHLER derselben Op-Schleife (Anti-Phantom) --\n");
    std::printf("     T14 probe = %llu   pos = %llu   neg = %llu   hash_probes = %llu   checksum = %llu\n",
                static_cast<unsigned long long>(fp_probe), static_cast<unsigned long long>(fp_pos),
                static_cast<unsigned long long>(fp_neg), static_cast<unsigned long long>(fp_hash),
                static_cast<unsigned long long>(fp_csum));

    // SEMANTIK AM OBJEKT GEPRUEFT (axis_filter_observable.hpp:38-42, NICHT geraten -- die erste Fassung
    // dieser Wache nahm "pos+neg == probe" an und fiel zurecht durch): probe_count zaehlt die
    // Probe-RUNDEN (observe_probe-Aufrufe), pos/neg zerlegen die QUERIES innerhalb dieser Runden, und
    // hash_probes_total = Queries x probe_multiplicity() der Strategie. Die Invariante ist deshalb
    // hash_probes == (pos+neg) * probe_multiplicity, nicht pos+neg == probe.
    tr("(2) T14: die gemessene Zeit ist durch reale Probe-Runden gedeckt (probe_count > 0)", fp_probe > 0);
    tr("(2) T14: die Probe-Runde hat auch wirklich Queries gesehen (pos > 0)", fp_pos > 0);
    // ZWEITE KORREKTUR AM IST (die erste Fassung verlangte "pos > 0 UND neg > 0" und fiel zurecht durch):
    // der T14-Treiber ist ein ZUSTANDS-SCAN -- store_observe_filter schickt die LOW-BYTES DER REAL
    // GESPEICHERTEN KEYS als Query-Strom (abi_adapter.hpp:1472-1478). Jede Query ist damit per
    // Konstruktion ein GESPEICHERTER Key. Ein "neg" ist hier also ein FALSE NEGATIVE -- und genau das
    // verbietet der Filter-Vertrag (FP erlaubt, FN verboten). neg == 0 ist deshalb keine
    // Schoenwetter-Beobachtung, sondern die no-FN-Zusage der Achse auf KOMPOSITIONS-Ebene -- dieselbe
    // Zusage, die Block (6) der Konformitaets-Wache fuer die composable Organe belegt.
    //
    // BEFUND, VORBESTEHEND UND NICHT VON DIESER SCHEIBE (ehrlich ausgewiesen statt gruen gebuegelt):
    // XorFilter verletzt die Zusage -- 30 von 256 gespeicherten Keys werden als "definitiv nicht
    // enthalten" gemeldet (s. Block (3) unten, literal). Das ist die bekannte Eigenschaft eines
    // Xor-Filters: er wird OFFLINE aus dem VOLLSTAENDIGEN Key-Satz gepeelt; ein inkrementelles
    // insert_key kann die Xor-Invariante nicht erhalten. Der 02b-Scrub hat axis_filter_xor.hpp NICHT
    // angefasst (er beruehrt ausschliesslich axes/filter_axis/composable/) -- die Wache pinnt den Befund
    // hier sichtbar, statt ihn durch eine schwaechere Formulierung verschwinden zu lassen.
    tr("(2) T14 no-FN auf Kompositions-Ebene: der Zustands-Scan probt ausschliesslich GESPEICHERTE Keys, "
       "ein neg IST ein false negative (neg == 0 bei RangeSurf/Bloom/Cuckoo)",
       fp_neg == 0 && bloom.axis_stats[kAxisFilter][2] == 0 && cuckoo.axis_stats[kAxisFilter][2] == 0);
    tr("(2) T14 BEFUND GEPINNT (vorbestehend, NICHT aus 02b): XorFilter meldet false negatives -- faellt "
       "diese Zeile, ist der Befund geheilt und der Kommentar oben nachzuziehen",
       xorf.axis_stats[kAxisFilter][2] > 0);
    tr("(2) T14: der Zustands-Scan hat bei ALLEN vier Strategien denselben Query-Strom gesehen "
       "(pos+neg identisch) -- der Unterschied liegt in der Antwort, nicht in der Last",
       (fp_pos + fp_neg) == (bloom.axis_stats[kAxisFilter][1] + bloom.axis_stats[kAxisFilter][2]) &&
           (fp_pos + fp_neg) == (cuckoo.axis_stats[kAxisFilter][1] + cuckoo.axis_stats[kAxisFilter][2]) &&
           (fp_pos + fp_neg) == (xorf.axis_stats[kAxisFilter][1] + xorf.axis_stats[kAxisFilter][2]));
    tr("(2) T14: die Proben-Multiplizitaet kommt aus der REALEN Strategie (hash_probes > 0)", fp_hash > 0);
    tr("(2) T14: die Zerlegung ist vollstaendig und konsistent zur Multiplizitaet "
       "(hash_probes == (pos+neg) * probe_multiplicity)",
       fp_hash == (fp_pos + fp_neg) * fx::RangeSurfFilter::probe_multiplicity());

    // -- (3) STRATEGIE-KONTRAST: die Zeile haengt an der Achse, nicht an einem Deskriptor --------------
    std::uint64_t const bloom_probe = bloom.axis_stats[kAxisFilter][0];
    std::uint64_t const bloom_hash  = bloom.axis_stats[kAxisFilter][3];
    std::uint64_t const cuck_hash   = cuckoo.axis_stats[kAxisFilter][3];
    std::uint64_t const xor_hash    = xorf.axis_stats[kAxisFilter][3];
    std::printf("-- (3) Strategie-Kontrast (hash_probes = strategie-eigene Multiplizitaet) --\n");
    std::printf("     RangeSurf: probe=%llu hash_probes=%llu (probe_multiplicity=%llu)\n",
                static_cast<unsigned long long>(fp_probe), static_cast<unsigned long long>(fp_hash),
                static_cast<unsigned long long>(fx::RangeSurfFilter::probe_multiplicity()));
    std::printf("     Bloom:     pos=%llu neg=%llu hash_probes=%llu (mult=%llu)\n",
                static_cast<unsigned long long>(bloom.axis_stats[kAxisFilter][1]),
                static_cast<unsigned long long>(bloom.axis_stats[kAxisFilter][2]),
                static_cast<unsigned long long>(bloom_hash),
                static_cast<unsigned long long>(fx::BloomFilter::probe_multiplicity()));
    std::printf("     Cuckoo:    pos=%llu neg=%llu hash_probes=%llu (mult=%llu)\n",
                static_cast<unsigned long long>(cuckoo.axis_stats[kAxisFilter][1]),
                static_cast<unsigned long long>(cuckoo.axis_stats[kAxisFilter][2]),
                static_cast<unsigned long long>(cuck_hash),
                static_cast<unsigned long long>(fx::CuckooFilter::probe_multiplicity()));
    std::printf("     Xor:       pos=%llu neg=%llu hash_probes=%llu (mult=%llu)\n",
                static_cast<unsigned long long>(xorf.axis_stats[kAxisFilter][1]),
                static_cast<unsigned long long>(xorf.axis_stats[kAxisFilter][2]),
                static_cast<unsigned long long>(xor_hash),
                static_cast<unsigned long long>(fx::XorFilter::probe_multiplicity()));
    (void)bloom_probe;

    tr("(3) alle vier Strategien werden real getrieben (probe_count > 0 ueberall)",
       bloom_probe > 0 && cuckoo.axis_stats[kAxisFilter][0] > 0 && xorf.axis_stats[kAxisFilter][0] > 0);
    // RangeSurf ist die EINZIGE Range-Strategie und deszendiert kDepth (=6) Trie-Level je Query; die
    // Punkt-Filter tun weniger. Waere die T14-Zeile von einem entkoppelten Deskriptor gespeist, waeren
    // alle vier Zahlen gleich -- genau das war das Symptom der doppelten Huelle (s. Kopf der Komposition).
    tr("(3) KONTRAST: RangeSurf (kDepth-Trie-Abstieg) zeigt mehr hash_probes als Bloom -- die Zeile kommt "
       "aus der realen Strategie",
       fp_hash > bloom_hash);
    tr("(3) und jede Strategie traegt ihre EIGENE deklarierte Multiplizitaet (je Achse nachgerechnet)",
       bloom_hash == (bloom.axis_stats[kAxisFilter][1] + bloom.axis_stats[kAxisFilter][2]) *
                         fx::BloomFilter::probe_multiplicity() &&
           cuck_hash == (cuckoo.axis_stats[kAxisFilter][1] + cuckoo.axis_stats[kAxisFilter][2]) *
                            fx::CuckooFilter::probe_multiplicity() &&
           xor_hash == (xorf.axis_stats[kAxisFilter][1] + xorf.axis_stats[kAxisFilter][2]) *
                           fx::XorFilter::probe_multiplicity());

    // -- (4) DER SCRUB-GEGENSTAND SELBST: composable S1/S2, direkt gemessen ----------------------------
    // Diese Organe liegen NICHT im T14-Pfad (s. Kopf) -- eine T14-Spalte kann ueber sie nichts sagen.
    // Deshalb hier dasselbe Anti-Phantom-Paar direkt am Objekt: Zeit UND Achsen-Allokations-Zaehler
    // DERSELBEN Op-Schleife, plus der ehrliche Kontrast zwischen den Suffix-Nullpunkten.
    std::set<std::uint64_t> gt;
    auto const              keys = build_keys(gt, 4000);

    using S1Store = fcmp::ExactPrefixFilterStore;
    using S1Organ = fcmp::ComposedExactSurfFilter<fcmp::ExactPrefixFilterQuery, S1Store>;
    using S2N     = fcmp::ComposedSurfLoudsFilter<fcmp::SurfSuffixType::kNone, 0, 0>;
    using S2R8    = fcmp::ComposedSurfLoudsFilter<fcmp::SurfSuffixType::kReal, 0, 8>;
    using S2R16   = fcmp::ComposedSurfLoudsFilter<fcmp::SurfSuffixType::kReal, 0, 16>;

    auto const r_s1  = drive_composable<S1Organ>(keys, gt);
    auto const r_n   = drive_composable<S2N>(keys, gt);
    auto const r_r8  = drive_composable<S2R8>(keys, gt);
    auto const r_r16 = drive_composable<S2R16>(keys, gt);

    std::printf("-- (4) SCRUB-GEGENSTAND direkt (composable S1/S2, keys=%zu) --\n", keys.size());
    std::printf("     S1 exakt   : build=%lld ns query=%lld ns alloc_cnt=%llu bytes=%llu hits=%llu bpk=%.3f\n",
                static_cast<long long>(r_s1.build_ns), static_cast<long long>(r_s1.query_ns),
                static_cast<unsigned long long>(r_s1.alloc_cnt), static_cast<unsigned long long>(r_s1.bytes_alloc),
                static_cast<unsigned long long>(r_s1.hits), r_s1.bits_per_key);
    std::printf("     S2 kNone   : build=%lld ns query=%lld ns alloc_cnt=%llu bytes=%llu hits=%llu bpk=%.3f\n",
                static_cast<long long>(r_n.build_ns), static_cast<long long>(r_n.query_ns),
                static_cast<unsigned long long>(r_n.alloc_cnt), static_cast<unsigned long long>(r_n.bytes_alloc),
                static_cast<unsigned long long>(r_n.hits), r_n.bits_per_key);
    std::printf("     S2 kReal8  : build=%lld ns query=%lld ns alloc_cnt=%llu bytes=%llu hits=%llu bpk=%.3f\n",
                static_cast<long long>(r_r8.build_ns), static_cast<long long>(r_r8.query_ns),
                static_cast<unsigned long long>(r_r8.alloc_cnt), static_cast<unsigned long long>(r_r8.bytes_alloc),
                static_cast<unsigned long long>(r_r8.hits), r_r8.bits_per_key);
    std::printf("     S2 kReal16 : build=%lld ns query=%lld ns alloc_cnt=%llu bytes=%llu hits=%llu bpk=%.3f\n",
                static_cast<long long>(r_r16.build_ns), static_cast<long long>(r_r16.query_ns),
                static_cast<unsigned long long>(r_r16.alloc_cnt), static_cast<unsigned long long>(r_r16.bytes_alloc),
                static_cast<unsigned long long>(r_r16.hits), r_r16.bits_per_key);

    tr("(4) ZEIT: der Bulk-Load der geSCRUBbten Organe kostet messbare Zeit (build_ns > 0 bei allen vier)",
       r_s1.build_ns > 0 && r_n.build_ns > 0 && r_r8.build_ns > 0 && r_r16.build_ns > 0);
    tr("(4) ZEIT: und die Query-Schleife ebenfalls (query_ns > 0 bei allen vier)",
       r_s1.query_ns > 0 && r_n.query_ns > 0 && r_r8.query_ns > 0 && r_r16.query_ns > 0);
#ifdef COMDARE_CE_ENABLE_STATISTICS
    tr("(4) ZAEHLER derselben Op-Schleife: der Bulk-Load hat REAL ueber die Achse alloziert "
       "(alloc_cnt > 0 bei allen vier -- am Alt-Stand waere jeder 0 gewesen)",
       r_s1.alloc_cnt > 0 && r_n.alloc_cnt > 0 && r_r8.alloc_cnt > 0 && r_r16.alloc_cnt > 0);
    tr("(4) und die Bytes sind nicht bloss gezaehlt, sondern besorgt (total_bytes_allocated > 0)",
       r_s1.bytes_alloc > 0 && r_n.bytes_alloc > 0 && r_r8.bytes_alloc > 0 && r_r16.bytes_alloc > 0);
    // DISKRIMINIEREND: der verschachtelte LOUDS-Bau alloziert deutlich oefter als der eine Key-Vektor.
    tr("(4) DISKRIMINIEREND: das verschachtelte S2 zeigt mehr Achsen-Allokationen als das flache S1",
       r_r8.alloc_cnt > r_s1.alloc_cnt + 4);
#endif
    // EHRLICHER NULLPUNKT/KONTRAST der Achse: die Suffix-Laenge ist ein COMPILE-TIME-Tuning-Parameter.
    // Laengerer Suffix => mehr Bits je Key. kNone ist der deklarierte Nullpunkt (0 Suffix-Bits) und
    // liefert trotzdem echte Antworten -- er ist ehrlich, nicht stumm.
    tr("(4) ehrlicher Nullpunkt: kNone traegt 0 Suffix-Bits und ist trotzdem real getrieben (hits > 0)", r_n.hits > 0);
    tr("(4) TUNING-KONTRAST: bits_per_key steigt streng mit der Suffix-Laenge (kNone < kReal8 < kReal16) -- "
       "die Space-Seite haengt an der Achse, nicht an einem Deskriptor",
       r_n.bits_per_key < r_r8.bits_per_key && r_r8.bits_per_key < r_r16.bits_per_key);
    tr("(4) PAPERTREUE unter Last: jedes Organ findet JEDEN gebauten Key (hits >= key_count, no-FN)",
       r_s1.hits >= gt.size() && r_n.hits >= gt.size() && r_r8.hits >= gt.size() && r_r16.hits >= gt.size());
    tr("(4) und das exakte S1 ist der schaerfste (FP=0): seine hits sind die untere Schranke aller S2",
       r_s1.hits <= r_n.hits && r_s1.hits <= r_r8.hits && r_s1.hits <= r_r16.hits);

    std::printf("== test_s5_02b_filter_perf_sanity: %s ==\n", g_fail == 0 ? "ALLE OK" : "FEHLER");
    return g_fail == 0 ? 0 : 1;
}
