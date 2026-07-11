#pragma once
// #169(A) Nutzerfreundlichkeit (User 2026-06-19) — validate_profile: das REIN-LESENDE Profil-Validat gegen die
// AxisRegistry/EnabledStrategies, BEVOR teuer gebaut/gemessen wird.
//
// PROBLEM: ein getipptes comdare_thesis_profile (z.B. <value>node_4</value> statt node4, oder eine unbekannte
// <axis ref="…">) faellt heute erst zur LAUFZEIT auf (parse OK, aber build_axis_levels selektiert die falsche/keine
// Achse → falsche Matrix ODER spaeter Source-Fehler) — nach langer Wartezeit. validate_profile prueft das Profil
// SOFORT (kein DLL-Bau, keine Messung) gegen die im CODE bekannten gueltigen Achsen-Namen + Achsen-Werte.
//
// SINGLE-SOURCE der gueltigen Werte (NICHT hartkodiert): die AxisRegistry (= axis-ref → volle name()-Werteliste)
// stammt aus den REALEN EnabledStrategies/Registry-Listen der Achsen — der Host baut sie via
// build_all_axis_levels() (registry_to_axis_levels.hpp; reflect_names<TopicConfigSet::StaticAxisVariants*>) und
// reicht sie HIER herein. validate_profile selbst trifft KEINE Achsen-Wahl + inkludiert KEINE schweren Umbrella-
// Header → es ist reine, engine-agnostische Pruef-Logik ueber die hereingereichte Registry. (Die binary_id-Achsen-
// Namen kommen aus axis_path_serialization.hpp:27 kCompositionAxisNames + denselben Registry-Keys.)
//
// PRUEFUNGEN (Plan #169(A)):
//   (1) jeder <axis ref="X"> in <permute_axes> ist ein bekannter Achsen-Name (Registry-Key / kCompositionAxisNames;
//       "cacheline" = KF-3-Sonderzweig, separat erlaubt).
//   (2) jeder <value>Y</value> dieser Achse ist ein gueltiger Achsen-Wert (= ein name() der EnabledStrategies/Registry
//       dieser Achse). Bei Fehler: nennt Achse + ungueltigen Wert + die gueltigen Werte.
//   (3) jeder <axis_sweep axis="X"> + jede <sota_series lebewesen="L"> referenziert eine deklarierte Achse / ein
//       deklariertes base_tier.
//
// AUSGABE: klare Meldung je Fehler; bool-Ergebnis (true = OK). Der Host mappt true→Exit 0 (+ Zusammenfassung),
// false→Exit != 0. Pattern: Specification/Validator (read-only Gegen-Pruefung); C++23, header-only.

#include <builder/experiment_tree/experiment_tree.hpp>         // AxisLevel
#include <builder/experiment_tree/profile_to_tree.hpp>         // AxisRegistry (axis-ref → Werteliste)
#include <builder/experiment_tree/axis_path_serialization.hpp> // kCompositionAxisNames (die 19 Komposition-Achsen)
#include "xml_config_parser/xml_config_parser.hpp"             // ThesisProfile

#include <algorithm>
#include <map>
#include <ostream>
#include <set>
#include <string>
#include <vector>

namespace comdare::cache_engine::thesis_lazy {

namespace ex = ::comdare::cache_engine::builder::experiment;
namespace cx = ::comdare::builder::xml;

// ── Ergebnis-POD: bool + die Fehler-Liste (literal, fuer die Host-Ausgabe + Tests). ──
struct ProfileValidationResult {
    bool                     ok = true;
    std::vector<std::string> errors;   // je ungueltiger Eintrag eine klare Meldung
    std::vector<std::string> warnings; // nicht-fatale Hinweise (z.B. leere <value>-Liste = Registry-Expansion)
    std::size_t              axes_checked   = 0;
    std::size_t              values_checked = 0;
    std::size_t              sweeps_checked = 0;
    std::size_t              series_checked = 0;
};

/// axis_registry_from_levels — formt die AxisLevel-Liste (z.B. build_all_axis_levels()) in eine AxisRegistry
/// (axis-Name → gueltige Werte). Das ist der READ-ONLY Brueckenkopf: build_all_axis_levels() reflektiert die
/// REALEN EnabledStrategies (reflect_names<…StaticAxisVariants*>), HIER nur als map umgehaengt — keine Wahl.
[[nodiscard]] inline ex::AxisRegistry axis_registry_from_levels(std::vector<ex::AxisLevel> const& levels) {
    ex::AxisRegistry reg;
    for (auto const& l : levels) reg[l.axis] = l.values; // axis → volle name()-Liste (EnabledStrategies)
    return reg;
}

// ── Hilfs: gueltige Werte-Vorschau (max 12, sonst "… (+N)") fuer eine lesbare Fehlermeldung. ──
[[nodiscard]] inline std::string preview_values(std::vector<std::string> const& vals) {
    std::string       s;
    std::size_t const cap = (std::min<std::size_t>)(vals.size(), 12);
    for (std::size_t i = 0; i < cap; ++i) {
        if (i) s += ", ";
        s += vals[i];
    }
    if (vals.size() > cap) s += ", … (+" + std::to_string(vals.size() - cap) + ")";
    return s;
}

/// validate_profile — DIE reine Pruef-Logik (read-only). `registry` = axis-Name → gueltige Werte (aus den
/// EnabledStrategies, vom Host via axis_registry_from_levels(build_all_axis_levels()) hereingereicht). Prueft
/// (1) Achsen-Namen (2) Achsen-Werte (3) axis_sweep- + sota_series-Referenzen. Schreibt KEINE Datei, baut KEINE
/// DLL, misst NICHTS. Gibt das Ergebnis-POD zurueck; der Caller (Host) druckt + mappt auf den Exit-Code.
[[nodiscard]] inline ProfileValidationResult validate_profile(cx::ThesisProfile const& tp,
                                                              ex::AxisRegistry const&  registry) {
    ProfileValidationResult r;

    // Menge der bekannten Achsen-Namen: Registry-Keys (EnabledStrategies) ∪ kCompositionAxisNames (die 19 Slots).
    std::set<std::string> known_axes;
    for (auto const& [axis, _] : registry) known_axes.insert(axis);
    for (auto const& a : ex::kCompositionAxisNames) known_axes.insert(std::string{a});

    // ── (1)+(2): jede <axis ref> bekannt + jeder <value> gueltig. ──
    for (auto const& ax : tp.permute_axes) {
        ++r.axes_checked;
        if (ax.ref == "cacheline") { // KF-3-Sonderzweig (compile-time Cache-Line-Unterachse) — separat.
            r.warnings.push_back("axis ref=\"cacheline\": KF-3-Sonderzweig (Cache-Line-Unterachse), "
                                 "Werte (line_size/alignment/sw_hint) nicht gegen die Achsen-Registry geprueft.");
            continue;
        }
        // (1) Achsen-Name bekannt?
        if (known_axes.find(ax.ref) == known_axes.end()) {
            r.ok = false;
            r.errors.push_back("UNBEKANNTE Achse <axis ref=\"" + ax.ref +
                               "\">: kein Achsen-Name der Registry/"
                               "EnabledStrategies. Gueltige Achsen = " +
                               preview_values([&] {
                                   std::vector<std::string> a(known_axes.begin(), known_axes.end());
                                   return a;
                               }()));
            continue; // ohne bekannte Achse koennen die Werte nicht geprueft werden
        }
        // (2) Achsen-Werte gueltig? Gueltige Werte aus der Registry dieser Achse.
        auto it = registry.find(ax.ref);
        if (it == registry.end()) {
            // Achse ist ein bekannter Slot (kCompositionAxisNames), aber NICHT in der hereingereichten Registry
            // (z.B. Host reichte nur eine Teilmenge). Nicht-fatal: Werte koennen nicht geprueft werden.
            r.warnings.push_back("axis ref=\"" + ax.ref +
                                 "\": bekannter Slot, aber keine Werteliste in der "
                                 "hereingereichten Registry — <value>-Pruefung uebersprungen.");
            continue;
        }
        std::vector<std::string> const& valid = it->second;
        if (ax.values.empty()) {
            // Leere <value>-Liste = build_axis_levels expandiert die volle Registry-Liste (legitim) → nur Hinweis.
            r.warnings.push_back("axis ref=\"" + ax.ref + "\": keine <value> — die volle Registry-Liste (" +
                                 std::to_string(valid.size()) + " Werte) wird expandiert.");
            continue;
        }
        for (auto const& v : ax.values) {
            ++r.values_checked;
            if (std::find(valid.begin(), valid.end(), v) == valid.end()) {
                r.ok = false;
                r.errors.push_back("UNGUELTIGER Wert <axis ref=\"" + ax.ref + "\"><value>" + v +
                                   "</value>: kein "
                                   "name() der EnabledStrategies dieser Achse. Gueltige Werte = " +
                                   preview_values(valid));
            }
        }
    }

    // ── Menge der im Profil DEKLARIERTEN Achsen (fuer axis_sweep-Referenz-Pruefung). ──
    std::set<std::string> declared_axes;
    for (auto const& ax : tp.permute_axes) declared_axes.insert(ax.ref);

    // ── (3a): jeder <axis_sweep axis="X"> referenziert eine deklarierte (bzw. bekannte) Achse. ──
    for (auto const& sw : tp.axis_sweeps) {
        ++r.sweeps_checked;
        bool const is_declared = declared_axes.find(sw.axis) != declared_axes.end();
        bool const is_known    = known_axes.find(sw.axis) != known_axes.end();
        if (!is_declared && !is_known) {
            r.ok = false;
            r.errors.push_back("UNBEKANNTE Sweep-Achse <axis_sweep axis=\"" + sw.axis +
                               "\">: weder im Profil "
                               "<permute_axes> deklariert noch ein bekannter Achsen-Name der Registry.");
        } else if (!is_declared) {
            r.warnings.push_back("axis_sweep axis=\"" + sw.axis +
                                 "\": bekannte Achse, aber NICHT in "
                                 "<permute_axes> deklariert (Sweep nutzt die Baseline/eigene Sweep-View).");
        }
    }

    // ── Menge der deklarierten base_tier-ids (fuer sota_series-lebewesen-Pruefung). ──
    std::set<std::string> tier_ids;
    for (auto const& t : tp.base_tiers) tier_ids.insert(t.id);

    // ── (3b): jede <sota_series lebewesen="L"> referenziert ein deklariertes <base_tier id="L">. ──
    for (auto const& ss : tp.sota_series) {
        ++r.series_checked;
        if (tier_ids.find(ss.lebewesen) == tier_ids.end()) {
            r.ok = false;
            std::vector<std::string> ids(tier_ids.begin(), tier_ids.end());
            r.errors.push_back("UNBEKANNTES Lebewesen <sota_series id=\"" + ss.id + "\" lebewesen=\"" + ss.lebewesen +
                               "\">: kein deklariertes <base_tier>. Deklarierte base_tiers = " + preview_values(ids));
        }
    }

    return r;
}

/// print_validation_report — druckt das Ergebnis menschenlesbar (Host nutzt es; der Test prueft die Literale).
inline void print_validation_report(ProfileValidationResult const& r, cx::ThesisProfile const& tp, std::ostream& os) {
    os << "=== PROFIL-VALIDAT (rein-lesend; KEIN DLL-Bau, KEINE Messung) ===\n";
    os << "  Profil id=" << tp.id << " schema_version=" << tp.schema_version << "\n";
    os << "  geprueft: " << r.axes_checked << " Achsen, " << r.values_checked << " Werte, " << r.sweeps_checked
       << " axis_sweeps, " << r.series_checked << " sota_series\n";
    for (auto const& w : r.warnings) os << "  [HINWEIS] " << w << "\n";
    for (auto const& e : r.errors) os << "  [FEHLER]  " << e << "\n";
    if (r.ok)
        os << "VALIDAT OK: das Profil ist gegen die AxisRegistry/EnabledStrategies konsistent.\n";
    else
        os << "VALIDAT FEHLGESCHLAGEN: " << r.errors.size() << " Fehler — Profil NICHT baubar (Abbruch vor Bau).\n";
}

} // namespace comdare::cache_engine::thesis_lazy
