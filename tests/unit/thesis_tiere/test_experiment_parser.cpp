// test_experiment_parser — INC-D (2026-07-14): ctest-Gate fuer den comdare_experiment-Parser als MODUL im
// allgemeinen ce-Parser (parse_experiment_profile, common-Schicht) + die layering-saubere Validierung
// (validate_experiment_profile, cache_engine-Schicht). Muster: test_measurement_categories.cpp (INC-3 Familie A).
//
// BEWEIST LITERAL:
//   (a) PARSE — die committete Golden-Instanz experiment_golden.xml (INC-C) parst in ExperimentProfile mit den
//       woertlich erwarteten Feldern: 2 engines (mit registry), 3 phases (mit merge/engine/engines/pruefling),
//       7 lebewesen, axes_default_lookup (enabled + 3 Achsen mit allowed_variants), 6 workloads, 3 datasets,
//       5 measurement_categories, 3 op_types, output.comparison_metrics==true.
//   (b) VALIDATE OK — gegen die REALE ce-Registry (cache_engine_axis_registry.xml) + prt-Registry: genau 2
//       engines, 3 gueltige merges, allowed_variants ⊆ ce-Registry-baustein-names, categories ∈
//       kAllMeasurementCategories → ok, alle Zaehler stimmen.
//   (c) VALIDATE FEHLER — (c1) ungueltige phase.merge, (c2) allowed_variant nicht-in-Registry, (c3) != 2 engines.
//
// LESE-Schicht: kein Treiber-Lauf, keine #156-Messdaten, kein DLL-Bau. Die Fixtures werden NUR GELESEN.

#include "validate_profile.hpp"                    // validate_experiment_profile / ExperimentValidationResult
#include "xml_config_parser/xml_config_parser.hpp" // XmlConfigParser / ExperimentProfile

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <optional>
#include <set>
#include <string>
#include <system_error>
#include <vector>

#ifndef COMDARE_EXPERIMENT_GOLDEN
#error "COMDARE_EXPERIMENT_GOLDEN must point to tests/unit/thesis_tiere/experiment_golden.xml"
#endif
#ifndef COMDARE_CE_AXIS_REGISTRY
#error "COMDARE_CE_AXIS_REGISTRY must point to libs/cache_engine/algorithm_profiles/cache_engine_axis_registry.xml"
#endif
#ifndef COMDARE_PRT_AXIS_REGISTRY
#error "COMDARE_PRT_AXIS_REGISTRY must point to tests/unit/thesis_tiere/prt_art_axis_registry.xml"
#endif

namespace {

namespace cx  = comdare::builder::xml;
namespace tlz = comdare::cache_engine::thesis_lazy;
namespace fs  = std::filesystem;

// Baut ein temporaeres Registry-Verzeichnis, in das die REALE ce-Registry (Single-Source) + die prt-Registry
// unter den vom Golden referenzierten Dateinamen kopiert werden. So loest validate_experiment_profile die
// je-engine `registry`-Dateinamen (bare) gegen EIN Verzeichnis auf.
fs::path make_registry_dir() {
    fs::path const dir =
        fs::temp_directory_path() /
        ("comdare_inc_d_registry_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::error_code ec;
    fs::create_directories(dir, ec);
    fs::copy_file(fs::path{COMDARE_CE_AXIS_REGISTRY}, dir / "cache_engine_axis_registry.xml",
                  fs::copy_options::overwrite_existing, ec);
    fs::copy_file(fs::path{COMDARE_PRT_AXIS_REGISTRY}, dir / "prt_art_axis_registry.xml",
                  fs::copy_options::overwrite_existing, ec);
    return dir;
}

bool any_contains(std::vector<std::string> const& msgs, std::string const& needle) {
    for (auto const& m : msgs)
        if (m.find(needle) != std::string::npos) return true;
    return false;
}

std::optional<cx::ExperimentProfile> parse_golden() {
    cx::XmlConfigParser const parser;
    return parser.parse_experiment_profile(fs::path{COMDARE_EXPERIMENT_GOLDEN});
}

// S2/A2 P-SYSREG: schreibt ein Minimal-<comdare_experiment> mit frei waehlbarem <system_axes>-Block in eine
// Temp-Datei und parst es. Beweist die Parser-Naht end-to-end (ohne die Golden-Instanz anzufassen -> golden-neutral).
std::optional<cx::ExperimentProfile> parse_experiment_with_system_axes(std::string const& system_axes_block) {
    fs::path const p = fs::temp_directory_path() /
                       ("comdare_sysreg_experiment_" +
                        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".xml");
    {
        std::ofstream out{p};
        out << R"(<?xml version="1.0" encoding="UTF-8"?>
<comdare_experiment version="1" id="sysreg_parse_fixture">
)" << system_axes_block
            << "</comdare_experiment>\n";
    }
    cx::XmlConfigParser const            parser;
    std::optional<cx::ExperimentProfile> ep = parser.parse_experiment_profile(p);
    std::error_code                      ec;
    fs::remove(p, ec);
    return ep;
}

} // namespace

// (a) PARSE — die Golden-Instanz parst woertlich in ExperimentProfile.
TEST(ExperimentParser, ParsesGoldenInstanceLiterally) {
    auto const ep = parse_golden();
    ASSERT_TRUE(ep.has_value()) << "experiment_golden.xml nicht parsbar: " << COMDARE_EXPERIMENT_GOLDEN;

    EXPECT_EQ(ep->version, "1");
    EXPECT_EQ(ep->id, "diplomarbeit_golden");
    EXPECT_EQ(ep->metadata.name, "diplomarbeit_golden");
    EXPECT_EQ(ep->metadata.mode, "defined");

    // 2 engines mit registry.
    ASSERT_EQ(ep->engines.size(), 2u);
    EXPECT_EQ(ep->engines[0].id, "ee_ce");
    EXPECT_EQ(ep->engines[0].type, "CacheEngineExecutionEngineAdapter");
    EXPECT_EQ(ep->engines[0].registry, "cache_engine_axis_registry.xml");
    EXPECT_EQ(ep->engines[1].id, "ee_prt");
    EXPECT_EQ(ep->engines[1].type, "PrtArtExecutionEngineAdapter");
    EXPECT_EQ(ep->engines[1].registry, "prt_art_axis_registry.xml");

    // 7 lebewesen (base_tier-ids).
    ASSERT_EQ(ep->lebewesen.size(), 7u);
    EXPECT_EQ(ep->lebewesen.front(), "prt_art");
    EXPECT_EQ(ep->lebewesen.back(), "wormhole");

    // 3 phases mit merge/engine/engines/pruefling.
    ASSERT_EQ(ep->phases.size(), 3u);
    EXPECT_EQ(ep->phases[0].name, "phase2_cache_engine");
    EXPECT_EQ(ep->phases[0].merge, "Stufe1_CeOnly");
    EXPECT_EQ(ep->phases[0].engine, "ee_ce");
    EXPECT_TRUE(ep->phases[0].engines.empty());
    EXPECT_EQ(ep->phases[1].name, "phase1_prt_art");
    EXPECT_EQ(ep->phases[1].merge, "Stufe2_PrueflingReplace");
    EXPECT_EQ(ep->phases[1].engine, "ee_prt");
    EXPECT_EQ(ep->phases[1].pruefling, "prt_art");
    EXPECT_EQ(ep->phases[2].name, "phase3_kombiniert");
    EXPECT_EQ(ep->phases[2].merge, "Stufe3_FullJoin");
    EXPECT_TRUE(ep->phases[2].engine.empty());
    ASSERT_EQ(ep->phases[2].engines.size(), 2u);
    EXPECT_EQ(ep->phases[2].engines[0], "ee_ce");
    EXPECT_EQ(ep->phases[2].engines[1], "ee_prt");

    // axes_default_lookup (enabled + 3 Achsen mit allowed_variants).
    EXPECT_TRUE(ep->axes_default_lookup_enabled);
    ASSERT_EQ(ep->axes_default_lookup.size(), 3u);
    // INC-2d-Verbund (Registry-Regen 18->17): isa verliess die Komposition (System-Achse target_isa) ->
    // Fixture-Achse [0] ist jetzt mapping (echte Organ-Achse mit exakt 2 Varianten, Kardinalitaeten stabil).
    EXPECT_EQ(ep->axes_default_lookup[0].ref, "mapping");
    ASSERT_EQ(ep->axes_default_lookup[0].allowed_variants.size(), 2u);
    EXPECT_EQ(ep->axes_default_lookup[0].allowed_variants[0], "direct_placement");
    EXPECT_EQ(ep->axes_default_lookup[0].allowed_variants[1], "pool_relative");
    EXPECT_EQ(ep->axes_default_lookup[1].ref, "search_algo");
    EXPECT_EQ(ep->axes_default_lookup[2].ref, "path_compression");

    // workloads / datasets / measurement_categories / op_types.
    ASSERT_EQ(ep->workloads.size(), 6u);
    EXPECT_EQ(ep->workloads.front(), "ycsb_a");
    EXPECT_EQ(ep->workloads.back(), "ycsb_f");
    ASSERT_EQ(ep->datasets.size(), 3u);
    EXPECT_EQ(ep->datasets[0].id, "url");
    EXPECT_EQ(ep->datasets[0].akte_ref, "test_data_xml/url.test_data.xml");
    EXPECT_EQ(ep->datasets[0].loader, "string_corpus");
    EXPECT_EQ(ep->datasets[2].loader, "sosd_uint64");
    ASSERT_EQ(ep->measurement_categories.size(), 5u);
    EXPECT_EQ(ep->measurement_categories[0], "CLU");
    EXPECT_EQ(ep->measurement_categories[1], "CACHE_MISS_L3");
    EXPECT_EQ(ep->measurement_categories[2], "IPC_CPI");
    EXPECT_EQ(ep->measurement_categories[3], "LATENCY_P99");
    EXPECT_EQ(ep->measurement_categories[4], "THROUGHPUT");
    ASSERT_EQ(ep->op_types.size(), 3u);
    EXPECT_EQ(ep->op_types[0], "OP-1");
    EXPECT_EQ(ep->op_types[1], "OP-3");
    EXPECT_EQ(ep->op_types[2], "OP-4");

    // <system_axes> (opt-f/A3): opt_level O2/O3 + simd no_extension/avx2 (System-Achsen, binary_id-neutral).
    ASSERT_EQ(ep->compiler.opt_levels.size(), 2u);
    EXPECT_EQ(ep->compiler.opt_levels[0], "O2");
    EXPECT_EQ(ep->compiler.opt_levels[1], "O3");
    ASSERT_EQ(ep->extension_hardware.simd_options.size(), 2u);
    EXPECT_EQ(ep->extension_hardware.simd_options[0], "no_extension");
    EXPECT_EQ(ep->extension_hardware.simd_options[1], "avx2");

    // output.comparison_metrics == true (+ Pfade nicht leer).
    EXPECT_FALSE(ep->output.binary_path.empty());
    EXPECT_FALSE(ep->output.csv_path.empty());
    EXPECT_FALSE(ep->output.latex_path.empty());
    EXPECT_TRUE(ep->output.comparison_metrics);
}

// (b) VALIDATE OK — gegen die reale ce-Registry + prt-Registry.
TEST(ExperimentParser, ValidatesGoldenAgainstRegistries) {
    auto const ep = parse_golden();
    ASSERT_TRUE(ep.has_value());

    fs::path const                        reg = make_registry_dir();
    tlz::ExperimentValidationResult const vr  = tlz::validate_experiment_profile(*ep, reg);
    for (auto const& e : vr.errors) ADD_FAILURE() << "[validate] " << e;
    EXPECT_TRUE(vr.ok);
    EXPECT_EQ(vr.engines_checked, 2u);
    EXPECT_EQ(vr.phases_checked, 3u);
    EXPECT_EQ(vr.variants_checked, 6u); // 2 (mapping) + 2 (search_algo) + 2 (path_compression)
    EXPECT_EQ(vr.categories_checked, 5u);
    EXPECT_EQ(vr.opt_levels_checked, 2u); // opt-f/A3: O2 + O3
    EXPECT_EQ(vr.simd_checked, 2u);       // opt-f/A3: no_extension + avx2

    std::error_code ec;
    fs::remove_all(reg, ec);
}

// (c1) VALIDATE FEHLER — eine ungueltige phase.merge ist ein HARTER Fehler (kein MergeStrategy-Enum-Wert).
TEST(ExperimentParser, InvalidMergeStrategyIsError) {
    auto ep = parse_golden();
    ASSERT_TRUE(ep.has_value());
    ep->phases[0].merge = "Stufe9_Bogus";

    tlz::ExperimentValidationResult const vr = tlz::validate_experiment_profile(*ep);
    EXPECT_FALSE(vr.ok);
    EXPECT_TRUE(any_contains(vr.errors, "UNGUELTIGE Merge-Strategie"));
    EXPECT_TRUE(any_contains(vr.errors, "Stufe9_Bogus"));
}

// (c2) VALIDATE FEHLER — eine allowed_variant, die kein baustein-name der ce-Registry ist.
TEST(ExperimentParser, AllowedVariantNotInRegistryIsError) {
    auto ep = parse_golden();
    ASSERT_TRUE(ep.has_value());
    ep->axes_default_lookup[0].allowed_variants.push_back("mapping_does_not_exist");

    fs::path const                        reg = make_registry_dir();
    tlz::ExperimentValidationResult const vr  = tlz::validate_experiment_profile(*ep, reg);
    EXPECT_FALSE(vr.ok);
    EXPECT_TRUE(any_contains(vr.errors, "UNGUELTIGE allowed_variant"));
    EXPECT_TRUE(any_contains(vr.errors, "mapping_does_not_exist"));

    std::error_code ec;
    fs::remove_all(reg, ec);
}

// (c3) VALIDATE FEHLER — != 2 engines ist ein HARTER Fehler (Schema verlangt GENAU 2).
TEST(ExperimentParser, EngineCountMustBeExactlyTwo) {
    auto ep = parse_golden();
    ASSERT_TRUE(ep.has_value());
    ep->engines.pop_back(); // nur noch 1 engine

    tlz::ExperimentValidationResult const vr = tlz::validate_experiment_profile(*ep);
    EXPECT_FALSE(vr.ok);
    EXPECT_EQ(vr.engines_checked, 1u);
    EXPECT_TRUE(any_contains(vr.errors, "GENAU 2"));
}

// (c4) F22 (WP-3, 2026-07-16) — doppelte engine-id ist ein HARTER Fehler (Referenz-Schluessel der Phasen).
TEST(ExperimentParser, DuplicateEngineIdIsError) {
    auto ep = parse_golden();
    ASSERT_TRUE(ep.has_value());
    ep->engines[1].id = "ee_ce"; // Duplikat (weiterhin 2 engines -> nur die Eindeutigkeit schlaegt an)

    tlz::ExperimentValidationResult const vr = tlz::validate_experiment_profile(*ep);
    EXPECT_FALSE(vr.ok);
    EXPECT_TRUE(any_contains(vr.errors, "DOPPELTE engine-id"));
}

// (c5) F22 (WP-3, 2026-07-16) — phase.pruefling muss ein deklariertes <lebewesen><tier id> sein.
TEST(ExperimentParser, UnknownPrueflingIsError) {
    auto ep = parse_golden();
    ASSERT_TRUE(ep.has_value());
    ep->phases[1].pruefling = "prt_atr"; // Tippfehler (Golden deklariert prt_art)

    tlz::ExperimentValidationResult const vr = tlz::validate_experiment_profile(*ep);
    EXPECT_FALSE(vr.ok);
    EXPECT_TRUE(any_contains(vr.errors, "UNBEKANNTES Pruefling-Lebewesen"));
    EXPECT_TRUE(any_contains(vr.errors, "prt_atr"));
}

// (c6) F28 (WP-3, 2026-07-16) — eine UNLESBARE Registry-Datei ist jetzt ein HARTER Fehler (vorher nur
//      WARNUNG: eine korrupte Registry validierte 'ok' mit still uebersprungener allowed_variants-Pruefung).
TEST(ExperimentParser, UnreadableRegistryIsError) {
    auto const ep = parse_golden();
    ASSERT_TRUE(ep.has_value());

    fs::path const reg = make_registry_dir();
    {
        std::ofstream out{reg / "prt_art_axis_registry.xml", std::ios::binary | std::ios::trunc};
        out << "<kaputt"; // nicht wohlgeformt -> read_axis_registry == nullopt
    }
    tlz::ExperimentValidationResult const vr = tlz::validate_experiment_profile(*ep, reg);
    EXPECT_FALSE(vr.ok);
    EXPECT_TRUE(any_contains(vr.errors, "REGISTRY-Datei unlesbar"));

    std::error_code ec;
    fs::remove_all(reg, ec);
}

// (c7) F22/F28 (WP-3, 2026-07-16) — engine-Attribut-Abgleich (2-Registry-Kanon): traegt die von ee_prt
//      referenzierte Datei engine="cache_engine" (Copy-Paste-Fehler), ist das ein MISMATCH-Fehler UND ein
//      Doppel-ce-Fehler (vorher: stilles last-wins ueberschrieb die echte ce-Registry im (5)-Check).
TEST(ExperimentParser, RegistryEngineMismatchAndDuplicateCeAreErrors) {
    auto const ep = parse_golden();
    ASSERT_TRUE(ep.has_value());

    fs::path const  reg = make_registry_dir();
    std::error_code ec;
    fs::copy_file(reg / "cache_engine_axis_registry.xml", reg / "prt_art_axis_registry.xml",
                  fs::copy_options::overwrite_existing, ec); // prt-Dateiname traegt jetzt die ce-Registry
    tlz::ExperimentValidationResult const vr = tlz::validate_experiment_profile(*ep, reg);
    EXPECT_FALSE(vr.ok);
    EXPECT_TRUE(any_contains(vr.errors, "REGISTRY-ENGINE-MISMATCH"));
    EXPECT_TRUE(any_contains(vr.errors, "DOPPELTE ce-Registry"));

    fs::remove_all(reg, ec);
}

// (c8) F12-Validator-Haelfte (WP-3, 2026-07-16) — ein Bogus-<op_types>-Token ist ein HARTER Fehler
//      (XSD-Enumeration OP-1..OP-6; vorher mislabelte es die Messzeile still).
TEST(ExperimentParser, BogusOpTypeTokenIsError) {
    auto ep = parse_golden();
    ASSERT_TRUE(ep.has_value());
    ep->op_types.push_back("OP-9"); // ausserhalb der XSD-Enumeration

    tlz::ExperimentValidationResult const vr = tlz::validate_experiment_profile(*ep);
    EXPECT_FALSE(vr.ok);
    EXPECT_TRUE(any_contains(vr.errors, "UNGUELTIGES <op_types>-Token"));
    EXPECT_TRUE(any_contains(vr.errors, "OP-9"));
}

// (c9) F12-Validator-Haelfte (WP-3, 2026-07-16) — ein LEERES <op_types> ist ein HARTER Fehler
//      (XSD: required; vorher erfand der deprecatete Parallel-Antrieb still "OP-1").
TEST(ExperimentParser, EmptyOpTypesIsError) {
    auto ep = parse_golden();
    ASSERT_TRUE(ep.has_value());
    ep->op_types.clear();

    tlz::ExperimentValidationResult const vr = tlz::validate_experiment_profile(*ep);
    EXPECT_FALSE(vr.ok);
    EXPECT_TRUE(any_contains(vr.errors, "LEERES/FEHLENDES <op_types>"));
}

// (c10) Bruecke-I1 (2026-07-16) — eine <workloads>-id, die keine entdeckte load_profiles/-id ist, ist ein
//       HARTER Fehler, SOBALD der Host die bekannte Menge hereinreicht (rein-lesendes Achse-2-Gate, spiegelt
//       das Thesis-Profil-M-CE-12). So faellt ein Tippfehler SCHON bei --validate auf statt erst im CEB-Lauf.
TEST(ExperimentParser, UnknownWorkloadIdIsErrorWhenKnownSetProvided) {
    auto ep = parse_golden();
    ASSERT_TRUE(ep.has_value());
    ep->workloads.push_back("ycsb_TYPO"); // keine reale load_profiles/-id

    std::set<std::string> const           known = {"ycsb_a", "ycsb_b", "ycsb_c", "ycsb_d", "ycsb_e", "ycsb_f"};
    tlz::ExperimentValidationResult const vr    = tlz::validate_experiment_profile(*ep, {}, known);
    EXPECT_FALSE(vr.ok);
    EXPECT_TRUE(any_contains(vr.errors, "UNBEKANNTE Workload-id"));
    EXPECT_TRUE(any_contains(vr.errors, "ycsb_TYPO"));
    EXPECT_EQ(vr.workloads_checked, 7u); // 6 Golden + 1 Tippfehler
}

// (c11) Bruecke-I1 — ohne hereingereichte bekannte Menge (2-arg, execute_messreihe-Pfad) bleibt das
//       Workload-Gate uebersprungen: die Golden-Instanz validiert weiterhin OK (rueckwaerts-kompatibel).
TEST(ExperimentParser, WorkloadGateSkippedWithoutKnownSet) {
    auto const ep = parse_golden();
    ASSERT_TRUE(ep.has_value());

    fs::path const                        reg = make_registry_dir();
    tlz::ExperimentValidationResult const vr  = tlz::validate_experiment_profile(*ep, reg); // 2-arg: known leer
    for (auto const& e : vr.errors) ADD_FAILURE() << "[validate] " << e;
    EXPECT_TRUE(vr.ok);
    EXPECT_EQ(vr.workloads_checked, 0u); // uebersprungen

    std::error_code ec;
    fs::remove_all(reg, ec);
}

// (c12) opt-f/A3 — ein Bogus-<opt_level>-Wert ist ein HARTER Fehler (XSD-Enumeration O0/O1/O2/O3/Ofast).
//       So faellt ein Tippfehler in der System-Achsen-Permutation SCHON bei --validate auf, nicht erst
//       als stiller /Od im CEB-Bau (Fehlerklassen-Pflicht: sichtbar statt still).
TEST(ExperimentParser, BogusOptLevelIsError) {
    auto ep = parse_golden();
    ASSERT_TRUE(ep.has_value());
    ep->compiler.opt_levels.push_back("O9"); // ausserhalb O0/O1/O2/O3/Ofast

    tlz::ExperimentValidationResult const vr = tlz::validate_experiment_profile(*ep);
    EXPECT_FALSE(vr.ok);
    EXPECT_TRUE(any_contains(vr.errors, "UNGUELTIGE <opt_level"));
    EXPECT_TRUE(any_contains(vr.errors, "O9"));
    EXPECT_EQ(vr.opt_levels_checked, 3u); // 2 Golden (O2/O3) + 1 Tippfehler
}

// (c13) opt-f/A3 — ein Bogus-<simd>-Wert ist ein HARTER Fehler (no_extension/avx2/avx512).
TEST(ExperimentParser, BogusSimdExtensionIsError) {
    auto ep = parse_golden();
    ASSERT_TRUE(ep.has_value());
    ep->extension_hardware.simd_options.push_back("avx1024"); // ausserhalb der Enumeration

    tlz::ExperimentValidationResult const vr = tlz::validate_experiment_profile(*ep);
    EXPECT_FALSE(vr.ok);
    EXPECT_TRUE(any_contains(vr.errors, "UNGUELTIGE <simd"));
    EXPECT_TRUE(any_contains(vr.errors, "avx1024"));
}

// (c14) opt-f/A3 — LEERE <system_axes> sind ZULAESSIG (minOccurs=0; leer = CEB-Default O3 / no_extension).
//       Die golden-Byte-Identitaet der 320 binary_ids bleibt unberuehrt (opt/simd sind binary_id-neutral).
TEST(ExperimentParser, EmptySystemAxesIsOk) {
    auto ep = parse_golden();
    ASSERT_TRUE(ep.has_value());
    ep->compiler.opt_levels.clear();
    ep->extension_hardware.simd_options.clear();

    tlz::ExperimentValidationResult const vr = tlz::validate_experiment_profile(*ep);
    EXPECT_TRUE(vr.ok) << "leere <system_axes> duerfen kein Fehler sein (additiv, CEB-Default)";
    EXPECT_EQ(vr.opt_levels_checked, 0u);
    EXPECT_EQ(vr.simd_checked, 0u);
}

// (c15) S2/A2 P-SYSREG — der Parser liest die ZWEI neu verdrahteten Offer-Sub-Achsen aus <system_axes>:
//       compiler/atomic128 (ZWEITE Unter-Achse unter compiler, neben opt_level) + target_isa (EIGENE Haupt-Achse,
//       Optionen DIREKT darunter). End-to-End-Beweis der Parser-Naht (nicht nur Struct-Injektion).
TEST(ExperimentParser, ParsesAtomic128AndTargetIsaFromSystemAxes) {
    auto const ep = parse_experiment_with_system_axes(
        "  <system_axes>\n"
        "    <compiler>\n"
        "      <opt_level><option value=\"O2\"/><option value=\"O3\"/></opt_level>\n"
        "      <atomic128><option value=\"no_cx16\"/><option value=\"cx16\"/></atomic128>\n"
        "    </compiler>\n"
        "    <extension_hardware>\n"
        "      <simd><option value=\"no_extension\"/><option value=\"avx2\"/></simd>\n"
        "    </extension_hardware>\n"
        "    <target_isa><option value=\"x86_64\"/><option value=\"aarch64\"/></target_isa>\n"
        "  </system_axes>\n");
    ASSERT_TRUE(ep.has_value());
    // opt_level + atomic128 koexistieren unter compiler (zwei Unter-Achsen, je EIN Container).
    ASSERT_EQ(ep->compiler.opt_levels.size(), 2u);
    EXPECT_EQ(ep->compiler.opt_levels[0], "O2");
    EXPECT_EQ(ep->compiler.opt_levels[1], "O3");
    ASSERT_EQ(ep->compiler.atomic128.size(), 2u);
    EXPECT_EQ(ep->compiler.atomic128[0], "no_cx16");
    EXPECT_EQ(ep->compiler.atomic128[1], "cx16");
    ASSERT_EQ(ep->extension_hardware.simd_options.size(), 2u);
    EXPECT_EQ(ep->extension_hardware.simd_options[0], "no_extension");
    // target_isa = eigene Haupt-Achse: die Optionen stehen DIREKT unter <target_isa> (kein Zwischen-Container).
    ASSERT_EQ(ep->target_isa.isa.size(), 2u);
    EXPECT_EQ(ep->target_isa.isa[0], "x86_64");
    EXPECT_EQ(ep->target_isa.isa[1], "aarch64");
}

// (c16) S2/A2 P-SYSREG — gueltige atomic128 + target_isa werden AKZEPTIERT (System-Achsen, binary_id-neutral);
//       die Zaehler stimmen. Belegt, dass die neu verdrahteten Validat-Bloecke die kanonischen Werte durchlassen.
TEST(ExperimentParser, ValidAtomic128AndTargetIsaAreAccepted) {
    auto ep = parse_golden();
    ASSERT_TRUE(ep.has_value());
    ep->compiler.atomic128 = {"no_cx16", "cx16"};
    ep->target_isa.isa     = {"x86_64", "aarch64"};

    fs::path const                        reg = make_registry_dir();
    tlz::ExperimentValidationResult const vr  = tlz::validate_experiment_profile(*ep, reg);
    for (auto const& e : vr.errors) ADD_FAILURE() << "[validate] " << e;
    EXPECT_TRUE(vr.ok);
    EXPECT_EQ(vr.atomic128_checked, 2u);
    EXPECT_EQ(vr.target_isa_checked, 2u);

    std::error_code ec;
    fs::remove_all(reg, ec);
}

// (c17) S2/A2 P-SYSREG — ein Bogus-atomic128-Wert (Tippfehler cx32) ist ein HARTER Fehler (no_cx16/cx16).
TEST(ExperimentParser, BogusAtomic128IsError) {
    auto ep = parse_golden();
    ASSERT_TRUE(ep.has_value());
    ep->compiler.atomic128.push_back("cx32"); // ausserhalb no_cx16/cx16

    tlz::ExperimentValidationResult const vr = tlz::validate_experiment_profile(*ep);
    EXPECT_FALSE(vr.ok);
    EXPECT_TRUE(any_contains(vr.errors, "UNGUELTIGE <atomic128"));
    EXPECT_TRUE(any_contains(vr.errors, "cx32"));
    EXPECT_EQ(vr.atomic128_checked, 1u);
}

// (c18) S2/A2 P-SYSREG — ein Bogus-target_isa-Wert (Tippfehler riscv) ist ein HARTER Fehler (x86_64/aarch64).
TEST(ExperimentParser, BogusTargetIsaIsError) {
    auto ep = parse_golden();
    ASSERT_TRUE(ep.has_value());
    ep->target_isa.isa.push_back("riscv"); // ausserhalb x86_64/aarch64

    tlz::ExperimentValidationResult const vr = tlz::validate_experiment_profile(*ep);
    EXPECT_FALSE(vr.ok);
    EXPECT_TRUE(any_contains(vr.errors, "UNGUELTIGE <target_isa"));
    EXPECT_TRUE(any_contains(vr.errors, "riscv"));
    EXPECT_EQ(vr.target_isa_checked, 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// KERN-A (S4 Mess-Schema, 2026-07-20) — die additiven Felder + Checks der Mess-Schema-Steuerung. golden-neutral:
// die Golden-Instanz wird nur gelesen/in-memory mutiert (kein XML angefasst); die Parser-Naht wird ueber Temp-XML
// end-to-end belegt. Beweist: measurement_tooling-HAUPT PASSIV geparst (K1; die Semantik/Validierung ist P-MESSTOOL),
// per-Achse merge_mode (K1/K3), phase.identity / CacheEngine-self (K1/K3), <template> (K1 + tolerante K3),
// leere phases/categories = derive/all-16 (K3).
// ─────────────────────────────────────────────────────────────────────────────

// (k1a) K1 — der Parser liest die Mess-Tooling-HAUPT-Achse PASSIV: je <combo tools=".."/> EINE Tooling-KONFIG.
//       (Die id-Validierung + der Fan-out gehoeren dem Schwester-Paket P-MESSTOOL; KERN-A traegt das Feld nur.)
TEST(ExperimentParser, ParsesMeasurementToolingHauptAchse) {
    auto const ep = parse_experiment_with_system_axes("  <measurement_tooling>\n"
                                                      "    <combo tools=\"wallclock macro\"/>\n"
                                                      "    <combo tools=\"micro\"/>\n"
                                                      "  </measurement_tooling>\n");
    ASSERT_TRUE(ep.has_value());
    ASSERT_EQ(ep->measurement_tooling.size(), 2u);
    ASSERT_EQ(ep->measurement_tooling[0].size(), 2u);
    EXPECT_EQ(ep->measurement_tooling[0][0], "wallclock");
    EXPECT_EQ(ep->measurement_tooling[0][1], "macro");
    ASSERT_EQ(ep->measurement_tooling[1].size(), 1u);
    EXPECT_EQ(ep->measurement_tooling[1][0], "micro");
}

// (k1c) K1 — der Parser liest das optionale <template ref mode> (Research-Gesamtalgorithmus-Load).
TEST(ExperimentParser, ParsesTemplateElement) {
    auto const ep = parse_experiment_with_system_axes("  <template ref=\"research_full\" mode=\"full\"/>\n");
    ASSERT_TRUE(ep.has_value());
    EXPECT_EQ(ep->templ.ref, "research_full");
    EXPECT_EQ(ep->templ.mode, "full");
}

// (k3-tmpl-a) K3 — ein <template mode="full"> OHNE ref wird AKZEPTIERT (mode="full" = der bereits funktionierende
//             Voll-Whitelist-Pfad + axes_default_lookup restrict/extend). Abwesenheit von ref = kein Fehler.
TEST(ExperimentParser, TemplateModeFullWithoutRefIsAccepted) {
    auto ep = parse_golden();
    ASSERT_TRUE(ep.has_value());
    ep->templ.ref  = ""; // kein benanntes Paper-Template -> rein mode-getrieben
    ep->templ.mode = "full";

    fs::path const                        reg = make_registry_dir();
    tlz::ExperimentValidationResult const vr  = tlz::validate_experiment_profile(*ep, reg);
    for (auto const& e : vr.errors) ADD_FAILURE() << "[validate] " << e;
    EXPECT_TRUE(vr.ok) << "<template mode=\"full\"> ohne ref = der heutige Voll-Whitelist-Pfad (kein Fehler)";

    std::error_code ec;
    fs::remove_all(reg, ec);
}

// (k3-tmpl-b) K3 — ein <template mode="full"> mit (noch) unbekanntem ref validiert TOLERANT (kein harter Fehler;
//             die Paper-Template-Aufloesung ist post-v3, faellt auf full zurueck). mode="full" = heutiger Pfad.
TEST(ExperimentParser, TemplateWithUnknownRefValidatesTolerant) {
    auto ep = parse_golden();
    ASSERT_TRUE(ep.has_value());
    ep->templ.ref  = "some_future_paper_template"; // die Registries fuehren es NOCH NICHT
    ep->templ.mode = "full";

    fs::path const                        reg = make_registry_dir();
    tlz::ExperimentValidationResult const vr  = tlz::validate_experiment_profile(*ep, reg);
    for (auto const& e : vr.errors) ADD_FAILURE() << "[validate] " << e;
    EXPECT_TRUE(vr.ok) << "unbekannter template-ref = kein harter Fehler (post-v3, faellt auf full zurueck)";

    std::error_code ec;
    fs::remove_all(reg, ec);
}

// (k1b) K1 — der Parser liest per-Achse merge (axes_default_lookup/axis) + phase identity (CacheEngine-self-Marker).
TEST(ExperimentParser, ParsesPerAxisMergeAndPhaseIdentity) {
    auto const ep = parse_experiment_with_system_axes(
        "  <phases>\n"
        "    <phase name=\"self_phase\" merge=\"Stufe1_CeOnly\" identity=\"CacheEngine\"/>\n"
        "  </phases>\n"
        "  <axes_default_lookup enabled=\"true\">\n"
        "    <axis ref=\"mapping\" allowed_variants=\"direct_placement\" merge=\"merge\"/>\n"
        "    <axis ref=\"search_algo\" allowed_variants=\"btree\"/>\n"
        "  </axes_default_lookup>\n");
    ASSERT_TRUE(ep.has_value());
    ASSERT_EQ(ep->phases.size(), 1u);
    EXPECT_EQ(ep->phases[0].identity, "CacheEngine");
    ASSERT_EQ(ep->axes_default_lookup.size(), 2u);
    EXPECT_EQ(ep->axes_default_lookup[0].merge_mode, "merge");  // gesetzt
    EXPECT_TRUE(ep->axes_default_lookup[1].merge_mode.empty()); // abwesend = replace-Default
}

// (k3c) K3 — ein Bogus per-Achse Merge-Modus ist ein HARTER Fehler ({replace,merge}; leer = replace-Default).
TEST(ExperimentParser, BogusPerAxisMergeModeIsError) {
    auto ep = parse_golden();
    ASSERT_TRUE(ep.has_value());
    ep->axes_default_lookup[0].merge_mode = "kombiniere"; // ausserhalb {replace,merge}

    tlz::ExperimentValidationResult const vr = tlz::validate_experiment_profile(*ep);
    EXPECT_FALSE(vr.ok);
    EXPECT_TRUE(any_contains(vr.errors, "UNGUELTIGER Merge-Modus"));
    EXPECT_TRUE(any_contains(vr.errors, "kombiniere"));
    EXPECT_EQ(vr.axis_merge_checked, 1u);
}

// (k3d) K3 — ein gueltiger per-Achse Merge-Modus ("merge") wird AKZEPTIERT; leere merge (Default) zaehlt nicht mit.
TEST(ExperimentParser, ValidPerAxisMergeModeIsAccepted) {
    auto ep = parse_golden();
    ASSERT_TRUE(ep.has_value());
    ep->axes_default_lookup[0].merge_mode = "merge"; // gueltig; axes[1..2] bleiben leer = replace-Default

    fs::path const                        reg = make_registry_dir();
    tlz::ExperimentValidationResult const vr  = tlz::validate_experiment_profile(*ep, reg);
    for (auto const& e : vr.errors) ADD_FAILURE() << "[validate] " << e;
    EXPECT_TRUE(vr.ok);
    EXPECT_EQ(vr.axis_merge_checked, 1u); // nur die EINE gesetzte Achse

    std::error_code ec;
    fs::remove_all(reg, ec);
}

// (k3e) K3 Fork 3 — ein pruefling="CacheEngine" (self-Marker) wird AKZEPTIERT (misst die CacheEngine SELBST,
//       NICHT dem <lebewesen>-Zwang unterworfen); ein identity="self" ebenso.
TEST(ExperimentParser, CacheEngineSelfPrueflingAndIdentityAccepted) {
    auto ep = parse_golden();
    ASSERT_TRUE(ep.has_value());
    ep->phases[1].pruefling = "CacheEngine"; // self-Marker statt eines <lebewesen>
    ep->phases[0].identity  = "self";        // gueltiger self-Marker

    fs::path const                        reg = make_registry_dir();
    tlz::ExperimentValidationResult const vr  = tlz::validate_experiment_profile(*ep, reg);
    for (auto const& e : vr.errors) ADD_FAILURE() << "[validate] " << e;
    EXPECT_TRUE(vr.ok);
    EXPECT_FALSE(any_contains(vr.errors, "UNBEKANNTES Pruefling-Lebewesen"));

    std::error_code ec;
    fs::remove_all(reg, ec);
}

// (k3f) K3 Fork 3 — ein GESETZTES identity, das KEIN anerkannter self-Marker ist, ist ein HARTER Fehler
//       (ein Tippfehler wuerde sonst still keinen CacheEngine-self-Pass markieren).
TEST(ExperimentParser, BogusPhaseIdentityIsError) {
    auto ep = parse_golden();
    ASSERT_TRUE(ep.has_value());
    ep->phases[0].identity = "CachEngine"; // Tippfehler (nicht CacheEngine/self)

    tlz::ExperimentValidationResult const vr = tlz::validate_experiment_profile(*ep);
    EXPECT_FALSE(vr.ok);
    EXPECT_TRUE(any_contains(vr.errors, "kein anerkannter CacheEngine-self-Marker"));
    EXPECT_TRUE(any_contains(vr.errors, "CachEngine"));
}

// (k3g) K3 — LEERE <measurement_categories> = "alle 16 Kategorien" = OK (Check (6) uebersprungen, kein Fehler).
TEST(ExperimentParser, EmptyMeasurementCategoriesIsOk) {
    auto ep = parse_golden();
    ASSERT_TRUE(ep.has_value());
    ep->measurement_categories.clear();

    fs::path const                        reg = make_registry_dir();
    tlz::ExperimentValidationResult const vr  = tlz::validate_experiment_profile(*ep, reg);
    for (auto const& e : vr.errors) ADD_FAILURE() << "[validate] " << e;
    EXPECT_TRUE(vr.ok) << "leere categories = alle 16 (kein Fehler)";
    EXPECT_EQ(vr.categories_checked, 0u);

    std::error_code ec;
    fs::remove_all(reg, ec);
}

// (k3h) K3 — LEERE <phases> ist KEIN Fehler mehr (KERN-A: der Planer leitet die 3 Default-Stufen ab).
TEST(ExperimentParser, EmptyPhasesIsOkAndDerives) {
    auto ep = parse_golden();
    ASSERT_TRUE(ep.has_value());
    ep->phases.clear();

    fs::path const                        reg = make_registry_dir();
    tlz::ExperimentValidationResult const vr  = tlz::validate_experiment_profile(*ep, reg);
    for (auto const& e : vr.errors) ADD_FAILURE() << "[validate] " << e;
    EXPECT_TRUE(vr.ok) << "leere <phases> = derive (kein Fehler)";
    EXPECT_EQ(vr.phases_checked, 0u);

    std::error_code ec;
    fs::remove_all(reg, ec);
}
