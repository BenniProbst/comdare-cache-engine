// test_progress_delta.cpp -- Welle 5 (E-W5-2, §38-Fortschritts-Rueck-Kanal, W5-C, 2026-07-19). Gate fuer den
// builder-Leaf builder/experiment_tree/progress_delta.hpp (nach der Layering-Verlagerung: die Up-Channel-PODs +
// Delta-Logik liegen in der BUILDER-Schicht, GLEICHER Namespace wie der Iterator, KEIN Aufwaerts-Include).
//
// Die Delta-Erzeugung ist als freistehende, iterator-/view-UNABHAENGIGE Funktion (compute_progress_deltas)
// geschnitten -> hier direkt getestet (KEIN DLL-Bau, keine Compiler-Aufrufe -- exakt der vom Bauplan vorgesehene
// Schnitt). Der Test speist die mixed-radix-Ziffernfolge der REALEN StaticBinaryView (view.variant_tuple, die EINE
// Single-Source, die auch der Iterator feuert) und prueft: (a) Rekonstruktion aus Voll-Erstmeldung+Deltas == die
// von der StaticBinaryView gelieferte Folge (binary_id-genau), (b) done GENAU 1x am Fensterende, (c) erste Meldung =
// Voll-Konfiguration, (d) minimale Deltas. Geprueft ueber ein volles Fenster UND ein Teil-Fenster.
//
// Bewusst leicht: nur der builder-Leaf progress_delta.hpp + experiment_tree.hpp (StaticBinaryView) -- KEIN
// experiment_dock_payload.hpp (planner), KEIN xml_reader. Plain-main (check_*; exit 0 = alle OK).

#include <builder/experiment_tree/progress_delta.hpp> // ProgressDelta / progress_delta_between / compute_progress_deltas / reconstruct_configs
#include <builder/experiment_tree/experiment_tree.hpp> // ExperimentTree / StaticBinaryView / AxisLevel / variant_tuple

#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace ex = ::comdare::cache_engine::builder::experiment;

namespace {

int g_fail = 0;

void check(char const* what, bool ok) {
    std::cout << "  [" << (ok ? " ok " : "FAIL") << "] " << what << '\n';
    if (!ok) ++g_fail;
}

template <class A, class B>
void check_eq(char const* what, A const& got, B const& want) {
    bool const ok = (got == want);
    std::cout << "  [" << (ok ? " ok " : "FAIL") << "] " << what << " = " << got;
    if (!ok) std::cout << "  (erwartet: " << want << ")";
    std::cout << '\n';
    if (!ok) ++g_fail;
}

// binary_id aus einer mixed-radix-Ziffernfolge rekonstruieren (Spiegel StaticBinaryView::operator[], alle Ebenen
// nicht-leer): so wird die aus den Deltas rekonstruierte Konfig-Folge binary_id-genau gegen view[i] geprueft.
[[nodiscard]] std::string id_from_tuple(std::vector<ex::AxisLevel> const& levels, std::vector<std::size_t> const& t) {
    std::string bin;
    for (std::size_t d = 0; d < levels.size(); ++d) {
        std::string seg = levels[d].axis + "=" + levels[d].values[t[d]];
        if (!bin.empty()) bin += '/';
        bin += seg;
    }
    return bin;
}

void delta_over_window(ex::StaticBinaryView const& view, std::vector<ex::AxisLevel> const& levels, std::size_t start,
                       std::size_t count, char const* tag) {
    std::cout << "== Delta-Kanal ueber Fenster [" << start << "," << (start + count) << ") -- " << tag << " ==\n";

    // Konfig-Folge = die mixed-radix-Ziffern der REALEN StaticBinaryView (view.variant_tuple = die EINE Single-Source,
    // die auch der Iterator je Binary feuert). Genau die Folge, die der §38-Rueck-Kanal rekonstruieren koennen MUSS.
    std::vector<std::vector<std::size_t>> configs;
    configs.reserve(count);
    for (std::size_t j = 0; j < count; ++j) configs.push_back(view.variant_tuple(start + j));

    std::vector<ex::ProgressDelta> const deltas = ex::compute_progress_deltas(configs);

    // done GENAU 1x, am Ende, cursor == Fenster-Groesse.
    std::size_t done_count = 0;
    for (auto const& d : deltas)
        if (d.done) ++done_count;
    check_eq("(a) done genau 1x", done_count, std::size_t{1});
    check("(a) done ist das LETZTE Delta", !deltas.empty() && deltas.back().done);
    check("(a) done-Delta traegt keine Achsen-Aenderung", !deltas.empty() && deltas.back().changed.empty());
    check_eq("(a) done-Delta cursor == Fenster-Groesse", deltas.empty() ? std::size_t{999} : deltas.back().cursor,
             count);
    check_eq("(a) Delta-Zahl == Fenster + 1 (Terminal)", deltas.size(), count + 1);

    // Erste Meldung = Voll-Konfiguration (alle Ebenen gelistet, cursor 0, nicht done).
    check("(b) erste Meldung cursor 0 + nicht done",
          !deltas.empty() && deltas.front().cursor == 0 && !deltas.front().done);
    check_eq("(b) erste Meldung listet ALLE Achsen (Voll-Konfiguration)",
             deltas.empty() ? std::size_t{0} : deltas.front().changed.size(), levels.size());

    // Rekonstruktion aus Voll-Erstmeldung + Deltas == die StaticBinaryView-Folge (binary_id-genau).
    std::vector<std::vector<std::size_t>> const recon = ex::reconstruct_configs(deltas, levels.size());
    check_eq("(c) Rekonstruktions-Laenge == Fenster", recon.size(), count);
    bool tuple_match = (recon == configs);
    bool id_match    = (recon.size() == count);
    for (std::size_t j = 0; j < count && j < recon.size(); ++j)
        if (id_from_tuple(levels, recon[j]) != view[start + j].binary_id) id_match = false;
    check("(c) rekonstruierte Ziffern-Folge == view.variant_tuple-Folge", tuple_match);
    check("(c) rekonstruierte Folge == StaticBinaryView-binary_id-Folge (Kern-Gate)", id_match);

    // Minimalitaet: fuer ein KONTIGUIERLICHES Fenster wechselt jeder Schritt >=1 Achse (die niederwertigste kippt
    // immer); NIE mehr Achsen als es Ebenen gibt. (Der Kern-Beweis ist die Rekonstruktions-Gleichheit oben.)
    bool minimal_ok = true;
    for (std::size_t k = 1; k < deltas.size(); ++k) {
        if (deltas[k].done) continue;
        if (deltas[k].changed.empty() || deltas[k].changed.size() > levels.size()) minimal_ok = false;
    }
    check("(d) Folge-Deltas: 1..|Ebenen| Achsen (mixed-radix-minimal, nie leer, nie ueber-voll)", minimal_ok);
}

void primitive_edge_cases() {
    std::cout << "== progress_delta_between: Primitive-Randfaelle ==\n";
    // prev leer => Voll-Konfiguration (alle Achsen).
    ex::ProgressDelta const full = ex::progress_delta_between({}, {3, 0, 2}, 0);
    check_eq("(e) prev leer -> alle Achsen gelistet", full.changed.size(), std::size_t{3});
    check("(e) prev leer -> cursor 0 + nicht done", full.cursor == 0 && !full.done);
    // prev == cur => KEINE Aenderung (leerer Delta) -- konsistent (kein Phantom-Wechsel).
    ex::ProgressDelta const same = ex::progress_delta_between({3, 0, 2}, {3, 0, 2}, 7);
    check("(e) prev == cur -> leerer changed-Delta (cursor erhalten)", same.changed.empty() && same.cursor == 7);
    // genau EINE Achse geaendert.
    ex::ProgressDelta const one = ex::progress_delta_between({3, 0, 2}, {3, 1, 2}, 8);
    check("(e) genau EINE Achse geaendert -> 1 Eintrag (axis_index 1)",
          one.changed.size() == 1 && one.changed[0].axis_index == 1 && one.changed[0].variant_index == 1);
}

} // namespace

int main() {
    std::cout << "==== W5-C progress_delta Gate (§38 hinauf: Delta-Rekonstruktion) ====\n";

    // Ein 3-Achsen-Baum mit unterschiedlichen Radizes (2 x 3 x 4 = 24) -> nicht-triviale mixed-radix-Deltas.
    std::vector<ex::AxisLevel> levels;
    levels.push_back(ex::AxisLevel{"axis_a", {"a0", "a1"}, /*is_static=*/true, "", ""});
    levels.push_back(ex::AxisLevel{"axis_b", {"b0", "b1", "b2"}, /*is_static=*/true, "", ""});
    levels.push_back(ex::AxisLevel{"axis_c", {"c0", "c1", "c2", "c3"}, /*is_static=*/true, "", ""});
    auto               factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree tree{factory};
    tree.build(levels);
    ex::StaticBinaryView const view = tree.static_binary_view();
    check_eq("(setup) view.size() == 2*3*4", view.size(), std::size_t{24});
    check_eq("(setup) level_count() == 3", view.level_count(), std::size_t{3});

    delta_over_window(view, levels, /*start=*/0, /*count=*/24, "volles Fenster");
    delta_over_window(view, levels, /*start=*/5, /*count=*/8, "Teil-Fenster (golden-N-Chunk-Analog)");
    primitive_edge_cases();

    std::cout << (g_fail == 0 ? "\nPROGRESS_DELTA_OK\n" : "\nPROGRESS_DELTA_FAIL\n");
    return g_fail == 0 ? 0 : 1;
}
