// SPDX-License-Identifier: Apache-2.0
// V41.F.6.1 R5.D — comdare-f15-compare: CLI-Treiber fuer den F15-Messlauf ueber materialisierte
// Permutations-DLLs.
//
// Verpackt die in tests/unit/test_v41_anatomy_adhoc_autobuilt_load.cpp bewiesene Treiber-Kette als
// echtes Kommandozeilen-Tool: ein Verzeichnis voller dlopen-faehiger Anatomie-DLLs (z.B. die vom
// R5.G-Auto-Emitter gebauten Permutationen) wird geladen, jede DLL via IMeasurableWorkload::run_workload
// (Stufe B, in-DLL) gemessen, gegen eine Baseline verglichen (Welch + Holm-FWER) und als Summary +
// optional CSV/JSON ausgegeben. Beantwortet die F15-Kernfrage "bringt die CacheEngine messbaren Wert?".
//
// Aufruf:
//   comdare-f15-compare <dll-dir> [--alpha=0.05] [--baseline=0] [--ops=2000] [--batches=128]
//                                 [--seed=11] [--csv=out.csv] [--json=out.json]

#include <builder/anatomy_module_loader/anatomy_module_loader.hpp>
#include <anatomy/anatomy_base.hpp>
#include <anatomy/measurable_workload.hpp>
#include <builder/commands/multi_compare.hpp>
#include <builder/commands/hdr_perzentil_auswertung.hpp> // D5-5: HDR neben dem Kanon (Auswertung)
#include <builder/commands/lade_bilanz.hpp> // D4e: Bilanz des Mess-Laufs (Nenner + Summenprobe + Exit-Codes)
#include <builder/commands/result_aggregator.hpp>
// V41 OpenDone.2 — Pfad-B Prüf-Dock-Observe-Modus (Option B: Standalone-CLI mit measure_genus_sequential).
#include <builder/pruef_dock/pruef_dock.hpp>
#include <builder/pruef_dock/search_algorithm_dock.hpp>
// E-24 C4 (d/5): die Voll-Belegung der Dock-Registry ueber alle FUENF Gattungen. Ab hier misst
// f15 --observe nicht mehr nur SearchAlgorithm-DLLs, sondern jede Gattung ueber ihr eigenes Dock.
// Die Liste selbst steht bewusst NICHT hier, sondern in register_all_genus_docks() -- damit sie
// von einer Test-TU geprueft werden kann und nicht als zweite, driftende Liste in einer main() lebt.
#include <builder/pruef_dock/pruef_dock_registry_default.hpp>
#include <builder/pruef_dock/pruef_dock_registry.hpp>
#include <builder/pruef_dock/pruef_dock_sequencer.hpp>
// E-24 C5 (FK-7): die CEB-seitige Klassifizierung der Dock-/Lade-Fehlerpfade. Ohne sie endete ein
// status != ok hier in einer blossen Konsolen-Zeile -- ohne CSV-Datensatz und ohne stabiles Etikett.
#include <builder/pruef_dock/dock_error_classification.hpp>
#include <builder/pruef_dock/conformance_gate.hpp> // V5: std::map-Konformitäts-Gate vor Messung
// V5-I9/I10: host-seitiger Lastprofil-Orchestrator (mehrere Lastprofile je Binary, Zwei-Phasen-Messung).
#include <builder/workload_driver/workload_orchestrator.hpp>
#include <builder/workload_driver/workload_profiles.hpp> // INC-0: Single-Source profile_by_name (geteilt mit perm_runner)
#include <builder/measurement_snapshot.hpp> // V5-I1 (#50): EIN autoritativer 16+6-Mess-POD + Pipeline-16-Serializer
#include <cache_engine/measurement/pipeline_csv_schema.hpp> // B-3: DIE EINE Pipeline-Spaltenliste (direkt, nicht transitiv)
#include <builder/provenance_manifest.hpp> // AP-9/#243: host-seitiges Provenance-Sidecar (Compiler/Flags/ISA/Allokator/4-Repo-git-SHAs)
#include <builder/measurement/thread_pinning.hpp> // AP-13/#247: opt-in Mess-Thread-Pinning (--pin-core=N), host-seitig, Default no-op
#include <builder/pmc_source_factory.hpp> // V5-#26 / Task #153: make_pmc_source() (Windows-Intel-PCM o. NullPmcSource)
#include <anatomy/observable_tier.hpp>
#include <anatomy/rollbackable_tier.hpp>
#include <anatomy/scannable_tier.hpp> // V5-#49-E: Range-Scan-Sub-Interface (YCSB-E)
// F5.R1 (#35): erster echter Konsument der Master-Framework-Gesamt-Fassade (get_cache_engine() ueber die EINE Tuer).
#include <cache_engine/api/i_cache_engine.hpp>

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace loader     = ::comdare::cache_engine::builder::anatomy_loader;
namespace ana        = ::comdare::cache_engine::anatomy;
namespace cmd        = ::comdare::cache_engine::builder::commands;
namespace stats      = ::comdare::cache_engine::builder::commands::stats;
namespace auswertung = ::comdare::cache_engine::builder::commands::auswertung; // D5-5: HDR neben dem Kanon
namespace wd         = ::comdare::cache_engine::builder::workload_driver;
namespace pd         = ::comdare::cache_engine::builder::pruef_dock;
namespace bld        = ::comdare::cache_engine::builder;     // V5-I1 (#50): ComdareMeasurementSnapshotV1 + Serializer
namespace meas       = ::comdare::cache_engine::measurement; // A2-Neben Stufe 1: PmcCounters/IPmcSource

namespace {

/// Bildet einen Lastprofil-Namen (CLI-Token) auf eine WorkloadConfig ab. Unbekannt → name leer (Skip).
/// INC-0 (2026-06-07): NICHT mehr lokal dupliziert — Single-Source workload_driver::profile_by_name
/// (builder/workload_driver/workload_profiles.hpp), geteilt mit dem FullPilot-B+-Baum-Mess-Pfad (perm_runner).
using wd::profile_by_name;

void print_usage() {
    std::cerr << "Usage: comdare-f15-compare [<dll-dir>] [options]\n"
              << "  <dll-dir>   Verzeichnis mit Anatomie-Permutations-DLLs (comdare_anatomy_perm_*).\n"
              << "              Fehlt es, wird die Umgebungsvariable COMDARE_PERM_ROOT verwendet (B5-Discovery).\n"
              << "Optionen:\n"
              << "  --alpha=F      Signifikanz-Niveau (FWER), Default 0.05\n"
              << "  --baseline=N   Index der Baseline-DLL, Default 0\n"
              << "  --ops=N        ops_per_batch, Default 2000\n"
              << "  --batches=N    Batches (= Latenz-Samples pro DLL), Default 128\n"
              << "  --seed=N       RNG-Seed, Default 11\n"
              << "  --pin-core=N   Mess-Thread optional auf Core N pinnen (Default: no-op)\n"
              << "  --csv=PATH     Report zusaetzlich als CSV schreiben\n"
              << "  --json=PATH    Report zusaetzlich als JSON schreiben\n"
              << "  --observe      Pfad-B Prüf-Dock-Modus: jede DLL ueber das gattungs-passende Prüf-Dock\n"
              << "                 (measure_genus_sequential) messen + Observer-Trace (CSV+JSON) je DLL schreiben.\n"
              << "                 Funktioniert ab 1 DLL (kein Baseline-Vergleich). Statt des Pfad-A-Vergleichs.\n"
              << "  --observe-out=DIR  Ausgabe-Verzeichnis fuer die je-DLL Observer-Traces (Default: .)\n"
              << "  --measurement-plan=A,B,C,D  V5 Lastprofil-Plan: jede DLL host-seitig gegen mehrere YCSB-\n"
              << "                 Lastprofile messen (Zwei-Phasen via memento_all). Profile: A=mixed_a B=mixed_b\n"
              << "                 C=ycsb_c(read-only) D=ycsb_d(read-latest) IH=insert_heavy LH=lookup_heavy.\n"
              << "                 Nutzt --ops (Ops je Profil) + --seed; Konformitaets-Gate vor jeder Messung;\n"
              << "                 schreibt <comp>.plan.csv je DLL nach --observe-out. Funktioniert ab 1 DLL.\n";
}

bool parse_flag_u64(std::string_view arg, std::string_view key, std::uint64_t& out) {
    if (!arg.starts_with(key)) return false;
    arg.remove_prefix(key.size());
    std::uint64_t v{};
    auto const*   end = arg.data() + arg.size();
    auto [p, ec]      = std::from_chars(arg.data(), end, v);
    if (ec == std::errc{} && p == end) {
        out = v;
        return true;
    }
    return false;
}

bool parse_flag_double(std::string_view arg, std::string_view key, double& out) {
    if (!arg.starts_with(key)) return false;
    out = std::strtod(std::string{arg.substr(key.size())}.c_str(), nullptr);
    return true;
}

bool parse_flag_str(std::string_view arg, std::string_view key, std::string& out) {
    if (!arg.starts_with(key)) return false;
    out = std::string{arg.substr(key.size())};
    return true;
}

bool write_text_file(std::string const& path, std::string const& content) {
    std::ofstream os(path, std::ios::binary | std::ios::trunc);
    if (!os) return false;
    os.write(content.data(), static_cast<std::streamsize>(content.size()));
    return os.good();
}

} // namespace

int main(int argc, char** argv) {
    // F5.R1 (#35): das README-Beispiel eingelöst — ein minimaler REALER Aufruf über die Fassade beim Start.
    // Additive stderr-Diagnose (wie die uebrigen cerr-Meldungen); die eigentliche Vergleichs-Ausgabe bleibt
    // unveraendert. Belegt, dass get_cache_engine() in einem ECHTEN Binary linkt + laeuft (Linker-Falle zu).
    {
        auto&      ce  = ::comdare::cache_engine::api::get_cache_engine();
        auto const isa = ce.isa_dispatch().detect();
        std::cerr << "[cache-engine] Framework " << ce.framework_version() << " | ISA "
                  << (isa.vendor.empty() ? "?" : isa.vendor) << " cache-line=" << isa.cache_line_bytes << "B"
                  << " | System-Mess-Achsen=" << ce.measurement().system_axis_count()
                  << " | deferred(#274)=" << ce.deferred_providers().size() << "\n";
    }

    // B5 Plugin-Discovery: erstes Positions-Argument ist das DLL-Verzeichnis; fehlt es (oder beginnt
    // direkt mit einer Option), faellt die CLI auf die Umgebungsvariable COMDARE_PERM_ROOT zurueck.
    std::string dll_dir_str;
    int         first_opt = 1;
    if (argc >= 2 && !std::string_view{argv[1]}.starts_with("--")) {
        dll_dir_str = argv[1];
        first_opt   = 2;
    } else if (char const* env = std::getenv("COMDARE_PERM_ROOT"); env != nullptr) {
        dll_dir_str = env;
    }
    if (dll_dir_str.empty()) {
        std::cerr << "Kein DLL-Verzeichnis angegeben (Argument oder Umgebungsvariable COMDARE_PERM_ROOT).\n";
        print_usage();
        return 1;
    }
    std::filesystem::path const dll_dir{dll_dir_str};

    double                  alpha    = 0.05;
    std::uint64_t           baseline = 0, ops = 2000, batches = 128, seed = 11;
    std::string             csv_path, json_path, observe_out, pipeline_csv;
    std::string             workload_label = "micro";
    std::string             measurement_plan; // V5-I10: --measurement-plan=A,B,C,D (host-seitige Lastprofile je Binary)
    bool                    observe = false;
    std::optional<unsigned> pin_core; // AP-13/#247: opt-in Mess-Thread-Pinning (--pin-core=N); leer => kein Pinning
    for (int i = first_opt; i < argc; ++i) {
        std::string_view a{argv[i]};
        std::uint64_t    u{};
        if (a == "--observe") {
            observe = true;
        } else if (parse_flag_double(a, "--alpha=", alpha)) {
        } else if (parse_flag_u64(a, "--baseline=", u)) {
            baseline = u;
        } else if (parse_flag_u64(a, "--ops=", u)) {
            ops = u;
        } else if (parse_flag_u64(a, "--batches=", u)) {
            batches = u;
        } else if (parse_flag_u64(a, "--seed=", u)) {
            seed = u;
        } else if (parse_flag_str(a, "--csv=", csv_path)) {
        } else if (parse_flag_str(a, "--json=", json_path)) {
        }
        // V41.P3-Bridge: 16-Spalten-Pipeline-Mess-CSV (eine Zeile je Composition; total_cycles=ns-Latenz,
        // wie Stufe-05-Konvention) — speist die LaTeX-Pipeline (03/04/05) mit ECHTEN Mess-Zahlen.
        else if (parse_flag_str(a, "--pipeline-csv=", pipeline_csv)) {
        } else if (parse_flag_str(a, "--workload=", workload_label)) {
        } else if (parse_flag_str(a, "--observe-out=", observe_out)) {
        } else if (parse_flag_str(a, "--measurement-plan=", measurement_plan)) {
        } else if (parse_flag_u64(a, "--pin-core=", u)) {
            if (u <= static_cast<std::uint64_t>(std::numeric_limits<unsigned>::max())) {
                pin_core = static_cast<unsigned>(u);
            } else {
                pin_core.reset();
            }
        } else {
            std::cerr << "Unbekannte Option: " << a << "\n";
            print_usage();
            return 1;
        }
    }
    if (batches == 0) batches = 1;

    std::vector<loader::AnatomyModuleHandle> handles;
    int const                                st = loader::AnatomyModuleLoader::load_all(dll_dir, handles);
    if (st != loader::status_ok) {
        // E-24 C5 / FK-7: auch der LADE-Fehler wird klassifiziert gemeldet (er ist der frueheste
        // Mess-Eintritts-Fehlerpfad ueberhaupt) -- stabiles Etikett statt nur des rohen Transport-Namens.
        // Die Zeile daneben bleibt woertlich stehen: sie ist der Bestands-Text, den Skripte greppen.
        std::cerr << "load_all fehlgeschlagen: " << loader::status_name(st) << " (dir=" << dll_dir << ")\n";
        std::cerr << pd::dock_failure_log_line(
                         pd::classify_loader_status(st).value_or(pd::cem::DockErrorClass::ModulLadeFehler),
                         dll_dir.string(), loader::status_name(st))
                  << "\n";
        return 2;
    }

    auto measurement_pin = pin_core.has_value() ? bld::CorePinPolicy{*pin_core}.pin() : bld::NoPinPolicy{}.pin();
    // Die Wache steht auf has_value() UND active(), nicht auf active() allein. Bisher trug den
    // `*pin_core`-Zugriff nur eine Zusicherung aus einem ANDEREN Header: NoPinPolicy::pin() liefert
    // einen default-konstruierten ScopedThreadPin (active_{false}), also folgte aus active() faktisch
    // has_value(). Faktisch -- nicht strukturell. Gaebe NoPinPolicy je einen aktiven Pin zurueck
    // (etwa ein Default-Pinning), laese diese Zeile ein LEERES optional aus: undefiniertes Verhalten.
    // GCC meldet genau das ab -O3, wo das Inlining die Analyse traegt: "-Wmaybe-uninitialized" auf
    // dem _M_payload des optional. Im Debug-Bau ist die Stelle UNSICHTBAR -- sie ist der Beleg
    // dafuer, warum beide Bauweisen gefahren werden muessen.
    if (pin_core.has_value() && measurement_pin.active()) {
        std::cout << "AP-13 Mess-Thread auf Core " << *pin_core << " gepinnt.\n";
    }

    // ── OpenDone.2 / Pfad B — Prüf-Dock-Observe-Modus (Option B: Standalone-CLI mit measure_genus_sequential).
    // Wireht den (in test_v41_anatomy_adhoc_dll_load bewiesenen) Prüf-Dock-Mess-Pfad als CLI: jede REAL
    // dlopen-geladene DLL wird gattungs-sequentiell über ihr passendes Dock gemessen (IObservableTier →
    // drive_tier_observe_trace_abi → CSV/JSON), die Observer-Traces je DLL persistiert. Funktioniert ab 1 DLL.
    if (observe) {
        namespace pd = ::comdare::cache_engine::builder::pruef_dock;
        pd::PruefDockRegistry reg;
        // E-24 C4 (d/5) -- ALLE FUENF Gattungen. Bis hierher war dies die einzige produktive
        // register_dock-Stelle im Baum (Bauplan Paragraf 1.0 / OP-4-Rest) und sie kannte genau EIN
        // Dock: eine geladene Set-/Sequence-/Adapter-/View-DLL fiel deshalb in select_for() durch
        // und wurde als "kein passendes Dock" gemeldet, statt gemessen zu werden.
        pd::register_all_genus_docks(reg);

        // Checkpoints inkl. 1000 (> typischer Fixed-Capacity-Tier): der drive_tier_observe_trace_abi-
        // Stagnations-Guard (max_insert_stagnation) bricht die WRITE-Phase an der effektiven Tier-Kapazität
        // ab statt zu haengen → der Checkpoint zeigt dann den Kapazitäts-Füllstand. Robust gegen jede Permutation.
        pd::PruefDockMeasureOptions opts;
        opts.fill_checkpoints       = {50, 200, 1000};
        opts.lookups_per_checkpoint = ops;
        opts.deletes_per_checkpoint = ops / 10;

        auto const results = pd::measure_genus_sequential(reg, handles, opts); // sortiert handles in-place
        std::filesystem::path const out_dir =
            observe_out.empty() ? std::filesystem::path{"."} : std::filesystem::path{observe_out};
        std::cout << "F15 Pfad-B Prüf-Dock-Observe über " << results.size()
                  << " DLL(s) (gattungs-sequentiell, out=" << out_dir.string() << "):\n";
        int ok = 0;
        for (std::size_t i = 0; i < results.size(); ++i) {
            auto const&       r  = results[i];
            std::string const nm = (i < handles.size() && handles[i].anatomy() != nullptr)
                                       ? std::string{handles[i].anatomy()->composition_name()} + "_" + std::to_string(i)
                                       : std::string{"module_"} + std::to_string(i);
            std::cout << "  [" << i << "] " << nm << "  dock=" << (r.dock_name.empty() ? "<none>" : r.dock_name)
                      << "  genus=" << static_cast<int>(r.genus) << "  status=" << pd::dock_status_name(r.status)
                      << "\n";
            if (r.status == pd::dock_status_ok) {
                std::filesystem::path const csv_p  = out_dir / (nm + ".observe.csv");
                std::filesystem::path const json_p = out_dir / (nm + ".observe.json");
                if (write_text_file(csv_p.string(), r.csv) && write_text_file(json_p.string(), r.json)) {
                    std::cout << "        CSV -> " << csv_p.string() << "  JSON -> " << json_p.string() << "\n";
                    ++ok;
                } else {
                    std::cerr << "        Trace-Schreiben fehlgeschlagen (" << nm << ")\n";
                }
            } else {
                // E-24 C5 / FK-7 (Bauplan Paragraf 6.2): DER FEHLERPFAD BEKOMMT EINEN DATENSATZ.
                // Bis hierher endete ein status != ok genau in der Konsolen-Zeile darueber: keine CSV,
                // keine klassifizierte Log-Zeile. Die Auswertung sah eine FEHLENDE Datei -- also ein
                // Nichts -- statt eines Fehlschlags, und das ist der Fall, den die Mess-Fehler-Doktrin
                // ausdruecklich verbietet ("failed"-Zelle + Log, NIE eine stille 0/null).
                // Der Datensatz traegt dieselbe .observe.csv-Namensform wie der Erfolgsfall: eine
                // Auswertung, die je Modul eine Datei erwartet, findet sie jetzt IMMER -- mit "failed"
                // darin statt gar nicht. Exit-Code-Semantik unveraendert (ok zaehlt weiterhin nur Erfolge).
                pd::cem::DockErrorClass const klasse =
                    r.error_class.value_or(pd::cem::DockErrorClass::UnbekannterDockStatus);
                std::cerr << "        " << pd::dock_failure_log_line(klasse, nm, pd::dock_status_name(r.status))
                          << "\n";
                std::filesystem::path const fail_p = out_dir / (nm + ".observe.csv");
                if (!write_text_file(fail_p.string(), pd::dock_failure_csv(klasse, nm, pd::dock_status_name(r.status))))
                    std::cerr << "        Fehler-Datensatz nicht schreibbar (" << nm << ")\n";
                else
                    std::cout << "        FAILED-CSV -> " << fail_p.string() << "\n";
            }
        }
        std::cout << "  " << ok << "/" << results.size() << " Modul(e) erfolgreich observiert (Pfad B).\n";
        return (ok > 0) ? 0 : 5;
    }

    // ── V5-I10 — Lastprofil-Plan-Modus: host-seitiger WorkloadOrchestrator (mehrere Lastprofile je Binary).
    // Pro REAL geladener DLL: Konformitäts-Gate (import→GATE→messen) → run_measurement_plan (jedes YCSB-Profil,
    // jede Op ZWEI-PHASIG via memento_all wenn IRollbackableTier vorhanden) → WorkloadRunResult-CSV je DLL.
    // Das ist die host-relokalisierte IMeasurableWorkload-Achse (run_workload-in-DLL = V3-Designfehler behoben).
    // Hinweis: run_measurement_plan leert den Tier je Profil → reine Read-Profile (YCSB-C) messen gegen den
    // leeren Tier (Testdaten-Vorladung pro Profil = Lastprofil-Design-Refinement, separater Punkt).
    if (!measurement_plan.empty()) {
        wd::MeasurementPlan plan;
        std::string_view    rest{measurement_plan};
        while (!rest.empty()) {
            auto const       comma = rest.find(',');
            std::string_view tok   = (comma == std::string_view::npos) ? rest : rest.substr(0, comma);
            if (!tok.empty()) {
                auto cfg = profile_by_name(tok, seed, ops);
                if (cfg.name.empty()) {
                    std::cerr << "Unbekanntes Lastprofil: " << tok << "\n";
                    print_usage();
                    return 1;
                }
                plan.profiles.push_back(cfg);
            }
            if (comma == std::string_view::npos) break;
            rest.remove_prefix(comma + 1);
        }
        if (plan.profiles.empty()) {
            std::cerr << "--measurement-plan: keine gueltigen Profile.\n";
            return 1;
        }

        std::filesystem::path const out_dir =
            observe_out.empty() ? std::filesystem::path{"."} : std::filesystem::path{observe_out};
        std::cout << "F15 Lastprofil-Plan (" << plan.profiles.size() << " Profile) ueber " << handles.size()
                  << " DLL(s) (host-seitig, zwei-phasig, out=" << out_dir.string() << "):\n";
        // V5-I1 (#50): autoritative 16+6-Mess-POD-Zeilen sammeln (eine je Komposition×Lastprofil) → Pipeline-Bridge.
        std::vector<bld::ComdareMeasurementSnapshotV1> snaps;
        std::vector<std::string>                       snap_ids, snap_wls;
        // V5-#26 / Task #153: die EINE PMC-Quelle (Factory wählt build-/OS-abhängig: Windows-Intel-PCM unter
        // COMDARE_ENABLE_PMC, sonst NullPmcSource → available=false → HW-Spalten ehrlich 0). begin()/end()
        // klammern den gemessenen Lauf je Komposition; das Counter-Delta speist die +6 HW-Spalten.
        std::unique_ptr<meas::IPmcSource> pmc = bld::make_pmc_source();
        int                               ok  = 0;
        for (std::size_t i = 0; i < handles.size(); ++i) {
            auto* base = handles[i].anatomy();
            if (base == nullptr) continue;
            auto* tier = dynamic_cast<ana::IObservableTier*>(base);
            if (tier == nullptr) {
                std::cerr << "  [" << i << "] kein IObservableTier — uebersprungen.\n";
                continue;
            }
            std::string const nm = std::string{base->composition_name()} + "_" + std::to_string(i);
            if (!pd::run_conformance_gate(*tier).passed()) {
                std::cerr << "  [" << i << "] " << nm << "  KONFORMITAET FEHLGESCHLAGEN — nicht gemessen.\n";
                continue;
            }
            auto* rb = dynamic_cast<ana::IRollbackableTier*>(base); // nullptr → Kalt-Messung (kein Rollback)
            auto* sc = dynamic_cast<ana::IScannableTier*>(base);    // V5-#49-E: nullptr → YCSB-E-Scan-Ops übersprungen
            pmc->begin();                                           // V5-#26: HW-Counter-Snapshot vor dem Mess-Lauf
            auto const                  results   = wd::run_measurement_plan(*tier, rb, plan, sc);
            auto const                  pmc_delta = pmc->end(); // Delta (available=false bei NullPmcSource)
            auto const                  csv       = wd::serialize_workload_run_results_csv(results);
            std::filesystem::path const csv_p     = out_dir / (nm + ".plan.csv");
            if (write_text_file(csv_p.string(), csv)) {
                std::cout << "  [" << i << "] " << nm << "  two_phase=" << (rb != nullptr)
                          << "  Profile=" << results.size() << "  CSV -> " << csv_p.string() << "\n";
                ++ok;
            } else {
                std::cerr << "  [" << i << "] " << nm << "  CSV-Schreiben fehlgeschlagen.\n";
            }
            // POD je (Komposition × Lastprofil): echte Observer-Daten in die 16 Kern-Spalten; workload_used =
            // Profil-Name (steuert die PDF-Diagramm-Gruppierung). Die +6 Observer-Spalten + pmc_available
            // EHRLICH aus der PMC-Quelle: reale L2/L3-Cache-Misses unter COMDARE_ENABLE_PMC, sonst 0.
            for (auto const& res : results) {
                snaps.push_back(bld::measurement_from_workload_result(res, nm, pmc_delta));
                snap_ids.push_back(nm);
                snap_wls.push_back(res.profile_name);
            }
        }
        // V5-I1 (#50) Bridge: --pipeline-csv schreibt die 16-col-Pipeline-Sicht aus dem autoritativen POD →
        // Stufe 04/05/06 → PDF (echte V5-Zwei-Phasen-Daten, per Lastprofil gruppiert). Re-Audit-Blocker 1+2.
        if (!pipeline_csv.empty()) {
            auto const p16 = bld::serialize_measurements_pipeline16_csv(snaps, snap_ids, snap_wls);
            if (write_text_file(pipeline_csv, p16))
                std::cout << "  Pipeline-CSV (16-col aus autoritativem 16+6-POD, " << snaps.size() << " Zeilen) -> "
                          << pipeline_csv << "\n";
            else {
                std::cerr << "Pipeline-CSV-Schreiben fehlgeschlagen: " << pipeline_csv << "\n";
                return 4;
            }
        }
        // AP-9/#243: Provenance-Manifest (Compiler/Flags/ISA/Allokator/4-Repo-Commit-Hashes) einmal je Mess-Lauf als
        // Sidecar neben die CSVs — Reproduzierbarkeit; host-seitig, berührt weder POD noch die 16/25-CSV-Spalten.
        {
            std::filesystem::path const prov_p = out_dir / "provenance.txt";
            if (bld::write_provenance_manifest(prov_p))
                std::cout << "  Provenance-Manifest -> " << prov_p.string() << "\n";
            else
                std::cerr << "  Provenance-Manifest-Schreiben fehlgeschlagen: " << prov_p.string() << "\n";
        }
        std::cout << "  " << ok << "/" << handles.size() << " DLL(s) per Lastprofil-Plan gemessen.\n";
        return (ok > 0) ? 0 : 5;
    }

    if (handles.size() < 2) {
        std::cerr << "Brauche >= 2 DLLs fuer einen Vergleich (geladen: " << handles.size() << ")\n";
        return 3;
    }

    // Jede geladene DLL messen → ExecutionResult. names reserviert (string_view-Stabilitaet!).
    std::vector<std::string> names;
    names.reserve(handles.size());
    std::vector<cmd::ExecutionResult> results;
    results.reserve(handles.size());
    // D4e: die Lade-Schleife hatte drei benannte Ausschluss-Gruende und keinen einzigen Zaehler.
    // Am Ende stand nur "N gemessen" -- ohne Grundgesamtheit. Ab hier wird JEDER Pfad verbucht,
    // und die Summenprobe A+B+C+D+gemessen == geladen deckt einen vergessenen Pfad auf.
    cmd::LadeBilanz bilanz{};
    bilanz.geladen = handles.size();
    for (std::size_t i = 0; i < handles.size(); ++i) {
        auto* base = handles[i].anatomy();
        auto* mw   = dynamic_cast<ana::IMeasurableWorkload*>(base);
        if (mw == nullptr) {
            std::cerr << "DLL " << i << " nicht mess-faehig — uebersprungen\n";
            ++bilanz.nicht_messfaehig;
            continue;
        }
        // V5-Audit-Fix (Konformitäts-Gate-Pflicht, Mess-Architektur): JEDE Tier-Binary besteht VOR der Messung
        // das std::map-Hüllen-Gate — auch der Pfad-A-Multi-Compare (vorher ungegateter Mess-Eintrittspunkt).
        // import → GATE → messen. dt = IDriveableTier-Sicht (Gate treibt den funktionalen Antrieb, leert am Ende).
        auto* dt = dynamic_cast<ana::IDriveableTier*>(base);
        if (dt == nullptr || !pd::run_conformance_gate(*dt).passed()) {
            std::cerr << "DLL " << i << " KONFORMITAET FEHLGESCHLAGEN — nicht gemessen\n";
            ++bilanz.konformitaet_fehlt;
            continue;
        }
        std::vector<std::int64_t> samples(static_cast<std::size_t>(batches));
        auto const                n = mw->run_workload(ops, batches, seed, samples.data(), samples.size());
        if (n < 2) {
            std::cerr << "DLL " << i << " lieferte < 2 Samples — uebersprungen\n";
            ++bilanz.zu_wenige_proben;
            continue;
        }
        samples.resize(static_cast<std::size_t>(n));
        std::string nm = std::string{handles[i].anatomy()->composition_name()} + "_" + std::to_string(i);
        // D4e: die VIERTE Gruppe. Ohne sie kommt ein Ergebnis aus lauter Nullen in `results` und
        // GEWINNT anschliessend das p50-Ranking -- eine praeparierte Null-DLL waere die schnellste
        // Komposition der Messung. Die Entscheidung faellt NICHT hier, sondern in
        // make_execution_result (D4d): die CLI liest sie nur, damit es genau eine Definition gibt.
        auto ergebnis = cmd::make_execution_result(nm, std::move(samples));
        if (ergebnis.degeneriert) {
            std::cerr << "DLL " << i << " lieferte TOTE PROBEN (keine einzige > 0 ns) — nicht gewertet\n";
            ++bilanz.tote_proben;
            continue;
        }
        names.push_back(std::move(nm));
        ergebnis.engine_name = names.back(); // string_view auf den stabilen Speicher umhaengen
        results.push_back(std::move(ergebnis));
        ++bilanz.gemessen;
    }
    std::cout << "  " << bilanz.zeile() << "\n";
    if (!bilanz.summe_stimmt()) {
        std::cerr << "BILANZ GEHT NICHT AUF — die Lade-Schleife hat einen unverbuchten Pfad.\n";
        return cmd::kExitBilanzGehtNichtAuf;
    }
    if (results.size() < 2) {
        std::cerr << "Weniger als 2 mess-faehige DLLs — Abbruch.\n";
        return 3;
    }

    std::size_t const                 base_idx = (baseline < results.size()) ? static_cast<std::size_t>(baseline) : 0;
    std::vector<cmd::ExecutionResult> candidates;
    candidates.reserve(results.size() - 1);
    for (std::size_t i = 0; i < results.size(); ++i)
        if (i != base_idx) candidates.push_back(results[i]);

    auto const rep = stats::multi_compare_against_baseline(results[base_idx],
                                                           std::span<const cmd::ExecutionResult>{candidates}, alpha);
    auto const sum = stats::summarize(rep);

    std::cout << "F15 multi-compare ueber " << results.size() << " DLLs (baseline=" << names[base_idx]
              << ", alpha=" << alpha << ", samples/DLL=" << batches << ")\n";
    std::cout << "  significant_faster=" << sum.significant_faster << "  significant_slower=" << sum.significant_slower
              << "  not_significant=" << sum.not_significant << "  degeneriert=" << sum.degeneriert << "\n";
    // D4c: die Headline-Zahl steht NIE ohne ihre Grundgesamtheit. Der Text kommt aus
    // win_rate_zeile(), damit CLI, CSV und Bericht dieselbe Formulierung tragen.
    std::cout << "  win_rate=" << stats::win_rate_zeile(sum) << "\n";
    // R5.D — robuster Rang-Test (Mann-Whitney-U) als Gegenprobe zum parametrischen Welch.
    std::cout << "  robust_significant(MWU)=" << sum.robust_significant
              << "  discrepancies(Welch<->MWU)=" << sum.discrepancies
              << (sum.discrepancies > 0 ? "  <- ausreisser-verdaechtige Welch-Befunde" : "") << "\n";
    for (auto const& c : rep.comparisons) {
        std::cout << "    " << c.name << ": welch_p=" << c.adjusted_p << (c.significant ? "  SIGNIFIKANT" : "  n.s.")
                  << "  | mwu_p=" << c.robust_adjusted_p << (c.robust_significant ? "  ROBUST-SIG" : "  robust-n.s.")
                  << "  delta=" << c.mwu.cliff_delta << "(" << stats::cliff_delta_magnitude(c.mwu.cliff_delta) << ")"
                  << (c.faster_than_baseline ? "  (schneller)" : "  (langsamer)")
                  << (c.significance_discrepancy ? "  [DISKREPANZ]" : "") << "\n";
    }

    // Ranking ueber ALLE Kompositionen (inkl. Baseline) nach MEDIAN (p50) statt Mittelwert — die
    // praktische F15-Antwort "welche Achsen-Komposition gewinnt?". Der Median ist ROBUST gegen
    // Wall-Clock-Ausreisser: einzelne Batch-Spitzen (Scheduler-Preemption, Page-Faults, Turbo-Takt-
    // Schwankungen) verzerren den Mittelwert massiv (beobachtet: bis ~10× Ausreisser), nicht aber den
    // Median. mean wird zum Vergleich mit ausgewiesen. Schnellste zuerst.
    std::vector<std::pair<double, std::size_t>> ranking;
    ranking.reserve(results.size());
    for (std::size_t i = 0; i < results.size(); ++i) {
        double const p50 = static_cast<double>(
            stats::latency_p50_ns(std::span<const std::int64_t>{results[i].latency_samples_ns}).count());
        ranking.emplace_back(p50, i);
    }
    std::sort(ranking.begin(), ranking.end(), [](auto const& a, auto const& b) { return a.first < b.first; });
    std::cout << "  Ranking (schnellste zuerst, robuster Median p50 ns; mean zum Vergleich):\n";
    for (std::size_t r = 0; r < ranking.size(); ++r) {
        auto const [p50, idx] = ranking[r];
        double const mean     = stats::latency_mean_ns(std::span<const std::int64_t>{results[idx].latency_samples_ns});
        std::cout << "    #" << (r + 1) << "  " << names[idx] << "  p50=" << p50 << " ns"
                  << "  mean=" << mean << " ns" << (idx == base_idx ? "  [baseline]" : "") << "\n";
    }
    if (ranking.size() >= 2 && ranking.front().first > 0.0) {
        std::cout << "  Spanne langsamste/schnellste (p50) = " << (ranking.back().first / ranking.front().first)
                  << "x\n";
    }

    // == D5-5 (2026-08-09): HDR-PERZENTILE NEBEN DEM KANON ========================================
    // Thesis-Zusage (03_messsystem_prtart.tex + 05_evaluation.tex, Praesens): die Perzentile werden
    // ueber HDR-Histogramme bestimmt. Hier ist der Produktions-Konsument dafuer -- in der
    // AUSWERTUNG, nachdem oben schon stumpf real gemessen wurde, nicht im Messpfad.
    //
    // Es ersetzt den Kanon NICHT: das Ranking oben bleibt auf stats::latency_p50_ns. HDR steht
    // DANEBEN, zusammen mit der Abweichung und dem Urteil gegen die aus significant_figures
    // hergeleitete Toleranz. Eine Abweichung ausserhalb der Toleranz ist ein BEFUND fuer den
    // Auswerter (Rang-Divergenz an einem Verteilungssprung), kein Abbruchgrund -- deshalb wird sie
    // ausgewiesen und nicht als Fehler zurueckgegeben.
    std::cout << "  HDR-Perzentile (Toleranz " << (auswertung::GeltendeGeometrie::kRelativeToleranz * 100.0)
              << " % aus significant_figures=" << meas::LatencyHdrHistogram::kSignifikanteStellen << " hergeleitet):\n";
    for (std::size_t i = 0; i < results.size(); ++i) {
        auto const a = auswertung::hdr_auswerten(std::span<const std::int64_t>{results[i].latency_samples_ns});
        std::cout << "    " << names[i] << "  p50: kanon=" << a.p50.kanon_ns << " hdr=" << a.p50.hdr_ns
                  << "  p95: kanon=" << a.p95.kanon_ns << " hdr=" << a.p95.hdr_ns << "  p99: kanon=" << a.p99.kanon_ns
                  << " hdr=" << a.p99.hdr_ns << "\n";
        // Der Verwurf bekommt IMMER seinen Nenner -- eine Zahl ohne Grundgesamtheit ist unlesbar.
        if (a.verworfen_null != 0 || a.verworfen_negativ != 0 || a.verworfen_ausserhalb != 0) {
            std::cout << "      VERWURF von " << a.vorgelegt << " Proben: " << a.verworfen_null
                      << "x 0ns (unter Uhr-Aufloesung), " << a.verworfen_negativ << "x negativ (DEFEKT der Zeitnahme), "
                      << a.verworfen_ausserhalb << "x ausserhalb der Histogramm-Spanne\n";
        }
        if (!a.p50.in_toleranz || !a.p95.in_toleranz || !a.p99.in_toleranz) {
            std::cout << "      BEFUND: HDR weicht staerker vom Kanon ab als die Bucket-Breite erlaubt"
                      << " (p50=" << (a.p50.abweichung_rel * 100.0) << " % p95=" << (a.p95.abweichung_rel * 100.0)
                      << " % p99=" << (a.p99.abweichung_rel * 100.0) << " %)\n";
        }
        if (!a.histogramm_bereit) std::cout << "      BEFUND: hdr_init fehlgeschlagen -- ALLE Proben verloren\n";
    }

    // V41.P3-Bridge: 16-Spalten-Pipeline-Mess-CSV (eine Zeile je gemessener Organ-Composition).
    // total_cycles = gemessene mittlere Latenz in ns (Stufe-05-Konvention "cycles als ns interpretiert") = REAL.
    // Anti-Phantom (Ledger §0): PMU-/Energie-Spalten UND bytes_allocated/bytes_in_use_peak = honest-0 (P4-gated,
    // kein PMC/Allocator-Zaehler) -- KEINE ops*64-Byte-Schaetzung mehr. Nur die Latenz ist real gemessen.
    if (!pipeline_csv.empty()) {
        // B-3 (2026-08-09): Spaltenliste aus der EINEN Quelle (cache_engine/measurement/pipeline_csv_schema.hpp),
        // vorher hier als Literal abgeschrieben -- eine Aenderung dort brach diese Stelle nicht.
        std::string out = ::comdare::cache_engine::measurement::pipeline16_csv_header();
        for (std::size_t i = 0; i < results.size(); ++i) {
            double const  mean = stats::latency_mean_ns(std::span<const std::int64_t>{results[i].latency_samples_ns});
            std::uint64_t fp   = 14695981039346656037ULL; // FNV-1a über den Namen → stabiler Fingerprint
            for (char c : names[i]) {
                fp ^= static_cast<unsigned char>(c);
                fp *= 1099511628211ULL;
            }
            // Reihenfolge ab total_cycles: cache_l1,l2,l3,dtlb,coherence,energy,bytes_allocated,
            // bytes_in_use_peak,external_frag,internal_frag = 10x honest-0 (kein PMC/Allocator-Zaehler).
            out += names[i] + ',' + std::to_string(fp) + ",1," + workload_label + ',' + std::to_string(ops) + ',' +
                   std::to_string(static_cast<std::uint64_t>(mean)) + ",0,0,0,0,0,0,0,0,0,0\n";
        }
        if (write_text_file(pipeline_csv, out))
            std::cout << "  Pipeline-CSV (16-col; Latenz real, PMU/Bytes honest-0) -> " << pipeline_csv << "\n";
        else {
            std::cerr << "Pipeline-CSV-Schreiben fehlgeschlagen: " << pipeline_csv << "\n";
            return 4;
        }
    }

    if (!csv_path.empty()) {
        if (write_text_file(csv_path, stats::report_to_csv(rep)))
            std::cout << "  CSV  -> " << csv_path << "\n";
        else {
            std::cerr << "CSV-Schreiben fehlgeschlagen: " << csv_path << "\n";
            return 4;
        }
    }
    if (!json_path.empty()) {
        if (write_text_file(json_path, stats::report_to_json(rep)))
            std::cout << "  JSON -> " << json_path << "\n";
        else {
            std::cerr << "JSON-Schreiben fehlgeschlagen: " << json_path << "\n";
            return 4;
        }
    }
    // D4e: der Lauf ist FACHLICH durch (Ausgaben und Dateien sind geschrieben), aber er hat tote
    // Proben gesehen. Das ist ein BEFUND fuer den Auswerter, kein Schreibfehler -- deshalb ein
    // eigener Code am Ende und nicht ein frueher Abbruch. Code 6, weil 5 schon vergeben ist
    // (Lastprofil-Pfad oben: "keine einzige DLL gemessen").
    if (int const bilanz_code = cmd::lade_bilanz_exit_code(bilanz); bilanz_code != 0) {
        std::cerr << "BEFUND: " << bilanz.tote_proben << " von " << bilanz.geladen
                  << " geladenen DLLs lieferten tote Proben -- sie sind NICHT in die Auswertung eingegangen.\n";
        return bilanz_code;
    }
    return 0;
}
