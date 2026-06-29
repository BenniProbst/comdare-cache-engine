#pragma once
// BR-1 (2026-06-02, Doc 27 §3) — GENERISCHE Registry→Baum-Reflektions-Helfer (umbrella-UNABHÄNGIG).
//
// Getrennt von registry_to_axis_levels.hpp (das die ConfigSets aller 22 Achsen (15 Topics) inkludiert = umbrella-schwer),
// damit der Reflektions-MECHANISMUS gegen eine ECHTE Achsen-Teilmenge schnell + robust verifizierbar ist
// (der volle 22-Achsen-Einzel-TU ist compiler-heap-schwer). Reflektiert eine Enabled-mp_list je Achse in einen
// statischen AxisLevel (block_id-getaggt → Knoten-Rück-Referenz, Bidirektionalität). C++23, header-only.

#include "experiment_tree.hpp"

#include <boost/mp11.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace comdare::cache_engine::builder::experiment {

namespace mp = boost::mp11;

/// Reflektiert eine mp_list von Wrapper-Typen → ihre Wert-Namen (W::name()). mp_identity vermeidet, die
/// Wrapper default-zu-konstruieren (nur der Typ wird benötigt).
template <class List>
[[nodiscard]] inline std::vector<std::string> reflect_names() {
    std::vector<std::string> out;
    mp::mp_for_each<mp::mp_transform<mp::mp_identity, List>>([&](auto id) {
        using W = typename decltype(id)::type;
        out.emplace_back(std::string{W::name()});
    });
    return out;
}

/// Anzahl Wrapper in einer Enabled-Liste (compile-time).
template <class List>
inline constexpr std::size_t enabled_count = mp::mp_size<List>::value;

/// Hängt einen STATISCHEN AxisLevel aus der realen Enabled-Liste an (Fanout = volles Enabled-Inventar);
/// block_id = Achsen-Name → die materialisierten Knoten verweisen darauf zurück (Bidirektionalität).
template <class List>
inline void push_static_axis(std::vector<AxisLevel>& lv, char const* axis) {
    lv.push_back(
        AxisLevel{axis, reflect_names<List>(), /*is_static=*/true, std::string{}, /*block_id=*/std::string{axis}});
}

} // namespace comdare::cache_engine::builder::experiment
