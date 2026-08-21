#pragma once

// genus_leistungs_version.hpp -- K02/F2-7a: die GENUS-LEISTUNGS-Version als CT-Komposition.
//
// OWNER-SOLL (KON2-06, 10.08.2026, verbatim; vom Owner bestaetigt "Das ist jetzt alles korrekt."):
//   "Jedes Genus hat fuer sein Interface eine Versionierung fuer das was es Leistet und das setzt
//    sich als compile time Versionierungsstempel aus allen Mess/System/Organ-Achsen zusammen."
// KON2-07 (Mechanismus): je Glied die feste Achsen-Reihenfolge seiner Quelle; die drei Glieder
// isoliert -- Organ aus der gewaehlten lokalen Algorithmus-Versionierung (Comp), System isoliert
// fuer Hardware (statische Code-Identitaet), Mess statisch zur Compile-Zeit (Tooling-Registry).
//
// ERGAENZUNG, KEIN ERSATZ (algo_semver.hpp, Versions-Klasse (i)): die fuenf Dock-Literale in
// pruef_dock_version.hpp sind die VERTRAGS-Version des Docks und bleiben unberuehrt. DIESE Datei
// traegt die LEISTUNGS-Version -- was das Interface eines Genus leistet, nicht was sein Dock
// zusichert. Kein Konsument ausser dem Test; KEIN Preimage-/golden-/Stempel-Anschluss (ein
// Anschluss waere ein eigenes deklariertes Byte-Ereignis).
//
// ORT (bewusste Abweichung vom BAULISTE-Vorschlag measurement/): am Objekt gemessen existiert
// heute KEINE include-Kante measurement->abi, abi->anatomy oder anatomy->abi (beide Richtungen
// 0 Treffer, 2026-08-21). Ein measurement-Header mit AnatomyGenus-Parameter haette die erste
// solche Kante gelegt -- ein Schicht-Ereignis, kein Versions-Bau. HIER (builder/pruef_dock)
// treffen sich beide Welten heute schon: pruef_dock_version.hpp inkludiert anatomy UND
// measurement; die Leistungs-Version wohnt damit NEBEN der Vertrags-Version, deren Ergaenzung
// sie ist.
//
// GRAMMATIK der Komposition (eine Zeile, CT-auswertbar und RT-identisch):
//   "genus=<token>|mess=<glied>|system=<glied>|organ=<glied>"
// Glieder-Reihenfolge MESS, SYSTEM, ORGAN == Aussen-Ebenen-Ordnung (S-6a/#15, KON21-03; dieselbe
// Feldfolge wie abi::AnatomyVersionLines measurement_line/system_line/organ_line). Je Glied
// ';'-getrennte "achse=algo@X.Y.Z"-Segmente ueber build_axis_version_stamp_line -- die EINE
// Grammatik-Funktion der Stempel-Zeilen (measurement/axis_version_stamp.hpp), seit K02/F2-7a
// constexpr. Meta-Meta-Klammer-Anhaenge (A13-M2) reisen hier NICHT mit: KON2-06 komponiert
// ACHSEN; Meta-Metas sind eine eigene Ebene (PMC=Meta-Meta, Owner 10.08.) und bleiben Sache der
// Stempel-Zeilen.
//
// PARTITION (dasselbe Muster wie pruef_dock_version_for, beide Richtungen):
//   HIN  -- die fuenf andockenden Genera tragen eine nicht-leere Leistungs-Version.
//   RUECK-- FunctionInterfaceReroute traegt die LEERE: ein KLASSIFIKATIONS-Genus (Owner-E-1
//           "Weg C") leistet ueber sein Interface das ZIEL-Genus, nie sich selbst.

#include "../experiment_tree/axis_path_serialization.hpp" // experiment::kCompositionAxisNames (ORG-18)

#include <anatomy/anatomy_base.hpp>                        // AnatomyGenus (Ebene 2)
#include <cache_engine/abi/system_axis_code_versions.hpp>  // kSystemAxisCodeVersions (Single-Source)
#include <cache_engine/measurement/axis_version_stamp.hpp> // AxisVersionEntry + build_axis_version_stamp_line
#include <mess_axes/measurement_tooling_registry.hpp>      // kMeasurementToolingRegistry (Vollmenge)

#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>

namespace comdare::cache_engine::builder::pruef_dock {

namespace glv_detail {
namespace meas = ::comdare::cache_engine::measurement;
namespace exp  = ::comdare::cache_engine::builder::experiment;
} // namespace glv_detail

/// Der Genus-Token der Leistungs-Version (snake_case-Hausform). BEWUSSTE KOPIE des Switch aus
/// bestandslog/lager_baum_writer.hpp::lager_genus_token -- der Writer ist ein 945-Zeilen-fstream-
/// Header und als Include fuer eine reine constexpr-Versions-Quelle falsch; die Gleichheit beider
/// Switches ist in test_genus_ct_komposition.cpp fuer alle 6 Genera gepinnt (Drift-Wache am
/// Verbrauch, wie ueberall in dieser Welt: eine Wache an der Quelle, eine am Verbrauch).
[[nodiscard]] constexpr std::string_view genus_leistungs_token(anatomy::AnatomyGenus g) noexcept {
    switch (g) {
        case anatomy::AnatomyGenus::SearchAlgorithm: return "search_algorithm";
        case anatomy::AnatomyGenus::Set: return "set";
        case anatomy::AnatomyGenus::Sequence: return "sequence";
        case anatomy::AnatomyGenus::Adapter: return "adapter";
        case anatomy::AnatomyGenus::View: return "view";
        case anatomy::AnatomyGenus::FunctionInterfaceReroute: return "function_interface_reroute";
    }
    return {};
}

/// MESS-Glied: die volle Tooling-Menge in Registry-Reihenfolge (Index == Tooling, static_assert-
/// gesichert in der Registry selbst). Dieselbe Eintrags-Form wie measurement_stamp_line
/// ("measurement_tooling=<id>@X.Y.Z"), ohne Meta-Meta-Anhang (Kopf: Achsen, keine Meta-Metas).
[[nodiscard]] constexpr std::string genus_leistungs_mess_glied() {
    using glv_detail::meas::AxisVersionEntry;
    std::array<AxisVersionEntry, glv_detail::meas::kMeasurementToolingCount> entries{};
    for (std::size_t i = 0; i < entries.size(); ++i) {
        auto const& info = glv_detail::meas::kMeasurementToolingRegistry[i];
        entries[i]       = {"measurement_tooling", info.id, info.version};
    }
    return glv_detail::meas::build_axis_version_stamp_line(entries);
}

/// SYSTEM-Glied: die drei Haupt-Achsen mit ihrer statischen Code-Identitaet ("code"-Marker wie
/// system_stamp_line, Phase-1-Form). Zellwerte sind BAU-Ereignisse einer konkreten Binary und
/// gehoeren nicht in die Interface-Leistung eines Genus.
[[nodiscard]] constexpr std::string genus_leistungs_system_glied() {
    using glv_detail::meas::AxisVersionEntry;
    std::array<AxisVersionEntry, ::comdare::cache_engine::abi::kSystemAxisCodeCount> entries{};
    for (std::size_t i = 0; i < entries.size(); ++i) {
        auto const& e = ::comdare::cache_engine::abi::kSystemAxisCodeVersions[i];
        entries[i]    = {e.axis, "code", e.version};
    }
    return glv_detail::meas::build_axis_version_stamp_line(entries);
}

/// ORGAN-Glied: die 18 Organ-Haupt-Achsen der Composition in kCompositionAxisNames-Reihenfolge
/// (KON2-07: "in fester Achsen-Reihenfolge in der Organ-Achsen-Klammer eingereiht"). Die Namen
/// kommen NICHT aus einer zweiten Liste: jede Position ist unten per static_assert gegen
/// experiment::kCompositionAxisNames gepinnt -- die Aufzaehlung verbindet nur Achsen-Name und
/// Comp-Member (dieselbe unvermeidbare Bruecke wie abi::organ_stamp_line, inkl. Form-Wache).
template <class Comp>
[[nodiscard]] constexpr std::string genus_leistungs_organ_glied() {
    using glv_detail::meas::AxisVersionEntry;
    static constexpr std::array<AxisVersionEntry, glv_detail::exp::kCompositionAxisNames.size()> entries{{
        {"search_algo", Comp::search_algo::name(), Comp::search_algo::algo_version},
        {"cache_traversal", Comp::cache_traversal::name(), Comp::cache_traversal::algo_version},
        {"mapping", Comp::mapping::name(), Comp::mapping::algo_version},
        {"path_compression", Comp::path_compression::name(), Comp::path_compression::algo_version},
        {"node_type", Comp::node_type::name(), Comp::node_type::algo_version},
        {"memory_layout", Comp::memory_layout::name(), Comp::memory_layout::algo_version},
        {"allocator", Comp::allocator::name(), Comp::allocator::algo_version},
        {"prefetch", Comp::prefetch::name(), Comp::prefetch::algo_version},
        {"concurrency", Comp::concurrency::name(), Comp::concurrency::algo_version},
        {"serialization", Comp::serialization::name(), Comp::serialization::algo_version},
        {"value_handle", Comp::value_handle::name(), Comp::value_handle::algo_version},
        {"index_organization", Comp::index_organization::name(), Comp::index_organization::algo_version},
        {"io_dispatch", Comp::io_dispatch::name(), Comp::io_dispatch::algo_version},
        {"migration_policy", Comp::migration_policy::name(), Comp::migration_policy::algo_version},
        {"filter", Comp::filter::name(), Comp::filter::algo_version},
        {"queuing_q1", Comp::queuing_q1::name(), Comp::queuing_q1::algo_version},
        {"queuing_q2", Comp::queuing_q2::name(), Comp::queuing_q2::algo_version},
        {"persistence_target", Comp::persistence_target::name(), Comp::persistence_target::algo_version},
    }};
    // Positions-Pin gegen die EINE Namensliste (keine zweite Quelle, nur eine Bruecke):
    static_assert(
        [] {
            for (std::size_t i = 0; i < entries.size(); ++i)
                if (entries[i].axis != glv_detail::exp::kCompositionAxisNames[i]) return false;
            return true;
        }(),
        "Organ-Glied: Achsen-Name oder -Reihenfolge weicht von kCompositionAxisNames ab (ORG-18).");
    // Dieselbe Form-Wache wie an den Stempel-Zeilen (stiller "@0.0.0"-Kollaps ist verboten):
    static_assert(glv_detail::meas::axis_version_entries_are_wellformed(entries),
                  "Organ-Glied: eine algo_version dieser Composition ist UNPARSBAR (Flag-Grammatik v2 "
                  "oder Sentinel \"0.0.0\") und wuerde still als '@0.0.0' komponiert.");
    return glv_detail::meas::build_axis_version_stamp_line(entries);
}

/// Die GENUS-LEISTUNGS-Version (KON2-06): CT-Komposition der drei Glieder in Aussen-Ordnung
/// MESS, SYSTEM, ORGAN. Reroute -> LEER (Partition, Kopf). constexpr UND Laufzeit -- dieselbe
/// Funktion, dieselben Bytes (Beweis: static_assert + RT-Vergleich im Test).
template <class Comp>
[[nodiscard]] constexpr std::string genus_leistungs_version(anatomy::AnatomyGenus g) {
    if (g == anatomy::AnatomyGenus::FunctionInterfaceReroute) return {};
    std::string out{"genus="};
    out += genus_leistungs_token(g);
    out += "|mess=";
    out += genus_leistungs_mess_glied();
    out += "|system=";
    out += genus_leistungs_system_glied();
    out += "|organ=";
    out += genus_leistungs_organ_glied<Comp>();
    return out;
}

} // namespace comdare::cache_engine::builder::pruef_dock
