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
//           external_utils.simd_options) + system_axis_opt_flag_of / system_axis_march_of
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
#include "../system_version_suffix.hpp" // Lane F R3: die EINE Suffix-Quelle
//   + profile_sweep_passes (via profile_runner.hpp) + project_experiment_to_sota_passes
//   (via sota_catalog.hpp) + cm::Default*Option + cx::ThesisProfile/XmlConfigParser
#include "validate_profile.hpp"    // RegistryTrio / RegistryContents (Registry-Trio-Annotation des Plan-Kopfs)
#include "planner/plan_legend.hpp" // W10-A: das dreistufige Legenden-Namensschema (EINE Formatierungs-Single-Source)
#include <builder/build_orchestrator/build_orchestrator.hpp> // OP-9: ex::orch_make_stem == die REALE Stem-Bildung
#include <builder/experiment_tree/slice_marker.hpp>   // E-04-P1: kSliceMarkerTraceMarken (Tee-Filter-Single-Source)
#include <builder/bestandslog/planer_block_value.hpp> // G4b-2/E4: make_planer_block_reservation_value (der Wert-Kern)
#include <builder/bestandslog/reservation_lifecycle.hpp> // G4a(7): make_pro_forma_reservation / BatchTyp::planer_block
// LAG-P4 (Korn-Wache, s. unten bei kGnBatchSlice): die beiden ANDEREN Traeger des 4096er-Korns. Der Weg
// hierher besteht ohnehin transitiv (profile_run_entry.hpp -> cache_engine_builder_iterator.hpp ->
// planer_driven_build.hpp -> batch_planner.hpp); die Wache haengt aber NICHT an einer fremden
// Include-Kette, die ein spaeterer Umbau still kappen koennte -- sie zieht ihre Operanden selbst.
#include <builder/bestandslog/planer_driven_build.hpp> // LAG-P4: bestandslog::kBuildSliceGrain (+ kGnBatchSlice)
#include <cache_engine/measurement/run_methodology_registry.hpp> // S5-P1: WorkMode-Registry (Single-Source)
#include "planner/pmc_host_probe.hpp" // 10.08.2026: die LAUFZEIT-Erkennung der PMC-Hardwareform dieses Hosts

#include "xml_config_parser/xml_config_parser.hpp" // cx::ExperimentProfile / cx::ThesisProfile (explizit)

#include <algorithm> // S5-P1: std::find ueber das A9.1-Feld run_methodology (Build-Semantik-Aufloesung)
#include <cstdlib>   // S2-NACHT-2: std::getenv (Forward-Werte LITERAL aus der Planer-Env, kein X: "$X")
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
namespace bex = ::comdare::cache_engine::builder::experiment; // E-04-P1: die Marken der Marker-Familie v2

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

/// S5-P1 (P-VOLLZUG, 2026-07-20): die vom Planer aufgeloeste Build-/Mess-Semantik der S5-Mess-Strecke -- die Zeile
/// der AKTIVEN Methodik aus der run_methodology_registry. Default = measure-Semantik (Release/misst/1-Thread) =>
/// byte-identisch zum Vor-S5-tier:build. GOLDEN/binary_id-NEUTRAL: reine Bau-/Mess-Matrix, KEIN Stempel.
///
/// WER SIE LIEST -- gezaehlt, nicht behauptet (Welle B/3, 2026-08-07; Nenner: ALLE Zugriffe der Form
/// header_.build_semantic.* im Repo, 8 Stueck):
///     cmake_build_type   8 Leser (Tier-Emitter: -DCMAKE_BUILD_TYPE, COMDARE_BUILD_TYPE-Env, (j3)-Dual-Compile)
///     measurement_on     0 Leser
///     single_thread      0 Leser
/// Die beiden letzten werden GESCHRIEBEN UND NIE GELESEN. Sie bleiben trotzdem stehen und sind ausdruecklich NICHT
/// zurueckgebaut: sie sind der benannte S6-Konsum (der per-Methodik-Fanout {build,measure,compare,release} zu N
/// Mess-Strecken; A-05/V-12: debug ausgebaut). Ein stiller Rueckbau von etwas Gemeintem waere teurer als ein totes
/// Feld. Was NICHT bleibt, ist
/// die Behauptung, sie wuerden gelesen -- die stand bis zum 07.08.2026 hier und liess ein Phantom wie eine
/// Verdrahtung aussehen.
///
/// WARUM DER EMITTER measurement_on HEUTE NICHT BRAUCHT (und warum man es nicht beilaeufig "nachverdrahten" darf,
/// ohne den Fanout zu bauen): der Emitter verzweigt auf cmake_build_type == "Debug" -- das ist die BAU-Frage
/// (Reuse-Schluessel, (j3)-Vorlauf, ARTEFAKT_TRIES), nicht die Mess-Frage. Die Mess-Frage stellt die Runtime-Naht,
/// und die liest die REGISTRY-Zeile direkt (measure_parallelism.hpp: !m.measurement_on || m.single_thread =>
/// 1-Thread), nicht diesen Plan-seitigen Spiegel. Der Spiegel ist deshalb heute reine Plan-Annotation. Damit er als
/// Annotation nicht still falsch werden kann, nagelt ihn eine Wache je Modus an die Registry-Zeile
/// (test_experiment_plan_director: PlanBuildSemanticSpiegeltDieRegistryZeileFuerJedenModus).
struct PlanBuildSemantic {
    std::string cmake_build_type = "Release"; // CMAKE_BUILD_TYPE des Tier-Baus/Mess-Baus (measure => "Release")
    bool        measurement_on   = true;      // misst das Profil (measure: true; build/compare/release: false;
                                              // A-05/V-12: debug ausgebaut) -- 0 Leser, S6-Fanout-Reserve (s.o.)
    bool single_thread = true; // 1-Thread-deterministischer Mess-Vollzug (Section 38.b) -- 0 Leser, S6 (s.o.)
};

/// Kopf des Plans: Provenienz (Quelle/Profil) + Perm-Zahl + Registry-Trio-Annotation.
// Plan-Kopf v1.1: die profil-seitigen Substanz-Zahlen als EIN Argument (statt zweier loser size_t am
// ohnehin langen walk_perms_-Kopf). Je Profil-Art anders erhoben, s. PlanHeader::profile_axis_count.
struct PlanProfileSubstance {
    std::size_t axes   = 0;
    std::size_t values = 0;
};

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
    // V-2/2a-Nachzug (Plan-Kopf v1.1, 2026-07-27): die PROFIL-SEITIGE Substanz -- wie viele Achsen und
    // Werte das Anwender-Profil ueberhaupt deklariert. Warum das noetig wurde: perm_count und die
    // Schritt-Zahl sind fuer ein Thesis-Profil IMMER >= 1 (es gibt stets eine Default-System-Perm und den
    // Basis-Pass), ein achsenloses Profil sah im Plan also aus wie ein gueltiger Lauf und fiel erst tief
    // unten an der Achse-2-Wache um. Diese beiden Zahlen machen "das Profil deklariert nichts" AM PLAN
    // sichtbar -- der Validat kannte sie laengst ("geprueft: N Achsen, M Werte"), der Plan nicht.
    // Thesis: |permute_axes| und die Summe ihrer <value>-Eintraege. Experiment: |axes_default_lookup| und
    // die Summe der allowed_variants. Reine Kopf-Annotation, kein Filter (binary_id-neutral).
    std::size_t                profile_axis_count  = 0;
    std::size_t                profile_value_count = 0;
    PlanRegistryTrioAnnotation registries;
    // S3 P-RESOLVER (2026-07-20): der klassifizierte Organ-Position-Reject/Route-Report (resolve_axis_refs_against_
    // trio). resolved=false (INERT-Default) wenn der Director OHNE volles RegistryTrio konstruiert wurde; sonst
    // ok=true bei organ-reinem Profil (0 Rejects). binary_id-neutral -- reine Plan-Kopf-Annotation (kein Filter).
    tlz::ResolverReport resolver;
    // S5-P1: die aufgeloeste Build-/Mess-Semantik (Zeile der aktiven Methodik). Nur der Tier-Emitter
    // (emit_batch_targets / emit_batch_*_job) greift ueberhaupt zu, und AUSSCHLIESSLICH auf cmake_build_type; die
    // uebrigen Builder rendern sie NICHT (=> ihre Emission unveraendert). measurement_on/single_thread haben heute
    // 0 Leser und sind fuer den S6-Fanout reserviert -- die Zaehlung steht an der Struct-Doku. Default
    // (measure => Release) haelt den tier:build-Teil byte-identisch zu HEAD.
    PlanBuildSemantic build_semantic;
    // I-PMC-2 (Owner 10.08.2026): der auf DIESEM Host GEMESSENE PMC-Befund. Er sitzt im Plan-KOPF und nicht
    // an den vier Emissionsstellen, weil es genau EINE Erhebung je Planer-Lauf gibt -- vier Erhebungen
    // koennten vier verschiedene Antworten geben (Rechte-Wechsel zwischen den Aufrufen) und die Emission in
    // sich widerspruechlich machen. Default = Unbrauchbar/probe_gefahren=false: wer nicht gefragt hat,
    // behauptet kein PMC (fail-closed). Der Planer-Einstieg (apps/experiment_planner) fragt einmal.
    PmcHostBefund pmc_befund;
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
    std::string merge;     // experiment: PrueflingVerbundStrategy ; thesis: "-"
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

// W2 (Owner-GO mittag-6 R1 "-D-Define wie empfohlen", Fork-A-Minimalhaerte): der CONFIGURE-ZUSATZ, der die
// gewaehlte Mess-Combo COMPILE-hart in die CEB einbaut. EINE Ableitung fuer ALLE VIER CEB-Compile-Stellen der
// Emission (ceb:build + ceb:emit der Stufe 1; tier-build-batch + measure-batch der Stufe 2) -- die vier Stellen
// sind genau die, an denen der comdare-messung-driver=CEB kompiliert wird und damit die DLL-Quellen samt
// Mess-Stempel real ENTSTEHEN. Die TIER-DLL-Compile-Flags (make_gpp_compile_fn) bleiben UNANGETASTET.
//
// FORM: spezifische Combo => der doppelt gequotete -D-Zusatz, damit die []-Klammern der Legende nicht der
// Shell-Glob-Expansion anheimfallen.
//
// F-B1 (CODEX-NACHREVIEW W1/W2, Ledger-Nachtrag 05.08.2026 nachmittag-7) -- WARUM [all] NICHT MEHR SCHWEIGT:
// COMDARE_MEASUREMENT_COMBO ist eine CMAKE-CACHE-VARIABLE (profile_facade/CMakeLists.txt). Cache-Variablen sind
// STICKY: ein Build-Verzeichnis, das zuvor mit -DCOMDARE_MEASUREMENT_COMBO=<spezifisch> konfiguriert wurde,
// behaelt den Wert -- und damit das Compile-Define COMDARE_MEASUREMENT_COMBO_CT -- bei jedem Folge-Configure,
// der die Variable nicht anfasst. Genau das ist die Lage der emittierten Jobs: sie konfigurieren stets
// `cmake -B build` in EINEM wiederverwendeten Verzeichnis, und die Batch-Jobs halten `build`/`gn_out` per
// emit_gn_out_persistence_variables sogar ueber den Checkout-Clean hinweg. Ein [all]-Folgelauf haette also
// still die Combo des Vorgaengers weitergefahren. Ein SCHWEIGENDER Zusatz kann diesen Zustand nicht aufloesen;
// nur eine EXPLIZITE Anweisung kann es.
//
// GEWAEHLTE FORM: -U (Cache-Eintrag ENTFERNEN), nicht die leere Zuweisung -D...= . Beide loeschen die Wirkung
// (live geprueft: nach -DCOMDARE_MEASUREMENT_COMBO=[wallclock] liefert ein Folge-Configure ohne Zusatz weiterhin
// das Define, mit -U bzw. mit leerer -D-Zuweisung nicht mehr). -U ist der praezisere Ausdruck, weil er die
// ABSICHT nennt statt einen Wert zu setzen: [all] heisst "keine Combo gewaehlt", nicht "Combo == Leerstring".
// Nach dem Entfernen legt der `set(... CACHE STRING)` des Projekts den Eintrag mit seinem eigenen Default neu
// an -- der Cache steht danach exakt wie in einem jungfraeulichen Build-Verzeichnis. -U ist ausserdem auf einem
// Verzeichnis OHNE den Eintrag ein folgenloser No-Op (Glob ohne Treffer, kein Fehler), taugt also an genau
// derselben Stelle fuer beide Zustaende. Die angehaengte Schreibweise -UNAME (ohne Leerzeichen) und die
// Stellung HINTER den -D-Argumenten sind live gegen cmake 4.3.4 verifiziert.
//
// KONSEQUENZ, BEWUSST UND DEKLARIERT: die [all]-Emission ist damit NICHT MEHR byte-identisch zur Vor-F-B1-Form
// (die W1-Byte-Identitaets-Aussage bezog sich auf den Stand VOR diesem Fix). Betroffen ist AUSSCHLIESSLICH der
// emittierte JOB-TEXT -- kein Tier-Fingerprint, kein Stempel-Byte, keine XML.
//
// Die zweite Verteidigungslinie liegt an der Stempel-Naht (F-B2, measurement_stamp_from_env): ein dennoch
// einkompiliertes Define ohne synchrone Env wirft dort fail-loud. Deshalb braucht der bare-metal-Weg
// (CMakeGraphBuilder, der den Treiber NICHT selbst konfiguriert, s.u.) keine eigene [all]-Hinweiszeile.
//
// Der Env-Export (combo_env / combo_line) BLEIBT daneben bestehen: er speist +mtool (Objekt-Cache-Key) und die
// Bestandslog-Zelle z.combo -- das ist die W-11-Flaeche, die diese Welle bewusst NICHT entscheidet.

/// Die EINE [all]/Vollmengen-Erkennung der Emissionsseite. Frueher war sie in ceb_combo_compile_define
/// eingeschlossen und wurde von der Hinweis-Stelle indirekt ueber "Rueckgabe leer?" abgefragt -- seit F-B1
/// liefert die Funktion auch fuer [all] Text, also braucht das Praedikat einen eigenen Namen (sonst haette die
/// Hinweis-Stelle still ihre Bedingung invertiert).
[[nodiscard]] inline bool ceb_combo_is_full_set(std::string const& combo_legend) {
    return combo_legend.empty() || combo_legend == "[all]";
}

/// NAMENS-VERMERK (F-B1): die Funktion liefert seit dieser Welle BEIDE Richtungen des Defines -- die Zuweisung
/// (-D) UND die Loeschung (-U). Der Name bleibt UNVERAENDERT, weil er die SACHE trifft ("das Argument, das ueber
/// das Compile-Define entscheidet") und eine Umbenennung fuenf Aufrufstellen plus Pins ohne Substanzgewinn
/// bewegte; ein praeziserer Name (ceb_combo_compile_define_arg) ist Kandidat fuer den Abschluss-Aufraeumpass.
[[nodiscard]] inline std::string ceb_combo_compile_define(std::string const& combo_legend) {
    if (ceb_combo_is_full_set(combo_legend)) return " -UCOMDARE_MEASUREMENT_COMBO";
    return " \"-DCOMDARE_MEASUREMENT_COMBO=" + combo_legend + "\"";
}

// M-2 / B1 (P-PMC-1, 2026-08-06) -- DIE PMC-PFLICHT ALS INVARIANTE, NICHT ALS JOB-NAME.
//
// I-PMC-1 (F9, User 2026-07-16 "PFLICHT fuer Vollstaendigkeit aller perf-Messwerte"): -DCOMDARE_ENABLE_PMC=ON
// aktiviert unter __linux__ die reale perf_event_open-Quelle (pmc_source_factory.hpp:19/32; prod1-Rechte
// seit 25.06. bewiesen, Job 189916: pmc_available=1, cache_misses_l1=4190096). Ohne das Flag schrieb der
// offizielle Mess-Lauf pmc_available=0/honest-0 trotz fertiger Infra.
// (WORTLAUT-SINGLE-SOURCE: identisch zu super .gitlab-ci.yml Kommentar der Jobs measure:smoke / measure:golden-320
// -- die Begruendung soll nicht in zwei Fassungen driften.)
//
// WARUM EINE FUNKTION UND KEIN LITERAL JE STELLE: die Pflicht ging am 16.07. verloren, weil sie an ZWEI
// JOB-NAMEN hing (measure:smoke + measure:golden-320) statt an der Eigenschaft "dieser Job baut den
// Mess-Treiber". Als die Arbeit in den Planer wanderte, wanderte die Pflicht nicht mit. Die Invariante,
// die diese Funktion traegt und die der Test JedeTreiberKonfigurationTraegtDasPmcFlag pinnt, lautet:
//
//     Zu JEDER emittierten `cmake -B build`-Zeile, deren FOLGEZEILE den comdare-messung-driver baut,
//     gehoert -DCOMDARE_ENABLE_PMC=ON.
//
// Sie ist nicht an die Zahl der Emissionsstellen gebunden (heute vier: ceb:build + ceb:emit in Stufe 1,
// tier-build-batch + measure-batch in Stufe 2) und faengt eine kuenftige fuenfte automatisch. Umgekehrt
// bleibt eine Emission OHNE Treiber-Bau bewusst flaglos -- dieser Fall ist im Haus vorgesehen (super
// .gitlab-ci.yml: Auswertungswerkzeuge ohne Messung bleiben bewusst ohne COMDARE_ENABLE_PMC).
//
// TIER-NEUTRALITAET: add_compile_definitions(COMDARE_ENABLE_PMC) (ce CMakeLists.txt:77) wirkt auf den
// CMAKE-Teilbaum, also auf den Treiber. Die Tier-.so entsteht ueber einen eigenen g++-Subprozess des
// Treibers (build_orchestrator.hpp) und traegt das Makro NICHT -- Klasse CEB-ONLY, 0 Tier-Binaries.
//
// STELLUNG IM KOMMANDO: direkt hinter -DCOMDARE_V32_ENABLE=ON und VOR -DCMAKE_BUILD_TYPE -- exakt die
// Reihenfolge der beiden super-Jobs, und sie laesst die Nachbarschaft (BUILD_TYPE + Combo-Define)
// unangetastet, an der die W2-Pins haengen.
//
// ================================================================================================
// I-PMC-2 (Owner 10.08.2026) -- DIE PFLICHT WIRD ERKANNT, NICHT BEHAUPTET.
// ================================================================================================
// OWNER-WORTLAUT (verbatim): "Dabei erkennt jeder Planer auf jeder Maschine fuer sich, ob PMC existiert und
// ob daher das PMC in der CEB verbaut wird. Es entsteht hier WIEDER EINE PERMUTATION [...] JEDE
// HARDWAREFORM EINER PMC AMD/INTEL IST ZU UNTERSCHEIDEN."
//
// WAS SICH GEGENUEBER I-PMC-1 AENDERT, UND WAS NICHT. Die Funktion nahm KEIN Argument und lieferte
// unbedingt " -DCOMDARE_ENABLE_PMC=ON". Der Planer erkannte damit nichts: auf einem Host ohne benutzbare
// PMU emittierte er byte-gleich dasselbe wie auf prod1, und die dort gebaute CEB trug den vollen
// Messfuehler-Overhead bei pmc_available=0. Genau die Ununterscheidbarkeit, die der Owner-Satz
// ausschliesst.
//
// RANGFOLGE-MERGE (OWNER > PLAN), nicht Verdraengung: F9 ("PFLICHT fuer Vollstaendigkeit aller
// perf-Messwerte", 16.07.) gilt UNVERAENDERT WEITER -- fuer jeden Host, auf dem PMC benutzbar ist. Der
// Owner-Satz vom 10.08. fuegt hinzu, was F9 nicht regeln konnte: auf einem Host OHNE benutzbare PMU ist
// "ohne PMC" die EHRLICHE Permutation und kein Pflichtverstoss. Das unbedingte ON war die einzige Lesart,
// solange es keine Erkennung gab; jetzt gibt es sie.
//
// DIE PERMUTATION, DIE HIER ENTSTEHT: {ohne, amd, intel} -- der CEB-Raum wird dreifach. Der
// TIER-Fingerprint bleibt unberuehrt (Owner 10.08.: "Eine aus-/eingebaute Messeinrichtung erzeugt eine
// andere CEB, NICHT ein anderes Tier-Binary. Wer beides gleichsetzt, kommt auf einen ABI-Bump, der nicht
// anfaellt."). Mechanisch gedeckt: add_compile_definitions wirkt auf den CMAKE-Teilbaum, also auf den
// Treiber; die Tier-.so entsteht ueber einen eigenen g++-Subprozess des Treibers (build_orchestrator.hpp)
// und traegt weder COMDARE_ENABLE_PMC noch ein Vendor-Makro.
//
// ERWEITERUNG, KEIN NEUBAU (Owner 10.08., verbatim): "Eine Erweiterung der ISA und System, Meta-Meta-Achsen
// zur Hardware-Erweiterung ist STETS ZULAESSIG [...] werden einfach nur die NEU MOEGLICHEN FEHLENDEN
// PERMUTATIONEN ZUSAETZLICH gebaut." Der Bestand bleibt gueltig; es faellt kein Neubau an.
//
// FAIL-CLOSED: ein Befund, der nicht gefahren wurde (Default-konstruiert), ist "unbrauchbar" und emittiert
// KEIN Flag. Lieber eine ehrliche Ohne-PMC-Permutation als ein behauptetes ON -- die umgekehrte Wahl war
// exakt der Defekt.
[[nodiscard]] inline std::string ceb_pmc_compile_define(PmcHostBefund const& befund) {
    if (befund.lage == PmcLage::Unbrauchbar) return {};
    std::string s{" -DCOMDARE_ENABLE_PMC=ON -DCOMDARE_PMC_VENDOR="};
    s += befund.vendor_id();
    return s;
}

/// Die BEFUND-Zeile, die JEDER Treiber-Bau-Emission vorangeht -- auch der flaglosen (V-1: der Nenner
/// gehoert in die Ausgabe). Sie ist ein YAML-Kommentar und damit fuer die Kind-Pipeline folgenlos; ihr
/// Zweck ist, dass der Leser eines emittierten Jobs unterscheiden kann, ob der Planer GEMESSEN oder
/// GERATEN hat. Eine Emission, die zu PMC schweigt, kann das nicht.
/// `einzug` traegt die YAML-Einrueckung der Nachbarzeile, damit der Kommentar im Block steht.
[[nodiscard]] inline std::string pmc_befund_zeile(PmcHostBefund const& befund, char const* einzug = "    ") {
    return std::string{einzug} + "# " + befund.nenner_zeile() + "\n";
}

// ================================================================================================
// CI-DUAL (Owner 14.08.2026) -- COMPILER-PINS + clang-ZWILLING DER TRAEGERSTUFEN.
// ================================================================================================
// OWNER-WORTLAUT (verbatim): "... dass in ALLEN pipelines aller C++ Projekte, die binaries nicht wie
// geplant mit gcc und clang dual in compile Release und Debug gebaut werden, um ganz sicher mit 2
// verschiedenen compilern die Kompatibilitaet zu beweisen ... unter anderem in jeder Traegerstufe
// der Cache Engine."
//
// (E1/F3-Default, Design 14.08.) Die Emission pinnt die GEPLANTEN Versionen HART (KON55-01
// "Versionen sind jeweils geplant": gcc 15.3 / clang 22.1.8). Vorher baute JEDER Grandchild-Job den
// Host-Default -- ungepinnt und damit je Runner verschieden; der Pin macht die Belegung EHRLICH und
// achsen-vorbereitend. Ein fehlender Compiler faellt LAUT (cmake bricht den Configure ab) statt
// still auf einen anderen zu wechseln. KEINE Stempel-/Preimage-Beruehrung: der Compiler wird erst
// mit S-9/S-11 Achsen-Wert (N1). Die Repo-CIs behalten ihren Neuester-Loop (F3: nur die EMISSION
// pinnt hart, bis der Compiler Achsen-Wert wird).
//
// INVARIANTE (Testform CompilerPinInvariante.*, dieselbe Klasse wie I-PMC-1): zu JEDER emittierten
// `cmake -B build`-Zeile, deren Folgezeile den comdare-messung-driver baut, gehoert dieser Pin.
// STELLUNG IM KOMMANDO: ANGEFUEGT, hinter dem Combo-Define -- die gepinnte W2-Nachbarschaft
// (V32/PMC/BUILD_TYPE/Combo) bleibt byte-unveraendert.
// CACHE-STICKY-LAGE (am Objekt verifiziert, cmake 4.3.4, 14.08.2026): ein bestehendes, mit
// Host-Default konfiguriertes build/-Verzeichnis akzeptiert den Pin-Wechsel (rc=0, Re-Detect +
// einmaliger Neubau) -- Cold-Kosten der deklarierten Klasse, kein harter Configure-Fehler.
[[nodiscard]] inline std::string ceb_gcc_pin_define() {
    return " -DCMAKE_C_COMPILER=gcc-15 -DCMAKE_CXX_COMPILER=g++-15";
}
[[nodiscard]] inline std::string ceb_clang_pin_define() {
    return " -DCMAKE_C_COMPILER=clang-22 -DCMAKE_CXX_COMPILER=clang++-22";
}

// (E2, Design 14.08.) Der clang-22-ZWILLING der BAU-Stufen (Stufe 1 ceb:build, Stufe 2
// tier-build-batch): ZWEITE Sequenz IM SELBEN Job -- sequentiell, also EIN Bau-Slot (T-11b), kein
// paralleler Vollbau auf dem Runner. Er traegt DIESELBEN Defines (PMC/Combo/BUILD_TYPE) wie die
// gcc-Haelfte -- ein Zwilling mit anderen Defines pruefte eine ANDERE CEB -- und baut in ein
// EIGENES Verzeichnis build-clang (kein Reconfigure-Pendeln am geteilten Code/build; `find build`
// der Bestands-Gates trifft build-clang nicht, verschiedene Wurzeln).
// TEST-GATE DES STUFEN-BINARIES: die UNBEDINGT registrierten Treiber-Tests
// (Code/02_messung_driver/CMakeLists.txt :204-:267 -- test_messreihen_workload,
// test_messreihe_v32_parser, test_experiment_phase_strategy, test_lane_vendor_guard; das
// Verzeichnis-Aggregat 02_messung_driver/all baut sie OHNE Handliste, R4-Klasse) via
// `ctest --test-dir build-clang/02_messung_driver`. --no-tests=error macht den Leerlauf zum
// Fehler (rc=8; dieselbe W0b-3-Haertung wie der PMC-Preflight) -- ein leeres ctest waere ein
// Schein-Gate. Der Zwilling provisioniert nichts und misst NIE (N2; RELEASE-/Lager-Glieder nehmen
// weiter NUR das gcc-Artefakt, Auflage 5b). Mess-Batches erhalten KEINEN Zwilling.
inline void emit_ci_dual_clang_twin(std::string& s, PmcHostBefund const& pmc_befund, std::string const& combo_legend,
                                    std::string const& build_type, std::string const& testat_felder) {
    s += "    # CI-DUAL (Owner 14.08.): clang-22-Zwilling -- Bau+Test-Gate der Stufe, sequentiell im selben\n";
    s += "    # Slot (T-11b); gleiche Defines, eigenes build-clang; misst NIE (N2).\n";
    s += "    - cmake -B build-clang -G Ninja -DCOMDARE_V32_ENABLE=ON" + ceb_pmc_compile_define(pmc_befund) +
         " -DCMAKE_BUILD_TYPE=" + build_type + ceb_combo_compile_define(combo_legend) + ceb_clang_pin_define() + "\n";
    s += "    - cmake --build build-clang --target comdare-messung-driver\n";
    s += "    - |\n";
    s += "      set -euo pipefail\n";
    s += "      DRIVER_CLANG=$(find build-clang -type f -name \"comdare-messung-driver\" | head -1)\n";
    s += "      test -n \"$DRIVER_CLANG\" -a -x \"$DRIVER_CLANG\" || { echo \"comdare-messung-driver "
         "(clang-Zwilling) fehlt\"; exit 1; }\n";
    s += "      cmake --build build-clang --target 02_messung_driver/all\n";
    s += "      ctest --test-dir build-clang/02_messung_driver --no-tests=error --output-on-failure\n";
    s += "      echo \"[DUAL-TESTAT] ts=$(date -u +%FT%TZ) compiler=clang-22 build_type=" + build_type +
         " status=gebaut+getestet" + (testat_felder.empty() ? std::string{} : " " + testat_felder) + "\"\n";
}

// ── PlanTextBuilder — ConcreteBuilder + der --dump-plan-Traeger. Deterministische Zeilen-Textform. ──────────
//    Format (stabil, byte-reproduzierbar; keine host-abhaengigen Felder AUSSER der einen deklarierten
//    Owner-Ausnahme `pmc_befund=` -- Owner 10.08.2026: "erkennt jeder Planer auf jeder Maschine fuer sich".
//    Diese eine Zeile MUSS sich zwischen prod1 und prod2 unterscheiden duerfen; waere sie es nicht,
//    gaebe es die Erkennung nicht. Alle uebrigen Felder bleiben host-unabhaengig.):
//      # comdare-experiment-plan v1.1
//      source_kind=<thesis|experiment>
//      profile_id=<id>
//      profile axes=<n> values=<m>          (v1.1: profil-seitige Substanz, additiv)
//      registry_trio loaded=<0|1> organ=<engine> organ_axes=<n> organ_offers=<n> system=... measurement=...
//      perm_count=<n>
//      perm <i> opt=<id> simd=<id> opt_flag=<f> march_flag=<f> build_version_suffix=<s>
//        step <j> kind=<k> label=<l> merge=<m> binary_id=<b> series=<s> pruefling_type=<p> lebewesen=<le>
//             built_stem=<st>          (OP-9, additiv unter v1.1: der real gebaute Datei-Stem; "-" = keine id
//                                       bzw. Stem ohne View-Index nicht bestimmbar, s. built_stem_field)
class PlanTextBuilder final : public IPlanBuilder {
public:
    void begin_plan(PlanHeader const& h) override {
        out_ += "# comdare-experiment-plan v1.1\n";
        out_ += "source_kind=" + h.source_kind + "\n";
        out_ += "profile_id=" + h.profile_id + "\n";
        // v1.1 (2026-07-27): die profil-seitige Substanz. Steht bewusst VOR registry_trio -- erst was das
        // Profil deklariert, dann was das Angebot dagegen haelt.
        out_ += "profile axes=" + std::to_string(h.profile_axis_count) +
                " values=" + std::to_string(h.profile_value_count) + "\n";
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
        // I-PMC-2 (Owner 10.08.2026): der GEMESSENE PMC-Befund dieses Hosts. Er ist damit die EINZIGE
        // host-abhaengige Zeile des --dump-plan -- und das ist die deklarierte Owner-Ausnahme zur
        // Format-Zusage im Kopf: der Plan MUSS hier host-abhaengig sein, denn genau das ist die Sache
        // ("erkennt jeder Planer auf jeder Maschine fuer sich"). Ein Plan, der auf prod1 und prod2
        // byte-gleich waere, haette die Erkennung nicht.
        out_ += "pmc_befund=" + std::string{h.pmc_befund.lage_label()} +
                " events=" + std::to_string(h.pmc_befund.events_gebissen) + "/" +
                std::to_string(h.pmc_befund.events_geprueft) +
                " erhoben=" + std::string(h.pmc_befund.probe_gefahren ? "1" : "0") + "\n";
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
                " pruefling_type=" + nz(s.pruefling_type) + " lebewesen=" + nz(s.lebewesen) +
                " built_stem=" + built_stem_field(s.binary_id) + "\n";
    }
    void end_perm(PlanPerm const&) override {}
    void end_plan(PlanHeader const&) override {}

    [[nodiscard]] std::string const& text() const noexcept { return out_; }

private:
    // Leerer String -> "-" (stabile, eindeutige Feldtrennung; kein Feld bleibt leer im Zeilen-Text).
    [[nodiscard]] static std::string nz(std::string const& s) { return s.empty() ? std::string{"-"} : s; }

    /// OP-9: der DATEI-STEM, den der Orchestrator fuer diesen Schritt real erzeugt -- damit der
    /// Auswerter ihn LIEST statt ihn nachzubauen (super/tier_binary_report.hpp:103 fuehrt heute eine
    /// Zweit-Implementierung der Sanitisierung; die faellt damit weg).
    ///
    /// EIN KANAL: der Wert kommt aus orch_make_stem, derselben Funktion, die build_orchestrator.hpp:420
    /// beim echten Bau aufruft -- keine nachgebaute Regel.
    ///
    /// WARUM DER INDEX HIER FEHLEN DARF, und wann NICHT: orch_make_stem kappt nur IDs, deren sanitisierte
    /// Form kStemMax (120) ueberschreitet, und haengt dann '_<perm-index>_<fnv1a-hex>' an. Unterhalb der
    /// Grenze ist das Ergebnis allein aus der binary_id bestimmt -- der Index ist dort nachweislich
    /// irrelevant. Genau dieser Fall ist im Plan der einzige reale: der THESIS-Kanal fuehrt gar keine ids
    /// ("-"), der EXPERIMENT-Kanal fuehrt "sota_tier=<name>" (gemessen: max. 55 Zeichen). Die ~520 Zeichen
    /// langen 17-Achsen-ids entstehen erst in der Bau-View, nicht im Plan.
    /// Sollte dennoch je eine lange id hier ankommen, wird KEIN Stem behauptet: der Director kennt den
    /// View-Index der Binary an dieser Stelle nicht (PlanStep::index ist der Schritt-Index INNERHALB der
    /// Perm, BinarySpec::index dagegen der View-Index), und ein mit falschem Index gebildeter Hash-Suffix
    /// waere schlimmer als kein Wert -- er zeigte auf eine Datei, die es nicht gibt. Dann steht "-".
    [[nodiscard]] static std::string built_stem_field(std::string const& binary_id) {
        if (binary_id.empty() || binary_id == "-") return "-";
        if (::comdare::cache_engine::builder::experiment::orch_sanitize(binary_id).size() >
            ::comdare::cache_engine::builder::experiment::kStemMax)
            return "-"; // Index unbekannt -> nichts behaupten
        return ::comdare::cache_engine::builder::experiment::orch_make_stem(binary_id,
                                                                            0); // Kurz-Zweig: index-unabhaengig
    }
    std::string out_;
};

// -- PlanSizeBuilder -- ConcreteBuilder, der NICHTS emittiert, sondern nur die Groesse des Plans zaehlt. ----
// V-2/2a (Bauplan TEIL V, M3-Ersatz-Gate, 2026-07-27): das Treiber-Startgate fragte bis hierher ein
// CONFIGURE-ZEIT-Artefakt (generated/permutations_manifest.txt) -- also die Perm-DLL-Menge des Alt-Kanals,
// NICHT die Frage, ob der bevorstehende E4-XML-Lauf ueberhaupt etwas zu tun hat. Dieser Builder beantwortet
// die richtige Frage, und zwar am SELBEN deterministischen Director-Walk, den der Lauf ohnehin nimmt
// (construct_plan_into): ein Kanal, eine Wahrheit (§73.1). Der Kopf-Kommentar des IPlanBuilder sieht
// "struktur-zaehlende Builder" ausdruecklich vor -- das hier ist einer.
//
// Bewusst OHNE Text/Allokation: der Gate-Pfad laeuft vor JEDEM Messlauf und darf nichts kosten.
class PlanSizeBuilder final : public IPlanBuilder {
public:
    void begin_plan(PlanHeader const& h) override {
        axes_   = h.profile_axis_count; // Plan-Kopf v1.1
        values_ = h.profile_value_count;
    }
    void begin_perm(PlanPerm const&) override { ++perms_; }
    void on_step(PlanStep const&) override { ++steps_; }
    void end_perm(PlanPerm const&) override {}
    void end_plan(PlanHeader const&) override {}

    [[nodiscard]] std::size_t perm_count() const noexcept { return perms_; }
    [[nodiscard]] std::size_t step_count() const noexcept { return steps_; }
    [[nodiscard]] std::size_t profile_axis_count() const noexcept { return axes_; }
    [[nodiscard]] std::size_t profile_value_count() const noexcept { return values_; }

    /// LEER = es gibt nichts zu tun. DREI Zahlen, weil sie drei verschiedene Leer-Arten treffen:
    ///   perms_==0   -- keine Rekombination geplant (Experiment-Wurzel ohne Kombinationen);
    ///   steps_==0   -- Perms ohne Arbeit (Experiment-Wurzel ohne Phasen);
    ///   axes_==0    -- das PROFIL deklariert gar keine Achse. Diese dritte Art war der Grund fuer v1.1:
    ///                  ein achsenloses THESIS-Profil hat immer >=1 Perm (Default-System-Perm) und >=1
    ///                  Schritt (Basis-Pass) -- es sah am Plan wie ein gueltiger Lauf aus und fiel erst
    ///                  tief unten an der Achse-2-Wache um (Exit 4). Jetzt faellt es hier, benannt.
    /// Der Aufrufer meldet ALLE Zahlen, damit die Arten unterscheidbar bleiben statt in einem pauschalen
    /// "leer" zu verschwinden.
    [[nodiscard]] bool empty() const noexcept { return perms_ == 0 || steps_ == 0 || axes_ == 0; }

private:
    std::size_t perms_  = 0;
    std::size_t steps_  = 0;
    std::size_t axes_   = 0;
    std::size_t values_ = 0;
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
        out_ += "# EIN CEB-Emit-Target ('tier cmake' => Stufe-2-Plan). Dreistufiger Bare-Metal-Bau:\n";
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
    // STUFE 1 haengt an der Mess-Achsen-Ebene: je Mess-Kombination CEB-Bau + CEB-Emit("tier cmake").
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
        // W2 (bare-metal-Gegenpart): hier ist ceb:build nur Echo/Stamp -- den Treiber baut der AEUSSERE Configure.
        // Deshalb kann dieser Weg den Define nicht selbst setzen; er SAGT dem Bediener, welcher Zusatz an den
        // aeusseren Configure gehoert. [all] => kein Hinweis => Emission byte-identisch (Topologie unberuehrt).
        // F-B1: die Bedingung fragt jetzt das PRAEDIKAT statt "Rueckgabe leer?" -- ceb_combo_compile_define
        // liefert fuer [all] die -U-Loeschung und waere damit nie mehr leer. Diese Stelle bleibt bewusst
        // [all]-STUMM: sie emittiert keinen Configure-Aufruf, sondern nur einen Hinweis an den Bediener, und der
        // sticky-Fall des AEUSSEREN Configure faellt hier in die F-B2-Wache an der Stempel-Naht (fail-loud).
        if (!ceb_combo_is_full_set(c.legend))
            out_ += "        COMMAND \"${CMAKE_COMMAND}\" -E echo \"ceb:build " + c.legend +
                    " -- aeusserer Configure braucht -DCOMDARE_MEASUREMENT_COMBO=" + c.legend +
                    " (W2: CT-Einbau der Mess-Combo)\"\n";
        // CI-DUAL (Owner 14.08., E4): dasselbe W2-Muster -- dieser Weg baut den Treiber nicht selbst,
        // er SAGT dem Bediener die Pflicht des aeusseren Configure. CI und bare-metal fahren dieselben
        // Zellen (V-5, beide literal); der Zwilling ist dort Bau+Test-Gate, nie Mess-Glied (N2).
        out_ += "        COMMAND \"${CMAKE_COMMAND}\" -E echo \"ceb:build " + c.legend +
                " -- CI-DUAL (Owner 14.08.): aeusserer Configure pinnt -DCMAKE_C_COMPILER=gcc-15"
                " -DCMAKE_CXX_COMPILER=g++-15; clang-22-Zwilling (build-clang) = Bau+Test-Gate der Stufe\"\n";
        out_ += "        COMMAND \"${CMAKE_COMMAND}\" -E touch \"" + bstamp + "\"\n";
        out_ += "        COMMENT \"ceb:build " + c.legend + "\"\n";
        out_ += "        VERBATIM)\n";
        out_ += "    add_custom_target(" + ceb_build_target(slug) + " DEPENDS \"" + bstamp + "\")\n";
        out_ += "endif()\n";
        // CEB-Emit-Target (STUFE 1->2): die GEBAUTE CEB emittiert ihre STUFE-2-Sicht ("tier cmake"). CEB-Hoheit
        // (§40.b). DEPENDS auf den CEB-Bau-Stamp (Bau->Emit-Kante). Host-unabhaengig (Treiber/Profil = Variablen).
        // P8-REST-ZWILLING (27.07.): Subkommando-Form wie auf der CI-Seite. In CMake stehen "tier" und "cmake"
        // unquoted nebeneinander und werden damit als ZWEI Argumente uebergeben -- genau das, was der Dispatcher
        // erwartet (argv[1]/argv[2]). VERBATIM escaped sie einzeln, klebt sie also nicht zusammen.
        out_ += "if(NOT TARGET " + ceb_emit_target(slug) + ")\n";
        out_ += "    add_custom_command(\n";
        out_ += "        OUTPUT \"" + tierpl + "\"\n";
        out_ += "        COMMAND \"${CMAKE_COMMAND}\" -E make_directory \"${COMDARE_PLAN_TIER_OUT}\"\n";
        out_ +=
            "        COMMAND \"${COMDARE_PLAN_DRIVER}\" tier cmake \"${COMDARE_PLAN_PROFILE}\" > \"" + tierpl + "\"\n";
        out_ += "        DEPENDS \"" + bstamp + "\" # ceb:build->ceb:emit-Kante\n";
        out_ += "        COMMENT \"ceb:emit " + c.legend + " ('tier cmake' => Stufe-2-Plan)\"\n";
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

// §62-B Lane-Bau-Parallelitaet Single-Source (S4, 2026-07-23): die harte parallele Compile-Zahl
// (COMDARE_BUILD_PARALLEL) je Host-Lane des Build+Mess-Batches. COMDARE_BUILD_PARALLEL ist der Bau-Pool-WORKER-Override
// ("harte parallele Compile-Zahl", profile_run_entry.hpp:132-135). User-KERN (Drosselung 2026-07-23, mit User-GO): amd
// von 32 auf 24 GEDROSSELT -- Slice-1-Empirie: 32 Worker (19,95 min) ~ 24 Worker (19,4 min), aber 32 verursacht ~20G
// Swap-Thrashing (RAM-Bound). intel bleibt 24. => BEIDE Lanes 24 (nicht mehr nproc-Max amd; die fruehere 32/24-T-Wert-
// Lesart ist RAM-korrigiert). Der Host-Parameter/die Struktur bleiben (per-Host tunebar, falls die Lanes wieder
// divergieren). Voll ausschoepfbar, weil P4 (geteilte resource_group ceb-measure-<host>) je Maschine nur EINEN Batch
// gleichzeitig laufen laesst; das MESSEN selbst bleibt 1-Thread (run_profile-Loop). GETEILTE Single-Source fuer beide
// Stufen (TierCiYamlBuilder Build-/Mess-Batch + TierCmakeGraphBuilder bare-metal-Batch).
[[nodiscard]] inline std::size_t lane_build_parallelism(std::string const& host) { return host == "amd" ? 24 : 24; }

// E-20/R-15 (21.08.2026): die MESS-PIN-SOLL-MENGE je Host-Lane -- Deklaration der L3-homogenen CPU-Menge, auf
// der der 1-Thread-Messlauf laufen SOLL. prod1/amd (Zen5 9950X3D) traegt ZWEI ungleiche CCDs: CCD0 = 96 MiB L3
// (cpu 0-7,16-23, X3D-V-Cache), CCD1 = 32 MiB (cpu 8-15,24-31) -- /sys/devices/system/cpu/cpu*/cache/index3
// am Objekt gemessen (21.08.2026; lscpu "L3: 128 MiB (2 instances)" ist die SUMME 96+32, nicht 2x128). Ein
// ungepinnter Messlauf wandert zwischen den CCDs, der wirksame L3 springt 96<->32 MiB mitten in der Serie:
// Cache-Messwerte sind dann NICHT reproduzierbar (R-15). Deklariert wird die GROSSE homogene Menge (CCD0
// komplett, SMT-Geschwister inklusive) -- die Kern-Wahl INNERHALB des CCDs ist Aktuator-Politik
// (ScopedThreadPin, builder/measurement/thread_pinning.hpp, AP-13/#247), kein Emissions-Vertrag.
// intel/prod2 (RaptorLake): hybride P-/E-Kern-Topologie, am Objekt NICHT vermessen -- eine erfundene Maske
// waere ein richtiges Messgeraet am falschen Gegenstand; die Lane bleibt LEER (= UNVERMESSEN, wird in der
// Emission laut deklariert), bis prod2 vermessen ist. KEIN job-weites taskset: dieselbe Treiber-Invokation
// BAUT parallel (COMDARE_BUILD_PARALLEL) und MISST 1-Thread -- ein taskset ueber den ganzen Job droesselte
// den Bau auf die Pin-Menge (24 Worker auf 8 Kernen).
[[nodiscard]] inline std::string lane_measure_pin_cpus(std::string const& host) {
    return host == "amd" ? "0-7,16-23" : "";
}

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

// ===========================================================================
// LAG-P4 -- DIE KORN-WACHE: das 4096er-Korn ist ab hier COMPILE-HART gebunden.
//
// SELBSTCHECK: diese drei static_assert sichern zu, dass die DREI Traeger des Batch-Korns denselben
// Wert tragen UND dass dieser Wert die Owner-Zahl 4096 ist. Sie sichern NICHT zu, dass das Korn an
// jeder Verwendungsstelle auch wirklich BENUTZT wird (wer eine vierte Konstante neu erfindet, faellt
// hier nicht auf), und sie sagen NICHTS ueber die Laufzeit-Groesse eines konkreten Fensters (ein Rest-
// Fenster am Ende einer Selektion ist zulaessig kleiner).
//
// WARUM COMPILE-HART UND NICHT ALS KOMMENTAR. Bis zu diesem Paket hielt die Bindung allein ein
// Kommentar an jeder der drei Stellen. Das ist am Objekt widerlegt: der Spiegel-Kommentar in
// batch_planner.hpp verwies auf "experiment_plan_director.hpp:439" -- die Konstante stand real bei
// :662. Der Verweis war also selbst schon abgedriftet, waehrend er die Bindung behauptete. Ein
// Kommentar kann eine Invariante beschreiben; halten kann er sie nicht.
//
// WAS AUSEINANDERLAUFEN WUERDE, wenn die drei divergieren -- je Zusicherung eine eigene Folge, siehe
// die Fehlertexte. Der Kern: der Planer EMITTIERT die Scheiben-Grenzen, das Bestandslog RESERVIERT
// und DEDUPLIZIERT genau diese Scheiben, und der planer-getriebene Bau STEMPELT das Korn in den
// Batch-Plan, den der spaetere Mess-Lauf ueber denselben Stempel wiederfindet. Drei Rollen, EIN Korn.
// ===========================================================================
static_assert(kGnBatchSlice == 4096,
              "LAG-P4/Korn-Wache: planner::kGnBatchSlice (experiment_plan_director.hpp) ist nicht 4096. Das "
              "Batch-Korn ist eine OWNER-FESTLEGUNG vom 22.07.2026 ('Batch stets 4096, mit Zeitstempel "
              "reserviert'; F6 vom 01.08.2026: 4096er-Batches sind JOB-MEILENSTEINE genau EINER Maschine, nie "
              "geteilt, nie unterbrochen). Wer diese Zahl aendert, aendert die Reservierungs-Groesse des "
              "Bestandslogs, den Wiederaufnahme-Takt nach einem Abbruch und die Gleichverteilung zwischen den "
              "Maschinen. Das ist kein Zahlendreher, den man hier still korrigiert, sondern ein Owner-Entscheid.");
static_assert(::comdare::cache_engine::builder::bestandslog::kGnBatchSlice == kGnBatchSlice,
              "LAG-P4/Korn-Wache: bestandslog::kGnBatchSlice (batch_planner.hpp) und planner::kGnBatchSlice "
              "(experiment_plan_director.hpp) laufen auseinander. Beide MUESSEN dasselbe Korn tragen: der Planer "
              "emittiert die Scheiben-Grenzen, und das Bestandslog reserviert und dedupliziert GENAU diese "
              "Scheiben. Bei ungleichem Korn beansprucht eine Maschine ein Fenster, das die andere nie als "
              "Fenster sieht -- aus der Gleichverteilung wird Doppelarbeit, und der Takeover bei ETA+50% gibt "
              "eine Menge frei, die der uebernehmende Lauf gar nicht deckt.");
static_assert(::comdare::cache_engine::builder::bestandslog::kBuildSliceGrain == kGnBatchSlice,
              "LAG-P4/Korn-Wache: bestandslog::kBuildSliceGrain (planer_driven_build.hpp) und "
              "planner::kGnBatchSlice (experiment_plan_director.hpp) laufen auseinander. Der planer-getriebene "
              "Bau slict die selektierten Indizes mit diesem Korn und stempelt es in den Batch-Plan ('|korn='); "
              "der SPAETERE Mess-Lauf sucht seinen Plan ueber genau diesen Stempel. Bei ungleichem Korn findet "
              "der Mess-Lauf den Plan seines eigenen Bau-Laufs NICHT mehr und schreibt die Mess-Front nicht "
              "fort -- fail-closed, aber still falsch: der Plan IST da, er wird nur nicht wiedererkannt.");

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

// G4a P-A (2026-07-26): Storage-Scharfschaltung + Retry-Deckel, WORTGLEICH in ALLEN Batch-Emissionen (eine
// Literal-Quelle -> die Tests zaehlen Vorkommen == Zahl der Emissionen). Steht INNERHALB des `- |`-Blocks, nicht als
// einfache `- `-Zeile: eine Plain-Scalar-Zeile mit " #" wuerde YAML-seitig als Kommentar abgeschnitten, hier sind
// es echte Shell-Kommentare. Laeuft nach `cd Code`, der Pfad ist workdir-relativ (KLASSE: kein $CI_PROJECT_DIR).
//
// G4b-2/2.4-(7) (2026-07-26): von einem statischen TierCiYamlBuilder-Member zu einer FREIEN Funktion auf
// Namespace-Ebene gehoben -- unveraendert im Rumpf. Grund: der dritte Aufrufer ist emit_ceb_emit_job, und der liegt
// in CiYamlBuilder (Zeile 552 ff.), also in einer ANDEREN Klasse und VOR TierCiYamlBuilder. Ein Klassen-Member waere
// von dort nicht erreichbar gewesen, und die Alternative -- die Zeilen ein zweites Mal hinzuschreiben -- haette
// genau die eine Literal-Quelle zerstoert, auf der die Vorkommens-Zaehlung der Tests beruht. Der Rumpf greift auf
// keinen Klassen-Zustand zu (nur `s +=`), die Verschiebung ist daher rein mechanisch.
inline void emit_storage_activation(std::string& s) {
    s += "      # G4a P-A: MinIO-Rohcreds maschinenlokal in MC_HOST_prodcache falten. Das Skript ist "
         "source-sicher\n";
    s += "      # (kein exit, kein set -e) und bei COMDARE_STORAGE_CACHE != true vollstaendig inert -- das\n";
    s += "      # unbedingte Sourcen ist damit unschaedlich und haelt den Nicht-Storage-Lauf unveraendert.\n";
    s += "      . external/comdare-cache-engine/scripts/comdare_storage_activation.sh\n";
    s += "      # Blackhole-Schutz (P-A): der ArtifactCache-Default ist 12 Versuche (artifact_cache.hpp:923); mit\n";
    s += "      # dem 120s-Timeout je Versuch grindet ein unerreichbarer Store pathologisch lange je Objekt.\n";
    s += "      # Deckel NUR wenn Storage ueberhaupt scharf ist; ein bereits gesetzter Wert gewinnt. Die\n";
    s += "      # :-Expansion ist Pflicht -- der umgebende Block laeuft unter `set -u`.\n";
    s += "      if [ \"${COMDARE_STORAGE_CACHE:-}\" = \"true\" ]; then\n";
    s += "        export COMDARE_ARTEFAKT_TRIES=\"${COMDARE_ARTEFAKT_TRIES:-2}\"\n";
    s += "      fi\n";
}

// ── CiYamlBuilder — STUFE-1-Emitter (Planer-Rolle, PAKET W10-A / §42, ersetzt die W7-A-Zweistufigkeit). ───────
//    Emittiert eine deterministische GitLab-Child-Pipeline-YAML: die MESS-ACHSEN-Stufe der CE-gesteuerten Kette.
//    Je Mess-Achsen-Kombination [a,b,c] (aus der Anwender-XML, <measurement_categories>) EINE dynamische
//    CEB-Pipeline:
//      "ceb:build:[a,b,c]"   -> baut die CEB fuer diesen CEB-Typ (Messsystem).
//      "ceb:emit:[a,b,c]"    -> die GEBAUTE CEB emittiert SELBST Child-2 ("tier ci", §40.b-Praezisierung:
//                               Planer steuert CEB-Jobs, CEB steuert Tier-Jobs) aus ihren einkompilierten
//                               System-Achsen-Freigaben. Heute EINE Binary in zwei Rollen (Planer-Rolle "plan ci"
//                               vs CEB-Rolle "tier ci"; ehrlich dokumentiert, kein Schein-Split).
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
        out_ += "#           (ceb:build -> ceb:emit('tier ci') -> ceb:trigger). Der PLANER steuert die CEB-Jobs.\n";
        out_ += "#   STUFE 2 (CEB-emittiert, 'tier ci', S4-§62-B-Batch): je Host-Lane EIN Build+Pruef-Batch\n";
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
        out_ += emit_ceb_build_job(c, header_.profile_basename, header_.pmc_befund);
        // A5 (§56-T2-FANOUT D4): der Selektor-Naht ist NUR bei N>1 CEB-Konfigs aktiv (measurement_combo_count > 1);
        // count==1 (heutige Live-Strecke) => KEIN --measurement-combo => byte-identisch zu vor A5.
        out_ += emit_ceb_emit_job(c, header_.measurement_combo_count > 1, header_.profile_basename, header_.pmc_befund);
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
    // I-PMC-2: der Host-Befund kommt als PARAMETER herein, nicht aus einem statischen Zugriff -- diese
    // Emitter sind static, und ein globaler/statischer Befund waere genau der versteckte Zustand, der
    // die vier Emissionsstellen wieder auseinanderlaufen liesse.
    [[nodiscard]] static std::string emit_ceb_build_job(PlanMeasurementCombo const& c,
                                                        std::string const&          profile_basename,
                                                        PmcHostBefund const&        pmc_befund) {
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
        // W2: die CEB wird MIT der einkompilierten Mess-Combo konfiguriert. F-B1 (05.08.2026): [all] emittiert
        // nicht mehr NICHTS, sondern die EXPLIZITE Loeschung -UCOMDARE_MEASUREMENT_COMBO (sticky Cache-Var,
        // s. F-B1-Block ueber ceb_combo_is_full_set); die Alt-Aussage "kein Zusatz => byte-identisch" galt
        // NUR VOR F-B1.
        // M-2/B1: PMC-Pflicht (F9). Diese Zeile konfiguriert den Bau des comdare-messung-driver in der
        // Folgezeile -- damit greift die Invariante aus ceb_pmc_compile_define().
        // I-PMC-2 (10.08.2026): der Befund kommt aus dem Plan-KOPF (eine Erhebung je Lauf) und steht als
        // Kommentar VOR dem Configure -- auch wenn er flaglos ausfaellt.
        s += pmc_befund_zeile(pmc_befund);
        // CI-DUAL (Owner 14.08.): gcc-15-Pin ANGEFUEGT (E1; Invariante s. ceb_gcc_pin_define).
        s += "    - cmake -B build -G Ninja -DCOMDARE_V32_ENABLE=ON" + ceb_pmc_compile_define(pmc_befund) +
             " -DCMAKE_BUILD_TYPE=Release" + ceb_combo_compile_define(c.legend) + ceb_gcc_pin_define() + "\n";
        s += "    - cmake --build build --target comdare-messung-driver\n";
        s += "    - |\n";
        s += "      set -euo pipefail\n";
        s += "      DRIVER=$(find build -type f -name \"comdare-messung-driver\" | head -1)\n";
        s +=
            "      test -n \"$DRIVER\" -a -x \"$DRIVER\" || { echo \"comdare-messung-driver (CEB) fehlt\"; exit 1; }\n";
        s += "      echo \"== STUFE 1: CEB " + c.legend + " gebaut (Messsystem-Typ) ==\"\n";
        // E-04-P1 (Teil 1b): dasselbe Ereignis MASCHINENLESBAR. Der Owner-KERN fragt woertlich, "ob der
        // CacheEngineBuilder Orchestrator gebaut wird" -- die Prosa-Zeile darueber beantwortet das fuer den
        // Menschen, diese fuer den Aggregator (gleiche Testat-Grammatik: Marke, ts=, dann Felder).
        s += "      echo \"[CEB-TESTAT] ts=$(date -u +%FT%TZ) ceb=" + c.legend + " status=gebaut\"\n";
        // CI-DUAL (E2): der clang-22-Zwilling der Stufe 1 -- NUR im BAU-Job (ceb:emit baut denselben
        // Treiber als Emissions-Vehikel erneut und braucht keinen zweiten Zwilling derselben Stufe).
        emit_ci_dual_clang_twin(s, pmc_befund, c.legend, "Release", "ceb=" + c.legend);
        return s;
    }

    // STUFE 1b: die GEBAUTE CEB emittiert SELBST Child-2 ("tier ci", CEB-Rolle) -> Artefakt. §40.b-Hoheit.
    [[nodiscard]] static std::string emit_ceb_emit_job(PlanMeasurementCombo const& c, bool emit_combo_selector,
                                                       std::string const&   profile_basename,
                                                       PmcHostBefund const& pmc_befund) {
        std::string const slug = legend::cmake_slug(c.legend);
        std::string const art  = "tier-child-" + slug + ".yml";
        std::string       s;
        s += "# JOB ceb-emit combo " + std::to_string(c.index) + " (STUFE 1->2: CEB emittiert Child-2, CEB-Hoheit)\n";
        s += "\"" + legend::ceb_emit_job(c.legend) + "\":\n";
        s += "  stage: ceb-emit\n";
        s += "  tags: [\"amd64\"]\n";
        s += "  needs: [\"" + legend::ceb_build_job(c.legend) + "\"]\n";
        s += "  script:\n";
        // S2-NACHT: der Prolog verdrahtet COMDARE_GOLDEN_N_PROFILE auf das AKTIVE Profil; der "tier ci"-Aufruf
        // unten liest GENAU diese Variable ($COMDARE_GOLDEN_N_PROFILE) -> der Grandchild-Katalog folgt dem Profil-Scope.
        emit_child_submodule_prolog(s, profile_basename); // W10-Nacharbeit 2: ceb:emit baut Treiber neu -> ce-Quellen
        s += "    - cd Code\n";
        // W2: derselbe CT-Einbau wie im ceb:build-Job -- die hier NEU gebaute CEB muss dieselbe Combo tragen,
        // sonst emittierte eine [all]-CEB die Stufe-2 einer combo-gehaerteten Strecke. F-B1 (05.08.2026):
        // [all] traegt auch hier die -U-Loeschung statt "kein Zusatz" (Alt-Aussage galt NUR VOR F-B1;
        // sticky Cache-Var, s. F-B1-Block ueber ceb_combo_is_full_set).
        // M-2/B1: PMC-Pflicht (F9) -- Spiegel des ceb:build-Jobs. Auch dieser Job baut den Mess-Treiber neu,
        // also gilt hier dieselbe Invariante (ceb_pmc_compile_define()).
        // I-PMC-2: derselbe Befund aus demselben Plan-Kopf -- die vier Stellen koennen nicht auseinanderlaufen.
        s += pmc_befund_zeile(pmc_befund);
        // CI-DUAL (Owner 14.08.): derselbe gcc-15-Pin wie im ceb:build-Spiegel (E1) -- die vier
        // Emissionsstellen koennen nicht auseinanderlaufen; KEIN Zwilling (der gehoert der BAU-Stufe).
        s += "    - cmake -B build -G Ninja -DCOMDARE_V32_ENABLE=ON" + ceb_pmc_compile_define(pmc_befund) +
             " -DCMAKE_BUILD_TYPE=Release" + ceb_combo_compile_define(c.legend) + ceb_gcc_pin_define() + "\n";
        s += "    - cmake --build build --target comdare-messung-driver\n";
        s += "    - |\n";
        s += "      set -euo pipefail\n";
        s += "      DRIVER=$(find build -type f -name \"comdare-messung-driver\" | head -1)\n";
        // G4b-2/2.4-(7): ceb:emit war der EINZIGE Job, der die Storage-Aktivierung nicht rief -- und zugleich der
        // Job, der "tier ci" faehrt. Ohne den Aufruf ist hier Push/Pull inert, der 12er-TRIES-Default greift,
        // und eine Bestandslog-Reservierung waere Fiktion (minio aus) oder stiller Leerlauf. Platzierung wie an den
        // beiden anderen Aufrufstellen (:988/:1117): direkt hinter der DRIVER-Ermittlung, im selben `- |`-Block.
        emit_storage_activation(s); // G4a P-A: Push/Pull scharfschalten (inert ohne COMDARE_STORAGE_CACHE) + Deckel
        s += "      # §40.b-Praezisierung: die CEB (nicht der Planer) emittiert ihre STUFE-2-Sicht (System-Perms\n";
        s += "      # des FREIGEGEBENEN Raums + je-Host Build+Pruef-Batch + gegatete Mess-Batches) via "
             "'tier ci'.\n";
        // A5 (§56-T2-FANOUT D4): bei N>1 CEB-Konfigs traegt der ceb:emit-Aufruf den distinct Combo-Selektor
        // (--measurement-combo=<cmake_slug>) -- so emittiert jeder ceb:emit-Job GENAU seine EINE CEB-Konfig
        // (Kollisionsschutz der combo-unabhaengigen tier:build-Job-Namen, §56/T6). count==1 => leer => byte-identisch.
        std::string const combo_arg = emit_combo_selector ? " --measurement-combo=" + slug : std::string{};
        // P8-REST (27.07.): der emittierte Aufruf steht in der SUBKOMMANDO-Form 'tier ci'. Das Alt-Flag
        // --emit-tier-ci ist heute nur noch ein DEPRECATED-Alias, den der Abschluss-Aufraeumpass (Ledger §75)
        // entfernt -- eine Emission, die den Alias faehrt, wuerde in genau diesem Moment brechen. Der Selektor
        // bleibt ein Flag und haengt unveraendert hinten an (Dispatcher kanonisiert nur argv[1]/argv[2]).
        s += "      \"$DRIVER\" tier ci \"$COMDARE_GOLDEN_N_PROFILE\"" + combo_arg + " > \"$CI_PROJECT_DIR/" + art +
             "\"\n";
        s +=
            "      echo \"== CEB-emittierte STUFE-2 (erste 20 Zeilen) ==\"; head -20 \"$CI_PROJECT_DIR/" + art + "\"\n";
        s += "  artifacts:\n";
        s += "    paths:\n";
        s += "      - " + art + "\n";
        return s;
    }

    // S2-NACHT-2 (2026-07-23): eine Forward-Allowlist-Zeile LITERAL aus der Planer-Env. Ungesetzt/leer => Zeile
    // ENTFAELLT (kein NAME: "" -- das ueberschriebe im Grandchild die 'nicht gesetzt'-Rules-Semantik). NIE die
    // Selbst-Referenz NAME: "$NAME" (GitLab-'circular variable reference' im Child). KLASSE: nur Werte, nie $CI_PROJECT_DIR.
    static void append_forward_var_literal(std::string& out, char const* name) {
        char const* const val = std::getenv(name);
        if (val == nullptr || *val == '\0') return; // ungesetzt/leer => Zeile weglassen (Grandchild-Default-Semantik)
        out += "    " + std::string{name} + ": \"" + val + "\"\n";
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
        // S1/A1 P-TOTAL (Ledger 46) + S2-NACHT-2 (2026-07-23, Zirkularitaets-Fix): explizite Forward-Allowlist an der
        // ZWEITEN Trigger-Grenze. Self-contained Grandchild-Pipelines erben Pipeline-Variablen NICHT -> die Werte als
        // YAML-Variablen dieses Bridge-Jobs deklarieren und via forward:yaml_variables reichen. Die RHS wird zur
        // EMISSIONSZEIT aus der Planer-Env LITERAL eingebrannt -- NICHT NAME: "$NAME": diese Selbst-Referenz wertet
        // GitLab im Child als 'circular variable reference' aus (config_error, leeres failed-Child; Befund Struktur-
        // Smoke 12628/12663). Leer/ungesetzt => Zeile ENTFAELLT (eine leere YAML-Variable ueberschriebe im Grandchild
        // die 'nicht gesetzt'-Semantik: die STUFE-3-Mess-Rule '$COMDARE_MEASURE_PROFILE == "smoke"'-Auto-Run bzw. den
        // ${COMDARE_GN_TOTAL:-16}-Fallback des Tier-Baus). Deterministische Reihenfolge; KLASSE: nur Werte/Basenames,
        // nie $CI_PROJECT_DIR. Die Env setzt die super-YAML (Schicht 4): GN_TOTAL=131072 im Voll-Bau; MEASURE_PROFILE=
        // smoke + METHODIK-Basename im Smoke. Leerer variables:-Block wird ausgelassen. KEIN pipeline_variables (Isolation).
        std::string vars;
        append_forward_var_literal(vars, "COMDARE_GN_TOTAL");
        append_forward_var_literal(vars, "COMDARE_MEASURE_PROFILE");
        append_forward_var_literal(vars, "COMDARE_PLAN_METHODIK_PROFILE");
        // G4a P-A (2026-07-26): der Storage-Schalter muss die ZWEITE Trigger-Grenze ueberleben, sonst laeuft der
        // Grandchild-Strang (Bau-/Mess-Batch) storage-INERT -- kein Push, kein Pull, kein Prune. Reiner bool-Schalter,
        // KEIN Credential: die MinIO-Rohcreds faltet scripts/comdare_storage_activation.sh maschinenlokal, sie duerfen
        // diese Literal-Naht nie passieren (KLASSEN-Regel: nur Schalter/Schluessel/Namen, nie Geheimnisse).
        append_forward_var_literal(vars, "COMDARE_STORAGE_CACHE");
        // G4a Lager-/Gate-Durchreiche: die Opt-in-Schalter der beiden Gates, die Folge-A/Folge-B eingezogen haben.
        // Alle NICHT-geheim: BESTANDSLOG=bool, DOC_KEY=Objekt-Schluessel im Store, OWNER_UUID=UUID, MASCHINE=Hostname,
        // VARIANT_GATE=bool. Ungesetzt => Zeile entfaellt => YAML byte-identisch zum Stand vor G4a (Default-Semantik
        // im Grandchild bleibt 'nicht gesetzt', nicht 'leer gesetzt').
        append_forward_var_literal(vars, "COMDARE_BESTANDSLOG");
        append_forward_var_literal(vars, "COMDARE_BESTANDSLOG_DOC_KEY");
        append_forward_var_literal(vars, "COMDARE_BESTANDSLOG_OWNER_UUID");
        append_forward_var_literal(vars, "COMDARE_BESTANDSLOG_MASCHINE");
        append_forward_var_literal(vars, "COMDARE_VARIANT_GATE");
        if (!vars.empty()) {
            s += "  variables:\n";
            s += vars;
        }
        s += "  trigger:\n";
        s += "    include:\n";
        s += "      - artifact: " + art + "\n";
        s += "        job: \"" + legend::ceb_emit_job(c.legend) + "\"\n";
        s += "    strategy: depend\n";
        s += "    forward:\n";
        s += "      yaml_variables: true       # Allowlist (exakt der variables:-Block oben) an Grandchild\n";
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
//    serialisiert sie nativ) -- so laeuft je Maschine hoechstens EIN Batch, das Lane-Budget (je 24) ist voll
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
        out_ += "# Ledger §42.b/§56 + §62-B-Batch (CEB-Rolle 'tier ci'): NUR die STUFE-2-Sicht, O(Maschinen).\n";
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
    // DRINGEND (2026-07-23, Resume-CI-Fix): der GitLab-Checkout-Default 'git clean -ffdx' loescht Code/gn_out/ (Bau-
    // Artefakte je Zelle: .so + .version-Sidecar) UND Code/build/ am Job-Start -> der per-Binary-Sidecar-Resume
    // (dll_is_current) ist ueber Job-Grenzen WIRKUNGSLOS (jeder Batch baut die Slice neu; Befund: prod1 0 .so/.version
    // nach 12690-Vorlauf, 12718-amd baut Slice 1 komplett neu). Je STUFE-2-Batch-Job GIT_CLEAN_FLAGS mit -e-Ausnahmen
    // fuer beide Verzeichnisse + GIT_STRATEGY:fetch -> gn_out/build ueberleben zwischen Jobs DESSELBEN Runners, der
    // Resume greift real. KLASSE: reine Literale, workdir-relative Pfade, KEIN $CI_PROJECT_DIR. NUR Stufe 2 (die
    // Stufe-1-build/ ist der Treiber-CMake -- separates Merkposten-Paket). Geteilte Single-Source fuer Build- + Mess-Batch.
    static void emit_gn_out_persistence_variables(std::string& s) {
        s += "  variables:\n";
        // G4a P-C (2026-07-26): Code/measure_out zusaetzlich vom Checkout-Clean ausgenommen. Ohne den Exclude loescht
        // GitLabs Default `git clean -ffdx` die bereits geschriebenen Mess-CSV am Job-Start -- bei einem mehrtaegigen
        // Mess-Batch, der neu aufgesetzt/resumiert wird, waeren das VERLORENE MESSDATEN (Doktrin: Messdaten nie
        // loeschen). Bewusst in BEIDEN Batches derselbe Literal-String: im Bau-Batch existiert measure_out gar nicht,
        // der Exclude ist dort folgenlos -- dafuer bleibt es EINE Zeile, EINE Wahrheit, EIN Pin.
        s += "    GIT_CLEAN_FLAGS: \"-ffdx -e Code/gn_out -e Code/build -e Code/measure_out\"\n";
        s += "    GIT_STRATEGY: \"fetch\"\n";
    }

    // G4b-2/2.4-(7): emit_storage_activation stand hier als statisches Member. Sie ist auf Namespace-Ebene gehoben
    // (oberhalb von CiYamlBuilder), weil emit_ceb_emit_job als dritter Aufrufer in CiYamlBuilder liegt -- Rumpf
    // unveraendert, eine Literal-Quelle wie zuvor. Die Aufrufe hier unten bleiben wortgleich.

    // #27 (2026-07-23): das LOG EINMAL explizit truncieren, BEVOR die Treiber-Invocation im APPEND-Modus schreibt.
    // ZWINGEND (Review-Fix): ohne dieses `: >` und mit `>` (O_TRUNC) haette stdout einen EIGENEN Offset gegen den
    // O_APPEND-Offset von tee -> stdout-Writes ueberschrieben bereits per tee angehaengte stderr-/[FEHLER-TESTAT]-Zeilen
    // (Log-Korruption in der Diagnosequelle). `: > <log>` haelt zudem die Frisch-Truncate-Semantik beim GitLab-Retry
    // (LOGDIR liegt jetzt im clean-excludierten Bestand und kann Altstaende tragen).
    static void emit_driver_log_truncate(std::string& s, std::string const& indent, std::string const& log_path) {
        s += indent + ": > \"" + log_path + "\"   # #27: frisch truncieren, dann stdout+stderr im APPEND-Modus\n";
    }

    // #27 (2026-07-23, Slice-inneres Fortschritts-Testat): die Redirect-Naht JEDER Treiber-Invocation. stdout+stderr-
    // Detail geht vollstaendig nach <log> (Trace-Hygiene, Artefakt) im APPEND-Modus (>> + tee -a: O_APPEND-Writes sind
    // atomar ans Dateiende -> KEINE Offset-Kollision, der Voraussetzung ist das emit_driver_log_truncate davor). Die
    // [heartbeat]-Zeilen (ProgressHeartbeat, std::cerr) werden ZUSAETZLICH line-gepuffert in den Job-Trace (fd 2 des
    // Jobs) getee't -> der Zuschauer sieht "alle K Builds" den Slice-Fortschritt. Process-Substitution `2> >(...)` statt
    // Pipe: der Treiber-Exit-Code bleibt UNANGETASTET (der `if !`-Guard + pipefail sehen GENAU den Treiber, nicht
    // tee/grep -> Fehlersichtbarkeit unveraendert). Die Substitution wird nicht abgewartet (best-effort Trace);
    // `-F -e '...'` = fixe Zeichenketten (kein Regex-Escape), `--line-buffered` = sofort sichtbar, `|| true` =
    // set-e-sicher. Alle nicht gelisteten Zeilen bleiben NUR im <log>.
    //
    // E-04-P1 (Teil 1c): die Filter-Liste traegt zusaetzlich zum [heartbeat] die Marken der Marker-Familie v2 --
    // ABGELEITET aus bex::kSliceMarkerTraceMarken (dieselbe Konstante, die der Treiber emittiert). Eine per Hand
    // gepflegte zweite Liste haette den Filter still hinter einem umbenannten/ergaenzten Marker zurueckgelassen:
    // der Kanal waere gruen und stumm zugleich. Trace-Hygiene bleibt: Detail (Compiler-Ausgabe, [pruef-fail],
    // [bestandslog]) bleibt im Artefakt-Log; im Trace stehen nur KOPF + Testate.
    [[nodiscard]] static std::string driver_log_redirect(std::string const& log_path) {
        std::string filter = "grep --line-buffered -F -e '[heartbeat]'";
        for (std::string_view const marke : bex::kSliceMarkerTraceMarken) {
            filter += " -e '[";
            filter += marke;
            filter += "]'";
        }
        return ">> \"" + log_path + "\" 2> >(tee -a \"" + log_path + "\" | " + filter + " >&2 || true)";
    }

    // #29 (2026-07-23, Cancel-Sauberkeit, lokal verifiziert): der Runner-Cancel schickt SIGTERM an die (orphan-)
    // Job-bash -- die traegt eine EIGENE Prozessgruppe (PGID==$$, deshalb trifft der Wrapper-Kill die interne bash NICHT,
    // #29-Befund). Der Trap demontiert sich ZUERST (trap - TERM INT), dann killt er die eigene Gruppe (kill -- -$$): das
    // an SELF gesendete TERM terminiert per Default (KEIN Re-Entry-Loop), waehrend Driver + Compiler-Enkel (gleiche
    // Gruppe) MITSTERBEN -> keine Waisen-Schleife mehr. Lokal verifiziert: Marker-Enkel friert nach Cancel ein, Trap
    // feuert genau 1x. `2>/dev/null` schluckt die Fehlermeldung, falls die Gruppe schon weg ist.
    static void emit_batch_cancel_trap(std::string& s) {
        s += "      trap 'trap - TERM INT; kill -- -$$ 2>/dev/null' TERM INT   # #29: Cancel -> eigene Prozessgruppe "
             "(bash-Loop+Driver+Compiler) beenden, keine Waisen\n";
    }

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
        std::string const par  = std::to_string(lane_build_parallelism(host)); // §62-B Lane-Budget-Literal (T-Wert)
        // S6-P1b Env-Bruecke (d): ab N>1 traegt das Treiber-Kommando die gewaehlte Mess-Combo.
        // [KORREKTUR 12.08.2026] Hier stand die Kurzform "[all] -> LEER -> byte-stabil". Sie ist missverstaendlich:
        // LEER meint AUSSCHLIESSLICH die emittierte ENV-ZEILE (bei [all] wird KEIN COMDARE_MEASUREMENT_COMBO=
        // exportiert), NICHT den Mess-Stempel. Der Code darunter ist unveraendert korrekt; nur die Kurzform
        // verleitete Leser zu "[all] stempelt leer". Die Konsumentenseite leitet aus der FEHLENDEN Env die
        // VOLLMENGE ab: UNGESETZT -> "[all]" (mess_achsen_naht.hpp:317) -> Vollmengen-Zeile
        // (anatomy_version_stamp.hpp:378, Section 64-D1-B 22.07.2026). byte-stabil ist also die [all]-LANE
        // (alle [all]-Laeufe rendern dieselbe Vollmengen-Zeile), nicht "leer".
        // KEINE AUSSAGE UEBER DIE EMITTIERTE DLL: ob die Tier-Quelle diese Zeile auch TRAEGT, entscheidet nicht
        // diese Env-Zeile, sondern welcher Source-Gen die id bedient (INC-G6-Vorrang base_union vor lazy_gen).
        // Fuer Basis-320-Zellen ist das der Katalog mit der 2-arg-Form -- am Objekt gemessen 12.08.2026, Herleitung
        // und Messwerte in profile_facade/profile_run_entry.hpp am lazy_gen (offener F1-Durchstich-Blocker).
        // NUR KOMMENTAR -- kein Code geaendert. (i) §61-STUFEN: nur der Nicht-Default-Build (Debug) traegt
        // COMDARE_BUILD_TYPE (Reuse-Schluessel).
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
        emit_gn_out_persistence_variables(
            s); // Resume-CI-Fix: gn_out/build ueberleben den Checkout-Clean (dll_is_current)
        s += "  script:\n";
        // S2-NACHT: der Prolog verdrahtet COMDARE_GOLDEN_N_PROFILE auf das AKTIVE Profil (header_.profile_basename).
        emit_child_submodule_prolog(s, header_.profile_basename); // W10-Nacharbeit 2: manueller ce-Submodul-Klon
        s += "    - cd Code\n";
        // S5-P1: CMAKE_BUILD_TYPE aus der aufgeloesten Run-Methodik (measure => "Release"); Default byte-identisch zu HEAD.
        // W2: der CEB-NEUBAU im Batch-Job-Kontext traegt die Combo COMPILE-hart -- HIER entstehen die Stempel real
        // (die CEB generiert in diesem Job die DLL-Quellen); ohne den Define bliebe die Haertung Fiktion.
        // M-2/B1: PMC-Pflicht (F9). WICHTIG fuer die Kosten: dieser Bau-Batch und der Mess-Batch teilen sich
        // DASSELBE Code/build (emit_gn_out_persistence_variables). Traegt nur EINER der beiden das Flag,
        // rekonfiguriert der jeweils andere das Verzeichnis zurueck und der Treiber-Neubau faellt bei JEDEM
        // Job-Wechsel an -- ueber einen 7-Tage-Batch die eigentliche Kostenfalle. Deshalb beide oder keiner.
        // I-PMC-2: dasselbe gilt fuer die VENDOR-Wahl -- ein Vendor-Unterschied zwischen Bau- und Mess-Batch
        // rekonfigurierte dasselbe Code/build hin und her und erzwaenge denselben Dauer-Neubau.
        s += pmc_befund_zeile(header_.pmc_befund);
        // CI-DUAL (Owner 14.08.): gcc-15-Pin ANGEFUEGT (E3; beide Batches teilen Code/build => derselbe
        // Pin in Bau- UND Mess-Batch, sonst rekonfigurierte der Wechsel das Verzeichnis bei jedem Job).
        s += "    - cmake -B build -G Ninja -DCOMDARE_V32_ENABLE=ON" + ceb_pmc_compile_define(header_.pmc_befund) +
             " -DCMAKE_BUILD_TYPE=" + header_.build_semantic.cmake_build_type +
             ceb_combo_compile_define(combo_legend_) + ceb_gcc_pin_define() + "\n";
        s += "    - cmake --build build --target comdare-messung-driver\n";
        // CI-DUAL (E2): der clang-22-Zwilling der Stufe 2 -- VOR dem Provision-Batch (fail-fast: eine
        // clang-Inkompatibilitaet faellt, BEVOR ein 7d-Batch startet). BUILD_TYPE folgt der Stufe --
        // im (j3)-Debug-Profil ist das die clang-x-Debug-Zelle der SOLL-Matrix. Der Mess-Batch
        // (Schwesterstelle unten) erhaelt KEINEN Zwilling (N2: Mess-Glieder single-compiler).
        emit_ci_dual_clang_twin(s, header_.pmc_befund, combo_legend_, header_.build_semantic.cmake_build_type,
                                "ceb=" + combo_legend_ + " lane=" + host);
        s += "    - |\n";
        s += "      set -euo pipefail\n";
        emit_batch_cancel_trap(
            s); // #29: Cancel-Sauberkeit -- SIGTERM/INT beendet die eigene Prozessgruppe, keine Waisen
        s += "      DRIVER=$(find build -type f -name \"comdare-messung-driver\" | head -1)\n";
        s += "      test -n \"$DRIVER\" -a -x \"$DRIVER\" || { echo \"comdare-messung-driver fehlt\"; exit 1; }\n";
        emit_storage_activation(s); // G4a P-A: Push/Pull scharfschalten (inert ohne COMDARE_STORAGE_CACHE) + Deckel
        // §62-B Lane-Budget (Bau-Pool-WORKER-Override, KEIN $(nproc)): lane_build_parallelism(host) = 24 (beide Lanes; amd von 32 gedrosselt, RAM-Bound).
        s += "      export COMDARE_BUILD_PARALLEL=\"" + par + "\"   # §62-B Lane-Budget " + host +
             " (lane_build_parallelism; harte Compile-Worker-Zahl statt der nproc-Heuristik; amd RAM-gedrosselt)\n";
        s += "      TOTAL=\"${COMDARE_GN_TOTAL:-16}\"   # Default 16 = sicherer Serie-Test; Voll-Bau: "
             "COMDARE_GN_TOTAL=131072\n";
        s += "      SLICE=" + std::to_string(kGnBatchSlice) +
             "   # §62-B-Bestandslog-Korn (harte Konstante, KEIN Env-Override)\n";
        s += "      FAIL=0\n";
        s += "      LOGDIR=\"$CI_PROJECT_DIR/Code/gn_out/" + slug + "/" + host + "/logs\"\n";
        s += "      mkdir -p \"$LOGDIR\"\n";
        // E-04-P1: die Host-Lane als EXPLIZITER Vertrag an den Treiber -- er rendert daraus das Pflichtfeld lane=
        // seiner Marker-Zeilen. Ohne diese Zeile muesste der Treiber die Lane raten (Hostname-Heuristik); der
        // emittierende Planer kennt sie und sagt sie.
        s += "      export COMDARE_LANE=\"" + host +
             "\"   # E-04-P1: Pflichtfeld lane= der Treiber-Marker (Planer sagt die Lane, der Treiber raet nie)\n";
        // C-10/E-10 (21.08.2026): window_belongs_to-VERDRAHTUNG, Generator-Haelfte (Gen-2-Kampagnenbetrieb,
        // KON29-04). Die Fenster-Schleife unten traegt ab jetzt die PARITAETS-ZUTEILUNG des Bestandslogs --
        // WOERTLICH die Formel von bestandslog::window_belongs_to (batch_planner.hpp: Fenster w gehoert
        // Maschine rank von n, wenn w % n == rank; n == 0 -> alle; Semantik-Paritaet testgedeckt).
        // Topologie-Quelle sind die CI-Variablen COMDARE_MACHINE_RANK / COMDARE_MACHINES; die Defaults 0/1
        // sind das HEUTIGE Verhalten (jede Lane-Maschine baut ALLE Fenster ihrer Perms) -- der W3-Zug
        // "zweilanige Messung" belegt sie nur noch mit Werten, statt die Emission umzubauen.
        // FAIL-CLOSED: rank >= n hiesse w % n == rank NIE -- die Maschine baute STILL NICHTS und der Batch
        // endete gruen mit leerer Ausbeute; deshalb harter Abbruch VOR der ersten Perm.
        // NICHT Teil dieser Haelfte (W3/E-13, ausdruecklich benannt): Job-Verdopplung je Lane, Artefakt-
        // Transport zwischen Maschinen (minio Ebene B), Pruef-/Mess-Fenster bei echter Teilung, dynamische
        // Claims a 4096 + Takeover ETA+50%.
        s += "      RANK=\"${COMDARE_MACHINE_RANK:-0}\"; MASCHINEN=\"${COMDARE_MACHINES:-1}\"   # C-10 "
             "Gen-2-Topologie (Default 0/1 = alle Fenster dieser Lane-Maschine)\n";
        s += "      if [ \"$MASCHINEN\" -gt 0 ] && [ \"$RANK\" -ge \"$MASCHINEN\" ]; then\n";
        s += "        echo \"FEHLER: COMDARE_MACHINE_RANK=$RANK >= COMDARE_MACHINES=$MASCHINEN -- diese "
             "Maschine bekaeme STILL kein Fenster (window_belongs_to nie wahr). Abbruch statt leerem Gruen.\"\n";
        s += "        exit 1\n";
        s += "      fi\n";
        s += "      echo \"== [FENSTER-TOPOLOGIE] lane=" + host +
             " rank=$RANK maschinen=$MASCHINEN semantik=window_belongs_to (w%n==rank; n=0->alle; "
             "batch_planner.hpp) ==\"\n";
        // Batch-KOPF (Echo, einmal): CEB-Identitaet [a,b,c] + Lane -> Trace-Legende der CEB-Ebene.
        // E-04-P1 (Teil 1b): zusaetzlich die BEZUGSGROESSEN des Fortschritts -- fenster_gesamt (Aufrundung
        // TOTAL/SLICE, rein Shell-arithmetisch und damit byte-deterministisch) und perms (die Zahl der Perms
        // DIESER Lane). Erst damit ist ein Testat "fenster=X:Y" ohne Kenntnis der Emission einordenbar.
        s += "      echo \"== [BATCH-BAU] ceb=" + combo_legend_ + " lane=" + host +
             " total=$TOTAL slice=" + std::to_string(kGnBatchSlice) +
             " fenster_gesamt=$(( (TOTAL + SLICE - 1) / SLICE )) perms=" + std::to_string(perms.size()) +
             " ts=$(date -u +%FT%TZ) ==\"\n";
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
            // C-10/E-10: die Paritaets-Zuteilung des Bestandslogs, WOERTLICH gespiegelt (window_belongs_to,
            // batch_planner.hpp -- (w % n) == rank; n == 0 -> alle; FENSTER = START/SLICE ist der Fenster-INDEX
            // im 4096er-Korn). Ein fremdes Fenster wird NICHT gebaut und traegt seine EIGENE
            // [FENSTER-FREMD]-Zeile -- BEWUSST OHNE "TESTAT" im Namen und OHNE offen=: die
            // [TESTAT]-Marken zaehlen GEBAUTE Fenster (W0b-3/E-04-P1, kTestatKlassifikation), ein
            // uebersprungenes fremdes Fenster darf weder dort noch in der offen=-Gleichung ("je Bau-Fenster
            // ZWEI offen=-Zeilen") mitzaehlen. Mit der Default-Topologie 0/1 ist der Zweig unerreichbar
            // (0 % 1 == 0): das heutige Verhalten ist im Testat-Strom byte-fuer-byte unveraendert.
            s += "        FENSTER=$(( START / SLICE ))\n";
            s += "        if [ \"$MASCHINEN\" -gt 0 ] && [ $(( FENSTER % MASCHINEN )) -ne \"$RANK\" ]; then\n";
            s += "          echo \"[FENSTER-FREMD] ts=$(date -u +%FT%TZ) lane=" + host + " zelle=" + cell +
                 " phase=bau fenster=${START}:${COUNT} rank=$RANK maschinen=$MASCHINEN\"\n";
            s += "          START=$(( START + COUNT )); continue\n";
            s += "        fi\n";
            emit_driver_log_truncate(s, "        ", "$LOGDIR/perm" + idx + "_bau_${START}.log");
            s += "        if ! COMDARE_THESIS_PROFILE=\"$COMDARE_GOLDEN_N_PROFILE\" \\\n";
            s += "             " + combo_env + build_type_env + "COMDARE_GN_OPT=\"" + opt + "\" COMDARE_GN_SIMD=\"" +
                 simd + "\" COMDARE_GOLDEN_N_PROVISION_ONLY=true COMDARE_RUN_SOTA=0 \\\n";
            s += "             COMDARE_GOLDEN_N_RANGE=\"${START}:${COUNT}\" \\\n";
            s += "             \"$DRIVER\" experiment_config \"" + dll_dir + "\" \\\n";
            // #27: Detail nach <log>; die [heartbeat]-Zeilen (alle K Builds via ProgressHeartbeat every_n) zusaetzlich in den Trace.
            s += "             " + driver_log_redirect("$LOGDIR/perm" + idx + "_bau_${START}.log") + "; then\n";
            // W0b-3 (2026-08-08): offen= steht ZUSAETZLICH auf der Fehler-Zeile. Vor der else-Bindung unten
            // trug jedes Fenster sein offen= ueber das unbedingte [TESTAT]; mit der Bindung faellt es fuer
            // das gescheiterte Fenster ersatzlos weg -- also genau dort, wo "wie viele davon noch offen"
            // am meisten zaehlt. Beide Zeilen eines Guards tragen deshalb DENSELBEN Feldsatz; der Aggregator
            // sieht den Fortschritt auf beiden Pfaden. Rein Shell-arithmetisch, damit byte-deterministisch.
            s += "          echo \"[FEHLER-TESTAT] ts=$(date -u +%FT%TZ) lane=" + host + " zelle=" + cell +
                 " phase=bau fenster=${START}:${COUNT} offen=$(( TOTAL - START - COUNT ))\"; FAIL=1\n";
            // W0b-3 (2026-08-08) -- SCHWESTERSTELLE zu D3-5, derselbe Defekt im Bau-Batch: das [TESTAT] stand
            // bis hierher HINTER dem fi und wurde damit UNBEDINGT gedruckt, auch fuer das eben gescheiterte
            // Fenster. Der Guard ist WEICH (FAIL=1 statt Abbruch), die Zeile lief also wirklich; ein
            // fehlgeschlagenes Fenster trug beide Testate untereinander. Wer die [TESTAT]-Zeilen als
            // "gebaute Fenster" zaehlt -- der naheliegendste Gebrauch --, zaehlte die gescheiterten mit: der
            // Nenner war um genau die Fehlerzahl zu gross und schoente sich, je mehr schiefging.
            // Ab jetzt im else-Zweig: [TESTAT] heisst "dieses Fenster WURDE gebaut", nicht "wir sind hier
            // vorbeigekommen". Die beiden Testate schliessen einander aus; je Fenster steht genau eins.
            s += "        else\n";
            // E-04-P1 (Teil 1b): offen= = die nach DIESEM Fenster noch ausstehenden Binaries der Perm
            // (TOTAL - START - COUNT, rein Shell-arithmetisch => byte-deterministisch). Beantwortet den
            // Owner-KERN "wie viele davon noch offen" auf der SHELL-Ebene; die Treiber-Zeile [PLAN-TESTAT]
            // beantwortet dieselbe Frage INNERHALB des Fensters (zu_bauen nach Lager-Filter). Zwei Sichten,
            // eine Frage -- deshalb dasselbe Vokabular.
            s += "          echo \"[TESTAT] ts=$(date -u +%FT%TZ) lane=" + host + " zelle=" + cell +
                 " phase=bau fenster=${START}:${COUNT} offen=$(( TOTAL - START - COUNT ))\"\n";
            s += "        fi\n";
            s += "        START=$(( START + COUNT ))\n";
            s += "      done\n";
            // S3-PRUEF-Schritt (UNBEDINGT je Perm nach der Fenster-Schleife): COMDARE_PRUEF_ONLY=true faehrt NUR das
            // Konformitaets-Gate je gebauter .so ueber 0:TOTAL im GLEICHEN dll_dir (kein Bau, keine Messung). [d,e,f][g,h,i].
            s += "      echo \"== [PRUEF] zelle=" + cell + " lane=" + host + " ts=$(date -u +%FT%TZ) ==\"\n";
            emit_driver_log_truncate(s, "      ", "$LOGDIR/perm" + idx + "_pruef.log");
            s += "      if ! COMDARE_THESIS_PROFILE=\"$COMDARE_GOLDEN_N_PROFILE\" \\\n";
            s += "           " + combo_env + build_type_env + "COMDARE_GN_OPT=\"" + opt + "\" COMDARE_GN_SIMD=\"" +
                 simd + "\" COMDARE_PRUEF_ONLY=true COMDARE_RUN_SOTA=0 \\\n";
            s += "           COMDARE_GOLDEN_N_RANGE=\"0:${TOTAL}\" \\\n";
            s += "           \"$DRIVER\" experiment_config \"" + dll_dir + "\" \\\n";
            // #27: Detail nach <log>; die [heartbeat]-Zeilen (pruef-zelle) zusaetzlich in den Trace.
            s += "           " + driver_log_redirect("$LOGDIR/perm" + idx + "_pruef.log") + "; then\n";
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
        std::string const par =
            std::to_string(lane_build_parallelism(host)); // §62-B Lane-Budget (T-Wert, ersetzt $(nproc))
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
        emit_gn_out_persistence_variables(
            s); // Resume-CI-Fix: gn_out/build ueberleben den Checkout-Clean (dll_is_current)
        s += "  needs:\n";
        // §62-B: EINE Bau->Mess-Kante auf den Build-Batch derselben Lane (statt der 4 Chunk-Kanten je Perm).
        s += "    - \"" + legend::tier_batch_build_job(host) + "\"\n";
        // §41/320er: Auto-Messlauf NUR im smoke-Profil; sonst when:manual (BYTE-GLEICH zur Vor-S4-Emission).
        s += "  rules:\n";
        s += "    - if: '$COMDARE_MEASURE_PROFILE == \"smoke\"'\n";
        s += "      when: on_success   # smoke: Auto-Messlauf (kleiner Umfang, Rauch-Test der Mess-Strecke)\n";
        s += "    - when: manual       # sonst: 320er-§41-Gate (Voll-Messlauf erst nach User-Entscheid)\n";
        // #278 / OV-16 (2026-08-09): HIER STAND EINE EMISSION DES VERBOTENEN FLAGS -- ersatzlos entfernt.
        // (Die entfernte Zeile wird hier bewusst NICHT als Emissions-Literal zitiert -- ein Audit, das nach
        //  Emissions-Zeilen dieses Flags sucht, soll 0 Treffer haben und nicht an einem Kommentar haengen.)
        // SELBSTCHECK: der Mess-Batch traegt KEIN allow_failure; sein Schluss-Verdikt ist `exit $FAIL` (unten),
        //   also faellt der CI-Job hart, sobald auch nur eine Zelle scheiterte. Zugesichert von
        //   test_experiment_plan_director.cpp TierCiYamlBuilder.KeinAllowFailureInEmittierterJobYamlBeideStufen...
        //   (0 Treffer je Batch, Nenner 2 Host-Lanes) -- der Test war mit dieser Zeile ROT.
        // DIE ZWEI EBENEN, die hier verschmolzen waren und nie wieder verschmelzen duerfen:
        //   ZELLE -> ein Mess-Fehler gibt [FEHLER-TESTAT] + Log aus, setzt FAIL=1 und die Schleife MISST WEITER
        //            (die CSV-Zelle traegt "failed", nie eine stille Null; perm_runner.hpp:145/354).
        //   JOB   -> der Batch endet auf `exit $FAIL` und ist damit HART ROT. Das ist die Owner-Regel.
        // PROVENIENZ DES DEFEKTS: Owner 2026-07-06 14:16:43 UTC "bei einer harten Pipeline darf es kein allow
        //   failure geben" (Issue #278), Verschaerfung 2026-07-17 "die gesamte Pipeline IMMER hart gruen".
        //   Zwei Tage nach der Verschaerfung fuegte b5e64a51c (2026-07-19) das Flag ohne GO ein; 0d91dc1e3
        //   (2026-07-20) klebte den Kommentar "Sichtbarkeits-Doktrin" darueber -- die erste Erwaehnung des
        //   Begriffs ueberhaupt. Die Owner-Aussage vom 2026-07-16, auf die er sich berief, galt der CSV-ZELLE.
        //   Bestaetigung Owner 2026-08-09: "Allow failure war schon IMMER verboten ... aber der CI-Job failed
        //   immer hart."
        // FOLGE-STUFEN (am Objekt geprueft, super-Pipelines 15372/15306): ein hart roter Mess-Batch stoppt
        //   persist:measurements / anhang:forward / thesis:pdf NICHT -- keiner von ihnen hat eine needs-Kante
        //   auf planer:delegate-trigger. In 15372 fiel die Bridge trigger:cache-engine hart, und thesis:pdf
        //   (spaetere Stage) lief trotzdem auf success.
        s += "  script:\n";
        // S2-NACHT: der Prolog verdrahtet COMDARE_GOLDEN_N_PROFILE auf das AKTIVE Profil (header_.profile_basename).
        emit_child_submodule_prolog(s, header_.profile_basename); // ce-Submodul-Klon, Spiegel des Bau-Jobs
        s += "    - cd Code\n";
        // W2: Spiegel des Build-Batch -- der Mess-Batch baut die CEB neu und muss dieselbe Combo einkompiliert
        // tragen, sonst truege dieselbe Zelle je nach Job zwei verschiedene Mess-Zeilen. F-B1 (05.08.2026):
        // [all] traegt auch hier die -U-Loeschung statt "kein Zusatz" (Alt-Aussage galt NUR VOR F-B1;
        // sticky Cache-Var, s. F-B1-Block ueber ceb_combo_is_full_set).
        // M-2/B1: PMC-Pflicht (F9) -- DIE Stelle, an der die 131.072-Zellen-Matrix ihren Treiber baut. Ohne das
        // Flag liefert pmc_source_factory.hpp die NullPmcSource und ALLE HW-Spalten sind strukturell 0; der
        // #37-Preflight zwei Bloecke weiter unten wertete genau diesen Ausfall als Erfolg (pmc=ok).
        // Spiegel des Bau-Batches: dasselbe geteilte Code/build, deshalb dasselbe Configure-Kommando.
        // I-PMC-2: und derselbe Befund -- Spiegel heisst hier auch Vendor-Spiegel.
        // CI-DUAL (Owner 14.08.): Spiegel schliesst den gcc-15-Pin EIN (E3, geteiltes Code/build);
        // KEIN clang-Zwilling -- in den Mess-Gliedern wird NIE mit zwei Compilern gemessen (N2).
        s += pmc_befund_zeile(header_.pmc_befund);
        s += "    - cmake -B build -G Ninja -DCOMDARE_V32_ENABLE=ON" + ceb_pmc_compile_define(header_.pmc_befund) +
             " -DCMAKE_BUILD_TYPE=" + header_.build_semantic.cmake_build_type +
             ceb_combo_compile_define(combo_legend_) + ceb_gcc_pin_define() + "\n";
        s += "    - cmake --build build --target comdare-messung-driver\n";
        s += "    - |\n";
        s += "      set -euo pipefail\n";
        emit_batch_cancel_trap(
            s); // #29: Cancel-Sauberkeit -- SIGTERM/INT beendet die eigene Prozessgruppe, keine Waisen
        s += "      DRIVER=$(find build -type f -name \"comdare-messung-driver\" | head -1)\n";
        s += "      test -n \"$DRIVER\" -a -x \"$DRIVER\" || { echo \"comdare-messung-driver fehlt\"; exit 1; }\n";
        emit_storage_activation(s); // G4a P-A: Pull/Push + Prune scharfschalten (inert ohne COMDARE_STORAGE_CACHE)
        // G4a #37 / §66-N2 (PMC-DOKTRIN je Vendor): HARTER PMC-Preflight auf DIESER Lane-Maschine, VOR der ersten
        // Messung. Grund: die super-Pipeline haelt zwar pmc:amd/pmc:intel hart, aber dieser Batch laeuft im
        // GRANDCHILD-Strang und kann auf jene Jobs kein `needs:` setzen -- ohne Preflight koennte eine Lane eine
        // mehrtaegige Messung mit kaputtem perf_event_open durchlaufen und lauter 0-Zaehler produzieren. Der Probe-Weg
        // ist der OFFIZIELLE (die bestehenden ctest-Targets), KEIN Behelfs-Probeprogramm: Code/CMakeLists.txt zieht die
        // ce per add_subdirectory ein und COMDARE_BUILD_TESTS ist ON -> beide PMC-Targets sind im `build`-Baum bereits
        // KONFIGURIERT und muessen nur noch gebaut werden (kein zweiter cmake-Configure). linux_perf_pmc_smoke steht
        // unter if(UNIX AND NOT APPLE) und existiert auf den Linux-Lanes daher immer. Unter `set -e` bricht ein
        // Exit != 0 den Batch ehrlich ab -- HART in BEIDEN Profilen, auch smoke (§66-N2 "beide hart").
        s += "      echo \"== [PMC-PREFLIGHT] lane=" + host + " ts=$(date -u +%FT%TZ) ==\"\n";
        s += "      cmake --build build --target m3v2_pmc_smoke linux_perf_pmc_smoke\n";
        // W0b-3 (2026-08-08), per T-6 gefundene Schwesterstelle DERSELBEN Klasse (ein Marker behauptet mehr,
        // als er weiss): `ctest -L pmc` gibt bei NULL passenden Tests rc=0 aus ("No tests were found!!!", am
        // Objekt gemessen) -- `set -e` greift also nicht, und die Zeile darunter meldet ungeruehrt pmc=ok.
        // Ein Preflight, der nichts gefunden hat, ist damit von einem bestandenen nicht zu unterscheiden.
        // Das ist keine graue Theorie: Commit 7dc372c7 fand am selben Tag Registrierungen, die es lautlos
        // NICHT in die ctest-Inventur geschafft hatten. Traefe das die beiden pmc-Ziele, liefe eine
        // mehrtaegige Messung mit kaputtem perf_event_open durch -- genau der Fall, den dieser Preflight
        // verhindern soll. --no-tests=error macht den Leerlauf zum Fehler (rc=8).
        s += "      ctest --test-dir build -L pmc --no-tests=error --output-on-failure\n";
        s += "      echo \"[PMC-TESTAT] ts=$(date -u +%FT%TZ) lane=" + host + " pmc=ok\"\n";
        // Mess-Fenster = das VOLLE [0:COMDARE_GN_TOTAL) der Zelle (BYTE-GLEICH zur Vor-S4-Emission). Einmal je Batch.
        s +=
            "      export COMDARE_GOLDEN_N_RANGE=\"0:${COMDARE_GN_TOTAL:-16}\"   # volles Zell-Fenster (Voll-Messlauf: "
            "COMDARE_GN_TOTAL=131072)\n";
        // §61-MODI: der DLL-Bau laeuft PARALLEL, aber mit dem §62-B-K-Budget-Literal (ersetzt $(nproc)) -- NUR das
        // MESSEN ist 1-Thread (run_profile-Loop). Der Treiber-cmake-Bau bleibt beim Parent-CMAKE_BUILD_PARALLEL_LEVEL-Deckel.
        s += "      export COMDARE_BUILD_PARALLEL=\"" + par + "\"   # §62-B Lane-Budget " + host +
             " (DLL-Bau parallel; Messen 1-Thread, run_profile-Loop; statt nproc-Heuristik; amd RAM-gedrosselt)\n";
        // (platform-Tag) §61/§62 Plattform-Provenienz: die CSV-Spalte "platform" MUSS die MESSENDE Maschine tragen
        // (compile_time_platform_tag trennt amd/intel-x86_64 NICHT). Einmal je Batch (die Lane ist fix je Job).
        s += "      export COMDARE_PLATFORM=\"" + host +
             "@$(hostname)\"   # (platform-Tag) ISA-Lane@Maschine -> CSV-Provenienz (§61/§62 per-Maschine)\n";
        // E-04-P1: dieselbe Lane-Aussage wie im Bau-Batch. Auch der Mess-Batch faehrt einen Treiber-Bau (der
        // Fallback-Kanal emittiert dort seine Bilanz) -- ohne diese Zeile stuende dort lane=unbelegt.
        s += "      export COMDARE_LANE=\"" + host +
             "\"   # E-04-P1: Pflichtfeld lane= der Treiber-Marker (Planer sagt die Lane)\n";
        // E-20/R-15 (21.08.2026): PIN-PFLICHT-DEKLARATION im Mess-Emissionspfad -- SOLL laut sagen, IST ehrlich
        // dazu. Der Aktuator existiert (ScopedThreadPin, builder/measurement/thread_pinning.hpp, AP-13/#247),
        // ist im run_profile-Loop aber NICHT verdrahtet: pin_ist=UNGEPINNT wird deshalb MITemittiert, statt
        // einen Vollzug zu behaupten, den es nicht gibt (Verdrahtung = W3-Vorstaffel zweilanige Messung,
        // C-05-Naehe). Ungepinnt heisst auf prod1 (96/32-MiB-L3-Asymmetrie, s. lane_measure_pin_cpus):
        // NICHT reproduzierbar -- genau das sagt die Zeile woertlich. KEIN taskset-Praefix, Begruendung dort.
        if (std::string const pin = lane_measure_pin_cpus(host); !pin.empty()) {
            s += "      export COMDARE_MEASURE_PIN_CPUS=\"" + pin +
                 "\"   # E-20/R-15: L3-homogene SOLL-Menge (CCD0, 96 MiB); Konsument = Mess-Aktuator (W3)\n";
            s += "      echo \"[PIN-DEKLARATION] lane=" + host + " pin_soll=" + pin +
                 " pin_ist=UNGEPINNT grund=aktuator-unverdrahtet l3=96+32MiB-asymmetrisch "
                 "folge=ungepinnt-nicht-reproduzierbar (R-15)\"\n";
        } else {
            s += "      echo \"[PIN-DEKLARATION] lane=" + host +
                 " pin_soll=UNVERMESSEN pin_ist=UNGEPINNT grund=lane-topologie-nicht-vermessen "
                 "(R-15 ist prod1-scoped; prod2/RaptorLake-Hybridkerne unvermessen)\"\n";
        }
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
                emit_driver_log_truncate(s, "      ", "$LOGDIR/perm" + idx + "_mess.log");
                s += "      if ! COMDARE_THESIS_PROFILE=\"$COMDARE_GOLDEN_N_PROFILE\" \\\n";
                s += "           " + combo_env + build_type_env + "COMDARE_GN_OPT=\"" + opt + "\" COMDARE_GN_SIMD=\"" +
                     simd + "\" COMDARE_RUN_SOTA=0 \\\n";
                s += "           \"$DRIVER\" experiment_config \"" + measure_out + "\" \\\n";
                // #27: Detail nach <log>; die [heartbeat]-Zeilen (mess-zelle) zusaetzlich in den Trace.
                s += "           " + driver_log_redirect("$LOGDIR/perm" + idx + "_mess.log") + "; then\n";
                s += "        echo \"[FEHLER-TESTAT] ts=$(date -u +%FT%TZ) lane=" + host + " zelle=" + cell3 +
                     " phase=mess fenster=0:${COMDARE_GN_TOTAL:-16}\"; FAIL=1\n";
                // D3-5 (2026-08-08): das [MESS-TESTAT] stand bis hierher AUSSERHALB des fi und wurde damit
                // UNBEDINGT gedruckt -- auch unmittelbar nach einem [FEHLER-TESTAT] derselben Zelle. Eine
                // gescheiterte Zelle trug beide Testate, und wer die [MESS-TESTAT]-Zeilen als "gemessene
                // Zellen" zaehlt (der naheliegendste Gebrauch), zaehlte die gescheiterten mit. Der Nenner
                // war damit strukturell zu gross, und zwar genau um die Fehlerzahl -- die Kennzahl schoente
                // sich umso staerker, je mehr schiefging.
                // Ab jetzt im else-Zweig: [MESS-TESTAT] heisst "diese Zelle WURDE gemessen", nicht "wir sind
                // hier vorbeigekommen". Die beiden Testate schliessen einander aus; je Zelle steht genau eins.
                s += "      else\n";
                s += "        echo \"[MESS-TESTAT] ts=$(date -u +%FT%TZ) lane=" + host + " zelle=" + cell3 +
                     " phase=mess fenster=0:${COMDARE_GN_TOTAL:-16}\"\n";
                s += "      fi\n";
            }
        }
        // G4a P-B (§65 lokal->0, #35): der PRUNE-Schritt als LETZTER Schritt des MESS-Batches, je Perm.
        // REIHENFOLGE-BEGRUENDUNG (Manager-Entscheid D1, Abweichung vom urspruenglichen "am Bau-Ende"): Bau- und
        // Mess-Batch teilen sich die resource_group ceb-measure-<host>, laufen also nacheinander auf DERSELBEN
        // Maschine. Ein Prune zwischen Bau und Messung wuerde die eben gebauten Binaries loeschen und den Mess-Batch
        // zwingen, sie vollstaendig ueber cache_pull aus minio zurueckzuholen -- ein kompletter Netz-Rueckweg ohne
        // Gegenwert, denn die Platte ist nach dem Cleanup NICHT der Engpass. §65 meint "lokal->0 nach gesichertem
        // Bestand UND abgeschlossener Messung": der Bau-Batch behaelt die Binaries lokal, der Mess-Batch findet sie
        // vor, und erst danach wird lokal geraeumt.
        // NICHT-FATAL (D2): kein FAIL=1, `|| true`. Ein fehlgeschlagener Prune laesst die lokalen Artefakte stehen --
        // das ist der sichere Ausgang; die Messung daran scheitern zu lassen waere unverhaeltnismaessig. Sichtbar
        // bleibt er trotzdem: der Treiber gibt selbst "[PRUNE-TESTAT] verified=.. pruned=.. behalten=.. skipped=.."
        // aus (02_messung_driver/main.cpp). COMDARE_BUILD_VERSION wird bewusst NICHT gesetzt: der Remote-Praefix
        // stammt seit FIX A NICHT mehr aus dieser Variablen, sondern aus der lokalen perm.dll.version NEBEN jedem
        // Artefakt (artifact_cache.hpp::prune_key_base) -- also aus genau dem per-Perm-String, unter dem
        // push_tier_binary abgelegt hat. Der Treiber-Default dient nur noch als Rueckfall fuer Stems ohne Sidecar.
        // (Vor FIX A zeigte der Praefix auf den nackten Basis-Wert und traf nie -> pruned=0 dauerhaft; s. den
        // Folge-Commit zu f3a6e68d.)
        s += "      if [ \"${COMDARE_STORAGE_CACHE:-}\" = \"true\" ]; then\n";
        for (auto const& p : perms) {
            std::string const idx = std::to_string(p.index);
            std::string const cell3 =
                combo_legend_ + legend::system_perm(p.opt_id, p.simd_id) + legend::organ_reference();
            std::string const measure_out = "$CI_PROJECT_DIR/Code/measure_out/" + slug + "/perm" + idx;
            s += "        echo \"== [PRUNE] zelle=" + cell3 + " lane=" + host + " ts=$(date -u +%FT%TZ) ==\"\n";
            s += "        COMDARE_PRUNE_ONLY=true \"$DRIVER\" experiment_config \"" + measure_out + "\" || true\n";
        }
        s += "      fi\n";
        s += "      exit $FAIL   # Mess-Fehler je Zelle sichtbar ([FEHLER-TESTAT] + Log); der Batch misst durch\n";
        s += "  artifacts:\n";
        s += "    when: always\n";
        s += "    paths:\n";
        s += "      - Code/measure_out/" + slug + "/logs/\n";
        // G4a P-C: die Mess-CSV selbst als Artefakt sichern. Bisher reisten nur die Logs mit -- die eigentlichen
        // Messdaten haetten einen Runner-Verlust nicht ueberlebt (Messdaten-nie-loeschen-Doktrin).
        s += "      - Code/measure_out/" + slug + "/**/*.csv\n";
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
// G4a (7) / Integrations-Doc-I2-Restpunkt: die PLANER-VORRESERVIERUNG (BatchTyp::planer_block, 30min pro-forma, OHNE
// ETA -- bestandslog_document.hpp:83). Sie meldet dem Lager, dass DIESER Planer gleich eine CEB-Compile-Strecke
// anstoesst, damit ein zweiter Planer auf einer anderen Maschine nicht dieselbe Strecke doppelt reserviert.
//
// SCHNITT (bewusst, siehe Meldung): der Director ist ein REINER TEXT-EMITTER -- er haelt keinen Transport, macht kein
// IO, und seine construct()-Walks sind byte-determinismus-getestet. Eine Schreib-Operation hier hinein zu legen wuerde
// genau diese Reinheit zerstoeren und construct() umgebungsabhaengig machen. Deshalb liefert der ce den WERT der
// Reservierung (rein, deterministisch, hier testbar), und der Host fuehrt den Effekt aus -- dort, wo das IO ohnehin
// schon lebt (der Treiber haelt bereits ArtifactCache::from_env() und den Bestandslog-Transport). Das ist dieselbe
// Dependency-Inversion wie bei bestand_transport/cache_pull, nur ohne eine tote std::function im Emitter.
//
// Zeitstempel kommen wie bei make_pro_forma_reservation VOM AUFRUFER ("Zeit-Formatierung ist nicht Sache dieser
// Zustandsmaschine", reservation_lifecycle.hpp:54) -- damit ist die Funktion uhrfrei und exakt testbar.
// slice_begin/slice_count sind 0/0: ein planer_block reserviert KEINE Datenscheibe, er blockiert die Strecke als
// Ganzes (im Gegensatz zu BatchTyp::tier, der das 4096er-Korn traegt).
//
// G4b-2/E4 (2026-07-26): der WERT-Aufbau ist nach bestandslog/planer_block_value.hpp gewandert; diese Funktion ist
// jetzt eine reine DELEGATION und behaelt genau ihre bisherige Signatur samt id-Bildung `owner_uuid + "/" + seq`.
// Grund fuer den Umzug: der Emissions-Pfad und die Test-TU mit dem builder-Include-Satz brauchen den Wert, ohne
// diesen katalog-schweren Planer-Header zu ziehen. Kein Verhaltens-Unterschied, kein toter Code -- die gepinnten
// Tests (test_experiment_plan_director.cpp:1502/:1517-1519) laufen unveraendert gegen diese Huelle.
[[nodiscard]] inline ::comdare::cache_engine::builder::bestandslog::BatchReservierung
make_planer_block_reservation(std::string owner_uuid, std::size_t seq, std::string maschine, unsigned threads,
                              std::string reserviert_utc, std::string pro_forma_bis_utc) {
    namespace bl = ::comdare::cache_engine::builder::bestandslog;
    // ceb_legende/ceb_key_sha512 bleiben LEER (E3: optional, "nicht gemeldet"): diese Huelle kennt weder die
    // Mess-Achsen-Klammer der Emission noch den CEB-Fingerprint -- sie bildet nur den Wert. Der Emissions-Pfad
    // (profile_run_facade.cpp) belegt beide. Leer heisst, der Emitter schreibt die Attribute gar nicht -- das
    // Dokument bleibt fuer einen v2-Leser unveraendert, und die gepinnten Tests hier bleiben wertgleich.
    return bl::make_planer_block_reservation_value(
        std::move(owner_uuid) + "/" + std::to_string(seq), std::move(maschine), threads, /*ceb_legende=*/std::string{},
        /*ceb_key_sha512=*/std::string{}, std::move(reserviert_utc), std::move(pro_forma_bis_utc));
}

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
        out_ += "# Ledger §42.b + §62-B-Batch (CEB-Rolle 'tier cmake'): je Host-Lane EIN Build+Pruef-Aggregat\n";
        out_ += "# comdare_tier_batch_<host> (per-Perm Provision + S3-Pruef, SCHARF) + EIN Mess-Target\n";
        out_ += "# comdare_tier_measure_<host> (SCHARF, misst). Bare-Metal:\n";
        out_ += "#   cmake --build <dir> --target comdare_tier_batch_amd   # baut+prueft die amd-Lane-DLLs\n";
        out_ += "# CI-DUAL (Owner 14.08.): Treiber vorgebaut (COMDARE_PLAN_DRIVER); der aeussere Configure "
                "pinnt gcc-15/g++-15,\n";
        out_ += "# der clang-22-Zwilling (build-clang) ist Bau+Test-Gate je Stufe; Mess-Targets bleiben "
                "single-compiler (N2).\n";
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
    // E-04-P1 (Section 61-Dual-Weg-Symmetrie): JEDER der vier Treiber-COMMANDs traegt COMDARE_LANE -- der
    // bare-metal-Weg
    // liefert dieselbe Marker-Lane wie die CI-Emission. Fehlte sie hier, stuende der lokale Dual-Weg-Beweis mit
    // lane=unbelegt gegen die CI-Zeilen und die beiden Wege waeren nicht mehr vergleichbar. fenster_gesamt/offen=
    // haben hier bewusst KEIN Gegenstueck: dieser Weg faehrt EIN Fenster (${COMDARE_PLAN_RANGE}), keine
    // Scheiben-Schleife -- eine erfundene Fenster-Zahl waere eine Aussage ueber etwas, das es hier nicht gibt.
    void emit_batch_targets(std::string const& host, std::vector<PlanPerm> const& perms) {
        std::string const slug    = legend::cmake_slug(combo_legend_);
        std::string const stemdir = "${CMAKE_CURRENT_BINARY_DIR}/tier";
        // S6-P1b Env-Bruecke (d): ab N>1 COMDARE_MEASUREMENT_COMBO im -E env-Block.
        // [KORREKTUR 12.08.2026] wie emit_batch_build_job (dort die ausfuehrliche Herleitung): LEER meint NUR die
        // emittierte ENV-ZEILE im -E-Block, NICHT den Mess-Stempel. Die fehlende Env loest konsumentenseitig zur
        // VOLLMENGE auf (mess_achsen_naht.hpp:317 -> anatomy_version_stamp.hpp:378). byte-stabil ist die
        // [all]-LANE, nicht "leer"; was die emittierte DLL traegt, entscheidet der Source-Gen-Vorrang (INC-G6),
        // nicht diese Zeile. NUR KOMMENTAR, kein Code.
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
            out_ += "            \"COMDARE_LANE=" + host + "\"\n";
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
            out_ += "            \"COMDARE_LANE=" + host + "\"\n";
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
                out_ += "            \"COMDARE_LANE=" + host + "\"\n";
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
            out_ += "            \"COMDARE_LANE=" + host + "\"\n";
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

    /// I-PMC-2 (Owner 10.08.2026): den auf DIESEM Host gemessenen PMC-Befund setzen. Getrennt vom
    /// Konstruktor und ausdruecklich NICHT im Konstruktor GEMESSEN -- ein Konstruktor, der still
    /// perf_event_open ruft, machte jeden Director-Test host-abhaengig und die Erhebung unbeobachtbar.
    /// Der Planer-Einstieg (apps/experiment_planner/main.cpp) fragt EINMAL und setzt hier; die Tests
    /// setzen erzwungene Befunde und pruefen damit alle drei Lagen auf jeder Maschine.
    /// NICHT GESETZT == Unbrauchbar == "ohne PMC" (fail-closed).
    void                               set_pmc_befund(PmcHostBefund befund) { pmc_befund_ = std::move(befund); }
    [[nodiscard]] PmcHostBefund const& pmc_befund() const noexcept { return pmc_befund_; }

    /// Thesis-Kanal: opt x simd x profile_sweep_passes(tp, ""). KEIN Bau; die Selektions-Pass-Liste ist die
    /// deterministische #26/GO-5-Enumeration (Basis-Pass zuerst + je <axis_sweep> ein Pass in Dokument-Reihenfolge).
    void construct(cx::ThesisProfile const& tp, IPlanBuilder& b, std::string const& combo_selector = {},
                   std::vector<std::string> const& methodik_run_methodology = {},
                   std::string const&              profile_basename         = {}) const {
        std::vector<std::string> const opt_perms  = opt_perms_of(tp.compiler.opt_levels);
        std::vector<std::string> const simd_perms = simd_perms_of(tp.external_utils.simd_options);
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
        // Plan-Kopf v1.1: Substanz des THESIS-Profils = permutierte Achsen + Summe ihrer <value>-Eintraege
        // (dieselbe Zaehlung, die der Validat als "N Achsen, M Werte" meldet).
        PlanProfileSubstance thesis_substance;
        thesis_substance.axes = tp.permute_axes.size();
        for (auto const& ax : tp.permute_axes) thesis_substance.values += ax.values.size();
        walk_perms_("thesis", tp.id, profile_basename, thesis_substance, combos, opt_perms, simd_perms, resolver,
                    build_semantic, b, [&](IPlanBuilder& bb) {
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
        std::vector<std::string> const                    opt_perms   = opt_perms_of(ep.compiler.opt_levels);
        std::vector<std::string> const                    simd_perms  = simd_perms_of(ep.external_utils.simd_options);
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
        // Plan-Kopf v1.1: Substanz des EXPERIMENT-Profils = axes_default_lookup-Eintraege + Summe ihrer
        // allowed_variants (die Default-/Limit-Schicht ist hier die Achsen-Deklaration des Anwenders).
        PlanProfileSubstance experiment_substance;
        experiment_substance.axes = ep.axes_default_lookup.size();
        for (auto const& ax : ep.axes_default_lookup) experiment_substance.values += ax.allowed_variants.size();
        walk_perms_("experiment", ep.id, profile_basename, experiment_substance, combos, opt_perms, simd_perms,
                    resolver, build_semantic, b, [&](IPlanBuilder& bb) {
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
    // byte-identisch zum Vor-S5-Verhalten (Default-Release). measurement_on spiegelt, ob das Profil ueberhaupt misst
    // (leer ODER measure deklariert => ja; build/compare/release => nein, Bau- bzw. Referenz-Laeufe;
    // A-05/V-12: debug ist ausgebaut).
    // ES WIRD HEUTE VON NIEMANDEM GELESEN: hier stand bis zum 07.08.2026 der Satz "Das Feld wird gelesen" -- er war
    // falsch (0 Leser, nachgezaehlt ueber alle header_.build_semantic-Zugriffe, s. Struct-Doku). Der Konsument ist
    // der per-Methodik-Fanout {build,measure,compare,release} zu N Mess-Strecken, und der ist S6.
    [[nodiscard]] static PlanBuildSemantic
    build_semantic_of_run_methodology(std::vector<std::string> const& run_methodology) {
        // §61-STUFEN/(j2): GENAU EIN aktiver Modus je Profil (validate erzwingt exactly-one, j1). Die Build-Semantik
        // kommt aus DIESEM Modus -- 4er-Welt (A-05/V-12): Build={Release,misst NICHT,parallel},
        // Measure={Release,misst,1-Thread}, Compare/Release={Release,misst NICHT,parallel}; der fruehere
        // Debug={Debug,misst,parallel} ist ausgebaut. NICHT mehr fix measure (Vor-(j2)-Lesart). Leer => Default
        // measure (Release, 1-Thread). Die
        // Methodik-Quelle ist das PROFIL, NICHT die Env (COMDARE_MEASURE_PROFILE bleibt NUR der rules-Auto-Run-Trigger).
        // R5: >1 ist ein Kontraktbruch (validate gated ihn VOR dem Bau) -- HART statt still front(), damit ein
        // 2-Modi-Profil nie zufaellig eine Modus-Semantik waehlt (symmetrisch zu run_methodology_for_ids).
        if (run_methodology.size() > 1)
            throw std::invalid_argument(
                "build_semantic_of_run_methodology: " + std::to_string(run_methodology.size()) +
                " Modi deklariert -- GENAU EINER erlaubt (exactly-one je Call, Ledger 61-STUFEN).");
        // Welle B/1 (2026-08-07) FAIL-CLOSED + SINGLE-SOURCE: die Token-Aufloesung (leer => measure-Default,
        // unbekannt => HARTER Fehler) steht ab jetzt EINMAL in der Registry (run_methodology_for_ids). Hier stand
        // bis heute eine ZWEITE, eigene Suchschleife, die ein unbekanntes Token STILL auf measure warf -- ein
        // Tippfehler im Profil emittierte damit eine vollstaendige Mess-Strecke, die niemand angefordert hat.
        // Die >1-Wache bleibt HIER (eigene, den Planer nennende Meldung); die Registry wiederholt sie nur.
        cm::WorkModeInfo const& m = cm::run_methodology_for_ids(run_methodology);
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
                     PlanProfileSubstance const& substance, std::vector<PlanMeasurementCombo> const& combos,
                     std::vector<std::string> const& opt_perms, std::vector<std::string> const& simd_perms,
                     tlz::ResolverReport const& resolver, PlanBuildSemantic const& build_semantic, IPlanBuilder& b,
                     StepEmitter&& emit_steps) const {
        PlanHeader header;
        header.source_kind             = std::string{source_kind};
        header.profile_id              = profile_id;
        header.profile_basename        = profile_basename; // S2-NACHT: Basename des aktiven Profils
        header.perm_count              = opt_perms.size() * simd_perms.size();
        header.measurement_combo_count = combos.size();
        header.profile_axis_count      = substance.axes;   // Plan-Kopf v1.1
        header.profile_value_count     = substance.values; // Plan-Kopf v1.1
        header.registries              = trio_;
        header.resolver                = resolver; // S3 P-RESOLVER: Organ-Position-Report im Plan-Kopf (Annotation)
        header.build_semantic          = build_semantic; // S5-P1: measure-Methodik-Build-/Mess-Semantik (Tier-Emitter)
        // I-PMC-2: EINE Erhebung je Planer-Lauf, in den Plan-Kopf. Alle vier Emissionsstellen lesen von hier
        // -- sie koennen damit strukturell nicht auseinanderlaufen.
        header.pmc_befund = pmc_befund_;
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
                    // Lane F R3 (O-8 Schritt 10): auch die Planer-ANNOTATION kommt aus der EINEN
                    // Suffix-Quelle. Sie traegt bewusst KEIN +cxx (der Planer kennt den Compiler-Tag an
                    // dieser Stelle nicht) -- ein leeres Glied erzeugt kein Segment, die Ordnung der
                    // uebrigen bleibt trotzdem die bindende.
                    {
                        ::comdare::cache_engine::profile_facade::SystemVersionSuffixParts plan_parts;
                        plan_parts.opt = opt_id;
                        if (simd_id != std::string{cm::SimdNoExtOption::simd_id()}) plan_parts.simd = simd_id;
                        perm.build_version_suffix =
                            ::comdare::cache_engine::profile_facade::compose_system_version_suffix(plan_parts);
                    }
                    b.begin_perm(perm);
                    emit_steps(b);
                    b.end_perm(perm);
                }
            }
            b.end_measurement_combo(combo);
        }
        b.end_plan(header);
    }

    // I-PMC-2: der gemessene Host-Befund. Default = Unbrauchbar/nicht erhoben (fail-closed).
    PmcHostBefund              pmc_befund_;
    PlanRegistryTrioAnnotation trio_; // leer/loaded=false, wenn ohne Registry-Trio konstruiert
    // S3 P-RESOLVER: das VOLLE RegistryTrio (Achsen-Namen-Mengen) fuer die Organ-Position-Aufloesung. nullopt =>
    // der Resolver ist INERT (resolved=false) -- Default-/Annotation-Konstruktor-Pfad, Vor-S3-Verhalten byte-identisch.
    std::optional<tlz::RegistryTrio> full_trio_;
};

} // namespace comdare::cache_engine::planner
