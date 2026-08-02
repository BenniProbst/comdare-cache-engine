// abi/anatomy_version_stamp.hpp -- compile-time-Ableitung der Organ-Stempel-Zeile aus einer Composition
// (Bau W12-A, Section 43, Inkrement 4-Wiring).
//
// Section 43: die Tier-Binary traegt ihre kOrganAxisVersionLine einkompiliert. Diese Zeile wird aus den 17
// Kompositions-Achsen-Typen der AdHocComposition abgeleitet (jede exponiert name() + algo_version), in
// kanonischer compose-Ordnung (Entscheid W12-A-5). NUR Haupt-Achsen (Section 42.b).
//
// BYTE-SICHER (Round-Trip-Wache): Dieser Helfer wird IM Makro COMDARE_DEFINE_ANATOMY_MODULE aus dem
// Composition-Typ aufgerufen -- der emittierte .cpp-QUELLTEXT bleibt unveraendert (weiterhin nur
// COMDARE_DEFINE_ANATOMY_MODULE_ADHOC(<typen>)), die Emitter-Round-Trip-Byte-Wache (test_lazy_adhoc_source_gen)
// bleibt gruen. Der Stempel lebt im kompilierten Binary. SEPARATE Welt zur .algos-Sig (X.Y.Z-Voll-Form).

#pragma once

#include <cache_engine/abi/meta_meta_stamp_suffix.hpp>             // A13-M2: Klammer-Anhang der Meta-Metas (Owner-Q1)
#include <cache_engine/abi/system_axis_code_versions.hpp>          // A2 (G2-4): kSystemAxisCodeVersions (Single-Source)
#include <cache_engine/measurement/axis_version_stamp.hpp>         // AxisVersionEntry + build_axis_version_stamp_line
#include <cache_engine/measurement/external_utils_family_axis.hpp> // A13-M2: ExternalUtilsHub (System-Meta-Meta-Glieder)
#include <cache_engine/measurement/measurement_framework_registry.hpp> // O-8 Schritt 9: load_framework-Segment
#include <cache_engine/measurement/measurement_tooling_registry.hpp>   // K7b-2: kMeasurementToolingRegistry (Vollmenge)

#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::abi {

/// Die Zahl der Organ-HAUPT-Achsen (ORG-18, topics/axis.hpp:18 "18 Slots"). Sie steht hier als
/// benannte Konstante und nicht als nackte 17/18 im Array-Typ, damit der static_assert unten die
/// Drift fangen kann, die A8.2 gerade behoben hat.
///
/// WARUM NICHT kCompositionAxisNames direkt: das Array wohnt in builder/experiment_tree/ und dieser
/// Header in abi/ -- ein Include waere eine Layer-Inversion (abi darf nicht auf builder zeigen). Die
/// Kopplung laeuft deshalb ueber die Zahl plus die Namens-Liste im Array unten, nicht ueber einen
/// Include. Wer ORG-18 aendert, aendert BEIDE Orte.
inline constexpr std::size_t kOrganAxisCount = 18;

/// organ_stamp_line<Comp>() -- die kOrganAxisVersionLine "achse=algo@X.Y.Z;..." aus den 18 benannten
/// Achsen-Aliassen einer Composition, in kanonischer compose-Ordnung (== AdHocComposition-Alias-Ordnung
/// == kCompositionAxisNames). Jede Achse muss name() + algo_version tragen.
///
/// A8.2 (O-8 Schritt 7, OP-11): die Zeile traegt jetzt ACHTZEHN Eintraege -- persistence_target ist
/// DAZUGEKOMMEN. Das 17er-Array war ein golden-neutral bedingter Nicht-Nachzug der ORG-18-Welle: der
/// Stempel durfte damals nicht brechen, also blieb die 18. Achse draussen. Das OD-1-Stempel-PRINZIP
/// verlangt aber ALLE Organ-Achsen mit je-Achse-Algorithmus-Version; eine fehlende 18. Achse waere
/// eine Stempel-BLINDSTELLE, in der eine persistence_target-Drift unsichtbar bliebe.
/// KEINE Meta-Meta-Eintraege in dieser Zeile (RF-7: je Typ EINE Array-Zeile, Typ-Trennung) --
/// load_framework stempelt in der MESS-Zeile, die System-Meta-Metas in der System-Sphaere.
///
/// BLOCKER (W12-A, Live-Code-Befund): die REALEN AdHocComposition-Achsen-Typen sind STRATEGIE-Typen
/// (z.B. ObservableComposedContainer<...>) und tragen KEIN name()/algo_version -- nur die Registry-WRAPPER
/// (StaticAxisVariants_*) tragen sie. Diese Funktion ist daher (noch) NICHT auf reale Module anwendbar; sie
/// beweist die Format-/Ordnungs-Logik gegen name()/algo_version-tragende Typen (Test: Mock-Composition). Die
/// Metadaten-Quelle fuer den realen Modul-Stempel ist eine offene Architektur-Frage (Registry-basiert im
/// Emitter vs. Wrapper-Typ-Liste durchs Makro) -- beide beruehren die Emitter-Round-Trip-Byte-Wache.
template <class Comp>
[[nodiscard]] inline std::string organ_stamp_line() {
    using ::comdare::cache_engine::measurement::AxisVersionEntry;
    using ::comdare::cache_engine::measurement::build_axis_version_stamp_line;
    std::array<AxisVersionEntry, kOrganAxisCount> const entries{{
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
        // A8.2 (OP-11): der 18. Slot. Er stand als einziger nicht in dieser Zeile.
        {"persistence_target", Comp::persistence_target::name(), Comp::persistence_target::algo_version},
    }};
    static_assert(entries.size() == kOrganAxisCount,
                  "Die Organ-Stempel-Zeile muss ALLE Organ-Haupt-Achsen tragen (ORG-18). Genau diese "
                  "Luecke war A8.2: das Array stand auf 17 und liess persistence_target aus, wodurch "
                  "eine Drift dieser Achse im Stempel unsichtbar blieb.");
    return build_axis_version_stamp_line(entries);
}

/// system_stamp_line() -- die kSystemAxisVersionLine (Section 43, Entscheid W12-A-1). ZWEIPHASIG dokumentiert:
/// HEUTE traegt sie die STATISCHEN System-Achsen-ALGORITHMUS-Versionen (Compiler-/SIMD-/Scheduling-Achsen-
/// Code-Version), NICHT die gewaehlten System-Zellwerte -- der Tier-Emitter ist system-blind (W4-B-Invariante),
/// und die Zellwerte existieren bereits als Provenienz im .version-Sidecar. W10-ANSCHLUSS: die CEB-Naht
/// (perm_compile kennt die Zelle) ergaenzt die Zellwerte via Compile-Define -> DANN ist die Zeile komplett,
/// ohne den Emitter zu entblinden. Format identisch zur Organ-Zeile ("achse=algo@X.Y.Z"); Algorithmus-Marker
/// "code" = die statische Code-Identitaet der System-Achse.
///
/// A13-M2 (Owner-Entscheid E2 + Antwort Q1 vom 02.08.2026): HINTER die drei Haupt-Achsen-Segmente haengt die
/// Zeile jetzt den KLAMMER-ANHANG der System-Meta-Metas -- heute "[simd=code@1.0.0]", also VIER Eintraege statt
/// drei. Die Glieder kommen aus der EINEN Typliste ExternalUtilsHub::meta_metas (keine zweite Liste); die
/// EBENE steckt in der Klammer-Tiefe, nicht im Namen. Owner-E2 woertlich: Meta-Metas werden "einfach dynamisch
/// ans Ende der Kette in den bestehenden Zeilen angehaengt" -- keine Sonderzeile, kein Sonderfeld.
/// BYTE-FOLGE (beabsichtigt, nicht Nebeneffekt): die System-Zeile aendert sich fuer JEDE Binary -> alle
/// kuenftigen Fingerprints/Lager-Keys verschieben sich. Das ist der geplante M2-Anteil des einen
/// Fingerprint-Global-Shifts VOR Voll-Bau-4 (vorher existiert kein schuetzenswerter Bestand).
[[nodiscard]] inline std::string system_stamp_line() {
    using ::comdare::cache_engine::measurement::AxisVersionEntry;
    using ::comdare::cache_engine::measurement::build_axis_version_stamp_line;
    // A2 (G2-4 Schritt 3): die Achsen + Code-Versionen aus der Single-Source system_axis_code_versions.hpp
    // (frueher hartkodiert als {"<achse>","code","v1"}); "code" bleibt der Achsen-Marker, die Version ist je
    // Achse bump-bar. A3 (O-8 Schritt 4): die Zeile traegt GENAU DREI HAUPT-Achsen-Segmente statt fuenf -- die
    // Schleife zieht das aus kSystemAxisCodeCount automatisch nach, hier war KEIN Edit noetig. Genau dafuer
    // wurde die Hartkodierung damals aufgeloest.
    // Render-neutral: "v1.0.0" -> "1.0.0" wie zuvor "v1" -> "1.0.0".
    std::array<AxisVersionEntry, kSystemAxisCodeCount> entries{};
    for (std::size_t i = 0; i < kSystemAxisCodeCount; ++i)
        entries[i] = {kSystemAxisCodeVersions[i].axis, "code", kSystemAxisCodeVersions[i].version};
    std::string line = build_axis_version_stamp_line(entries);
    // A13-M2: der Meta-Meta-Klammer-Anhang ANS ENDE. Single-Source der Glieder == ExternalUtilsHub::meta_metas
    // (external_utils_family_axis.hpp:149) -- ein spaeteres gpu/fpga/npu-Glied erscheint hier ohne Edit.
    append_meta_meta_suffix(line, meta_meta_stamp_suffix<::comdare::cache_engine::measurement::ExternalUtilsHub>());
    return line;
}

/// measurement_stamp_line(tooling) -- die kMeasurementAxisVersionLine (Section 43, Section 47: Mess-Tooling == HAUPT,
/// W12-A3). Traegt GENAU die gewaehlte Mess-Tooling-HAUPT-Wahl {wallclock/macro/micro} (die collector-Achse,
/// Plan-D1: von binary_id="never" zur Fan-out-HAUPT promotet, binary_id-relevant NUR ueber diesen Version-Line-
/// Stempel) als EINEN Eintrag "measurement_tooling=<tooling>@X.Y.Z". Analog zu system_stamp_line(): dieselbe
/// AxisVersionEntry/build_axis_version_stamp_line-Welt, dieselbe X.Y.Z-Voll-Form (SEPARATE Welt zur .algos-Sig).
///
/// Section-43-INVARIANTE, PRAEZISIERT (O-8 Schritt 9 / RF-1): NUR Haupt-Achsen. Die Ablaufmethodik
/// (run_methodology debug/measure/release) bleibt UNTER-Achse (Laufzeit-Sweep) und NIE Stempel-Bestandteil.
/// Beim Last-Framework ist zu TRENNEN, was der alte Text zusammenwarf: die Framework-WAHL ist seit A3 eine
/// Meta-Meta-HAUPT-Achse des Mess-Realms und damit STEMPELBAR (sie steht als erstes Segment in dieser Zeile);
/// die Workload-WERTE (ycsb_a..f, Ranges) bleiben RT-Unter-Achse und werden NIE gestempelt. Der Algorithmus-
/// Marker == die gewaehlte Tooling-id; die Version == die statische Code-Identitaet der Mess-Tooling-Achse
/// ("v1" -> 1.0.0). Leere Wahl -> leere Zeile (ehrlich: kein Mess-Tooling einkompiliert; die Makro-
/// Materialisierung legt dann measurement_line auf "" mit measurement_len==0).
/// O-8 Schritt 9 (RF-1 / Ledger 70.1, IV.2.4 K3): das load_framework-SEGMENT der Mess-Zeile.
///
/// SCHARFSCHALTUNG des in Schritt 0A inert angelegten version-Feldes: die Mess-Framework-Achse wird
/// jetzt gestempelt. load_framework hat mit A3 die System-Welt ERSATZLOS verlassen und ist
/// Meta-Meta-HAUPT-Achse des MESS-Realms -- sein Segment gehoert deshalb NUR in diese Zeile und NIE in
/// die System- oder Organ-Zeile (RF-7: je Achsen-Typ EINE Array-Zeile).
///
/// POSITION (OP-3, Manager-Entscheid): ERSTES Meta-Meta-Segment, VOR measurement_tooling -- die
/// Bauplan-Vorgabe "load_framework = ERSTE Meta-Meta" auf die Segment-Ordnung uebertragen.
///
/// WARUM NUR BEI NICHT-LEERER ZEILE: eine leere Mess-Zeile heisst "kein Mess-Tooling einkompiliert",
/// und die Makro-Materialisierung verlaesst sich darauf (measurement_line == "", measurement_len == 0).
/// Ein Segment in eine sonst leere Zeile zu schreiben wuerde aus "nichts gemessen" ein "etwas
/// gemessen" machen -- die Zeile bleibt deshalb leer, wenn sie leer ist.
[[nodiscard]] inline ::comdare::cache_engine::measurement::AxisVersionEntry load_framework_stamp_entry() noexcept {
    namespace cm = ::comdare::cache_engine::measurement;
    // Heute genau EIN reales Last-Framework (kMeasurementFrameworkCount == 1, static_assert-gesichert);
    // id und Version kommen aus der Registry, nicht aus Literalen.
    auto const& info = cm::measurement_framework_info(cm::MeasurementFramework::Ycsb);
    return {"load_framework", info.id, info.version};
}

[[nodiscard]] inline std::string measurement_stamp_line(std::string_view tooling) {
    using ::comdare::cache_engine::measurement::AxisVersionEntry;
    using ::comdare::cache_engine::measurement::build_axis_version_stamp_line;
    if (tooling.empty()) return {};
    // A2 (G2-4 Schritt 4): die Code-Version aus der Tooling-Registry (Lookup per id) statt der "v1"-Hartkodierung;
    // bekannte id -> "v1.0.0" (render-neutral zu "1.0.0"), unbekannte id -> "v0.0.0"-Sentinel (@0.0.0, nur ungueltige
    // ids; A13-M1b: dreistellig nach Owner-Q3, byte-neutral zum frueheren "v0").
    // O-8 Schritt 9 (OP-3): load_framework steht als ERSTES Segment VOR measurement_tooling.
    std::array<AxisVersionEntry, 2> const entries{{
        load_framework_stamp_entry(),
        {"measurement_tooling", tooling, ::comdare::cache_engine::measurement::tooling_version_for_id(tooling)},
    }};
    return build_axis_version_stamp_line(entries);
}

/// measurement_stamp_line(toolings) -- K7b-2 (Section 64-D1-B, 2026-07-22): die MENGEN-Form der
/// kMeasurementAxisVersionLine. Statt EINER Tooling-Wahl traegt die Zeile die MENGE der gewaehlten Mess-Tools als N
/// Eintraege "measurement_tooling=<t>@1.0.0" (';'-getrennt, Eingabe-Reihenfolge; Section-64-Vollmengen-Provenienz).
/// Leere Tokens werden uebersprungen; leere/leer-gefilterte Menge -> leere Zeile. Dieselbe X.Y.Z-Voll-Form / SEPARATE
/// Welt zur .algos-Sig wie die Einzel-Form (build_axis_version_stamp_line). binary_id-NEUTRAL (Mess-Achse
/// binary_id="never" -> der Stempel lebt nur im Version-Line/Binary, nie in der binary_id/CRC).
[[nodiscard]] inline std::string measurement_stamp_line(std::span<std::string_view const> toolings) {
    using ::comdare::cache_engine::measurement::AxisVersionEntry;
    using ::comdare::cache_engine::measurement::build_axis_version_stamp_line;
    std::vector<AxisVersionEntry> entries;
    entries.reserve(toolings.size() + 1);
    // O-8 Schritt 9 (OP-3): load_framework als ERSTES Segment -- aber nur, wenn die Zeile ueberhaupt
    // entsteht. Die Leer-Semantik ("kein Mess-Tooling einkompiliert" => leere Zeile) bleibt unberuehrt;
    // der Eintrag wird deshalb erst NACH der Filterung vorangestellt.
    for (std::string_view const t : toolings)
        if (!t.empty())
            // A2 (G2-4 Schritt 4): Code-Version per id-Lookup (Registry) statt "v1"-Hartkodierung; Sentinel "v0.0.0"
            // fuer unbekannte ids (render-neutral fuer die gueltigen wallclock/macro/micro).
            entries.push_back(
                {"measurement_tooling", t, ::comdare::cache_engine::measurement::tooling_version_for_id(t)});
    if (entries.empty()) return {};
    entries.insert(entries.begin(), load_framework_stamp_entry());
    return build_axis_version_stamp_line(entries);
}

/// measurement_stamp_line_full_set() -- K7b-2 (Section 64-D1-B): die VOLLE Mess-Tooling-Vollmenge
/// {wallclock,macro,micro} (Single-Source kMeasurementToolingRegistry, Registry-Reihenfolge) als Mengen-Stempel. Das
/// ist der Section-64-[all]-Default: eine [all]-CEB misst mit dem vollen Tooling-Angebot -> ihre Provenienz traegt
/// ALLE Tools. Genau kMeasurementToolingCount Eintraege, immer non-empty (die Registry ist nie leer, static_assert).
[[nodiscard]] inline std::string measurement_stamp_line_full_set() {
    using ::comdare::cache_engine::measurement::kMeasurementToolingCount;
    using ::comdare::cache_engine::measurement::kMeasurementToolingRegistry;
    std::array<std::string_view, kMeasurementToolingCount> ids{};
    for (std::size_t i = 0; i < kMeasurementToolingCount; ++i) ids[i] = kMeasurementToolingRegistry[i].id;
    return measurement_stamp_line(std::span<std::string_view const>{ids});
}

/// measurement_stamp_line_from_combo_legend(legend) -- die Mess-Tooling-Stempel-Zeile aus einer Planer-Combo-Legende
/// (S6-P1b Env-Bruecke: COMDARE_MEASUREMENT_COMBO traegt die vom Planer gewaehlte [a,b,c]-Legende, z.B. "[wallclock]").
/// LEER oder "[all]" (das volle Mess-System, kein Tooling-spezifischer Fan-out) -> "" (kein Stempel; der byte-stabile
/// Default-Pfad -> emittierte Quelltexte byte-identisch). Sonst: die inneren Tooling-ids (ohne die []-Klammern) als
/// Stempel-Tooling. So reist die gewaehlte Combo bis in die emittierte DLL-Source, ohne den Emitter zu entblinden.
[[nodiscard]] inline std::string measurement_stamp_line_from_combo_legend(std::string_view legend) {
    // Leere Legende = "keine Legende gereicht" -> byte-stabil leer (der from_env-UNGESETZT-Zweig entscheidet dort
    // ueber die Vollmengen-Default-Semantik, NICHT dieser reine Legenden-Parser).
    if (legend.empty()) return {};
    std::string_view inner = legend;
    if (inner.size() >= 2 && inner.front() == '[' && inner.back() == ']') inner = inner.substr(1, inner.size() - 2);
    // Section 64-D1-B (2026-07-22): [all] / leer-innen -> die VOLLE 3-Tool-Vollmenge (Vollmengen-Provenienz), NICHT
    // mehr "" (das war die Regression: die [all]-Lane emittierte leere Mess-Provenienz).
    if (inner.empty() || inner == "all") return measurement_stamp_line_full_set();
    // Sonst: die inneren Tooling-ids als MENGE (komma-getrennt, Eingabe-Reihenfolge) -> N-Eintrags-Stempel.
    std::vector<std::string_view> toks;
    for (std::size_t start = 0; start <= inner.size();) {
        std::size_t const comma = inner.find(',', start);
        std::size_t const end   = comma == std::string_view::npos ? inner.size() : comma;
        if (end > start) toks.push_back(inner.substr(start, end - start));
        if (comma == std::string_view::npos) break;
        start = comma + 1;
    }
    return measurement_stamp_line(std::span<std::string_view const>{toks});
}

/// merge_stamp_line(strategy, pruefling, merged_axes) -- die kMergeAxisVersionLine (Section 59, K6a): der DRITTE
/// Tier-Binary-Stempel = die MERGE-KOMBINATION. Zusaetzlich zu den zwei Section-58-Arrays (System + Organ) traegt
/// die Tier-Binary damit die Merge-Art (die MergeStrategy: Stufe1/Stufe2/Stufe3) + die Pruefling-Identitaet + die
/// Namen/Versionen der beteiligten Achsen-Algorithmen (Section 59-C: "Namen + Versionen JEDER Achsen-Algorithmen
/// IMMER im Stempel; PLUS ein dritter Tier-Binary-Stempel = die Merge-Kombination"). Format:
///   "merge=<strategy>;pruefling=<pruefling>[;<axis>=<algo>@X.Y.Z;...]"
/// R6 (§59-A(2)/A(3), Nacht-Audit 2026-07-22): der Stempel traegt die Strategie VERBATIM -- Stufe2_Hybrid (merge-
/// Modus, CE+Pruefling-Hybrid je Pruefling) erzeugt damit einen ANDEREN Stempel als Stufe3_FullJoin (fulljoin-Modus,
/// kombinierte Union); die beiden Merge-Kategorien bleiben am 3. Tier-Stempel getrennt (ihre Vermischung war die
/// Regression, merge_plan.hpp::merge_mode_to_strategy).
/// -- dieselbe X.Y.Z-Voll-Form / SEPARATE Welt zur .algos-Sig wie organ/system/measurement (build_axis_version_
/// stamp_line fuer den Achsen-Teil). NUR HAUPT-Achsen (Section 58-V; Unter-Achsen fliessen dynamisch zur Laufzeit
/// durch).
///
/// ce-only-/IDENTITAETS-Fall -> LEERE Zeile (Section 59-C-golden-Konsequenz): leere/Stufe1_CeOnly-Strategie ODER
/// leere/"CacheEngine"/"self"-Pruefling-Identitaet -> "" (kein Merge einkompiliert). So bleibt der ce-only-/
/// Katalog-Pfad byte-identisch (die Makro-Materialisierung legt merge_line auf "" mit merge_len==0), der
/// golden-CRC 0xF1C1F26A1232073B unberuehrt -- die Merges sind ein additiver id-Satz.
[[nodiscard]] inline std::string
merge_stamp_line(std::string_view strategy, std::string_view pruefling,
                 std::span<::comdare::cache_engine::measurement::AxisVersionEntry const> merged_axes = {}) {
    using ::comdare::cache_engine::measurement::build_axis_version_stamp_line;
    // ce-only (Stufe1 / keine Merge-Art) -> leer (byte-identischer golden-Pfad).
    if (strategy.empty() || strategy == "Stufe1_CeOnly") return {};
    // Identitaets-/self-Pruefling ("CacheEngine"/self / kein Pruefling) -> leer (Fork 3: identity=self ist ce).
    if (pruefling.empty() || pruefling == "CacheEngine" || pruefling == "self") return {};
    std::string out = "merge=";
    out += strategy;
    out += ";pruefling=";
    out += pruefling;
    std::string const axes = build_axis_version_stamp_line(merged_axes);
    if (!axes.empty()) {
        out += ';';
        out += axes;
    }
    return out;
}

} // namespace comdare::cache_engine::abi
