#pragma once
// KERN-B K5 (Section 59, 2026-07-20; User-GO "Anatomy = Stempel-Vorlage, nach Plan reparieren + weiterentwickeln")
// -- merge_plan: die DEKLARATIVE NAHT zwischen dem geparsten ExperimentProfile (KERN-A: per-Achse merge_mode +
// Pruefling-Identitaet, xml_config_parser.hpp) und dem direktiven-getriebenen Emitter (sota_catalog K5). KERN-A hat
// den per-Achse merge_mode NUR geparst + validiert (validate_profile.hpp:965-980, {replace,merge}); dieses Modul
// projiziert ihn auf einen expliziten Direktiven-Vektor, der den Emitter von der KATALOG-Verdrahtung (die 6 hart
// aufgelisteten <Host>PrtStufeN-Typen) auf eine AxisMergeDirective-getriebene MergeAxis<>-Instanziierung
// generalisiert (Section 59-A: "kein Neubau, sondern Umverdrahtung + Schema").
//
// REIN DATEN (POD/Strings), isoliert testbar, KEIN Bau/keine Messung/kein DLL-Emit. ADDITIV + golden-neutral:
//   * LEERES / heutiges Profil (KEIN <axes_default_lookup><axis merge=..>) -> LEERER Direktiven-Vektor -> der
//     Aufrufer faellt auf den bestehenden KATALOG-Pfad zurueck (byte-identisch; s. sota_catalog K5).
//   * Ein per-Achse-merge-Profil -> je markierter Achse EINE Direktive -> der Direktiven-Pfad greift.
// Heute traegt KEIN committetes Profil per-Achse-Direktiven => alle emittierten .cpp-Quelltexte bleiben byte-gleich,
// der golden-CRC 0xF1C1F26A1232073B unberuehrt (die Merges sind ein additiver id-Satz).
//
// @related sota_catalog.hpp (render_directive_merge_module_source) ; pruefling_merge.hpp (MergeStrategy/MergeAxis)
//          ; anatomy_version_stamp.hpp (merge_stamp_line = der dritte Tier-Stempel aus diesen Direktiven)

#include "xml_config_parser/xml_config_parser.hpp" // ExperimentProfile / ExperimentAxisDefault / ExperimentPhase

#include <string>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::thesis_lazy {

namespace mp_cx = ::comdare::builder::xml;

/// AxisMergeDirective -- EINE per-Achse Merge-Direktive (POD/Strings). Sie ersetzt die frueher hart aufgelistete
/// (Host, Stufe)-Katalog-Wahl durch eine deklarative Achsen-Anweisung: WELCHE Achse (axis_ref) wird mit WELCHER
/// MergeStrategy (strategy, pruefling_merge.hpp-Name) um WELCHEN Pruefling-Slot (pruefling_slot) gemergt, ueber
/// WELCHE Varianten-Whitelist (allowed_variants; leer = die volle Registry-Liste, Fork 5 Obergrenze = Angebot).
struct AxisMergeDirective {
    std::string axis_ref;       ///< Registry-axis-id (== ExperimentAxisDefault::ref, z.B. "path_compression")
    std::string strategy;       ///< MergeStrategy-Name ("Stufe1_CeOnly"/"Stufe2_PrueflingReplace"/"Stufe3_FullJoin")
    std::string pruefling_slot; ///< Pruefling-Identitaet, die den Slot belegt ("" / "CacheEngine"/"self" = ce, Stufe1)
    std::vector<std::string> allowed_variants; ///< Achsen-Varianten-Whitelist (Teilmenge; leer = volle Liste)
};

/// merge_mode_to_strategy(merge_mode) -- die per-Achse merge_mode-Semantik ({replace,merge,fulljoin}) auf die
/// MergeStrategy-Namen abbilden (Single-Source der Zuordnung). ""/"replace" => Stufe2_PrueflingReplace (die
/// Pruefling-Achse ERSETZT die CE-Achse mit Fallback); "merge"/"fulljoin" => Stufe3_FullJoin (Union CE + Pruefling,
/// non-redundant). KERN #48-S4 (Verdikt V-a): "fulljoin" ist der EXPLIZITE Phase-3-Token -- validate erzwingt seine
/// Phase-3-Bindung (validate_profile.hpp), waehrend "merge" der tolerante Legacy-Token bleibt; beide projizieren auf
/// dieselbe FullJoin-Union (das MergeStrategy-Enum traegt heute genau drei Werte, pruefling_merge.hpp). Die volle
/// Materialisierung einer getrennten Stufe-2-Hybrid-Strategie ist Director-Konsum (post-S4). Section-59-A(1)
/// Stufe1_CeOnly ist die Abwesenheit einer Pruefling-Direktive (kein axes_default_lookup-merge / self).
[[nodiscard]] inline std::string merge_mode_to_strategy(std::string const& merge_mode) {
    if (merge_mode == "merge" || merge_mode == "fulljoin") return "Stufe3_FullJoin";
    return "Stufe2_PrueflingReplace"; // "" (Default) und "replace" => ERSETZT-mit-Fallback (Stufe2)
}

/// profile_pruefling_identity(ep) -- die Pruefling-Identitaet der Merge-Phasen des Profils. Die erste <phase>, die
/// einen expliziten pruefling deklariert und NICHT als CacheEngine-self markiert ist (identity!="CacheEngine"/"self",
/// pruefling!="CacheEngine"/"self"), liefert den Pruefling; sonst "" (kein Pruefling = ce/self, Stufe1). Fork 3:
/// identity="CacheEngine"/self ist ein expliziter self-Pruefling => trage NICHTS zum Merge bei (leere Slot-Liste).
[[nodiscard]] inline std::string profile_pruefling_identity(mp_cx::ExperimentProfile const& ep) {
    auto const is_self = [](std::string const& s) { return s == "CacheEngine" || s == "self"; };
    for (auto const& ph : ep.phases) {
        if (is_self(ph.identity)) continue; // Fork 3: self-Phase traegt keinen Merge-Pruefling
        if (!ph.pruefling.empty() && !is_self(ph.pruefling)) return ph.pruefling;
    }
    return {}; // kein deklarierter Merge-Pruefling => ce/self (Stufe1, leere Slot-Liste)
}

/// merge_plan_from_profile(ep) -- die DEKLARATIVE Projektion: je <axes_default_lookup><axis ref merge=..>-Eintrag
/// mit NICHT-LEEREM merge_mode EINE AxisMergeDirective (Dokument-Reihenfolge, stabil). Der Pruefling-Slot je
/// Direktive = die Merge-Phasen-Pruefling-Identitaet des Profils (profile_pruefling_identity); die Strategie =
/// merge_mode_to_strategy(merge_mode). allowed_variants = die per-Achse-Whitelist des Eintrags.
///
/// LEER, wenn KEINE Achse einen merge_mode traegt (heutiges Profil) => der Aufrufer nutzt den KATALOG-Pfad
/// (byte-identisch). Reine Ableitung -- KEIN Bau, KEINE Messung. golden-neutral.
[[nodiscard]] inline std::vector<AxisMergeDirective> merge_plan_from_profile(mp_cx::ExperimentProfile const& ep) {
    std::vector<AxisMergeDirective> out;
    std::string const               pruefling = profile_pruefling_identity(ep);
    for (auto const& ax : ep.axes_default_lookup) {
        if (ax.merge_mode.empty()) continue; // leer = replace-Default OHNE explizite Direktive => Katalog-Pfad
        out.push_back(
            AxisMergeDirective{ax.ref, merge_mode_to_strategy(ax.merge_mode), pruefling, ax.allowed_variants});
    }
    return out;
}

} // namespace comdare::cache_engine::thesis_lazy
