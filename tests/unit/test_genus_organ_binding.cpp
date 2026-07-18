// test_genus_organ_binding -- INC-1b (2026-07-18): die Organ-Achsen definieren typisiert + drift-guarded, welche
// Gattung welche Organ-Achsen verwendet/braucht (genus_organ_binding.hpp). Der eigentliche Beweis sind die
// static_asserts IM HEADER (feuern beim Kompilieren dieser TU); die Checks hier spiegeln sie zur Laufzeit + belegen
// die Organ-Universum-Trennung (System-Achsen sind KEINE Organ-Achsen). Custom-Main-Stil (wie test_genus_binding,
// kein gtest im foreach-Block). Rein lesend, keine #156-Messdaten, kein DLL.

#include "builder/experiment_tree/genus_organ_binding.hpp"

#include <iostream>
#include <string_view>

namespace {

namespace ex  = ::comdare::cache_engine::builder::experiment;
namespace cea = ::comdare::cache_engine::anatomy;

int  g_fail = 0;
void check(bool c, std::string_view what) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!c) ++g_fail;
}

} // namespace

int main() {
    std::cout << "INC-1b: Organ-Achsen definieren die Gattungen (typisiert + drift-guarded):\n";

    // (a) alle 5 gebundenen Gattungen konsistent (Slot-Zahl == axis_names, rein-Organ, dublettenfrei).
    check(ex::genus_organ_binding_consistent<cea::AnatomyGenus::SearchAlgorithm>(),
          "SearchAlgorithm-Bindung konsistent");
    check(ex::genus_organ_binding_consistent<cea::AnatomyGenus::Adapter>(), "Adapter-Bindung konsistent");
    check(ex::genus_organ_binding_consistent<cea::AnatomyGenus::Set>(), "Set-Bindung konsistent");
    check(ex::genus_organ_binding_consistent<cea::AnatomyGenus::Sequence>(), "Sequence-Bindung konsistent");
    check(ex::genus_organ_binding_consistent<cea::AnatomyGenus::View>(), "View-Bindung konsistent");

    // (b) Organ-Universum: Komposition-Achsen + genus-Erweiterungen = Organ; System-Achsen = NICHT Organ.
    check(ex::is_organ_axis_label("search_algo"), "search_algo ist Organ (Komposition T0)");
    check(ex::is_organ_axis_label("isa"), "isa ist Organ (T11, jetzt unterm Dach)");
    check(ex::is_organ_axis_label("inner_container"), "inner_container ist Organ-Erweiterung (Adapter)");
    check(ex::is_organ_axis_label("accessor_policy"), "accessor_policy ist Organ-Erweiterung (View)");
    check(!ex::is_organ_axis_label("telemetry"), "telemetry ist KEINE Organ-Achse (system_measurement/config)");
    check(!ex::is_organ_axis_label("compiler"), "compiler ist KEINE Organ-Achse (system_config)");
    check(!ex::is_organ_axis_label("opt_level"), "opt_level ist KEINE Organ-Achse (System-Unter-Achse)");
    check(!ex::is_organ_axis_label("extension_hardware"), "extension_hardware ist KEINE Organ-Achse (System)");

    // (c) RequiredOrgans<G>() liefert genau die slot_count Organ-Achsen je Gattung.
    check(ex::RequiredOrgans<cea::AnatomyGenus::SearchAlgorithm>().size() == 18u, "SearchAlgorithm RequiredOrgans==18");
    check(ex::RequiredOrgans<cea::AnatomyGenus::Adapter>().size() == 12u, "Adapter RequiredOrgans==12");
    check(ex::RequiredOrgans<cea::AnatomyGenus::Set>().size() == 14u, "Set RequiredOrgans==14");
    check(ex::RequiredOrgans<cea::AnatomyGenus::Sequence>().size() == 10u, "Sequence RequiredOrgans==10");
    check(ex::RequiredOrgans<cea::AnatomyGenus::View>().size() == 6u, "View RequiredOrgans==6");

    std::cout << (g_fail == 0 ? "\n==== INC-1b Organ-Gattungs-Bindung: ALLE OK ====\n" : "\n!! INC-1b: FEHLER\n");
    return g_fail == 0 ? 0 : 1;
}
