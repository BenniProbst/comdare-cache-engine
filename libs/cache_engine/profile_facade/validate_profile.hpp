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
#include <cache_engine/measurement/system_axis.hpp> // kAllMeasurementCategories (INC-D: Single-Source der 16 Kategorie-Enums)
#include <cache_engine/measurement/optimization_level_sub_axis.hpp> // kAllOptLevelIds (Single-Source der opt_level-ids)
#include <cache_engine/measurement/simd_sub_axis.hpp>               // kAllSimdIds (Single-Source der simd-ids, F-SIMD)
#include <cache_engine/measurement/extension_hardware_family_axis.hpp> // GN-1: aktiver extension_hardware-Familien-Knoten
#include <anatomy/pruefling_merge.hpp>             // MergeStrategy-Enum (INC-D: die 3 Kompositionalen Joins)
#include "xml_config_parser/xml_config_parser.hpp" // ThesisProfile / ExperimentProfile
#include "xml_config_parser/xml_reader.hpp"        // INC-D: common-DOM zum Lesen der Registry-XML (kein regex)

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <ostream>
#include <set>
#include <sstream>
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
    std::size_t opt_levels_checked = 0; // GN-3/§32-F4: gepruefte <system_axes><compiler><opt_level> (0 = keine)
    std::size_t simd_checked       = 0; // GN-3: gepruefte <system_axes><extension_hardware><simd> (0 = keine)
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

    // ── (8) GN-3 (§33 Systembeweis-Traeger, 2026-07-19): <system_axes> HART gegen die System-Achsen-Klassen.
    //    opt_level: nur die IEEE-754-DETERMINISTISCHEN Stufen {O0,O1,O2,O3} sind im golden-Mess-Kanal zulaessig;
    //    Ofast ist REJECT (§32-F4) — es bricht IEEE-754-/Run-to-Run-Determinismus und den CRC64-golden-Anker.
    //    simd: gegen die SimdSubAxis-Optionen (kAllSimdIds = no_extension/avx2/avx512). Beide sind system_config →
    //    binary_id-NEUTRAL (Provenienz build_version/Sidecar, NIE binary_id, NIE N); hier nur die Wert-Gueltigkeit.
    //    LEER = kein <system_axes> = heutiges Verhalten byte-identisch (nichts zu pruefen, rueckwaerts-kompatibel). ──
    {
        // §32-F4-ANKER: O0..O3 IEEE-754-deterministisch, Ofast NICHT. Wechselt eine Achsen-Klasse ihre Semantik,
        // bricht DIESER static_assert (nicht erst der Laufzeit-Check) — die Reject-Liste bleibt Single-Source der
        // Klassen (kein Literal-Duplikat der Determinismus-Politik).
        static_assert(ms::OptO0Option::is_ieee754_deterministic() && ms::OptO1Option::is_ieee754_deterministic() &&
                          ms::OptO2Option::is_ieee754_deterministic() && ms::OptO3Option::is_ieee754_deterministic() &&
                          !ms::OptOfastOption::is_ieee754_deterministic(),
                      "§32-F4: O0..O3 sind IEEE-754-deterministisch, Ofast bricht den Determinismus — golden-Anker.");
        static constexpr std::string_view kThesisOptLevels[] = {
            ms::OptO0Option::opt_level_id(), ms::OptO1Option::opt_level_id(), ms::OptO2Option::opt_level_id(),
            ms::OptO3Option::opt_level_id()};
        for (auto const& lvl : tp.compiler.opt_levels) {
            ++r.opt_levels_checked;
            bool const zulaessig =
                std::find(std::begin(kThesisOptLevels), std::end(kThesisOptLevels), lvl) != std::end(kThesisOptLevels);
            if (zulaessig) continue;
            r.ok = false;
            if (lvl == ms::OptOfastOption::opt_level_id()) {
                r.errors.push_back("UNZULAESSIGES <system_axes><compiler><opt_level value=\"Ofast\">: Ofast ist im "
                                   "golden-Mess-Kanal REJECT (§32-F4) — es bricht IEEE-754-/Run-to-Run-Determinismus "
                                   "und den CRC64-golden-Anker. Erlaubt sind O0/O1/O2/O3.");
            } else {
                r.errors.push_back("UNGUELTIGE <system_axes><compiler><opt_level value=\"" + lvl +
                                   "\">: kein opt_level-Wert (optimization_level_sub_axis.hpp). Erlaubt (§32-F4, "
                                   "deterministisch) = O0, O1, O2, O3.");
            }
        }
        for (auto const& s : tp.extension_hardware.simd_options) {
            ++r.simd_checked;
            bool const known = std::find(ms::kAllSimdIds.begin(), ms::kAllSimdIds.end(), s) != ms::kAllSimdIds.end();
            if (!known) {
                r.ok = false;
                r.errors.push_back("UNGUELTIGE <system_axes><extension_hardware><simd value=\"" + s +
                                   "\">: kein simd-Wert der SimdSubAxis-Optionen (simd_sub_axis.hpp). Erlaubt = "
                                   "no_extension, avx2, avx512.");
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
    // GN-3/§32-F4: system_axes (opt_level/simd) NUR ausgeben, wenn deklariert — die --validate-Ausgabe bestehender
    // Profile (ohne <system_axes>) bleibt byte-identisch (Default-Verhaltens-Gate, wie datasets/categories).
    if (r.opt_levels_checked > 0) os << ", " << r.opt_levels_checked << " opt_levels";
    if (r.simd_checked > 0) os << ", " << r.simd_checked << " simd";
    os << "\n";
    for (auto const& w : r.warnings) os << "  [HINWEIS] " << w << "\n";
    for (auto const& e : r.errors) os << "  [FEHLER]  " << e << "\n";
    if (r.ok)
        os << "VALIDAT OK: das Profil ist gegen die AxisRegistry/EnabledStrategies konsistent.\n";
    else
        os << "VALIDAT FEHLGESCHLAGEN: " << r.errors.size() << " Fehler — Profil NICHT baubar (Abbruch vor Bau).\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// INC-D (2026-07-14): validate_experiment_profile — das REIN-LESENDE Validat des comdare_experiment-
// Profils (ExperimentProfile, common-Schicht) gegen die cache_engine-Wahrheiten. Muster = validate_profile
// (Specification/Validator; read-only). Die common-Schicht traegt NUR Strings; DIESE cache_engine-Schicht
// loest sie gegen die MergeStrategy-Enum + die Registry-XML + kAllMeasurementCategories auf (Baseline-
// Layering: common referenziert NIE cache_engine).
//
// PRUEFUNGEN:
//   (1) GENAU 2 <execution_engines><engine> (ee_ce + ee_prt).
//   (2) mindestens 1 <phases><phase>.
//   (3) jede phase.merge ∈ MergeStrategy-Enum (pruefling_merge.hpp: Stufe1_CeOnly/Stufe2_.../Stufe3_...).
//   (4) die je-engine referenzierten Registry-Dateien existieren (registry_dir/<registry>) UND sind als
//       comdare_axis_registry lesbar (F28, WP-3 2026-07-16: unlesbar war vorher nur WARNUNG — eine korrupte
//       Registry validierte 'ok' mit still uebersprungener (5)-Pruefung; jetzt HARTER Fehler). Zusaetzlich
//       engine-Attribut-Abgleich (F22/F28): die Registry einer bekannten engine-id muss das passende
//       Wurzel-engine-Attribut tragen (ee_ce→"cache_engine", ee_prt→"prt_art"; Copy-Paste-Mismatch=FEHLER);
//       eine ZWEITE Registry mit engine="cache_engine" ist ein FEHLER (vorher last-wins: sie ueberschrieb
//       still die echte ce-Registry im (5)-Check).
//   (5) jede <axes_default_lookup> allowed_variants ⊆ der `baustein name`-Werte der ce-Registry
//       (Wurzel engine="cache_engine") fuer die Achse `ref`; unbekannte Achse = Fehler.
//   (6) jede <measurement_categories> category ∈ kAllMeasurementCategories (16, measurement_category.hpp) —
//       Single-Source ueber measurement_axis_registry::axis_info(cat).name; Duplikat = WARNUNG.
//   (7) F22 (WP-3, 2026-07-16): engine-id-EINDEUTIGKEIT — eine doppelte <engine id=..> ist ein FEHLER
//       (Referenz-Schluessel der Phasen; vorher ungeprueft).
//   (8) F22 (WP-3, 2026-07-16): jedes gesetzte phase.pruefling ∈ den deklarierten <lebewesen><tier id=..>
//       (vorher ungeprueft — ein Tippfehler validierte ok).
//   (9) F12-Validator-Haelfte (WP-3, 2026-07-16): jedes <op_types>-Token ∈ {OP-1..OP-6} (HART) und ein
//       LEERES/fehlendes <op_types> ist ein FEHLER — das XSD deklariert das Element als required mit
//       enumeriertem OpTypeType (super Code/test_data_xml/experiment_schema.xsd:33/168-177). Vorher
//       ungeprueft: ein Bogus-Token mislabelte die Messzeile still (phase@<bogus> misst den Basis-Workload),
//       ein leeres Element erfand still "OP-1". Die 6 Token spiegeln die XSD-Enumeration; der super-Treiber
//       fuehrt dieselbe Tabelle (02_messung_driver/op_type_filter.hpp kOpTypeTable) — ce darf super nicht
//       inkludieren (Baseline-Layering), daher hier der XSD-Kontrakt als Single-Source zitiert.
//   HINWEIS K7 (E11-gated, BEWUSST NICHT hier): Kardinalitaets-/Struktur-Regeln, die vom E11-Entscheid
//   (Phasen-Kardinalitaet/Serie) abhaengen — ==3-Phasen-Haertung, engine/engines-XOR, phase.engine(s)-
//   Referenz-Checks gegen die engine-ids, merge-Enum-Kardinalitaet je Phase (F19/F21) — bleiben offen,
//   bis der User-Fork C (F21) entschieden ist.
// registry_dir leer = (4)+(5) uebersprungen (rein strukturelle Validierung; der Host reicht das
// Registry-Verzeichnis herein — analog validate_profile::known_workload_ids).
// ─────────────────────────────────────────────────────────────────────────────

namespace pm = ::comdare::cache_engine::anatomy::pruefling;

// Die 3 gueltigen MergeStrategy-Enum-Werte (Single-Source = das Enum selbst).
inline constexpr pm::MergeStrategy kExperimentMergeStrategies[] = {
    pm::MergeStrategy::Stufe1_CeOnly, pm::MergeStrategy::Stufe2_PrueflingReplace, pm::MergeStrategy::Stufe3_FullJoin};

// merge_strategy_name — Enum -> kanonischer XML-String. switch OHNE default: -Wswitch faengt Enum-Drift
// (eine 4. Stufe muss hier + im XSD nachgezogen werden), statt still eine gueltige Stufe zu verwerfen.
[[nodiscard]] inline std::string_view merge_strategy_name(pm::MergeStrategy s) {
    switch (s) {
        case pm::MergeStrategy::Stufe1_CeOnly: return "Stufe1_CeOnly";
        case pm::MergeStrategy::Stufe2_PrueflingReplace: return "Stufe2_PrueflingReplace";
        case pm::MergeStrategy::Stufe3_FullJoin: return "Stufe3_FullJoin";
    }
    return "";
}

// ── Ergebnis-POD: bool + Fehler-/Warnungs-Liste (literal, fuer Host-Ausgabe + Tests). ──
struct ExperimentValidationResult {
    bool                     ok = true;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::size_t              engines_checked    = 0;
    std::size_t              phases_checked     = 0;
    std::size_t              variants_checked   = 0; // gepruefte allowed_variants (0 = registry_dir leer / keine)
    std::size_t              categories_checked = 0; // gepruefte <category>-Namen (0 = keine deklariert)
    std::size_t              workloads_checked  = 0; // Bruecke-I1/M-CE-12: gepruefte <workloads>-ids (0 = known leer)
    std::size_t opt_levels_checked = 0; // opt-f/A3: gepruefte <system_axes><compiler><opt_level> (0 = keine)
    std::size_t simd_checked       = 0; // opt-f/A3: gepruefte <system_axes><extension_hardware><simd> (0 = keine)
};

// ── Inhalt einer comdare_axis_registry.xml: engine-Attr + axis-id -> {baustein name}. ──
struct RegistryContents {
    std::string                                  engine;     // Wurzel @engine ("cache_engine" / "prt_art")
    std::map<std::string, std::set<std::string>> axis_names; // axis-id -> {baustein name}
};

// read_axis_registry — liest EINE comdare_axis_registry.xml ueber den common-DOM (KEIN regex/tinyxml2).
// nullopt bei Lese-/Parse-Fehler oder falschem Wurzel-Tag.
[[nodiscard]] inline std::optional<RegistryContents> read_axis_registry(std::filesystem::path const& registry_file) {
    std::ifstream in{registry_file};
    if (!in) return std::nullopt;
    std::ostringstream ss;
    ss << in.rdbuf();
    auto root = ::comdare::common::xml::parse_document(ss.str());
    if (!root || root->tag != "comdare_axis_registry") return std::nullopt;
    RegistryContents rc;
    rc.engine = root->attr("engine");
    for (auto const* axis : root->children_named("axis")) {
        auto& names = rc.axis_names[axis->attr("id")];
        for (auto const* b : axis->children_named("baustein")) names.insert(b->attr("name"));
    }
    return rc;
}

// ─────────────────────────────────────────────────────────────────────────────
// PAKET W3-B / Planer-I1 (2026-07-19) — RegistryTrio: die DREI Achsen-Art-Angebots-Registries als EIN POD
// (Ledger §28/§30, STUFE §3.C KONSOLIDIERUNG). GENERALISIERUNG von read_axis_registry OHNE zweiten Parser:
//   * Organ-Registry       engine="cache_engine"             (Angebot der Tier-Stufe, bildet binary_id)
//   * System-Registry      engine="cache_engine_system"      (Angebot der CEB-Stufe, NIE binary_id)
//   * Mess-Registry        engine="cache_engine_measurement" (Angebot der Planer-Stufe, NIE binary_id)
// Alle drei tragen dieselbe <comdare_axis_registry>-Wurzel => dieselbe read_axis_registry-Naht liest ALLE drei;
// RegistryTrio buendelt sie NUR (Resolver-Vorstufe: der Experiment-Planer annotiert seinen Plan mit den 3
// Angebots-Quellen). ADDITIV — die bestehende Einzel-read_axis_registry-Nutzung bleibt unberuehrt.
// ─────────────────────────────────────────────────────────────────────────────
struct RegistryTrio {
    RegistryContents organ;       // Wurzel engine="cache_engine"
    RegistryContents system;      // Wurzel engine="cache_engine_system"
    RegistryContents measurement; // Wurzel engine="cache_engine_measurement"

    // Zahl der Angebots-Achsen je Art (Organ-golden = 17, System = 5).
    [[nodiscard]] std::size_t organ_axis_count() const { return organ.axis_names.size(); }
    [[nodiscard]] std::size_t system_axis_count() const { return system.axis_names.size(); }
    [[nodiscard]] std::size_t measurement_axis_count() const { return measurement.axis_names.size(); }
    // Zahl der Angebots-Bausteine EINER Achse (0, wenn die Achse in dieser Registry fehlt).
    [[nodiscard]] static std::size_t baustein_count(RegistryContents const& rc, std::string const& axis_id) {
        auto const it = rc.axis_names.find(axis_id);
        return it == rc.axis_names.end() ? std::size_t{0} : it->second.size();
    }
    // Zahl der Mess-KATEGORIEN (Angebots-Bausteine der Achse "measurement_category" = 16).
    [[nodiscard]] std::size_t measurement_category_count() const {
        return baustein_count(measurement, "measurement_category");
    }
};

// read_axis_registry_trio — liest die 3 Art-Registries ueber DIESELBE read_axis_registry-Naht (kein zweiter
// Parser). nullopt, wenn EINE der drei nicht lesbar / keine comdare_axis_registry-Wurzel ist (harter Ausfall
// analog eines fehlenden dlsym-Symbols, STUFE §3.C.3 E-RES-I). Die Pfade reicht der Host per CMake-Interface
// herein (statische Registry-Pfad-Defines, feedback_ceb_config_cmake_interface_static_registry_paths_prt_module).
[[nodiscard]] inline std::optional<RegistryTrio>
read_axis_registry_trio(std::filesystem::path const& organ_registry, std::filesystem::path const& system_registry,
                        std::filesystem::path const& measurement_registry) {
    auto o = read_axis_registry(organ_registry);
    auto s = read_axis_registry(system_registry);
    auto m = read_axis_registry(measurement_registry);
    if (!o || !s || !m) return std::nullopt;
    return RegistryTrio{std::move(*o), std::move(*s), std::move(*m)};
}

/// validate_experiment_profile — DIE reine Pruef-Logik (read-only). Zwei Registry-Aufloesungs-Modi:
///   • `engine_registry_paths` NICHT leer (Bruecke-I2, 2-Registry-Kanon): je Engine wird ihre Registry aus
///     DIESER Map (Schluessel = engine-id ee_ce/ee_prt, Wert = voller STATISCHER Registry-Pfad) aufgeloest — die
///     Registries liegen an verschiedenen, per CMake-Interface dokumentierten Pfaden (je Engine EINE), NICHT
///     co-lokalisiert und NICHT als registry_dir/<bare filename>. Der Host reicht beide statischen Pfade herein
///     (die ce-Fassade kennt das prt-art-Repo-Layout NICHT — Baseline-Layering: ce=Framework).
///   • `engine_registry_paths` leer: der bestehende `registry_dir`-Modus — die je-engine `registry`-Dateinamen
///     werden als registry_dir/<filename> aufgeloest (registry_dir leer = (4)+(5) uebersprungen). Die Checks
///     selbst (Existenz / parsbar / engine-Attribut-Abgleich / Doppel-ce / allowed_variants⊆ce-Registry) sind
///     in BEIDEN Modi identisch — nur der Aufloesungs-Pfad je Engine unterscheidet sich. RUECKWAERTS-KOMPATIBEL.
/// `known_workload_ids` = die REAL vorhandenen load_profiles/-ids (aus wd::discover_load_profiles, vom Host
/// hereingereicht); leer = die <workloads>-Pruefung (10) wird uebersprungen (rueckwaerts-kompatibel, Bruecke-I1).
/// Schreibt KEINE Datei, baut KEINE DLL, misst NICHTS. Gibt das Ergebnis-POD zurueck.
[[nodiscard]] inline ExperimentValidationResult
validate_experiment_profile(cx::ExperimentProfile const& ep, std::filesystem::path const& registry_dir = {},
                            std::set<std::string> const&                        known_workload_ids    = {},
                            std::map<std::string, std::filesystem::path> const& engine_registry_paths = {}) {
    ExperimentValidationResult r;

    // ── (1) GENAU 2 engines. ──
    r.engines_checked = ep.engines.size();
    if (ep.engines.size() != 2) {
        r.ok = false;
        r.errors.push_back("EXPERIMENT braucht GENAU 2 <execution_engines><engine> (ee_ce + ee_prt), gefunden: " +
                           std::to_string(ep.engines.size()) + ".");
    }

    // ── (7) F22: engine-id-Eindeutigkeit (Referenz-Schluessel der Phasen; Duplikat = HARTER Fehler). ──
    {
        std::set<std::string> engine_ids;
        for (auto const& e : ep.engines) {
            if (!engine_ids.insert(e.id).second) {
                r.ok = false;
                r.errors.push_back("DOPPELTE engine-id <engine id=\"" + e.id +
                                   "\">: die id muss je Experiment eindeutig sein (Referenz-Schluessel der "
                                   "<phase engine(s)>-Zuordnung).");
            }
        }
    }

    // ── (2) mindestens 1 phase. ──
    if (ep.phases.empty()) {
        r.ok = false;
        r.errors.push_back("EXPERIMENT braucht mindestens 1 <phases><phase> (die Kompositionalen Joins).");
    }

    // ── (3) jede phase.merge ∈ MergeStrategy-Enum. (8) F22: gesetztes pruefling ∈ <lebewesen>-tier-ids. ──
    std::set<std::string> valid_merges;
    for (auto const s : kExperimentMergeStrategies) valid_merges.insert(std::string{merge_strategy_name(s)});
    std::set<std::string> const lebewesen_ids(ep.lebewesen.begin(), ep.lebewesen.end());
    for (auto const& ph : ep.phases) {
        ++r.phases_checked;
        if (valid_merges.find(ph.merge) == valid_merges.end()) {
            r.ok = false;
            std::vector<std::string> const vm(valid_merges.begin(), valid_merges.end());
            r.errors.push_back(
                "UNGUELTIGE Merge-Strategie <phase name=\"" + ph.name + "\" merge=\"" + ph.merge +
                "\">: kein MergeStrategy-Enum-Wert (pruefling_merge.hpp). Gueltig = " + preview_values(vm));
        }
        if (!ph.pruefling.empty() && lebewesen_ids.find(ph.pruefling) == lebewesen_ids.end()) {
            r.ok = false;
            std::vector<std::string> const ids(lebewesen_ids.begin(), lebewesen_ids.end());
            r.errors.push_back("UNBEKANNTES Pruefling-Lebewesen <phase name=\"" + ph.name + "\" pruefling=\"" +
                               ph.pruefling +
                               "\">: keine deklarierte <lebewesen><tier id>. Deklariert = " + preview_values(ids));
        }
    }

    // ── (9) F12-Validator-Haelfte: <op_types> HART gegen die XSD-Enumeration OP-1..OP-6; leer = FEHLER. ──
    // XSD-Kontrakt (super Code/test_data_xml/experiment_schema.xsd:33 required, :168-177 OpTypeType-Enum);
    // vorher mislabelte ein Bogus-Token die Messzeile still und ein leeres Element erfand "OP-1".
    static constexpr std::string_view kValidOpTypes[] = {"OP-1", "OP-2", "OP-3", "OP-4", "OP-5", "OP-6"};
    if (ep.op_types.empty()) {
        r.ok = false;
        r.errors.push_back("LEERES/FEHLENDES <op_types>: das XSD deklariert das Element als required "
                           "(Whitespace-Liste aus OP-1..OP-6) — es wird KEIN Default erfunden.");
    }
    for (auto const& tok : ep.op_types) {
        bool const known =
            std::find(std::begin(kValidOpTypes), std::end(kValidOpTypes), tok) != std::end(kValidOpTypes);
        if (!known) {
            r.ok = false;
            r.errors.push_back("UNGUELTIGES <op_types>-Token \"" + tok +
                               "\": kein Wert der XSD-Enumeration OP-1..OP-6 (experiment_schema.xsd OpTypeType).");
        }
    }

    // ── (10) opt-f/A3: <system_axes> HART gegen die OptO*Option-ids / SimdSubAxis::simd_id()s. LEER ist zulaessig
    //    (CEB-Default O3 / no_extension; XSD minOccurs=0). opt_level/simd sind system_config → binary_id-NEUTRAL
    //    (Provenienz build_version/H-10-Sidecar); hier nur die Enum-Wohlgeformtheit. Die Aufloesung id→Flag
    //    (-O<n> / -march) macht die opt-g-Facade (make_gpp_compile_fn-Kanal), die ISA-Gegatung ebenso.
    // Single-Source: die gueltigen opt_level-ids kommen aus der OptimizationLevelSubAxis-Familie (kAllOptLevelIds),
    // NICHT hartkodiert (Konformitaets-Single-Source; deckungsgleich zur XSD-Enumeration compiler/opt_level/option).
    for (auto const& lvl : ep.compiler.opt_levels) { // Optionen der opt_level-Unter-Achse unter compiler
        ++r.opt_levels_checked;
        bool const known =
            std::find(ms::kAllOptLevelIds.begin(), ms::kAllOptLevelIds.end(), lvl) != ms::kAllOptLevelIds.end();
        if (!known) {
            r.ok = false;
            r.errors.push_back(
                "UNGUELTIGE <opt_level value=\"" + lvl +
                "\">: kein Wert der XSD-Enumeration O0/O1/O2/O3/Ofast (optimization_level_sub_axis.hpp).");
        }
    }
    // Single-Source: die gueltigen simd-ids kommen aus der SimdSubAxis-Familie (kAllSimdIds), NICHT hartkodiert
    // (Konformitaets-NACH F-SIMD 2026-07-18; deckungsgleich zur XSD-Enumeration extension_hardware/simd/option).
    // GN-1-Anker: die <system_axes><extension_hardware>-Sektion validiert gegen die Unter-Achse des AKTIVEN
    // Familien-Knotens (extension_hardware_family_axis.hpp) -- Label-Drift bricht compile-time, nicht erst hier.
    static_assert(ms::SimdNoExtOption::parent_axis_label() == ms::SimdExtensionHardwareFamily::axis_label(),
                  "GN-1: simd haengt unter dem aktiven extension_hardware-Familien-Knoten.");
    for (auto const& s :
         ep.extension_hardware.simd_options) { // Optionen der simd-Unter-Achse (extension_hardware → simd)
        ++r.simd_checked;
        bool const known = std::find(ms::kAllSimdIds.begin(), ms::kAllSimdIds.end(), s) != ms::kAllSimdIds.end();
        if (!known) {
            r.ok = false;
            r.errors.push_back("UNGUELTIGE <simd value=\"" + s +
                               "\">: kein Wert der XSD-Enumeration no_extension/avx2/avx512 "
                               "(simd_sub_axis.hpp).");
        }
    }

    // ── (4)+(5): Registry-Dateien existieren + allowed_variants ⊆ ce-Registry-baustein-names. Zwei
    //    Aufloesungs-Modi (Bruecke-I2, s. Kopf-Doku): (A) engine_registry_paths NICHT leer -> je Engine der
    //    volle STATISCHE Pfad aus der Map (2-Registry-Kanon; je Engine EINE Registry an dokumentiertem Pfad);
    //    (B) sonst der bestehende registry_dir/<bare filename>-Pfad. Die Checks selbst sind modus-invariant. ──
    bool const map_mode = !engine_registry_paths.empty();
    if (!map_mode && registry_dir.empty()) {
        r.warnings.push_back("registry_dir nicht gesetzt: Registry-Existenz (4) + allowed_variants (5) "
                             "uebersprungen — rein strukturelle Validierung.");
    } else {
        // F22/F28 (WP-3, 2026-07-16): 2-Registry-Kanon — erwartetes Wurzel-engine-Attribut je BEKANNTER
        // engine-id (ee_ce→cache_engine, ee_prt→prt_art). Unbekannte ids tragen keine Erwartung (das
        // WELCHE-ids-Vokabular ist Schema-/E11-Frage, K7) — geprueft wird nur der Copy-Paste-Mismatch.
        auto const expected_registry_engine = [](std::string const& engine_id) -> std::string_view {
            if (engine_id == "ee_ce") return "cache_engine";
            if (engine_id == "ee_prt") return "prt_art";
            return {};
        };
        std::optional<RegistryContents> ce_registry; // die Registry mit engine="cache_engine"
        for (auto const& e : ep.engines) {
            // Bruecke-I2: je-Engine-Aufloesung. Map-Modus -> voller statischer Pfad aus der Map (Schluessel =
            // engine-id); fehlt der Eintrag, kann die Registry nicht aufgeloest werden = HARTER Fehler (sonst
            // fiele die (4)/(5)-Gegenpruefung fuer diese Engine still aus). Sonst registry_dir/<bare filename>.
            std::filesystem::path rf;
            if (map_mode) {
                auto const pit = engine_registry_paths.find(e.id);
                if (pit == engine_registry_paths.end()) {
                    r.ok = false;
                    r.errors.push_back("KEIN statischer Registry-Pfad fuer <engine id=\"" + e.id +
                                       "\">: die engine_registry_paths-Map (2-Registry-Kanon, Bruecke-I2) fuehrt "
                                       "keinen Pfad fuer diese engine-id.");
                    continue;
                }
                rf = pit->second;
            } else {
                rf = registry_dir / e.registry;
            }
            if (!std::filesystem::exists(rf)) {
                r.ok = false;
                r.errors.push_back("REGISTRY-Datei fehlt <engine id=\"" + e.id + "\" registry=\"" + e.registry +
                                   "\">: nicht gefunden unter " + rf.string() + ".");
                continue;
            }
            auto parsed = read_axis_registry(rf);
            if (!parsed) {
                // F28: HARTER Fehler (vorher WARNUNG) — eine korrupte Registry validierte 'ok', waehrend die
                // allowed_variants-Pruefung (5) still uebersprungen wurde; die fehlende Datei war bereits Fehler.
                r.ok = false;
                r.errors.push_back("REGISTRY-Datei unlesbar <engine id=\"" + e.id + "\" registry=\"" + e.registry +
                                   "\">: nicht als comdare_axis_registry parsbar (" + rf.string() +
                                   ") — korrupte/fremde Registry darf nicht 'ok' validieren.");
                continue;
            }
            // F22/F28: engine-Attribut-Abgleich fuer bekannte ids (Copy-Paste-Fehler = HARTER Fehler).
            std::string_view const expected = expected_registry_engine(e.id);
            if (!expected.empty() && parsed->engine != expected) {
                r.ok = false;
                r.errors.push_back("REGISTRY-ENGINE-MISMATCH <engine id=\"" + e.id + "\" registry=\"" + e.registry +
                                   "\">: Wurzel-Attribut engine=\"" + parsed->engine + "\", erwartet \"" +
                                   std::string{expected} + "\" (2-Registry-Kanon; Copy-Paste-Fehler?).");
            }
            if (parsed->engine == "cache_engine") {
                if (ce_registry) {
                    // F28: Doppel-ce war vorher last-wins — die zweite Datei ueberschrieb still die echte
                    // ce-Registry im (5)-Check. Jetzt HARTER Fehler; die ERSTE bleibt Pruef-Referenz.
                    r.ok = false;
                    r.errors.push_back("DOPPELTE ce-Registry <engine id=\"" + e.id + "\" registry=\"" + e.registry +
                                       "\">: es darf genau EINE Registry mit engine=\"cache_engine\" referenziert "
                                       "sein (vorher stilles last-wins im allowed_variants-Check).");
                } else {
                    ce_registry = std::move(parsed);
                }
            }
        }
        // (5) allowed_variants gegen die ce-Registry (Golden-Doku: Teilmenge der ce-`baustein name`-Werte).
        if (!ep.axes_default_lookup.empty()) {
            if (!ce_registry) {
                r.warnings.push_back("keine ce-Registry (engine=\"cache_engine\") aufloesbar — "
                                     "allowed_variants (5) nicht gegen die Registry geprueft.");
            } else {
                for (auto const& ax : ep.axes_default_lookup) {
                    auto it = ce_registry->axis_names.find(ax.ref);
                    if (it == ce_registry->axis_names.end()) {
                        r.ok = false;
                        r.errors.push_back("UNBEKANNTE Achse <axes_default_lookup><axis ref=\"" + ax.ref +
                                           "\">: kein axis-id der ce-Registry (cache_engine_axis_registry.xml).");
                        continue;
                    }
                    std::set<std::string> const& valid = it->second;
                    for (auto const& v : ax.allowed_variants) {
                        ++r.variants_checked;
                        if (valid.find(v) == valid.end()) {
                            r.ok = false;
                            std::vector<std::string> const vv(valid.begin(), valid.end());
                            r.errors.push_back("UNGUELTIGE allowed_variant <axis ref=\"" + ax.ref + "\">: \"" + v +
                                               "\" ist kein `baustein name` dieser ce-Registry-Achse. Gueltig = " +
                                               preview_values(vv));
                        }
                    }
                }
            }
        }
    }

    // ── (6) jede category ∈ kAllMeasurementCategories (16). Single-Source = das Enum + der Registry-Name. ──
    if (!ep.measurement_categories.empty()) {
        std::set<std::string> valid_categories;
        for (auto const cat : ms::kAllMeasurementCategories)
            valid_categories.insert(std::string{ms::axis_info(cat).name});
        std::set<std::string> seen_categories;
        for (auto const& cat : ep.measurement_categories) {
            ++r.categories_checked;
            if (valid_categories.find(cat) == valid_categories.end()) {
                r.ok = false;
                std::vector<std::string> const valid(valid_categories.begin(), valid_categories.end());
                r.errors.push_back("UNGUELTIGE Mess-Kategorie <measurement_categories><category name=\"" + cat +
                                   "\">: kein MeasurementCategory-Name (kAllMeasurementCategories, 16). Gueltig = " +
                                   preview_values(valid));
            } else if (!seen_categories.insert(cat).second) {
                r.warnings.push_back("category name=\"" + cat +
                                     "\": mehrfach deklariert — redundante Spalten-Projektion (nicht fatal).");
            }
        }
    }

    // ── (10) Bruecke-I1/M-CE-12 (2026-07-16): jede <workloads>-id ist eine REAL existente load_profiles/-id.
    //    Nur wenn der Host die entdeckten ids hereinreicht (known_workload_ids nicht leer) — leer = uebersprungen
    //    (rueckwaerts-kompatibel: 2-arg-Aufrufer ohne Host-Enumeration, z.B. execute_messreihe). Spiegelt das
    //    Thesis-Profil-Gate (validate_profile Pruefung 6): so faellt eine getippte Experiment-<workloads>-id
    //    SCHON hier rein-lesend auf, statt erst im teuren CEB-Lauf mit exit 4 (schliesst die Luecke, dass der
    //    deprecatete Antrieb mit leerem registry_dir UND ohne Workload-Gegenpruefung validierte).
    if (!known_workload_ids.empty()) {
        for (auto const& w : ep.workloads) {
            ++r.workloads_checked;
            if (known_workload_ids.find(w) == known_workload_ids.end()) {
                r.ok = false;
                std::vector<std::string> const valid_wl(known_workload_ids.begin(), known_workload_ids.end());
                r.errors.push_back("UNBEKANNTE Workload-id \"" + w +
                                   "\" in <workloads>: keine load_profiles/-id (ycsb_*/lp_*). Die <workloads>-"
                                   "Auswahl ist die autoritative Achse-2-Auswahl (#229). Bekannt = " +
                                   preview_values(valid_wl));
            }
        }
    }

    return r;
}

} // namespace comdare::cache_engine::thesis_lazy
