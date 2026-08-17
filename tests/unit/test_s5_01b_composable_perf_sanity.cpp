// A8-S5 Familie 01_read_path / Sub-Scheibe 01b (composable-Rest) -- PERF-SANITY-HARNESS.
//
// Dossier: docs/architecture/20260803-a8_f2_benchmarking_schnitt_soll_design.md Abschn. 5-S5
// ("GATE je Familie: Grundkadenz + Konformitaets-GTest der Familie + Perf-Sanity (keine 0-/Phantom-Zeiten)").
// Zweite Haelfte des GATE-MUSTERS: waehrend test_s5_01b_composable_alloc_conformance auf Typ-Ebene pinnt und
// die Verdrahtung am Objekt belegt, belegt DIESE TU, dass der Scrub die Messbarkeit nicht beschaedigt hat.
//
// WARUM DIESE WACHE NOETIG IST -- und warum sie fuer 01b SCHAERFER sein muss als fuer 01a: diese Scheibe hat
// nicht nur einen Allokator getauscht, sie hat den WALK-ALGORITHMUS dreier Trie-Familien umgestellt (von
// "alle Kinder aller Ebenen auf den Stack" auf "ein Rahmen je Ebene mit Kind-Cursor"). Ein Walk, der still
// aufhoert, Records zu liefern, macht die DEG-1-Ernte im Mess-Pfad (abi_adapter::fill_segment_timing_v3
// zieht die realen Keys ueber for_each_record) leer -- der Lauf bliebe gruen, und die per-op-Achsen T0-T3
// wuerden ab da auf einem 1-Element-Ersatzschluessel messen statt auf dem echten Bestand. Genau das faengt
// Block (2). Verlangt werden deshalb ZWEI Dinge ZUGLEICH:
//   (1) ZEIT:    die gemessene Walk-/Op-Zeit ist > 0 unter Last.
//   (2) ZAEHLER: die Zaehler DERSELBEN Schleife sind > 0 und stimmen mit dem Bestand ueberein.
// Zeit ohne Zaehler waere eine Phantom-Zeit (der Timer misst eine leere Schleife); Zaehler ohne Zeit waere
// ein toter Timer. Dazu (3) die EHRLICHEN NULLPUNKTE als Gegenprobe.
//
// FAMILIEN-SPEZIFISCHE ZAEHLER-WAHL (BEGRUENDET, nicht geraten): gemessen werden die drei Groessen, die
// diese Scheibe wirklich bewegt hat --
//   * die BESUCHSZAHL des Walks (for_each_record) je Form-A- und Form-B-Organ -- der DEG-1-Vertrag,
//   * der WALK-ALLOKATIONS-Zaehler der Form-B-Organe (Achsen-Statistik) -- der Form-B-Beleg,
//   * die MEMENTO-Groesse + sein Achsen-Zaehler -- der zweite Schalen-Puffer.
// Ueber Achsen, die diese Scheibe nicht angefasst hat, wird NICHTS behauptet -- das waere Schmuck, keine Wache.
//
// KEINE ABSOLUT-SCHWELLEN, KEINE RANGFOLGEN zwischen Organen: Laufzeit-Verhaeltnisse zwischen ART, BST und
// B-Baum sind hardware- und wachstums-abhaengig und waeren eine Flake-Quelle. Behauptet wird nur, was
// strukturell gilt (> 0, Gleichheit mit dem Bestand, Monotonie desselben Zaehlers).
//
// Standalone (plain int main, KEIN gtest).

#include <organ_axes/lookup/composable/tier_to_organ_mapping.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string_view>

namespace lkc = ::comdare::cache_engine::lookup::composable;

namespace {

int g_fail = 0;

void tr(char const* what, bool ok) {
    std::printf("  [%s] %s\n", ok ? " ok " : "FAIL", what);
    if (!ok) ++g_fail;
}

using clock_t_ = std::chrono::steady_clock;

[[nodiscard]] std::int64_t ns_between(clock_t_::time_point a, clock_t_::time_point b) noexcept {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(b - a).count();
}

[[nodiscard]] std::uint64_t spread(std::uint64_t i) noexcept {
    return (i * 2654435761ull) ^ (i << 13) ^ 0x9E3779B97F4A7C15ull;
}

/// EIN Mess-Block je Organ: fuellen (Substrat-Last) und ernten (Walk-Last), Zeit UND Zaehler aus DERSELBEN
/// Schleife. `expect_walk_alloc` unterscheidet die beiden Schnitt-Formen: Form B MUSS beim Walk ueber die
/// Achse allozieren, Form A darf es NICHT -- eine Wache, die beides durchgehen liesse, pinnte nichts.
template <class Organ>
void measure_organ(std::string_view label, std::uint64_t n_ops, bool expect_walk_alloc) {
    Organ organ{};

    auto const t0 = clock_t_::now();
    for (std::uint64_t k = 0; k < n_ops; ++k) organ.insert(spread(k), k * 7u + 1u);
    auto const t1 = clock_t_::now();

    std::uint64_t checksum = 0;
    std::size_t   visited  = 0;
    auto const    t2       = clock_t_::now();
    for (int rep = 0; rep < 4; ++rep) // vier Ernten: hebt die Walk-Zeit ueber die Uhr-Aufloesung
        visited = organ.for_each_record([&](std::uint64_t k, std::uint64_t v) { checksum += k ^ v; });
    auto const t3 = clock_t_::now();

    std::int64_t const fill_ns = ns_between(t0, t1);
    std::int64_t const walk_ns = ns_between(t2, t3);

    std::printf("  %-16.*s fill_ns %8lld  walk_ns %8lld  visited %5zu  occupied %5zu  checksum %llu\n",
                static_cast<int>(label.size()), label.data(), static_cast<long long>(fill_ns),
                static_cast<long long>(walk_ns), visited, organ.occupied_count(),
                static_cast<unsigned long long>(checksum));

    tr("(1) ZEIT: die Fuell-Schleife hat messbar Zeit gekostet (fill_ns > 0)", fill_ns > 0);
    tr("(1) ZEIT: die Walk-Schleife hat messbar Zeit gekostet (walk_ns > 0)", walk_ns > 0);
    tr("(2) ZAEHLER: der Walk hat jeden Record genau einmal besucht (visited == occupied_count)",
       visited == organ.occupied_count());
    tr("(2) ZAEHLER: der Bestand ist vollstaendig (occupied_count == n_ops)", organ.occupied_count() == n_ops);
    tr("(2) ZAEHLER: der Sink wurde wirklich gerufen (checksum != 0)", checksum != 0);

    if constexpr (requires(Organ const& o) { o.walk_allocator_statistics(); }) {
        auto const w = organ.walk_allocator_statistics();
        std::printf("  %-16.*s walk alloc_cnt %llu  bytes_in_use %llu  failures %llu\n", static_cast<int>(label.size()),
                    label.data(), static_cast<unsigned long long>(w.allocation_count),
                    static_cast<unsigned long long>(w.total_bytes_in_use),
                    static_cast<unsigned long long>(w.failure_count));
        tr("(2) FORM B: der Walk-Zaehler der Achse ist unter Last > 0", w.allocation_count > 0);
        tr("(2) FORM B: keine verdeckten Fehlschlaege", w.failure_count == 0);
        tr("(2) FORM B: diese Familie erwartet einen Walk-Zaehler", expect_walk_alloc);
    } else {
        std::printf("  %-16.*s FORM A -- kein Walk-Allokator (die staerkere Aussage)\n", static_cast<int>(label.size()),
                    label.data());
        tr("(2) FORM A: diese Familie erwartet KEINEN Walk-Zaehler", !expect_walk_alloc);
    }
}

/// (3) EHRLICHE NULLPUNKTE. Zwei Stueck, beide mit Aussage:
///   (a) das ungetriebene Organ liefert 0 Records -- und der Walk laeuft trotzdem sauber durch (kein Hang,
///       kein Ueberlauf am leeren Baum: genau der Rand, den die neue Cursor-Form neu beruehrt),
///   (b) der Memento eines leeren Organs ist leer UND sein Achsen-Zaehler ist 0 -- kein Puffer, der schon
///       beim Anlegen "vorsorglich" alloziert und damit eine Nicht-Null vortaeuscht.
template <class Organ>
void measure_zero_point(std::string_view label) {
    Organ             organ{};
    std::uint64_t     touched = 0;
    std::size_t const visited = organ.for_each_record([&](std::uint64_t, std::uint64_t) { ++touched; });
    std::printf("  %-16.*s NULLPUNKT: visited %zu  sink-Aufrufe %llu\n", static_cast<int>(label.size()), label.data(),
                visited, static_cast<unsigned long long>(touched));
    tr("(3a) leeres Organ: der Walk besucht ehrlich 0 Records (und laeuft durch)", visited == 0);
    tr("(3a) leeres Organ: der Sink wurde kein einziges Mal gerufen", touched == 0);
}

void measure_memento_zero_point() {
    lkc::SortedBinaryOrgan organ{};
    auto const             empty = organ.save_state();
    auto const             es    = empty.allocator_statistics();
    std::printf("  %-16s NULLPUNKT: memento size %zu  alloc_cnt %llu\n", "memento", empty.size(),
                static_cast<unsigned long long>(es.allocation_count));
    tr("(3b) Memento eines leeren Organs ist leer", empty.size() == 0);
    tr("(3b) und hat ehrlich NICHT alloziert (kein Vorrats-Puffer, der eine Nicht-Null vortaeuscht)",
       es.allocation_count == 0);

    for (std::uint64_t k = 0; k < 1024; ++k) organ.insert(spread(k), k * 7u + 1u);
    auto const    full = organ.save_state();
    auto const    fs   = full.allocator_statistics();
    std::uint64_t sum  = 0;
    for (auto const& kv : full) sum += kv.first ^ kv.second;
    std::printf("  %-16s LAST:      memento size %zu  alloc_cnt %llu  bytes_in_use %llu\n", "memento", full.size(),
                static_cast<unsigned long long>(fs.allocation_count),
                static_cast<unsigned long long>(fs.total_bytes_in_use));
    tr("(2) der gefuellte Memento traegt den vollen Bestand", full.size() == 1024u);
    tr("(2) und hat dafuer REAL ueber die Achse alloziert", fs.allocation_count > 0 && fs.total_bytes_in_use > 0);
    tr("(2) die Elemente sind lesbar (Bereichs-Iteration ueber (first,second) unveraendert)", sum != 0);
}

} // namespace

int main() {
    std::printf("== A8-S5 Familie 01b -- composable-Rest: Perf-Sanity (keine 0-/Phantom-Zeiten) ==\n");
    std::printf("   Gemessen wird, was DIESE Scheibe bewegt hat: Walk-Vollstaendigkeit, Walk-Allokation,\n");
    std::printf("   Memento-Puffer. Keine Absolut-Schwellen, keine Rangfolgen zwischen Organen.\n");

    std::printf("-- FORM A (heap-freier Walk, CT-Kappe aus dem Schluesseltyp) --\n");
    measure_organ<lkc::ArtTrieOrgan>("art_trie", 2048, /*expect_walk_alloc=*/false);
    measure_organ<lkc::HotPatriciaOrgan>("hot_patricia", 2048, false);
    measure_organ<lkc::StartTrieOrgan>("start_trie", 2048, false);

    std::printf("-- FORM B (Walk-Stack ueber die Allokator-Achse) --\n");
    measure_organ<lkc::BstTreeOrgan>("bst_tree", 2048, /*expect_walk_alloc=*/true);
    measure_organ<lkc::BTreeSearchOrgan>("btree", 2048, true);

    std::printf("-- (3) EHRLICHE NULLPUNKTE --\n");
    measure_zero_point<lkc::ArtTrieOrgan>("art_trie");
    measure_zero_point<lkc::HotPatriciaOrgan>("hot_patricia");
    measure_zero_point<lkc::StartTrieOrgan>("start_trie");
    measure_zero_point<lkc::BstTreeOrgan>("bst_tree");
    measure_zero_point<lkc::BTreeSearchOrgan>("btree");
    measure_memento_zero_point();

    std::printf("== test_s5_01b_composable_perf_sanity: %s ==\n", g_fail == 0 ? "ALLE OK" : "FEHLER");
    return g_fail == 0 ? 0 : 1;
}
