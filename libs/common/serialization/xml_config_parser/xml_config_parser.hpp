#pragma once
// XML-Config-Parser - Liest 4 Konfigurations-Saetze (REV 7 §5.2)
//
//   cache_engine_permutations.xml
//   search_algorithm_permutations.xml
//   allocator_permutations.xml
//   test_data_sets.xml   [DEPRECATED-Slot — s.u.]
//
// GO-5 Fork 2 (Dataset-Wahrheitsquelle, 2026-07-12, Dossier 20260712-go5 A.2): `test_data_sets.xml`
// existiert NIRGENDS im Bestand (0x; einzig ein Unit-Test erzeugt sie sich synthetisch). Die
// WAHRHEITSQUELLE der Datensaetze sind die test_data-AKTEN `Code/test_data_xml/*.test_data.xml`
// (super; Checksum/line_count/preprocessing via compute_dataset_akte, #25-Kanon). Der Parser-Slot
// (`CacheEngineConfig::test_data_sets`) bleibt als Legacy-Schnittstelle stehen (Doku-nie-loeschen;
// der Legacy-Pfad ist ohnehin COMDARE_LEGACY_MESSREIHEN-gated), wird aber NICHT befuellt und darf
// NICHT nachtraeglich mit einer eigenen test_data_sets.xml gefuellt werden (KEINE Doppelquelle, R2).
// Der E4-Weg konsumiert die Akten ueber `<datasets>` im comdare_thesis_profile (Fork 1/A.1, s.u.).
//
// Minimaler XML-Reader fuer Phase 6.4 Skelett. Phase 7 ersetzt durch
// echten XML-Parser (tinyxml2 oder eigene Implementation).
//
// REV 7.6 V8.6 — Modi 'defined' (vorhandene SOTA-Profile aus
// algorithm_profiles/sota/) vs. 'full' (alle Achsen-Permutationen aus
// permutation_axes.xml). User-Direktive 2026-05-13/14 + Habich-Antwort
// 2026-05-14: "immer so vollstaendig wie moeglich".

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace comdare::builder::xml {

struct PermutationEntry {
    std::string                                  id;
    std::unordered_map<std::string, std::string> attributes;
};

// REV 7.6 V8.6 — Mess-Modi
enum class MessreihenMode {
    Defined, // nur referenzierte SOTA-Profile aus algorithm_profiles/sota/
    Full     // alle Permutationen aus permutation_axes.xml (Cartesian product)
};

[[nodiscard]] inline MessreihenMode parse_mode(std::string_view s) noexcept {
    if (s == "full") return MessreihenMode::Full;
    return MessreihenMode::Defined; // default
}

[[nodiscard]] inline std::string_view mode_to_string(MessreihenMode m) noexcept {
    return (m == MessreihenMode::Full) ? "full" : "defined";
}

// REV 7.6 V8.6 — Ein Profil aus algorithm_profiles/sota/<id>.profile.xml
// REV 7.6 V19.1 — expected_workload Tag (optional Override fuer V11.2-Heuristik)
// REV 7.7 V29.A — allocator_override Tag (optional Override fuer axes/allocator)
struct AlgorithmProfile {
    std::string id;        // z.B. "art", "hot", "masstree"
    std::string paper_ref; // z.B. "P01", "P02"
    std::string
        pruefling_type; // "" = full / "abstract" (AP-6/#240 rev): Prüfling-Marker-Profil (P08/P09/P33), zeigt nur auf Organ-Achse(n) + Host-Fallback
    std::unordered_map<std::string, std::string> axes; // page, node, traversal, ...
    std::string                                  key_types;
    std::string                                  value_types;
    std::string expected_workload;  // V19.1: optional, z.B. "YCSB_A".."YCSB_F" oder leer
    std::string allocator_override; // V29.A: optional, Allokator-Profile-id (z.B. "mimalloc")
};

// REV 7.6 V8.6 — Eine Messreihe aus messreihe-XML
struct Messreihe {
    std::string    id;
    MessreihenMode mode = MessreihenMode::Defined;
    // defined-mode: referenzierte sota-Profile (per id)
    std::vector<std::string> sota_profile_refs;
    // full-mode: keine Refs noetig — Builder enumeriert alle Achsen-Permutationen
};

struct CacheEngineConfig {
    std::vector<PermutationEntry> cache_engine_permutations;
    std::vector<PermutationEntry> search_algorithm_permutations;
    std::vector<PermutationEntry> allocator_permutations;
    std::vector<PermutationEntry> test_data_sets;

    // REV 7.6 V8.6 — geladene Profile + Messreihen
    std::vector<AlgorithmProfile> sota_profiles;
    std::vector<Messreihe>        messreihen;
};

// ─────────────────────────────────────────────────────────────────────────────
// KF-2/KF-1 (2026-06-02): comdare_thesis_profile — Diplomarbeit-Cache-Line-Konfigurator.
// Schema: algorithm_profiles/thesis_profiles/SCHEMA.md.
// ─────────────────────────────────────────────────────────────────────────────
struct ThesisTier { // Paper-Original = statisches Achsen-Tupel
    std::string id;
    std::string profile_ref; // relativer Pfad zu sota/*.profile.xml
    std::string paper_ref;
};

struct ThesisAxisSpec {              // eine permutierte (dynamische) Achse
    std::string              ref;    // z.B. "layout","isa","cacheline","node_width","alloc_hw"
    std::vector<std::string> values; // leer = volle Liste aus permutation_axes.xml
    // Nur fuer ref=="cacheline" (per-Organ Cache-Line-Unterachse, KF-3):
    std::vector<std::string> per_organ;         // Organe mit JE eigener Einstellung
    std::vector<std::string> line_sizes;        // 32/64/128/256 (C1 2026-07-12: 32 additiv, KF-5 thesis-treu)
    std::vector<std::string> alignments;        // none/cacheline_aligned/padded
    std::vector<std::string> sw_prefetch_hints; // none/T0/T1/T2/NTA
    // Nur fuer ref=="node_width" (FF2-Unterachse Knoten-Breite in Cache-Lines, C2 2026-07-12). ADDITIV —
    // leer bei allen anderen Achsen/Profilen (Round-Trip bestehender Profile byte-identisch).
    std::vector<std::string> width_in_lines; // 1/2/4/8/16
    // Nur fuer ref=="alloc_hw" (NUMA/Page->allocator-Unterachse, F-B 2026-07-12). ADDITIV — leer bei
    // allen anderen Achsen/Profilen (Round-Trip bestehender Profile byte-identisch).
    std::vector<std::string> alloc_numa_nodes; // auto/0/1
    std::vector<std::string> alloc_pages;      // 4k/2m
};

struct ThesisMode { // einer der 3 Permutationsmodi
    std::string              name;
    std::string              merge;         // Stufe1_CeOnly / Stufe2_PrueflingReplace / Stufe3_FullJoin
    std::vector<std::string> active_axes;   // Achsen-Teilmenge dieses Modus
    std::string              pruefling;     // optional (Stufe 2/3)
    std::vector<std::string> replaces_axes; // optional (Stufe 2)
};

// ─────────────────────────────────────────────────────────────────────────────
// STRANG-A KORRIGIERT Increment 2 / S1 (2026-06-18) — die 4 deklarativen Konstrukte, die die m3v2-Selektion als
// comdare_thesis_profile-Felder ausdruecken. (Die fruehere Code-Selektion m3v2_select_profile.hpp/SelectMode wurde in
// Strang A Inc 4 ENTFERNT — diese Profil-Felder sind jetzt die EINZIGE Quelle.) ADDITIV (kein bestehendes Feld geaendert) → das base_pilot-
// /cacheline_study-Profil und der Round-Trip bleiben byte-identisch (alle 4 Listen leer = Default). Plan §S1.
// ─────────────────────────────────────────────────────────────────────────────

// (b) <axis_sweep axis=".." baseline="index0"/> — EINE Achse gegen eine feste Baseline (alle uebrigen Ebenen
//     Index 0) variieren. Ersetzt die fruehere make_axis_sweep + axis_to_level-Map (Strang A Inc 4 entfernt). Deklarativ:
//     nur Achsen-Name + Baseline-Marker; der Treiber loest die Achse via StaticBinaryView::flat_index auf.
struct ThesisAxisSweep {
    std::string axis;                // z.B. "node_type" — gegen die Baseline variiert
    std::string baseline = "index0"; // Baseline-Tier-Marker (alle Ebenen Index 0); aktuell nur "index0"
};

// (c) <sota_series id="A|B|C" lebewesen=".." merge=".."/> — eine SOTA-/PRT-ART-Reihe (Stufe 1/2/3 = die 3
//     Kompositionalen Joins). Ersetzt die fruehere sota_lebewesen_names/sota_series_ids (Strang A Inc 4 entfernt).
//     `merge` nutzt die BESTEHENDEN ThesisMode-merge-Felder (Stufe1_CeOnly/Stufe2_PrueflingReplace/Stufe3_FullJoin).
struct ThesisSotaSeries {
    std::string id;        // "A" / "B" / "C"
    std::string lebewesen; // Lebewesen-/Tier-Name (prt_art/art/hot/masstree/surf/start/wormhole)
    std::string merge;     // Stufe1_CeOnly / Stufe2_PrueflingReplace / Stufe3_FullJoin
    // #171 (Text-Agent AP-X2/TODO-2, 2026-06-20): die Pruefling-Auspraegung "full" vs "abstract" (Doc 14
    // §18-§19 Pruefling-Slot-Pattern + cacheline-doc §0/§1.2 "Originalkonfiguration"). ADDITIV + OPTIONAL —
    // leer = aus `merge` ABGELEITET (Stufe1_CeOnly == self-contained == "full"; Stufe2/Stufe3 == Teilmenge +
    // Host-Fallback == "abstract"). Ein explizites <sota_series pruefling_type=".."/> uebersteuert die
    // Ableitung (z.B. Forscher-eigene Reihe). Die Ableitung lebt in sota_catalog::derive_pruefling_type
    // (Single-Source) — KEIN neues Selektions-Konzept, nur eine 1:1-Sicht auf die bestehende MergeStrategy.
    std::string pruefling_type; // "" (ableiten) / "full" / "abstract"
    // GO-5 Fork 6 (Fairness-Modus, 2026-07-12, Dossier A.6 + Thesis §sec:fairness
    // kapitel/de/06_evaluation_methodology.tex:128-136): "Jeder Vergleich TRENNT einen gemeinsamen
    // Minimalmodus (Common-Denominator: externe Werte-Handles, keine PRT-ART-Spezialpfade) vom
    // PRT-ART-Native-Modus (Inline-Umschaltung, Cache-Engine, Seitentyp-Scheduler)." ADDITIV +
    // OPTIONAL: "" (ungesetzt = heutiges Verhalten, CSV-Tag "-") / "common_denominator" / "native".
    // HEUTIGER Konsum: Reihen-Tag bis in die Runner-Spec (SotaPass.fairness_mode) + CSV-Spalte
    // fairness_mode + Resume-Stamp. Die Kompositions-Pinnung des common_denominator-Falls
    // (value_handle_external + PRT-Spezialpfade aus) + die MESS-Abnahme sind DATEN-gated
    // (#156/#162-Fenster) — ehrlich dokumentiert, hier NICHT vorgebaut.
    std::string fairness; // "" (ungesetzt) / "common_denominator" / "native"
};

// GO-5 Fork 1 (W/D/K-XML-Strecke, Option A', 2026-07-12, Dossier A.1): <datasets>/<dataset .../> —
// die Mess-INPUT-Dimension D (Dataset) ADDITIV im E4-Profil-Schema. Je Eintrag referenziert
// `akte_ref` eine test_data-AKTE `Code/test_data_xml/<name>.test_data.xml` (super) = die
// SINGLE-SOURCE der Datensatz-Provenienz (Fork 2/R2: KEINE test_data_sets.xml-Doppelquelle).
// `loader` = DatasetLoaderRegistry-loader_id (dataset_loader.hpp; Repo-Loader: "string_corpus"
// fuer die 6er-Kanon-String-Korpora, "sosd_uint64" fuer die binaere SOSD-Akte). Datasets sind
// MESS-INPUTS, keine Binary-Achsen → binary_id-neutral (golden-Roundtrip unberuehrt). Fehlt
// <datasets> ⇒ Liste leer = heutiges Verhalten (synthetischer YCSB-Generator) byte-identisch.
// Der Loader-MESS-Konsum (load_or_generate_ycsb im Workload-Pfad) ist der dokumentierte offene
// Folge-Schritt (lauf-gated; der Loader-Slot selbst ist seit #184 hermetisch bewiesen).
struct ThesisDatasetRef {
    std::string id;       // Kurzname (z.B. "url") — eindeutiger Referenz-Schluessel (Log/CSV-Meta)
    std::string akte_ref; // Pfad/Name der Akte (…<name>.test_data.xml) — Format-Check im --validate
    std::string loader;   // DatasetLoaderRegistry-loader_id (z.B. "string_corpus")
};

// (d) <run_options cap=".." platform=".." build_version=".." resume=".."/> — die Lauf-Steuerung, die heute aus
//     argv/env von run_lazy_150 kommt. Deklarativ im Profil; der Treiber liest sie als Defaults (argv/env darf
//     (run_lazy_150 geloescht 2026-07-11; Quelle argv/env heute Code/02_messung_driver / XML run_options)
//     weiterhin uebersteuern — Rueckwaerts-Kompatibilitaet). cap=0 / leere Strings = "ungesetzt".
struct ThesisRunOptions {
    int           cap   = 0;          // max_binaries-Obergrenze (0 = ungesetzt → Treiber-Default)
    std::uint64_t n_ops = 0;          // Ops je (Binary×Setting) (0 = ungesetzt → Treiber-/Fassaden-Default, G5)
    std::string   platform;           // CSV-Tag platform (z.B. "win-x86_64")
    std::string   build_version;      // CSV-Tag build_version (z.B. "m3v2")
    bool          resume     = true;  // Mess-Resume an/aus (#139)
    bool          resume_set = false; // true wenn <run_options resume=..> explizit gesetzt war
};

struct ThesisProfile {
    std::string                 id;
    int                         schema_version = 0;
    std::vector<ThesisTier>     base_tiers;
    std::vector<ThesisAxisSpec> permute_axes;
    // compile_dims
    std::vector<std::string> workloads;      // YCSB A..F
    std::string              telemetry_mode; // on/off/leaf_sampled/all
    bool                     telemetry_silent = false;
    // runtime_dynamic (OS-seitig, CacheEngineBuilder-Durchlauf)
    std::vector<std::string>           thread_counts; // 1/2/4
    std::vector<std::string>           hw_prefetcher; // all_on/adjacent_off/all_off
    std::vector<std::string>           prefetch_distances;
    std::vector<std::string>           pool_budgets_bytes;
    std::vector<std::string>           batch_sizes;
    std::vector<std::string>           inline_thresholds_bytes;
    std::map<std::string, std::string> fixed_conditions; // turbo/smt/aslr/numa/governor
    // repetitions
    int  repetitions             = 3;
    bool repetitions_interpolate = false;
    bool repetitions_overlay     = true;
    // modes + static
    std::vector<ThesisMode> modes;
    std::string             static_axes_from; // "base_tier"
    std::string             key_types, value_types;

    // ── S1 (Increment 2, 2026-06-18): die 4 deklarativen m3v2-Selektions-Konstrukte (ADDITIV; leer = Default). ──
    // (a) <working_set_sweep>{N-Liste}</working_set_sweep> — die Working-Set-N-Werte (Record-Zahlen) der aeusseren
    //     Lauf-Iteration. Ersetzt COMDARE_WORKLOAD_RECORDS + den alten PS-foreach-Behelf (entfernt).
    //     Leer = einmaliger Lauf (rueckwaerts-kompatibel). Als String-Liste (wie thread_counts) gehalten.
    std::vector<std::string>      working_set_sweep;
    std::vector<ThesisAxisSweep>  axis_sweeps; // (b) <axis_sweep .../> — eine Achse gegen feste Baseline
    std::vector<ThesisSotaSeries> sota_series; // (c) <sota_series .../> — SOTA-/PRT-ART-Reihen A/B/C
    ThesisRunOptions              run_options; // (d) <run_options .../> — Lauf-Steuerung (cap/platform/...)

    // ── GO-5 Fork 1 (2026-07-12): <datasets>/<dataset id akte_ref loader/> — die deklarierten
    //    Datensatz-AKTEN-Referenzen (ADDITIV; leer = Default = synthetischer YCSB-Generator,
    //    byte-identisch zum heutigen Verhalten). Doku an ThesisDatasetRef. ──
    std::vector<ThesisDatasetRef> datasets;

    // ── INC-3 Familie A (2026-07-14): <measurement_categories>/<category name=".."/> — die dritte
    //    W/D/K-Dimension K (Mess-KATEGORIE) ADDITIV im E4-Schema. Eine reine SPALTEN-PROJEKTION ueber die
    //    16 gemessenen System-Kategorien (cache_engine/measurement/measurement_category.hpp): eine
    //    SICHT-Auswahl, KEINE Binary-/Kompositions-Achse → binary_id-neutral (golden-Roundtrip unberuehrt).
    //    Fehlt <measurement_categories>, bleibt die Liste leer = heutiges Verhalten (alle 16 Kategorien)
    //    byte-identisch. Als NAMEN-Strings gehalten (NICHT als MeasurementCategory-Enum): ThesisProfile lebt
    //    in der common-Schicht, die die cache_engine-Enum nicht referenzieren darf (Baseline-Layering in
    //    Stein) — die String->Enum-Aufloesung + Gueltigkeits-Pruefung gegen die Single-Source
    //    (kMeasurementAxisRegistry) lebt in der cache_engine-Schicht (profile_facade/validate_profile.hpp),
    //    exakt wie ThesisDatasetRef.loader gegen kKnownDatasetLoaderIds. ──
    std::vector<std::string> measurement_categories;
};

// ─────────────────────────────────────────────────────────────────────────────
// INC-D (2026-07-14): comdare_experiment — der Experiment-Parser als MODUL im allgemeinen
// ce-Parser. REINE LESE-Schicht ueber common::xml::parse_document (KEIN tinyxml2/regex).
// Schema: Code/test_data_xml/experiment_schema.xsd (INC-C), Golden-Instanz experiment_golden.xml.
// LAYERING (Baseline in Stein): `common` referenziert NIE `cache_engine` — die ExperimentProfile-
// Felder halten NAMEN als STRINGS (merge / registry / allowed_variants / categories). Die Enum-/
// Registry-/MergeStrategy-Aufloesung + Gueltigkeits-Pruefung leben in der cache_engine-Schicht
// (profile_facade/validate_profile.hpp::validate_experiment_profile), exakt wie ThesisProfile.
// ─────────────────────────────────────────────────────────────────────────────
struct ExperimentMetadata {
    std::string name; // <metadata><name>
    std::string mode; // <metadata><mode> — defined|full|full_sampled (String; Enum-Pruefung = cache_engine-Schicht)
};

struct ExperimentEngine {
    std::string id;       // <engine id=..>
    std::string type;     // <engine type=..> (Adapter-Typname, roh belassen)
    std::string registry; // <engine registry=..> (Registry-XML-Dateiname; Existenz/Namen = validate)
};

struct ExperimentPhase {
    std::string              name;      // <phase name=..>
    std::string              merge;     // <phase merge=..> (MergeStrategy-Name als String)
    std::string              engine;    // <phase engine=..> (optional Einzel-EE)
    std::vector<std::string> engines;   // <phase engines=..> (optional Whitespace-Liste von EE-ids)
    std::string              pruefling; // <phase pruefling=..> (optional)
};

struct ExperimentAxisDefault {                 // <axes_default_lookup><axis ref allowed_variants/>
    std::string              ref;              // Registry-axis-id (z.B. "isa")
    std::vector<std::string> allowed_variants; // Whitespace-Liste von baustein-name-Werten (Teilmenge)
};

struct ExperimentOutput {
    std::string binary_path;                // <output><binary_path>
    std::string csv_path;                   // <output><csv_path>
    std::string latex_path;                 // <output><latex_path>
    bool        comparison_metrics = false; // <output><comparison_metrics> (bool)
};

// <system_axes> — CEB-System-Achsen (opt-f/A3), konform zur V35-Tabelle §2.1: Haupt-Achse → Unter-Achse → Optionen.
// Die Parent-Ebene (compiler / extension_hardware) wird ERHALTEN (nicht flach gedroppt). binary_id-NEUTRAL
// (system_config; opt/simd stehen NIE in kCompositionAxisNames). Rohstrings (cache_engine-frei); die Enum-/Flag-
// Aufloesung (O0..Ofast → -O<n> / no_extension|avx2|avx512 → -march) erfolgt in der cache_engine-Schicht.
struct CompilerAxisSel {                 // Haupt-System-Achse "compiler" (15) — trägt dynamische Unter-Achsen
    std::vector<std::string> opt_levels; // Unter-Achse "opt_level" (15.2): ihre Optionen <option value=O0..Ofast>
};
struct ExtensionHardwareAxisSel { // Haupt-System-Achse "extension_hardware" (6., Q2 Option C)
    std::vector<std::string>
        options; // Optionen der simd-Unter-Achse <simd><option value=no_extension|avx2|avx512> (symmetrisch opt_level)
};

struct ExperimentProfile {
    std::string                   version;   // <comdare_experiment version=..>
    std::string                   id;        // <comdare_experiment id=..>
    ExperimentMetadata            metadata;  // <metadata>
    std::vector<ExperimentEngine> engines;   // <execution_engines><engine>* (Schema: GENAU 2)
    std::vector<std::string>      lebewesen; // <lebewesen><tier id=..>* (base_tier-ids)
    std::vector<ExperimentPhase>  phases;    // <phases><phase>* (Schema: >=1; Golden: 3 Kompositionale Joins)
    // <axes_default_lookup enabled=..> — reines LIMIT (ungenannte Achsen = volle Registry-Liste).
    bool                               axes_default_lookup_enabled = false;
    std::vector<ExperimentAxisDefault> axes_default_lookup;
    std::vector<std::string>           workloads;    // <workloads> (Whitespace-Tokens, YCSB-ids)
    std::vector<ThesisDatasetRef>      datasets;     // <datasets><dataset id akte_ref loader>* (Single-Source-Akten)
    std::vector<std::string> measurement_categories; // <measurement_categories><category name=..>* (Spalten-Projektion)
    std::vector<std::string> op_types;               // <op_types> (Whitespace-Tokens OP-1..OP-6)
    CompilerAxisSel          compiler; // <system_axes><compiler> (Haupt-Achse → opt_level-Unter-Achse → Optionen)
    ExtensionHardwareAxisSel
                     extension_hardware; // <system_axes><extension_hardware><simd> (Haupt-Achse → simd-Unter-Achse)
    ExperimentOutput output;             // <output>
};

class XmlConfigParser {
public:
    [[nodiscard]] CacheEngineConfig             parse(std::filesystem::path const& root_dir) const;
    [[nodiscard]] std::vector<PermutationEntry> parse_one(std::filesystem::path const& xml_file) const;

    // REV 7.6 V8.6 — algorithm_profiles/-Loader
    [[nodiscard]] std::vector<AlgorithmProfile> load_sota_profiles(std::filesystem::path const& sota_dir) const;
    [[nodiscard]] AlgorithmProfile              parse_profile(std::filesystem::path const& profile_xml) const;
    [[nodiscard]] std::vector<Messreihe>        load_messreihen(std::filesystem::path const& messreihen_xml) const;

    // KF-1 (2026-06-02): comdare_thesis_profile-Parser (self-contained XML-DOM, xml_reader.hpp).
    // Liefert nullopt bei fehlender/fehlerhafter Datei oder falschem Wurzel-Tag.
    [[nodiscard]] std::optional<ThesisProfile> parse_thesis_profile(std::filesystem::path const& xml_file) const;

    // INC-D (2026-07-14): comdare_experiment-Parser (self-contained XML-DOM, xml_reader.hpp). Liest ALLE
    // Elemente des Schemas (Attribute UND verschachtelte Werte). Fehlertolerant (fehlende optionale ->
    // Default); nullopt bei fehlender/nicht-wohlgeformter Datei oder falschem Wurzel-Tag (comdare_experiment).
    [[nodiscard]] std::optional<ExperimentProfile>
    parse_experiment_profile(std::filesystem::path const& xml_file) const;
};

} // namespace comdare::builder::xml
