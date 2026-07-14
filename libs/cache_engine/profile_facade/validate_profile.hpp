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
//       "cacheline" = KF-3-Sonderzweig, separat erlaubt; "node_width" = C2/FF2-Sonderzweig, ebenso separat;
//       "alloc_hw" = F-B-Sonderzweig (NUMA/Page->allocator), ebenso separat).
//   (2) jeder <value>Y</value> dieser Achse ist ein gueltiger Achsen-Wert (= ein name() der EnabledStrategies/Registry
//       dieser Achse). Bei Fehler: nennt Achse + ungueltigen Wert + die gueltigen Werte.
//   (3) jeder <axis_sweep axis="X"> + jede <sota_series lebewesen="L"> referenziert eine deklarierte Achse / ein
//       deklariertes base_tier.
//   (4) GO-5 Fork 6 (2026-07-12): <sota_series fairness=".."> traegt nur die erlaubten Thesis-§sec:fairness-Modi
//       (common_denominator|native; leer = ungesetzt). Zwei Reihen desselben (lebewesen,merge) mit verschiedenem
//       fairness erzeugen bis zur DATEN-gated Kompositions-Pinnung DIESELBE binary_id → WARNUNG (kein Fehler).
//   (5) GO-5 Fork 1 (2026-07-12): jeder <dataset id akte_ref loader> ist FORMAT-plausibel — id nicht leer +
//       eindeutig, akte_ref endet auf ".test_data.xml" (die Akten sind die Single-Source, Fork 2/R2; die
//       DATEI-Existenz ist super-seitig und wird hier bewusst NICHT geprueft), loader nicht leer. Ein loader
//       ausserhalb der Repo-Loader-ids (kKnownDatasetLoaderIds) ist eine WARNUNG (die DatasetLoaderRegistry ist
//       laufzeit-offen), denn ein Tippfehler fiele beim spaeteren Mess-Konsum still auf den YCSB-Generator
//       zurueck (load_or_generate_ycsb-Fallback) — genau die stille Fehlerklasse, die --validate sichtbar macht.
//   (6) M-CE-12 (2026-07-13): jede <workloads>-id ist eine REAL existente load_profiles/-id. Die <workloads>-
//       Auswahl ist die AUTORITATIVE Achse-2-Auswahl (profile_run_facade.cpp:113-141); eine getippte id matcht
//       0 Lastprofile → der E4-Lauf bricht mit exit 4 ab (--validate soll genau DIESE stille Fehlerklasse
//       vorab sichtbar machen). Der Host reicht die real entdeckten ids (via wd::discover_load_profiles) als
//       known_workload_ids herein; ist die Menge leer (2-arg-Aufrufer / Test ohne Host-Enumeration), wird die
//       Pruefung uebersprungen (rueckwaerts-kompatibel). Leere <workloads> im Profil = "alle Lastprofile"
//       (legitim) → nichts zu pruefen; eine UNBEKANNTE id ist ein HARTER Fehler (Report-ok=false → Exit != 0).
//   (7) INC-3 Familie A (2026-07-14): jede <measurement_categories><category name="X"/> ist ein GUELTIGER
//       MeasurementCategory-Name. SINGLE-SOURCE der gueltigen Namen = kMeasurementAxisRegistry
//       (measurement_axis_registry.hpp; die 16 System-Kategorien) — NICHT hartkodiert. Kategorien sind eine
//       Spalten-PROJEKTION ueber die gemessenen Kategorien, KEINE Achse → binary_id-neutral (golden-Roundtrip
//       unberuehrt). Ein getippter Name (z.B. "LATENCY_P90") ist ein HARTER Fehler (er erzeugte sonst still eine
//       leere/falsche Projektion); eine mehrfach genannte Kategorie ist eine WARNUNG (redundante Spalte, nicht
//       fatal). Fehlt das Element = "alle 16 Kategorien" (legitim) → nichts zu pruefen (rueckwaerts-kompatibel).
//
// AUSGABE: klare Meldung je Fehler; bool-Ergebnis (true = OK). Der Host mappt true→Exit 0 (+ Zusammenfassung),
// false→Exit != 0. Pattern: Specification/Validator (read-only Gegen-Pruefung); C++23, header-only.

#include <builder/experiment_tree/experiment_tree.hpp>         // AxisLevel
#include <builder/experiment_tree/profile_to_tree.hpp>         // AxisRegistry (axis-ref → Werteliste)
#include <builder/experiment_tree/axis_path_serialization.hpp> // kCompositionAxisNames (die 19 Komposition-Achsen)
#include <cache_engine/measurement/measurement_axis_registry.hpp> // kMeasurementAxisRegistry (INC-3: Single-Source der 16 Kategorie-Namen)
#include "xml_config_parser/xml_config_parser.hpp" // ThesisProfile

#include <algorithm>
#include <map>
#include <ostream>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::thesis_lazy {

namespace ex = ::comdare::cache_engine::builder::experiment;
namespace cx = ::comdare::builder::xml;
namespace ms = ::comdare::cache_engine::measurement; // INC-3: kMeasurementAxisRegistry (Kategorie-Namen)

// ── Ergebnis-POD: bool + die Fehler-Liste (literal, fuer die Host-Ausgabe + Tests). ──
struct ProfileValidationResult {
    bool                     ok = true;
    std::vector<std::string> errors;   // je ungueltiger Eintrag eine klare Meldung
    std::vector<std::string> warnings; // nicht-fatale Hinweise (z.B. leere <value>-Liste = Registry-Expansion)
    std::size_t              axes_checked     = 0;
    std::size_t              values_checked   = 0;
    std::size_t              sweeps_checked   = 0;
    std::size_t              series_checked   = 0;
    std::size_t              datasets_checked = 0; // GO-5 Fork 1: geprueften <dataset>-Eintraege (0 = keine deklariert)
    std::size_t workloads_checked = 0; // M-CE-12: gepruefte <workloads>-ids (0 = keine / Pruefung uebersprungen)
    std::size_t categories_checked =
        0; // INC-3 Familie A: gepruefte <measurement_categories>-Namen (0 = keine deklariert)
};

// ── GO-5 Fork 6 (Thesis §sec:fairness): die zwei erlaubten Fairness-Modi einer <sota_series>. ──
inline constexpr char const* kFairnessModes[] = {"common_denominator", "native"};

// ── GO-5 Fork 1: die im Repo existenten DatasetLoaderRegistry-loader_ids (Quelle: die Selbst-
//    Registrierungen in libs/common/measurement/dataset_loader/include/.../loaders/*.hpp —
//    string_corpus_loader.hpp + sosd_uint64_loader.hpp). Die Registry ist laufzeit-OFFEN (weitere
//    Loader registrierbar) → ein fremder loader ist WARNUNG, kein Fehler (s. Kopf-Doku (5)). ──
inline constexpr char const* kKnownDatasetLoaderIds[] = {"string_corpus", "sosd_uint64"};

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
/// EnabledStrategies, vom Host via axis_registry_from_levels(build_all_axis_levels()) hereingereicht).
/// `known_workload_ids` = die REAL vorhandenen load_profiles/-ids (aus wd::discover_load_profiles), vom Host
/// hereingereicht; leer = die <workloads>-Pruefung (6) wird uebersprungen (rueckwaerts-kompatibel). Prueft
/// (1) Achsen-Namen (2) Achsen-Werte (3) axis_sweep- + sota_series-Referenzen (4) fairness (5) datasets
/// (6) <workloads>-ids. Schreibt KEINE Datei, baut KEINE DLL, misst NICHTS. Gibt das Ergebnis-POD zurueck;
/// der Caller (Host) druckt + mappt auf den Exit-Code.
[[nodiscard]] inline ProfileValidationResult validate_profile(cx::ThesisProfile const&     tp,
                                                              ex::AxisRegistry const&      registry,
                                                              std::set<std::string> const& known_workload_ids = {}) {
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
        if (ax.ref == "node_width") { // C2/FF2-Sonderzweig (compile-time Knoten-Breite in Cache-Lines) — separat.
            r.warnings.push_back("axis ref=\"node_width\": C2/FF2-Sonderzweig (Knoten-Breiten-Unterachse), "
                                 "Werte (width_in_lines) nicht gegen die Achsen-Registry geprueft.");
            continue;
        }
        if (ax.ref == "alloc_hw") { // F-B-Sonderzweig (compile-time NUMA/Page->allocator) — separat.
            r.warnings.push_back("axis ref=\"alloc_hw\": F-B-Sonderzweig (NUMA/Page-Unterachse), "
                                 "Werte (numa_node/page) nicht gegen die Achsen-Registry geprueft.");
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

    // ── (3b): jede <sota_series lebewesen="L"> referenziert ein deklariertes <base_tier id="L">.
    //    (4) GO-5 Fork 6: fairness (falls gesetzt) ∈ kFairnessModes; fairness-Varianten desselben
    //    (lebewesen,merge)-Paars sind bis zur DATEN-gated Kompositions-Pinnung binary_id-GLEICH → WARNUNG. ──
    std::map<std::string, std::string> fairness_by_pair; // "lebewesen|merge" → erster fairness-Wert
    for (auto const& ss : tp.sota_series) {
        ++r.series_checked;
        if (tier_ids.find(ss.lebewesen) == tier_ids.end()) {
            r.ok = false;
            std::vector<std::string> ids(tier_ids.begin(), tier_ids.end());
            r.errors.push_back("UNBEKANNTES Lebewesen <sota_series id=\"" + ss.id + "\" lebewesen=\"" + ss.lebewesen +
                               "\">: kein deklariertes <base_tier>. Deklarierte base_tiers = " + preview_values(ids));
        }
        if (!ss.fairness.empty()) {
            bool known = false;
            for (char const* m : kFairnessModes) known = known || (ss.fairness == m);
            if (!known) {
                r.ok = false;
                r.errors.push_back("UNGUELTIGER Fairness-Modus <sota_series id=\"" + ss.id + "\" lebewesen=\"" +
                                   ss.lebewesen + "\" fairness=\"" + ss.fairness +
                                   "\">: erlaubt sind common_denominator|native (Thesis §sec:fairness) oder "
                                   "weglassen (ungesetzt).");
            }
        }
        auto const [it, inserted] = fairness_by_pair.emplace(ss.lebewesen + "|" + ss.merge, ss.fairness);
        if (!inserted && it->second != ss.fairness) {
            r.warnings.push_back(
                "sota_series lebewesen=\"" + ss.lebewesen + "\" merge=\"" + ss.merge +
                "\": mehrere fairness-Varianten desselben (lebewesen,merge)-Paars erzeugen bis zur "
                "DATEN-gated Kompositions-Pinnung (GO-5 Fork 6) DIESELBE binary_id/DLL — die Paesse "
                "teilen sich ein per-Binary-Verzeichnis (Resume unterscheidet sie nur ueber den Stamp).");
        }
    }

    // ── (5) GO-5 Fork 1: <datasets>-FORMAT-Checks (Datei-Existenz ist super-seitig — hier bewusst NICHT). ──
    std::set<std::string> seen_dataset_ids;
    for (auto const& d : tp.datasets) {
        ++r.datasets_checked;
        if (d.id.empty()) {
            r.ok = false;
            r.errors.push_back("DATASET ohne id: <dataset akte_ref=\"" + d.akte_ref +
                               "\"> braucht einen eindeutigen Kurznamen (id).");
        } else if (!seen_dataset_ids.insert(d.id).second) {
            r.ok = false;
            r.errors.push_back("DOPPELTE Dataset-id <dataset id=\"" + d.id +
                               "\">: die id muss je Profil eindeutig sein (Referenz-Schluessel).");
        }
        static constexpr std::string_view kAkteSuffix = ".test_data.xml";
        bool const                        suffix_ok =
            d.akte_ref.size() > kAkteSuffix.size() &&
            d.akte_ref.compare(d.akte_ref.size() - kAkteSuffix.size(), kAkteSuffix.size(), kAkteSuffix) == 0;
        if (!suffix_ok) {
            r.ok = false;
            r.errors.push_back("UNGUELTIGE Akten-Referenz <dataset id=\"" + d.id + "\" akte_ref=\"" + d.akte_ref +
                               "\">: muss auf eine test_data-AKTE (…<name>.test_data.xml) zeigen — die Akten "
                               "sind die Single-Source (GO-5 Fork 2/R2; Datei-Existenz prueft die super-Seite).");
        }
        if (d.loader.empty()) {
            r.ok = false;
            r.errors.push_back("DATASET ohne loader: <dataset id=\"" + d.id +
                               "\"> braucht eine DatasetLoaderRegistry-loader_id (z.B. string_corpus).");
        } else {
            bool known = false;
            for (char const* l : kKnownDatasetLoaderIds) known = known || (d.loader == l);
            if (!known) {
                r.warnings.push_back("dataset id=\"" + d.id + "\" loader=\"" + d.loader +
                                     "\": keine Repo-Loader-id (string_corpus|sosd_uint64). Die Registry ist "
                                     "laufzeit-offen, ABER ein Tippfehler fiele beim Mess-Konsum still auf den "
                                     "YCSB-Generator zurueck (load_or_generate_ycsb-Fallback).");
            }
        }
    }

    // ── (7) INC-3 Familie A: <measurement_categories>-Namen-Check. SINGLE-SOURCE der gueltigen Namen =
    //    kMeasurementAxisRegistry (die 16 System-Kategorien, measurement_axis_registry.hpp) — NICHT hartkodiert.
    //    Kategorien sind eine Spalten-PROJEKTION, keine Achse → binary_id-neutral. Unbekannter Name = HARTER
    //    Fehler (sonst stille leere/falsche Projektion); mehrfache Nennung = WARNUNG (redundante Spalte). ──
    if (!tp.measurement_categories.empty()) {
        std::set<std::string> valid_categories;
        for (auto const& info : ms::kMeasurementAxisRegistry) valid_categories.insert(std::string{info.name});
        std::set<std::string> seen_categories;
        for (auto const& cat : tp.measurement_categories) {
            ++r.categories_checked;
            if (valid_categories.find(cat) == valid_categories.end()) {
                r.ok = false;
                std::vector<std::string> const valid(valid_categories.begin(), valid_categories.end());
                r.errors.push_back("UNGUELTIGE Mess-Kategorie <measurement_categories><category name=\"" + cat +
                                   "\">: kein MeasurementCategory-Name (Single-Source kMeasurementAxisRegistry, "
                                   "die 16 System-Kategorien). Gueltige Kategorien = " +
                                   preview_values(valid));
            } else if (!seen_categories.insert(cat).second) {
                r.warnings.push_back("category name=\"" + cat +
                                     "\": mehrfach deklariert — redundante Spalten-Projektion (nicht fatal).");
            }
        }
    }

    // ── (6) M-CE-12: jede <workloads>-id ist eine REAL existente load_profiles/-id. Nur wenn der Host die
    //    entdeckten ids hereinreicht (known_workload_ids nicht leer) — leer = Pruefung uebersprungen
    //    (rueckwaerts-kompatibel: 2-arg-Aufrufer/Tests ohne Host-Enumeration). Leere <workloads> im Profil =
    //    "alle Lastprofile" (legitim) → nichts zu pruefen. Eine unbekannte id ist ein HARTER Fehler: sie matcht
    //    0 Lastprofile, wodurch der spaetere E4-Lauf mit exit 4 abbricht (Achse 2 darf nicht still entfallen). ──
    if (!known_workload_ids.empty()) {
        std::vector<std::string> const valid_wl(known_workload_ids.begin(), known_workload_ids.end());
        for (auto const& w : tp.workloads) {
            ++r.workloads_checked;
            if (known_workload_ids.find(w) == known_workload_ids.end()) {
                r.ok = false;
                r.errors.push_back("UNBEKANNTE Workload-id <workloads>… " + w +
                                   " …</workloads>: keine load_profiles/-id. Die <workloads>-Auswahl ist die "
                                   "AUTORITATIVE Achse-2-Auswahl; eine unbekannte id matcht 0 Lastprofile → der "
                                   "E4-Lauf braeche mit exit 4 ab (Achse 2 darf nicht still entfallen). "
                                   "Gueltige ids = " +
                                   preview_values(valid_wl));
            }
        }
    }

    return r;
}

/// print_validation_report — druckt das Ergebnis menschenlesbar (Host nutzt es; der Test prueft die Literale).
inline void print_validation_report(ProfileValidationResult const& r, cx::ThesisProfile const& tp, std::ostream& os) {
    os << "=== PROFIL-VALIDAT (rein-lesend; KEIN DLL-Bau, KEINE Messung) ===\n";
    os << "  Profil id=" << tp.id << " schema_version=" << tp.schema_version << "\n";
    os << "  geprueft: " << r.axes_checked << " Achsen, " << r.values_checked << " Werte, " << r.sweeps_checked
       << " axis_sweeps, " << r.series_checked << " sota_series";
    // M-CE-12: workloads NUR ausgeben, wenn geprueft (Host reichte known_workload_ids herein) — die --validate-
    // Ausgabe der 2-arg-Aufrufer (Tests ohne Host-Enumeration) bleibt byte-identisch.
    if (r.workloads_checked > 0) os << ", " << r.workloads_checked << " workloads";
    // GO-5 Fork 1: datasets NUR ausgeben, wenn deklariert — die --validate-Ausgabe bestehender Profile
    // (ohne <datasets>) bleibt byte-identisch (Default-Verhaltens-Gate dieses Increments).
    if (r.datasets_checked > 0) os << ", " << r.datasets_checked << " datasets";
    // INC-3 Familie A: measurement_categories NUR ausgeben, wenn deklariert — die --validate-Ausgabe
    // bestehender Profile (ohne <measurement_categories>) bleibt byte-identisch (Default-Verhaltens-Gate).
    if (r.categories_checked > 0) os << ", " << r.categories_checked << " measurement_categories";
    os << "\n";
    for (auto const& w : r.warnings) os << "  [HINWEIS] " << w << "\n";
    for (auto const& e : r.errors) os << "  [FEHLER]  " << e << "\n";
    if (r.ok)
        os << "VALIDAT OK: das Profil ist gegen die AxisRegistry/EnabledStrategies konsistent.\n";
    else
        os << "VALIDAT FEHLGESCHLAGEN: " << r.errors.size() << " Fehler — Profil NICHT baubar (Abbruch vor Bau).\n";
}

} // namespace comdare::cache_engine::thesis_lazy
