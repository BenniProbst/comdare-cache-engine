#pragma once
// Inkrementeller Tier-Binary-Cache (Bauplan §2+§4, 2026-07-18) — compile-time {axis,variant -> algo_version}-Tabelle.
//
// Der Rebuild-/Neu-Mess-Selektor braucht je Binary eine deterministische Organ-Algorithmus-Signatur (algo_sig).
// Diese Tabelle liefert das Fundament: sie reflektiert die 17 KOMPOSITIONS-Achsen (kCompositionAxisNames-Reihenfolge,
// axis_path_serialization.hpp) aus DENSELBEN Registry-Enabled-Listen wie registry_to_axis_levels.hpp (axes26-Aliase,
// BR-1, REGISTRY-getrieben statt string-getrieben) in Tripel {axis, W::name(), W::algo_version}. compose_algo_signature
// (axis_path_serialization.hpp) schlaegt hier je (axis,value) die Version nach und baut die Flach-Signatur.
//
// **DIES IST DIE UNIVERSELLE COMPILE-ZEIT-DURCHSETZUNG (Bauplan §2 „Concept erzwingt version()"):** das mp_for_each
// greift W::algo_version je registrierter Variante ab — fehlt einer Variante das Member, ist der Zugriff ill-formed und
// die Kompilation bricht MIT DEM TYP-NAMEN. Eine Organ-Variante ohne algo_version kann so nicht unbemerkt in eine
// Registry gelangen. Ergaenzend traegt JEDE der 17 Kompositions-StrategyBases (search_algo..queuing_q2) im CRTP-Ctor
// einen fokussierten `static_assert(requires { Derived::algo_version; })` (Bauplan §2, Exemplar
// axis_06_allocator_strategy_base.hpp) — der Concept-Guard je Konstruktion. Bewusst NICHT an der gemeinsamen Wurzel
// topics::OrganAxis: die traegt auch die System-Achse ISA und die 4 Shape-Achsen, die KEIN algo_version fuehren
// (Organ- vs System-/Shape-Provenienz strikt getrennt) — ein Wurzel-Guard wuerde die faelschlich brechen.
//
// System-Achsen (telemetry/isa/page_type/simd_extension/general_hardware) und die 4 Shape-Achsen bleiben AUSSEN: sie
// tragen KEINE algo_sig (Organ- vs System-Provenienz strikt getrennt; Shapes sind Default-OFF-Anhang). C++23, header-
// only; heap-schwer (inkludiert ALLE Kompositions-Registries via registry_to_axis_levels.hpp) — nur dort einbinden,
// wo die Registries ohnehin praesent sind (Facade / dedizierter Test), NIE in einen schlanken TU.

#include "axis_path_serialization.hpp" // kCompositionAxisNames (Slot-Reihenfolge der algo_sig)
#include "registry_to_axis_levels.hpp" // axes26::T* Registry-Aliase (dieselbe Quelle wie build_all_axis_levels)

#include <axes/alloc/alloc_hw_config.hpp>       // kAllocHwSubaxisVersion (Sub-Achsen-Werteset, Bauplan §2)
#include <axes/cacheline/cacheline_config.hpp>  // kCacheLineSubaxisVersion
#include <axes/cacheline/node_width_config.hpp> // kNodeWidthSubaxisVersion

#include <boost/mp11.hpp>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::experiment {

namespace mp = boost::mp11;

/// Ein Tabellen-Eintrag: (Kompositions-Achsen-Name, Varianten-name(), algo_version).
struct AxisVariantVersion {
    std::string_view axis;    ///< kCompositionAxisNames-Slot (z.B. "search_algo")
    std::string      variant; ///< W::name() (z.B. "bst")
    std::string      version; ///< W::algo_version (z.B. "v1")
};

/// mp_for_each ueber eine Registry-Enabled-Liste -> {axis, W::name(), W::algo_version} je Variante. mp_identity
/// vermeidet das Default-Konstruieren der Wrapper (nur der Typ wird benoetigt — analog reflect_names, axis_reflect.hpp).
/// Der W::algo_version-Zugriff ist die harte Compile-Zeit-Durchsetzung (siehe Datei-Kopf).
template <class List>
inline void reflect_versions(std::string_view axis, std::vector<AxisVariantVersion>& out) {
    mp::mp_for_each<mp::mp_transform<mp::mp_identity, List>>([&](auto id) {
        using W = typename decltype(id)::type;
        out.push_back(AxisVariantVersion{axis, std::string{W::name()}, std::string{W::algo_version}});
    });
}

/// Baut die {axis,variant->version}-Tabelle ueber GENAU die 17 Kompositions-Achsen (kCompositionAxisNames-Reihenfolge
/// = algo_sig-Slot-Reihenfolge). Registry-getrieben (axes26-Aliase). Der Alias-Fahrplan spiegelt exakt
/// append_organ_core_axis_levels() + append_composition_tail_axis_levels() (die q1/q2-Slots) — OHNE die build-only-/
/// System-Achsen und OHNE die Shape-Achsen (die tragen keine algo_sig).
[[nodiscard]] inline std::vector<AxisVariantVersion> build_axis_variant_version_table() {
    std::vector<AxisVariantVersion> t;
    reflect_versions<axes26::T00_search_algo>("search_algo", t);
    reflect_versions<axes26::T01_cache_traversal>("cache_traversal", t);
    reflect_versions<axes26::T02_mapping>("mapping", t);
    reflect_versions<axes26::T03_path_compression>("path_compression", t);
    reflect_versions<axes26::T04_node_type>("node_type", t);
    reflect_versions<axes26::T05_memory_layout>("memory_layout", t);
    reflect_versions<axes26::T06_allocator>("allocator", t);
    reflect_versions<axes26::T07_prefetch>("prefetch", t);
    reflect_versions<axes26::T08_concurrency>("concurrency", t);
    reflect_versions<axes26::T09_serialization>("serialization", t);
    reflect_versions<axes26::T11_value_handle>("value_handle", t);
    reflect_versions<axes26::T13_index_organization>("index_organization", t);
    reflect_versions<axes26::T14_io_dispatch>("io_dispatch", t);
    reflect_versions<axes26::T15_migration_policy>("migration_policy", t);
    reflect_versions<axes26::T16_filter>("filter", t);
    reflect_versions<axes26::T20_queuing_q1>("queuing_q1", t);
    reflect_versions<axes26::T21_queuing_q2>("queuing_q2", t);
    return t;
}

/// Nachschlag der algo_version einer (axis, variant)-Kombination. Nicht gefunden -> leerer view (der Aufrufer
/// compose_algo_signature emittiert dann den Sentinel @v0 statt zu raten). Lineare Suche ueber die kleine Tabelle.
[[nodiscard]] inline std::string_view lookup_algo_version(std::vector<AxisVariantVersion> const& table,
                                                          std::string_view axis, std::string_view variant) {
    for (AxisVariantVersion const& e : table)
        if (e.axis == axis && e.variant == variant) return e.version;
    return std::string_view{};
}

/// Die globalen Sub-Achsen-Werteset-Versionen (Bauplan §2): eine Werteset-ERWEITERUNG (z.B. neuer CacheLineConfig-
/// Wert) aendert das serialisierte Bit-Layout, ohne dass eine Varianten-algo_version bumpt -> muss in die algo_sig,
/// sonst wuerde eine layout-geaenderte Binary STILL reused. Sie sind BUILD-global (nicht per-Variante) -> als fester
/// Schwanz an jede algo_sig gehaengt; ein Bump invalidiert konsequenterweise ALLE Binaries (grobkoernig, aber korrekt
/// und selten). Deterministisch, plattform-stabil.
[[nodiscard]] inline std::string sub_axis_valueset_segment() {
    return "sub=cacheline@v" + std::to_string(::comdare::cache_engine::cacheline::kCacheLineSubaxisVersion) +
           ",node_width@v" + std::to_string(::comdare::cache_engine::cacheline::kNodeWidthSubaxisVersion) +
           ",alloc_hw@v" + std::to_string(::comdare::cache_engine::alloc::kAllocHwSubaxisVersion);
}

/// compose_algo_signature(axes, table) — die deterministische Organ-Algorithmus-Signatur EINER Binary. Iteriert die
/// kCompositionAxisNames-Slots in FESTER Reihenfolge (plattform-stabil, unabhaengig von der spec.axes-Reihenfolge),
/// sucht je Slot den (axis,value) in spec.axes und schlaegt dessen algo_version in der Tabelle nach. Format je Slot
/// "<axis>=<variant>@<version>", per ';' gejoint (Vorbild perm.algos-Sidecar, Bauplan §1), abgeschlossen vom
/// Sub-Achsen-Werteset-Schwanz. Nicht-Kompositions-Achsen (Shapes) und in spec.axes fehlende Slots werden
/// uebersprungen; eine unbekannte (axis,value)-Kombination -> @v0-Sentinel (statt zu raten). Der Rebuild-/Neu-Mess-
/// Selektor vergleicht diese Signatur String-gleich gegen den .algos-Sidecar -> nur Binaries mit geaenderter Signatur
/// (= eine gebumpte Variante im 17-Tupel ODER ein Werteset-Bump) werden neu gebaut/gemessen; die binary_id bleibt
/// unberuehrt (die Version lebt ausschliesslich im Sidecar).
[[nodiscard]] inline std::string compose_algo_signature(std::vector<std::pair<std::string, std::string>> const& axes,
                                                        std::vector<AxisVariantVersion> const&                  table) {
    std::string out;
    for (std::string_view const slot : kCompositionAxisNames) {
        for (auto const& [ax, val] : axes) {
            if (ax != slot) continue;
            std::string_view const ver = lookup_algo_version(table, slot, val);
            if (!out.empty()) out += ';';
            out += slot;
            out += '=';
            out += val;
            out += '@';
            out += ver.empty() ? std::string_view{"v0"} : ver;
            break;
        }
    }
    if (!out.empty()) out += ';';
    out += sub_axis_valueset_segment();
    return out;
}

} // namespace comdare::cache_engine::builder::experiment
