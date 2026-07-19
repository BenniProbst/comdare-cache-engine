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

#include "xml_config_parser/xml_config_parser.hpp" // cx::ExperimentProfile / cx::ThesisProfile (explizit)

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

/// EINE Mess-Achsen-Kombination (W10-A / §42): der AEUSSERSTE Walk-Schritt (Mess-Kombination -> System-Perms ->
/// Chunk-Buendel). Sie bestimmt den CEB-TYP: die Anwender-XML (<measurement_categories>) waehlt die Mess-Achsen
/// [a,b,c]; je Kombination EINE dynamische CEB-Pipeline. `legend` = die kanonische [a,b,c]-Kurzform (plan_legend;
/// `[all]` = alle 16 System-Kategorien / kein Subset deklariert). `categories` = die deklarierten Roh-Namen
/// (Dokument-Reihenfolge). Heute typisch EINE Kombination (ersetzt den GN_MSYS="default"-Platzhalter des Piloten).
struct PlanMeasurementCombo {
    std::size_t              index = 0;  // 0-basierter Kombinations-Index im deterministischen Walk
    std::vector<std::string> categories; // die deklarierten <measurement_categories> (leer = alle 16 Default)
    std::string              legend;     // kanonische [a,b,c]-Kurzform (legend::measurement_combo)
};

/// Kopf des Plans: Provenienz (Quelle/Profil) + Perm-Zahl + Registry-Trio-Annotation.
struct PlanHeader {
    std::string source_kind; // "thesis" | "experiment"
    std::string profile_id;
    std::size_t perm_count              = 0; // |opt x simd| JE Mess-Kombination
    std::size_t measurement_combo_count = 0; // W10-A: Zahl der Mess-Achsen-Kombinationen (heute typisch 1)
    PlanRegistryTrioAnnotation registries;
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
        out_ +=
            "#   (dann Stufe-2: cmake -P/Konfiguration dieses tier_plan + --build comdare_tier_build_perm0_chunk0)\n";
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

// Wie viele Chunk-Bau-Jobs die CEB-Rolle je System-Perm emittiert (Organ-Raum-Buendelung, §42.b). Default 4 =
// die Pilot-Matrix-Chunk-Zahl (GN_CHUNK 0..3, super/.gitlab-ci.yml) -- 2^17 Einzel-Jobs waeren keine Legende.
inline constexpr std::size_t kTierChunkCount = 4;

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
inline void emit_child_submodule_prolog(std::string& out) {
    out += "    - |\n";
    out += "      set -euo pipefail\n";
    out += "      # RUNTIME-Shell-Export aller $CI_PROJECT_DIR-abhaengigen Env (Nacharbeit 3+4, KLASSE): immun gegen\n";
    out += "      # die GitLab-variables-Vorexpansion/-Vererbung (die vorexpandierte Parent-Def ueberschriebe sonst\n";
    out += "      # versions-/wegabhaengig die Child-Def -> leer expandiert -> /.ccache bzw. /Code/...-fehlt).\n";
    out += "      export CCACHE_DIR=\"${CI_PROJECT_DIR}/.ccache\"\n";
    out += "      export CCACHE_MAXSIZE=\"3G\"\n";
    out += "      export COMDARE_GOLDEN_N_PROFILE=\"${CI_PROJECT_DIR}/Code/external/comdare-cache-engine/libs/"
           "cache_engine/algorithm_profiles/thesis_profiles/all_axes_golden.profile.xml\"\n";
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
        out_ += "#   STUFE 2 (CEB-emittiert, --emit-tier-ci): je System-Perm [d,e,f] die Tier-Chunk-Jobs\n";
        out_ += "#           \"tier:build:[a,b,c][d,e,f]:chunk<k>\" + GN-11/320er-gegatete Mess-Jobs.\n";
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
        out_ += emit_ceb_build_job(c);
        out_ += emit_ceb_emit_job(c);
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

private:
    // STUFE 1a: der CEB-Bau-Job dieser Mess-Kombination. Baut die CEB (heute: comdare-messung-driver). Tag amd64
    // (broadest: der CEB-Bau ist compiler-only; die SIMD-Wahl faellt erst in der CEB-Rolle je System-Perm).
    [[nodiscard]] static std::string emit_ceb_build_job(PlanMeasurementCombo const& c) {
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
        emit_child_submodule_prolog(s); // W10-Nacharbeit 2: manueller ce-Submodul-Klon (kein Auto-Fetch)
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
    [[nodiscard]] static std::string emit_ceb_emit_job(PlanMeasurementCombo const& c) {
        std::string const slug = legend::cmake_slug(c.legend);
        std::string const art  = "tier-child-" + slug + ".yml";
        std::string       s;
        s += "# JOB ceb-emit combo " + std::to_string(c.index) + " (STUFE 1->2: CEB emittiert Child-2, CEB-Hoheit)\n";
        s += "\"" + legend::ceb_emit_job(c.legend) + "\":\n";
        s += "  stage: ceb-emit\n";
        s += "  tags: [\"amd64\"]\n";
        s += "  needs: [\"" + legend::ceb_build_job(c.legend) + "\"]\n";
        s += "  script:\n";
        emit_child_submodule_prolog(s); // W10-Nacharbeit 2: ceb:emit baut den Treiber neu -> braucht ce-Quellen
        s += "    - cd Code\n";
        s += "    - cmake -B build -G Ninja -DCOMDARE_V32_ENABLE=ON -DCMAKE_BUILD_TYPE=Release\n";
        s += "    - cmake --build build --target comdare-messung-driver\n";
        s += "    - |\n";
        s += "      set -euo pipefail\n";
        s += "      DRIVER=$(find build -type f -name \"comdare-messung-driver\" | head -1)\n";
        s += "      # §40.b-Praezisierung: die CEB (nicht der Planer) emittiert ihre STUFE-2-Sicht (System-Perms\n";
        s += "      # des FREIGEGEBENEN Raums + Tier-Chunk-Jobs + gegatete Mess-Jobs) via --emit-tier-ci.\n";
        s += "      \"$DRIVER\" --emit-tier-ci \"$COMDARE_GOLDEN_N_PROFILE\" > \"$CI_PROJECT_DIR/" + art + "\"\n";
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
        s += "  trigger:\n";
        s += "    include:\n";
        s += "      - artifact: " + art + "\n";
        s += "        job: \"" + legend::ceb_emit_job(c.legend) + "\"\n";
        s += "    strategy: depend\n";
        return s;
    }

    PlanHeader                         header_;
    std::vector<PlanPerm>              perms_;
    std::vector<std::vector<PlanStep>> steps_per_perm_;
    std::string                        out_;
};

// ── TierCiYamlBuilder — STUFE-2-Emitter (CEB-Rolle, PAKET W10-A / §42/§42.b, --emit-tier-ci). ─────────────────
//    Emittiert NUR die STUFE-2-Sicht der CE-gesteuerten Kette: den FREIGEGEBENEN System-Achsen-Raum [d,e,f]
//    (opt x simd des Director-Walks) einer CEB (=Mess-Kombination [a,b,c]) + die Tier-Chunk-Bau-Jobs + die
//    GN-11/320er-gegateten Mess-Jobs. Dies ist die CEB-HOHEIT (§40.b-Praezisierung: der Planer steuert die
//    CEB-Jobs, die CEB steuert die Tier-Jobs). Heute traegt EINE Binary beide Rollen (Planer-Rolle CiYamlBuilder
//    vs CEB-Rolle DIESER Builder) -- ehrlich getrennt ueber getrennte CLI-Modi + getrennte Emissions-Sichten.
//
//    Je System-Perm [d,e,f] (des FREIGEGEBENEN Raums der Mess-Kombination):
//      "tier:build:[a,b,c][d,e,f]:chunk<k>" (k=0..kTierChunkCount-1) -- baut die Tier-Binaries dieser Zelle;
//         der 2^17-Organ-Raum ist als chunk<k> GEBUENDELT (§42.b: 2^17 Einzel-Jobs waeren Rauschen). Die
//         Bau-Job-Legende traegt NUR HAUPT-Achsen (Mess [a,b,c] + System [d,e,f]) + chunk -- KEINE Organ-Haupt-
//         Achse (die steckt im binary_id-Artefaktpfad) und KEINE Unter-Achse (Bau=Haupt-only-Gate, §42.b).
//      "measure:[a,b,c][d,e,f][g,h,i]" -- der GN-11/320er-GEGATETE Mess-Job (when: manual = Struktur ja,
//         Auto-Messlauf nein). [g,h,i] = Organ-Referenz (fuehrende Organ-Haupt-Achsen; die reale Auffaecherung
//         ist EIN Job je Organ-Haupt-Achsen-Perm = binary_id, gated). Der Job sweept zur Laufzeit die
//         Unter-Achsen (§42.b) unter selbem Compile und schreibt EIN CSV zurueck.
//
//    GOLDEN/HOST-NEUTRAL: reine Text-Emission; nur CI-Variablen + [a,b,c][d,e,f]-Legenden als LITERALE =>
//    byte-deterministisch. Isomorph zum Director-Walk (perms()/steps_per_perm() = struktureller Zeuge).
class TierCiYamlBuilder final : public IPlanBuilder {
public:
    void begin_plan(PlanHeader const& h) override {
        header_ = h;
        out_ += "# comdare CEB-emitted tier child-pipeline (TierCiYamlBuilder v1, STUFE 2 = System-Achsen-Stufe) "
                "-- GENERIERT, deterministisch, host-unabhaengig.\n";
        out_ += "# source_kind=" + h.source_kind + " profile_id=" + h.profile_id +
                " measurement_combo_count=" + std::to_string(h.measurement_combo_count) +
                " perm_count=" + std::to_string(h.perm_count) + " chunks_per_perm=" + std::to_string(kTierChunkCount) +
                "\n";
        out_ += "#\n";
        out_ += "# Ledger §42.b (CEB-Rolle --emit-tier-ci): NUR die STUFE-2-Sicht des FREIGEGEBENEN CEB-Raums.\n";
        out_ += "#   tier:build:[a,b,c][d,e,f]:chunk<k> -- Tier-Bau (Organ-Raum als chunk gebuendelt, Haupt-only).\n";
        out_ += "#   measure:[a,b,c][d,e,f][g,h,i]      -- GN-11/320er-GEGATET (when: manual = Struktur ja, kein "
                "Auto-Messlauf).\n";
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
    // Die CEB-Rolle emittiert je Mess-Kombination (die CEB, die --emit-tier-ci aufruft) ihre STUFE-2-Sicht. Heute
    // ist das die EINE Kombination des Profils; der combo-Kontext [a,b,c] wird fuer die Perm-Legenden gehalten.
    void begin_measurement_combo(PlanMeasurementCombo const& c) override {
        combo_legend_ = c.legend;
        out_ += "\n# --- CEB-Raum " + c.legend + " (Mess-Kombination " + std::to_string(c.index) + ") ---\n";
    }
    void begin_perm(PlanPerm const& p) override {
        perms_.push_back(p);
        steps_per_perm_.emplace_back();
    }
    void on_step(PlanStep const& s) override { steps_per_perm_.back().push_back(s); }
    // Je System-Perm: die Tier-Chunk-Bau-Jobs + der gegatete Mess-Job (am Perm-Ende, wenn combo-Kontext steht).
    void end_perm(PlanPerm const& p) override {
        std::string const perm_legend = legend::system_perm(p.opt_id, p.simd_id);
        out_ += "\n# perm " + std::to_string(p.index) + ": " + perm_legend +
                " (runner-tag=" + simd_runner_tag(p.simd_id) + ")\n";
        for (std::size_t k = 0; k < kTierChunkCount; ++k) out_ += emit_tier_build_job(p, perm_legend, k);
        out_ += emit_measure_job(p, perm_legend);
    }
    void end_plan(PlanHeader const&) override {}

    [[nodiscard]] std::string const&                        text() const noexcept { return out_; }
    [[nodiscard]] PlanHeader const&                         header() const noexcept { return header_; }
    [[nodiscard]] std::vector<PlanPerm> const&              perms() const noexcept { return perms_; }
    [[nodiscard]] std::vector<std::vector<PlanStep>> const& steps_per_perm() const noexcept { return steps_per_perm_; }

private:
    // STUFE 2 Bau-Job: EIN Chunk des 2^17-Organ-Raums dieser Zelle (Haupt-only-Legende, §42.b). script =
    // provision-only-Treiber-Aufruf (Bau ohne Messung, golden-neutral), mit dem Chunk-Fenster als GN-Range.
    [[nodiscard]] std::string emit_tier_build_job(PlanPerm const& p, std::string const& perm_legend,
                                                  std::size_t k) const {
        std::string const job  = legend::tier_build_job(combo_legend_, perm_legend, k);
        std::string const opt  = p.opt_id.empty() ? std::string{"O3"} : p.opt_id;
        std::string const simd = p.simd_id.empty() ? std::string{"no_extension"} : p.simd_id;
        std::string const kstr = std::to_string(k);
        std::string       s;
        s += "# JOB tier-build combo=" + combo_legend_ + " perm " + std::to_string(p.index) + " chunk " + kstr +
             " (STUFE 2: CEB steuert Tier-Bau)\n";
        s += "\"" + job + "\":\n";
        s += "  stage: tier-build\n";
        s += "  tags: [\"" + simd_runner_tag(p.simd_id) + "\"]\n";
        s += "  resource_group: \"tier-" + legend::cmake_slug(combo_legend_) + "-" + simd + "-" + opt + "-c" + kstr +
             "\"\n";
        s += "  interruptible: false   # provision-Bau darf nie auto-cancelt werden\n";
        s += "  script:\n";
        emit_child_submodule_prolog(s); // W10-Nacharbeit 2: manueller ce-Submodul-Klon (kein Auto-Fetch)
        s += "    - cd Code\n";
        s += "    - cmake -B build -G Ninja -DCOMDARE_V32_ENABLE=ON -DCMAKE_BUILD_TYPE=Release\n";
        s += "    - cmake --build build --target comdare-messung-driver\n";
        s += "    - |\n";
        s += "      set -euo pipefail\n";
        s += "      DRIVER=$(find build -type f -name \"comdare-messung-driver\" | head -1)\n";
        s += "      # STUFE 2: Tier-Bau der Zelle [a,b,c][d,e,f]=" + combo_legend_ + perm_legend + " chunk " + kstr +
             " (provision-only, golden-neutral).\n";
        s += "      COMDARE_THESIS_PROFILE=\"$COMDARE_GOLDEN_N_PROFILE\" "
             "COMDARE_GOLDEN_N_RANGE=\"${COMDARE_GN_RANGE:-0:4}\" \\\n";
        s += "        COMDARE_GN_OPT=\"" + opt + "\" COMDARE_GN_SIMD=\"" + simd +
             "\" COMDARE_GOLDEN_N_PROVISION_ONLY=true COMDARE_RUN_SOTA=0 \\\n";
        s += "        \"$DRIVER\" experiment_config \"$CI_PROJECT_DIR/Code/gn_out/" +
             legend::cmake_slug(combo_legend_) + "/perm" + std::to_string(p.index) + "/chunk" + kstr + "\"\n";
        return s;
    }

    // STUFE 3 Mess-Job (GN-11/320er-GEGATET): when: manual = Struktur ja, Auto-Messlauf nein. [g,h,i] =
    // Organ-Referenz (fuehrende Organ-Haupt-Achsen; reale Auffaecherung = EIN Job je binary_id, gated). needs =
    // die Chunk-Bau-Jobs derselben Perm (Bau->Mess-Kante). Das echte Mess-Kommando steht NUR als Skelett-Kommentar.
    [[nodiscard]] std::string emit_measure_job(PlanPerm const& p, std::string const& perm_legend) const {
        std::string const organ = legend::organ_reference();
        std::string const job   = legend::measure_job(combo_legend_, perm_legend, organ);
        std::string const opt   = p.opt_id.empty() ? std::string{"O3"} : p.opt_id;
        std::string const simd  = p.simd_id.empty() ? std::string{"no_extension"} : p.simd_id;
        std::string       s;
        s += "# JOB measure combo=" + combo_legend_ + " perm " + std::to_string(p.index) +
             " (STUFE 3: GN-11/320er-GEGATET, when: manual = Struktur ja / kein Auto-Messlauf)\n";
        s += "\"" + job + "\":\n";
        s += "  stage: measure\n";
        s += "  tags: [\"" + simd_runner_tag(p.simd_id) + "\"]\n";
        s += "  needs:\n";
        for (std::size_t k = 0; k < kTierChunkCount; ++k)
            s += "    - \"" + legend::tier_build_job(combo_legend_, perm_legend, k) + "\"\n";
        s += "  rules:\n";
        s += "    - when: manual   # GN-11/320er-Gate: Messen erst nach User-Entscheid (kein Auto-Messlauf)\n";
        s += "  allow_failure: true\n";
        s += "  script:\n";
        s += "    - |\n";
        s += "      set -euo pipefail\n";
        s += "      echo \"measure GATED (GN-11/320er): Zelle [a,b,c][d,e,f][g,h,i]=" + combo_legend_ + perm_legend +
             organ + " -- Skelett\"\n";
        s += "      # MESS-KOMMANDO-SKELETT (GN-11/320er-gated, NICHT aktiv; COMDARE_GOLDEN_N_PROVISION_ONLY "
             "ENTFERNT = echte Messung, 1-Thread-Doktrin §38.b, voller Unter-Achsen-Sweep, EIN CSV zurueck):\n";
        s += "      #   DRIVER=$(find build -type f -name comdare-messung-driver | head -1)\n";
        s += "      #   COMDARE_THESIS_PROFILE=\"$COMDARE_GOLDEN_N_PROFILE\" COMDARE_GN_OPT=\"" + opt +
             "\" COMDARE_GN_SIMD=\"" + simd + "\" COMDARE_RUN_MEASURE=true \\\n";
        s += "      #     \"$DRIVER\" experiment_config \"$CI_PROJECT_DIR/Code/measure_out\"\n";
        return s;
    }

    PlanHeader                         header_;
    std::string                        combo_legend_ = "[all]"; // gesetzt in begin_measurement_combo
    std::vector<PlanPerm>              perms_;
    std::vector<std::vector<PlanStep>> steps_per_perm_;
    std::string                        out_;
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

// ── TierCmakeGraphBuilder — STUFE-2-Emitter (CEB-Rolle, PAKET W10-A / §42/§42.b, --emit-tier-cmake). ──────────
//    Der Bare-Metal-Gegenpart zum TierCiYamlBuilder: emittiert das tier_plan.cmake einer CEB (Mess-Kombination
//    [a,b,c]) -- je System-Perm [d,e,f] die REALEN provision-only-Tier-Bau-Targets (Organ-Raum als chunk<k>
//    gebuendelt) + je Perm EIN GN-11/320er-gegatetes measure:-Skelett-Target. Dies traegt den SCHARFEN
//    provision-only-Treiber-Aufruf (der frueher in der W7-B-CMakeGraphBuilder-Zelle lag) -- die STUFE-2 ist der
//    Ort des Tier-Baus. Ein Bare-Metal-Lauf der DREISTUFIGEN Kette: --dump-cmake (Stufe 1) -> CEB -> --emit-tier-
//    cmake (Stufe 2, DIESER Builder) -> `cmake --build <dir> --target comdare_tier_build_perm0_chunk0`.
//
//    Host-unabhaengig: Treiber/Profil/Range/Out = CMake-Variablen mit Defaults; nur [a,b,c][d,e,f]-Legenden +
//    chunk sind Plan-Konstanten. measure:-Target bleibt GN-11/320er-gegatet (Echo-Skelett, kein Auto-Messlauf).
class TierCmakeGraphBuilder final : public IPlanBuilder {
public:
    void begin_plan(PlanHeader const& h) override {
        header_ = h;
        out_ += "# comdare tier-plan (generated .cmake blueprint, TierCmakeGraphBuilder v1 = STUFE 2)\n";
        out_ += "# source_kind=" + h.source_kind + " profile_id=" + h.profile_id +
                " perm_count=" + std::to_string(h.perm_count) + " chunks_per_perm=" + std::to_string(kTierChunkCount) +
                "\n";
        out_ += "#\n";
        out_ += "# Ledger §42.b (CEB-Rolle --emit-tier-cmake): je System-Perm [d,e,f] die Tier-Chunk-Bau-Targets\n";
        out_ += "# (provision-only, SCHARF) + EIN GN-11/320er-gegatetes measure:-Skelett-Target. Bare-Metal:\n";
        out_ += "#   cmake --build <dir> --target comdare_tier_build_perm0_chunk0   # baut die Tier-DLLs (perm.dll)\n";
        out_ += "# Konfigurierbare Eingaben (per -D ueberschreibbar):\n";
        out_ += "#   COMDARE_PLAN_DRIVER  = Pfad/Name des comdare-messung-driver (Default: PATH-Suche)\n";
        out_ += "#   COMDARE_PLAN_PROFILE = Thesis-/Experiment-Profil-XML (Default: leer => Treiber-Default-Profil)\n";
        out_ += "#   COMDARE_PLAN_RANGE   = golden-N Chunk-Fenster start:count (Default: 0:4 = SICHER klein)\n";
        out_ += "#   COMDARE_PLAN_OUT     = Ausgabe-Wurzel fuer die provision-DLLs (Default: <bindir>/tier/out)\n";
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
    }
    void begin_measurement_combo(PlanMeasurementCombo const& c) override {
        combo_legend_ = c.legend;
        out_ += "\n# --- CEB-Raum " + c.legend + " (Mess-Kombination " + std::to_string(c.index) + ") ---\n";
    }
    void begin_perm(PlanPerm const& p) override {
        perms_.push_back(p);
        steps_per_perm_.emplace_back();
    }
    void on_step(PlanStep const& s) override { steps_per_perm_.back().push_back(s); }
    void end_perm(PlanPerm const& p) override {
        std::string const idx         = std::to_string(p.index);
        std::string const opt         = p.opt_id.empty() ? std::string{"O3"} : p.opt_id;
        std::string const simd        = p.simd_id.empty() ? std::string{"no_extension"} : p.simd_id;
        std::string const slug        = legend::cmake_slug(combo_legend_);
        std::string const stemdir     = "${CMAKE_CURRENT_BINARY_DIR}/tier";
        std::string const perm_legend = legend::system_perm(p.opt_id, p.simd_id);
        out_ += "\n# perm " + idx + ": [a,b,c][d,e,f]=" + combo_legend_ + perm_legend + "\n";
        // K Tier-Chunk-Bau-Targets (provision-only, SCHARF) je System-Perm. Organ-Raum als chunk<k> gebuendelt.
        std::vector<std::string> chunk_stamps;
        for (std::size_t k = 0; k < kTierChunkCount; ++k) {
            std::string const kstr   = std::to_string(k);
            std::string const bstamp = stemdir + "/" + slug + "_perm" + idx + "_chunk" + kstr + ".build.stamp";
            std::string const tgt    = tier_build_target(p.index, k);
            std::string const cell   = "combo=" + combo_legend_ + " perm " + idx + " " + perm_legend + " chunk " + kstr;
            chunk_stamps.push_back(bstamp);
            out_ += "if(NOT TARGET " + tgt + ")\n";
            out_ += "    add_custom_command(\n";
            out_ += "        OUTPUT \"" + bstamp + "\"\n";
            out_ += "        COMMAND \"${CMAKE_COMMAND}\" -E make_directory \"" + stemdir + "\"\n";
            out_ += "        COMMAND \"${CMAKE_COMMAND}\" -E echo \"tier:build (provision-only): " + cell + "\"\n";
            out_ += "        COMMAND \"${CMAKE_COMMAND}\" -E env\n";
            out_ += "            \"COMDARE_THESIS_PROFILE=${COMDARE_PLAN_PROFILE}\"\n";
            out_ += "            \"COMDARE_GOLDEN_N_RANGE=${COMDARE_PLAN_RANGE}\"\n";
            out_ += "            \"COMDARE_GN_OPT=" + opt + "\"\n";
            out_ += "            \"COMDARE_GN_SIMD=" + simd + "\"\n";
            out_ += "            COMDARE_GOLDEN_N_PROVISION_ONLY=true\n";
            out_ += "            COMDARE_RUN_SOTA=0\n";
            out_ += "            \"${COMDARE_PLAN_DRIVER}\" experiment_config \"${COMDARE_PLAN_OUT}/" + slug + "/perm" +
                    idx + "/chunk" + kstr + "\"\n";
            out_ += "        COMMAND \"${CMAKE_COMMAND}\" -E touch \"" + bstamp + "\"\n";
            out_ += "        COMMENT \"tier:build (provision-only): " + cell + "\"\n";
            out_ += "        VERBATIM)\n";
            out_ += "    add_custom_target(" + tgt + " DEPENDS \"" + bstamp + "\")\n";
            out_ += "endif()\n";
        }
        // measure:-Target (GN-11/320er-GEGATET): Echo-Skelett, DEPENDS auf ALLE Chunk-Bau-Stamps derselben Perm
        // (Bau->Mess-Kante). Das echte Mess-Kommando steht NUR als Kommentar-Skelett (kein Auto-Messlauf).
        std::string const mstamp = stemdir + "/" + slug + "_perm" + idx + ".measure.stamp";
        std::string const mtgt   = tier_measure_target(p.index);
        std::string const organ  = legend::organ_reference();
        out_ += "if(NOT TARGET " + mtgt + ")\n";
        out_ += "    add_custom_command(\n";
        out_ += "        OUTPUT \"" + mstamp + "\"\n";
        out_ += "        COMMAND \"${CMAKE_COMMAND}\" -E echo\n";
        out_ += "            \"measure GATED (GN-11/320er): [a,b,c][d,e,f][g,h,i]=" + combo_legend_ + perm_legend +
                organ + " -- Skelett\"\n";
        out_ += "        # MESS-KOMMANDO-SKELETT (GN-11/320er-gated, NICHT aktiv; PROVISION_ONLY ENTFERNT = echte "
                "Messung, 1-Thread §38.b):\n";
        out_ += "        #   \"${CMAKE_COMMAND}\" -E env \"COMDARE_THESIS_PROFILE=${COMDARE_PLAN_PROFILE}\"\n";
        out_ += "        #     \"COMDARE_GN_OPT=" + opt + "\" \"COMDARE_GN_SIMD=" + simd +
                "\" COMDARE_RUN_MEASURE=true \\\n";
        out_ += "        #     \"${COMDARE_PLAN_DRIVER}\" experiment_config \"${COMDARE_PLAN_OUT}/measure\"\n";
        out_ += "        COMMAND \"${CMAKE_COMMAND}\" -E touch \"" + mstamp + "\"\n";
        for (auto const& st : chunk_stamps) out_ += "        DEPENDS \"" + st + "\" # tier:build->measure:-Kante\n";
        out_ +=
            "        COMMENT \"measure (GN-11/320er-gated skeleton): combo=" + combo_legend_ + " perm " + idx + "\"\n";
        out_ += "        VERBATIM)\n";
        out_ += "    add_custom_target(" + mtgt + " DEPENDS \"" + mstamp + "\")\n";
        out_ += "endif()\n";
    }
    void end_plan(PlanHeader const&) override {
        out_ += "\n# Aggregat: alle measure:-Targets (transitiv alle tier:build-Targets via DEPENDS-Kante).\n";
        out_ += "if(NOT TARGET " + all_target() + ")\n";
        out_ += "    add_custom_target(" + all_target() + " DEPENDS";
        for (auto const& p : perms_) out_ += "\n        " + tier_measure_target(p.index);
        out_ += ")\n";
        out_ += "endif()\n";
    }

    [[nodiscard]] std::string const&                        text() const noexcept { return out_; }
    [[nodiscard]] PlanHeader const&                         header() const noexcept { return header_; }
    [[nodiscard]] std::vector<PlanPerm> const&              perms() const noexcept { return perms_; }
    [[nodiscard]] std::vector<std::vector<PlanStep>> const& steps_per_perm() const noexcept { return steps_per_perm_; }

private:
    [[nodiscard]] static std::string tier_build_target(std::size_t i, std::size_t k) {
        return "comdare_tier_build_perm" + std::to_string(i) + "_chunk" + std::to_string(k);
    }
    [[nodiscard]] static std::string tier_measure_target(std::size_t i) {
        return "comdare_tier_measure_perm" + std::to_string(i);
    }
    [[nodiscard]] static std::string all_target() { return "comdare_tier_plan_all"; }

    PlanHeader                         header_;
    std::string                        combo_legend_ = "[all]";
    std::vector<PlanPerm>              perms_;
    std::vector<std::vector<PlanStep>> steps_per_perm_;
    std::string                        out_;
};

// ── ExperimentPlanDirector — DIRECTOR (GoF Builder). EIN deterministischer Walk, zwei Kanaele. ──────────────
class ExperimentPlanDirector {
public:
    ExperimentPlanDirector() = default;
    explicit ExperimentPlanDirector(PlanRegistryTrioAnnotation trio) : trio_(std::move(trio)) {}

    /// Thesis-Kanal: opt x simd x profile_sweep_passes(tp, ""). KEIN Bau; die Selektions-Pass-Liste ist die
    /// deterministische #26/GO-5-Enumeration (Basis-Pass zuerst + je <axis_sweep> ein Pass in Dokument-Reihenfolge).
    void construct(cx::ThesisProfile const& tp, IPlanBuilder& b) const {
        std::vector<std::string> const          opt_perms  = opt_perms_of(tp.compiler.opt_levels);
        std::vector<std::string> const          simd_perms = simd_perms_of(tp.extension_hardware.simd_options);
        std::vector<std::string> const          passes     = tlz::profile_sweep_passes(tp, /*requested_axis=*/"");
        std::vector<PlanMeasurementCombo> const combos     = measurement_combos_of(tp.measurement_categories);
        walk_perms_("thesis", tp.id, combos, opt_perms, simd_perms, b, [&](IPlanBuilder& bb) {
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
    void construct(cx::ExperimentProfile const& ep, IPlanBuilder& b) const {
        std::vector<std::string> const opt_perms  = opt_perms_of(ep.compiler.opt_levels);
        std::vector<std::string> const simd_perms = simd_perms_of(ep.extension_hardware.simd_options);
        std::vector<tlz::ExperimentPhaseProjection> const projections = tlz::project_experiment_to_sota_passes(ep);
        std::vector<PlanMeasurementCombo> const           combos = measurement_combos_of(ep.measurement_categories);
        walk_perms_("experiment", ep.id, combos, opt_perms, simd_perms, b, [&](IPlanBuilder& bb) {
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

    // W10-A (§42): die Mess-Achsen-Kombinationen aus der Anwender-XML (<measurement_categories>). Heute traegt die
    // Schema EINEN <measurement_categories>-Block => GENAU EINE Kombination (der CEB-Typ [a,b,c]); die Legende ist
    // die kanonische Kurzform (legend::measurement_combo; `[all]` = alle 16 / kein Subset). Die Rueckgabe ist ein
    // VECTOR (Struktur fuer die Zukunft: mehrere deklarierte Mess-Systeme => mehrere CEB-Pipelines), heute size==1.
    // Ersetzt den konzeptlosen GN_MSYS="default"-Platzhalter des W4-Piloten durch die ECHT deklarierte Legende.
    [[nodiscard]] static std::vector<PlanMeasurementCombo>
    measurement_combos_of(std::vector<std::string> const& measurement_categories) {
        PlanMeasurementCombo combo;
        combo.index      = 0;
        combo.categories = measurement_categories;
        combo.legend     = legend::measurement_combo(measurement_categories);
        return {combo};
    }

    // Der EINE dreistufige Walk (Mess-Kombination -> System-Perm (opt x simd) -> Chunk-Buendel) + Builder-Treiber.
    // Der Steps-Emitter ist der einzige art-abhaengige Teil (er ruft die BESTEHENDEN Callees). KEIN dritter
    // Enumerations-Walk: opt/simd/pass/phase stammen vollstaendig aus profile_sweep_passes/
    // project_experiment_to_sota_passes + den XML-System-Achsen-Listen; die Mess-Kombinationen aus
    // measurement_combos_of. perm_count = |opt x simd| JE Mess-Kombination (heute 1 Kombination => byte-identische
    // Perm-Menge zum Vor-W10-Verhalten; der perm_index laeuft ueber den gesamten Walk).
    template <class StepEmitter>
    void walk_perms_(std::string_view source_kind, std::string const& profile_id,
                     std::vector<PlanMeasurementCombo> const& combos, std::vector<std::string> const& opt_perms,
                     std::vector<std::string> const& simd_perms, IPlanBuilder& b, StepEmitter&& emit_steps) const {
        PlanHeader header;
        header.source_kind             = std::string{source_kind};
        header.profile_id              = profile_id;
        header.perm_count              = opt_perms.size() * simd_perms.size();
        header.measurement_combo_count = combos.size();
        header.registries              = trio_;
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
};

} // namespace comdare::cache_engine::planner
