// A8-S5 Familie 05_write_path_io / RESTSTRECKE queuing -- PERF-SANITY-HARNESS (T15/T16).
//
// Dossier: docs/architecture/20260803-a8_f2_benchmarking_schnitt_soll_design.md Abschn. 5-S5
// ("GATE je Familie: Grundkadenz + Konformitaets-GTest der Familie + Perf-Sanity (keine 0-/Phantom-Zeiten)").
// Zweite Haelfte des GATE-MUSTERS: waehrend test_s5_05q_queuing_alloc_conformance auf TYP-Ebene pinnt,
// dass kein Familien-Organ am Allokator-Achsen-Interface vorbei alloziert (und dass das Universum aus der
// Einzigquelle kommt), belegt DIESE TU zur LAUFZEIT, dass der Q1-Schnitt die Messbarkeit der Achse nicht
// beschaedigt hat.
//
// WARUM DIESE WACHE GERADE HIER NOETIG IST -- und zwar mehr als bei den Geschwister-Familien:
// die Q1-Organe haben in dieser Welle ihren KOMPLETTEN Speicher-Unterbau gewechselt (std::allocator ->
// StdAllocatorAdapter der Allokator-Achse; bei den zwei lock-freien Organen zusaetzlich make_unique ->
// AxisCellArray). Ein solcher Wechsel kann eine Achse still stumm schalten: der Bau bleibt gruen, die
// Tests bleiben gruen, und die Mess-Spalte steht ab da auf 0. Genau diese Klasse war der A8-S1-/A8-S3-
// Befund (stiller Messwert-Verlust bzw. strukturell-0). Die Wache verlangt deshalb ZWEI Dinge ZUGLEICH:
//   (1) ZEIT:    seg_ns[T15]/seg_ns[T16] > 0 unter Last.
//   (2) ZAEHLER: die Observer-Zaehler DERSELBEN Op-Schleife > 0.
// Zeit ohne Zaehler waere eine Phantom-Zeit (der Timer misst eine leere Schleife); Zaehler ohne Zeit waere
// ein toter Timer. Nur beides zusammen heisst "die Achse wird real durchgemessen".
// Dazu (3) der EHRLICHE NULLPUNKT als Gegenprobe (s.u.).
//
// ===================================================================================================
// WARUM GENAU DIESE SPALTEN -- die Begruendung der Zaehler-Wahl (nicht irgendein ">0")
// ===================================================================================================
// Schema-Quelle ist kV3AxisSchema (observable_tier.hpp), Schreib-Reihenfolge fill_observer_v3:
//     T15 queuing_q1 = {0 put, 1 get, 2 overflow, 3 underflow, 4 peak_size}
//     T16 queuing_q2 = {0 decisions, 1 full_flush, 2 partial_flush, 3 no_flush, 4 flush_complete}
//
//   * T15 Feld 4 (peak_size) IST die Spalte dieser Welle, und deshalb traegt sie hier die Hauptlast.
//     put/get belegen nur, DASS das Organ gerufen wurde -- das taete auch ein Organ, das gar nichts
//     haelt. peak_size ist der einzige T15-Zaehler, der von GEHALTENEM Zustand spricht, und gehaltener
//     Zustand ist genau das, was in dieser Welle an die Allokator-Achse gewandert ist. Eine Wache, die
//     nur put>0 pruefte, waere nach einem Schnitt, der den Puffer versehentlich leer laufen laesst,
//     immer noch gruen.
//   * T15 Feld 3 (underflow) traegt den Nullpunkt: NoBuffer::get() liefert per Deklaration immer
//     nullopt und zaehlt jedes Mal underflow. Der Nullpunkt ist damit nicht "zufaellig 0", sondern eine
//     Aussage, die das Organ selbst macht -- und sie ist an einem ANDEREN Feld sichtbar als die 0.
//   * T16 Feld 1/3 (full_flush vs. no_flush) sind ein DISKRIMINIERENDES Paar: EagerFlush entscheidet
//     ausschliesslich FullFlush, LazyFlush ausschliesslich NoFlush. Die beiden Spalten tauschen also die
//     Rollen, wenn man die Achsen-Wahl tauscht. Das ist die staerkere Aussage als ein einseitiges ">0":
//     eine Wache, die das Framework statt der Achse misst, koennte sie nicht erfuellen.
//
// Q2 ist in dieser Welle NICHT geschnitten worden (die 5 Flush-Policies tragen die MINIMAL-Form, sie
// halten ueberhaupt keinen dynamischen Zustand -- in der Konformitaets-TU einzeln gepinnt). Die T16-
// Spalten stehen hier trotzdem, weil die Familie 05 als GANZES nachgewiesen wird: waere Q2 durch den
// Q1-Umbau still verstummt (beide Organe haengen im selben Treiber-Zweig, abi_adapter tier_insert), saehe
// man es nur an T16.
//
// ACHSEN-INDIZES werden aus kCompositionAxisNames ABGELEITET (Name -> Position), nie als Literal
// geschrieben: verschiebt sich die Achsen-Ordnung, wandert die Wache mit, statt still die falsche Spalte
// zu pruefen. Die Namensliste selbst wird nur GELESEN (golden-Tabu).
//
// Standalone (plain int main, KEIN gtest), COMDARE_MEASUREMENT_ON/-STATISTICS kommen global aus dem
// Haupt-CMakeLists (COMDARE_MEASUREMENT_MODE=ON) -- konsistent mit den uebrigen S5-Wachen.

#include <anatomy/abi_adapter.hpp>
#include <anatomy/composition_factory.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>

#include <builder/experiment_tree/axis_path_serialization.hpp> // kCompositionAxisNames (NUR gelesen)
#include <compositions/hot_reference.hpp>
#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_array256.hpp>

#include <axes/persistence_target/axis_persistence_target_memory_only.hpp>
#include <topics/queuing/axis_q1_queuing/axis_q1_queuing_fifo.hpp>
#include <topics/queuing/axis_q1_queuing/axis_q1_queuing_no_buffer.hpp>
#include <topics/queuing/axis_q2_queuing/axis_q2_queuing_eager.hpp>
#include <topics/queuing/axis_q2_queuing/axis_q2_queuing_lazy.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string_view>

namespace an    = ::comdare::cache_engine::anatomy;
namespace comp  = ::comdare::cache_engine::compositions;
namespace ce03a = ::comdare::cache_engine::traversal::axis_03a_search_algo;
namespace ex    = ::comdare::cache_engine::builder::experiment;
namespace q1    = ::comdare::cache_engine::queuing::axis_q1_queuing;
namespace q2    = ::comdare::cache_engine::queuing::axis_q2_queuing;

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

// -- Familien-Definition: die beiden queuing-Achsen der Familie 05 ---------------------------------
constexpr std::string_view kFamilyAxes[] = {"queuing_q1", "queuing_q2"};

constexpr std::size_t kAxisQ1 = axis_index_of("queuing_q1");
constexpr std::size_t kAxisQ2 = axis_index_of("queuing_q2");

static_assert(kAxisQ1 < an::kV3AxisCount,
              "S5-05q Perf-Sanity: Achse 'queuing_q1' steht nicht mehr in kCompositionAxisNames -- die Wache "
              "wuerde eine falsche seg_ns-Spalte pruefen.");
static_assert(kAxisQ2 < an::kV3AxisCount,
              "S5-05q Perf-Sanity: Achse 'queuing_q2' steht nicht mehr in kCompositionAxisNames -- die Wache "
              "wuerde eine falsche seg_ns-Spalte pruefen.");
static_assert(ex::kCompositionAxisNames.size() == an::kV3AxisCount,
              "S5-05q Perf-Sanity: Achsen-Namensliste und Snapshot-Achsenzahl sind auseinandergelaufen.");

// Die Feld-Indizes werden NICHT als nackte Zahlen gestreut, sondern hier EINMAL benannt und gegen die
// Schema-Tabelle geprueft. Dreht jemand die Schreib-Reihenfolge in fill_observer_v3, faellt der
// static_assert -- statt dass die Wache still die Nachbarspalte liest.
constexpr std::size_t kQ1Put       = 0;
constexpr std::size_t kQ1Get       = 1;
constexpr std::size_t kQ1Underflow = 3;
constexpr std::size_t kQ1PeakSize  = 4;
constexpr std::size_t kQ2Decisions = 0;
constexpr std::size_t kQ2FullFlush = 1;
constexpr std::size_t kQ2NoFlush   = 3;
constexpr std::size_t kQ2Complete  = 4;

[[nodiscard]] constexpr bool feld_heisst(std::size_t axis, std::size_t field, std::string_view name) noexcept {
    char const* n = an::kV3AxisSchema[axis].names[field];
    return n != nullptr && std::string_view{n} == name;
}
static_assert(feld_heisst(kAxisQ1, kQ1Put, "put") && feld_heisst(kAxisQ1, kQ1Get, "get") &&
                  feld_heisst(kAxisQ1, kQ1Underflow, "underflow") && feld_heisst(kAxisQ1, kQ1PeakSize, "peak_size"),
              "S5-05q Perf-Sanity: die T15-Feld-Belegung von kV3AxisSchema hat sich gedreht -- die Wache laese "
              "die falschen Spalten (put/get/underflow/peak_size).");
static_assert(feld_heisst(kAxisQ2, kQ2Decisions, "decisions") && feld_heisst(kAxisQ2, kQ2FullFlush, "full_flush") &&
                  feld_heisst(kAxisQ2, kQ2NoFlush, "no_flush") && feld_heisst(kAxisQ2, kQ2Complete, "flush_complete"),
              "S5-05q Perf-Sanity: die T16-Feld-Belegung von kV3AxisSchema hat sich gedreht -- die Wache laese "
              "die falschen Spalten (decisions/full_flush/no_flush/flush_complete).");

// -- Store-getragene Komposition: Array256 (Byte-Domaene) + Hot-Backing, queuing_q1/q2 variabel.
//    Nur ueber ein REAL befuelltes Store-Backing entsteht ein echter Descent -- eine Huellen-Komposition
//    wuerde den Treiber blockieren und die Wache um ihre Aussage bringen (04-Pilot-Befund).
template <class Q1Strategy, class Q2Policy>
using Family05Composition = an::AdHocComposition<
    ce03a::Array256SearchAlgo, comp::HotComposition::cache_traversal, comp::HotComposition::mapping,
    comp::HotComposition::path_compression, comp::HotComposition::node_type, comp::HotComposition::memory_layout,
    comp::HotComposition::allocator, comp::HotComposition::prefetch, comp::HotComposition::concurrency,
    comp::HotComposition::serialization, comp::HotComposition::value_handle, comp::HotComposition::index_organization,
    comp::HotComposition::io_dispatch, comp::HotComposition::migration_policy, comp::HotComposition::filter, Q1Strategy,
    Q2Policy, ::comdare::cache_engine::persistence_target::MemoryOnlyTarget>;

[[nodiscard]] std::uint64_t spread_key(std::uint64_t i) noexcept {
    return (i * 2654435761ull) ^ (i << 17) ^ 0x9E3779B97F4A7C15ull;
}

/// Treibt ein echtes Tier unter Last (insert + lookup) und zieht EINEN Observer-Snapshot.
/// Die Q1-/Q2-Organe haengen AUTO-gekoppelt im selben Treiber-Zweig (abi_adapter tier_insert put +
/// should_flush/on_flush_complete, tier_lookup get) -- Zeit und Zaehler stammen deshalb aus DERSELBEN
/// Op-Schleife, was die Anti-Phantom-Aussage ueberhaupt erst tragfaehig macht.
template <class Q1Strategy, class Q2Policy>
[[nodiscard]] an::ComdareTierObserverSnapshot drive_and_observe(std::uint64_t n_ops) {
    using Anatomy = an::SearchAlgorithmAnatomy<Family05Composition<Q1Strategy, Q2Policy>>;
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
    for (std::uint64_t i = 0; i < n_ops; ++i) (void)drv->tier_insert(spread_key(i) & 0xFFull, i * 11u + 5u);
    std::uint64_t v = 0;
    for (std::uint64_t i = 0; i < n_ops; ++i) (void)drv->tier_lookup(spread_key(i) & 0xFFull, &v);
    obs->tier_observe(&snap);
    return snap;
}

} // namespace

int main() {
    std::printf("== A8-S5 Familie 05_write_path_io / queuing -- Perf-Sanity T15/T16 (keine 0-/Phantom-Zeiten) ==\n");
    constexpr std::uint64_t kLoad = 2000;

    // Familien-Besetzung unter Last: ein Q1-Organ, das WIRKLICH Zustand haelt (und ihn seit dieser Welle
    // ueber die Allokator-Achse haelt) + eine Q2-Policy, die WIRKLICH flusht.
    auto const fifo_eager = drive_and_observe<q1::FIFOQueueBuffer, q2::EagerFlush>(kLoad);
    // Gegenprobe: der deklarierte Nullpunkt BEIDER Achsen (NoBuffer haelt nie etwas, LazyFlush flusht nie).
    auto const none_lazy = drive_and_observe<q1::NoBuffer, q2::LazyFlush>(kLoad);

    std::printf("-- (1) ZEIT: seg_ns der Familien-Achsen unter Last (n_ops=%llu je Phase) --\n",
                static_cast<unsigned long long>(kLoad));
    for (auto const axis : kFamilyAxes) {
        std::size_t const  idx = axis_index_of(axis);
        std::int64_t const ns  = fifo_eager.seg_ns[idx];
        std::printf("     seg_ns[T%zu %-12.*s] = %lld ns\n", idx, static_cast<int>(axis.size()), axis.data(),
                    static_cast<long long>(ns));
    }
    std::printf("     seg_run_total_ns = %lld ns   batches_measured = %llu\n",
                static_cast<long long>(fifo_eager.seg_run_total_ns),
                static_cast<unsigned long long>(fifo_eager.batches_measured));

    tr("(1) der Segment-Lauf hat ueberhaupt stattgefunden (batches_measured > 0)", fifo_eager.batches_measured > 0);
    tr("(1) seg_ns[T15 queuing_q1] > 0 unter Last (keine 0-Zeit)", fifo_eager.seg_ns[kAxisQ1] > 0);
    tr("(1) seg_ns[T16 queuing_q2] > 0 unter Last (keine 0-Zeit)", fifo_eager.seg_ns[kAxisQ2] > 0);
    tr("(1) Kommensurabel: Summe der Familien-Segmente <= seg_run_total_ns (kein Zeit-Ueberlauf)",
       fifo_eager.seg_ns[kAxisQ1] + fifo_eager.seg_ns[kAxisQ2] <= fifo_eager.seg_run_total_ns);

    // -- (2) die Zaehler DERSELBEN Op-Schleife (Anti-Phantom) ---------------------------------------
    std::uint64_t const q1_put  = fifo_eager.axis_stats[kAxisQ1][kQ1Put];
    std::uint64_t const q1_get  = fifo_eager.axis_stats[kAxisQ1][kQ1Get];
    std::uint64_t const q1_peak = fifo_eager.axis_stats[kAxisQ1][kQ1PeakSize];
    std::uint64_t const q2_dec  = fifo_eager.axis_stats[kAxisQ2][kQ2Decisions];
    std::uint64_t const q2_full = fifo_eager.axis_stats[kAxisQ2][kQ2FullFlush];
    std::uint64_t const q2_done = fifo_eager.axis_stats[kAxisQ2][kQ2Complete];
    std::printf("-- (2) ZAEHLER derselben Op-Schleife (Anti-Phantom) --\n");
    std::printf("     T15 put = %llu   get = %llu   peak_size = %llu\n", static_cast<unsigned long long>(q1_put),
                static_cast<unsigned long long>(q1_get), static_cast<unsigned long long>(q1_peak));
    std::printf("     T16 decisions = %llu   full_flush = %llu   flush_complete = %llu\n",
                static_cast<unsigned long long>(q2_dec), static_cast<unsigned long long>(q2_full),
                static_cast<unsigned long long>(q2_done));
    tr("(2) T15: die gemessene Zeit ist durch reale Puffer-Ops gedeckt (put > 0 UND get > 0)",
       q1_put > 0 && q1_get > 0);
    tr("(2) T15: der Puffer hat WIRKLICH Zustand gehalten (peak_size > 0) -- die Spalte dieser Welle; "
       "ein Organ, dessen Speicher der Schnitt leer laufen liesse, faellt genau hier",
       q1_peak > 0);
    tr("(2) T16: die gemessene Zeit ist durch reale Flush-Entscheidungen gedeckt (decisions > 0, "
       "flush_complete > 0)",
       q2_dec > 0 && q2_done > 0);
    tr("(2) T16: EagerFlush entscheidet ausschliesslich FullFlush (full_flush == decisions)", q2_full == q2_dec);

    // -- (3) der ehrliche Nullpunkt als Gegenprobe -- BEIDE Achsen ----------------------------------
    std::uint64_t const n_peak      = none_lazy.axis_stats[kAxisQ1][kQ1PeakSize];
    std::uint64_t const n_underflow = none_lazy.axis_stats[kAxisQ1][kQ1Underflow];
    std::uint64_t const n_get       = none_lazy.axis_stats[kAxisQ1][kQ1Get];
    std::uint64_t const n_full      = none_lazy.axis_stats[kAxisQ2][kQ2FullFlush];
    std::uint64_t const n_no        = none_lazy.axis_stats[kAxisQ2][kQ2NoFlush];
    std::uint64_t const n_dec       = none_lazy.axis_stats[kAxisQ2][kQ2Decisions];
    std::printf("-- (3) ehrlicher Nullpunkt (Gegenprobe NoBuffer + LazyFlush) --\n");
    std::printf("     T15 NoBuffer: peak_size = %llu   underflow = %llu   get = %llu   (FIFO peak_size: %llu)\n",
                static_cast<unsigned long long>(n_peak), static_cast<unsigned long long>(n_underflow),
                static_cast<unsigned long long>(n_get), static_cast<unsigned long long>(q1_peak));
    std::printf("     T16 LazyFlush: no_flush = %llu   full_flush = %llu   decisions = %llu   (Eager full: %llu)\n",
                static_cast<unsigned long long>(n_no), static_cast<unsigned long long>(n_full),
                static_cast<unsigned long long>(n_dec), static_cast<unsigned long long>(q2_full));

    tr("(3) T15: NoBuffer meldet peak_size == 0 (deklarierter Nullpunkt: das Organ haelt nie Zustand)", n_peak == 0);
    // AM OBJEKT ERHOBEN (die Erst-Fassung dieser Zeile pruefte underflow == get und FIEL): total_get_count
    // zaehlt nur ERFOLGREICHE get(), nicht die Aufrufe. NoBuffer liefert per Deklaration immer nullopt --
    // seine get-Spalte ist deshalb ehrlich 0, und der Betrieb steht vollstaendig in underflow. Genau diese
    // Spalten-Aufteilung ist die staerkere Aussage: die 0 in peak_size UND get ist kein Totalausfall,
    // sondern zweimal dieselbe deklarierte Eigenschaft, waehrend underflow den Lauf belegt.
    tr("(3) T15: die 0 ist KEIN Totalausfall -- dasselbe Organ belegt den Lauf an der Spalte, die zu seiner "
       "Deklaration passt (underflow > 0 bei get == 0: jedes get() lief, keines lieferte je ein Element)",
       n_underflow > 0 && n_get == 0);
    tr("(3) T15-Kontrast an derselben Spalte: FIFO liefert real Elemente aus (get > 0), NoBuffer nie "
       "(get == 0) -- die beiden Nullpunkte sind unterscheidbar, nicht bloss beide 0",
       q1_get > 0 && n_get == 0);
    tr("(3) T15-Kontrast: FIFO haelt Zustand, NoBuffer nicht (peak_size FIFO > NoBuffer) -- die Zeit haengt "
       "an der Achse, nicht am Framework",
       q1_peak > n_peak);
    tr("(3) T16: LazyFlush entscheidet ausschliesslich NoFlush (no_flush == decisions > 0, full_flush == 0)",
       n_dec > 0 && n_no == n_dec && n_full == 0);
    tr("(3) T16-Kontrast: die Spalten full_flush/no_flush TAUSCHEN mit der Achsen-Wahl -- eine Wache, die "
       "das Framework statt der Achse saehe, koennte das nicht erfuellen",
       q2_full > 0 && n_full == 0 && n_no > 0 && fifo_eager.axis_stats[kAxisQ2][kQ2NoFlush] == 0);

    std::printf("== test_s5_05q_queuing_perf_sanity: %s ==\n", g_fail == 0 ? "ALLE OK" : "FEHLER");
    return g_fail == 0 ? 0 : 1;
}
