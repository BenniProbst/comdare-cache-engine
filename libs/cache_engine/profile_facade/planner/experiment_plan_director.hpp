#pragma once
// -----------------------------------------------------------------------------
// PAKET W3-B / Phase-1-I1 (2026-07-19) — ExperimentPlanDirector: der EINE benannte Planer-Walk (GoF Builder).
//
// Basis (Bauplan docs/plaene/20260719-planer-ceb-tier-topdown-BAUPLAN.md, Phase 1 + KONSOLIDIERUNG §30 +
// Resolver-STUFE 20260719-registry-angebot-anwender-xml-resolver-STUFE.md §3.C/§4): der musterlose 5-tiefe
// Schleifennest der Lauf-Pfade (run_profile / run_experiment_profile, opt x simd x Passes) traegt keinen
// benannten Baustein. Diese Naht zieht die ENUMERATION als GoF-Director + GoF-Builder heraus, OHNE die
// golden Callees anzutasten:
//
//   Pattern-Zuordnung (musterlos verboten, Bauplan Pattern-Tabelle):
//     * ExperimentPlanDirector::construct(IPlanBuilder&) = DIRECTOR (GoF Builder) — besitzt EINEN
//       deterministischen Walk opt x simd x (Thesis: Sweep-Passes / Experiment: Phasen->Passes). Er
//       ORCHESTRIERT nur: er ruft die BESTEHENDEN Zerlegungs-Bausteine
//         - thesis_lazy::profile_sweep_passes(tp, "")            (profile_runner.hpp:283, #26/GO-5)
//         - thesis_lazy::project_experiment_to_sota_passes(ep)   (sota_catalog.hpp:470, Bruecke I3)
//         - die opt/simd-Listen aus dem GEPARSTEN Profil (Welle-2-Naht: compiler.opt_levels /
//           extension_hardware.simd_options) + system_axis_opt_flag_of / system_axis_march_of
//           (profile_run_entry.hpp:125/134, geteilte GN-3-Naht).
//       KEINE Aenderung der Callees, KEIN dritter Enumerations-Walk (der v32-Antrieb im super-Repo bleibt
//       unberuehrt — Bauplan Phase-0-Kritik-Blocker). Der Perm-Walk ist EINMAL implementiert (walk_perms_),
//       der Steps-Emitter ist der einzige art-abhaengige Teil.
//     * IPlanBuilder = BUILDER-Interface (abstrakter Bauplan-Emitter) — Hook-Methoden begin_plan/begin_perm/
//       on_step/end_perm/end_plan. Der Director treibt die Konstruktion, der ConcreteBuilder waehlt die Syntax.
//     * PlanTextBuilder = ConcreteBuilder — der --dump-plan-Traeger (deterministische Zeilen-Textform,
//       Contract-Test-Basis: 2 Laeufe byte-gleich). Fork-B des Bauplans (dritter trivialer PlanTextBuilder vs
//       Director-Methode) hier als eigener ConcreteBuilder entschieden — sauber fuer den Contract-Test.
//
//   Resolver-Vorstufe (STUFE §3.C/§4.1): der Director ANNOTIERT seinen Plan-Kopf mit den DREI Angebots-Quellen
//   (RegistryTrio: Organ/System/Mess, Ledger §28/§30 1:1 auf die Kettenstufen). Das ist die Vorstufe des
//   spaeteren LinkedExperimentPlan; hier NUR die Angebots-Herkunft (engine + Zaehlungen), noch kein Link.
//
// GOLDEN/ABI-NEUTRAL (Anti-Phantom, Bauplan §Golden/ABI-Neutralitaet):
//   * REINER Lese-/Enumerations-/Render-Schritt: KEIN DLL-Bau, KEINE Messung, KEIN run_lazy_static_then_dynamic,
//     KEINE CSV. opt/simd sind system_config => fliessen NIE in binary_id (Q2 Option C); der Plan beschreibt
//     nur die MESS-/BAU-Matrix. Der Host-ISA-Gate (system_axis_host_supports_simd) wird als PlanPerm-Annotation
//     mitgefuehrt, aber NICHT gefiltert (Resolver annotiert, entscheidet nicht — STUFE §3.C.3 E-RES-C) und
//     NICHT in den kanonischen --dump-plan-Text gerendert (=> der Plan-Text ist host-unabhaengig reproduzierbar).
//   * INERT-by-default: nichts ruft den Director ausser dem opt-in Contract-Test; kein Lauf-Pfad-Verhalten
//     geaendert (die golden Callees run_profile/run_experiment_profile sind unberuehrt).
//
// ⚠️ Katalog-/Umbrella-schwer (zieht ueber profile_run_entry.hpp den generierten Basis-Katalog + sota_catalog)
//    => gehoert in die HARNESS-/Fassaden-.cpp, NICHT in einen engine-agnostischen Treiber-Header. C++23, header-only.
// -----------------------------------------------------------------------------

#include "profile_run_entry.hpp" // system_axis_opt_flag_of / system_axis_march_of / system_axis_host_supports_simd
//   + profile_sweep_passes (via profile_runner.hpp) + project_experiment_to_sota_passes
//   (via sota_catalog.hpp) + cm::Default*Option + cx::ThesisProfile/XmlConfigParser
#include "validate_profile.hpp"    // RegistryTrio / RegistryContents (Registry-Trio-Annotation des Plan-Kopfs)
#include "planner/plan_legend.hpp" // W10-A: das dreistufige Legenden-Namensschema (EINE Formatierungs-Single-Source)
#include <cache_engine/measurement/run_methodology_registry.hpp> // S5-P1: RunMethodology-Registry (Build-Semantik-Single-Source)

#include "xml_config_parser/xml_config_parser.hpp" // cx::ExperimentProfile / cx::ThesisProfile (explizit)

#include <algorithm> // S5-P1: std::find ueber das A9.1-Feld run_methodology (Build-Semantik-Aufloesung)
#include <optional>  // S3 P-RESOLVER: der volle RegistryTrio als optionaler Director-Zustand (Resolver-Lauf)
#include <stdexcept> // R5: std::invalid_argument (exactly-one Kontraktbruch in build_semantic_of_run_methodology)
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace comdare::cache_engine::planner {

namespace cx  = ::comdare::builder::xml;
namespace cm  = ::comdare::cache_engine::measurement;
namespace tlz = ::comdare::cache_engine::thesis_lazy;

// ── Plan-Wertobjekte (POD, reine Beschreibung — kein Verhalten, keine Bau-Semantik). ────────────────────────

/// EINE Angebots-Registry-Quelle (Resolver-Vorstufe): Wurzel-engine + Zaehlungen. axis_count = Zahl der <axis>,
/// baustein_count = Summe aller <baustein name>-Angebote ueber die Achsen dieser Registry.
struct PlanRegistrySource {
    std::string engine; // comdare_axis_registry @engine (cache_engine / cache_engine_system / ..._measurement)
    std::size_t axis_count     = 0; // Zahl der Angebots-Achsen
    std::size_t baustein_count = 0; // Zahl der Angebots-Bausteine (Summe ueber die Achsen)
};

/// Der Plan-Kopf annotiert mit den DREI Angebots-Quellen (Ledger §28/§30: Mess->Planer / System->CEB / Organ->Tier).
struct PlanRegistryTrioAnnotation {
    PlanRegistrySource organ;          // engine="cache_engine" (Organ-Angebot der Tier-Stufe)
    PlanRegistrySource system;         // engine="cache_engine_system" (System-Angebot der CEB-Stufe)
    PlanRegistrySource measurement;    // engine="cache_engine_measurement" (Mess-Angebot der Planer-Stufe)
    bool               loaded = false; // true = alle 3 Registries gelesen und annotiert
};

/// EINE Mess-Achsen-Kombination (W10-A / §42, §47/§54-T2/§55): der AEUSSERSTE Walk-Schritt (Mess-Kombination ->
/// System-Perms -> Chunk-Buendel). Sie bestimmt den CEB-TYP. Die HAUPT-Auffaecherung [a,b,c] kommt aus der
/// Mess-Tooling-HAUPT-Achse {wallclock/macro/micro} (§47/§55: N Tooling-Konfigs -> N ceb:build:[a,b,c]-Strecken),
/// NICHT aus den 16 <measurement_categories> (die sind Mess-Tooling-UNTER = CSV-Spalten, §54-T2). `tooling` = die
/// Tooling-HAUPT-KONFIG dieser Kombination (leer = volles Angebot => `[all]`). `legend` = die kanonische
/// [a,b,c]-HAUPT-Kurzform (legend::measurement_tooling_combo). `categories` = die 16 <measurement_categories> als
/// UNTER (fuer die CSV-Spalten downstream; faechern den CEB-Typ NICHT auf). Heute typisch EINE Kombination (das
/// implizite volle Mess-System; die Mehr-Konfig-Auffaecherung ist XML-gated, s. measurement_combos_of).
struct PlanMeasurementCombo {
    std::size_t              index = 0;  // 0-basierter Kombinations-Index im deterministischen Walk
    std::vector<std::string> tooling;    // Mess-Tooling-HAUPT-KONFIG {wallclock/macro/micro} (leer = volles Angebot)
    std::vector<std::string> categories; // Mess-Tooling-UNTER: die 16 <measurement_categories> (CSV-Spalten, §54-T2)
    std::string              legend;     // kanonische [a,b,c]-HAUPT-Kurzform (legend::measurement_tooling_combo)
};

/// S5-P1 (P-VOLLZUG, 2026-07-20): die vom Planer aufgeloeste Build-/Mess-Semantik der S5-Mess-Strecke (die measure-
/// Methodik der run_methodology_registry). Der Tier-Emitter speist daraus CMAKE_BUILD_TYPE (Tier-Bau + Mess-Job)
/// und die 1-Thread-Politik (COMDARE_BUILD_PARALLEL im Mess-Job). Default = measure-Semantik (Release/misst/1-Thread)
/// => byte-identisch zum Vor-S5-tier:build. GOLDEN/binary_id-NEUTRAL: reine Bau-/Mess-Matrix, KEIN Stempel.
struct PlanBuildSemantic {
    std::string cmake_build_type = "Release"; // CMAKE_BUILD_TYPE des Tier-Baus/Mess-Baus (measure => "Release")
    bool        measurement_on   = true;      // misst das Profil (measure/debug: true; nur-release: false) -- S6-Konsum
    bool        single_thread    = true;      // 1-Thread-deterministischer Mess-Vollzug (Section 38.b)
};

/// Kopf des Plans: Provenienz (Quelle/Profil) + Perm-Zahl + Registry-Trio-Annotation.
struct PlanHeader {
    std::string source_kind; // "thesis" | "experiment"
    std::string profile_id;
    // S2-NACHT (2026-07-23): der Datei-BASENAME des AKTIVEN Profils (facade: profile_path.filename()). Der emittierte
    // Child-Prolog verdrahtet COMDARE_GOLDEN_N_PROFILE damit auf GENAU dieses Profil (thesis_profiles/<basename>) statt
    // hart all_axes_golden -- sonst exerzierten die Stufe-1/2-Jobs trotz Smoke-Scope den vollen all_axes-Katalog.
    // Quelle ist die DATEI, nicht profile_id (id != Basename moeglich, z.B. m3v2_sota_pilot.profile.xml traegt id="C").
    // Leer (Legacy-/Direkt-Ctor-Pfad ohne profile_path) => Prolog-Fallback all_axes_golden.profile.xml (byte-identisch
    // zu HEAD fuer die direkten Builder-Tests). KLASSEN-Regel bleibt: Prolog-Re-Derive mit frischem ${CI_PROJECT_DIR},
    // nur der Basename ist dynamisch.
    std::string profile_basename;
    std::size_t perm_count              = 0; // |opt x simd| JE Mess-Kombination
    std::size_t measurement_combo_count = 0; // W10-A: Zahl der Mess-Achsen-Kombinationen (heute typisch 1)
    PlanRegistryTrioAnnotation registries;
    // S3 P-RESOLVER (2026-07-20): der klassifizierte Organ-Position-Reject/Route-Report (resolve_axis_refs_against_
    // trio). resolved=false (INERT-Default) wenn der Director OHNE volles RegistryTrio konstruiert wurde; sonst
    // ok=true bei organ-reinem Profil (0 Rejects). binary_id-neutral -- reine Plan-Kopf-Annotation (kein Filter).
    tlz::ResolverReport resolver;
    // S5-P1: die aufgeloeste Build-/Mess-Semantik (measure-Methodik). Nur der Tier-Emitter (emit_batch_build_job /
    // emit_batch_measure_job) liest sie; die uebrigen Builder rendern sie NICHT (=> ihre Emission unveraendert). Default
    // (measure => Release) haelt den tier:build-Teil byte-identisch zu HEAD.
    PlanBuildSemantic build_semantic;
};

/// EINE opt x simd Permutation (system_config => NIE binary_id, NIE N; nur BAU-/MESS-Matrix + build_version-Suffix).
struct PlanPerm {
    std::size_t index = 0;  // 0-basierter Perm-Index im deterministischen Walk
    std::string opt_id;     // z.B. "O3"
    std::string simd_id;    // z.B. "no_extension" / "avx2"
    std::string opt_flag;   // aus system_axis_opt_flag_of (z.B. "-O3"); leer => Caller-Default (D1-Log-Fall)
    std::string march_flag; // aus system_axis_march_of (z.B. "-mavx2"); leer bei no_extension
    // build_version-Suffix OHNE +cxx= (compiler_tag ist Host-Provenienz, kein Planungs-Entscheid => der Plan
    // bleibt compiler-tag-agnostisch/reproduzierbar). Form: "+opt=<id>" bzw. "+opt=<id>+ext=<simd>".
    std::string build_version_suffix;
    // ANNOTATION (kein Filter): ob der Bau-/Mess-Host diese SIMD-Erweiterung bietet. Wird NICHT in den
    // kanonischen Plan-Text gerendert (host-unabhaengige Reproduzierbarkeit), steht aber jedem Builder offen.
    bool host_supports_simd = true;
};

/// EIN Walk-Schritt unter einer Perm. Deckt beide Kanaele mit stabilen "-"/"<basis>"-Fuellwerten ab.
struct PlanStep {
    std::size_t index = 0; // 0-basierter Schritt-Index INNERHALB der aktuellen Perm
    std::string kind;      // "thesis_sweep_pass" | "experiment_phase_pass"
    std::string label;     // thesis: Sweep-Achse ("" = Basis-Pass) ; experiment: Phasen-Name
    std::string merge;     // experiment: MergeStrategy ; thesis: "-"
    std::string
        binary_id;      // experiment: view_binary_id ; thesis: "-" (Basis-binary_ids entstehen erst in der Selektion)
    std::string series; // experiment: Reihe A/B ; thesis: "-"
    std::string pruefling_type; // experiment: full/abstract ; thesis: "-"
    std::string lebewesen;      // experiment: das SOTA-Lebewesen ; thesis: "-"
};

// ── IPlanBuilder — das GoF-Builder-Interface: der Director treibt, der ConcreteBuilder waehlt die Syntax. ────
class IPlanBuilder {
public:
    virtual ~IPlanBuilder()                           = default;
    virtual void begin_plan(PlanHeader const& header) = 0;
    virtual void begin_perm(PlanPerm const& perm)     = 0;
    virtual void on_step(PlanStep const& step)        = 0;
    virtual void end_perm(PlanPerm const& perm)       = 0;
    virtual void end_plan(PlanHeader const& header)   = 0;
    // W10-A (§42): die AEUSSERE Mess-Achsen-Stufe. DEFAULT-No-Op, damit die bestehenden ConcreteBuilder
    // (PlanTextBuilder) und struktur-zaehlende Builder unveraendert bleiben (additiv). Die Stufen-Builder
    // (CiYamlBuilder Stufe 1 / TierCiYamlBuilder Stufe 2) ueberschreiben sie und emittieren an dieser Ebene.
    virtual void begin_measurement_combo(PlanMeasurementCombo const& /*combo*/) {}
    virtual void end_measurement_combo(PlanMeasurementCombo const& /*combo*/) {}
};

// ── PlanTextBuilder — ConcreteBuilder + der --dump-plan-Traeger. Deterministische Zeilen-Textform. ──────────
//    Format (stabil, byte-reproduzierbar; keine host-abhaengigen Felder):
//      # comdare-experiment-plan v1
//      source_kind=<thesis|experiment>
//      profile_id=<id>
//      registry_trio loaded=<0|1> organ=<engine> organ_axes=<n> organ_offers=<n> system=... measurement=...
//      perm_count=<n>
//      perm <i> opt=<id> simd=<id> opt_flag=<f> march_flag=<f> build_version_suffix=<s>
//        step <j> kind=<k> label=<l> merge=<m> binary_id=<b> series=<s> pruefling_type=<p> lebewesen=<le>
class PlanTextBuilder final : public IPlanBuilder {
public:
    void begin_plan(PlanHeader const& h) override {
        out_ += "# comdare-experiment-plan v1\n";
        out_ += "source_kind=" + h.source_kind + "\n";
        out_ += "profile_id=" + h.profile_id + "\n";
        out_ += "registry_trio loaded=" + std::string(h.registries.loaded ? "1" : "0") +
                " organ=" + nz(h.registries.organ.engine) +
                " organ_axes=" + std::to_string(h.registries.organ.axis_count) +
                " organ_offers=" + std::to_string(h.registries.organ.baustein_count) +
                " system=" + nz(h.registries.system.engine) +
                " system_axes=" + std::to_string(h.registries.system.axis_count) +
                " system_offers=" + std::to_string(h.registries.system.baustein_count) +
                " measurement=" + nz(h.registries.measurement.engine) +
                " measurement_axes=" + std::to_string(h.registries.measurement.axis_count) +
                " measurement_offers=" + std::to_string(h.registries.measurement.baustein_count) + "\n";
        out_ += "measurement_combo_count=" + std::to_string(h.measurement_combo_count) + "\n";
        out_ += "perm_count=" + std::to_string(h.perm_count) + "\n";
        // S3 P-RESOLVER: der Organ-Position-Reject/Route-Report sichtbar im --dump-plan (INERT-Default: resolved=0).
        out_ += "resolver resolved=" + std::string(h.resolver.resolved ? "1" : "0") +
                " ok=" + std::string(h.resolver.ok ? "1" : "0") +
                " rejects=" + std::to_string(h.resolver.rejects.size()) + "\n";
        for (auto const& rj : h.resolver.rejects)
            out_ += "  reject code=" + rj.code + " ref=" + rj.ref + " message=" + rj.message + "\n";
    }
    // W10-A: die aeussere Mess-Achsen-Stufe im Plan-Text sichtbar machen (dreistufige Topologie).
    void begin_measurement_combo(PlanMeasurementCombo const& c) override {
        out_ += "measurement_combo " + std::to_string(c.index) + " legend=" + c.legend + "\n";
    }
    void begin_perm(PlanPerm const& p) override {
        out_ += "perm " + std::to_string(p.index) + " opt=" + nz(p.opt_id) + " simd=" + nz(p.simd_id) +
                " opt_flag=" + nz(p.opt_flag) + " march_flag=" + nz(p.march_flag) +
                " build_version_suffix=" + nz(p.build_version_suffix) + "\n";
    }
    void on_step(PlanStep const& s) override {
        out_ += "  step " + std::to_string(s.index) + " kind=" + nz(s.kind) +
                " label=" + (s.label.empty() ? std::string{"<basis>"} : s.label) + " merge=" + nz(s.merge) +
                " binary_id=" + nz(s.binary_id) + " series=" + nz(s.series) +
                " pruefling_type=" + nz(s.pruefling_type) + " lebewesen=" + nz(s.lebewesen) + "\n";
    }
    void end_perm(PlanPerm const&) override {}
    void end_plan(PlanHeader const&) override {}

    [[nodiscard]] std::string const& text() const noexcept { return out_; }

private:
    // Leerer String -> "-" (stabile, eindeutige Feldtrennung; kein Feld bleibt leer im Zeilen-Text).
    [[nodiscard]] static std::string nz(std::string const& s) { return s.empty() ? std::string{"-"} : s; }
    std::string                      out_;
};

// ── CMakeGraphBuilder — STUFE-1-Emitter (Planer-Rolle, PAKET W10-A / §42, ersetzt die W7-B-Zell-Direktbau-Sicht).
//    Emittiert ein deterministisches experiment_plan.cmake der MESS-ACHSEN-Stufe: je Mess-Kombination [a,b,c]
//    (aus der Anwender-XML) EIN CEB-Bau-Target + EIN CEB-Emit-Target, das die CEB SELBST die STUFE-2-Sicht
//    emittieren laesst (--emit-tier-cmake). Der Bare-Metal-Bau ist damit DREISTUFIG (§42: „das Konzept der
//    lokalen Compile" identisch korrigiert): --dump-cmake -> Stufe-1 (CEB) -> --emit-tier-cmake -> Stufe-2
//    (Tier-Binaries). Der REALE provision-only-Tier-Bau lebt in der Stufe-2 (TierCmakeGraphBuilder), nicht hier.
//
//    Host-unabhaengig: Treiber/Profil sind CMake-Variablen mit Defaults (per -D ueberschreibbar), nur die
//    [a,b,c]-Legenden sind Plan-Konstanten. Isomorph zum Director-Walk (perms()/steps_per_perm() = Zeuge).
class CMakeGraphBuilder final : public IPlanBuilder {
public:
    void begin_plan(PlanHeader const& h) override {
        header_ = h;
        out_ += "# comdare-experiment-plan (generated .cmake blueprint, CMakeGraphBuilder v2 = STUFE 1)\n";
        out_ += "# source_kind=" + h.source_kind + " profile_id=" + h.profile_id +
                " measurement_combo_count=" + std::to_string(h.measurement_combo_count) +
                " perm_count=" + std::to_string(h.perm_count) +
                " registry_trio_loaded=" + std::string(h.registries.loaded ? "1" : "0") + "\n";
        out_ += "#\n";
        out_ += "# Ledger §42: STUFE 1 (Mess-Achsen-Stufe) -- je Mess-Kombination [a,b,c] EIN CEB-Bau-Target +\n";
        out_ += "# EIN CEB-Emit-Target (--emit-tier-cmake => Stufe-2-Plan). Dreistufiger Bare-Metal-Bau:\n";
        out_ += "#   cmake --build <dir> --target comdare_ceb_emit_<combo>  # emittiert tier_plan_<combo>.cmake\n";
        out_ += "#   (dann Stufe-2: cmake -P/Konfiguration dieses tier_plan + --build comdare_tier_batch_amd)\n";
        out_ += "# Konfigurierbare Eingaben (per -D ueberschreibbar):\n";
        out_ += "#   COMDARE_PLAN_DRIVER   = Pfad/Name des comdare-messung-driver / CEB (Default: PATH-Suche)\n";
        out_ += "#   COMDARE_PLAN_PROFILE  = Thesis-/Experiment-Profil-XML (Default: leer => Treiber-Default-Profil)\n";
        out_ += "#   COMDARE_PLAN_TIER_OUT = Ausgabe-Wurzel fuer die emittierten Stufe-2-cmake (Default: "
                "<bindir>/tier_plans)\n";
        out_ += "if(NOT DEFINED COMDARE_PLAN_DRIVER)\n";
        out_ += "    set(COMDARE_PLAN_DRIVER \"comdare-messung-driver\")\n";
        out_ += "endif()\n";
        out_ += "if(NOT DEFINED COMDARE_PLAN_PROFILE)\n";
        out_ += "    set(COMDARE_PLAN_PROFILE \"\")\n";
        out_ += "endif()\n";
        out_ += "if(NOT DEFINED COMDARE_PLAN_TIER_OUT)\n";
        out_ += "    set(COMDARE_PLAN_TIER_OUT \"${CMAKE_CURRENT_BINARY_DIR}/tier_plans\")\n";
        out_ += "endif()\n";
    }
    // STUFE 1 haengt an der Mess-Achsen-Ebene: je Mess-Kombination CEB-Bau + CEB-Emit(--emit-tier-cmake).
    void begin_measurement_combo(PlanMeasurementCombo const& c) override {
        combos_.push_back(c);
        std::string const slug    = legend::cmake_slug(c.legend);
        std::string const stemdir = "${CMAKE_CURRENT_BINARY_DIR}/experiment_plan";
        std::string const bstamp  = stemdir + "/ceb_" + slug + ".build.stamp";
        std::string const tierpl  = "${COMDARE_PLAN_TIER_OUT}/tier_plan_" + slug + ".cmake";
        out_ += "\n# --- measurement_combo " + std::to_string(c.index) + ": " + c.legend + " (CEB-Typ) ---\n";
        // CEB-Bau-Target (STUFE 1): der CEB (=comdare-messung-driver) ist die Voraussetzung. Echo-Acknowledgment
        // (der Treiber wird ueber COMDARE_PLAN_DRIVER hereingereicht bzw. vom aeusseren cmake gebaut).
        out_ += "if(NOT TARGET " + ceb_build_target(slug) + ")\n";
        out_ += "    add_custom_command(\n";
        out_ += "        OUTPUT \"" + bstamp + "\"\n";
        out_ += "        COMMAND \"${CMAKE_COMMAND}\" -E make_directory \"" + stemdir + "\"\n";
        out_ += "        COMMAND \"${CMAKE_COMMAND}\" -E echo \"ceb:build " + c.legend +
                " (CEB-Typ = comdare-messung-driver)\"\n";
        out_ += "        COMMAND \"${CMAKE_COMMAND}\" -E touch \"" + bstamp + "\"\n";
        out_ += "        COMMENT \"ceb:build " + c.legend + "\"\n";
        out_ += "        VERBATIM)\n";
        out_ += "    add_custom_target(" + ceb_build_target(slug) + " DEPENDS \"" + bstamp + "\")\n";
        out_ += "endif()\n";
        // CEB-Emit-Target (STUFE 1->2): die GEBAUTE CEB emittiert ihre STUFE-2-Sicht (--emit-tier-cmake). CEB-Hoheit
        // (§40.b). DEPENDS auf den CEB-Bau-Stamp (Bau->Emit-Kante). Host-unabhaengig (Treiber/Profil = Variablen).
        out_ += "if(NOT TARGET " + ceb_emit_target(slug) + ")\n";
        out_ += "    add_custom_command(\n";
        out_ += "        OUTPUT \"" + tierpl + "\"\n";
        out_ += "        COMMAND \"${CMAKE_COMMAND}\" -E make_directory \"${COMDARE_PLAN_TIER_OUT}\"\n";
        out_ += "        COMMAND \"${COMDARE_PLAN_DRIVER}\" --emit-tier-cmake \"${COMDARE_PLAN_PROFILE}\" > \"" +
                tierpl + "\"\n";
        out_ += "        DEPENDS \"" + bstamp + "\" # ceb:build->ceb:emit-Kante\n";
        out_ += "        COMMENT \"ceb:emit " + c.legend + " (--emit-tier-cmake => Stufe-2-Plan)\"\n";
        out_ += "        VERBATIM)\n";
        out_ += "    add_custom_target(" + ceb_emit_target(slug) + " DEPENDS \"" + tierpl + "\")\n";
        out_ += "endif()\n";
    }
    void begin_perm(PlanPerm const& p) override {
        perms_.push_back(p);
        steps_per_perm_.emplace_back();
    }
    void on_step(PlanStep const& s) override { steps_per_perm_.back().push_back(s); }
    void end_perm(PlanPerm const&) override {}
    void end_plan(PlanHeader const&) override {
        // Aggregat-Target: alle CEB-Emit-Targets (=> transitiv alle CEB-Bau-Targets via Bau->Emit-Kante).
        out_ += "\n# Aggregat: alle ceb:emit-Targets (transitiv alle ceb:build-Targets via DEPENDS-Kante).\n";
        out_ += "if(NOT TARGET " + all_target() + ")\n";
        out_ += "    add_custom_target(" + all_target() + " DEPENDS";
        for (auto const& c : combos_) out_ += "\n        " + ceb_emit_target(legend::cmake_slug(c.legend));
        out_ += ")\n";
        out_ += "endif()\n";
    }

    [[nodiscard]] std::string const&                        text() const noexcept { return out_; }
    [[nodiscard]] PlanHeader const&                         header() const noexcept { return header_; }
    [[nodiscard]] std::vector<PlanPerm> const&              perms() const noexcept { return perms_; }
    [[nodiscard]] std::vector<std::vector<PlanStep>> const& steps_per_perm() const noexcept { return steps_per_perm_; }

private:
    [[nodiscard]] static std::string ceb_build_target(std::string const& slug) { return "comdare_ceb_build_" + slug; }
    [[nodiscard]] static std::string ceb_emit_target(std::string const& slug) { return "comdare_ceb_emit_" + slug; }
    [[nodiscard]] static std::string all_target() { return "comdare_experiment_plan_all"; }

    PlanHeader                         header_;
    std::vector<PlanMeasurementCombo>  combos_;
    std::vector<PlanPerm>              perms_;
    std::vector<std::vector<PlanStep>> steps_per_perm_;
    std::string                        out_;
};

// SIMD-Capability-Routing (Pilot-Matrix §36.3): no_extension->amd64, avx2->avx2, avx512->avx512. Ein unbekannter
// SIMD-Wert routet auf seinen eigenen Namen als Tag (die Infra taggt Nodes nach realer CPU-Faehigkeit) --
// deterministisch, kein stiller Fallback auf einen falschen Tag. GETEILT zwischen Stufe-1 (CiYamlBuilder) und
// Stufe-2 (TierCiYamlBuilder) -- EINE Routing-Single-Source.
[[nodiscard]] inline std::string simd_runner_tag(std::string const& simd_id) {
    if (simd_id.empty() || simd_id == "no_extension") return "amd64";
    return simd_id; // avx2 -> "avx2", avx512 -> "avx512", sonst der ISA-Name selbst
}

// A3 (Task #23/#24, Manager-Ruling Weg a = "Tags sind die REAL gebauten Flags"): die EHRLICHE flag-granulare
// Runner-Tag-LISTE. Waehrend simd_runner_tag() EINEN groben ISA-Namen liefert, leitet diese Liste die Tags aus den
// REAL an den Compiler gereichten -march-Flags ab (system_axis_march_of -- die SELBE Single-Source, aus der
// PlanPerm::march_flag entsteht): je "-m<flag>"-Token EIN Tag (fuehrendes "-m" gestrippt). So taggt die Infra
// einen Node nach seiner ECHTEN Maschinen-Faehigkeit und GitLab wertet die Liste als UND-Bedingung (der Job laeuft
// NUR auf einem Runner, der ALLE gelisteten Tags traegt). Heute ein Element je simd: no_extension/leer -> {"amd64"};
// avx2 -> {"avx2"} (aus -mavx2); avx512 -> {"avx512f"} (aus -mavx512f, static_assert simd_sub_axis.hpp:107 --
// NICHT "avx512"). Unbekannte simd-id (march leer, aber nicht no_extension) -> {simd_id} (ISA-Name selbst,
// deckungsgleich zu simd_runner_tag; kein stiller Fallback auf einen falschen Tag). Die Liste WAECHST automatisch
// mit, sobald A7/§40.a die Flag-Signatur anreichert (z.B. avx512 -> "-mavx512f -mavx512dq" => {"avx512f",
// "avx512dq"}). GETEILT zwischen Stufe-1 (CiYamlBuilder) und Stufe-2 (TierCiYamlBuilder) -- EINE Routing-Single-Source.
[[nodiscard]] inline std::vector<std::string> simd_runner_tags(std::string const& simd_id) {
    if (simd_id.empty() || simd_id == "no_extension") return {"amd64"};
    std::string const march = tlz::system_axis_march_of(simd_id);
    if (march.empty()) return {simd_id}; // unbekannte id => ISA-Name selbst (kein falscher Tag)
    std::vector<std::string> tags;
    std::size_t              pos = 0;
    while (pos < march.size()) { // an Leerzeichen splitten; je Token fuehrendes "-m" strippen
        std::size_t const sp    = march.find(' ', pos);
        std::size_t const end   = (sp == std::string::npos) ? march.size() : sp;
        std::string_view  token = std::string_view{march}.substr(pos, end - pos);
        if (token.substr(0, 2) == "-m") token.remove_prefix(2); // -mavx2 -> avx2, -mavx512f -> avx512f
        if (!token.empty()) tags.push_back(std::string(token));
        pos = (sp == std::string::npos) ? march.size() : sp + 1;
    }
    return tags;
}

// (h)/(k) §61-MODI/§61-KONSOLIDIERUNG(d) Multi-Maschinen-Pflicht: die deterministische HOST-LANE eines Mess-Jobs
// (per-Host-resource_group + Runner-Tag, statt der Vor-§61 globalen ceb-measurement-exclusive-Serialisierung --
// "tagelanges Warten"). Cluster-Inventar (MATRIX 20260719-cluster-parallel-build Z.24-42): prod1=Zen5=EINZIGER
// avx512-Node (Tag "amd"); prod2=RaptorLake=avx2 (Tag "intel"). Regel: (1) avx512* -> amd (Hardware-Zwang, nur prod1
// kann es); (2) avx2 -> intel (Standard-Routing prod2; schlaegt die Combo); (3) no_extension (kein march) -> die
// MESS-TOOLING-COMBO entscheidet die Lane (F-4-Aufloesung): macro -> intel (prod2), wallclock/micro/[all] -> amd
// (prod1). OHNE (3) laegen reine no_extension-Laeufe (F-4-320er) KOMPLETT auf prod1 = Multi-Maschinen-Verstoss. JEDE
// Combo-Lane bleibt VOLLSTAENDIG auf EINER Maschine (Vergleichbarkeit); avx512 NIE auf intel. Plattform-Provenienz
// via platform-Tag im CSV (Cross-Combo-Overhead ist bei no_extension plattform-konfundiert, Replay-Nachmessung
// moeglich). Exklusivitaet gilt PRO Maschine (resource_group ceb-measure-<host>), nicht mehr global.
[[nodiscard]] inline std::string measure_host_lane(std::string const& simd_id, std::string const& combo_legend) {
    std::string const march = tlz::system_axis_march_of(simd_id);
    if (march.find("avx512") != std::string::npos) return "amd"; // (1) Hardware-Zwang: nur prod1/Zen5 traegt avx512
    if (simd_id == "avx2") return "intel"; // (2) Standard-Routing prod2/RaptorLake (schlaegt Combo)
    // (3) no_extension: die Combo entscheidet -- macro -> intel (prod2), wallclock/micro/[all] -> amd (prod1).
    if (combo_legend.find("macro") != std::string::npos) return "intel";
    return "amd";
}

// §62-B-K-Budget Single-Source (S4, 2026-07-23): die harte parallele Compile-Zahl (COMDARE_BUILD_PARALLEL) je
// Host-Lane des Build+Mess-Batches. COMDARE_BUILD_PARALLEL ist der Bau-Pool-WORKER-Override ("harte parallele
// Compile-Zahl", profile_run_entry.hpp:132-135) -- deshalb die §62-B-K-BUDGETS 24 (prod1=amd) / 16 (prod2=intel),
// NICHT die T-Werte 32/24 (die waeren ein §62-B-Budget-Verstoss: T ist die Node-Thread-Freigabe, K der Compile-
// Worker-Deckel). Voll ausschoepfbar, weil P4 (geteilte resource_group ceb-measure-<host>) je Maschine nur EINEN
// Batch gleichzeitig laufen laesst; das MESSEN selbst bleibt 1-Thread (der run_profile-Loop). GETEILTE Single-Source
// fuer beide Stufen (TierCiYamlBuilder Build-/Mess-Batch + TierCmakeGraphBuilder bare-metal-Batch).
[[nodiscard]] inline std::size_t lane_build_parallelism(std::string const& host) { return host == "amd" ? 24 : 16; }

// A3: EINE Formatierungs-Single-Source der YAML-Tag-Sequenz ["a","b",...] (doppelte Quotes, komma-getrennt, kein
// Leerraum) -- GitLab wertet die Liste als UND-Bedingung: der Runner muss ALLE Tags tragen. Ein-Element-Listen
// rendern byte-identisch zur alten tags: ["x"]-Emission (=> golden-neutral fuer die no_extension/avx2-Live-Strecke).
[[nodiscard]] inline std::string yaml_tag_list(std::vector<std::string> const& tags) {
    std::string s = "[";
    for (std::size_t i = 0; i < tags.size(); ++i) {
        if (i != 0) s += ",";
        s += "\"" + tags[i] + "\"";
    }
    s += "]";
    return s;
}

// §62-B-Bestandslog-Korn (S4, 2026-07-23): die Scheiben-Groesse, in der EIN Build+Pruef-Batch je Host das
// [0,COMDARE_GN_TOTAL)-Fenster INTERN durchlaeuft (4096er-Scheiben => bei 2^17 genau 32 Scheiben je Perm). HARTE
// inline-Konstante mit BEWUSST KEINEM Env-Override: das verworfene COMDARE_GN_BATCH_SLICE waere ein §61-Verstoss --
// die Env wuerde einen Methodik-nahen WERT waehlen (kein Profil) und die einheitliche Bestandslog-Saat (#46b)
// gefaehrden. Das 4096er-Korn ist der Wiederaufnahme-Takt (Abbruch/Retry setzt an der ersten unfertigen Scheibe
// fort) und der Testat-Rhythmus im Trace.
//
// DEPRECATED-Doku (§56/§57 nie loeschen): die frueheren kTierChunkCount=4 Chunk-Bau-Jobs je System-Perm (Pilot-
// Matrix GN_CHUNK 0..3, super/.gitlab-ci.yml) buendelten das kombinierte System-Freigabe-Durchfuehrungs- x
// Organ-Bau-Volumen als chunk<k> -- O(Perms x Chunks) war INTERIM (W4/INC-G6, vor dem §62-B-Gesetz). Ersetzt durch
// die O(Maschinen)-Batch-Emission (kGnBatchSlice-Scheiben INNERHALB EINES Batch-Jobs je Host).
inline constexpr std::size_t kGnBatchSlice = 4096;

// W10-Nacharbeit (§42, Serie-E2E 11562/11566): die dynamischen Child-Pipelines ERBEN die globalen Parent-Variablen
// NICHT (self-contained). Ohne die ccache-Konfiguration scheitert der CEB-/Tier-Bau am Runner an
// `ccache: Failed to create directory /.ccache/lock: Permission denied` (ccache faellt auf $HOME/.ccache zurueck,
// das im Runner-Container nicht schreibbar ist). Diese Naht spiegelt EXAKT den Parent (super/.gitlab-ci.yml:
// CCACHE_DIR/MAXSIZE + CMAKE_BUILD_PARALLEL_LEVEL + top-level cache: key "ccache-$CI_PROJECT_NAME") -- EINE
// Single-Source fuer BEIDE Stufen (CiYamlBuilder Stufe 1 + TierCiYamlBuilder Stufe 2). NUR diese Variablen (die
// Submodul-Mechanik laeuft nachweislich ueber den REV-17-Deploy-Token). Der gleiche cache-Key wie der Parent =>
// der Warm-ccache-Bestand zieht auch im Child (CI_PROJECT_NAME/CI_PROJECT_DIR sind im Child dieselben).
inline void emit_child_ccache_config(std::string& out) {
    // (a) ccache-/Parallel-Variablen (unter dem bereits geoeffneten variables:-Block). NUR reine Literale --
    //     CCACHE_DIR ("$CI_PROJECT_DIR/.ccache") gehoert NICHT hierher (W10-Nacharbeit 4, KLASSE): es wird
    //     ausschliesslich per Runtime-Shell-Export im Prolog gesetzt (die vererbte Parent-Vorexpansion
    //     ueberschriebe die Child-Def sonst versions-/wegabhaengig -> leeres /.ccache).
    out += "  CCACHE_MAXSIZE: \"3G\"\n";
    out +=
        "  CMAKE_BUILD_PARALLEL_LEVEL: \"6\"                 # Parent-Spiegel: Runner-Core-Budget (nicht -j nproc)\n";
    // (b) top-level cache:-Block (gleicher Key wie Parent -> Warm-ccache zieht auch im Child). paths ist workdir-
    //     relativ (.ccache), unabhaengig von der $CI_PROJECT_DIR-Frage; der Key nutzt $CI_PROJECT_NAME (Cache-
    //     System-Expansion, nicht $CI_PROJECT_DIR).
    out += "cache:\n";
    out += "  key: \"ccache-$CI_PROJECT_NAME\"\n";
    out += "  paths: [\".ccache\"]\n";
}

// W10-Nacharbeit 2 (§42, Serie-E2E 11569/11576): die dynamischen Child-Pipelines erben `default:before_script`
// NICHT -> ohne diesen PROLOG kompilieren die Bau-Jobs gegen STALE ce-Quellen frueherer Runner-Jobs (der
// Child-Trace meldet `Skipping Git submodules setup`). Diese Naht spiegelt EXAKT den Parent-Klon-Mechanismus
// (super/.gitlab-ci.yml default:before_script, REV-15/17): der Runner-Auto-Fetch failt am extraheader, DESHALB
// GIT_SUBMODULE_STRATEGY:none + MANUELLER Klon mit Deploy-Token. Der Token steht NUR als CI-Variablen-Referenz
// ($CE_SUBMODULE_USER/$CE_SUBMODULE_TOKEN -- projekt-weit/unprotected, auch im Child verfuegbar), NIE im
// Klartext (=> Byte-Determinismus + kein Leak). Auf ce + prt-art PFAD-GESKOPT (die Overleaf-Thesis braucht der
// C++-Bau nicht); --recursive holt ce's nested public-github-Submodul (concurrentqueue); --force + sync auf den
// GEPINNTEN gitlink-SHA (idempotent auf stalem Workdir). EINE Single-Source fuer alle Bau-Jobs beider Stufen.
//
// W10-Nacharbeit 3+4 (§42, Serie-E2E 11586/Lauf 4, prod1-Klasse): der Prolog setzt ALLE $CI_PROJECT_DIR-abhaengigen
// Env-Variablen per RUNTIME-SHELL-EXPORT (CCACHE_DIR, COMDARE_GOLDEN_N_PROFILE). Die YAML-variables-Definition
// allein reicht NICHT: die gitlab-seitig VOREXPANDIERTE Parent-globale Variable wird als Pipeline-Variable an das
// Child vererbt und ueberschreibt die Child-YAML-Definition versions-/vererbungsabhaengig -> leer expandiertes
// $CI_PROJECT_DIR (/.ccache -> Permission denied; /Code/external/... -> "profile fehlt"). Der Shell-Export zur
// Laufzeit ist immun gegen jede GitLab-Expansions-/Vererbungs-/Runner-Versions-Semantik und schlaegt die vererbte
// Env-Variable (er laeuft in der Job-Shell VOR jedem cmake-/Treiber-Aufruf). KLASSEN-Regel: KEIN $CI_PROJECT_DIR-
// Wert steht mehr in variables: (Contract-Test-Wache); nur reine Literale + der cache:-Block (workdir-relativ)
// bleiben dort.
// S2-NACHT (2026-07-23): profile_basename = der Datei-Basename des AKTIVEN Profils (PlanHeader::profile_basename). Der
// COMDARE_GOLDEN_N_PROFILE-Export zeigt damit auf GENAU dieses Profil (thesis_profiles/<basename>), statt hart
// all_axes_golden -- so exerzieren die von der CEB emittierten Stufe-1/2-Jobs den scope-richtigen Katalog (Smoke bleibt
// Smoke). Leerer Basename (Legacy-/Direkt-Ctor-Pfad ohne profile_path) => Fallback all_axes_golden.profile.xml
// (byte-identisch zu HEAD). KLASSEN-Regel bleibt: Re-Derive mit frischem ${CI_PROJECT_DIR}, nur der Basename dynamisch.
inline void emit_child_submodule_prolog(std::string& out, std::string const& profile_basename) {
    std::string const golden_basename =
        profile_basename.empty() ? std::string{"all_axes_golden.profile.xml"} : profile_basename;
    out += "    - |\n";
    out += "      set -euo pipefail\n";
    out += "      # RUNTIME-Shell-Export aller $CI_PROJECT_DIR-abhaengigen Env (Nacharbeit 3+4, KLASSE): immun gegen\n";
    out += "      # die GitLab-variables-Vorexpansion/-Vererbung (die vorexpandierte Parent-Def ueberschriebe sonst\n";
    out += "      # versions-/wegabhaengig die Child-Def -> leer expandiert -> /.ccache bzw. /Code/...-fehlt).\n";
    out += "      export CCACHE_DIR=\"${CI_PROJECT_DIR}/.ccache\"\n";
    out += "      export CCACHE_MAXSIZE=\"3G\"\n";
    out += "      export COMDARE_GOLDEN_N_PROFILE=\"${CI_PROJECT_DIR}/Code/external/comdare-cache-engine/libs/"
           "cache_engine/algorithm_profiles/thesis_profiles/" +
           golden_basename + "\"\n";
    // smoke=>debug-Entkopplung (2026-07-22): COMDARE_PLAN_METHODIK_PROFILE (Methodik-Profil-Selektor) analog FRISCH
    // per Runtime-Export montieren. Die super-YAML forwardet NUR den BASENAME (KLASSE: KEIN $CI_PROJECT_DIR in den
    // geforwardeten Child/Grandchild-variables -- sonst leer-vorexpandiert). Der Basename wird hier zum grandchild-
    // lokalen Voll-Pfad unter thesis_profiles/ mit frischem ${CI_PROJECT_DIR} -- gilt fuer den CEB-emit (build_semantik
    // = debug) UND den Grandchild-Mess-Run (parallel messen). IDEMPOTENT + Dual-Weg: leer => No-Op (kein Override =>
    // byte-identisch); absolute /*-Pfade (bare-metal: lokaler Voll-Pfad steht direkt in der Env) bleiben unangetastet.
    out += "      case \"${COMDARE_PLAN_METHODIK_PROFILE:-}\" in\n";
    out += "        '') : ;;\n";
    out += "        /*) : ;;\n";
    out += "        *) export COMDARE_PLAN_METHODIK_PROFILE=\"${CI_PROJECT_DIR}/Code/external/comdare-cache-engine/"
           "libs/cache_engine/algorithm_profiles/thesis_profiles/${COMDARE_PLAN_METHODIK_PROFILE}\" ;;\n";
    out += "      esac\n";
    out += "      # CHILD-SUBMODULE-KLON (W10-Nacharbeit 2): Parent-Spiegel, Deploy-Token via CI-Variablen (NIE "
           "Klartext).\n";
    out += "      if [ -f .gitmodules ] && git rev-parse --is-inside-work-tree >/dev/null 2>&1; then\n";
    out += "        for spec in \"Code/external/comdare-cache-engine:comdare-cache-engine\" "
           "\"Code/external/comdare-prt-art:comdare-prt-art\"; do\n";
    out += "          p=\"${spec%%:*}\"; r=\"${spec##*:}\"\n";
    out += "          git config -f .gitmodules \"submodule.${p}.url\" "
           "\"https://${CE_SUBMODULE_USER}:${CE_SUBMODULE_TOKEN}@${CI_SERVER_HOST}/comdare/research/${r}.git\"\n";
    out += "        done\n";
    out += "        git submodule sync --recursive -- Code/external/comdare-cache-engine "
           "Code/external/comdare-prt-art\n";
    out += "        git submodule update --init --recursive --force -- Code/external/comdare-cache-engine "
           "Code/external/comdare-prt-art\n";
    out += "        git checkout -- .gitmodules 2>/dev/null || true\n";
    out += "      fi\n";
}

// ── CiYamlBuilder — STUFE-1-Emitter (Planer-Rolle, PAKET W10-A / §42, ersetzt die W7-A-Zweistufigkeit). ───────
//    Emittiert eine deterministische GitLab-Child-Pipeline-YAML: die MESS-ACHSEN-Stufe der CE-gesteuerten Kette.
//    Je Mess-Achsen-Kombination [a,b,c] (aus der Anwender-XML, <measurement_categories>) EINE dynamische
//    CEB-Pipeline:
//      "ceb:build:[a,b,c]"   -> baut die CEB fuer diesen CEB-Typ (Messsystem).
//      "ceb:emit:[a,b,c]"    -> die GEBAUTE CEB emittiert SELBST Child-2 (--emit-tier-ci, §40.b-Praezisierung:
//                               Planer steuert CEB-Jobs, CEB steuert Tier-Jobs) aus ihren einkompilierten
//                               System-Achsen-Freigaben. Heute EINE Binary in zwei Rollen (Planer-Rolle --dump-ci
//                               vs CEB-Rolle --emit-tier-ci; ehrlich dokumentiert, kein Schein-Split).
//      "ceb:trigger:[a,b,c]" -> Grandchild-Trigger der Child-2-YAML (trigger: include: artifact:).
//
//    Der Director-Walk ist dreistufig (Mess-Kombination -> System-Perm -> Chunk-Buendel); die STUFE-1-Emission
//    haengt an der Mess-Kombinations-Ebene (begin_measurement_combo). Die System-Perms [d,e,f] und die
//    Tier-Chunk-/Mess-Jobs gehoeren in die STUFE-2-Sicht (TierCiYamlBuilder, CEB-Rolle) -- NICHT hierher.
//
//    GITLAB-NESTING: super(parent) -> diese YAML(child, planer:delegate) -> CEB-emittierte Child-2(grandchild)
//    (max. 2 Ebenen tiefe Verschachtelung -- genau ausgeschoepft). GOLDEN/HOST-NEUTRAL: reine Text-Emission,
//    KEIN Bau/Messung im Builder; nur CI-Variablen + [a,b,c]-Legenden als LITERALE => byte-deterministisch.
//    perms()/steps_per_perm() bleiben aufgezeichnet (struktureller Zeuge fuer den Isomorphie-Contract-Test).
class CiYamlBuilder final : public IPlanBuilder {
public:
    void begin_plan(PlanHeader const& h) override {
        header_ = h;
        out_ += "# comdare dynamic planer child-pipeline (CiYamlBuilder v2, STUFE 1 = Mess-Achsen-Stufe) -- "
                "GENERIERT, deterministisch, host-unabhaengig.\n";
        out_ += "# source_kind=" + h.source_kind + " profile_id=" + h.profile_id +
                " measurement_combo_count=" + std::to_string(h.measurement_combo_count) +
                " perm_count=" + std::to_string(h.perm_count) +
                " registry_trio_loaded=" + std::string(h.registries.loaded ? "1" : "0") + "\n";
        out_ += "#\n";
        out_ += "# Ledger §42: CE erhaelt die XML und steuert ALLES. DREISTUFIGE Legenden-Kette --\n";
        out_ += "#   STUFE 1 (HIER, ceb-build/ceb-emit): je Mess-Kombination [a,b,c] EINE CEB-Pipeline\n";
        out_ +=
            "#           (ceb:build -> ceb:emit(--emit-tier-ci) -> ceb:trigger). Der PLANER steuert die CEB-Jobs.\n";
        out_ += "#   STUFE 2 (CEB-emittiert, --emit-tier-ci, S4-§62-B-Batch): je Host-Lane EIN Build+Pruef-Batch\n";
        out_ += "#           \"tier:build-batch:<host>\" (iteriert intern Perms x 4096er-Scheiben, Testate "
                "[d,e,f][g,h,i]) + EIN Mess-Batch \"measure:[a,b,c]:batch:<host>\" (GN-11/320er-gegatet).\n";
        out_ += "stages:\n";
        out_ += "  - ceb-build\n";
        out_ += "  - ceb-emit\n";
        // Child-eigene Defaults (self-contained fuer standalone-Lint; der Parent-Trigger reicht globale Variablen
        // ohnehin durch). Profil-Default = CI_PROJECT_DIR-relativ (host-unabhaengig). Range-Default = kleines,
        // SICHERES Fenster 0:4 (kein versehentlicher 2^17-Voll-Bau; COMDARE_GN_RANGE override).
        // W10-Nacharbeit 4 (KLASSE): KEIN $CI_PROJECT_DIR-Wert mehr in variables: (die gitlab-seitige
        // Vorexpansion + Vererbung an die Child ueberschreibt versions-/wegabhaengig und expandiert leer ->
        // /Code/external/... -> "profile fehlt"). $CI_PROJECT_DIR-Werte werden AUSSCHLIESSLICH per Runtime-Shell-
        // Export im Prolog gesetzt (emit_child_submodule_prolog). Hier bleiben NUR reine Literale.
        out_ += "variables:\n";
        out_ +=
            "  COMDARE_GN_RANGE: \"0:4\"   # SICHERES kleines Fenster (Pilot->Serie); Voll-Bau ist INC-G6-gegatet\n";
        // W10-Nacharbeit 2: KEIN Runner-Auto-Fetch (failt am extraheader) -> die Bau-Jobs klonen die Submodule
        // MANUELL im Prolog (emit_child_submodule_prolog). Deshalb global GIT_SUBMODULE_STRATEGY:none deklarieren.
        out_ += "  GIT_SUBMODULE_STRATEGY: \"none\"   # Child: manueller Deploy-Token-Klon im Job-Prolog, kein "
                "Auto-Fetch\n";
        // W10-Nacharbeit: das Child erbt die Parent-Globals nicht -> ccache/Parallel-Variablen + cache:-Block selbst
        // emittieren (sonst ccache-Permission-Fail am Runner). Single-Source-Spiegel des Parent (s. Fn-Doku).
        emit_child_ccache_config(out_);
    }
    // STUFE 1 haengt an der Mess-Achsen-Ebene: je Mess-Kombination die drei CEB-Jobs.
    void begin_measurement_combo(PlanMeasurementCombo const& c) override {
        out_ += "\n# =================================================================================\n";
        out_ += "# measurement_combo " + std::to_string(c.index) + " legend=" + c.legend + " (CEB-Typ)\n";
        out_ += "# =================================================================================\n";
        out_ += emit_ceb_build_job(c, header_.profile_basename);
        // A5 (§56-T2-FANOUT D4): der Selektor-Naht ist NUR bei N>1 CEB-Konfigs aktiv (measurement_combo_count > 1);
        // count==1 (heutige Live-Strecke) => KEIN --measurement-combo => byte-identisch zu vor A5.
        out_ += emit_ceb_emit_job(c, header_.measurement_combo_count > 1, header_.profile_basename);
        out_ += emit_ceb_trigger_job(c);
    }
    void begin_perm(PlanPerm const& p) override {
        perms_.push_back(p);
        steps_per_perm_.emplace_back();
    }
    void on_step(PlanStep const& s) override { steps_per_perm_.back().push_back(s); }
    void end_perm(PlanPerm const&) override {}
    void end_plan(PlanHeader const&) override {}

    [[nodiscard]] std::string const&                        text() const noexcept { return out_; }
    [[nodiscard]] PlanHeader const&                         header() const noexcept { return header_; }
    [[nodiscard]] std::vector<PlanPerm> const&              perms() const noexcept { return perms_; }
    [[nodiscard]] std::vector<std::vector<PlanStep>> const& steps_per_perm() const noexcept { return steps_per_perm_; }

    [[nodiscard]] static std::string simd_runner_tag(std::string const& simd_id) {
        return ::comdare::cache_engine::planner::simd_runner_tag(simd_id);
    }
    // A3: die flag-granulare Tag-LISTE als Test-/Konsumenten-Surface (Symmetrie zur Free-Function-Single-Source).
    [[nodiscard]] static std::vector<std::string> simd_runner_tags(std::string const& simd_id) {
        return ::comdare::cache_engine::planner::simd_runner_tags(simd_id);
    }

private:
    // STUFE 1a: der CEB-Bau-Job dieser Mess-Kombination. Baut die CEB (heute: comdare-messung-driver). Tag amd64
    // (broadest: der CEB-Bau ist compiler-only; die SIMD-Wahl faellt erst in der CEB-Rolle je System-Perm).
    [[nodiscard]] static std::string emit_ceb_build_job(PlanMeasurementCombo const& c,
                                                        std::string const&          profile_basename) {
        std::string const slug = legend::cmake_slug(c.legend);
        std::string       s;
        s += "# JOB ceb-build combo " + std::to_string(c.index) + " (STUFE 1: Planer steuert CEB-Bau, CEB-Typ " +
             c.legend + ")\n";
        s += "\"" + legend::ceb_build_job(c.legend) + "\":\n";
        s += "  stage: ceb-build\n";
        s += "  tags: [\"amd64\"]\n";
        s += "  resource_group: \"ceb-" + slug + "\"\n";
        s += "  interruptible: false   # CEB-Bau darf nie auto-cancelt werden\n";
        s += "  script:\n";
        // S2-NACHT: der Prolog verdrahtet COMDARE_GOLDEN_N_PROFILE auf das AKTIVE Profil (profile_basename).
        emit_child_submodule_prolog(s, profile_basename); // W10-Nacharbeit 2: manueller ce-Submodul-Klon
        s += "    - 'echo \"== Toolchain ==\"; cmake --version; (g++ --version || c++ --version || echo \"KEIN "
             "C++-Compiler\")'\n";
        s += "    - cd Code\n";
        s += "    - cmake -B build -G Ninja -DCOMDARE_V32_ENABLE=ON -DCMAKE_BUILD_TYPE=Release\n";
        s += "    - cmake --build build --target comdare-messung-driver\n";
        s += "    - |\n";
        s += "      set -euo pipefail\n";
        s += "      DRIVER=$(find build -type f -name \"comdare-messung-driver\" | head -1)\n";
        s +=
            "      test -n \"$DRIVER\" -a -x \"$DRIVER\" || { echo \"comdare-messung-driver (CEB) fehlt\"; exit 1; }\n";
        s += "      echo \"== STUFE 1: CEB " + c.legend + " gebaut (Messsystem-Typ) ==\"\n";
        return s;
    }

    // STUFE 1b: die GEBAUTE CEB emittiert SELBST Child-2 (--emit-tier-ci, CEB-Rolle) -> Artefakt. §40.b-Hoheit.
    [[nodiscard]] static std::string emit_ceb_emit_job(PlanMeasurementCombo const& c, bool emit_combo_selector,
                                                       std::string const& profile_basename) {
        std::string const slug = legend::cmake_slug(c.legend);
        std::string const art  = "tier-child-" + slug + ".yml";
        std::string       s;
        s += "# JOB ceb-emit combo " + std::to_string(c.index) + " (STUFE 1->2: CEB emittiert Child-2, CEB-Hoheit)\n";
        s += "\"" + legend::ceb_emit_job(c.legend) + "\":\n";
        s += "  stage: ceb-emit\n";
        s += "  tags: [\"amd64\"]\n";
        s += "  needs: [\"" + legend::ceb_build_job(c.legend) + "\"]\n";
        s += "  script:\n";
        // S2-NACHT: der Prolog verdrahtet COMDARE_GOLDEN_N_PROFILE auf das AKTIVE Profil; der --emit-tier-ci-Aufruf
        // unten liest GENAU diese Variable ($COMDARE_GOLDEN_N_PROFILE) -> der Grandchild-Katalog folgt dem Profil-Scope.
        emit_child_submodule_prolog(s, profile_basename); // W10-Nacharbeit 2: ceb:emit baut Treiber neu -> ce-Quellen
        s += "    - cd Code\n";
        s += "    - cmake -B build -G Ninja -DCOMDARE_V32_ENABLE=ON -DCMAKE_BUILD_TYPE=Release\n";
        s += "    - cmake --build build --target comdare-messung-driver\n";
        s += "    - |\n";
        s += "      set -euo pipefail\n";
        s += "      DRIVER=$(find build -type f -name \"comdare-messung-driver\" | head -1)\n";
        s += "      # §40.b-Praezisierung: die CEB (nicht der Planer) emittiert ihre STUFE-2-Sicht (System-Perms\n";
        s += "      # des FREIGEGEBENEN Raums + je-Host Build+Pruef-Batch + gegatete Mess-Batches) via "
             "--emit-tier-ci.\n";
        // A5 (§56-T2-FANOUT D4): bei N>1 CEB-Konfigs traegt der ceb:emit-Aufruf den distinct Combo-Selektor
        // (--measurement-combo=<cmake_slug>) -- so emittiert jeder ceb:emit-Job GENAU seine EINE CEB-Konfig
        // (Kollisionsschutz der combo-unabhaengigen tier:build-Job-Namen, §56/T6). count==1 => leer => byte-identisch.
        std::string const combo_arg = emit_combo_selector ? " --measurement-combo=" + slug : std::string{};
        s += "      \"$DRIVER\" --emit-tier-ci \"$COMDARE_GOLDEN_N_PROFILE\"" + combo_arg + " > \"$CI_PROJECT_DIR/" +
             art + "\"\n";
        s +=
            "      echo \"== CEB-emittierte STUFE-2 (erste 20 Zeilen) ==\"; head -20 \"$CI_PROJECT_DIR/" + art + "\"\n";
        s += "  artifacts:\n";
        s += "    paths:\n";
        s += "      - " + art + "\n";
        return s;
    }

    // STUFE 1c: Grandchild-Trigger der CEB-emittierten Child-2-YAML (trigger: include: artifact:).
    [[nodiscard]] static std::string emit_ceb_trigger_job(PlanMeasurementCombo const& c) {
        std::string const slug = legend::cmake_slug(c.legend);
        std::string const art  = "tier-child-" + slug + ".yml";
        std::string       s;
        s += "# JOB ceb-trigger combo " + std::to_string(c.index) + " (STUFE 1: Grandchild-Trigger der Child-2)\n";
        s += "\"" + legend::ceb_trigger_job(c.legend) + "\":\n";
        s += "  stage: ceb-emit\n";
        s += "  needs:\n";
        s += "    - job: \"" + legend::ceb_emit_job(c.legend) + "\"\n";
        s += "      artifacts: true\n";
        // S1/A1 P-TOTAL (Ledger 46): explizite Forward-Allowlist an der ZWEITEN Trigger-Grenze. Der Grandchild-
        // Tier-Bau liest ${COMDARE_GN_TOTAL:-16}; self-contained Grandchild-Pipelines erben Pipeline-Variablen
        // NICHT -> COMDARE_GN_TOTAL als YAML-Variable dieses Bridge-Jobs deklarieren (RHS = der aus STUFE 1
        // geerbte Wert) und via forward:yaml_variables an die STUFE-2-Grandchild reichen. KEIN pipeline_variables
        // (Isolation + Byte-Determinismus). So liest der Voll-Build 131072 statt des Sicherheits-Fallbacks 16.
        s += "  variables:\n";
        s += "    COMDARE_GN_TOTAL: \"$COMDARE_GN_TOTAL\"\n";
        // S5-P2-Rest (Smoke-Propagation): COMDARE_MEASURE_PROFILE analog forwarden -- self-contained Grandchild-
        // Pipelines erben Pipeline-Variablen NICHT, sonst faellt die STUFE-3-Mess-Rule '$COMDARE_MEASURE_PROFILE ==
        // "smoke"' im Grandchild aus und die Mess-Jobs bleiben when:manual statt Auto-Run (Befund CI-Smoke 11840).
        s += "    COMDARE_MEASURE_PROFILE: \"$COMDARE_MEASURE_PROFILE\"\n";
        // smoke=>debug-Entkopplung (2026-07-22): COMDARE_PLAN_METHODIK_PROFILE (Methodik-Profil-Selektor) EBENSO an die
        // Grandchild forwarden -- self-contained Grandchild-Pipelines erben Pipeline-Variablen NICHT. Der Grandchild
        // liest daraus die Mess-Loop-Methodik (run_profile_facade); die STUFE-2-emit_batch_measure_job-Debug-Emission haengt
        // am build_semantik aus DERSELBEN Env (--emit-tier-ci der CEB). Unset (Voll-Lauf) => leer => kein Override =>
        // Mess-Verhalten byte-identisch. Die Env-Belegung (=m3_smoke_coverage im smoke) setzt die super-YAML (Schicht 4).
        s += "    COMDARE_PLAN_METHODIK_PROFILE: \"$COMDARE_PLAN_METHODIK_PROFILE\"\n";
        s += "  trigger:\n";
        s += "    include:\n";
        s += "      - artifact: " + art + "\n";
        s += "        job: \"" + legend::ceb_emit_job(c.legend) + "\"\n";
        s += "    strategy: depend\n";
        s += "    forward:\n";
        s += "      yaml_variables: true       # Allowlist (COMDARE_GN_TOTAL + COMDARE_MEASURE_PROFILE + "
             "COMDARE_PLAN_METHODIK_PROFILE) an Grandchild\n";
        s += "      pipeline_variables: false  # kein blindes Erben des Eltern-Variablenraums (Isolation)\n";
        return s;
    }

    PlanHeader                         header_;
    std::vector<PlanPerm>              perms_;
    std::vector<std::vector<PlanStep>> steps_per_perm_;
    std::string                        out_;
};

// ── TierCiYamlBuilder — STUFE-2-Emitter (CEB-Rolle, PAKET W10-A / §42/§42.b + S4-§62-B-Batch, --emit-tier-ci). ──
//    Emittiert NUR die STUFE-2-Sicht der CE-gesteuerten Kette: den FREIGEGEBENEN System-Achsen-Raum [d,e,f]
//    (opt x simd des Director-Walks) einer CEB (=Mess-Kombination [a,b,c]). Dies ist die CEB-HOHEIT (§40.b-
//    Praezisierung: der Planer steuert die CEB-Jobs, die CEB steuert die Tier-Jobs). Heute traegt EINE Binary
//    beide Rollen (Planer-Rolle CiYamlBuilder vs CEB-Rolle DIESER Builder) -- ehrlich getrennt ueber getrennte
//    CLI-Modi + getrennte Emissions-Sichten.
//
//    S4-§62-B-BATCH-EMISSION (2026-07-23): die Job-Anzahl ist O(MASCHINEN), nie O(Perms x Chunks). Je CEB
//    (Mess-Kombination [a,b,c]) und je HOST-LANE mit nicht-leerem Perm-Bucket (amd=prod1, intel=prod2; s.
//    measure_host_lane):
//      "tier:build-batch:<host>"          -- EIN Build+Pruef-Batch: iteriert INTERN alle der Lane zugeteilten
//         System-Perms; je Perm laeuft er das [0,COMDARE_GN_TOTAL)-Fenster in kGnBatchSlice-Scheiben (4096er-
//         Bestandslog-Korn) provision-only durch und faehrt DANACH das S3-Konformitaets-Gate (COMDARE_PRUEF_ONLY).
//         Der Job-Name traegt NUR die Host-Lane; die Bau-/Pruef-Testate im Trace tragen je Schritt
//         zelle=[d,e,f][g,h,i] (System-Haupt- x fuehrende Organ-Haupt-Achsen-Referenz, §42.b Haupt-only-Gate) --
//         KEINE Mess-Kombination [a,b,c] (die steht EINMAL im Batch-KOPF, CEB-Ebene) und KEINE Unter-Achse. §62-B-
//         NACHTRAG: System-Layer [d,e,f] und Organ-Layer [g,h,i] bleiben STRIKT GETRENNT -- auch in der Laufzeit
//         (aeussere System-Perm-Schleife, inneres binary_id-/Scheiben-Fenster), nie eine 6-Achsen-Struktur.
//      "measure:[a,b,c]:batch:<host>"     -- EIN Mess-Batch je Host: misst alle Lane-Perms real (EIN CSV je Zelle
//         nach measure_out/<slug>/perm<idx>). NUR MESS-Testate tragen alle drei Klammern zelle=[a,b,c][d,e,f][g,h,i].
//         rules: smoke=>Auto-Run, sonst when:manual (320er-§41-Gate).
//    P4 (§62-B): Build-Batch und Mess-Batch teilen je Maschine die resource_group "ceb-measure-<host>" (GitLab
//    serialisiert sie nativ) -- so laeuft je Maschine hoechstens EIN Batch, das K-Budget (24/16) ist voll
//    ausschoepfbar. timeout: 7d (GN-11-Mehrtaegigkeit); Trace-Hygiene: Treiber-Detail je Scheibe/Perm in
//    Artefakt-Log-Dateien (gn_out/.../logs/ bzw. measure_out/.../logs/), im Job-Trace stehen NUR KOPF + Testate.
//
//    GOLDEN/HOST-NEUTRAL: reine Text-Emission; nur CI-Variablen + die Legenden als LITERALE => byte-deterministisch.
//    Isomorph zum Director-Walk (perms()/steps_per_perm() = struktureller Zeuge; das Bucketing aendert den Walk NICHT).
class TierCiYamlBuilder final : public IPlanBuilder {
public:
    void begin_plan(PlanHeader const& h) override {
        header_ = h;
        out_ += "# comdare CEB-emitted tier child-pipeline (TierCiYamlBuilder v2, STUFE 2 = System-Achsen-Stufe, "
                "S4-§62-B-Batch) -- GENERIERT, deterministisch, host-unabhaengig.\n";
        out_ += "# source_kind=" + h.source_kind + " profile_id=" + h.profile_id +
                " measurement_combo_count=" + std::to_string(h.measurement_combo_count) +
                " perm_count=" + std::to_string(h.perm_count) + " batch_slice=" + std::to_string(kGnBatchSlice) +
                " host_lanes=amd,intel\n";
        out_ += "#\n";
        out_ += "# Ledger §42.b/§56 + §62-B-Batch (CEB-Rolle --emit-tier-ci): NUR die STUFE-2-Sicht, O(Maschinen).\n";
        out_ += "#   tier:build-batch:<host>       -- EIN Build+Pruef-Batch je Host (iteriert intern Perms x 4096er-"
                "Scheiben; Testate je Schritt zelle=[d,e,f][g,h,i]; Haupt-only, §42.b).\n";
        out_ += "#   measure:[a,b,c]:batch:<host>  -- EIN Mess-Batch je Host (smoke=>Auto / sonst when:manual, "
                "320er-§41-Gate; MESS-Testate zelle=[a,b,c][d,e,f][g,h,i]).\n";
        out_ += "stages:\n";
        out_ += "  - tier-build\n";
        out_ += "  - measure\n";
        // W10-Nacharbeit 4 (KLASSE): KEIN $CI_PROJECT_DIR-Wert mehr in variables: (die gitlab-seitige
        // Vorexpansion + Vererbung an die Child ueberschreibt versions-/wegabhaengig und expandiert leer ->
        // /Code/external/... -> "profile fehlt"). $CI_PROJECT_DIR-Werte werden AUSSCHLIESSLICH per Runtime-Shell-
        // Export im Prolog gesetzt (emit_child_submodule_prolog). Hier bleiben NUR reine Literale.
        out_ += "variables:\n";
        out_ +=
            "  COMDARE_GN_RANGE: \"0:4\"   # SICHERES kleines Fenster (Pilot->Serie); Voll-Bau ist INC-G6-gegatet\n";
        // W10-Nacharbeit 2: KEIN Runner-Auto-Fetch (failt am extraheader) -> die Bau-Jobs klonen die Submodule
        // MANUELL im Prolog (emit_child_submodule_prolog). Deshalb global GIT_SUBMODULE_STRATEGY:none deklarieren.
        out_ += "  GIT_SUBMODULE_STRATEGY: \"none\"   # Child: manueller Deploy-Token-Klon im Job-Prolog, kein "
                "Auto-Fetch\n";
        // W10-Nacharbeit: das Child erbt die Parent-Globals nicht -> ccache/Parallel-Variablen + cache:-Block selbst
        // emittieren (sonst ccache-Permission-Fail am Runner). Single-Source-Spiegel des Parent (s. Fn-Doku).
        emit_child_ccache_config(out_);
    }
    // Die CEB-Rolle emittiert je Mess-Kombination (die CEB, die --emit-tier-ci aufruft) ihre STUFE-2-Sicht. Der
    // combo-Kontext [a,b,c] wird fuer die Batch-KOPF-/Perm-Legenden gehalten. S4: die Host-Lane-Buckets werden je
    // CEB FRISCH gesammelt (die Batch-Jobs gehoeren EINER CEB -- die CEB IST die Mess-Kombination); die Emission
    // geschieht batchweise am Kombinations-Ende (end_measurement_combo).
    void begin_measurement_combo(PlanMeasurementCombo const& c) override {
        combo_legend_ = c.legend;
        lane_amd_.clear();
        lane_intel_.clear();
        out_ += "\n# --- CEB-Raum " + c.legend + " (Mess-Kombination " + std::to_string(c.index) + ") ---\n";
    }
    void begin_perm(PlanPerm const& p) override {
        perms_.push_back(p);
        steps_per_perm_.emplace_back();
    }
    void on_step(PlanStep const& s) override { steps_per_perm_.back().push_back(s); }
    // S4-§62-B: KEINE Job-Emission je Perm mehr (das war die O(Perms x Chunks)-Einzel-Job-Kette). Die Perm wird nur
    // (i) als deterministischer Kommentar dokumentiert und (ii) in ihre HOST-LANE gebucket (measure_host_lane);
    // die Batch-Jobs emittiert end_measurement_combo. perms_/steps_per_perm_ bleiben Walk-Zeuge (Isomorphie).
    void end_perm(PlanPerm const& p) override {
        std::string const perm_legend = legend::system_perm(p.opt_id, p.simd_id);
        std::string const host        = measure_host_lane(p.simd_id, combo_legend_);
        out_ += "# perm " + std::to_string(p.index) + ": " + perm_legend + " lane=" + host + "\n";
        (host == "amd" ? lane_amd_ : lane_intel_).push_back(p);
    }
    // S4-§62-B-Batch-Emission: je Host-Lane in FESTER Reihenfolge {amd, intel} und NUR bei nicht-leerem Bucket
    // (Leere-Lane-Regel: kein Job-Paar, kein toter needs-Verweis) EIN Build+Pruef-Batch + EIN Mess-Batch. Feste
    // Reihenfolge + benannte Bucket-Vektoren => Byte-Determinismus der Stufe-2-YAML.
    void end_measurement_combo(PlanMeasurementCombo const& /*c*/) override {
        for (auto const& host : {std::string_view{"amd"}, std::string_view{"intel"}}) {
            std::vector<PlanPerm> const& bucket = (host == "amd") ? lane_amd_ : lane_intel_;
            if (bucket.empty()) continue; // Leere-Lane-Regel
            std::string const h{host};
            out_ += emit_batch_build_job(h, bucket);
            out_ += emit_batch_measure_job(h, bucket);
        }
    }
    void end_plan(PlanHeader const&) override {}

    [[nodiscard]] std::string const&                        text() const noexcept { return out_; }
    [[nodiscard]] PlanHeader const&                         header() const noexcept { return header_; }
    [[nodiscard]] std::vector<PlanPerm> const&              perms() const noexcept { return perms_; }
    [[nodiscard]] std::vector<std::vector<PlanStep>> const& steps_per_perm() const noexcept { return steps_per_perm_; }

private:
    // S4-§62-B Build+Pruef-BATCH (2026-07-23): EIN Job je Host-Lane, der INTERN alle der Lane zugeteilten
    // System-Perms durchlaeuft. Je Perm laeuft er das [0,COMDARE_GN_TOTAL)-Fenster in kGnBatchSlice-Scheiben
    // (4096er-Bestandslog-Korn) provision-only durch (Bau ohne Messung, golden-neutral) und faehrt DANACH das
    // S3-Konformitaets-Gate (COMDARE_PRUEF_ONLY=true) ueber dasselbe Fenster/dll_dir. O(Maschinen) statt
    // O(Perms x Chunks). Trace-Hygiene: Treiber-Detail je Scheibe/Perm in Artefakt-Log-Dateien; im Job-Trace stehen
    // NUR der Batch-KOPF ([a,b,c]+lane, CEB-Ebene, einmal) + die Schritt-Testate ([d,e,f][g,h,i], zwei Klammern,
    // Bau=Haupt-only-Gate §42.b -- KEINE Unter-Achse, KEIN [a,b,c] je Schritt: die Layer bleiben getrennt, §62-B-
    // NACHTRAG). set-e-sichere if-Guards je Aufruf; FAIL-Sammel-Exit am Batch-Ende (Batch baut/prueft durch).
    [[nodiscard]] std::string emit_batch_build_job(std::string const& host, std::vector<PlanPerm> const& perms) const {
        std::string const slug = legend::cmake_slug(combo_legend_);
        std::string const job  = legend::tier_batch_build_job(host);
        std::string const par  = std::to_string(lane_build_parallelism(host)); // §62-B-K-Budget-Literal
        // S6-P1b Env-Bruecke (d): ab N>1 traegt das Treiber-Kommando die gewaehlte Mess-Combo. [all] => LEER =>
        // byte-stabil. (i) §61-STUFEN: nur der Nicht-Default-Build (Debug) traegt COMDARE_BUILD_TYPE (Reuse-Schluessel).
        std::string const combo_env =
            combo_legend_ == "[all]" ? std::string{} : "COMDARE_MEASUREMENT_COMBO=\"" + combo_legend_ + "\" ";
        std::string const build_type_env =
            header_.build_semantic.cmake_build_type == "Debug" ? "COMDARE_BUILD_TYPE=\"Debug\" " : std::string{};
        std::string s;
        // Batch-KOPF (Kommentar, einmal je Job): CEB-Identitaet [a,b,c] + Host-Lane. §62-B-NACHTRAG: [a,b,c] ist
        // die CEB-Ebene und steht NUR im KOPF, NICHT je Schritt.
        s += "# JOB tier-build-batch host=" + host + " ceb=" + combo_legend_ +
             " (STUFE 2 Batch, §62-B: O(Maschinen); Build+Pruef aller " + host +
             "-Lane-Perms; Testate je Schritt [d,e,f][g,h,i], ceb=[a,b,c] nur KOPF)\n";
        s += "\"" + job + "\":\n";
        s += "  stage: tier-build\n";
        // Host-Lane-Tag (avx512-Perms landen via measure_host_lane zwingend im amd-Bucket; das Treiber-ISA-Gate
        // profile_run_entry ist die zweite Wache). NICHT die flag-granulare simd-Tag-Liste (die galt der Einzel-Job-Ebene).
        s += "  tags: " + yaml_tag_list({host}) + "\n";
        // P4 (§62-B): resource_group ceb-measure-<host> LITERAL-REUSE -- GitLab serialisiert Build-Batch vs Mess-Batch
        // je Maschine (auch cross-pipeline gegen Alt-Jobs); NICHT umbenennen. So laeuft je Maschine EIN Batch.
        s += "  resource_group: \"ceb-measure-" + host + "\"\n";
        s += "  interruptible: false   # ein laufender Provision-/Pruef-Batch darf nie auto-cancelt werden\n";
        s += "  timeout: 7d            # GN-11-Mehrtaegigkeit (Runner-maximum_timeout >= 7d ist Infra-Vorbedingung)\n";
        s += "  script:\n";
        // S2-NACHT: der Prolog verdrahtet COMDARE_GOLDEN_N_PROFILE auf das AKTIVE Profil (header_.profile_basename).
        emit_child_submodule_prolog(s, header_.profile_basename); // W10-Nacharbeit 2: manueller ce-Submodul-Klon
        s += "    - cd Code\n";
        // S5-P1: CMAKE_BUILD_TYPE aus der aufgeloesten Run-Methodik (measure => "Release"); Default byte-identisch zu HEAD.
        s += "    - cmake -B build -G Ninja -DCOMDARE_V32_ENABLE=ON -DCMAKE_BUILD_TYPE=" +
             header_.build_semantic.cmake_build_type + "\n";
        s += "    - cmake --build build --target comdare-messung-driver\n";
        s += "    - |\n";
        s += "      set -euo pipefail\n";
        s += "      DRIVER=$(find build -type f -name \"comdare-messung-driver\" | head -1)\n";
        s += "      test -n \"$DRIVER\" -a -x \"$DRIVER\" || { echo \"comdare-messung-driver fehlt\"; exit 1; }\n";
        // §62-B-K-Budget (Bau-Pool-WORKER-Override, KEIN $(nproc)): lane_build_parallelism(host) = 24 (amd) / 16 (intel).
        s += "      export COMDARE_BUILD_PARALLEL=\"" + par + "\"   # §62-B-K-Budget " + host +
             " (lane_build_parallelism; harte Compile-Worker-Zahl statt der nproc-Heuristik)\n";
        s += "      TOTAL=\"${COMDARE_GN_TOTAL:-16}\"   # Default 16 = sicherer Serie-Test; Voll-Bau: "
             "COMDARE_GN_TOTAL=131072\n";
        s += "      SLICE=" + std::to_string(kGnBatchSlice) +
             "   # §62-B-Bestandslog-Korn (harte Konstante, KEIN Env-Override)\n";
        s += "      FAIL=0\n";
        s += "      LOGDIR=\"$CI_PROJECT_DIR/Code/gn_out/" + slug + "/" + host + "/logs\"\n";
        s += "      mkdir -p \"$LOGDIR\"\n";
        // Batch-KOPF (Echo, einmal): CEB-Identitaet [a,b,c] + Lane -> Trace-Legende der CEB-Ebene.
        s += "      echo \"== [BATCH-BAU] ceb=" + combo_legend_ + " lane=" + host +
             " total=$TOTAL slice=" + std::to_string(kGnBatchSlice) + " ts=$(date -u +%FT%TZ) ==\"\n";
        for (auto const& p : perms) {
            std::string const perm_legend = legend::system_perm(p.opt_id, p.simd_id);
            std::string const organ       = legend::organ_reference();
            std::string const opt         = p.opt_id.empty() ? std::string{"O3"} : p.opt_id;
            std::string const simd        = p.simd_id.empty() ? std::string{"no_extension"} : p.simd_id;
            std::string const idx         = std::to_string(p.index);
            // Schritt-Zelle: [d,e,f][g,h,i] -- ZWEI Klammern (System- x fuehrende Organ-Haupt-Achsen-Referenz).
            // KEIN [a,b,c] (Layer NIE verschmolzen), KEINE Unter-Achse (Bau=Haupt-only-Gate, §42.b).
            std::string const cell    = perm_legend + organ;
            std::string const dll_dir = "$CI_PROJECT_DIR/Code/gn_out/" + slug + "/" + host + "/perm" + idx;
            s += "      echo \"== [BAU] zelle=" + cell + " lane=" + host + " ts=$(date -u +%FT%TZ) ==\"\n";
            // Fenster-Schleife: kGnBatchSlice-Scheiben ueber [0,TOTAL) (Bestandslog-Korn 4096). Jede Scheibe ist
            // provision-only mit Inline-COMDARE_GOLDEN_N_RANGE; Detail nach $LOGDIR (Trace-Hygiene), if-Guard set-e-sicher.
            s += "      START=0\n";
            s += "      while [ \"$START\" -lt \"$TOTAL\" ]; do\n";
            s += "        REMAIN=$(( TOTAL - START )); COUNT=$SLICE; [ \"$COUNT\" -gt \"$REMAIN\" ] && COUNT=$REMAIN   "
                 "# letzte Scheibe klemmt\n";
            s += "        if ! COMDARE_THESIS_PROFILE=\"$COMDARE_GOLDEN_N_PROFILE\" \\\n";
            s += "             " + combo_env + build_type_env + "COMDARE_GN_OPT=\"" + opt + "\" COMDARE_GN_SIMD=\"" +
                 simd + "\" COMDARE_GOLDEN_N_PROVISION_ONLY=true COMDARE_RUN_SOTA=0 \\\n";
            s += "             COMDARE_GOLDEN_N_RANGE=\"${START}:${COUNT}\" \\\n";
            s += "             \"$DRIVER\" experiment_config \"" + dll_dir + "\" \\\n";
            s += "             > \"$LOGDIR/perm" + idx + "_bau_${START}.log\" 2>&1; then\n";
            s += "          echo \"[FEHLER-TESTAT] ts=$(date -u +%FT%TZ) lane=" + host + " zelle=" + cell +
                 " phase=bau fenster=${START}:${COUNT}\"; FAIL=1\n";
            s += "        fi\n";
            s += "        echo \"[TESTAT] ts=$(date -u +%FT%TZ) lane=" + host + " zelle=" + cell +
                 " phase=bau fenster=${START}:${COUNT}\"\n";
            s += "        START=$(( START + COUNT ))\n";
            s += "      done\n";
            // S3-PRUEF-Schritt (UNBEDINGT je Perm nach der Fenster-Schleife): COMDARE_PRUEF_ONLY=true faehrt NUR das
            // Konformitaets-Gate je gebauter .so ueber 0:TOTAL im GLEICHEN dll_dir (kein Bau, keine Messung). [d,e,f][g,h,i].
            s += "      echo \"== [PRUEF] zelle=" + cell + " lane=" + host + " ts=$(date -u +%FT%TZ) ==\"\n";
            s += "      if ! COMDARE_THESIS_PROFILE=\"$COMDARE_GOLDEN_N_PROFILE\" \\\n";
            s += "           " + combo_env + build_type_env + "COMDARE_GN_OPT=\"" + opt + "\" COMDARE_GN_SIMD=\"" +
                 simd + "\" COMDARE_PRUEF_ONLY=true COMDARE_RUN_SOTA=0 \\\n";
            s += "           COMDARE_GOLDEN_N_RANGE=\"0:${TOTAL}\" \\\n";
            s += "           \"$DRIVER\" experiment_config \"" + dll_dir + "\" \\\n";
            s += "           > \"$LOGDIR/perm" + idx + "_pruef.log\" 2>&1; then\n";
            s += "        echo \"[FEHLER-TESTAT] ts=$(date -u +%FT%TZ) lane=" + host + " zelle=" + cell +
                 " phase=pruef fenster=0:${TOTAL}\"; FAIL=1\n";
            s += "      else\n";
            s += "        echo \"[PRUEF-TESTAT] ts=$(date -u +%FT%TZ) lane=" + host + " zelle=" + cell +
                 " phase=pruef fenster=0:${TOTAL}\"\n";
            s += "      fi\n";
        }
        s += "      exit $FAIL   # Fehler je Zelle sichtbar ([FEHLER-TESTAT] + Log-Artefakt); der Batch baut/prueft "
             "durch\n";
        s += "  artifacts:\n";
        s += "    when: always\n";
        s += "    paths:\n";
        s += "      - Code/gn_out/" + slug + "/" + host + "/logs/\n";
        s += "    expire_in: 4 weeks\n";
        return s;
    }

    // S4-§62-B Mess-BATCH (2026-07-23): EIN Job je Host-Lane, der INTERN alle Lane-Perms real misst (Spiegel des
    // frueheren emit_measure_job als Perm-Schleife). Er baut+misst OHNE COMDARE_GOLDEN_N_PROVISION_ONLY => run_profile
    // MISST und schreibt EIN CSV je Zelle nach measure_out/<slug>/perm<idx> (BYTE-GLEICHE Pfade, KEINE win-
    // Fragmentierung, KEINE Lane-Konsolidierung). §61-MODI: der DLL-Bau laeuft PARALLEL (COMDARE_BUILD_PARALLEL=
    // §62-B-K-Budget) -- NUR das Messen ist 1-Thread (run_profile-Loop). (h) resource_group PRO MASCHINE
    // (ceb-measure-<host>, DIESELBE wie der Build-Batch = P4). rules (§41/320er): Auto-Run NUR im smoke-Profil, sonst
    // when:manual. needs = der Build-Batch derselben Lane (1 Kante). MESS-Testate tragen alle drei Klammern
    // zelle=[a,b,c][d,e,f][g,h,i] (nur die Mess-Ebene). Debug-Profil: (j3)-Dual-Compile je Perm UNVERAENDERT.
    [[nodiscard]] std::string emit_batch_measure_job(std::string const&           host,
                                                     std::vector<PlanPerm> const& perms) const {
        std::string const slug = legend::cmake_slug(combo_legend_);
        std::string const job  = legend::measure_batch_job(combo_legend_, host);
        std::string const par  = std::to_string(lane_build_parallelism(host)); // §62-B-K-Budget (ersetzt $(nproc))
        std::string const combo_env =
            combo_legend_ == "[all]" ? std::string{} : "COMDARE_MEASUREMENT_COMBO=\"" + combo_legend_ + "\" ";
        std::string const build_type_env =
            header_.build_semantic.cmake_build_type == "Debug" ? "COMDARE_BUILD_TYPE=\"Debug\" " : std::string{};
        std::string s;
        s += "# JOB measure-batch host=" + host + " ceb=" + combo_legend_ +
             " (STUFE 3 Batch, §62-B: O(Maschinen); realer Mess-Vollzug aller " + host +
             "-Lane-Perms; smoke=Auto-Run / sonst when:manual = 320er-§41-Gate)\n";
        s += "\"" + job + "\":\n";
        s += "  stage: measure\n";
        s += "  tags: " + yaml_tag_list({host}) + "\n";
        // P4 (§62-B): DIESELBE resource_group wie der Build-Batch derselben Maschine -> je Maschine laeuft entweder
        // der Build- ODER der Mess-Batch, nie beide gleichzeitig (native GitLab-Serialisierung; LITERAL-REUSE).
        s += "  resource_group: \"ceb-measure-" + host + "\"\n";
        s += "  interruptible: false   # ein laufender Messlauf darf nie auto-cancelt werden\n";
        s += "  timeout: 7d            # GN-11-Mehrtaegigkeit\n";
        s += "  needs:\n";
        // §62-B: EINE Bau->Mess-Kante auf den Build-Batch derselben Lane (statt der 4 Chunk-Kanten je Perm).
        s += "    - \"" + legend::tier_batch_build_job(host) + "\"\n";
        // §41/320er: Auto-Messlauf NUR im smoke-Profil; sonst when:manual (BYTE-GLEICH zur Vor-S4-Emission).
        s += "  rules:\n";
        s += "    - if: '$COMDARE_MEASURE_PROFILE == \"smoke\"'\n";
        s += "      when: on_success   # smoke: Auto-Messlauf (kleiner Umfang, Rauch-Test der Mess-Strecke)\n";
        s += "    - when: manual       # sonst: 320er-§41-Gate (Voll-Messlauf erst nach User-Entscheid)\n";
        // Sichtbarkeits-Doktrin: Mess-Fehler => CSV 'failed' + Log, die Pipeline bleibt gruen (nicht still verschluckt).
        s += "  allow_failure: true\n";
        s += "  script:\n";
        // S2-NACHT: der Prolog verdrahtet COMDARE_GOLDEN_N_PROFILE auf das AKTIVE Profil (header_.profile_basename).
        emit_child_submodule_prolog(s, header_.profile_basename); // ce-Submodul-Klon, Spiegel des Bau-Jobs
        s += "    - cd Code\n";
        s += "    - cmake -B build -G Ninja -DCOMDARE_V32_ENABLE=ON -DCMAKE_BUILD_TYPE=" +
             header_.build_semantic.cmake_build_type + "\n";
        s += "    - cmake --build build --target comdare-messung-driver\n";
        s += "    - |\n";
        s += "      set -euo pipefail\n";
        s += "      DRIVER=$(find build -type f -name \"comdare-messung-driver\" | head -1)\n";
        s += "      test -n \"$DRIVER\" -a -x \"$DRIVER\" || { echo \"comdare-messung-driver fehlt\"; exit 1; }\n";
        // Mess-Fenster = das VOLLE [0:COMDARE_GN_TOTAL) der Zelle (BYTE-GLEICH zur Vor-S4-Emission). Einmal je Batch.
        s +=
            "      export COMDARE_GOLDEN_N_RANGE=\"0:${COMDARE_GN_TOTAL:-16}\"   # volles Zell-Fenster (Voll-Messlauf: "
            "COMDARE_GN_TOTAL=131072)\n";
        // §61-MODI: der DLL-Bau laeuft PARALLEL, aber mit dem §62-B-K-Budget-Literal (ersetzt $(nproc)) -- NUR das
        // MESSEN ist 1-Thread (run_profile-Loop). Der Treiber-cmake-Bau bleibt beim Parent-CMAKE_BUILD_PARALLEL_LEVEL-Deckel.
        s += "      export COMDARE_BUILD_PARALLEL=\"" + par + "\"   # §62-B-K-Budget " + host +
             " (DLL-Bau parallel; Messen 1-Thread, run_profile-Loop; statt nproc-Heuristik)\n";
        // (platform-Tag) §61/§62 Plattform-Provenienz: die CSV-Spalte "platform" MUSS die MESSENDE Maschine tragen
        // (compile_time_platform_tag trennt amd/intel-x86_64 NICHT). Einmal je Batch (die Lane ist fix je Job).
        s += "      export COMDARE_PLATFORM=\"" + host +
             "@$(hostname)\"   # (platform-Tag) ISA-Lane@Maschine -> CSV-Provenienz (§61/§62 per-Maschine)\n";
        s += "      FAIL=0\n";
        s += "      LOGDIR=\"$CI_PROJECT_DIR/Code/measure_out/" + slug + "/logs\"\n";
        s += "      mkdir -p \"$LOGDIR\"\n";
        s += "      echo \"== [BATCH-MESS] ceb=" + combo_legend_ + " lane=" + host + " ts=$(date -u +%FT%TZ) ==\"\n";
        for (auto const& p : perms) {
            std::string const perm_legend = legend::system_perm(p.opt_id, p.simd_id);
            std::string const organ       = legend::organ_reference();
            std::string const opt         = p.opt_id.empty() ? std::string{"O3"} : p.opt_id;
            std::string const simd        = p.simd_id.empty() ? std::string{"no_extension"} : p.simd_id;
            std::string const idx         = std::to_string(p.index);
            // NUR MESS: alle drei Klammern [a,b,c][d,e,f][g,h,i] (die Mess-Ebene traegt die volle Legende).
            std::string const cell3       = combo_legend_ + perm_legend + organ;
            std::string const measure_out = "$CI_PROJECT_DIR/Code/measure_out/" + slug + "/perm" + idx;
            if (header_.build_semantic.cmake_build_type == "Debug") {
                // (j3) §61-STUFEN Dual-Compile (Manager-Entscheid: UNVERAENDERT, nur in die Perm-Schleife verlegt):
                // (1) Release provision-only fuellt die shareable RELEASE-Reuse-Masse (eigenes _release_provision-Dir,
                // sonst ueberschriebe (2) die Release-.so bei gleicher binary_id), (2) baut+misst Debug (-O0/+bt).
                // Blocker #50: COMDARE_ARTEFAKT_TRIES=1. Debug=smoke (kleiner Umfang) -> kein if-Guard/Testat (UNVERAENDERT).
                s += "      export COMDARE_ARTEFAKT_TRIES=1   # (j3) Blocker #50: smoke/debug grindet nicht in "
                     "Retry-Schleifen (HART)\n";
                s += "      echo \"== (j3) Aufruf 1/2: Release provision-only (O2/O3-Reuse-Masse, Default-Stempel, "
                     "KEIN Messen): " +
                     combo_legend_ + perm_legend + organ + " ==\"\n";
                s += "      COMDARE_THESIS_PROFILE=\"$COMDARE_GOLDEN_N_PROFILE\" \\\n";
                s += "        " + combo_env + "COMDARE_GN_OPT=\"" + opt + "\" COMDARE_GN_SIMD=\"" + simd +
                     "\" COMDARE_GOLDEN_N_PROVISION_ONLY=true COMDARE_RUN_SOTA=0 \\\n";
                s += "        \"$DRIVER\" experiment_config \"" + measure_out + "_release_provision\"\n";
                s += "      echo \"== (j3) Aufruf 2/2: Debug-Bau+Messung (-O0/+bt via 2b+(i)), misst (KEIN "
                     "provision-only): " +
                     combo_legend_ + perm_legend + organ + " ==\"\n";
                s += "      COMDARE_THESIS_PROFILE=\"$COMDARE_GOLDEN_N_PROFILE\" \\\n";
                // smoke=>debug-Entkopplung: den METHODIK-PROFIL-Selektor an den Grandchild-Mess-Run durchreichen.
                s += "        COMDARE_PLAN_METHODIK_PROFILE=\"${COMDARE_PLAN_METHODIK_PROFILE:-}\" \\\n";
                s += "        " + combo_env + build_type_env + "COMDARE_GN_OPT=\"" + opt + "\" COMDARE_GN_SIMD=\"" +
                     simd + "\" COMDARE_RUN_SOTA=0 \\\n";
                s += "        \"$DRIVER\" experiment_config \"" + measure_out + "\"\n";
            } else {
                // Release: [MESS]-Schritt-KOPF (drei Klammern) + if-guarded Treiber-Aufruf (BYTE-GLEICHE Praefixe zur
                // Vor-S4-Emission) mit Trace-Hygiene-Log-Umleitung + [MESS-TESTAT]/[FEHLER-TESTAT] (set-e-sicher).
                s += "      echo \"== [MESS] zelle=" + cell3 + " lane=" + host + " ts=$(date -u +%FT%TZ) ==\"\n";
                s += "      if ! COMDARE_THESIS_PROFILE=\"$COMDARE_GOLDEN_N_PROFILE\" \\\n";
                s += "           " + combo_env + build_type_env + "COMDARE_GN_OPT=\"" + opt + "\" COMDARE_GN_SIMD=\"" +
                     simd + "\" COMDARE_RUN_SOTA=0 \\\n";
                s += "           \"$DRIVER\" experiment_config \"" + measure_out + "\" \\\n";
                s += "           > \"$LOGDIR/perm" + idx + "_mess.log\" 2>&1; then\n";
                s += "        echo \"[FEHLER-TESTAT] ts=$(date -u +%FT%TZ) lane=" + host + " zelle=" + cell3 +
                     " phase=mess fenster=0:${COMDARE_GN_TOTAL:-16}\"; FAIL=1\n";
                s += "      fi\n";
                s += "      echo \"[MESS-TESTAT] ts=$(date -u +%FT%TZ) lane=" + host + " zelle=" + cell3 +
                     " phase=mess fenster=0:${COMDARE_GN_TOTAL:-16}\"\n";
            }
        }
        s += "      exit $FAIL   # Mess-Fehler je Zelle sichtbar ([FEHLER-TESTAT] + Log); der Batch misst durch\n";
        s += "  artifacts:\n";
        s += "    when: always\n";
        s += "    paths:\n";
        s += "      - Code/measure_out/" + slug + "/logs/\n";
        s += "    expire_in: 4 weeks\n";
        return s;
    }

    PlanHeader                         header_;
    std::string                        combo_legend_ = "[all]"; // gesetzt in begin_measurement_combo
    std::vector<PlanPerm>              perms_;
    std::vector<std::vector<PlanStep>> steps_per_perm_;
    // S4-§62-B: die Host-Lane-Buckets der AKTUELLEN CEB (je begin_measurement_combo frisch). Benannte Vektoren
    // (statt map) => Byte-Determinismus; end_measurement_combo emittiert je nicht-leerem Bucket EIN Job-Paar.
    std::vector<PlanPerm> lane_amd_;
    std::vector<PlanPerm> lane_intel_;
    std::string           out_;
};

// ── Registry-Trio-Annotation aus einem gelesenen RegistryTrio (Resolver-Vorstufe). ──────────────────────────
[[nodiscard]] inline PlanRegistrySource make_plan_registry_source(tlz::RegistryContents const& rc) {
    std::size_t bausteine = 0;
    for (auto const& [axis, names] : rc.axis_names) bausteine += names.size();
    return PlanRegistrySource{rc.engine, rc.axis_names.size(), bausteine};
}
[[nodiscard]] inline PlanRegistryTrioAnnotation make_plan_registry_annotation(tlz::RegistryTrio const& trio) {
    PlanRegistryTrioAnnotation a;
    a.organ       = make_plan_registry_source(trio.organ);
    a.system      = make_plan_registry_source(trio.system);
    a.measurement = make_plan_registry_source(trio.measurement);
    a.loaded      = true;
    return a;
}

// ── TierCmakeGraphBuilder — STUFE-2-Emitter (CEB-Rolle, PAKET W10-A / §42/§42.b + S4-§62-B-Batch, --emit-tier-cmake).
//    Der Bare-Metal-Gegenpart (Dual-Weg §61-Spiegel) zum TierCiYamlBuilder: emittiert das tier_plan.cmake einer CEB
//    (Mess-Kombination [a,b,c]). S4-§62-B-BATCH: je HOST-LANE mit nicht-leerem Perm-Bucket EIN Build+Pruef-Aggregat-
//    Target comdare_tier_batch_<host> (verkettete per-Perm Provision- + S3-Pruef-Kommandos, provision-only SCHARF +
//    Konformitaets-Gate; gleiche per-Perm-Ausgabedirs) + EIN Mess-Target comdare_tier_measure_<host> (SCHARF, misst
//    real). Strukturell isomorph zum CI-Batch (emit_batch_build_job/emit_batch_measure_job). Bare-Metal-Lauf der
//    DREISTUFIGEN Kette: --dump-cmake (Stufe 1) -> CEB -> --emit-tier-cmake (Stufe 2) -> `cmake --build <dir>
//    --target comdare_tier_batch_amd` (baut+prueft die amd-Lane) bzw. `--target comdare_tier_measure_amd` (misst).
//
//    Host-unabhaengig: Treiber/Profil/Range/Out = CMake-Variablen mit Defaults; nur die [d,e,f][g,h,i]-Legende
//    (System x Organ, §56; die CEB-Mess-Kombination [a,b,c] ist nur Kontext, KOPF-Ebene) + die Host-Lanen sind
//    Plan-Konstanten. §62-B-NACHTRAG: Bau-/Pruef-Echos tragen zelle=[d,e,f][g,h,i], nur die Mess-Echos alle drei
//    Klammern. (DEPRECATED §56/§57: die per-(Perm x chunk<k>) comdare_tier_build_perm<i>_chunk<k>-Targets entfielen
//    in S4 -- Doku bleibt.)
class TierCmakeGraphBuilder final : public IPlanBuilder {
public:
    void begin_plan(PlanHeader const& h) override {
        header_ = h;
        out_ +=
            "# comdare tier-plan (generated .cmake blueprint, TierCmakeGraphBuilder v2 = STUFE 2, S4-§62-B-Batch)\n";
        out_ += "# source_kind=" + h.source_kind + " profile_id=" + h.profile_id +
                " perm_count=" + std::to_string(h.perm_count) + " batch_slice=" + std::to_string(kGnBatchSlice) +
                " host_lanes=amd,intel\n";
        out_ += "#\n";
        out_ += "# Ledger §42.b + §62-B-Batch (CEB-Rolle --emit-tier-cmake): je Host-Lane EIN Build+Pruef-Aggregat\n";
        out_ += "# comdare_tier_batch_<host> (per-Perm Provision + S3-Pruef, SCHARF) + EIN Mess-Target\n";
        out_ += "# comdare_tier_measure_<host> (SCHARF, misst). Bare-Metal:\n";
        out_ += "#   cmake --build <dir> --target comdare_tier_batch_amd   # baut+prueft die amd-Lane-DLLs\n";
        out_ += "# Konfigurierbare Eingaben (per -D ueberschreibbar):\n";
        out_ += "#   COMDARE_PLAN_DRIVER  = Pfad/Name des comdare-messung-driver (Default: PATH-Suche)\n";
        out_ += "#   COMDARE_PLAN_PROFILE = Thesis-/Experiment-Profil-XML (Default: leer => Treiber-Default-Profil)\n";
        out_ += "#   COMDARE_PLAN_RANGE   = golden-N Fenster start:count je Perm (Default: 0:4 = SICHER klein)\n";
        out_ += "#   COMDARE_PLAN_OUT     = Ausgabe-Wurzel fuer die provision-DLLs (Default: <bindir>/tier/out)\n";
        // (g)/§61-MODI: COMDARE_PLAN_MEASURE_PARALLEL = DLL-Bau-Pool im Mess-Target (Default = ProcessorCount, alle
        // Kerne -- §61 Regressions-Fix: der BAU laeuft parallel, NUR das Messen ist 1-Thread). Der Modus (Debug/
        // Release) kommt per (j2) STATISCH aus dem Profil (build_semantic, exactly-one) -- KEIN Env-Schalter mehr.
        out_ += "#   COMDARE_PLAN_MEASURE_PARALLEL = DLL-Bau-Pool des Mess-Targets (Default: ProcessorCount)\n";
        out_ += "if(NOT DEFINED COMDARE_PLAN_DRIVER)\n";
        out_ += "    set(COMDARE_PLAN_DRIVER \"comdare-messung-driver\")\n";
        out_ += "endif()\n";
        out_ += "if(NOT DEFINED COMDARE_PLAN_PROFILE)\n";
        out_ += "    set(COMDARE_PLAN_PROFILE \"\")\n";
        out_ += "endif()\n";
        out_ += "if(NOT DEFINED COMDARE_PLAN_RANGE)\n";
        out_ += "    set(COMDARE_PLAN_RANGE \"0:4\")\n";
        out_ += "endif()\n";
        out_ += "if(NOT DEFINED COMDARE_PLAN_OUT)\n";
        out_ += "    set(COMDARE_PLAN_OUT \"${CMAKE_CURRENT_BINARY_DIR}/tier/out\")\n";
        out_ += "endif()\n";
        // §61-MODI Regressions-Fix: der DLL-Bau des Mess-Targets laeuft PARALLEL (Default = alle Kerne); das Messen
        // bleibt 1-Thread (run_profile-Loop). ProcessorCount-Default = bare-metal-Aequivalent zu $(nproc) der CI.
        out_ += "if(NOT DEFINED COMDARE_PLAN_MEASURE_PARALLEL)\n";
        out_ += "    include(ProcessorCount)\n";
        out_ += "    ProcessorCount(_comdare_measure_nproc)\n";
        out_ += "    if(_comdare_measure_nproc EQUAL 0)\n";
        out_ += "        set(_comdare_measure_nproc 1)\n";
        out_ += "    endif()\n";
        out_ += "    set(COMDARE_PLAN_MEASURE_PARALLEL \"${_comdare_measure_nproc}\")\n";
        out_ += "endif()\n";
    }
    // S4: die Host-Lane-Buckets je CEB frisch sammeln (die Batch-Targets gehoeren EINER CEB); Emission am
    // Kombinations-Ende (end_measurement_combo), strukturell isomorph zum CI-Batch (TierCiYamlBuilder).
    void begin_measurement_combo(PlanMeasurementCombo const& c) override {
        combo_legend_ = c.legend;
        lane_amd_.clear();
        lane_intel_.clear();
        out_ += "\n# --- CEB-Raum " + c.legend + " (Mess-Kombination " + std::to_string(c.index) + ") ---\n";
    }
    void begin_perm(PlanPerm const& p) override {
        perms_.push_back(p);
        steps_per_perm_.emplace_back();
    }
    void on_step(PlanStep const& s) override { steps_per_perm_.back().push_back(s); }
    // S4-§62-B: KEINE per-Perm-Targets mehr (das war die O(Perms x Chunks)-Chunk-Target-Kette). Die Perm wird nur
    // dokumentiert + in ihre HOST-LANE gebucket; die je-Host-Aggregat-Targets emittiert end_measurement_combo.
    void end_perm(PlanPerm const& p) override {
        std::string const perm_legend = legend::system_perm(p.opt_id, p.simd_id);
        std::string const host        = measure_host_lane(p.simd_id, combo_legend_);
        out_ += "# perm " + std::to_string(p.index) + ": [d,e,f][g,h,i]=" + perm_legend + legend::organ_reference() +
                " lane=" + host + "\n";
        (host == "amd" ? lane_amd_ : lane_intel_).push_back(p);
    }
    // S4-§62-B-Batch: je Host-Lane in FESTER Reihenfolge {amd, intel}, NUR bei nicht-leerem Bucket (Leere-Lane-Regel).
    void end_measurement_combo(PlanMeasurementCombo const& /*c*/) override {
        for (auto const& host : {std::string_view{"amd"}, std::string_view{"intel"}}) {
            std::vector<PlanPerm> const& bucket = (host == "amd") ? lane_amd_ : lane_intel_;
            if (bucket.empty()) continue; // Leere-Lane-Regel
            std::string const h{host};
            emit_batch_targets(h, bucket);
            measure_targets_.push_back(tier_measure_target(h)); // fuer das end_plan-Aggregat
        }
    }
    void end_plan(PlanHeader const&) override {
        out_ +=
            "\n# Aggregat: alle je-Host-Mess-Targets (transitiv die Build+Pruef-Batch-Targets via DEPENDS-Kante).\n";
        out_ += "if(NOT TARGET " + all_target() + ")\n";
        out_ += "    add_custom_target(" + all_target() + " DEPENDS";
        for (auto const& t : measure_targets_) out_ += "\n        " + t;
        out_ += ")\n";
        out_ += "endif()\n";
    }

    [[nodiscard]] std::string const&                        text() const noexcept { return out_; }
    [[nodiscard]] PlanHeader const&                         header() const noexcept { return header_; }
    [[nodiscard]] std::vector<PlanPerm> const&              perms() const noexcept { return perms_; }
    [[nodiscard]] std::vector<std::vector<PlanStep>> const& steps_per_perm() const noexcept { return steps_per_perm_; }

private:
    // S4-§62-B bare-metal Build+Pruef-BATCH + Mess-Target je Host-Lane (Dual-Weg §61-Spiegel von
    // emit_batch_build_job/emit_batch_measure_job). Der Build+Pruef-Batch traegt verkettete per-Perm Provision- +
    // S3-Pruef-COMMANDs (COMDARE_PLAN_RANGE-Fenster, gleiche per-Perm-Ausgabedirs); das Mess-Target per-Perm
    // Mess-COMMANDs (Debug: (j3)-Dual UNVERAENDERT), DEPENDS auf den Build+Pruef-Stamp (Bau->Mess-Kante).
    void emit_batch_targets(std::string const& host, std::vector<PlanPerm> const& perms) {
        std::string const slug    = legend::cmake_slug(combo_legend_);
        std::string const stemdir = "${CMAKE_CURRENT_BINARY_DIR}/tier";
        // S6-P1b Env-Bruecke (d): ab N>1 COMDARE_MEASUREMENT_COMBO im -E env-Block. [all] => LEER => byte-stabil.
        std::string const combo_line = combo_legend_ == "[all]"
                                           ? std::string{}
                                           : "            \"COMDARE_MEASUREMENT_COMBO=" + combo_legend_ + "\"\n";
        // (i) §61-STUFEN: nur Debug traegt COMDARE_BUILD_TYPE (Reuse-Schluessel). (j3)/R4: nur Debug COMDARE_ARTEFAKT_TRIES=1.
        std::string const cmake_bt_env       = header_.build_semantic.cmake_build_type == "Debug"
                                                   ? std::string{"            \"COMDARE_BUILD_TYPE=Debug\"\n"}
                                                   : std::string{};
        std::string const artefakt_tries_env = header_.build_semantic.cmake_build_type == "Debug"
                                                   ? std::string{"            \"COMDARE_ARTEFAKT_TRIES=1\"\n"}
                                                   : std::string{};

        // ── Build+Pruef-Aggregat-Target comdare_tier_batch_<host>: verkettete per-Perm Provision- + Pruef-COMMANDs.
        std::string const bstamp = stemdir + "/" + slug + "_batch_" + host + ".build.stamp";
        std::string const btgt   = tier_batch_target(host);
        out_ += "\n# --- Host-Lane " + host + " (Build+Pruef-Batch, §62-B: alle " + host +
                "-Lane-Perms; Testate [d,e,f][g,h,i], ceb=[a,b,c] nur KOPF) ---\n";
        out_ += "if(NOT TARGET " + btgt + ")\n";
        out_ += "    add_custom_command(\n";
        out_ += "        OUTPUT \"" + bstamp + "\"\n";
        out_ += "        COMMAND \"${CMAKE_COMMAND}\" -E make_directory \"" + stemdir + "\"\n";
        out_ += "        COMMAND \"${CMAKE_COMMAND}\" -E echo \"[BATCH-BAU] ceb=" + combo_legend_ + " lane=" + host +
                "\"\n";
        for (auto const& p : perms) {
            std::string const idx         = std::to_string(p.index);
            std::string const opt         = p.opt_id.empty() ? std::string{"O3"} : p.opt_id;
            std::string const simd        = p.simd_id.empty() ? std::string{"no_extension"} : p.simd_id;
            std::string const perm_legend = legend::system_perm(p.opt_id, p.simd_id);
            std::string const organ       = legend::organ_reference();
            std::string const cell        = perm_legend + organ; // [d,e,f][g,h,i] (Haupt-only, kein [a,b,c])
            std::string const dll_dir     = "${COMDARE_PLAN_OUT}/" + slug + "/" + host + "/perm" + idx;
            // Provision-COMMAND (provision-only, SCHARF) ueber ${COMDARE_PLAN_RANGE}.
            out_ += "        COMMAND \"${CMAKE_COMMAND}\" -E echo \"[BAU] zelle=" + cell + " lane=" + host + "\"\n";
            out_ += "        COMMAND \"${CMAKE_COMMAND}\" -E env\n";
            out_ += "            \"COMDARE_THESIS_PROFILE=${COMDARE_PLAN_PROFILE}\"\n";
            out_ += "            \"COMDARE_GOLDEN_N_RANGE=${COMDARE_PLAN_RANGE}\"\n";
            out_ += "            \"COMDARE_GN_OPT=" + opt + "\"\n";
            out_ += "            \"COMDARE_GN_SIMD=" + simd + "\"\n";
            out_ += combo_line;
            out_ += cmake_bt_env;
            out_ += "            COMDARE_GOLDEN_N_PROVISION_ONLY=true\n";
            out_ += "            COMDARE_RUN_SOTA=0\n";
            out_ += "            \"${COMDARE_PLAN_DRIVER}\" experiment_config \"" + dll_dir + "\"\n";
            // S3-Pruef-COMMAND (Konformitaets-Gate) ueber dasselbe Fenster/dll_dir (kein Bau, keine Messung).
            out_ += "        COMMAND \"${CMAKE_COMMAND}\" -E echo \"[PRUEF] zelle=" + cell + " lane=" + host + "\"\n";
            out_ += "        COMMAND \"${CMAKE_COMMAND}\" -E env\n";
            out_ += "            \"COMDARE_THESIS_PROFILE=${COMDARE_PLAN_PROFILE}\"\n";
            out_ += "            \"COMDARE_GOLDEN_N_RANGE=${COMDARE_PLAN_RANGE}\"\n";
            out_ += "            \"COMDARE_GN_OPT=" + opt + "\"\n";
            out_ += "            \"COMDARE_GN_SIMD=" + simd + "\"\n";
            out_ += combo_line;
            out_ += cmake_bt_env;
            out_ += "            \"COMDARE_PRUEF_ONLY=true\"\n";
            out_ += "            COMDARE_RUN_SOTA=0\n";
            out_ += "            \"${COMDARE_PLAN_DRIVER}\" experiment_config \"" + dll_dir + "\"\n";
        }
        out_ += "        COMMAND \"${CMAKE_COMMAND}\" -E touch \"" + bstamp + "\"\n";
        out_ += "        COMMENT \"tier build+pruef batch (provision-only + Konformitaets-Gate): ceb=" + combo_legend_ +
                " lane=" + host + "\"\n";
        out_ += "        VERBATIM)\n";
        out_ += "    add_custom_target(" + btgt + " DEPENDS \"" + bstamp + "\")\n";
        out_ += "endif()\n";

        // ── Mess-Target comdare_tier_measure_<host> (S5-P2 SCHARF): per-Perm Mess-COMMANDs, DEPENDS auf den
        //    Build+Pruef-Batch-Stamp (Bau->Mess-Kante). §61-MODI: DLL-Bau PARALLEL (COMDARE_PLAN_MEASURE_PARALLEL),
        //    Messen 1-Thread. (j3) Debug: Release-Provision-Vorlauf je Perm (eigenes _release_provision-Dir), UNVERAENDERT.
        std::string const mstamp = stemdir + "/" + slug + "_batch_" + host + ".measure.stamp";
        std::string const mtgt   = tier_measure_target(host);
        out_ += "if(NOT TARGET " + mtgt + ")\n";
        out_ += "    add_custom_command(\n";
        out_ += "        OUTPUT \"" + mstamp + "\"\n";
        out_ += "        COMMAND \"${CMAKE_COMMAND}\" -E echo \"[BATCH-MESS] ceb=" + combo_legend_ + " lane=" + host +
                "\"\n";
        for (auto const& p : perms) {
            std::string const idx         = std::to_string(p.index);
            std::string const opt         = p.opt_id.empty() ? std::string{"O3"} : p.opt_id;
            std::string const simd        = p.simd_id.empty() ? std::string{"no_extension"} : p.simd_id;
            std::string const perm_legend = legend::system_perm(p.opt_id, p.simd_id);
            std::string const organ       = legend::organ_reference();
            std::string const cell3       = combo_legend_ + perm_legend + organ; // [a,b,c][d,e,f][g,h,i] (nur MESS)
            if (header_.build_semantic.cmake_build_type == "Debug") {
                // (j3) 1/2: Release-Provision-Vorlauf (eigenes _release_provision-Dir; sonst ueberschriebe der Debug-
                // Bau die Release-.so bei gleicher binary_id). COMDARE_ARTEFAKT_TRIES=1 (Blocker #50). UNVERAENDERT.
                out_ += "        COMMAND \"${CMAKE_COMMAND}\" -E echo\n";
                out_ += "            \"(j3) 1/2: Release provision-only (O2/O3-Reuse-Masse, Default-Stempel, KEIN "
                        "Messen): " +
                        combo_legend_ + perm_legend + organ + "\"\n";
                out_ += "        COMMAND \"${CMAKE_COMMAND}\" -E env\n";
                out_ += "            \"COMDARE_THESIS_PROFILE=${COMDARE_PLAN_PROFILE}\"\n";
                out_ += "            \"COMDARE_GOLDEN_N_RANGE=${COMDARE_PLAN_RANGE}\"\n";
                out_ += "            \"COMDARE_GN_OPT=" + opt + "\"\n";
                out_ += "            \"COMDARE_GN_SIMD=" + simd + "\"\n";
                out_ += combo_line;
                out_ += "            \"COMDARE_BUILD_PARALLEL=${COMDARE_PLAN_MEASURE_PARALLEL}\"\n";
                out_ += "            \"COMDARE_ARTEFAKT_TRIES=1\"\n";
                out_ += "            COMDARE_GOLDEN_N_PROVISION_ONLY=true\n";
                out_ += "            COMDARE_RUN_SOTA=0\n";
                out_ += "            \"${COMDARE_PLAN_DRIVER}\" experiment_config \"${COMDARE_PLAN_OUT}/measure/" +
                        slug + "/perm" + idx + "_release_provision\"\n";
            }
            // (2)/Release: realer Mess-COMMAND -- OHNE COMDARE_GOLDEN_N_PROVISION_ONLY (=> misst), EIN CSV je Zelle
            // nach ${COMDARE_PLAN_OUT}/measure/<slug>/perm<idx>. Echo traegt alle drei Klammern (nur die Mess-Ebene).
            out_ += "        COMMAND \"${CMAKE_COMMAND}\" -E echo\n";
            out_ += "            \"measure (S5-P2 scharf, misst): [a,b,c][d,e,f][g,h,i]=" + cell3 + "\"\n";
            out_ += "        COMMAND \"${CMAKE_COMMAND}\" -E env\n";
            out_ += "            \"COMDARE_THESIS_PROFILE=${COMDARE_PLAN_PROFILE}\"\n";
            out_ += "            \"COMDARE_GOLDEN_N_RANGE=${COMDARE_PLAN_RANGE}\"\n";
            out_ += "            \"COMDARE_GN_OPT=" + opt + "\"\n";
            out_ += "            \"COMDARE_GN_SIMD=" + simd + "\"\n";
            out_ += combo_line;
            out_ += "            \"COMDARE_BUILD_PARALLEL=${COMDARE_PLAN_MEASURE_PARALLEL}\"\n";
            out_ += cmake_bt_env;
            out_ += artefakt_tries_env;
            out_ += "            COMDARE_RUN_SOTA=0\n";
            out_ += "            \"${COMDARE_PLAN_DRIVER}\" experiment_config \"${COMDARE_PLAN_OUT}/measure/" + slug +
                    "/perm" + idx + "\"\n";
        }
        out_ += "        COMMAND \"${CMAKE_COMMAND}\" -E touch \"" + mstamp + "\"\n";
        out_ += "        DEPENDS \"" + bstamp + "\" # tier build+pruef -> measure-Kante\n";
        out_ +=
            "        COMMENT \"measure batch (S5-P2 scharf, misst): ceb=" + combo_legend_ + " lane=" + host + "\"\n";
        out_ += "        VERBATIM)\n";
        out_ += "    add_custom_target(" + mtgt + " DEPENDS \"" + mstamp + "\")\n";
        out_ += "endif()\n";
    }

    [[nodiscard]] static std::string tier_batch_target(std::string const& host) { return "comdare_tier_batch_" + host; }
    [[nodiscard]] static std::string tier_measure_target(std::string const& host) {
        return "comdare_tier_measure_" + host;
    }
    [[nodiscard]] static std::string all_target() { return "comdare_tier_plan_all"; }

    PlanHeader                         header_;
    std::string                        combo_legend_ = "[all]";
    std::vector<PlanPerm>              perms_;
    std::vector<std::vector<PlanStep>> steps_per_perm_;
    // S4-§62-B: Host-Lane-Buckets der aktuellen CEB + die je-Host-Mess-Targets fuer das end_plan-Aggregat.
    std::vector<PlanPerm>    lane_amd_;
    std::vector<PlanPerm>    lane_intel_;
    std::vector<std::string> measure_targets_;
    std::string              out_;
};

// ── ExperimentPlanDirector — DIRECTOR (GoF Builder). EIN deterministischer Walk, zwei Kanaele. ──────────────
class ExperimentPlanDirector {
public:
    ExperimentPlanDirector() = default;
    explicit ExperimentPlanDirector(PlanRegistryTrioAnnotation trio) : trio_(std::move(trio)) {}
    // S3 P-RESOLVER (2026-07-20): additiver Konstruktor mit dem VOLLEN RegistryTrio. Er traegt die Achsen-Namen-
    // Mengen, die der Resolver (resolve_axis_refs_against_trio) fuer die Organ-Position-Aufloesung braucht -- die
    // reine Zaehl-Annotation (PlanRegistryTrioAnnotation) reicht dafuer NICHT. Die Zaehl-Annotation wird aus DEMSELBEN
    // Trio abgeleitet (make_plan_registry_annotation) -- EINE Quelle, kein Doppel-Argument. Init-Reihenfolge folgt der
    // Member-DEKLARATION (trio_ vor full_trio_): make_plan_registry_annotation liest `trio` VOR dem move. Ohne diesen
    // Konstruktor bleibt der Resolver INERT (resolved=false) und das Vor-S3-Verhalten byte-identisch.
    explicit ExperimentPlanDirector(tlz::RegistryTrio trio)
        : trio_(make_plan_registry_annotation(trio)), full_trio_(std::move(trio)) {}

    /// Thesis-Kanal: opt x simd x profile_sweep_passes(tp, ""). KEIN Bau; die Selektions-Pass-Liste ist die
    /// deterministische #26/GO-5-Enumeration (Basis-Pass zuerst + je <axis_sweep> ein Pass in Dokument-Reihenfolge).
    void construct(cx::ThesisProfile const& tp, IPlanBuilder& b, std::string const& combo_selector = {},
                   std::vector<std::string> const& methodik_run_methodology = {},
                   std::string const&              profile_basename         = {}) const {
        std::vector<std::string> const opt_perms  = opt_perms_of(tp.compiler.opt_levels);
        std::vector<std::string> const simd_perms = simd_perms_of(tp.extension_hardware.simd_options);
        std::vector<std::string> const passes     = tlz::profile_sweep_passes(tp, /*requested_axis=*/"");
        // Mess-Tooling-HAUPT {wallclock/macro/micro} faechert die [a,b,c]-Kombination auf. S6-P1 (SCHARF): die
        // Fan-out-Verdrahtung ist aktiv -- measurement_combos_of(cats, tp.measurement_tooling) faechert je
        // deklarierter <combo>-Tooling-Konfig EINE ceb:build:[a,b,c]-Strecke auf. LEER (kein <measurement_tooling>)
        // => Default {} => 1 Voll-Konfig [all] (byte-stabil zur heutigen 1-CEB-Strecke).
        std::vector<PlanMeasurementCombo> combos =
            measurement_combos_of(tp.measurement_categories, tp.measurement_tooling);
        // A5 (§56-T2-FANOUT D4): DORMANT bei leerem combo_selector (Live-Strecke {} => Identitaet, byte-stabil); ein
        // gesetzter Selektor (nur --emit-tier-ci-Naht) waehlt die EINE repraesentierte CEB-Konfig (Kollisionsschutz).
        combos                             = select_measurement_combo(std::move(combos), combo_selector);
        tlz::ResolverReport const resolver = resolve_organ_position_(tp); // S3: INERT ohne volles Trio
        // S5-P1: die Build-/Mess-Semantik der S5-Mess-Strecke aus dem A9.1-Feld run_methodology (measure-Methodik).
        // smoke=>debug-Entkopplung (2026-07-22): ist ein METHODIK-Profil gesetzt (methodik_run_methodology aus
        // COMDARE_PLAN_METHODIK_PROFILE, facade-validiert exactly-one), kommt die Methodik aus DIESEM Profil, waehrend
        // tp den Bau-Katalog (Achsen/Perms) liefert -- so misst ein all_axes_golden-Bau mit der debug-Methodik von
        // m3_smoke_coverage (parallel + (j3)-Dual-Compile). Leer => aus tp.run_methodology (BYTE-IDENTISCH). §61: die
        // Methodik bleibt profil-getrieben+exactly-one; die Env ist Profil-SELEKTOR, nicht Methodik-Wert.
        PlanBuildSemantic const build_semantic = build_semantic_of_run_methodology(
            methodik_run_methodology.empty() ? tp.run_methodology : methodik_run_methodology);
        walk_perms_("thesis", tp.id, profile_basename, combos, opt_perms, simd_perms, resolver, build_semantic, b,
                    [&](IPlanBuilder& bb) {
                        std::size_t j = 0;
                        for (auto const& sweep_axis : passes) {
                            PlanStep s;
                            s.index          = j++;
                            s.kind           = "thesis_sweep_pass";
                            s.label          = sweep_axis; // "" = Basis-Pass (PlanTextBuilder rendert <basis>)
                            s.merge          = "-";
                            s.binary_id      = "-";
                            s.series         = "-";
                            s.pruefling_type = "-";
                            s.lebewesen      = "-";
                            bb.on_step(s);
                        }
                    });
    }

    /// Experiment-Kanal: opt x simd x (phase -> je real baubarem (merge x lebewesen)-Pass EIN Schritt). Die
    /// Phasen-Projektion ist die Bruecke-I3-Enumeration (nullopt-Paare ehrlich ausgelassen, kein Phantom-Schritt).
    void construct(cx::ExperimentProfile const& ep, IPlanBuilder& b, std::string const& combo_selector = {},
                   std::vector<std::string> const& methodik_run_methodology = {},
                   std::string const&              profile_basename         = {}) const {
        std::vector<std::string> const opt_perms  = opt_perms_of(ep.compiler.opt_levels);
        std::vector<std::string> const simd_perms = simd_perms_of(ep.extension_hardware.simd_options);
        std::vector<tlz::ExperimentPhaseProjection> const projections = tlz::project_experiment_to_sota_passes(ep);
        // Mess-Tooling-HAUPT {wallclock/macro/micro} faechert die [a,b,c]-Kombination auf. S6-P1 (SCHARF): die
        // Fan-out-Verdrahtung ist aktiv -- measurement_combos_of(cats, ep.measurement_tooling) faechert je
        // deklarierter <combo>-Tooling-Konfig EINE ceb:build:[a,b,c]-Strecke auf. LEER (kein <measurement_tooling>)
        // => Default {} => 1 Voll-Konfig [all] (byte-stabil zur heutigen 1-CEB-Strecke).
        std::vector<PlanMeasurementCombo> combos =
            measurement_combos_of(ep.measurement_categories, ep.measurement_tooling);
        // A5 (§56-T2-FANOUT D4): DORMANT bei leerem combo_selector (Live-Strecke {} => Identitaet, byte-stabil); ein
        // gesetzter Selektor (nur --emit-tier-ci-Naht) waehlt die EINE repraesentierte CEB-Konfig (Kollisionsschutz).
        combos                             = select_measurement_combo(std::move(combos), combo_selector);
        tlz::ResolverReport const resolver = resolve_organ_position_(ep); // S3: INERT ohne volles Trio
        // S5-P1: die Build-/Mess-Semantik der S5-Mess-Strecke aus dem A9.1-Feld run_methodology (measure-Methodik).
        // smoke=>debug-Entkopplung (2026-07-22): METHODIK-Profil-Override (s. Thesis-Overload); leer => aus
        // ep.run_methodology (BYTE-IDENTISCH). Achsen/Perms bleiben aus ep.
        PlanBuildSemantic const build_semantic = build_semantic_of_run_methodology(
            methodik_run_methodology.empty() ? ep.run_methodology : methodik_run_methodology);
        walk_perms_("experiment", ep.id, profile_basename, combos, opt_perms, simd_perms, resolver, build_semantic, b,
                    [&](IPlanBuilder& bb) {
                        std::size_t j = 0;
                        for (auto const& proj : projections) {
                            for (auto const& p : proj.passes) {
                                PlanStep s;
                                s.index          = j++;
                                s.kind           = "experiment_phase_pass";
                                s.label          = proj.phase_name;
                                s.merge          = proj.merge;
                                s.binary_id      = p.view_binary_id;
                                s.series         = p.series.empty() ? std::string{"-"} : p.series;
                                s.pruefling_type = p.pruefling_type.empty() ? std::string{"-"} : p.pruefling_type;
                                s.lebewesen      = p.lebewesen.empty() ? std::string{"-"} : p.lebewesen;
                                bb.on_step(s);
                            }
                        }
                    });
    }

private:
    // S3 P-RESOLVER: der Organ-Position-Reject/Route-Report je Kanal. Ohne volles RegistryTrio (Default-/Annotation-
    // Konstruktor) INERT (resolved=false, 0 Rejects); mit vollem Trio klassifiziert er die permute_axes/axes_default_
    // lookup-Refs (tlz::organ_position_refs ueberladen je Profil-Art). READ-ONLY, binary_id-neutral (kein Filter).
    template <class Profile>
    [[nodiscard]] tlz::ResolverReport resolve_organ_position_(Profile const& p) const {
        if (!full_trio_) return tlz::ResolverReport{}; // INERT-Default (resolved=false)
        return tlz::resolve_axis_refs_against_trio(tlz::organ_position_refs(p), *full_trio_);
    }

    // S5-P1 (P-VOLLZUG): die Build-/Mess-Semantik der S5-Mess-Strecke aus dem A9.1-Feld run_methodology. Der Planer
    // waehlt fuer die Mess-Strecke die measure-Methodik (run_methodology_registry-Single-Source: Release / misst /
    // 1-Thread-deterministisch, §38.b); cmake_build_type/single_thread stammen IMMER aus dieser Zeile => tier:build
    // byte-identisch zum Vor-S5-Verhalten (Default-Release). Das Feld wird gelesen: measurement_on spiegelt, ob das
    // Profil ueberhaupt misst (leer ODER measure/debug deklariert => ja; NUR release => nein, reiner Referenz-
    // Durchsatz -- S6-Konsum). Der per-Methodik-Fanout {debug,measure,release} zu N Mess-Strecken ist S6.
    [[nodiscard]] static PlanBuildSemantic
    build_semantic_of_run_methodology(std::vector<std::string> const& run_methodology) {
        // §61-STUFEN/(j2): GENAU EIN aktiver Modus je Profil (validate erzwingt exactly-one, j1). Die Build-Semantik
        // kommt aus DIESEM Modus -- Debug={Debug,misst,parallel}, Measure={Release,misst,1-Thread}, Release={Release,
        // misst NICHT}. NICHT mehr fix measure (Vor-(j2)-Lesart). Leer => Default measure (Release, 1-Thread). Die
        // Methodik-Quelle ist das PROFIL, NICHT die Env (COMDARE_MEASURE_PROFILE bleibt NUR der rules-Auto-Run-Trigger).
        // R5: >1 ist ein Kontraktbruch (validate gated ihn VOR dem Bau) -- HART statt still front(), damit ein
        // 2-Modi-Profil nie zufaellig eine Modus-Semantik waehlt (symmetrisch zu run_methodology_for_ids).
        if (run_methodology.size() > 1)
            throw std::invalid_argument(
                "build_semantic_of_run_methodology: " + std::to_string(run_methodology.size()) +
                " Modi deklariert -- GENAU EINER erlaubt (exactly-one je Call, Ledger 61-STUFEN).");
        auto const info_for = [](std::string const& id) -> cm::RunMethodologyInfo const& {
            for (std::size_t i = 0; i < cm::kRunMethodologyCount; ++i)
                if (cm::kRunMethodologyRegistry[i].id == id) return cm::kRunMethodologyRegistry[i];
            return cm::run_methodology_info(cm::RunMethodology::Measure); // unbekannt => measure-Default
        };
        cm::RunMethodologyInfo const& m = run_methodology.empty()
                                              ? cm::run_methodology_info(cm::RunMethodology::Measure)
                                              : info_for(run_methodology.front());
        return PlanBuildSemantic{std::string(m.cmake_build_type), m.measurement_on, m.single_thread};
    }

    // opt/simd-Listen-Ableitung IDENTISCH zu run_profile/run_experiment_profile (Welle-2-Naht): leer => EINE
    // Identitaets-Perm auf dem CEB-Default (O3 / no_extension) = Vor-Wiring-Verhalten byte-identisch.
    [[nodiscard]] static std::vector<std::string> opt_perms_of(std::vector<std::string> const& xml_opt_levels) {
        return xml_opt_levels.empty() ? std::vector<std::string>{std::string{cm::DefaultOptLevelOption::opt_level_id()}}
                                      : xml_opt_levels;
    }
    [[nodiscard]] static std::vector<std::string> simd_perms_of(std::vector<std::string> const& xml_simd_options) {
        return xml_simd_options.empty() ? std::vector<std::string>{std::string{cm::DefaultSimdOption::simd_id()}}
                                        : xml_simd_options;
    }

public: // measurement_combos_of ist reine statische Fan-out-Kern-Logik -> als Contract-Surface fuer die
    // isolierten Fan-out-Tests (§47/§55) exponiert. Der uebrige Direktor-Zustand bleibt privat.
    // W10-A (§42) + §47/§54-T2/§55: die Mess-Achsen-Kombinationen. Die HAUPT-Auffaecherung [a,b,c] kommt aus der
    // Mess-Tooling-HAUPT-Achse {wallclock/macro/micro} (kMeasurementToolingRegistry): je Tooling-KONFIG EINE
    // dynamische CEB-Pipeline (ceb:build:[a,b,c]). Die 16 <measurement_categories> sind Mess-Tooling-UNTER
    // (CSV-Spalten, §54-T2) und faechern den CEB-Typ NICHT auf — sie reisen als combo.categories mit.
    //
    // `tooling_configs` = die deklarierten Tooling-HAUPT-Konfigs (je Eintrag EINE Konfig = ein Vektor von
    // Tooling-ids). LEER => EINE implizite VOLL-Konfig (das volle Mess-System, alle Tooling => Sentinel `[all]`) —
    // dieselbe "leer = volles Angebot"-Idiomatik wie legend::measurement_tooling_combo. Damit bleibt die heutige
    // Topologie (1 CEB-Strecke, Legende [all]) byte-stabil, bis die Mehr-Konfig-Deklaration verdrahtet ist.
    //
    // === OFFENE DESIGN-FRAGEN (§47/§55, GO-pflichtig — Manager, NICHT geraten): ===
    //   (D1) XML-Schema: <measurement_tooling>-Element (HAUPT, auffaechernd) FEHLT — nur <measurement_categories>
    //        (UNTER) existiert. Wie deklariert die Anwender-XML N Tooling-Konfigs? (validate_profile.hpp erweitern.)
    //   (D2) TEILWEISE — cx::ExperimentProfile traegt jetzt das PASSIVE Feld measurement_tooling (KERN-A: der Parser
    //        fuellt es aus <measurement_tooling><combo tools=..>; reine Schema-Vollstaendigkeit). Die Call-Site-
    //        Verdrahtung (construct() reicht ep.measurement_tooling hierher) + der Fan-out (D4) gehoeren dem
    //        Schwester-Paket P-MESSTOOL. HEUTE reicht construct() {} => 1 Voll-Konfig [all] (byte-stabil).
    //   (D3) Registry-Angebot -> Anwahl (Resolver): kMeasurementToolingRegistry ist das ANGEBOT; WIE das
    //        Anwender-XML (.pom) daraus die Konfigs waehlt (Resolver, §27/§28), ist offen.
    //   (D4) Fan-out-Aktivierung + Kollisionsschutz: N>1 Konfigs => N ceb:build:[a,b,c]-Strecken; ABER da §56/T6 die
    //        Mess-Konfig aus der tier:build-Legende ENTFERNT hat (tier:build:[d,e,f][g,h,i] ist combo-UNABHAENGIG),
    //        wuerden die tier:build-Job-Namen VERSCHIEDENER CEB-Konfigs im SELBEN --emit-tier-ci-Lauf KOLLIDIEREN.
    //        Loesung (offen): --emit-tier-ci selektiert die EINE CEB-Konfig, die es repraesentiert (je ceb:emit-Job
    //        genau eine Konfig — heute walkt --emit-tier-ci ALLE combos in EINE Datei, s. TierCiYamlBuilder). Zudem:
    //        N>1 verdreifacht die (GN-11/320er-)gegatete golden-Topologie -> Aktivierung ist Gate-/Manager-Entscheid.
    //
    // Der Fan-out-KERN (Tooling-Konfig -> Combo) ist hier real + isoliert testbar: measurement_combos_of(cats,
    // {{"wallclock"},{"macro"}}) => 2 Combos [wallclock]/[macro]. Die LIVE-Call-Sites reichen heute {} (D2) => 1 Combo.
    [[nodiscard]] static std::vector<PlanMeasurementCombo>
    measurement_combos_of(std::vector<std::string> const&              measurement_categories,
                          std::vector<std::vector<std::string>> const& tooling_configs = {}) {
        // Deklarierte Konfigs; leer => EINE implizite VOLL-Konfig (leerer Tooling-Vektor => [all]-Sentinel).
        std::vector<std::vector<std::string>> configs = tooling_configs;
        if (configs.empty()) configs.emplace_back(); // {} => volles Mess-System => [all]
        std::vector<PlanMeasurementCombo> combos;
        combos.reserve(configs.size());
        std::size_t idx = 0;
        for (auto const& cfg : configs) {
            PlanMeasurementCombo combo;
            combo.index      = idx++;
            combo.tooling    = cfg;                                    // Mess-Tooling-HAUPT-KONFIG (§47/§55)
            combo.categories = measurement_categories;                 // Mess-Tooling-UNTER (CSV-Spalten, §54-T2)
            combo.legend     = legend::measurement_tooling_combo(cfg); // [a,b,c]-HAUPT aus dem Tooling
            combos.push_back(std::move(combo));
        }
        return combos;
    }

    // A5 (§56-T2-FANOUT D4): der per-CEB Combo-Selektor. --emit-tier-ci repraesentiert GENAU EINE CEB-Konfig (je
    // ceb:emit-Job eine Konfig); da §56/T6 die Mess-Konfig aus der tier:build-Legende ENTFERNT hat (combo-unabhaengige
    // Job-Namen), wuerden N>1 CEB-Konfigs in EINEM --emit-tier-ci-Lauf KOLLIDIEREN. Dieser Selektor loest das: er
    // behaelt NUR die Kombination, deren cmake_slug(legend) == `selector` (der --measurement-combo-Wert). Leerer
    // Selektor = IDENTITAET (die heutige Live-Strecke {} => 1 Voll-Konfig [all], byte-stabil). Kein Treffer => leer
    // (ehrliche Null-Emission, kein Crash). KEIN Re-Indexing: die ueberlebende Kombination behaelt ihren
    // Original-`index` (Walk-Determinismus, der perm_index-Lauf bleibt konsistent zum Voll-Walk).
    [[nodiscard]] static std::vector<PlanMeasurementCombo>
    select_measurement_combo(std::vector<PlanMeasurementCombo> combos, std::string const& selector) {
        if (selector.empty()) return combos; // leerer Selektor = Identitaet (Live-Strecke, byte-stabil)
        std::erase_if(combos, [&](PlanMeasurementCombo const& c) { return legend::cmake_slug(c.legend) != selector; });
        return combos;
    }

private:
    // Der EINE dreistufige Walk (Mess-Kombination -> System-Perm (opt x simd) -> Chunk-Buendel) + Builder-Treiber.
    // Der Steps-Emitter ist der einzige art-abhaengige Teil (er ruft die BESTEHENDEN Callees). KEIN dritter
    // Enumerations-Walk: opt/simd/pass/phase stammen vollstaendig aus profile_sweep_passes/
    // project_experiment_to_sota_passes + den XML-System-Achsen-Listen; die Mess-Kombinationen aus
    // measurement_combos_of. perm_count = |opt x simd| JE Mess-Kombination (heute 1 Kombination => byte-identische
    // Perm-Menge zum Vor-W10-Verhalten; der perm_index laeuft ueber den gesamten Walk).
    template <class StepEmitter>
    void walk_perms_(std::string_view source_kind, std::string const& profile_id, std::string const& profile_basename,
                     std::vector<PlanMeasurementCombo> const& combos, std::vector<std::string> const& opt_perms,
                     std::vector<std::string> const& simd_perms, tlz::ResolverReport const& resolver,
                     PlanBuildSemantic const& build_semantic, IPlanBuilder& b, StepEmitter&& emit_steps) const {
        PlanHeader header;
        header.source_kind             = std::string{source_kind};
        header.profile_id              = profile_id;
        header.profile_basename        = profile_basename; // S2-NACHT: Basename des aktiven Profils
        header.perm_count              = opt_perms.size() * simd_perms.size();
        header.measurement_combo_count = combos.size();
        header.registries              = trio_;
        header.resolver                = resolver; // S3 P-RESOLVER: Organ-Position-Report im Plan-Kopf (Annotation)
        header.build_semantic          = build_semantic; // S5-P1: measure-Methodik-Build-/Mess-Semantik (Tier-Emitter)
        b.begin_plan(header);

        std::size_t perm_index = 0;
        for (auto const& combo : combos) {
            b.begin_measurement_combo(combo);
            for (auto const& opt_id : opt_perms) {
                for (auto const& simd_id : simd_perms) {
                    PlanPerm perm;
                    perm.index              = perm_index++;
                    perm.opt_id             = opt_id;
                    perm.simd_id            = simd_id;
                    perm.opt_flag           = tlz::system_axis_opt_flag_of(opt_id); // leer => D1-Degradierung beim Lauf
                    perm.march_flag         = tlz::system_axis_march_of(simd_id);   // leer bei no_extension
                    perm.host_supports_simd = tlz::system_axis_host_supports_simd(simd_id); // ANNOTATION, kein Filter
                    perm.build_version_suffix =
                        "+opt=" + opt_id +
                        (simd_id == std::string{cm::SimdNoExtOption::simd_id()} ? std::string{} : "+ext=" + simd_id);
                    b.begin_perm(perm);
                    emit_steps(b);
                    b.end_perm(perm);
                }
            }
            b.end_measurement_combo(combo);
        }
        b.end_plan(header);
    }

    PlanRegistryTrioAnnotation trio_; // leer/loaded=false, wenn ohne Registry-Trio konstruiert
    // S3 P-RESOLVER: das VOLLE RegistryTrio (Achsen-Namen-Mengen) fuer die Organ-Position-Aufloesung. nullopt =>
    // der Resolver ist INERT (resolved=false) -- Default-/Annotation-Konstruktor-Pfad, Vor-S3-Verhalten byte-identisch.
    std::optional<tlz::RegistryTrio> full_trio_;
};

} // namespace comdare::cache_engine::planner
