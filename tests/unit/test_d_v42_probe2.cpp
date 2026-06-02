// V42-Probe2 (2026-06-02) — KORREKTUR-Probe nach der Member-Hold-Sackgasse (Doc 29 §3b). Die erste Probe
// (test_d_v42_probe) testete InsertCounter/DensityTracker/LatencyHistogram, aber ArtComposition::telemetry =
// LeafOnlyCounter. Diese Probe testet den REALEN Wrapper + klaert drei Fragen literal:
//   (1) is_aggregate? -> falls ja, ist das `{}`-Aggregat-Init der Baufehler-Grund (protected Base-ctor direkt).
//   (2) Member-Hold OHNE `{}` (default-init) instanziierbar? -> falls ja, ist `T member;` der triviale Fix.
//   (3) ObservableAxis? -> LeafOnlyCounter ist ein reiner Strategie-Marker (kein statistics()) -> erwartet 0,
//       d.h. der telemetry-Observer muss eine Huelle mit EIGENER Statistik tragen (analog ObservableComposedSearch).
// Build: scratch_compile_test.ps1 -Test test_d_v42_probe2 -Boost -Extra @("build\generated")

#include <axes/telemetry_axis/axis_11_telemetry_leaf_only.hpp>
#include <axes/telemetry_axis/axis_11_telemetry_insert_counter.hpp>
#include "anatomy/observer_aggregate.hpp"

#include <iostream>
#include <type_traits>

namespace t = comdare::cache_engine::telemetry_axis;
namespace a = comdare::cache_engine::anatomy;

// Approximiert das Member-`{}`-Verhalten (value/aggregate-init): bei einem Aggregat mit unzugaenglicher
// Basis ist `T{}` ill-formed -> Concept false. Das ist genau der search_algorithm_anatomy.hpp-Baufehler.
template <class T> inline constexpr bool brace_ok = requires { T{}; };

// Member-Hold OHNE `{}` (default-init durch den umschliessenden default-ctor): kompiliert dies, ist
// `typename Composition::telemetry axis_telemetry_;` (ohne Brace) der Fix.
template <class T> struct HoldPlain { T x; };

int main() {
    std::cout << "==== V42-Probe2: REALER Wrapper LeafOnlyCounter (= ArtComposition::telemetry) ====\n";
    std::cout << "LeafOnlyCounter: is_aggregate=" << std::is_aggregate_v<t::LeafOnlyCounter>
              << " default_constructible=" << std::is_default_constructible_v<t::LeafOnlyCounter>
              << " brace_ok{T{}}=" << brace_ok<t::LeafOnlyCounter>
              << " ObservableAxis=" << a::ObservableAxis<t::LeafOnlyCounter> << "\n";
    std::cout << "InsertCounter:   is_aggregate=" << std::is_aggregate_v<t::InsertCounter>
              << " default_constructible=" << std::is_default_constructible_v<t::InsertCounter>
              << " brace_ok{T{}}=" << brace_ok<t::InsertCounter>
              << " ObservableAxis=" << a::ObservableAxis<t::InsertCounter> << "\n";

    // Der entscheidende Test: Member-Hold OHNE `{}` (default-init) fuer den realen Wrapper.
    HoldPlain<t::LeafOnlyCounter> hp;
    (void)hp;
    std::cout << "HoldPlain<LeafOnlyCounter> (Member OHNE {}) : KONSTRUIERT (default-init klappt)\n";

    std::cout << "\n==== Befund dokumentiert (Doc 29 §3c folgt) ====\n";
    return 0;
}
