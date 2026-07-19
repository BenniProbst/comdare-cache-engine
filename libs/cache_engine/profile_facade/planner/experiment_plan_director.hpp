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
#include "validate_profile.hpp" // RegistryTrio / RegistryContents (Registry-Trio-Annotation des Plan-Kopfs)

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

/// Kopf des Plans: Provenienz (Quelle/Profil) + Perm-Zahl + Registry-Trio-Annotation.
struct PlanHeader {
    std::string                source_kind; // "thesis" | "experiment"
    std::string                profile_id;
    std::size_t                perm_count = 0; // |opt x simd|
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
        out_ += "perm_count=" + std::to_string(h.perm_count) + "\n";
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

// ── CMakeGraphBuilder — zweiter ConcreteBuilder (GoF Builder) am SELBEN Director-Walk (PAKET W5-B / I2). ──────
//    Emittiert ein deterministisches experiment_plan.cmake: die Bau-/Mess-Matrix als CMake-Graph. Pro Perm
//    (opt x simd-Zelle) EIN build:-Schritt (Bau der Zell-Binaries) + EIN measure:-Schritt; die build:->measure:-
//    Kante ist das DEPENDS des measure- auf den build-Stamp (Blaupause add_custom_command/DEPENDS/VERBATIM,
//    cmake/catalog_codegen.cmake:27-37). REIN beschreibend: die COMMANDs sind No-Op (echo+touch), es findet KEIN
//    echter Compile/Run statt (Anti-Phantom, golden-neutral, wie der PlanTextBuilder-Schwesterzweig). Host-
//    unabhaengig reproduzierbar: der Text traegt nur CMake-Generator-Variablen (${CMAKE_CURRENT_BINARY_DIR}) als
//    LITERALE, keine emit-Zeit-Host-Werte => byte-deterministisch. Isomorph zum PlanTextBuilder: BEIDE sehen
//    ueber denselben Director-Walk dieselbe begin_perm/on_step-Folge (die perms()/steps_per_perm()-Aufzeichnung
//    ist der strukturelle Zeuge fuer den Contract-Test).
class CMakeGraphBuilder final : public IPlanBuilder {
public:
    void begin_plan(PlanHeader const& h) override {
        header_ = h;
        out_ += "# comdare-experiment-plan (generated .cmake blueprint, CMakeGraphBuilder v1)\n";
        out_ += "# source_kind=" + h.source_kind + " profile_id=" + h.profile_id +
                " perm_count=" + std::to_string(h.perm_count) +
                " registry_trio_loaded=" + std::string(h.registries.loaded ? "1" : "0") + "\n";
        out_ += "#\n";
        out_ += "# Deterministische Bau-/Mess-Matrix als CMake-Graph: pro Perm (opt x simd) EIN build:- + EIN\n";
        out_ += "# measure:-Schritt; die build:->measure:-Kante ist das DEPENDS des measure- auf den build-Stamp.\n";
    }
    void begin_perm(PlanPerm const& p) override {
        perms_.push_back(p);
        steps_per_perm_.emplace_back();
        out_ += "\n# --- perm " + std::to_string(p.index) + ": opt=" + nz(p.opt_id) + " simd=" + nz(p.simd_id) +
                " build_version_suffix=" + nz(p.build_version_suffix) + " ---\n";
    }
    void on_step(PlanStep const& s) override {
        steps_per_perm_.back().push_back(s);
        out_ += "#   step " + std::to_string(s.index) + " kind=" + nz(s.kind) +
                " label=" + (s.label.empty() ? std::string{"<basis>"} : s.label) + " merge=" + nz(s.merge) +
                " binary_id=" + nz(s.binary_id) + " lebewesen=" + nz(s.lebewesen) + "\n";
    }
    void end_perm(PlanPerm const& p) override {
        std::string const idx    = std::to_string(p.index);
        std::string const stem   = "${CMAKE_CURRENT_BINARY_DIR}/experiment_plan/perm" + idx;
        std::string const bstamp = stem + ".build.stamp";
        std::string const mstamp = stem + ".measure.stamp";
        std::string const cell   = "perm " + idx + " opt=" + nz(p.opt_id) + " simd=" + nz(p.simd_id) +
                                   " steps=" + std::to_string(steps_per_perm_.back().size());
        // build:-Schritt (Bau der Zell-Binaries dieser opt x simd-Perm).
        out_ += "if(NOT TARGET " + build_target(p.index) + ")\n";
        out_ += "    add_custom_command(\n";
        out_ += "        OUTPUT \"" + bstamp + "\"\n";
        out_ += "        COMMAND \"${CMAKE_COMMAND}\" -E echo \"build: " + cell + "\"\n";
        out_ += "        COMMAND \"${CMAKE_COMMAND}\" -E touch \"" + bstamp + "\"\n";
        out_ += "        COMMENT \"build: " + cell + "\"\n";
        out_ += "        VERBATIM)\n";
        out_ += "    add_custom_target(" + build_target(p.index) + " DEPENDS \"" + bstamp + "\")\n";
        out_ += "endif()\n";
        // measure:-Schritt mit build:->measure:-Kante (DEPENDS auf den build-Stamp derselben Perm).
        out_ += "if(NOT TARGET " + measure_target(p.index) + ")\n";
        out_ += "    add_custom_command(\n";
        out_ += "        OUTPUT \"" + mstamp + "\"\n";
        out_ += "        COMMAND \"${CMAKE_COMMAND}\" -E echo \"measure: " + cell + "\"\n";
        out_ += "        COMMAND \"${CMAKE_COMMAND}\" -E touch \"" + mstamp + "\"\n";
        out_ += "        DEPENDS \"" + bstamp + "\" # build:->measure:-Kante\n";
        out_ += "        COMMENT \"measure: " + cell + "\"\n";
        out_ += "        VERBATIM)\n";
        out_ += "    add_custom_target(" + measure_target(p.index) + " DEPENDS \"" + mstamp + "\")\n";
        out_ += "endif()\n";
    }
    void end_plan(PlanHeader const&) override {
        // Aggregat-Target: alle measure:-Schritte (=> transitiv alle build:-Schritte via build:->measure:-Kante).
        out_ += "\n# Aggregat: alle measure:-Schritte (transitiv alle build:-Schritte via DEPENDS-Kante).\n";
        out_ += "if(NOT TARGET " + all_target() + ")\n";
        out_ += "    add_custom_target(" + all_target() + " DEPENDS";
        for (auto const& p : perms_) out_ += "\n        " + measure_target(p.index);
        out_ += ")\n";
        out_ += "endif()\n";
    }

    [[nodiscard]] std::string const&                        text() const noexcept { return out_; }
    [[nodiscard]] PlanHeader const&                         header() const noexcept { return header_; }
    [[nodiscard]] std::vector<PlanPerm> const&              perms() const noexcept { return perms_; }
    [[nodiscard]] std::vector<std::vector<PlanStep>> const& steps_per_perm() const noexcept { return steps_per_perm_; }

private:
    [[nodiscard]] static std::string nz(std::string const& s) { return s.empty() ? std::string{"-"} : s; }
    [[nodiscard]] static std::string build_target(std::size_t i) {
        return "comdare_experiment_plan_build_perm" + std::to_string(i);
    }
    [[nodiscard]] static std::string measure_target(std::size_t i) {
        return "comdare_experiment_plan_measure_perm" + std::to_string(i);
    }
    [[nodiscard]] static std::string all_target() { return "comdare_experiment_plan_all"; }

    PlanHeader                         header_;
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

// ── ExperimentPlanDirector — DIRECTOR (GoF Builder). EIN deterministischer Walk, zwei Kanaele. ──────────────
class ExperimentPlanDirector {
public:
    ExperimentPlanDirector() = default;
    explicit ExperimentPlanDirector(PlanRegistryTrioAnnotation trio) : trio_(std::move(trio)) {}

    /// Thesis-Kanal: opt x simd x profile_sweep_passes(tp, ""). KEIN Bau; die Selektions-Pass-Liste ist die
    /// deterministische #26/GO-5-Enumeration (Basis-Pass zuerst + je <axis_sweep> ein Pass in Dokument-Reihenfolge).
    void construct(cx::ThesisProfile const& tp, IPlanBuilder& b) const {
        std::vector<std::string> const opt_perms  = opt_perms_of(tp.compiler.opt_levels);
        std::vector<std::string> const simd_perms = simd_perms_of(tp.extension_hardware.simd_options);
        std::vector<std::string> const passes     = tlz::profile_sweep_passes(tp, /*requested_axis=*/"");
        walk_perms_("thesis", tp.id, opt_perms, simd_perms, b, [&](IPlanBuilder& bb) {
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
        walk_perms_("experiment", ep.id, opt_perms, simd_perms, b, [&](IPlanBuilder& bb) {
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

    // Der EINE Perm-Walk (opt x simd) + Builder-Treiber. Der Steps-Emitter ist der einzige art-abhaengige Teil
    // (er ruft die BESTEHENDEN Callees). KEIN dritter Enumerations-Walk: die opt/simd/pass/phase-Enumeration
    // stammt vollstaendig aus profile_sweep_passes/project_experiment_to_sota_passes + den XML-System-Achsen-Listen.
    template <class StepEmitter>
    void walk_perms_(std::string_view source_kind, std::string const& profile_id,
                     std::vector<std::string> const& opt_perms, std::vector<std::string> const& simd_perms,
                     IPlanBuilder& b, StepEmitter&& emit_steps) const {
        PlanHeader header;
        header.source_kind = std::string{source_kind};
        header.profile_id  = profile_id;
        header.perm_count  = opt_perms.size() * simd_perms.size();
        header.registries  = trio_;
        b.begin_plan(header);

        std::size_t perm_index = 0;
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
        b.end_plan(header);
    }

    PlanRegistryTrioAnnotation trio_; // leer/loaded=false, wenn ohne Registry-Trio konstruiert
};

} // namespace comdare::cache_engine::planner
