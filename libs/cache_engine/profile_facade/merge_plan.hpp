#pragma once
// KERN-B K5 (Section 59, 2026-07-20; User-GO "Anatomy = Stempel-Vorlage, nach Plan reparieren + weiterentwickeln")
// -- merge_plan: die DEKLARATIVE NAHT zwischen dem geparsten ExperimentProfile (KERN-A: per-Achse merge_mode +
// Pruefling-Identitaet, xml_config_parser.hpp) und dem direktiven-getriebenen Emitter (sota_catalog K5). KERN-A hat
// den per-Achse merge_mode NUR geparst + validiert (validate_profile.hpp, KERN-A-Abschnitt axes_default_lookup;
// Mengen-Single-Source kExperimentAxisMergeModes UNTEN IN DIESER DATEI: {replace,merge,union}); dieses Modul
// projiziert ihn auf einen expliziten Direktiven-Vektor, der den Emitter von der KATALOG-Verdrahtung (die 6 hart
// aufgelisteten <Host>PrtVerbundN-Typen) auf eine AxisMergeDirective-getriebene MergeAxis<>-Instanziierung
// generalisiert (Section 59-A: "kein Neubau, sondern Umverdrahtung + Schema").
//
// REIN DATEN (POD/Strings), isoliert testbar, KEIN Bau/keine Messung/kein DLL-Emit. ADDITIV + golden-neutral:
//   * LEERES / heutiges Profil (KEIN <axes_default_lookup><axis merge=..>) -> LEERER Direktiven-Vektor -> der
//     Aufrufer faellt auf den bestehenden KATALOG-Pfad zurueck (byte-identisch; s. sota_catalog K5).
//   * Ein per-Achse-merge-Profil -> je markierter Achse EINE Direktive -> der Direktiven-Pfad greift.
// Heute traegt KEIN committetes Profil per-Achse-Direktiven => alle emittierten .cpp-Quelltexte bleiben byte-gleich,
// der golden-ids-CRC unberuehrt (die Merges sind ein additiver id-Satz). [B-10.3/golden-102: der hier
// zitierte Wert 0xF1C1F26A1232073B ist der ALT-Anker (Historie, vor 26.07.); lebender TABU-Anker =
// kNewGolden131072Crc64 = 0x56F1B721C72DC10E, source_catalog.hpp.]
//
// @related sota_catalog.hpp (render_directive_merge_module_source) ; pruefling_merge.hpp
// (PrueflingVerbundStrategy/MergeAxis
//          + die A13-M3/C5-Q2-Wache an der Merge-Namens-Naht)
//
// A13-M3/C5 (K-6-Sweep): der frueher hier verwiesene DRITTE Tier-Stempel (anatomy_version_stamp.hpp::
// merge_stamp_line) EXISTIERT NICHT MEHR -- Owner-E2 vom 02.08.2026: "Merge Zeile kann daher nicht
// existieren". DIESES Modul ist davon NICHT betroffen (Owner-Q2: die Merge-Strategie wird weiter
// durchgefuehrt); es verliert nur seinen Stempel-Abnehmer. Die Merge-Art lebt im Stempel ab jetzt ueber das
// 'e'-Experimentalflag und die erweiterten hierarchischen Namen -- dass zwei byte-verschiedene
// Merge-Binaries dabei nie namensgleiche Organ-Segmente rendern, sichert die CT-Wache in
// pruefling_merge.hpp (MergeImpl), nicht mehr eine eigene Stempel-Zeile.

#include "xml_config_parser/xml_config_parser.hpp" // ExperimentProfile / ExperimentAxisDefault / ExperimentPhase

#include <stdexcept> // A2.5-g5 (Review #15, Fix 16): fail-closed-Wurf des Rest-Kollektors
#include <string>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::thesis_lazy {

namespace mp_cx = ::comdare::builder::xml;

// -- KERN-A (S4 Mess-Schema, 2026-07-20) + KERN #48-S4 (Fork 4 dritter Token, 2026-07-22): die drei erlaubten
//    per-Achse Merge-Modi. Leer=""=replace-Default (heutiges Verhalten byte-identisch); "merge" = additiver
//    Zusammenschluss statt Ersetzen; "union" = die EXPLIZITE, Phase-3-gebundene Union (Verdikt V-a: Section-59-A
//    trennt Verbund-2-Hybrid und Verbund-3-Union woertlich). Single-Source der Token HIER.
//    A2.5-g5 (Review #15, Fix 16): aus validate_profile.hpp HIERHER gezogen (die Include-Richtung ist
//    validate -> merge_plan; der fail-closed-Kollektor unten leitet seine Fehlermenge aus DIESEM Array ab,
//    und ein zweites Array waere die zweite Wissensquelle). validate_profile prueft weiter gegen genau
//    dieses Symbol; der Phase-3-Bindungs-Check zu "union" lebt dort. --
inline constexpr char const* kExperimentAxisMergeModes[] = {"replace", "merge", "union"};

/// AxisMergeDirective -- EINE per-Achse Merge-Direktive (POD/Strings). Sie ersetzt die frueher hart aufgelistete
/// (Host, Stufe)-Katalog-Wahl durch eine deklarative Achsen-Anweisung: WELCHE Achse (axis_ref) wird mit WELCHER
/// PrueflingVerbundStrategy (strategy, pruefling_merge.hpp-Name) um WELCHEN Pruefling-Slot (pruefling_slot) gemergt,
/// ueber
/// WELCHE Varianten-Whitelist (allowed_variants; leer = die volle Registry-Liste, Fork 5 Obergrenze = Angebot).
struct AxisMergeDirective {
    std::string axis_ref; ///< Registry-axis-id (== ExperimentAxisDefault::ref, z.B. "path_compression")
    std::string
        // PrueflingVerbundStrategy-Name ("Verbund1_CeOnly"/"Verbund2_Replace"/"Verbund2_Hybrid"/"Verbund3_Union")
        strategy;
    // Pruefling-Identitaet, die den Slot belegt ("" / "CacheEngine"/"self" = ce, Verbund1)
    std::string              pruefling_slot;
    std::vector<std::string> allowed_variants; ///< Achsen-Varianten-Whitelist (Teilmenge; leer = volle Liste)
};

/// merge_mode_to_strategy(merge_mode) -- die per-Achse merge_mode-Semantik ({replace,merge,union}) auf die
/// PrueflingVerbundStrategy-Namen abbilden (Single-Source der Zuordnung). ""/"replace" => Verbund2_Replace (die
/// Pruefling-Achse ERSETZT die CE-Achse mit Fallback).
/// R6 (Nacht-Audit 2026-07-22, §59-A(2)-Wortlaut = Gesetz): "merge" und "union" sind NICHT dasselbe und werden
/// NICHT mehr vermischt --
///   "merge"    => Verbund2_Hybrid   (§59-A(2): CE + Pruefling HYBRID je Pruefling),
///   "union" => Verbund3_Union (§59-A(3): kombinierte Union CE + Pruefling, non-redundant; validate erzwingt die
///                 Phase-3-Bindung, validate_profile.hpp).
/// Die fruehere Projektion (beide -> Verbund3_Union) war die Regression: sie kollabierte die Trennung der
/// Merge-KATEGORIEN. HISTORIE (Doku-Doktrin: supersedieren, nie loeschen): bis A13-M3 fiel dieser Kollaps am
/// DRITTEN Tier-Stempel auf, weil merge_stamp_line die Strategie VERBATIM trug ("merge=Verbund2_Hybrid" !=
/// "merge=Verbund3_Union"). Diese Zeile gibt es seit Owner-E2 nicht mehr; die Trennung lebt hier -- in der
/// Projektion selbst -- und in den erweiterten Namen/Flags der gemergten Varianten (Owner-Q2).
/// MATERIALISIERUNG DEFERRED (Director-Konsum, post-S4): das PrueflingVerbundStrategy-Enum (pruefling_merge.hpp) traegt
/// heute
/// NUR Verbund1_CeOnly/Verbund2_Replace/Verbund3_Union -- KEIN Verbund2_Hybrid. Der direktiven-getriebene Emitter
/// (sota_catalog.hpp: render_directive_merge_module_source) rendert die Strategie als
/// pf::PrueflingVerbundStrategy::<strategy>-
/// TEXT; eine reale "merge"-Direktive MIT realem Slot ist damit erst kompilierbar, wenn die Materialisierung
/// (Enum-Wert Verbund2_Hybrid + Spezialisierung) landet -- heute 0 Produktions-Aufrufer (dormant, nur String-getestet).
/// Section-59-A(1) Verbund1_CeOnly ist die Abwesenheit einer Pruefling-Direktive (kein axes_default_lookup-merge /
/// self).
[[nodiscard]] inline std::string merge_mode_to_strategy(std::string const& merge_mode) {
    // "" (Default) und "replace" EXPLIZIT => ERSETZT-mit-Fallback (Verbund2). Kein Rest-Kollektor mehr.
    if (merge_mode.empty() || merge_mode == "replace") return "Verbund2_Replace";
    if (merge_mode == "union") return "Verbund3_Union";  // §59-A(3): kombinierte Union CE + Pruefling
    if (merge_mode == "merge") return "Verbund2_Hybrid"; // §59-A(2): CE + Pruefling HYBRID je Pruefling (!= Union)
    // A2.5-g5 (Review #15, Fix 16 / D-F3): FAIL-CLOSED statt Rest-Kollektor. Der alte Fallthrough
    // projizierte JEDES unbekannte Token still auf Verbund2_Replace -- fuer das Alt-Token "fulljoin"
    // (seit V-11R "union") war das eine stille SEMANTIK-INVERSION (Union -> Replace) auf dem
    // unvalidierten Direkt-Pfad; Doktrin-Asymmetrie zum fail-closed gebauten run_methodology_for_ids.
    // Die gueltige Menge kommt aus kExperimentAxisMergeModes (Array-Ableitung, keine Handliste).
    std::string menge;
    for (char const* m : kExperimentAxisMergeModes) {
        if (!menge.empty()) menge += ", ";
        menge += m;
    }
    throw std::invalid_argument{"merge_mode_to_strategy: unbekanntes merge-Token \"" + merge_mode +
                                "\" -- gueltig sind {" + menge +
                                "} (kExperimentAxisMergeModes; leer = replace-Default). Hinweis: "
                                "\"fulljoin\" ist seit V-11R \"union\"."};
}

/// profile_pruefling_identity(ep) -- die Pruefling-Identitaet der Merge-Phasen des Profils. Die erste <phase>, die
/// einen expliziten pruefling deklariert und NICHT als CacheEngine-self markiert ist (identity!="CacheEngine"/"self",
/// pruefling!="CacheEngine"/"self"), liefert den Pruefling; sonst "" (kein Pruefling = ce/self, Verbund1). Fork 3:
/// identity="CacheEngine"/self ist ein expliziter self-Pruefling => trage NICHTS zum Merge bei (leere Slot-Liste).
[[nodiscard]] inline std::string profile_pruefling_identity(mp_cx::ExperimentProfile const& ep) {
    auto const is_self = [](std::string const& s) { return s == "CacheEngine" || s == "self"; };
    for (auto const& ph : ep.phases) {
        if (is_self(ph.identity)) continue; // Fork 3: self-Phase traegt keinen Merge-Pruefling
        if (!ph.pruefling.empty() && !is_self(ph.pruefling)) return ph.pruefling;
    }
    return {}; // kein deklarierter Merge-Pruefling => ce/self (Verbund1, leere Slot-Liste)
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
