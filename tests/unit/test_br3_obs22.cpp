// test_br3_obs22 — BR-3-OBS-22 (2026-06-02, Doc 27 §0.1/§3) — 22-Observer-Differenzierung, „kein Wegschrumpfen".
//
// Belegt gegen die ECHTEN 22-Achsen-Bindung (BR-1 build_all_axis_levels): JEDE der 22 Achsen ist
// observer-klassifiziert (gattungs-korrekt) UND trägt ihre read-only Definition (reflect_names = reale
// Wrapper-Namen) — keine Achse fällt weg. 17 SearchAlgorithmObserver + 3 DefinitionOnly (page_type/09b/12) +
// 2 ContainerObserver (q1/q2) = 22.
//
// Build: cl /std:c++latest /EHsc /I<…> /I<build/generated…> (Voll-22-Include — RAM-Watchdog-gewahr, wie test_br1_full22)

#include "builder/experiment_tree/registry_to_axis_levels.hpp"      // build_all_axis_levels() (22 Achsen)
#include "builder/experiment_tree/axis_observer_classification.hpp" // AxisObserverKind + observer_kind_of

#include <iostream>
#include <string>

namespace ex = comdare::cache_engine::builder::experiment;

static int g_fail = 0;
template <typename A, typename B>
void check_eq(char const* what, A const& got, B const& want) {
    bool ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) {
        std::cout << "  (erwartet: " << want << ")";
        ++g_fail;
    }
    std::cout << "\n";
}
void check_true(char const* what, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!c) ++g_fail;
}

int main() {
    std::cout << "BR-3-OBS-22: 22-Observer-Differenzierung (kein Wegschrumpfen):\n";

    std::vector<ex::AxisLevel> lv = ex::build_all_axis_levels(); // BR-1: alle 22 Achsen, registry-getrieben
    check_eq("BR-1 liefert 22 Achsen", lv.size(), std::size_t{22});

    std::size_t classified = 0, with_def = 0, sa = 0, def_only = 0, cont = 0;
    for (auto const& l : lv) {
        ex::AxisObserverKind k{};
        bool const           found = ex::observer_kind_of(l.axis, k);
        if (found) {
            ++classified;
            if (k == ex::AxisObserverKind::SearchAlgorithmObserver)
                ++sa;
            else if (k == ex::AxisObserverKind::DefinitionOnly)
                ++def_only;
            else
                ++cont;
        }
        if (!l.values.empty()) ++with_def; // read-only Definition (reale Wrapper-Namen) vorhanden
        std::cout << "    " << (found ? "" : "[UNKLASSIFIZIERT] ") << l.axis << " : "
                  << (found ? std::string{ex::observer_kind_name(k)} : std::string{"?"}) << "  (" << l.values.size()
                  << " Wrapper)\n";
    }

    check_eq("JEDE der 22 Achsen ist observer-klassifiziert (kein Wegschrumpfen)", classified, std::size_t{22});
    check_eq("JEDE der 22 Achsen trägt eine read-only Definition (values>0)", with_def, std::size_t{22});
    // korr. 2026-06-03 (Doc 30 §8.0): queuing q1/q2 sind SA-Tier-Unterklasse-Achsen (Slots T17/T18) → 19/3/0=22,
    // KEINE Container-Gattung. ContainerObserver ist reserviert für die echte Container-Gattung (#87) → aktuell 0.
    check_eq("19 SearchAlgorithmObserver (ObserverAggregate<19>, inkl. queuing q1/q2)", sa, std::size_t{19});
    check_eq("3 DefinitionOnly (page_type/09b/12 = Build-Konstanten)", def_only, std::size_t{3});
    check_eq("0 ContainerObserver (queuing→SA; ContainerObserver reserviert für echte Container-Gattung #87)", cont,
             std::size_t{0});
    // Konsistenz der constexpr-Klassifikation selbst:
    check_eq("constexpr: 19 SearchAlgorithmObserver",
             ex::count_observer_kind(ex::AxisObserverKind::SearchAlgorithmObserver), std::size_t{19});
    check_eq("constexpr: 3 DefinitionOnly", ex::count_observer_kind(ex::AxisObserverKind::DefinitionOnly),
             std::size_t{3});
    check_eq("constexpr: 0 ContainerObserver", ex::count_observer_kind(ex::AxisObserverKind::ContainerObserver),
             std::size_t{0});
    check_true("Summe == 22 (alle Achsen abgedeckt, keine doppelt/fehlend)", sa + def_only + cont == 22);

    std::cout << "\n==== BR-3-OBS-22 (22 Observer differenziert): "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
