#pragma once
// STRANG A KORRIGIERT — Increment 6 / S7c (2026-06-19, profil-getrieben). profile_run_entry: die EINE
// deklarative CEB-Eintritts-API run_profile(...) — der "duenne profile_runner-Einstieg, den der
// messung_driver (Diplomarbeit) triggert" (Doc 10 §2.2 "messung_driver ruft CacheEngineBuilder").
//
// Plan: docs/sessions/20260618-STRANG-A-KORRIGIERT-PROFIL-GETRIEBEN-PLAN.md (S7). Diese Funktion vereinigt
// die drei S7-Bausteine zu EINEM Aufruf:
//   • S7a SourceGen-Merge  : source_catalog (Basis-320, "search_algo=.../...") UNION sota_catalog
//                            ("sota_tier=sota::S::name") → make_union_source_gen (disjunkte Namensraeume).
//   • S7b Multi-Pass       : aus EINEM Profil fuehrt run_profile MEHRERE Selektions-Paesse:
//                            (1) BASIS-Pass  = permute_axes → build_profile_basis_levels → 320 (bzw. cap),
//                            (1b) je deklariertem <axis_sweep> EIN Achsen-Sweep-Pass (#26/GO-5 Multi-Sweep,
//                                profile_sweep_passes; explizites args.sweep_axis behaelt Einzel-Pass-Vorrang),
//                            (2) je <sota_series>-Eintrag = EIN SOTA-Lebewesen-Pass (einwertiger "sota_tier"-
//                                Baum), getaggt mit series A/B/C. Alle Zeilen → EINE CSV (Header genau EINMAL).
//   • S7c Eintritts-API    : run_profile(RunProfileArgs&) — run_lazy_150.main reduziert sich auf Pfad+Output;
//     (run_lazy_150 geloescht 2026-07-11; Host/Emitter heute Code/02_messung_driver, E4-XML)
//                            die WHAT-Konfiguration kommt komplett aus dem Profil.
//
// Resume/binary_id-Stabilitaet bleibt (jeder Pass ruft run_lazy_static_then_dynamic → .version-Sidecar +
// per-Binary-result.csv-Stamp). Lazy-Compile (1 DLL = 1 TU) bleibt: die Vereinigung waehlt nur die Quelle.
//
// ⚠️ Katalog-/Umbrella-schwer (source_catalog.hpp zieht den all_axes_umbrella) → gehoert in die HARNESS-/Test-
//    .cpp (run_lazy_150.cpp / test_*), NICHT in den engine-agnostischen Treiber-Header. C++23, header-only.
//    (run_lazy_150.cpp geloescht 2026-07-11; Host/Emitter heute Code/02_messung_driver, E4-XML)

#include "ergebnis_mappe_naht.hpp" // A9-S5: Format-Wahl + die Mappe NEBEN der offiziellen CSV (additiv)
#include <cache_engine/measurement/machine_identity.hpp> // A9-S5: live_hostname() fuers INFO-Blatt
#include "build_type_stamp.hpp"         // (i) §61-STUFEN: build_type_version_suffix (+bt=Debug bei COMDARE_BUILD_TYPE)
#include "toolchain_stamp_naht.hpp"     // T2-B: PermToolchainAchsen/-GliedWert + der EINE Glied-[5]-Renderer
#include "generated_source_catalog.hpp" // generated_make_catalog_source_gen (Basis-320-Quelle)
#include "h2_score_akte.hpp"            // GO-5 Fork 7: parse_h2_score_akte / h2_score_for (CSV-Endspalte)
#include "lazy_adhoc_source_gen.hpp"    // INC-G6 (33/34): make_lazy_adhoc_source_gen (lazy golden-N-Fallback-Quelle)
#include "source_catalog.hpp"           // axis_sweep_source_map / axis_sweep_levels (Sweep-Quellen)
#include "sota_catalog.hpp"             // build_sota_passes / build_sota_view_source_map / kSotaTierAxis (S6/S7b)
#include "profile_runner.hpp" // load_thesis_profile / build_profile_basis_levels / profile_select / make_union_source_gen
#include "system_axes_entscheidung.hpp" // T-1: die EINE <system_axes>-Entscheidung + ihre Zwei-Parse-Wache
#include "gn_cell_filter.hpp" // W5-C+ (§36.1): gn_cell_opt_allowed / gn_cell_simd_allowed / gn_walk_cells (Single-Source)
#include "planner/plan_legend.hpp" // E-04-P1: die EINE Legenden-Quelle -- Treiber-Marker und Shell-Testate byte-gleich

#include <builder/experiment_tree/cache_engine_builder_iterator.hpp> // run_lazy_static_then_dynamic / lazy_csv_header / LazyRunConfig
#include <builder/experiment_tree/coverage_selection.hpp> // select_explicit
#include <builder/experiment_tree/selection_filter_chain.hpp> // A6/§50-CoR: resolve_selection (Dead-Code-Filter-Einhaengung)
#include <builder/experiment_tree/axis_variant_version_table.hpp> // Resthygiene-2: compose_algo_signature / build_axis_variant_version_table
#include <builder/experiment_tree/organ_fingerprint.hpp> // Resthygiene-2: organ_fingerprint_preimage_from_pairs (sort+concat)
#include <builder/build_orchestrator/system_ram.hpp>  // make_system_free_ram_fn
#include <builder/driver_build_variant_signature.hpp> // A7-B: driver_build_variant_signature (Mengen-Sig des Treibers)

#include <system_axes/optimization_level_sub_axis.hpp>  // GN-3: OptO*SubAxis (opt_level-id -> -O<n>)
#include <cache_engine/measurement/simd_build_gate.hpp> // C-3c: active_machine_signature (deklarierte Klasse)

#include "system_version_suffix.hpp"               // Lane F R3: die EINE Suffix-Quelle (Segment-Ordnung deklarativ)
#include "system_cell_values_naht.hpp"             // W10-C4: compose_system_cell_values (die EINE Zellwert-Wertform)
#include <system_axes/simd_sub_axis.hpp>           // GN-3/F-SIMD: simd-Unter-Achse (simd_id -> -march)
#include <cache_engine/measurement/axis_error.hpp> // GN-3: CompilerCompilerErrorClass (D1-Log der optxsimd-Naht)

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional> // GN-3: std::function (per-Perm-CompileFn-Fabrik der optxsimd-Naht)
#include <iostream>
#include <map>
#include <algorithm> // Resthygiene-2: std::sort/std::min im Chunk-Organ-Fingerprint
#include <memory>
#include <optional>
#include <utility> // Resthygiene-2: std::pair (stem, algo_sig)
#include <set>
#include <string>
#include <vector>

namespace comdare::cache_engine::thesis_lazy {

namespace ex = ::comdare::cache_engine::builder::experiment;
namespace wd = ::comdare::cache_engine::builder::workload_driver;
// E-04-P1: die EINE Legenden-Quelle des dreistufigen Namensschemas (plan_legend) -- der Treiber rendert seine
// Marker-Zelle damit BYTE-GLEICH zur emittierten Shell-Testat-Zelle (gemeinsamer Aggregator-Key).
namespace pl = ::comdare::cache_engine::planner::legend;

// ── Eingabe der EINEN CEB-Eintritts-API. ALLES, was NICHT aus dem Profil kommt (Pfade/Toolchain/Output) ──
//    Die WHAT-Konfiguration (Lebewesen/Achsen/Sweeps/SOTA/Working-Set/run_options) liest run_profile selbst
//    aus dem Profil (tp). cap/resume/platform/build_version werden aus <run_options> vorbelegt; argv/env darf
//    weiterhin uebersteuern (der Treiber-Host setzt die Felder hier vor dem Aufruf).
struct RunProfileArgs {
    std::filesystem::path profile_path; // das comdare_thesis_profile (m3v2_study.profile.xml)
    std::filesystem::path out_csv;      // Ziel-CSV (Header genau EINMAL, alle Paesse darunter)
    std::filesystem::path src_dir;      // perm_<id>.cpp-Ausgabe (per-Binary-Subdir-Basis)
    std::filesystem::path dll_dir;      // perm_<id>.dll-Ausgabe (per-Binary-Subdir-Basis)
    ex::CompileFn         compile;      // injizierter Compiler-Aufruf (cl @rsp) — wie BuildOrchestrator
    // GN-3 (§33, 2026-07-19): per-Permutation-CompileFn-Fabrik (System-Achsen opt_level x simd), SPIEGEL der
    // RunExperimentArgs::compile_for_perm-Naht. Der Facade-Planer liefert sie (kennt include_dirs/defines/cxx/
    // link_libs/fno_gnu_unique); run_profile permutiert optxsimd aus dem GEPARSTEN Profil (tp.compiler.opt_levels /
    // tp.external_utils.simd_options) SELBST und ruft die Fabrik je Perm mit den aufgeloesten Flags. Leer =>
    // Fallback auf `compile` (Einzel-Pfad, byte-identisch zum Vor-Wiring-Verhalten).
    ///
    /// W10-C4: die Fabrik nimmt zusaetzlich die System-ZELLWERTE dieser Zelle entgegen und haengt sie als
    /// -DCOMDARE_SYSTEM_CELL_VALUES an die rsp-Zeile. Der Typ ist der BENANNTE abi::SystemCellValues (K-1-Muster,
    /// Praezedenz OverlayHash) und ausdruecklich kein dritter nackter string -- die Signatur truege sonst drei
    /// gleichartige Strings, und ein vertauschter Slot kompilierte klaglos. Leere Werte == Identitaet.
    ///
    /// T2-B (Codex [CX-B1], KRITISCH): die Fabrik nimmt ZUSAETZLICH den fertig gerenderten per-Perm-Wert
    /// des Preimage-Glieds [5] entgegen. Er ist der Grund, warum diese Signatur ueberhaupt waechst: bis
    /// dahin trug das Glied nur die run-konstanten Felder (cxx/ceb/bt), womit O2 und O3 DERSELBEN Zelle
    /// denselben Fingerprint bekamen -- also einen falschen Skip im Skip-Gate. Der Typ ist wieder ein
    /// BENANNTER Traeger (K-1), diesmal mit eigenem Speicher, weil die Fabrik ihn ueber die gesamte
    /// Permutation haelt. LEER == kein Define == byte-identisch zum Vor-T2-B-Bau.
    std::function<ex::CompileFn(std::string const& opt_flag, std::string const& march_flag,
                                ::comdare::cache_engine::abi::SystemCellValues                         cell_values,
                                ::comdare::cache_engine::profile_facade::PermToolchainGliedWert const& toolchain_glied)>
        compile_for_perm;
    // T-1 (2026-08-09): DIE MITGEFUEHRTE <system_axes>-ENTSCHEIDUNG -- der Zwilling von compile_for_perm.
    // Die Facade entscheidet aus IHREM Parse, ob compile_for_perm belegt wird; run_profile entscheidet aus
    // SEINEM Parse derselben Datei, ob die Perm-Schleife laeuft. Das sind zwei Parses zu zwei Zeitpunkten.
    // Deshalb reist die Antwort ab jetzt als Wert mit und wird unten gegen den eigenen Parse geprueft
    // (system_achsen_entscheidung_haelt). NichtGetragen (Default) = Direkt-Aufrufer ohne Facade => die
    // Wache haelt immer => byte-neutral fuer jeden Bestands-Aufrufer.
    ::comdare::cache_engine::profile_facade::SystemAchsenEntscheidung system_achsen =
        ::comdare::cache_engine::profile_facade::SystemAchsenEntscheidung::NichtGetragen;
    std::string compiler_tag; // GN-3: +cxx=-Provenienz im per-Perm-build_version (NIE binary_id)
    // W10-C4 (Bauplan-Dossier 20260803, Sektion 1): die beiden LAUF-KONSTANTEN System-Zellen dieses Baus. Die
    // FACADE loest sie auf (sie kennt Profil-Deklaration und Bau-Plattform), die Perm-Schleife ergaenzt je Zelle
    // nur ihre eigene Schleifen-Variable simd_id -- exakt die WAS/WIE-Trennung von compiler_tag/compile_for_perm.
    // BEIDE LEER (Default) => keine Zellwerte => kein Define => byte-identischer Bau (der golden-neutrale Pfad
    // fuer jeden Host, der die Naht nicht verdrahtet).
    std::string system_cell_target_isa;       // Ziel-ISA-Zelle: "x86_64"/"aarch64"/"na"
    std::string system_cell_operating_system; // OS-FAMILIEN-Zelle: "linux"/"windows"/"macos"/"na"
    // W5-C+ (§36.1 Zellen-Locking, 2026-07-19): optionale GN-Zellen-Filter der optxsimd-Delegations-Naht. Die
    // CI-Matrix weist jeder Cluster-Zelle GENAU EINE System-Perm zu (COMDARE_GN_OPT/COMDARE_GN_SIMD). Sind diese
    // gesetzt, baut der Walk NUR die matchende (opt,simd)-Zelle; alle anderen Perms werden mit Log-Zeile
    // uebersprungen. Leer (Default) = kein Filter = heutiges Verhalten (alle Profil-Perms) => byte-neutral. Werte
    // matchen dieselben Bezeichner wie die Profil-System-Achsen (O2/O3 bzw. no_extension/avx2/avx512).
    std::string gn_cell_opt;  // leer = kein opt-Zellen-Filter
    std::string gn_cell_simd; // leer = kein simd-Zellen-Filter
    // smoke=>debug-Entkopplung (2026-07-22): optionaler METHODIK-Override (run_methodology aus dem METHODIK-Profil
    // COMDARE_PLAN_METHODIK_PROFILE, facade-resolved + exactly-one-validiert). Nicht-leer => resolve_measure_
    // parallelism nutzt DIESE Methodik (z.B. debug=parallel) statt tp.run_methodology -- das Katalog-Profil bleibt der
    // Bau-/Mess-Loop-Input. Leer (Default) => aus tp.run_methodology => byte-neutral zum Vor-Entkopplungs-Verhalten.
    std::vector<std::string> methodik_run_methodology;
    ex::AlgoSigFn            algo_sig;         // Bauplan §7: spec.axes → algo_sig (perm.algos); leer = Organ-Gate aus
    ex::CachePushFn          cache_push;       // Storage #51: perm.dll(+.version) -> Objekt-Store (B); leer = No-Op
    ex::CachePullFn          cache_pull;       // S2 (#46a): BATCH-Warm-Cache-Hydrierung VOR dem Bau; leer = No-Op
    ex::MeasurementSinkFn    measurement_sink; // Storage #51: Mess-Datei -> measure-drop additiv (C); leer = No-Op
    // G4b-1 (#46b I1, Host-Verdrahtung; Muster EXAKT wie die drei Felder darueber): die mittlere Schicht der
    // dreischichtigen Bestandslog-Naht. ProfileRunArgs (Fassade) -> DIESE Felder -> LazyRunConfig::bestand_*
    // (cache_engine_builder_iterator.hpp:215-226). Der Typ von bestand_key_of ist EXAKT der Iterator-Feldtyp
    // (:216) und ausdruecklich NICHT ex::FingerprintFn (build_orchestrator.hpp:174, string->string) -- die beiden
    // Namen sind benachbart und bedeuten Verschiedenes (AUF-A4). Der reale Provider ist
    // bestandslog::make_fingerprint_key_fn (fingerprint_key_source.hpp), den der Host injiziert.
    // KEIN Gate auf dieser Ebene: das harte Doppel-Gate sitzt beim Host (AUF-B3 -- COMDARE_BESTANDSLOG=="true" UND
    // minio_enabled() UND owner-Identitaet, s. profile_run_facade.hpp). Alle leer (Default) =>
    // bestandslog_active (:927-929) false => keine Registrierung/Dedup, Bau bleibt auf provision_all => byte-neutral.
    ex::BestandTransport                                                    bestand_transport;
    std::function<std::optional<std::string>(std::filesystem::path const&)> bestand_key_of;
    std::string                                                             bestand_doc_key;
    std::string                                                             bestand_owner_uuid;
    std::string                                                             bestand_maschine;
    // LAG-P2 (2026-08-09): die mittlere Schicht des MESSWERT-Genus. ProfileRunArgs::mess_bestand_*
    // (Fassade) -> DIESE Felder -> LazyRunConfig::mess_bestand_* (cache_engine_builder_iterator.hpp).
    // Die Typen sind EXAKT die Iterator-Feldtypen -- test_lagp2_messwert_genus haelt das compile-hart
    // fest (static_assert ueber std::is_same_v), weil ein abweichender Typ hier die AUF-A4-Falle
    // waere. KEIN Gate auf dieser Ebene; alle leer (Default) => mess_bestandslog_active false =>
    // keine Messwert-Registrierung => byte-neutral.
    std::function<std::optional<std::string>(std::filesystem::path const&)> mess_bestand_key_of;
    std::string                                                             mess_bestand_doc_key;
    std::string                                                             mess_bestand_versions;
    // T2-A/F4 (Owner-KERN Zaehler-Resume): die Plan-Ablage -- das SECHSTE Glied derselben dreischichtigen
    // Naht, im Muster der fuenf Felder darueber. ProfileRunArgs::batch_plan_datei (Fassade) -> DIESES Feld ->
    // LazyRunConfig::batch_plan_datei (cache_engine_builder_iterator.hpp:243). KEIN Gate auf dieser Ebene.
    // Leer (Default) => PlanPersistenz::aktiv()==false => keine Ablage, kein Plan-Resume => byte-neutral.
    //
    // EINE ABLAGE JE LAUF, NICHT JE PASS -- und das ist eine GRENZE, keine Zusage: make_cfg legt denselben
    // Pfad in JEDE Pass-cfg. Der Plan-Stempel (slice_plan_stamp: start/indizes/korn) haengt aber an der
    // Selektion DES PASSES. Ein Lauf mit mehreren Selektions-Paessen schreibt die Ablage deshalb mehrfach
    // hintereinander um; am Ende steht der Plan des LETZTEN Passes darin. Ein Folgelauf findet fuer seinen
    // Basis-Pass einen fremden Stempel vor -> read_phasen_zaehler ist fail-closed -> KEIN Resume-Anspruch,
    // voller Neubau. Verloren geht nur der Anspruch, nie ein Messwert und nie eine gebaute Binary;
    // ueberschrieben wird nur die Plan-Datei, nie ein Lauf-Artefakt.
    //
    // WIE OFT DAS GESCHIEHT -- PRAEZISE, weil "je <axis_sweep> ein Pass" die Groesse UNTERSCHAETZT. Im
    // BAU-Modus (provision_only, der einzige, in dem der Plan-Resume ueberhaupt greift: planer_driven_active)
    // multiplizieren sich DREI Schleifen um make_cfg herum, nicht eine:
    //   (1) die opt x simd-PERM-Schleife (GN-3) UM run_all_passes -- |opt_levels| x |simd_options|;
    //   (2) die SELEKTIONS-Paesse (profile_sweep_passes): Basis-Pass + je deklariertem <axis_sweep> einer;
    //   (3) die SOTA-REIHEN-Paesse (je <sota_series>-Eintrag einer), sofern run_sota_series.
    // Die vierte denkbare Schleife -- der <working_set_sweep> je Pass -- multipliziert hier NICHT: Task #31
    // kollabiert n_sweep im provision_only-Lauf auf genau EINEN Wert (die Tier-Binary ist N-unabhaengig).
    // Am all_axes_golden.profile.xml sind das 2 opt x 2 simd = 4 Perms mal (1 Basis + 17 <axis_sweep> +
    // 21 <sota_series>) = bis zu 156 Ueberschreibungen derselben Datei je Lauf (weniger, wo ein Pass wegen
    // pass_seen_ids ohne Selektion bleibt und vor make_cfg zurueckkehrt).
    //
    // TRAGFAEHIG ist der Plan-Resume damit heute in Laeufen mit GENAU EINEM Selektions-Pass, und zwar in
    // ALLEN DREI Dimensionen zugleich: ein Profil ohne <axis_sweeps> (oder explizites sweep_axis), mit
    // COMDARE_RUN_SOTA=0, und mit genau einer opt x simd-Perm. Ob die Ablage je Pass aufgefaechert werden
    // soll -- der Owner-KERN spricht von EINEM "Batch-Plan (Reihenfolge+Faecher)" je Lauf --, ist eine
    // OFFENE OWNER-ENTSCHEIDUNG und bewusst NICHT hier vorweggenommen. Der Grund, sie nicht nebenbei zu
    // treffen: ein eindeutiger Ablage-Name braucht als Quelle genau die Groesse, die auch den Plan-Stempel
    // bestimmt (die Selektion DES PASSES) -- diese Groesse kennt aber erst der Iterator (er kappt die
    // Indizes auf max_binaries). Sie hier ein zweites Mal abzuleiten waere die Format-Drift, gegen die die
    // ganze F4-Mechanik gebaut ist; sie dort abzuleiten macht aus diesem Feld einen PRAEFIX statt eines
    // Pfades -- eine Vertrags-Aenderung an genau dem Arg, das der Host (super-Facade/CEB) noch belegen muss.
    std::filesystem::path batch_plan_datei;
    // W11 (§43.c): der BAU-Modus Teil-Marker-Sink (nach je chunk_part_size gepushten DLLs) + N. Leer/0 = keine
    // Teil-Marker (byte-neutral). Der Host belegt sie aus dem ArtifactCache + COMDARE_GN_PART_SIZE (Default 1024).
    ex::PartialMarkerFn partial_marker_sink;
    std::size_t         chunk_part_size = 0;
    ex::ProgressSinkFn
        progress_sink; // Welle 5 (E-W5-2): §38-Fortschritts-Rueck-Kanal (No-Op-Default); make_cfg reicht ihn je Pass durch
    std::vector<std::string> compile_includes;       // ungenutzt hier (der Host backt die Includes in compile) — Doku
    std::uint64_t            n_ops         = 10000;  // Mess-Workload je dyn-Setting
    std::size_t              max_binaries  = 0;      // 0 ⇒ run_options.cap; beide 0 ⇒ KEIN Cap
    std::string              build_version = "m3v2"; // Resume-Marke (.version-Sidecar)
    // B-9/golden-102: die IDENTITAETS-wirksame BASIS der build_version (Preimage-Glied [10]).
    // `build_version` selbst traegt auf dem Einzel-Pfad den +cxx=...-Provenienz-Suffix (Facade :738)
    // und ist damit als Glied-Wert UNGEEIGNET (der Suffix ist via Glied [5]/[6] schon Identitaet --
    // Doppel-Tragung waere eine zweite Wahrheit). Die Facade belegt dieses Feld auf BEIDEN Pfaden mit
    // der reinen Basis (args.build_version); der Laufzeit-Zwilling reicht es explizit an
    // make_lazy_adhoc_fingerprint_fn_from_env. LEER == Identitaet (Bestands-Aufrufer ohne Facade
    // rechnen byte-identisch weiter -- ihre CompileFns tragen dann auch kein Define).
    std::string   build_version_basis = {};
    std::uint32_t n_repeats           = 3;     // Wiederholungen je (BinaryxSetting)
    std::size_t   cores_per_build     = 4;     // KF-16b Default
    double        min_free_gb         = 0.0;   // RAM-Admission (0 = aus)
    bool          resume_override_set = false; // true ⇒ resume kommt aus `resume`, nicht aus <run_options>
    bool          resume              = true;  // Mess-Resume (#139)
    std::string   sweep_axis;                  // leer = Basis-Selektion; sonst ein deklarierter <axis_sweep>
    std::string   platform_override;           // leer ⇒ <run_options>.platform; sonst Override (CSV-Tag)
    std::string   build_version_tag_override;  // leer ⇒ <run_options>.build_version; sonst Override (CSV-Tag)
    bool          run_sota_series = true;      // S7b: die <sota_series>-Paesse mitfahren (false = nur Basis)
    // Working-Set-Sweep: Default = der Profil-<working_set_sweep>. Ist `working_set_override` gesetzt (>0), ersetzt
    // er den Profil-Sweep durch EINEN einzigen N-Wert (rueckwaerts-kompatibel zur alten PS-foreach +
    // COMDARE_WORKLOAD_RECORDS, wo das Harness die aeussere N-Schleife selbst faehrt). 0 = Profil-Sweep nutzen.
    std::uint64_t working_set_override = 0;
    // INC-G6 (Ledger 33/34/35, 2026-07-19, BAUPLAN Abschnitt 6.1): der golden-N-Materialisierungs-Kanal.
    // ADDITIV/INERT -- golden_range_count==0 UND provision_only==false => byte-identisch zum Ist-Lauf.
    //   golden_range_start/count: EIN Fenster [start, start+count) ueber die Basis-StaticBinaryView (Chunk-Bau
    //     ueber den 2^17-Indexraum statt der ersten N; 0 = kein Fenster = profile_make_basis 0..N-1).
    //   provision_only: NUR bauen (DLLs + .version/.algos-Sidecars), NICHT messen -- entkoppelt den ~8h-Bau vom
    //     mehrtaegigen Messlauf; kein Schreiben von CSV-Mess-Zeilen (nur der CSV-Header).
    std::size_t golden_range_start = 0;
    std::size_t golden_range_count = 0;     // 0 = kein Fenster (Ist-Verhalten)
    bool        provision_only     = false; // true = nur bauen, nicht messen
    // S3 (§62-B COMDARE_PRUEF_ONLY): true = NUR das Konformitaets-Gate je gebauter .so (keine Messung; der
    // Bau laeuft als provision_all im Resume-Modus mit, s. lauf_modus_zusatz unten). Reist bis
    // LazyRunConfig::pruef_only durch.
    //
    // D3-7b-RIEGEL (2026-08-11): hier stand "Gegenseitig ausschliessend mit provision_only." -- eine
    // Zusage ohne Werkzeug. Am Objekt gemessen war es ein ZIRKELZITAT: die einzige Quelle, die der
    // Nachbar-Kommentar in profile_run_entry.hpp:1272 fuer die Ausschliessung anfuehrte ("siehe
    // RunProfileArgs"), war genau DIESE Zeile. Zwei Kommentare belegten einander, 0 Zeilen Code setzten
    // etwas durch. Durchgesetzt wird es jetzt von run_profile (unten, exit_code 7) und -- fuer jeden
    // Aufrufer, der an der Fassade vorbei direkt eine LazyRunConfig baut -- von
    // run_lazy_static_then_dynamic. Beide fragen dasselbe Praedikat: ex::lauf_modus_konflikt.
    bool pruef_only = false;
    // W6 (Ledger §32-F7): expliziter Bau-Pool-Worker-Override (COMDARE_BUILD_PARALLEL). 0 = ungesetzt =>
    // parallel_jobs()-Heuristik = byte-neutrales Ist. >0 = harte parallele Compile-Zahl (KOMPILATION parallel,
    // MESSEN bleibt 1-Thread). Reist bis LazyRunConfig::build_parallelism durch.
    std::size_t build_parallelism = 0;
    // Achse 2 (#135): XML-Lastprofil-Registry (id → WorkloadConfig). Vom Host via discover_load_profiles gesetzt.
    std::map<std::string, wd::WorkloadConfig> workload_registry;
    std::vector<std::string>                  workload_values; // nur fuers Log (Achse-2-Werte)
};

// ── Ergebnis (rein zaehlend; die CSV ist die maßgebliche Mess-Ausgabe). ──
struct RunProfileResult {
    int           exit_code        = 1;
    std::size_t   basis_rows       = 0; // CSV-Zeilen aus dem Basis-Pass (frisch+resumiert)
    std::size_t   sota_rows        = 0; // CSV-Zeilen aus den SOTA-Reihen-Paessen (frisch+resumiert)
    std::size_t   basis_binary_ids = 0; // distinkte Basis-binary_ids, die in DIESEM Lauf selektiert wurden
    std::size_t   sota_binary_ids  = 0; // distinkte SOTA-Reihen-binary_ids, die gebaut/gemessen wurden
    std::uint64_t any_measured     = 0;
    std::uint64_t any_resumed      = 0;
    std::uint64_t any_provisioned =
        0; // INC-G6: bereitgestellte DLLs (gebaut ODER resumiert) -- Erfolg im provision_only-Lauf
    // S3 (§62-B COMDARE_PRUEF_ONLY): Gate-Ergebnis-Aggregat ueber alle Paesse (nur im pruef_only-Lauf > 0).
    // any_pruef_failed > 0 => Exit != 0 (User-Vertrag: exit!=0 bei Gate-Fail).
    std::uint64_t any_pruef_ok     = 0;
    std::uint64_t any_pruef_failed = 0;
};

// Interne Helfer: zaehlt '\n' in einem CSV-Block (resumierte Zeilen liegen als String vor).
[[nodiscard]] inline std::size_t count_lines(std::string const& s) {
    std::size_t n = 0;
    for (char c : s)
        if (c == '\n') ++n;
    return n;
}

// -- GN-3 (Par.33 Systembeweis-Traeger, 2026-07-19): die GETEILTE optxsimd-Flag-Aufloesung. Vorher lebten diese drei
//    Helfer im comdare_experiment-Kanal (experiment_run_entry.hpp), der DIESEN Header inkludiert; relokiert nach
//    UNTEN, damit BEIDE Lauf-Pfade (run_profile hier + run_experiment_profile oben) die SELBE Naht nutzen (kein
//    Duplikat). Single-Source der Flags = die Achsen-Structs (OptO*SubAxis::gcc_opt_flag / SimdSubAxis::gcc_march_flag).
//    gcc/clang teilen -O<n>/-march (der Facade-compile_for_perm haengt sie an die eine rsp-Zeile). ──
[[nodiscard]] inline std::string system_axis_opt_flag_of(std::string_view opt_id) {
    namespace cm = ::comdare::cache_engine::measurement;
    if (opt_id == cm::OptO0Option::opt_level_id()) return std::string{cm::OptO0Option::gcc_opt_flag()};
    if (opt_id == cm::OptO1Option::opt_level_id()) return std::string{cm::OptO1Option::gcc_opt_flag()};
    if (opt_id == cm::OptO2Option::opt_level_id()) return std::string{cm::OptO2Option::gcc_opt_flag()};
    if (opt_id == cm::OptO3Option::opt_level_id()) return std::string{cm::OptO3Option::gcc_opt_flag()};
    if (opt_id == cm::OptOfastOption::opt_level_id()) return std::string{cm::OptOfastOption::gcc_opt_flag()};
    return {}; // unbekannt ⇒ Caller degradiert sichtbar auf den CEB-Default (D1, KonfigXmlParse)
}
[[nodiscard]] inline std::string system_axis_march_of(std::string_view simd_id) {
    namespace cm = ::comdare::cache_engine::measurement;
    if (simd_id == cm::SimdAvx2Option::simd_id()) return std::string{cm::SimdAvx2Option::gcc_march_flag()};
    if (simd_id == cm::SimdAvx512Option::simd_id()) return std::string{cm::SimdAvx512Option::gcc_march_flag()};
    return {}; // no_extension / unbekannt ⇒ generisch (kein -march)
}
// (i) §61-STUFEN Compile-Kennzeichnung: build_type_version_suffix() (+bt=Debug NUR bei COMDARE_BUILD_TYPE=Debug)
// lebt im winzigen build_type_stamp.hpp (isoliert testbar); hier via Include verfuegbar (perm_suffix unten).
// ISA-Gate (E1): die simd-Erweiterung nur zulassen, wenn sie verfuegbar ist. Die -march-Flag IST das
// Gate fuer die Organ-SIMD-Codegen (Organ-SIMD <= System-SIMD-Zulassung).
//
// C-3c (O-8 Schritt 6, Bauplan TEIL I P7, PATCH-AN-A3): die Zulassung fragt jetzt ZUERST die
// DEKLARIERTE KLASSEN-Identitaet, nicht den Host. Grund: der Host ist eine INSTANZ-Eigenschaft
// ("auf welcher Kiste laufe ich"), die Zulassung gehoert aber zur KLASSE ("was fuer eine Maschine
// ist das") -- sonst hat dieselbe Maschinen-Klasse auf zwei Kisten zwei verschiedene Zulassungen,
// und genau das verbietet Ledger 70.6. Die Kette ist die des O-4-Pakets: <machines>-Tupel ->
// kDeclaredMachines -> deklarierte SIMD-Signatur, mit Drift-Gegenprobe gegen die echte CPU.
// active_machine_signature() liefert AUSSCHLIESSLICH bei Verdict Match etwas; ohne Belegung, bei
// Tupel-Fehltreffer, nicht erhobener Kern-Kennung oder Abweichung bleibt sie leer.
//
// WARUM DIE HOST-PROBE BLEIBT: sie ist der ehrliche Fallback fuer den Zustand "es ist gar keine
// Maschine deklariert". [NACHGEFUEHRT S-7, 2026-08-13: der Satz "0 Produktions-Aufrufer" war seit
// der S-3-Landung STALE -- die CEB ruft set_active_machine_declaration seit S-3c produktiv ueber
// DIE EINE Naht (maschinen_deklarations_naht.hpp:69, erreicht aus run_experiment_profile_facade,
// profile_run_facade.cpp:1257). Unbelegt bleibt der Zustand nur noch XML-konditional: kein
// hostname_hint-Treffer / keine <machines>-Menge / kein ermittelbarer Hostname.] Die Probe
// ersatzlos zu streichen wuerde avx2/avx512 in genau diesem Zustand sofort ueberall verbieten und
// den Bau still veraendern. Sobald die Belegung steht, gewinnt die Deklaration; der Fallback
// verschwindet damit von selbst, ohne zweiten Kanal und ohne stillen Zwischenzustand.
[[nodiscard]] inline bool system_axis_host_supports_simd(std::string_view simd_id) {
    namespace cm = ::comdare::cache_engine::measurement;
    if (simd_id == cm::SimdNoExtOption::simd_id()) return true;
    // Welches cpuinfo-Flag die Grob-Route mindestens braucht (Unterstrich-Falle: cpuinfo-Id != -m-Flag).
    std::string_view required{};
    if (simd_id == cm::SimdAvx2Option::simd_id())
        required = "avx2";
    else if (simd_id == cm::SimdAvx512Option::simd_id())
        required = "avx512f";
    else
        return false; // unbekannte id => nicht zulassen
    if (auto const signature = cm::active_machine_signature(); !signature.empty()) {
        for (auto const& flag : signature)
            if (flag.cpuinfo == required) return true;
        return false; // deklarierte Klasse kennt das Flag NICHT -> abgelehnt, ohne Host-Rueckfrage
    }
#if defined(__x86_64__) || defined(_M_X64)
    // Keine Deklaration: Bau- und Mess-Host sind im golden-Lauf derselbe, daher die CPUID-Probe.
    // Fused-off-AVX512 (prod2) meldet sich hier korrekt als nicht verfuegbar.
    if (required == "avx2") return __builtin_cpu_supports("avx2");
    if (required == "avx512f") return __builtin_cpu_supports("avx512f");
#endif
    return false; // avx* auf nicht-x86 -> nicht zulassen
}

/// Cache-Resthygiene-2: das PRE-IMAGE des Chunk-Organ-Fingerprints (COMDARE_GN_ALGO_SIG-Quelle) -- die perm.dll.algos-
/// Inhalte (compose_algo_signature) der Range-Binaries, stem-sortiert + konkateniert. `| sha256sum` == der S1-F1-Marker-
/// algo_sig. Rein aus dem KATALOG, OHNE DLL-Bau (Pre-Pull nutzbar). range_count==0 => ganze View. Leere algo_sig
/// (Organ-Gate aus) => uebersprungen (write_algos_sidecar schriebe dann keine Datei -> nicht im find).
[[nodiscard]] inline std::string chunk_organ_fingerprint_preimage(std::filesystem::path const& profile_path,
                                                                  std::size_t range_start, std::size_t range_count) {
    std::optional<cx::ThesisProfile> const tp_opt = load_thesis_profile(profile_path);
    if (!tp_opt) return {};
    auto const&        tp        = *tp_opt;
    std::string const  mode_name = tp.modes.empty() ? std::string{"m3v2_base"} : tp.modes.front().name;
    auto               factory   = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree basis_tree{factory};
    basis_tree.build(build_profile_basis_levels(tp, mode_name, /*with_dynamic=*/true));
    ex::StaticBinaryView const basis_view = basis_tree.static_binary_view();
    auto const                 algo_table = ex::build_axis_variant_version_table();
    std::size_t const          start      = (std::min)(range_start, basis_view.size());
    std::size_t const          stop =
        (range_count == 0) ? basis_view.size() : (std::min)(range_start + range_count, basis_view.size());
    std::vector<std::pair<std::string, std::string>> pairs; // (stem, algo_sig)
    for (std::size_t i = start; i < stop; ++i) {
        ex::BinarySpec const spec = basis_view[i];
        std::string          algo = ex::compose_algo_signature(spec.axes, algo_table);
        if (algo.empty()) continue; // kein perm.dll.algos -> im find nicht enthalten (Organ-Gate aus)
        pairs.emplace_back(ex::orch_make_stem(spec.binary_id, i), std::move(algo));
    }
    return ex::organ_fingerprint_preimage_from_pairs(std::move(pairs));
}

/// D3-7b: DIE EINE QUELLE DES MODUS-TOKENS DER BILANZ-ZEILE.
///
/// WARUM ALS FUNKTION UND NICHT INLINE IM cout: die Zeile "RUN_PROFILE fertig: ..." ist eine
/// SCHNITTSTELLE, kein Log-Text. Ein Leser draussen (der Lauf-Marker der super-CI) zerlegt sie in
/// Felder und leitet daraus einen Modus ab. Solange die Token-Wahl als zwei unabhaengige ternaere
/// Ausdruecke mitten in der Ausgabe-Verkettung stand, konnte niemand die Zusage pruefen, die diese
/// Schnittstelle traegt: DIE ZEILE FUEHRT HOECHSTENS EIN MODUS-TOKEN. Zwei Token nebeneinander waeren
/// fuer jeden Feld-Zerleger mehrdeutig -- er nimmt das erste, das er kennt, und meldet einen Modus,
/// den der Lauf nicht gefahren hat. Als Funktion ist die Zusage ueber alle VIER Schalter-Belegungen
/// pruefbar, und der Test tut das (Nenner 4 von 4).
///
/// DER KONFLIKT-FALL KANN AUS DEM LAUF NICHT KOMMEN -- run_profile verweigert ihn oben mit exit 7.
/// Er ist hier trotzdem ausgeschrieben, statt per Vorrang auf einen der beiden Modi abgebildet zu
/// werden: eine Abbildung waere genau das Raten, das der Riegel abgeschafft hat. Das Konflikt-Token
/// ist KEIN Modus-Token; ein Zerleger, der es nicht kennt, findet ueberhaupt keinen Modus -- was in
/// dieser Lage die ehrliche Antwort ist.
[[nodiscard]] inline std::string lauf_modus_zusatz(bool provision_only, bool pruef_only) {
    if (ex::lauf_modus_konflikt(provision_only, pruef_only)) return std::string{" "} + ex::kLaufModusKonfliktMarke;
    if (provision_only) return " (provision-only)";
    if (pruef_only) return " (pruef-only)";
    return {};
}

/// run_profile — DIE EINE deklarative CEB-Eintritts-API (S7c). Faehrt aus EINEM Profil BEIDE Subsets:
///   (1) den BASIS-Pass (permute_axes → source_catalog) und
///   (2) je <sota_series> einen SOTA-Reihen-Pass (sota_catalog), getaggt A/B/C,
/// ueber EINE vereinigte SourceGenFn (make_union_source_gen) in EINE CSV. Die per-Pass-Iteration ueber
/// <working_set_sweep> bleibt erhalten (aeussere N-Schleife). Resume/binary_id-Stabilitaet wie der heutige
/// Treiber (run_lazy_static_then_dynamic je Pass; #139-Stamp pro Pass).
[[nodiscard]] inline RunProfileResult run_profile(RunProfileArgs const& a) {
    RunProfileResult res;

    // -- (0-) D3-7b-RIEGEL: DIE ZWEI LAUF-MODI SCHLIESSEN SICH AUS -- durchgesetzt, nicht zugesagt. ---
    // VOR dem Profil-Parse, und das ist die Aussage dieser Position: die Ablehnung haengt an NICHTS
    // ausser den zwei Schaltern. Kein Profil, kein Dateisystem, kein Host-Zustand kann sie kippen oder
    // verdecken -- und der Test kann sie ohne jede Fixture belegen (ein unlesbarer Pfad liefert mit
    // Konflikt trotzdem 7, ohne Konflikt 5; das trennt Riegel und Parse-Fehler sauber).
    //
    // WAS DAVOR GALT (am Objekt gemessen 11.08.2026, nicht aus einer Doku uebernommen): drei Kommentare
    // behaupteten die Ausschliessung, KEINE Zeile Code stellte sie her -- die Fassade kopierte beide
    // Schalter unveraendert weiter (profile_run_facade.cpp:750/:751). Der Zustand war erreichbar, weil
    // die beiden Schalter im Betrieb aus zwei getrennten Umgebungsvariablen kommen und der Planer die
    // erste nie zurueckzieht. Er war zugleich UNBEOBACHTBAR, weil der Iterator den Provision-Zweig fuhr
    // und die Bilanz-Zeile trotzdem den Pruef-Modus meldete.
    //
    // EIGENER EXIT-CODE 7: der Aufrufer soll "widerspruechlicher Auftrag" von "Profil unlesbar" (5) und
    // von "Profil hat sich zwischen den Parsen bewegt" (6) unterscheiden koennen. Ein gemeinsamer Code
    // machte drei verschiedene Ursachen im CI zu einer Zahl.
    if (::comdare::cache_engine::builder::experiment::lauf_modus_konflikt(a.provision_only, a.pruef_only)) {
        std::cerr << "[Compiler-Compiler-Fehler: "
                  << ::comdare::cache_engine::measurement::error_class_label(
                         ::comdare::cache_engine::measurement::CompilerCompilerErrorClass::KonfigXmlParse)
                  << "] " << ::comdare::cache_engine::builder::experiment::kLaufModusKonfliktMarke
                  << " provision_only=1 UND pruef_only=1. Die beiden Lauf-Modi schliessen sich aus:"
                     " provision_only baut und misst nicht, pruef_only misst nicht und gatet nur die"
                     " fertigen .so. Welcher gemeint war, weiss nur der Aufrufer -- deshalb ABBRUCH VOR"
                     " dem Profil-Parse und VOR jedem Bau (fail-closed) statt eines geratenen Vorrangs."
                     " Genau EINEN der beiden Schalter setzen (im Betrieb:"
                     " COMDARE_GOLDEN_N_PROVISION_ONLY oder COMDARE_PRUEF_ONLY, nie beide).\n";
        res.exit_code = 7;
        return res;
    }

    // ── (0) Profil EINMAL parsen (die EINZIGE WHAT-Quelle). ──
    std::optional<cx::ThesisProfile> const tp_opt = load_thesis_profile(a.profile_path);
    if (!tp_opt) {
        std::cerr << "run_profile: Profil '" << a.profile_path.string()
                  << "' nicht lesbar (parse_thesis_profile=nullopt) — Abbruch.\n";
        res.exit_code = 5;
        return res;
    }
    auto const& tp = *tp_opt;
    // -- (0a) T-1 DIE ZWEI-PARSE-WACHE. Der Kopf oben sagt "Profil EINMAL parsen" -- fuer DIESE Funktion
    //    stimmt das, fuer den LAUF nicht: die Facade hat dieselbe Datei schon einmal geparst und aus IHREM
    //    Parse entschieden, ob compile_for_perm belegt wird. Zwischen den beiden Parsen liegt Arbeit
    //    (Lastprofil-Entdeckung, Methodik-Aufloesung, Achsen-Versionstabelle) -- und damit ein Fenster, in
    //    dem die Datei sich bewegen kann. Bewegt sie sich, laeuft die Perm-Schleife unten mit einer halben
    //    Verdrahtung: ohne compile_for_perm ist perm_bau_je_zelle false, der per-Perm-Fingerprint-Provider
    //    greift nicht, es wirkt der lauf-konstante -- jede Zelle bekaeme denselben Digest (LAG-Z1, WEG F).
    //    Die Gegenrichtung kostet die Provenienz: die Basis-build_version traegt den system_axes_version_
    //    suffix() dann NICHT, obwohl die Schleife, die ihn je Perm angehaengt haette, nie laeuft.
    //    FAIL-CLOSED wie an den beiden Nachbar-Nahten (T2-C oben, W10-C4 unten): lieber ein ehrlicher
    //    Abbruch als ein Lauf, dessen Stempel eine Verdrahtung behauptet, die es nicht gibt.
    if (!::comdare::cache_engine::profile_facade::system_achsen_entscheidung_haelt(a.system_achsen, tp)) {
        std::cerr << "[Compiler-Compiler-Fehler: "
                  << ::comdare::cache_engine::measurement::error_class_label(
                         ::comdare::cache_engine::measurement::CompilerCompilerErrorClass::KonfigXmlParse)
                  << "] T-1: das Profil '" << a.profile_path.string()
                  << "' hat sich zwischen den beiden Parsen dieses Laufs bewegt. Die Facade entschied "
                  << (a.system_achsen == ::comdare::cache_engine::profile_facade::SystemAchsenEntscheidung::Ja
                          ? "<system_axes> JA"
                          : "<system_axes> NEIN")
                  << ", der eigene Parse sagt "
                  << (::comdare::cache_engine::profile_facade::profil_deklariert_system_achsen(tp) ? "JA" : "NEIN")
                  << ". Damit passten Perm-Schleife und compile_for_perm nicht mehr zusammen -- Abbruch VOR "
                     "jedem Bau (fail-closed: lieber kein Lauf als einer mit falschem Stempel).\n";
        res.exit_code = 6;
        return res;
    }
    std::string const       mode_name = tp.modes.empty() ? std::string{"m3v2_base"} : tp.modes.front().name;
    ProfileRunOptions const ro        = profile_run_options(tp);

    // ── (1) Der BASIS-Baum (build_profile_basis_levels = build_axis_levels OHNE tier-Ebene). ──
    auto                       factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree         basis_tree{factory};
    std::vector<ex::AxisLevel> basis_levels = build_profile_basis_levels(tp, mode_name, /*with_dynamic=*/true);
    // ── (1b) ACHSE 2 (Lastprofil) als DYNAMISCHE Ebene injizieren (STRANG-A-Wiring-Luecke, 2026-06-19).
    //    build_axis_levels emittiert die runtime_dynamic-DynDims (concurrency.thread_count / prefetch.hw_prefetcher /
    //    repetition.repetition_index), aber NICHT die Lastprofil-Achse (die wird separat ueber discover_load_profiles
    //    entdeckt → a.workload_values). Ohne diese Ebene fehlt im setting_label das Segment "workload.workload_id=X";
    //    der Iterator faellt dann auf run_observable_perm (fixer Workload, KEIN Zwei-Phasen-Cache-Warmup) zurueck →
    //    JEDE Zeile two_phase_valid=0 (Mess-UNGUELTIG). Mit der Ebene laeuft run_workload_perm mit Pflicht-Rollback
    //    → two_phase_valid=1. Gleiche Konvention wie die uebrigen DynDims (block_id.variable=value): block_id
    //    "workload", variable "workload_id" (lazy_extract_workload_id sucht genau "workload_id="). is_static=false ⇒
    //    veraendert die binary_id NICHT (Round-Trip-Gate unberuehrt).
    if (!a.workload_values.empty())
        basis_levels.push_back(
            ex::AxisLevel{"workload", a.workload_values, /*is_static=*/false, "workload_id", "workload"});
    basis_tree.build(basis_levels);
    ex::StaticBinaryView const basis_view = basis_tree.static_binary_view();

    // ── (2) cap/resume/Tags (Profil-Defaults; Override aus den Args). cap-Aufloesung = profile_effective_cap
    //    (Single-Source, profile_runner.hpp): cap="0"/fehlendes cap = KEIN Cap → alle Basis-Zellen (GO-4/GO-5-
    //    Nebenbefund 2026-07-12; vorher ergab eff_cap==0 eine LEERE Basis-Selektion — m3_golden_coverage
    //    (cap="0" = dokumentiert "KEIN kuenstliches Cap") bekam damit faelschlich keine Basis-320). ──
    bool const        resume = a.resume_override_set ? a.resume : ro.resume;
    std::size_t const N      = profile_effective_cap(ro.cap, a.max_binaries, basis_tree.binary_count());

    std::string tag_platform      = a.platform_override.empty()
                                        ? (ro.platform.empty() ? std::string{"win-x86_64"} : ro.platform)
                                        : a.platform_override;
    std::string tag_build_version = a.build_version_tag_override.empty()
                                        ? (ro.build_version.empty() ? std::string{"m3v2"} : ro.build_version)
                                        : a.build_version_tag_override;

    // ── (3) S7a + FF(#168): die EINE vereinigte SourceGenFn (Basis-320 ∪ Achsen-Sweeps ∪ SOTA-Reihen).
    //    Reihenfolge unkritisch: Basis-320 ("search_algo=…/migration_policy=migration_none/…"-Pfade) und die
    //    Achsen-Sweep-Map (gleicher 17-Achsen-Pfad-Namensraum, ABER andere Auspraegungen wie
    //    …/migration_policy=migration_hot_cold/…, die im Basis-320 NICHT vorkommen) sind ueberlappungsfrei (bis auf
    //    die Baseline-DLL + die Basis-Achsen-Sweep-ids, die identisch sind → idempotent; union_gen fragt die
    //    Basis-320 zuerst). SOTA liegt im disjunkten "sota_tier=…"-Raum.
    // M-1/D-2 (06.08.2026): DIE EINE Lesung der Mess-Zeile dieses Laufs. Sie hatte bis hierher genau zwei
    // Konsumenten (SOTA-Quellen + lazy Source-Gen, beide unten); seit M-1 der dritte -- die SOLL-Seite
    // des Mess-Vertrags am Pruefdock (LazyRunConfig::erwartete_mess_zeile) --, und seit KON47-01/Option a
    // (12.08.2026) der vierte: die Katalog-Emission (generated_make_catalog_source_gen + Achsen-Sweeps,
    // s. fused/base_union unten). Sie steht bewusst als EIN benanntes
    // Objekt da und wird nicht mehrfach gerufen: measurement_stamp_from_env() ist zwar deterministisch, aber
    // jeder weitere Aufruf waere ein weiterer Ableitungsweg in Wartestellung -- genau die O-8-Schritt-12-Falle,
    // und genau der Mechanismus, aus dem D-1 entstanden ist.
    // M-1/H-2 (06.08.2026): die PMC-Ausstattung gehoert zur Mess-Achse. Der Wurf steht VOR der ersten
    // gestempelten Quelle -- ein Lauf, dessen einkompilierte Mess-Achse und PMC-Ausstattung sich
    // widersprechen, darf gar nicht erst Binaries erzeugen. Herleitung + Messwerte: mess_achsen_naht.hpp.
    // Ohne einkompilierte Combo (der gesamte heutige Bestand) ist der Aufruf ein No-op.
    // KON47-01/Option a: der Block steht seit 12.08.2026 VOR der fused-Zeile -- die Sweep-map ist jetzt selbst
    // eine gestempelte Quelle, der H-2-Satz "Wurf VOR der ersten gestempelten Quelle" bleibt damit wahr.
    ::comdare::cache_engine::profile_facade::pruefe_pmc_gegen_mess_achse();
    std::string const live_mess_zeile        = measurement_stamp_from_env();
    std::string const sota_measurement_stamp = live_mess_zeile;
    // M-1/H-B (06.08.2026): traegt die Tier-Binary dieses Laufs den Observer? Aus DERSELBEN Aufloesung.
    bool const live_observer_ausstattung = ::comdare::cache_engine::profile_facade::live_mess_observer_ausstattung();
    // B2 (15.08.2026): das FEINKORN (G3, Segment-Timer) derselben Aufloesung -- eine Schicht hoeher.
    bool const live_feinkorn_ausstattung = ::comdare::cache_engine::profile_facade::live_mess_feinkorn_ausstattung();
    std::map<std::string, std::string> fused = make_all_axis_sweeps_source_map(
        live_mess_zeile); // alle 17 Achsen-Sweeps (#26/GO-5, INC-2d; Eintragszahl USE-Enable-abhaengig),
                          // seit KON47-01/Option a mit der Mess-Zeile DIESES Laufs gestempelt
    // A13-M3/C1 (K-3): die SOTA-Quellen tragen ab jetzt die VOLLEN Stempel-Zeilen. Die Mess-Combo kommt aus
    // DERSELBEN Aufloesung (live_mess_zeile oben, EINE Env-Bruecke measurement_stamp_from_env), die auch den
    // lazy Source-Gen unten speist -- die SOTA-DLLs eines Laufs stempeln damit dieselbe Tooling-Wahl wie die
    // adhoc-DLLs desselben Laufs. Diese Schleife KONSUMIERT sota_measurement_stamp und gehoert zur
    // FUSED-Befuellung -- sie steht bewusst NACH der fused-Erzeugung (Verify 12.08.2026).
    for (auto& [k, v] : build_sota_view_source_map(tp, sota_measurement_stamp))
        fused.emplace(k, std::move(v)); // + SOTA-Reihen (disjunkt)
    ex::SourceGenFn base_union =
        make_union_source_gen(generated_make_catalog_source_gen(live_mess_zeile), std::move(fused));
    // INC-G6 (33/34): der lazy Per-Index-Emitter als ZUSAETZLICHE Fallback-Quelle HINTER den bestehenden
    // (Basis-320 / Sweeps / SOTA). Die Reihenfolge ist byte-kritisch: fuer die 320er/Sweep/SOTA-ids liefert
    // base_union eine NICHT-leere Quelle -> der lazy Gen wird NIE konsultiert (golden-320 byte-identisch). Erst
    // fuer die uebrigen golden-N-ids (die base_union LEER laesst) rendert der lazy Emitter die reale Quelle
    // (O(17), umgeht build_pilot_source_map -> GN-2-Guard unberuehrt). Ohne golden-N-Fenster wird der lazy Gen
    // im Ist-Lauf nie erreicht (alle selektierten ids liegen in base_union) -> byte-identisch.
    // S6-P1b Env-Bruecke (e): die vom Planer gewaehlte Mess-Combo reist via COMDARE_MEASUREMENT_COMBO in den lazy
    // Source-Gen (make_lazy_adhoc_source_gen_from_env) -> die je-Combo-Bauten stempeln ihre DLLs REAL.
    //
    // [KORREKTUR 12.08.2026, AM OBJEKT GEMESSEN] Hier stand: "UNGESETZT/[all] => \"\" => byte-identische Quellen".
    // Der mittlere Pfeil ist FALSCH -- und die Kurzform verdeckt genau den Fall, an dem der F1-Durchstich heute
    // scheitert. Beides gehoert an diese Stelle, weil hier die Mess-Zeile DIESES Laufs entsteht.
    //
    // (A) WAS DIE AUFLOESUNG WIRKLICH LIEFERT -- nicht "":
    //     UNGESETZT -> "[all]"                          (mess_achsen_naht.hpp:317, CT-loser #else-Zweig)
    //     "[all]"   -> measurement_stamp_line_full_set() (anatomy_version_stamp.hpp:378, Section 64-D1-B 22.07.2026;
    //                  dort woertlich "NICHT mehr \"\" (das war die Regression)")
    //     live_mess_zeile oben ist also die VOLLE 3-Tool-Zeile. Genau sie wird zum SOLL des Mess-Vertrags am
    //     Pruefdock (LazyRunConfig::erwartete_mess_zeile, s. Block weiter oben).
    //
    // (B) [BEHOBEN 12.08.2026, KON47-01/Option a] HISTORISCHER BEFUND: die emittierte DLL trug diese Zeile
    //     NICHT. Der lazy_gen unten steht per INC-G6 HINTER base_union; fuer jede id, die base_union bedient
    //     (Basis-320 / Sweeps / SOTA), wird der lazy Gen NIE konsultiert (s. INC-G6-Block direkt darunter) --
    //     die Quelle kam aus dem Katalog und trug die 2-arg-Form OHNE Mess-Zeile.
    //     SEIT DEM FIX: generated_make_catalog_source_gen(live_mess_zeile) + make_all_axis_sweeps_source_map(
    //     live_mess_zeile) stempeln die Katalog-/Sweep-Quellen mit DERSELBEN Zeile, die das Pruefdock als SOLL
    //     fuehrt -- IST und SOLL kommen aus EINEM benannten Objekt (live_mess_zeile, s. M-1/D-2 oben).
    //
    // (C) GEMESSEN 12.08.2026 auf dem gepinnten Stand 670483c0, Profil f1_durchstich (Zelle label=basis-320):
    //     emittierte perm.cpp  -> COMDARE_ANATOMY_VERSION_STAMP(<organ>, <system>)  [2-arg, KEINE Mess-Zeile]
    //     Lauf                 -> fehlerklasse=mess_konsistenz status=deklaration_leer
    //                             (haupt_ist=0, haupt_soll=3), measured=0, Treiber-Exit 1
    //     SOLL-Seite (Vollmengen-Zeile aus (A)) und IST-Seite (Katalog-2-arg aus (B)) widersprachen sich also
    //     fuer genau die Zellen, die base_union bedient. Das WAR der offene F1-Durchstich-Blocker (KON44-01).
    //     [BEHOBEN 12.08.2026] Der Satz "gehoert ins S-6-Fenster" ist per KON47-01/Option a UEBERSTEUERT
    //     (OWNER>PLAN): der Fix aendert nur den WERT des Mess-Glieds der emittierten Stempel-Zeile (leer ->
    //     Vollmenge/Combo), NIE die Glieder-ORDNUNG -- die S-6-Sperre (Glieder-Reihenfolge) bleibt unberuehrt.
    //     Preimage-Wirkung ist GEWOLLT ("heute kostenlos", KON47-01): sha512_line + .fingerprint-Sidecar der
    //     Basis-320-/Sweep-/per-K-Zellen wandern mit; KEIN fingerprint_format-Bump.
    //
    // WAS RICHTIG BLEIBT: byte-stabil ist der Lauf INNERHALB der [all]-Lane -- alle [all]-Laeufe rendern dieselbe
    // Zeile --, nicht weil die Zeile leer waere. Der No-Arg-/Default-Pfad des Katalogs (2-arg) bleibt fuer alle
    // Alt-Aufrufer byte-identisch (Default {}); im LIVE-Lauf hier wird er seit dem Fix mit live_mess_zeile
    // gerufen. GEDECKT: (A) durch test_m1h_stufen_und_pmc_wache Fall (a), "env UNGESETZT == '[all]'" (:110-111);
    // (B)/(C) seit 12.08.2026 durch test_lazy_adhoc_source_gen Wache (g) (cat(stamp) == lazy(stamp) modulo
    // Index-Zeile, _M-Vollform, gewuerfelter Koeder beidseitig).
    ex::SourceGenFn const lazy_gen = make_lazy_adhoc_source_gen_from_env();
    // Task #59 (Additiv-Vertrag GLIED [6]) -- DER bvset-KLARTEXT DIESES LAUFS, EINMAL AUFGELOEST.
    //
    // EINMAL und nicht viermal: derselbe String reist an vier Stellen (in BEIDE Fingerprint-Maker als
    // expliziter Wert, in cfg.bvset_glied als Soll des Skip-Gates UND als Zeile 2 des Sidecars). Wuerde jede
    // Stelle live_build_variant_set_signature_glied() selbst rufen, waeren das vier Aufrufe derselben reinen
    // Funktion -- heute byte-gleich, aber die Gleichheit waere eine Behauptung ueber die Funktion statt eine
    // Eigenschaft des Codes. EIN Wert, vier Verwendungen: die Gleichheit ist dann nicht mehr zu pruefen,
    // sondern strukturell. Genau diese Begruendung traegt schon die Toolchain-Naht fuer das Glied [5]
    // (toolchain_stamp_naht.hpp:12-19: "koennte diese Gleichheit nicht mehr strukturell garantieren,
    // sondern nur behaupten").
    //
    // NB2-5: an die Maker geht er EXPLIZIT, obwohl er byte-identisch zum Live-Default ist. Ein uebergebener
    // Wert gewinnt dort IMMER -- damit steht im Code, welchen Wert diese Binary traegt, statt dass es sich
    // aus einem Default ergibt. Die Abnahme dazu ist ein Differenztest mit fremdem Nenner: derselbe Hash
    // ueber beide Wege.
    std::string const lauf_bvset_glied =
        ::comdare::cache_engine::profile_facade::live_build_variant_set_signature_glied();
    // I2 (Lager-Gate): opt-in Fingerprint-Provider fuer das .fingerprint-Sidecar (Lager-Index-Anker), EINMAL gebaut wie
    // lazy_gen. Gated auf COMDARE_BESTANDSLOG (Default aus => leer => kein Sidecar => byte-neutral). Drift-frei: fasst
    // dieselbe Combo (COMDARE_MEASUREMENT_COMBO) + version_table wie lazy_gen -> der Fingerprint deckt sich byte-genau
    // mit dem sha512_line, den die DLL einkompiliert traegt (anatomy_fingerprint_hex ueber dieselben 4 Zeilen).
    //
    // T2-C -- DIE DRITTE STUFE DER FAIL-CLOSED-REGEL: UNBEKANNTE TIER-REALVERSION HEISST NICHT SKIP-FAEHIG.
    // Die Sonde (toolchain_stamp_naht, einmal je Treiber-Tag) erhebt die Version am Tier-Treiber selbst.
    // Antwortet er nicht -- Treiber fehlt, Tag nicht sondierbar, Ausgabe unbrauchbar --, dann ist die
    // Identitaet der zu bauenden Binary nicht vollstaendig bestimmt. Eine unbestimmte Identitaet darf
    // keinen Skip tragen: der Provider faellt weg, damit kein `.fingerprint` entsteht, und dll_is_current
    // gibt bei leerer Erwartung IMMER false zurueck (build_orchestrator.hpp:305). Ehrlicher Neubau statt
    // geratener Wiederverwendung -- dieselbe Mechanik und dieselbe Begruendung wie beim `na`-Zellwert.
    ex::FingerprintFn const lazy_fingerprint = [&lauf_bvset_glied, &a] {
        char const* const bl = std::getenv("COMDARE_BESTANDSLOG");
        if (bl == nullptr || std::string_view{bl} != std::string_view{"true"}) return ex::FingerprintFn{};
        if (!::comdare::cache_engine::profile_facade::tier_realversion_ist_bekannt()) {
            std::cerr << "[Compiler-Compiler-Fehler: "
                      << ::comdare::cache_engine::measurement::error_class_label(
                             ::comdare::cache_engine::measurement::CompilerCompilerErrorClass::KonfigXmlParse)
                      << "] T2-C: die REALVERSION des Tier-Treibers '"
                      << ::comdare::cache_engine::profile_facade::active_cxx_driver_tag()
                      << "' konnte nicht erhoben werden (Sonde ohne brauchbare Antwort). Die Identitaet der "
                         "Binaries dieses Laufs ist damit nicht vollstaendig bestimmt -- es wird KEIN "
                         "Fingerprint-Sidecar geschrieben, also NICHTS uebersprungen und NICHTS ins Lager "
                         "zurueckgeschrieben (fail-closed: lieber ein ehrlicher Neubau als ein geratener "
                         "Skip).\n";
            return ex::FingerprintFn{};
        }
        // Task #59: das bvset-Glied EXPLIZIT (NB2-5) -- byte-identisch zum Live-Default, aber jetzt derselbe
        // String, der auch als Soll und als Sidecar-Zeile-2 reist. Der No-Perm-Pfad wird damit genauso
        // behandelt wie der Perm-Pfad unten: eine Naht, keine zwei.
        // B-9/golden-102: die build_version-BASIS ebenso EXPLIZIT -- derselbe Wert, den die Facade als
        // -DCOMDARE_BUILD_VERSION_GLIED an die Tier-Uebersetzung haengt (Glied [10]).
        return make_lazy_adhoc_fingerprint_fn_from_env({}, std::nullopt, lauf_bvset_glied, std::nullopt,
                                                       a.build_version_basis);
    }();
    // A7-B (Lager-Gate, G2 Folge-B): das BUILD-VARIANTEN-Gate, gleiches opt-in-Muster wie COMDARE_BESTANDSLOG darueber.
    // Gated auf COMDARE_VARIANT_GATE=true; Default AUS => leerer String => Variant-Gate AUS => byte-neutral (exakt der
    // bisherige Versions-/Organ-Skip, das `.variant`-Sidecar wird ignoriert). Scharf traegt es die MENGEN-Signatur der
    // CMake-enabled Achsen-Typlisten DIESES Treibers: eine per-Maschine anders konfigurierte Enable-Menge ergibt eine
    // andere Signatur -> `.variant`-Mismatch -> Neubau GENAU dieser Binary (das Cross-Maschinen-Gate). Bei identischer
    // Enable-Menge identisch => inert. EINMAL ausgewertet (Lauf-Konstante), nicht je Perm -- die Treiber-Config ist
    // eine Compile-Zeit-Eigenschaft dieses Prozesses. ZIRKULARITAETS-VERBOT: die Soll-Seite kommt aus der AKTUELLEN
    // Treiber-Konfiguration, NIEMALS aus der alten DLL oder deren Sidecar (das darf nur verglichen werden).
    //
    // [NACHGEFUEHRT 2026-08-07 -- EHRLICHE WIRKUNG. Der Absatz oben ist HISTORIK und in seinem Kern-Versprechen
    // UEBERHOLT: "eine andere Signatur -> `.variant`-Mismatch -> Neubau GENAU dieser Binary (das Cross-Maschinen-
    // Gate)". DIESEN MECHANISMUS GIBT ES NICHT MEHR. Seit der A2-Eichung ist `.fingerprint` DER EINE VERGLEICH von
    // dll_is_current (build_orchestrator.hpp:330-337) -- `.variant` wird beim Skip-Check nicht mehr gelesen, weder
    // scharf noch aus. Die Diskriminierung der Enable-MENGEN leistet seit Preimage-Format 3 das bvset-Glied [6] IM
    // Fingerprint (anatomy_fingerprint.hpp:299-306, F7-(b)): strukturell, und ohne diese Variable zu brauchen.
    //
    // WARUM DIE VARIABLE TROTZDEM STEHT -- sie ist DEPRECATED, NICHT TOT. Sie entscheidet heute allein darueber, ob
    // das `.variant`-PROVENIENZ-Sidecar neben der Binary liegt: scharf => write_variant_sidecar schreibt es; aus =>
    // leerer Wert => der Writer ist no-op UND prune_stale_sidecars entfernt ein vorhandenes (build_orchestrator.hpp
    // :440-446 und :698-703). Das ist ein ARTEFAKT-Effekt, kein SKIP-Effekt. Wer sie setzt, bekommt Provenienz auf
    // der Platte -- kein Gate. Genau diese Verwechslung soll der Absatz hier verhindern, denn der NAME verspricht
    // weiter ein Gate.
    //
    // ENTFERNUNG: erst nach vollstaendig gelandetem und gemessenem F7-(b). Der Aufraeumpass fuehrt den Posten als
    // AP-11 mit dieser harten Vorbedingung -- solange das bvset-Glied nicht in JEDEM relevanten Pfad im Fingerprint
    // steht, waere ihr Wegfall ein stiller Deckungsverlust. Bis dahin ist diese Notiz die Wache gegen den Namen.]
    std::string const variant_gate_sig = [] {
        char const* const vg = std::getenv("COMDARE_VARIANT_GATE");
        return (vg != nullptr && std::string_view{vg} == std::string_view{"true"})
                   ? ex::driver_build_variant_signature()
                   : std::string{};
    }();
    // G4a (3) / Folge-A §62-N4: die HOST-Belegung der drei Zell-Koordinaten [d,e,f] dieses Laufs. Der Lager-Schluessel
    // ist das TUPEL (Fingerprint, Zelle) -- der Fingerprint allein traegt die per-Zelle-ISA NICHT, zwei Bauten
    // derselben Permutation unter avx2 und avx512 wuerden sonst falsch dedupliziert. Die Quellen sind DIESELBEN, aus
    // denen die Testat-Zelle der emittierten Batch-Jobs gebildet wird (der Director exportiert je Perm
    // COMDARE_MEASUREMENT_COMBO / COMDARE_GN_OPT / COMDARE_GN_SIMD) -- KEINE zweite Ableitung, kein Raten aus der
    // build_version. Lauf-Konstante wie variant_gate_sig darueber: je Prozess EINE Zelle, nicht je Perm.
    // Ungesetzt => alle drei leer => ZellKoordinaten::empty() => das Dedup verhaelt sich exakt wie vor Folge-A.
    decltype(ex::LazyRunConfig::bestand_zelle) const bestand_zelle = [] {
        auto const env_or_empty = [](char const* name) {
            char const* const v = std::getenv(name);
            return (v != nullptr) ? std::string{v} : std::string{};
        };
        decltype(ex::LazyRunConfig::bestand_zelle) z;
        z.combo = env_or_empty("COMDARE_MEASUREMENT_COMBO");
        z.opt   = env_or_empty("COMDARE_GN_OPT");
        z.simd  = env_or_empty("COMDARE_GN_SIMD");
        return z;
    }();
    // E-04-P1 (Marker-Familie v2): die PFLICHT-Koordinaten des Live-Fortschritts-Kanals. Sie kommen aus GENAU
    // denselben Quellen wie die Zell-Koordinaten darueber -- KEINE zweite Ableitung. Gerendert wird ueber die EINE
    // Legenden-Quelle des Planers (plan_legend), damit die Treiber-Marker und die Shell-Testate desselben Fensters
    // BYTE-GLEICHE zelle=-Werte tragen: nur dann treffen beide Sichten auf denselben Aggregator-Key (zelle, fenster).
    // Layer-Trennung wie in der Shell-Grammatik (Section 62-B-NACHTRAG): zelle = [d,e,f][g,h,i] (System-Perm + Organ-
    // Referenz), die CEB-Ebene [a,b,c] steht im EIGENEN ceb-Feld. lane = COMDARE_LANE (die emittierende Stufe-2-
    // Bau-/Mess-Batch-Lane setzt sie); ungesetzt => der Renderer schreibt den ehrlichen Sentinel "unbelegt".
    // Die opt/simd-Defaults (O3 / no_extension) spiegeln die Emissions-Seite (experiment_plan_director), damit ein
    // Lauf ohne gesetzte Perm-Env dieselbe Zelle benennt wie der emittierte Job.
    ex::MarkerKontext const marker_kontext = [&bestand_zelle] {
        auto const env_or_empty = [](char const* name) {
            char const* const v = std::getenv(name);
            return (v != nullptr) ? std::string{v} : std::string{};
        };
        ex::MarkerKontext k;
        k.lane                 = env_or_empty("COMDARE_LANE");
        std::string const opt  = bestand_zelle.opt.empty() ? std::string{"O3"} : bestand_zelle.opt;
        std::string const simd = bestand_zelle.simd.empty() ? std::string{"no_extension"} : bestand_zelle.simd;
        k.zelle                = pl::system_perm(opt, simd) + pl::organ_reference();
        // COMDARE_MEASUREMENT_COMBO traegt bereits die fertige [a,b,c]-Legende der emittierenden CEB-Strecke
        // (der Director exportiert combo_legend_ woertlich); ungesetzt == der Sentinel des vollen Angebots.
        k.ceb = bestand_zelle.combo.empty() ? std::string{"[all]"} : bestand_zelle.combo;
        return k;
    }();
    ex::SourceGenFn const union_gen = [base = std::move(base_union),
                                       lazy = lazy_gen](std::string const& binary_id) -> std::string {
        std::string src = base ? base(binary_id) : std::string{};
        if (!src.empty()) return src;
        return lazy ? lazy(binary_id) : std::string{};
    };
    ex::FreeRamFn ram = ex::make_system_free_ram_fn();

    // ── (4) Working-Set-Sweep = die aeussere N-Liste (gilt fuer BEIDE Subsets identisch). Das Profil-
    //    <working_set_sweep> ist AUTORITATIV (XML steuert ALLES, #229/G3); working_set_override (env
    //    COMDARE_WORKLOAD_RECORDS, ehem. PS-foreach-Behelf) greift NUR als Fallback, wenn das Profil keinen
    //    Sweep setzt — sonst kollabierte der Behelfs-Override den mehrwertigen XML-Sweep still auf EIN N. ──
    std::vector<std::uint64_t> n_sweep = profile_working_set_sweep(tp); // Profil-<working_set_sweep> autoritativ
    if (n_sweep.empty() && a.working_set_override > 0)
        n_sweep.push_back(a.working_set_override); // Fallback nur ohne Profil-Sweep
    if (n_sweep.empty()) n_sweep.push_back(0);     // 0 ⇒ Iterator setzt records = n_ops
    // Task #31 (W11-Folge, Root-Cause): im provision_only-BAU ist die Tier-Binary N-UNABHAENGIG -- working_set_records
    // (ws_n) ist ein MESS-Parameter (Runtime-Workload-Groesse), KEIN Kompilations-Freiheitsgrad. Die aeussere N-Sweep-
    // Schleife (unten, je Pass) wuerde je N-Wert dieselben DLLs RE-provisionieren: der 2..k-te Durchlauf findet sie
    // dll_is_current (skip) und re-pusht sie im Storage-Modus -> 4x redundanter Push + gedaempfter Async-Overlap (die
    // Skip-Passe haben kein Bau-Fenster zum Ueberlappen). Daher im Bau-Modus GENAU EINE N-Iteration (der Sweep gehoert
    // zum Messen, nicht zum Bauen). Push-MENGE: je Binary genau EINMAL (statt |n_sweep|x). Cluster-Resume BLEIBT sicher
    // -- skip-resumierte Binaries werden weiterhin gepusht (nur nicht |n_sweep|-fach), also nie "geskippt bevor im
    // Store". MESS-Modus (provision_only==false) UNVERAENDERT: dort ist jedes N eine echte Messung (voller n_sweep).
    if (a.provision_only && n_sweep.size() > 1) {
        std::cout << "  [Task#31] provision-only: N-Sweep (" << n_sweep.size()
                  << " Werte) auf 1 kollabiert -- Tier-Binary ist N-unabhaengig (kein Re-Provision/Re-Push je N)\n";
        n_sweep.resize(1);
    }

    std::cout << "RUN_PROFILE (CEB-Eintritt): " << a.profile_path.string() << "  id=" << tp.id << " mode=" << mode_name
              << "  basis_count=" << basis_tree.binary_count() << " (N=" << N << ")"
              << "  sota_series=" << tp.sota_series.size() << "  working_set_n=" << n_sweep.size() << "\n";
    // GO-5 Fork 1 (2026-07-12): die deklarierten <datasets>-Akten-Referenzen als Lauf-Provenienz-Kopf
    // (Single-Source = die test_data-Akten, Fork 2/R2). EHRLICH: der Loader-MESS-Konsum (load_or_generate_ycsb
    // im Workload-Pfad) ist lauf-gated — heute reisen die Referenzen in den Resume-Stamp (make_cfg) + dieses Log.
    if (!tp.datasets.empty()) {
        std::cout << "  [DATASETS] deklariert=" << tp.datasets.size()
                  << " (Akten=Single-Source; Loader-Mess-Konsum lauf-gated, Signatur geht in den Resume-Stamp)\n";
        for (auto const& d : tp.datasets)
            std::cout << "      dataset id=" << d.id << " akte_ref=" << d.akte_ref << " loader=" << d.loader << "\n";
    }

    // ── EINE CSV; Header GENAU EINMAL; darunter Basis-Pass + SOTA-Paesse (alle N). ──
    // M11 (G5-Audit w289llo0o): Stream-Fehlerpruefung. Liess der open() scheitern (Pfad nicht
    // schreibbar / Platte voll), waere die CSV stillschweigend leer geblieben → exit_code haette
    // faelschlich Erfolg gemeldet. Open-Erfolg jetzt hart geprueft; Schreib-/Flush-Fehler fliessen
    // unten (csv.good() nach flush) in den exit_code ein.
    std::ofstream csv{a.out_csv.string(), std::ios::trunc};
    if (!csv) {
        std::cout << "RUN_PROFILE FEHLER: CSV nicht oeffenbar → " << a.out_csv.string() << "\n";
        res.exit_code = 1;
        return res;
    }
    csv << ex::lazy_csv_header();

    // -- A9-S5 (2026-08-09): DIESELBEN Zeilen ZUSAETZLICH in eine Ergebnis-Mappe. -----------------
    // ADDITIV: der rohe CSV-Strom oben bleibt unangetastet -- golden bleibt byte-identisch. Die Mappe
    // entsteht NEBEN a.out_csv, ihr Name folgt der bestandslog-Grammatik. Das Format kommt aus
    // <writeback_methods>; LEER/FEHLEND => xlsx (Owner-KERN 26.07.). Bis heute hatte der seit A9-S3
    // fertige xlsx-Writer NULL Produktions-Aufrufer -- das hier ist der erste. csv UND xlsx zugleich
    // sind seit dem Owner-Entscheid 09.08. gueltig -- dann nennt ziele() BEIDE Ausgaben DERSELBEN Mappe.
    // I/O: im Mess-Fenster werden Zeilen nur ENTGEGENGENOMMEN; geschrieben wird erst in schliessen()
    // unten, neben csv.flush() (Contention-Doktrin, Design-Dossier V-A9-4).
    ::comdare::cache_engine::lager_naht::MappenNaht mappe;
    mappe.oeffnen(a.out_csv, tp.writeback_methods);
    mappe.kopf_aus_csv(ex::lazy_csv_header());
    std::cout << "  [MAPPE] " << mappe.diagnose() << (mappe.scharf() ? "  ziele=" + mappe.ziele() : "") << "\n";

    // Gemeinsame Lauf-Config-Vorlage (je Pass kopiert + getaggt). 1 DLL = 1 TU bleibt.
    // #171 (2026-06-20): make_cfg traegt zusaetzlich pruefling_type (full/abstract/-). Basis/Sweep uebergeben
    // leer ("-"); die SOTA-Paesse uebergeben den aus merge abgeleiteten Typ (sota_catalog::derive_pruefling_type).
    // GO-5 Fork 6 (2026-07-12): zusaetzlich fairness_mode (common_denominator/native/-) — Basis/Sweep "-",
    // SOTA-Paesse den deklarierten <sota_series fairness=..>-Modus (SotaPass.fairness_mode).
    // GO-5 Fork 1 (2026-07-12): die <datasets>-Deklarations-Signatur des Profils geht lauf-weit in JEDE Pass-
    // Config (Resume-Stamp-Konsum; Ankunfts-Nachweis in der Spec — der Loader-Mess-Konsum ist lauf-gated).
    // GO-5 Fork 7 (2026-07-12): die TOOL-BERECHNETE H2-Score-Akte (sota_h2_scores.xml, Schwester-Datei der
    // sota/*.profile.xml — gleiche Co-Lokalisierungs-Ableitung wie load_profiles/ in run_profile_facade).
    // Fehlt die Akte, sind ALLE SOTA-Reihen honest "n/a" (kein Abbruch, kein 0-Phantom); Basis/Sweep = "-".
    std::string const                datasets_signature = profile_datasets_signature(tp);
    std::optional<H2ScoreAkte> const h2_akte =
        a.profile_path.empty()
            ? std::nullopt
            : parse_h2_score_akte(a.profile_path.parent_path().parent_path() / "sota" / "sota_h2_scores.xml");
    if (h2_akte.has_value())
        std::cout << "  [H2-AKTE] sota_h2_scores.xml geladen: " << h2_akte->entries.size()
                  << " Eintraege (tool=" << h2_akte->tool << " " << h2_akte->tool_version << ")\n";
    else
        std::cout << "  [H2-AKTE] keine sota_h2_scores.xml — h2_code_quality_score der SOTA-Reihen = n/a (honest)\n";

    // -- GN-3 (Par.33 Systembeweis-Traeger, 2026-07-19): per-Perm-Kontext der optxsimd-System-Achsen-Naht (Spiegel
    //    experiment_run_entry.hpp:257-289). make_cfg + die Pass-Treiber lesen diese drei Variablen; die optxsimd-
    //    Schleife (unten) setzt sie je Kombination. IDENTITAETS-DEFAULT (kein <system_axes> im Profil) = exakt das
    //    Vor-Wiring-Verhalten: perm_compile=a.compile, build_version/CSV-Tag UNVERAENDERT (die Facade traegt den
    //    system_axes_version_suffix bereits) → golden-320/golden_fullpilot byte-identisch. ──
    ex::CompileFn perm_compile           = a.compile;
    std::string   perm_build_version     = a.build_version;   // .version-Sidecar (Resume-Marke) je Perm
    std::string   perm_tag_build_version = tag_build_version; // CSV-Provenienz-Spalte build_version je Perm
    // F1 / C1-Interim (A2-Fix-Plan, T2-A): das AUSGABE-VERZEICHNIS je Perm. Bis hierher schrieben ALLE
    // System-Zellen in denselben Baum -- und weil der Ordner-Stamm an der binary_id haengt (die per Doktrin
    // NIE Toolchain-Glieder traegt), auf DIESELBE perm.dll, dasselbe .fingerprint-Sidecar und dieselbe
    // per-Binary result.csv. Seit dem Neuanker unterscheiden sich die Fingerprints der Zellen, also
    // verwirft dll_is_current wechselseitig den Bau der jeweils anderen: Dauer-Neubau, und die Mess-Ablage
    // der zuerst gelaufenen Zelle ist ueberschrieben. IDENTITAETS-DEFAULT = a.dll_dir (kein <system_axes>
    // => die Schleife laeuft nie => byte-identisch zum Vor-F1-Verhalten, golden-320 unberuehrt).
    std::filesystem::path perm_output_dir = a.dll_dir;
    // W10-C4: der Lager-Anker-Provider je Perm. Er MUSS per Perm beweglich sein, weil der Fingerprint ab jetzt
    // die ZELLE mitrechnet (die vervollstaendigte System-Zeile) -- ein lauf-konstanter Provider wuerde allen
    // Zellen denselben Lager-Key geben und damit exakt die Kollision zurueckholen, die W10 beseitigt. Er ist
    // ZUGLEICH der FAIL-CLOSED-Schalter: bei einem `na`-Zellwert wird er geleert (kein .fingerprint-Sidecar =>
    // bestand_key_of liefert nullopt => kein Lager-Rueckschrieb). Identitaets-Default = der lauf-weite Provider.
    ex::FingerprintFn perm_fingerprint = lazy_fingerprint;
    // Der Recompute-Provider (Bindungs-Pruefung). Er haengt am GLEICHEN opt-in wie der Fingerprint-Provider:
    // ohne Fingerprint-Provider ist expected_fp leer, das Gate ist per Punkt (1) fail-closed, und ein
    // Teilmengen-Pfad haette nichts zu pruefen. Leer = Pfad AUS (byte-neutral).
    // B-9/golden-102: dieselbe Basis wie im Fingerprint-Provider -- die Bindungs-Pruefung muss ueber
    // DASSELBE Preimage rechnen, das die Binary traegt (sonst Zirkelschluss mit falschem Glied [10]).
    ex::BvsetFingerprintFn const lauf_bvset_fingerprint =
        lazy_fingerprint
            ? make_lazy_adhoc_fingerprint_mit_bvset_fn_from_env({}, std::nullopt, std::nullopt, a.build_version_basis)
            : ex::BvsetFingerprintFn{};
    ex::BvsetFingerprintFn perm_bvset_fingerprint = lauf_bvset_fingerprint;
    // S-7 (KON9-05): der Achsen-Algo-Versions-Provider der STEMPEL-Zulassung -- EINMAL je Lauf aus der
    // Enabled-Tabelle gebaut (build_axis_variant_version_table feuert dabei zugleich
    // guard_all_registered_organ_versions: der Tabellen-Bau IST die Grammatik-Wache ueber die volle
    // registrierte Population). Die Versionen reisen NUR ueber diese Funktion in den achsen-blinden
    // Orchestrator (kein Registry-Include dort, Include-Kegel bleibt klein).
    //
    // KONTRAKT: der Provider liefert GENAU die Tabellen-Antwort; jeder Tabellen-Miss ist ein LEERER
    // String == "kein bekannter Versions-Stand fuer dieses Paar" => im Gate NEUTRAL uebersprungen.
    // Das deckt BEIDE Miss-Klassen, beide am Objekt gemessen (S-7-Abnahme 13.08.2026):
    //   * Achse ohne Tabellen-Zeile: Sub-Achsen-Ebenen "cacheline.*"/"node_width.*"/"alloc_hw.*"
    //     (profile_to_tree.hpp:104-125) und "tier" -- KEINE Versions-Traeger.
    //   * Slot-Achse mit profil-eigenem Wert ausserhalb der Enabled-Registry: planner_thesis_min
    //     permutiert search_algo=bplus, die Tabelle fuehrt {k_ary, interpolation, eytzinger,
    //     linear_scan} (4 Eintraege) -- der .algos-Pfad stempelt dafuer den Sentinel als reine
    //     PROVENIENZ (compose_algo_signature :291) und baut trotzdem; eine Gate-Ablehnung hier
    //     braeche diesen Bestand (test_t2a_f4_facade_plan_durchreichung, im S-7-Bau gemessen ROT
    //     mit Sentinel-Kontrakt, GRUEN mit diesem). Eine Version, die niemand deklariert hat,
    //     fordert nichts -- FORDERN tut nur ein echtes Literal.
    // Die Drift-Wache "jede registrierte Variante traegt eine parsbare Version" ist NICHT hier,
    // sondern COMPILE-HART am Tabellen-Bau (W::algo_version-Zugriff + assert_version_grammar) --
    // ein Runtime-Riegel darauf waere ein zweiter, schwaecherer Richter.
    // HEUTE NACHWEISLICH INERT: alle 123 Bestands-Literale sind nackte c-Formen und gegen JEDE
    // Signatur gedeckt (CT-Batterie algo_stempel_zulassung.hpp) -- byte-/golden-neutral by
    // construction.
    ex::AlgoVersionFn const lauf_algo_version_fn =
        [tabelle = std::make_shared<std::vector<ex::AxisVariantVersion> const>(ex::build_axis_variant_version_table())](
            std::string const& achse, std::string const& wert) -> std::string {
        return std::string{ex::lookup_algo_version(*tabelle, achse, wert)}; // leer bei Miss => neutral
    };
    auto make_cfg = [&](std::uint64_t ws_n, std::size_t cap_for_pass, std::string const& series,
                        std::string const& sweep_axis, std::string const& pruefling_type,
                        std::string const& fairness_mode, std::string const& h2_score) {
        ex::LazyRunConfig cfg;
        cfg.max_binaries = cap_for_pass;
        // G5: <run_options n_ops> ist autoritativ (XML steuert ALLES, #229); der Fassaden-/argv-Wert
        // greift nur als Fallback (n_ops=0 im Profil = ungesetzt).
        cfg.n_ops              = (tp.run_options.n_ops > 0) ? tp.run_options.n_ops : a.n_ops;
        cfg.workload_records   = ws_n;
        cfg.workload_configs   = a.workload_registry;
        cfg.build_version      = perm_build_version; // GN-3: per-Perm (+cxx=+opt=+ext=), sonst = a.build_version
        cfg.row_series         = series.empty() ? std::string{"-"} : series;
        cfg.row_pruefling_type = pruefling_type.empty() ? std::string{"-"} : pruefling_type;
        cfg.row_sweep_axis     = sweep_axis.empty() ? std::string{"-"} : sweep_axis;
        cfg.row_fairness_mode  = fairness_mode.empty() ? std::string{"-"} : fairness_mode; // GO-5 Fork 6
        cfg.row_h2_score       = h2_score.empty() ? std::string{"-"} : h2_score;           // GO-5 Fork 7
        cfg.profile_datasets   = datasets_signature; // GO-5 Fork 1: lauf-weite <datasets>-Signatur (Stamp)
        // M-1/D-2: die SOLL-Seite des Mess-Vertrags. DIESELBE Zeile, die oben die SOTA-Quellen und den lazy
        // Source-Gen stempelt -- der Vertrag vergleicht damit die Binary gegen das, was DIESER Lauf in sie
        // hineingestempelt hat, nicht gegen eine zweite Ableitung.
        cfg.erwartete_mess_zeile = live_mess_zeile;
        // M-1/H-B: die Observer-Ausstattung derselben Aufloesung. false => die stat_*-,
        // observable_axes-, fill_level- und filled_axes-Zellen sind ehrlich "n/a" statt 0.
        cfg.mess_observer_ausstattung = live_observer_ausstattung;
        // B2 (15.08.2026): FEINKORN (G3) derselben Aufloesung. false => die seg_*-Zellen sind ehrlich
        // "n/a" statt 0 -- auch wenn der Observer (G2) da ist ([wallclock,macro]-Binary).
        cfg.mess_feinkorn_ausstattung = live_feinkorn_ausstattung;
        cfg.row_platform              = tag_platform;
        cfg.row_build_version         = perm_tag_build_version; // GN-3: per-Perm-CSV-Tag, sonst = tag_build_version
        cfg.source_dir                = a.src_dir;
        cfg.output_dir                = perm_output_dir; // F1: per-Perm-Zell-Ordner, sonst == a.dll_dir
        cfg.cores_per_build           = a.cores_per_build;
        cfg.build_parallelism         = a.build_parallelism; // W6 (§32-F7): Bau-Pool-Worker-Override (0 = byte-neutral)
        cfg.per_binary_subdirs        = true;
        cfg.bestand_fingerprint_fn    = perm_fingerprint; // I2/W10-C4: per-Perm-Anker (leer = byte-neutral)
        // Task #59: derselbe EINE String, den auch der Fingerprint-Maker bekommen hat -- Soll des Gates UND
        // Zeile 2 des Sidecars. Der Recompute-Provider folgt derselben per-Perm-Umschaltung wie der
        // Fingerprint-Provider (beide werden im `na`-Fall gemeinsam geleert): eine Bindungs-Pruefung, die
        // ueber eine ANDERE Zelle rechnet als der erwartete Hash, wuerde jeden Skip verweigern und den
        // Teilmengen-Pfad still wirkungslos machen.
        cfg.bvset_glied          = lauf_bvset_glied;
        cfg.bvset_fingerprint_fn = perm_bvset_fingerprint;
        // S-7: DER EINE Aktivierungs-Aufruf der Stempel-Zulassungs-Bruecke (Muster S-3c: eine Naht,
        // ein Aufruf). Lauf-konstant wie bvset_glied -- die Tabelle ist eine Compile-Zeit-Eigenschaft
        // dieses Prozesses, nicht der Perm.
        cfg.algo_version_fn           = lauf_algo_version_fn;
        cfg.build_variant_sig         = variant_gate_sig; // A7-B: opt-in Build-Varianten-Gate (leer = byte-neutral)
        cfg.bestand_zelle             = bestand_zelle;    // G4a(3)/§62-N4: [d,e,f] des Lager-Schluessel-Tupels
        cfg.marker_kontext            = marker_kontext;   // E-04-P1: Pflicht-Koordinaten der Marker-Familie v2
        cfg.resume_completed_binaries = resume;
        cfg.provision_only            = a.provision_only; // INC-G6: nur bauen, nicht messen (byte-identisch bei false)
        cfg.pruef_only                = a.pruef_only;     // S3: nur Gate je gebauter .so (byte-identisch bei false)
        cfg.cache_push                = a.cache_push;     // Storage #51: bis zur per-Binary-Naht (No-Op-Default)
        cfg.cache_pull       = a.cache_pull;       // S2 (#46a): BATCH-Warm-Cache-Hydrierung VOR dem Bau (No-Op-Default)
        cfg.measurement_sink = a.measurement_sink; // Storage #51: result.csv -> measure-drop (No-Op-Default)
        // G4b-1 (#46b I1): die letzte Schicht der Bestandslog-Naht -- ab hier liest der Iterator. Erst
        // bestandslog_active (:927-929) entscheidet: alle vier Traeger belegt UND doc_key nicht leer. bestand_
        // fingerprint_fn (oben, das SCHREIBEN des Sidecars) und bestand_key_of (hier, das LESEN) sind die beiden
        // Haelften desselben Vorgangs und muessen zusammen belegt sein, damit das Lager etwas sieht -- beide haengen
        // host-seitig am gleichen COMDARE_BESTANDSLOG-Opt-in. bestand_present bleibt hier BEWUSST unbelegt: seit
        // Lager-TP1(B)/G-E2 bindet der ITERATOR sie selbst (lager_contains + bestand_fingerprint_fn + bestand_zelle)
        // und nutzt sie als per-Binary-BAU-FILTER (LEDGER:3397); eine Host-Injektion an dieser Stelle bleibt die
        // Test-Naht mit Vorrang.
        cfg.bestand_transport  = a.bestand_transport;
        cfg.bestand_key_of     = a.bestand_key_of;
        cfg.bestand_doc_key    = a.bestand_doc_key;
        cfg.bestand_owner_uuid = a.bestand_owner_uuid;
        cfg.bestand_maschine   = a.bestand_maschine;
        // LAG-P2: die LETZTE Schicht des MESSWERT-Genus -- ab hier liest der Iterator. Erst
        // mess_bestandslog_active entscheidet: Transport (fetch+store) UND Key-Provider UND
        // doc_key belegt. Das Gate ist EIGEN (eigener doc_key, eigener Provider), damit ein Host
        // das Binary-Lager fahren kann, ohne das Messwert-Lager zu fuehren -- und umgekehrt.
        // Ohne diese drei Zeilen war der vollstaendig gebaute Mess-Rueckschrieb des Iterators aus
        // dem produktiven Lauf unerreichbar (0 externe Zuweiser, LAG-P2-Befund).
        cfg.mess_bestand_key_of   = a.mess_bestand_key_of;
        cfg.mess_bestand_doc_key  = a.mess_bestand_doc_key;
        cfg.mess_bestand_versions = a.mess_bestand_versions;
        // T2-A/F4: die Plan-Ablage -- das letzte Glied der Kette ProfileRunArgs -> RunProfileArgs -> hier.
        // Ohne diese Zeile war die gesamte F4-Mechanik im produktiven Lauf unerreichbar (der Host baut keine
        // LazyRunConfig selbst). Leer (Default) => PlanPersistenz::aktiv()==false => keine Ablage => byte-
        // neutral. Wirksam wird sie erst unter planer_driven_active (bestandslog_active UND provision_only).
        cfg.batch_plan_datei    = a.batch_plan_datei;
        cfg.partial_marker_sink = a.partial_marker_sink; // W11 (§43.c): BAU-Modus Teil-Marker (No-Op-Default)
        cfg.chunk_part_size     = a.chunk_part_size;     // W11 (§43.c): Teil-Marker-Intervall N (0 = keine)
        cfg.progress_sink       = a.progress_sink; // Welle 5 (E-W5-2): §38-Rueck-Kanal (No-Op-Default => byte-neutral)
        // #45 (§16.2-M1/§61-MODI) + A-05/V-12: seit dem work_mode-Umbau misst KEIN Registry-Modus parallel
        // (der ausgebaute debug war der einzige) -- alle 4 Modi {build,measure,compare,release} und
        // undeklariert => 0 => sequentiell/1-Thread (byte-neutral). COMDARE_MEASURE_PARALLEL (getrennt vom
        // Compile-Pool) wirkt erst wieder, wenn die Faehigkeit "misst UND parallel" in Betrieb geht --
        // kuenftig via --debug-CLI-Flag (S-8/W2) bzw. Zustands-Injektion resolve_measure_parallelism_of_mode
        // (A2.5-g5/Fix 17: hier stand als Beispiel der seit V-12 WERFENDE Token "debug").
        // smoke=>debug-Entkopplung (2026-07-22, Ereignis-Name historisch): ist ein METHODIK-Override gesetzt
        // (a.methodik_run_methodology aus COMDARE_PLAN_METHODIK_PROFILE), speist DIESE Methodik den
        // Mess-Loop; leer => aus tp.run_methodology (byte-neutral). Profil-getrieben (facade-validiert).
        cfg.measure_parallelism = ex::resolve_measure_parallelism(
            a.methodik_run_methodology.empty() ? tp.run_methodology : a.methodik_run_methodology);
        // G4: informatives Feld konsistent aus <repetitions count> speisen (die echten Wiederholungen
        // laufen ohnehin ueber die repetition-DynDim aus tp.repetitions; cfg.n_repeats wird nicht geloopt).
        cfg.n_repeats = (tp.repetitions > 0) ? static_cast<std::uint32_t>(tp.repetitions) : a.n_repeats;
        // T-15 (2026-08-09): das LETZTE Glied der Drift-Kette XML -> ThesisProfile -> LazyRunConfig.
        // Ohne diese Zeilen traegt der produktive Lauf die Code-Defaults und das Profil-XML koennte die
        // Schwelle nicht stellen -- genau die Hartkodierung, die der Register-Befund S5-06 ruegt
        // ("reps/threshold/max_reruns aus dem Profil-XML, nicht hartkodiert").
        //
        // SELBSTCHECK: NUR bei deklariertem <drift_gate> wird ueberschrieben. Fehlt das Element, bleibt
        // der Owner-Default der cache_engine-Schicht (DriftGateConfig) stehen -- er ist die EINE
        // Wahrheit, und eine Kopie der Zahlen in der common-Schicht waere die zweite.
        if (tp.drift_gate_declared) {
            cfg.drift_gate.reps       = static_cast<std::uint32_t>(tp.drift_gate_reps);
            cfg.drift_gate.threshold  = static_cast<double>(tp.drift_gate_threshold_permille) / 1000.0;
            cfg.drift_gate.max_reruns = static_cast<std::uint32_t>(tp.drift_gate_max_reruns);
        }
        cfg.env_limits.thread_count = 16;
        if (a.min_free_gb > 0.0) {
            cfg.ram_per_build_bytes     = static_cast<std::uint64_t>(a.min_free_gb * 1024.0 * 1024.0 * 1024.0);
            cfg.ram_safety_margin_bytes = cfg.ram_per_build_bytes;
        }
        return cfg;
    };

    auto emit = [&](ex::LazyRunResult const& r, std::size_t* row_sink) {
        csv << r.resumed_csv_rows;
        // #165-B (P-MD8, 2026-06-20): den statistischen Ausreißer-Flag VOR der Emission je Pass befüllen. Lokale,
        // mutierbare Kopie der frisch gemessenen Zeilen (LazyRunResult bleibt unberührt); annotate_quality_flags
        // setzt nur das additive quality_flag-Feld (0/1) je (binary_id, profile_name)-Gruppe — gate-frei, keine
        // bestehende Spalte/kein Messwert berührt. resumed_csv_rows (Alt-Zeilen) bleiben unangetastet (Datenerhaltung).
        std::vector<ex::LazyMeasuredRow> rows = r.csv_rows;
        ex::annotate_quality_flags(rows);
        for (auto const& row : rows) csv << ex::format_csv_row(row);
        // A9-S5: DIESELBEN Zeilen in die Mappe -- aus derselben In-Memory-Quelle wie die rohe CSV
        // (Richtung xlsx -> csv, es wird NIE eine fertige CSV-Datei zurueckgelesen).
        mappe.blob_aus_csv(r.resumed_csv_rows);
        for (auto const& row : rows) mappe.zeile_aus_csv(ex::format_csv_row(row));
        *row_sink += count_lines(r.resumed_csv_rows) + rows.size();
        res.any_measured += r.measured;
        res.any_resumed += r.resumed_binaries;
        res.any_provisioned += r.built; // INC-G6: bereitgestellte DLLs (gebaut+resumiert) -- Erfolgsmass provision_only
        res.any_pruef_ok += r.pruef_ok; // S3 (§62-B): Gate bestanden je Zelle
        res.any_pruef_failed += r.pruef_failed; // S3 (§62-B): Gate durchgefallen / .so nicht ladbar
    };

    // ════════════════════════════════════════════════════════════════════════════════════════════════════
    // S7b — PASS 1..m: BASIS + ACHSEN-SWEEPS (permute_axes/axis_sweeps → source_catalog).
    //
    // FF(#168) — ACHSEN-SWEEP: ist die Pass-Achse sweep-katalogisiert (seit #26/GO-5 ALLE Achsen — heute 17, INC-2d,
    // is_deepened_axis), kann/soll der Basis-Baum sie nicht variieren (im Profil ggf. 1 Wert gepinnt →
    // level_size==1). Statt der Basis-View baut der Treiber einen EIGENEN Sweep-Baum aus axis_sweep_levels(axis)
    // — 16 Baseline-Ebenen + die gesweepte Achse VOLL — dessen 17-Achsen-binary_ids die axis_sweep_source_map-Keys
    // treffen (in der union_gen). So entsteht je Auspraegung eine REALE distinkte Lebewesen-DLL (z.B. migration_none
    // vs migration_hot_cold).
    //
    // #26/GO-5 (B.4.1-b, 2026-07-12) — MULTI-SWEEP-DURCHLAUF: vorher fuhr run_profile GENAU EINEN Selektions-Pass
    // (Basis ODER die eine args.sweep_axis) — und der E4-Treiber setzt sweep_axis nie, d.h. die im Profil
    // deklarierten <axis_sweeps> blieben im offiziellen XML-Weg UNGEFAHREN (Dossier-GO-5-Eigenbefund B.1; betraf
    // auch den #18-Coverage-Voll-Lauf). Jetzt liefert profile_sweep_passes die deterministische Pass-Liste:
    // explizites args.sweep_axis ⇒ genau 1 Pass (byte-identisch zum Alt-Verhalten); leer ⇒ Basis-Pass + je
    // deklariertem <axis_sweep> ein Pass in Dokument-Reihenfolge. pass_seen_ids dedupliziert ueber die Paesse
    // DIESES Laufs: die idempotente Baseline (und Basis-Achsen-Sweep-ids, die schon im Basis-Pass selektiert
    // wurden) wird nicht erneut selektiert — jede binary_id wird je Lauf GENAU EINMAL gemessen (keine
    // Doppel-Zeilen in der EINEN CSV); im Einzel-Pass-Fall ist das Set leer ⇒ Verhalten unveraendert.
    // ════════════════════════════════════════════════════════════════════════════════════════════════════
    std::set<std::string> pass_seen_ids; // binary_ids bereits gefahrener Paesse DIESES Laufs (Dedupe)

    auto run_selection_pass = [&](std::string const& pass_axis) {
        if (is_deepened_axis(pass_axis)) {
            std::vector<ex::AxisLevel> sweep_levels = axis_sweep_levels(pass_axis);
            // Dieselben DynamicDims wie der Basis-Baum anhaengen (gleiche thread_countxprefetchxrepetition-Variation).
            for (auto const& dd : basis_tree.dynamic_filter())
                sweep_levels.push_back(
                    ex::AxisLevel{dd.axis, dd.values, /*is_static=*/false, dd.variable, dd.block_id});
            auto               sweep_factory = std::make_shared<ex::ExperimentNodeFactory>();
            ex::ExperimentTree sweep_tree{sweep_factory};
            sweep_tree.build(sweep_levels);
            ex::StaticBinaryView const sweep_view = sweep_tree.static_binary_view();
            std::size_t const          sweep_n    = sweep_tree.binary_count(); // == |Achsen-Auspraegungen|
            std::vector<std::size_t>   ids;
            ids.reserve(sweep_n);
            for (std::size_t i = 0; i < sweep_n; ++i) // ALLE Auspraegungen (volle Achse), abzgl. bereits gefahrener
                if (pass_seen_ids.insert(sweep_view[i].binary_id).second) ids.push_back(i);
            std::size_t const        fresh_n = ids.size();
            ex::BuildSelection const sel     = ex::resolve_selection(ex::select_explicit(std::move(ids)));
            std::cout << "  [BASIS/deep-sweep] axis=" << pass_axis << " auspraegungen=" << sweep_n
                      << " davon neu=" << fresh_n << " (eigener Sweep-Baum, NICHT Basis-View)\n";
            for (std::size_t const idx : sel.indices)
                std::cout << "      sweep binary_id[" << idx << "] = " << sweep_view[idx].binary_id << "\n";
            if (fresh_n == 0) {
                std::cout << "      (alle Auspraegungen bereits in frueherem Pass dieses Laufs selektiert — "
                             "Pass uebersprungen)\n";
                return;
            }
            for (std::uint64_t const ws_n : n_sweep) {
                ex::LazyRunConfig       cfg = make_cfg(ws_n, sweep_n, /*series=*/"-", pass_axis, /*pruefling_type=*/"-",
                                                       /*fairness_mode=*/"-", /*h2_score=*/"-");
                ex::LazyRunResult const r =
                    ex::run_lazy_static_then_dynamic(sweep_tree, sel, perm_compile, union_gen, ram, cfg, a.algo_sig);
                emit(r, &res.basis_rows);
            }
        } else {
            ProfileTaggedSelection const pts      = profile_select(tp, basis_levels, basis_view, pass_axis, N);
            ex::BuildSelection           sel      = pts.selection;
            std::size_t                  pass_cap = N;
            // INC-G6 (33/34): Chunk-Fenster ueber den golden-N-Indexraum. NUR im reinen Basis-Pass (pass_axis
            // leer) und nur bei golden_range_count>0 -> ersetzt die profile_make_basis-Selektion (0..N-1) durch das
            // Fenster [start, start+count) ueber die Basis-View (geklemmt auf view.size()). pass_cap = Fenster-
            // Groesse, damit run_lazy_static_then_dynamic die Selektion NICHT auf N kappt. Inert fuer count==0.
            if (a.golden_range_count > 0 && pass_axis.empty()) {
                std::size_t const start = (std::min)(a.golden_range_start, basis_view.size());
                std::size_t const stop  = (std::min)(a.golden_range_start + a.golden_range_count, basis_view.size());
                std::vector<std::size_t> ids;
                if (stop > start) ids.reserve(stop - start);
                for (std::size_t i = start; i < stop; ++i) ids.push_back(i);
                pass_cap               = ids.size();
                std::string const prov = "golden_n_range:" + std::to_string(start) + ":" + std::to_string(pass_cap);
                sel                    = ex::select_explicit(std::move(ids));
                sel.provenance         = prov;
                std::cout << "  [INC-G6] golden-N Chunk-Fenster [" << start << "," << stop << ") = " << pass_cap
                          << " Binaries (view.size=" << basis_view.size() << ")"
                          << (a.provision_only ? " provision-only" : "") << "\n";
            }
            { // Multi-Sweep-Dedupe (im Einzel-Pass-Fall leer ⇒ no-op, Selektion byte-identisch).
                std::vector<std::size_t> fresh;
                fresh.reserve(sel.indices.size());
                for (std::size_t const idx : sel.indices)
                    if (pass_seen_ids.insert(basis_view[idx].binary_id).second) fresh.push_back(idx);
                if (fresh.size() != sel.indices.size()) {
                    std::string const prov = sel.provenance;
                    sel                    = ex::select_explicit(std::move(fresh));
                    sel.provenance         = prov;
                }
            }
            // A6/§50-CoR: die (mess-getriebene) Dead-Code-Filter-Kette einhaengen. Identitaets-Default (leere Kette)
            // => sel byte-identisch (indices Wert+Reihenfolge + provenance), golden-neutral; realer Handler = deferred #156.
            sel = ex::resolve_selection(std::move(sel));
            std::cout << "  [BASIS] label=" << pts.label << " provenance=" << sel.provenance
                      << " indices=" << sel.size() << " series=" << pts.series << " sweep=" << pts.sweep_axis << "\n";
            if (sel.indices.empty()) {
                std::cout << "      (keine neuen binary_ids in diesem Pass — Pass uebersprungen)\n";
                return;
            }
            for (std::uint64_t const ws_n : n_sweep) {
                // INC-G6: pass_cap == N im Ist-Lauf (byte-identisch), == Fenster-Groesse im golden-N-Chunk-Lauf.
                ex::LazyRunConfig cfg = make_cfg(ws_n, pass_cap, pts.series, pts.sweep_axis, /*pruefling_type=*/"-",
                                                 /*fairness_mode=*/"-", /*h2_score=*/"-");
                ex::LazyRunResult const r =
                    ex::run_lazy_static_then_dynamic(basis_tree, sel, perm_compile, union_gen, ram, cfg, a.algo_sig);
                emit(r, &res.basis_rows);
            }
        }
    };

    std::vector<std::string> const selection_passes = profile_sweep_passes(tp, a.sweep_axis);
    if (selection_passes.size() > 1)
        std::cout << "  [MULTI-SWEEP] Basis-Pass + " << (selection_passes.size() - 1)
                  << " deklarierte <axis_sweep>-Paesse (Dokument-Reihenfolge, #26/GO-5)\n";

    // ── run_all_passes: EIN kompletter Selektions-Durchlauf (Basis/Sweep-Paesse + SOTA-Reihen) fuer die AKTUELLE
    //    optxsimd-Perm. pass_seen_ids wird per-Perm frisch gesetzt (jede optxsimd-Stufe ist ein eigenes Bau-Rennen:
    //    gleiche binary_ids, anderes build_version → eigenes .version-Sidecar/eigene Messung). ──
    auto run_all_passes = [&]() {
        pass_seen_ids.clear(); // per-Perm-Reset (Identitaets-Fall: startet ohnehin leer)
        for (auto const& pass_axis : selection_passes) run_selection_pass(pass_axis);
        // Distinkte Basis-/Sweep-binary_ids DIESES Passes (Baseline je Pass genau 1x). Ueber die optxsimd-Perms
        // akkumuliert (jede Perm = eigenes Bau-Rennen); im Einzel-Pass/Identitaets-Fall == frueherer sel.size()-Wert.
        res.basis_binary_ids += pass_seen_ids.size();

        // ════════════════════════════════════════════════════════════════════════════════════════════════════
        // S7b — PASS 2..k: je <sota_series> EIN SOTA-Reihen-Lebewesen (einwertiger "sota_tier"-Baum, Tag A/B/C).
        //   Disjunkter binary_id-Namensraum ("sota_tier=sota::S::name") → die union_gen liefert die SOTA-Quelle.
        // ════════════════════════════════════════════════════════════════════════════════════════════════════
        if (a.run_sota_series) {
            std::vector<SotaPass> const passes = build_sota_passes(tp);
            std::cout << "  [SOTA] real baubare <sota_series>-Paesse = " << passes.size() << " (von "
                      << tp.sota_series.size() << " deklariert)\n";
            // M-CE-10 (Voll-Review 2026-07-13, Fix (b)): res.sota_binary_ids ist per Doku "distinkte SOTA-Reihen-
            // binary_ids, die gebaut/gemessen wurden" — NICHT die Pass-Zahl. build_sota_passes dedupliziert bereits
            // identische Messungen (KORREKTUR F23 2026-07-16: seit der per-Host-Auffaecherung 2026-07-14 sind V2-
            // Paesse per-Host GENUINE distinkt -- "V2 = 1 HOT-Pilot" war die VOR-M-CE-10-Semantik; dedupliziert
            // werden nur noch WIRKLICH identische Deklarationen, gleicher Host + gleicher fairness_mode); dieser
            // Set-Guard fängt zusätzlich die legitimen fairness-Varianten ab (gleiche binary_id, verschiedener
            // fairness_mode ⇒ EINE reale DLL). So bleibt der Zähler exakt == Zahl der real gebauten/gemessenen
            // distinkten DLLs (kein Über-Zählen wie vor dem Fix).
            std::set<std::string> sota_seen_bids;
            for (auto const& p : passes) {
                // Einwertiger Static-Baum: AxisLevel "sota_tier"=<sota_bid> + dieselben DynamicDims wie der Basis-Baum
                // (damit die SOTA-Zeilen die gleiche thread_countxprefetchxrepetition-Variation tragen).
                std::vector<ex::AxisLevel> sota_levels;
                sota_levels.push_back(
                    ex::AxisLevel{std::string{kSotaTierAxis}, {p.sota_bid}, /*is_static=*/true, "", ""});
                for (auto const& dd : basis_tree.dynamic_filter())
                    sota_levels.push_back(
                        ex::AxisLevel{dd.axis, dd.values, /*is_static=*/false, dd.variable, dd.block_id});

                auto               sota_factory = std::make_shared<ex::ExperimentNodeFactory>();
                ex::ExperimentTree sota_tree{sota_factory};
                sota_tree.build(sota_levels);
                ex::StaticBinaryView const sota_view = sota_tree.static_binary_view();
                // EIN Lebewesen je Reihe = view-Index 0. binary_id == p.view_binary_id ("sota_tier=…").
                ex::BuildSelection const sel = ex::resolve_selection(ex::select_explicit({0}));
                if (sota_seen_bids.insert(p.view_binary_id).second) ++res.sota_binary_ids; // M-CE-10 (b): distinkt
                // GO-5 Fork 7 + M-CE-10 (c): der tool-berechnete H2-Score wird ueber das HOST-Lebewesen (p.h2_lebewesen)
                // aufgeloest — host-dominant (#171: "abstract" = Host fuellt 16/17 Achsen, INC-2d). KORREKTUR F23 (2026-07-16):
                // die fruehere Aussage 'fuer V2 FIX "hot", NIE das angefragte lebewesen' beschrieb die VOR-M-CE-10-
                // Semantik (nur der HOT-Host existierte als V2-Komposition). Seit der per-Host-Auffaecherung
                // (2026-07-14) gilt fuer ALLE Stufen St1/V2/St3: h2_lebewesen == lebewesen (der per-Host-Replace hat
                // DIESEN Host; Gate: test_sota_st2_dedup asserted EXPECT_EQ(p.h2_lebewesen, p.lebewesen), 19 Paesse).
                // prt_art/fehlende Akte ⇒ honest "n/a" (sota-Profil-Dateistamm == Lebewesen-Name der 6 SOTA).
                std::string const h2_score = h2_score_for(h2_akte, p.h2_lebewesen);
                std::cout << "    SOTA-Pass series=" << p.series << " pruefling_type=" << p.pruefling_type
                          << " fairness=" << p.fairness_mode << " h2_score=" << h2_score
                          << " h2_lebewesen=" << p.h2_lebewesen << " lebewesen=" << p.lebewesen
                          << " binary_id=" << (sota_view.empty() ? std::string{"<leer>"} : sota_view[0].binary_id)
                          << "\n";
                for (std::uint64_t const ws_n : n_sweep) {
                    ex::LazyRunConfig cfg =
                        make_cfg(ws_n, 1, p.series, /*sweep_axis=*/"", p.pruefling_type, p.fairness_mode,
                                 h2_score); // #171 full/abstract + Fork 6 fairness + Fork 7 h2
                    ex::LazyRunResult const r =
                        ex::run_lazy_static_then_dynamic(sota_tree, sel, perm_compile, union_gen, ram, cfg, a.algo_sig);
                    emit(r, &res.sota_rows);
                }
            }
        }
    };

    // -- GN-3 (Par.33 Systembeweis-Traeger, 2026-07-19): optxsimd-System-Achsen-Permutation UM die Passes (Spiegel
    //    experiment_run_entry.hpp:257-289, KEINE Parallel-Schleife). Quelle = das GEPARSTE Profil
    //    (tp.compiler.opt_levels / tp.external_utils.simd_options, geteilte parse_system_axes-Naht). KEIN
    //    <system_axes> ⇒ EINE Identitaets-Perm: run_all_passes() genau einmal, perm_compile/perm_build_version/
    //    perm_tag_build_version bleiben auf a.compile/a.build_version/tag_build_version → byte-identisch zum
    //    Vor-Wiring-Verhalten (golden-320/golden_fullpilot unberuehrt; die Facade traegt dort den
    //    system_axes_version_suffix). MIT <system_axes> => je optxsimd eigene CompileFn (compile_for_perm) + eigenes
    //    build_version-/CSV-Suffix (+cxx=+opt=+ext=) + eigener pass/sota-Dedup-Reset; binary_id BLEIBT Organ-only
    //    (opt/simd=system_config, NIE binary_id, NIE N) -- es waechst NUR die MESS-Matrix (CSV x |optxsimd|).
    //    ISA-gegated (avx2 nur x86_64, wie system_axis_host_supports_simd). ──
    namespace cm = ::comdare::cache_engine::measurement;
    // T-1 (2026-08-09): DIESELBE Funktion, die auch die Facade fragt (profile_run_facade.cpp). Hier stand
    // frueher der De-Morgan-Duale des Facade-Ausdrucks ausgeschrieben -- zwei Wortlaute fuer EINE Frage.
    // Die Wache bei (0a) oben haelt zusaetzlich die beiden PARSE-ZEITPUNKTE zusammen.
    if (!::comdare::cache_engine::profile_facade::profil_deklariert_system_achsen(tp)) {
        run_all_passes(); // Identitaet: perm_* bleiben unveraendert (Vor-Wiring-Verhalten)
    } else {
        std::vector<std::string> const opt_perms =
            tp.compiler.opt_levels.empty()
                ? std::vector<std::string>{std::string{cm::DefaultOptLevelOption::opt_level_id()}}
                : tp.compiler.opt_levels;
        std::vector<std::string> const simd_perms =
            tp.external_utils.simd_options.empty()
                ? std::vector<std::string>{std::string{cm::DefaultSimdOption::simd_id()}}
                : tp.external_utils.simd_options;
        for (auto const& opt_id : opt_perms) {
            // W5-C+ (§36.1): GN-Zellen-Filter. Ist COMDARE_GN_OPT gesetzt, baut diese Cluster-Zelle NUR ihre eine
            // opt-Auspraegung; alle anderen werden hier uebersprungen (Muster der ISA-Gate-Skips unten).
            if (!gn_cell_opt_allowed(a.gn_cell_opt, opt_id)) {
                std::cout << "  [GN-ZELLE] opt=" << opt_id << " != Zellen-Filter '" << a.gn_cell_opt
                          << "' — uebersprungen (§36.1: eine System-Perm je Cluster-Zelle)\n";
                continue;
            }
            std::string opt_flag = system_axis_opt_flag_of(opt_id);
            if (opt_flag.empty()) {
                std::cerr << "[Compiler-Compiler-Fehler: "
                          << cm::error_class_label(cm::CompilerCompilerErrorClass::KonfigXmlParse) << "] <opt_level>='"
                          << opt_id << "' unbekannt; degradiere auf CEB-Default "
                          << cm::DefaultOptLevelOption::opt_level_id() << ".\n";
                opt_flag = std::string{cm::DefaultOptLevelOption::gcc_opt_flag()};
            }
            for (auto const& simd_id : simd_perms) {
                // W5-C+ (§36.1): GN-Zellen-Filter (VOR dem ISA-Gate, damit eine gefilterte Zelle als GN-Skip statt
                // ISA-Skip erscheint). Ist COMDARE_GN_SIMD gesetzt, baut diese Cluster-Zelle NUR ihre eine simd-Auspraegung.
                if (!gn_cell_simd_allowed(a.gn_cell_simd, simd_id)) {
                    std::cout << "  [GN-ZELLE] simd=" << simd_id << " != Zellen-Filter '" << a.gn_cell_simd
                              << "' — uebersprungen (§36.1: eine System-Perm je Cluster-Zelle)\n";
                    continue;
                }
                if (!system_axis_host_supports_simd(simd_id)) {
                    std::cerr << "[Compiler-Compiler-Fehler: "
                              << cm::error_class_label(cm::CompilerCompilerErrorClass::HardwareErweiterungFehlt)
                              << "] simd='" << simd_id
                              << "' auf dieser ISA nicht verfuegbar — Permutation uebersprungen.\n";
                    continue;
                }
                // FREIGABE-KOPPLUNG System->Organ (Ledger 36/37/37.b, 2026-07-19; SIMD = Pilot-Instanz): DIES ist die
                // CEB-BAU-DELEGATIONS-NAHT des Freigabe-Prinzips. Die freigegebene System-Zelle (simd_id, oben
                // host-ISA-gegated via system_axis_host_supports_simd) wird ueber system_axis_march_of zur -march-Flag
                // der per-Zelle montierten CompileFn `perm_compile`, die provision_all (BuildOrchestrator) je Binary
                // VOR der Kompilation nutzt. Das -march IST das Gate der Organ-SIMD-Codegen (Organ-SIMD <= System-
                // SIMD-Zulassung, ISA-Gate E1 oben): SIMD-faehige Organ-Varianten (SimdCapableStrategy: array256/
                // vector_u8u8/...) DEGRADIEREN unter no_extension via #ifdef auf Skalar -- KEIN Bau-Fehler. Der
                // Zulaessigkeits-Filter sitzt GENAU HIER (Auswahl VOR der Kompilation), NICHT im Emitter/Tier/Planer.
                // Kuenftige System-Consumer-Achsen (NUMA/NPU/GPU/FPGA) + ein constexpr requires_simd()-Bau-Gate
                // (Audit-G7) fuer HART-SIMD-Organe docken GENAU HIER an (State-Pattern durchs Pruef-Dock, Folge-
                // Increment). Details am Emitter: lazy_adhoc_source_gen.hpp (FREIGABE-KOPPLUNG-Abschnitt).
                std::string const march_flag = system_axis_march_of(simd_id);
                // W10-C4 (Bauplan-Dossier 20260803, Sektion 1/2) -- DIE SCHARFSCHALTUNG. Die Zellwerte dieser
                // Zelle entstehen aus GENAU DREI Quellen: den beiden lauf-konstanten Facade-Zellen (Ziel-ISA,
                // OS-Familie) und der Schleifen-Variable simd_id, aus der auch march_flag und die
                // ZellKoordinate[f] kommen -- KEINE Zweit-Ableitung, insbesondere kein Rueckschluss aus dem
                // -march-Flag. Sie reisen ueber die Compile-Define-Naht in die Makro-Expansion (der Emitter
                // bleibt system-blind, die emittierte Quelle byte-identisch) UND als derselbe String in den
                // Laufzeit-Zwilling unten: Lager-Key und einkompilierter Fingerprint sind konstruktiv gleich.
                std::string const perm_cell_values =
                    ::comdare::cache_engine::profile_facade::compose_system_cell_values(
                        a.system_cell_target_isa, a.system_cell_operating_system, simd_id);
                ::comdare::cache_engine::abi::SystemCellValues const perm_zellwerte{perm_cell_values};
                // T2-B (Codex [CX-B1], KRITISCH) -- DIE PER-PERM-GLIED-[5]-BILDUNG, EINMAL FUER BEIDE SEITEN.
                // Die Achsen kommen als das, was diese Schleife WIRKLICH permutiert: opt_id/simd_id und die
                // daraus aufgeloesten Achsen-Flags. KEINE Zweit-Ableitung -- insbesondere kein Rueckschluss
                // aus march_flag (wortgleiche Regel wie bei den Zellwerten oben). Der Gate-Beitrag wird HIER
                // gebildet, weil ihn ab jetzt zwei Verbraucher teilen (Glied und Suffix); er stand vorher
                // weiter unten und waere sonst ein zweites Mal berechnet worden.
                std::string const perm_simd_segment =
                    (simd_id == std::string{cm::SimdNoExtOption::simd_id()}) ? std::string{} : simd_id;
                std::string const perm_gate =
                    cm::gate_contribution_identity_text(cm::route_of_simd_id(simd_id), cm::SimdDialect::Gpp);
                ::comdare::cache_engine::profile_facade::PermToolchainAchsen perm_achsen{};
                perm_achsen.opt               = opt_id;
                perm_achsen.opt_flags         = opt_flag;
                perm_achsen.simd              = perm_simd_segment;
                perm_achsen.gate_contribution = perm_gate;
                ::comdare::cache_engine::profile_facade::PermToolchainGliedWert const perm_toolchain_glied{
                    ::comdare::cache_engine::profile_facade::compose_toolchain_stamp_glied_for_perm(perm_achsen)};
                // T2-A/H2 (Codex-Befund, 2026-08-06) -- DER FALLBACK STEMPELT AB HIER DEN BAU, DER WIRKLICH
                // STATTFINDET. Ist `compile_for_perm` unbelegt, faellt der Bau dieser Zelle auf die
                // LAUF-KONSTANTE CompileFn zurueck: keine per-Zelle montierten Flags, kein per-Perm-Define
                // (weder Zellwerte noch Glied [5]) -- die entstehende .so ist BYTE-GLEICH der Identitaets-
                // Binary. Gestempelt wurde sie trotzdem per Perm (eigener Fingerprint-Provider, eigenes
                // +cxx=+opt=+ext=-Suffix in .version und CSV-Provenienz). Das ist die schlimmste Sorte
                // Falschaussage, die dieser Pfad kennt: der Stempel behauptet eine Identitaet, die im
                // Binary nicht steht -- das Lager kaeme mit dem Schluessel nie wieder an seine Binary, und
                // die CSV wiese |opt x simd| verschieden getaggte Zeilen ueber EIN UND DENSELBEN Bau aus.
                // Ab jetzt ist der Fallback-Fall in einer benannten Bedingung sichtbar und ALLE drei
                // Stempel-Wege (Fingerprint, build_version, CSV-Tag) folgen ihr: gebaut wird das
                // lauf-konstante Binary, gestempelt wird die lauf-konstante Identitaet. Die Mess-Matrix
                // waechst weiterhin (die Zelle wird gemessen) -- nur luegt sie nicht mehr ueber ihren Bau.
                bool const perm_bau_je_zelle = static_cast<bool>(a.compile_for_perm);
                perm_compile = perm_bau_je_zelle
                                   ? a.compile_for_perm(opt_flag, march_flag, perm_zellwerte, perm_toolchain_glied)
                                   : a.compile;
                if (!perm_bau_je_zelle)
                    std::cerr << "[Compiler-Compiler-Fehler: "
                              << cm::error_class_label(cm::CompilerCompilerErrorClass::ToolchainFehlt)
                              << "] keine per-Perm-CompileFn (compile_for_perm unbelegt) -- opt=" << opt_id
                              << " simd=" << simd_id
                              << " baut mit dem LAUF-KONSTANTEN Bau; Fingerprint, .version-Suffix und "
                                 "CSV-Provenienz dieser Zelle tragen deshalb die lauf-konstante Identitaet "
                                 "(gestempelt wird, was wirklich gebaut wurde).\n";
                // W10-C4 FAIL-CLOSED (n/a-statt-NULL, bindend): ein `na` heisst "diese Zelle ist NICHT
                // BESTIMMBAR" -- die Binary darf gebaut und gemessen werden, aber sie ist nicht zuordbar und
                // geht deshalb NICHT ins Lager. Mechanisch: kein Fingerprint-Provider => kein
                // .fingerprint-Sidecar => bestand_key_of liefert nullopt => kein Rueckschrieb. Das deckt
                // zugleich die riscv64-Auflage (ISA ohne Achsen-Glied => na => kein Rueckschrieb), ohne dass
                // irgendwo eine ISA-Sonderregel steht.
                perm_fingerprint = lazy_fingerprint;
                // Task #59: der Recompute-Provider folgt dem Fingerprint-Provider Zweig fuer Zweig. Er ist
                // KEIN eigener Schalter -- wo kein Fingerprint gilt, gilt auch keine Bindungs-Pruefung.
                perm_bvset_fingerprint = lauf_bvset_fingerprint;
                if (::comdare::cache_engine::abi::system_cell_values_contain_na(perm_cell_values)) {
                    std::cerr << "[Compiler-Compiler-Fehler: "
                              << cm::error_class_label(cm::CompilerCompilerErrorClass::HardwareErweiterungFehlt)
                              << "] System-Zellwerte dieser Zelle nicht bestimmbar (" << perm_cell_values
                              << ") -- die Binaries dieser Perm werden gebaut und gemessen, aber NICHT ins Lager "
                                 "zurueckgeschrieben (kein Fingerprint-Anker: ein `na`-Stempel ist nicht "
                                 "zuordbar).\n";
                    perm_fingerprint       = ex::FingerprintFn{};
                    perm_bvset_fingerprint = ex::BvsetFingerprintFn{}; // Task #59: derselbe fail-closed-Schnitt
                } else if (lazy_fingerprint && perm_bau_je_zelle) {
                    // T2-A/H1 (Codex-Befund, 2026-08-06) -- DIE UMSCHALTUNG HAENGT NICHT MEHR AN `.empty()`.
                    // Die fruehere Bedingung war `lazy_fingerprint && !perm_cell_values.empty()`. Leere
                    // Zellwerte sind aber der API-DEFAULT dieses Einstiegs (compose_system_cell_values gibt
                    // {} zurueck, sobald Ziel-ISA oder OS-Familie unbelegt sind -- jeder Aufrufer, der die
                    // beiden Facade-Zellen nicht setzt, landet dort). Genau dann fiel der Zwilling auf den
                    // LAUF-KONSTANTEN Provider zurueck und rechnete fuer JEDE Zelle denselben Digest: O2 und
                    // O3 bekamen wieder EINEN Lager-Schluessel -- das Loch in T2-B, unter der Zeile
                    // versteckt, die es zu schliessen vorgab. Das per-Perm-Glied [5] ist von den Zellwerten
                    // UNABHAENGIG (es traegt opt/opt_flags/ext/gate), also darf ein leeres Zellwerte-Set die
                    // Umschaltung nicht verhindern. Leer bleibt dabei ehrlich leer: die System-Zeile geht
                    // unveraendert ins Preimage (dokumentierte Identitaet des Parameters), nur das Glied
                    // kommt jetzt per Zelle. Die `na`-Wache oben bleibt VORGESCHALTET (fail-closed schlaegt
                    // Differenzierung), und ohne per-Zelle-Bau (H2) waere ein per-Zelle-Digest eine Luege --
                    // deshalb steht `perm_bau_je_zelle` in derselben Bedingung.
                    //
                    // Der Zwilling rechnet ueber DIESELBE vervollstaendigte System-Zeile wie das consteval-Makro
                    // im Tier-Bau -- sonst zeigte der Lager-Key ab jetzt auf einen anderen Digest als das
                    // einkompilierte sha512_line, und das Lager fuende seine eigenen Binaries nicht wieder.
                    // T2-B: dasselbe gilt ab hier fuer das Glied [5] -- der Zwilling bekommt EXAKT den String,
                    // der oben als Define an die Tier-Uebersetzung ging (derselbe Wert, nicht derselbe Weg
                    // noch einmal gegangen). Ohne diese zweite Haelfte rechnete der Lager-Key ueber das
                    // run-konstante Glied, waehrend die Binary das per-Perm-Glied traegt: garantierter Miss.
                    // Task #59: das bvset-Glied geht ab hier EXPLIZIT herein (NB2-5: ein uebergebener Wert
                    // gewinnt IMMER). Es ist byte-identisch zum Live-Default -- genau das ist die Abnahme
                    // (V-7): zwei Wege, ein Hash. Der Gewinn ist nicht ein anderer Wert, sondern ein
                    // SICHTBARER: derselbe String steht jetzt im Preimage, in cfg.bvset_glied und in Zeile 2
                    // des Sidecars, und keine der drei Stellen leitet ihn selbst ab.
                    // B-9/golden-102: die build_version-BASIS explizit -- derselbe Wert, den die
                    // Facade als -DCOMDARE_BUILD_VERSION_GLIED an perm_compile haengt (Glied [10]).
                    perm_fingerprint =
                        make_lazy_adhoc_fingerprint_fn_from_env(perm_cell_values, perm_toolchain_glied.value,
                                                                lauf_bvset_glied, std::nullopt, a.build_version_basis);
                    perm_bvset_fingerprint = make_lazy_adhoc_fingerprint_mit_bvset_fn_from_env(
                        perm_cell_values, perm_toolchain_glied.value, std::nullopt, a.build_version_basis);
                }
                // Lane F R3 (O-8 Schritt 10): diese Kette WAR die bindende Form -- jetzt kommt sie aus der
                // EINEN Suffix-Quelle, statt sie hier ein zweites Mal zu buchstabieren. Die erzeugten Bytes
                // bleiben identisch (dieselbe Ordnung, dieselbe no_extension-Regel), aber es gibt nur noch
                // eine Stelle, die die Ordnung kennt -- und damit erstmals etwas, wogegen eine Wache prueft.
                ::comdare::cache_engine::profile_facade::SystemVersionSuffixParts perm_parts;
                perm_parts.cxx  = a.compiler_tag;
                perm_parts.opt  = opt_id;
                perm_parts.simd = perm_simd_segment; // T2-B: dieselbe no_extension-Regel wie im Glied [5]
                // W10-M2 (REV2-B2-Rest): das +ceb=-Glied fehlte der Perm-build_version -- der Perm-Pfad ist aber
                // GENAU der Pfad, an dem C4 die Zellwerte scharfschaltet. Ohne diese Verdrahtung waere der
                // Contract-Minor-Bump dort UNSICHTBAR geblieben: dll_is_current vergleicht .version/.algos/
                // .variant, und keines der drei bewegt sich durch C4 -- ein stales prae-W10-Binary haette den
                // Skip passiert und einen Stempel OHNE Zellwerte weitergetragen. Der Wert kommt aus der EINEN
                // Zusammensetzung (ceb_contract_version_text -> abi::kCebContractCodegenMinor).
                // A2-NACHZUG (2026-08-05, GATE 5): der W10-M2-Text darueber ist HISTORIK -- dll_is_current
                // vergleicht seither NUR den `.fingerprint`. Die Verdrahtung bleibt trotzdem noetig und richtig:
                // das +ceb-Glied wirkt jetzt ueber das PREIMAGE (es steht in der System-Zeile = Glied [2]), also
                // aendert der Contract-Minor-Bump direkt den erwarteten Fingerprint. Der Schutz vor dem stalen
                // prae-W10-Binary ist damit staerker als vorher, nicht schwaecher: er haengt nicht mehr daran,
                // dass jemand das Glied in einen Vergleichs-String einbaut.
                // T2-B-NACHZUG (2026-08-06, C-4-Rest) -- DIE VERORTUNG WIRD RICHTIG GESTELLT: der Satz oben
                // sagt, das +ceb-Glied wirke "ueber das PREIMAGE (es steht in der System-Zeile = Glied [2])".
                // Das war schon zum Zeitpunkt des A2-Nachzugs ungenau und ist seit Format 3 falsch: ceb ist
                // ein eigenes FELD des TOOLCHAIN-Glieds [5] (abi/toolchain_stamp_glied.hpp,
                // kToolchainGliedKeys), nicht Teil der System-Zeile [2]. Die WIRKUNG bleibt exakt dieselbe
                // (ein Contract-Minor-Bump aendert den erwarteten Fingerprint), nur der Ort ist ein anderer
                // -- und seit T2-B wird genau dieses Glied per Permutation befuellt und live einkompiliert,
                // womit die Aussage nicht mehr nur richtig, sondern auch wirksam ist.
                std::string const perm_ceb = ::comdare::cache_engine::profile_facade::ceb_contract_version_text();
                perm_parts.ceb             = perm_ceb;
                std::string const perm_bt  = build_type_version_value();
                perm_parts.build_type      = perm_bt; // (i) +bt=Debug NUR bei Debug (sonst byte-identisch)
                // Ledger 70.9 / OP-7: Gate-Beitraege am ENDE, leer => kein Segment (heute leer).
                // T2-B: derselbe perm_gate, den auch das Glied [5] traegt -- oben EINMAL gebildet.
                perm_parts.gate_contribution = perm_gate;
                // T2-A/H2: OHNE per-Zelle-Bau ist der Suffix LEER -- .version-Sidecar und CSV-Provenienz
                // nennen dann exakt die lauf-konstante Identitaet, die auch wirklich gebaut wurde. Die
                // Zusammensetzung selbst bleibt die EINE Suffix-Quelle (perm_parts unveraendert gefuellt);
                // es entscheidet nur noch, OB dieser Bau ein eigenes Suffix verdient hat.
                std::string const perm_suffix =
                    perm_bau_je_zelle
                        ? ::comdare::cache_engine::profile_facade::compose_system_version_suffix(perm_parts)
                        : std::string{};
                perm_build_version     = a.build_version + perm_suffix;   // .version-Sidecar je Perm
                perm_tag_build_version = tag_build_version + perm_suffix; // CSV-Provenienz-Spalte je Perm
                // F1 / C1-Interim: DER ZELL-PFAD dieser Zelle -- aus DENSELBEN perm_parts wie der Suffix
                // (EINE Quelle, EINE Ordnung; die Wache in system_version_suffix.hpp haelt beide zusammen).
                // Er haengt an derselben Bedingung wie die Identitaet: ohne per-Zelle-Bau (H2) entsteht
                // ueberall dasselbe Binary, und dann waeren getrennte Ordner nur |opt x simd| identische
                // Nachbauten. EINE Identitaet, EIN Pfad.
                std::string const perm_zellordner =
                    perm_bau_je_zelle ? ::comdare::cache_engine::profile_facade::compose_system_zell_pfad(perm_parts)
                                      : std::string{};
                perm_output_dir = perm_zellordner.empty() ? a.dll_dir : (a.dll_dir / perm_zellordner);
                // W10-C4: die Zellwerte stehen LITERAL in der Perm-Zeile -- der Bau-Log ist damit der Beleg,
                // welches Define diese Zelle real getragen hat (Praezedenz der C-3a-Auflage: eine Identitaets-
                // Aenderung muss im Trace sichtbar sein, nicht nur im Binary). F1 zieht den Zell-Ordner in
                // dieselbe Zeile: wo die Zelle gebaut hat, ist ab jetzt aus dem Trace belegbar.
                std::cout << "  [PERM] opt=" << opt_id << " simd=" << simd_id << " flags='" << opt_flag
                          << (march_flag.empty() ? std::string{} : (" " + march_flag))
                          << "' build_version=" << perm_build_version << " zellwerte='" << perm_cell_values
                          << "' zell_pfad='" << perm_output_dir.string() << "'\n";
                run_all_passes();
            }
        }
    }

    csv.flush();
    // M11 (G5-Audit w289llo0o): nach dem Flush das Stream-Ergebnis pruefen. Ein waehrend des
    // Schreibens/Flushens aufgetretener Fehler (Platte voll, IO-Fehler) setzt failbit/badbit und
    // wuerde sonst eine still abgeschnittene CSV als Erfolg ausgeben.
    bool const csv_ok = csv.good();

    // -- A9-S5: die Mappe schliessen. HIER liegt ihr einziger Datei-Schreibvorgang -- nach der
    // Mess-Schleife, nicht darin. info_blatt() vertraegt ueberwiegend leere Felder ("n/a statt Null"):
    // hostname ist per live_hostname() erreichbar, machine_id/cpu_fabrication/ram_pair/
    // identity_verdict stammen aus der XML-Systemachsen-Deklaration und liegen in RunProfileArgs
    // NICHT vor -- sie bleiben leer und werden von der Mappe als "n/a" gerendert, nicht erfunden.
    // BEWUSST NICHT exit-wirksam in diesem Schritt: die Mappe steht additiv neben der offiziellen CSV,
    // ein Mappen-Fehler darf einen sonst gueltigen Mess-Lauf nicht rot faerben. Verdeckt ist der Zweig
    // deshalb nicht -- mappe_ok und der Grund stehen in der Zeile unten.
    // DER BESTAND IM SPEICHER, VOR dem Schliessen gelesen: danach ist das Mappen-Objekt freigegeben
    // und die Aussage waere nicht mehr zu haben. Das ist die Zeile, die einen csv-only-Lauf belegt --
    // sie sagt, wieviele Blaetter und Zeilen die xlsx-Erzeugung wirklich getragen hat, GEGEN die Zahl
    // der angebotenen Zeilen. Ohne sie waere "nur csv persistiert" von "xlsx nie gelaufen" nicht zu
    // unterscheiden.
    std::cout << "  [MAPPE-BESTAND] " << mappe.bestand() << "\n";
    bool mappe_ok = false;
    if (mappe.mappe_lebt()) {
        ::comdare::cache_engine::builder::lager_ablage::MaschinenSysinfo sysinfo;
        sysinfo.hostname = ::comdare::cache_engine::measurement::live_hostname();
        mappe_ok         = mappe.schliessen(sysinfo, {}, {});
    }
    std::cout << "  [MAPPE] ok=" << (mappe_ok ? "1" : "0") << " zeilen=" << mappe.zeilen() << "/" << mappe.angeboten()
              << " (angenommen/angeboten) verworfen=" << mappe.verworfen()
              << " feld_abweichungen=" << mappe.feld_abweichungen() << "/" << mappe.kopf_spalten()
              << " (Abweichungen/Kopfspalten) " << mappe.diagnose() << "\n";

    // D3-7b (2026-08-10, KOMMENTAR GEHEILT 2026-08-11): DER MODUS GEHOERT IN DIE BILANZ-ZEILE, weil nur
    // DIESE Funktion ihn sicher weiss. Ein Modus, den der Aufrufer danebenlegt, waere eine Behauptung
    // ueber den Lauf statt seiner Aussage.
    //
    // WAS DIESE ZEILE HEUTE WIRKLICH BEDIENT -- und was NOCH NICHT (am super-Objekt gemessen 11.08.2026,
    // Worktree /home/comdare/wt-super-landung, Zweig development, HEAD 2388afdc):
    //   ci/lauf_marker.sh zerlegt die Zeile wirklich (Feld-awk auf measured=/resumed=/provisioned=/
    //   csv_ok=) und bildet daraus modus= fuer ci/mess_ausbeute_wache.sh. ABER: er kennt genau EIN
    //   Modus-Token, "(provision-only)" (:177), und setzt daraus MODUS=voll bzw. provision_only
    //   (:238-239). Treffer fuer "pruef-only" in dieser Datei: 0. Damit die 0 kein Werkzeug-Fehler ist,
    //   die Gegenprobe im SELBEN File mit demselben Werkzeug: "provision-only" = 2 Treffer -- es beisst.
    //   Die super-Haelfte, die "(pruef-only)" liest, EXISTIERT als Arbeit -- aber nicht auf
    //   development: sie liegt allein auf refs/heads/worktree-wf_bc389245-884-2 (3 Treffer), und
    //   `git merge-base --is-ancestor` gegen development liefert dort rc=1.
    // DESHALB STEHT HIER KEIN PRAESENS: bis diese super-Haelfte gelandet ist, ist der pruef-only-Zusatz
    // fuer den Marker ein UNBEKANNTES Feld -- er wird ignoriert, nicht gelesen. Das ist die Vorbereitung
    // der Kopplung, nicht die Kopplung. Wer die super-Haelfte landet, streicht diesen Absatz.
    // (Die vorige Fassung sagte "zitiert diese Zeile und stellt daraus modus= fuer die Ausbeute-Wache"
    //  im Praesens und schrieb dem Marker ausserdem ein Fail-closed-Verhalten fuer den Doppel-Fall zu.
    //  Beides war am Objekt falsch: der Marker kennt das Token nicht, und einen Doppel-Fall-Zweig hat er
    //  nicht -- er haette schlicht das erste bekannte Token genommen und provision_only gemeldet.)
    //
    // WAS DER pruef_only-LAUF WIRKLICH TUT (B4, Selbstwiderspruch geheilt): er MISST nicht und er gatet
    // jede fertige .so -- aber er "baut nicht" ist FALSCH. cache_engine_builder_iterator.hpp haelt an
    // seinem Fallback-Kanal ausdruecklich fest, dass AUCH der pruef_only-Lauf real einen provision_all
    // faehrt, im Resume-Modus, VOR dem Pruef-Zweig; und dass gebaut_neu>0 dort ein diagnostizierbarer
    // Ausgang ist (die still negierte Lager-Skip-Entscheidung). Die Vereinfachung "baut nicht" definierte
    // genau den Fall weg, den die Nachbarstelle als Warnsignal fuehrt. Richtig ist: baut im Resume-Modus
    // nichts NEU (Soll: gebaut_neu=0), misst nicht, gatet je fertiger .so.
    // Ohne diesen Zusatz sah der pruef_only-Lauf von aussen aus wie ein Voll-Lauf, der nichts gemessen
    // hat -- die Ausbeute-Wache haette ihn mit "0 Datenzeilen" rot gefaerbt.
    //
    // BEIDE ZUSAETZE ZUGLEICH KANN ES NICHT GEBEN -- und das ist seit dem D3-7b-Riegel oben (exit_code 7)
    // eine Eigenschaft des Codes, nicht mehr eine Bitte an den Aufrufer. lauf_modus_zusatz haelt die
    // Zusage zusaetzlich an ihrer eigenen Stelle: HOECHSTENS EIN Modus-Token je Zeile, ueber alle vier
    // Schalter-Belegungen geprueft.
    // INERT bei beiden Schaltern false: der Zusatz ist dann der leere String, die Zeile byte-identisch.
    std::cout << "RUN_PROFILE fertig: basis_rows=" << res.basis_rows << " sota_rows=" << res.sota_rows
              << " (basis_ids=" << res.basis_binary_ids << " sota_ids=" << res.sota_binary_ids << ")"
              << " measured=" << res.any_measured << " resumed=" << res.any_resumed
              << " provisioned=" << res.any_provisioned << lauf_modus_zusatz(a.provision_only, a.pruef_only)
              << " csv_ok=" << (csv_ok ? "1" : "0") << " → " << a.out_csv.string() << "\n";

    // Storage #51 (Ebene C, whole-run + datierter Baum): die EINE offizielle CSV NACH dem verifizierten Flush additiv
    // an die measure-drop-Senke. No-Op-Default (leere measurement_sink) => byte-neutral. Nur bei csv_ok (keine abgeschn.
    // CSV spiegeln). SYNCHRON (alle Paesse fertig) — kein async/detached.
    if (csv_ok && a.measurement_sink) a.measurement_sink(a.out_csv, "measurements.csv");

    // Exit 0 = mind. 1 (Binary x Setting) real gemessen ODER resumiert (Voll-Resume = gueltiger Lauf)
    // UND die CSV fehlerfrei geschrieben+geflusht (M11). Ein Stream-Schreib-/Flush-Fehler erzwingt exit!=0.
    // INC-G6: im provision_only-Lauf misst NICHTS -> Erfolg = mind. 1 DLL bereitgestellt (gebaut/resumiert).
    // Ohne provision_only unveraendert (any_provisioned floss zwar mit, wird aber nur in diesem Modus gewertet).
    bool const provision_ok = a.provision_only && res.any_provisioned > 0;
    // S3 (§62-B COMDARE_PRUEF_ONLY): im Pruef-only-Lauf wird NICHT gemessen und nichts NEU gebaut -> Erfolg = mind.
    // 1 .so geprueft UND KEIN Gate-Fehler (nicht-ladbar zaehlt als Fehler). exit!=0 bei JEDEM Gate-Fail
    // (User-Vertrag). CSV irrelevant (leer).
    // DIESER ZWEIG SETZT DIE AUSSCHLIESSUNG VORAUS und darf das seit dem D3-7b-Riegel oben auch: bei
    // provision_only=1 UND pruef_only=1 kam der Lauf frueher bis hierher, nahm diesen Zweig, fand
    // any_pruef_ok==0 (der Iterator war im Provision-Zweig zurueckgekehrt, bevor ein einziges Gate lief)
    // und meldete exit 1 -- fuer eine Provisionierung, die erfolgreich war und deren Erfolgsmass
    // provision_ok (Zeile darueber) einfach uebersprungen wurde. Der Riegel bricht diese Konstellation
    // jetzt VOR dem Profil-Parse ab; hier ist a.pruef_only deshalb eindeutig.
    if (a.pruef_only) {
        res.exit_code = (res.any_pruef_ok > 0 && res.any_pruef_failed == 0) ? 0 : 1;
        return res;
    }
    res.exit_code = (((res.any_measured > 0 || res.any_resumed > 0) || provision_ok) && csv_ok) ? 0 : 1;
    return res;
}

} // namespace comdare::cache_engine::thesis_lazy
