// SPDX-License-Identifier: Apache-2.0
// apps/experiment_planner/main.cpp -- comdare-experiment-planner: die PLANER-ROLLE als EIGENE Binary (W1).
//
// OWNER-KERN (05.08.2026, F1-HART): "Der Planer ist ZWINGEND eine eigene Binary mit dem User-CLI-Interface
// auf der Shell." Begruendung des Owners: "Definitiv vor Abgabe, weil sonst die Binaries aller Stufen nicht
// korrekt gebaut werden."
//
// WAS DIESE BINARY IST (Stufen-Doktrin, Ledger-Nachtraege mittag-9/-10/-11): der TRAEGER der Mess-Achsen-
// STUFE-1-RT-FREIGABE. Sie liest die Experiment-XML, faehrt den deterministischen ExperimentPlanDirector-Walk
// und emittiert die STUFE-1-Sicht (Textplan / CEB-Child-Pipeline-YAML / experiment_plan.cmake). Die CEB
// (comdare-messung-driver) ist die Stufe-2-Instanz: sie emittiert die Tier-Jobs (tier ci|cmake) und vollzieht
// die Messung (run). Section 42: "Eigentlich erhaelt die Cache Engine die XML und uebernimmt die gesamte
// Arbeit" -- deshalb lebt diese App in der ce-Welt, nicht im super-Repo.
//
// ZWEI MODULE, KEINE VERERBUNG (PV-2 "Haupt/Unter-DELEGATION zweier Rollen"): Planer-Binary und CEB-Treiber
// teilen NUR die Fassaden-Bibliothek comdare::profile_run_facade -- keine gemeinsame Klassenhierarchie, keine
// Basisklasse, kein dupliziertes Substanz-Modul. Die gesamte Planer-SUBSTANZ (Director, Stufe-1-Builder,
// RegistryTrio-Resolve, validate-Gate, measurement_combos_of, planer_block-Lebenszyklus) liegt seit jeher in
// jener Fassaden-Bibliothek; W1 zieht NUR den HOST-Anteil (CLI-Dispatch, Profil-/Env-Aufloesung,
// planer_block-Kontext, Fassaden-Aufrufe) aus super Code/02_messung_driver/main.cpp hierher.
//
// CLI (clig.dev, uebernommen aus der V-6vi-Subcommand-Linie des Treibers):
//   validate [<profil>] | plan dump|ci|cmake [<profil>] | cache-key | fingerprint [<profil>] | version |
//   help [<subcommand>]
// Der Treiber behaelt tier ci|cmake (CEB-Rolle) und run (Mess-Vollzug). KEINE Alt-Flags in dieser NEUEN
// Oberflaeche (die deprecated Aliase des Treibers wandern bewusst NICHT mit -- Aufraeumpass-Doktrin).
//
// Ausgaben (clig.dev): Daten/Emissionen -> stdout; Diagnose/Fehler -> stderr.

#include <profile_facade/g1_binary_version_stamp.hpp> // version: g1_build_type_label() (EINE Build-Typ-Wahrheit)
#include <profile_facade/planner/planner_cli_env.hpp> // W1-Hoist: env_trimmed/GoldenRange/parse_size_env_strict
#include <profile_facade/planner/planner_version.hpp> // Section 43.b/64: planner_version_stamp()
#include <profile_facade/profile_run_facade.hpp>      // die EINE Schnitt-Naht (kuenftige #35-.so-Schnittstelle)

#include <builder/artifact_transport/artifact_cache.hpp> // planer_block-Kontext: ArtifactCache::from_env

#include "xml_config_parser/xml_reader.hpp" // Bruecke-I2: Root-Tag-Sniff des validate-Profils (common-DOM)

#include <cstddef>
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {

namespace pln = ::comdare::cache_engine::planner;
namespace pf  = ::comdare::cache_engine::builder::profile_facade;

// ---------------------------------------------------------------------------------------------------------------
// G4b-2 (d2) / #46b I1b: das GATE des planer_block fuer die beiden CEB-Compile-Strecken (plan ci / plan cmake).
//
// Der planer_block meldet dem Lager, dass DIESER Planer gleich eine CEB-Compile-Strecke anstoesst, damit ein
// zweiter Planer auf einer anderen Maschine dieselbe Strecke nicht doppelt reserviert. Ausgefuehrt wird der
// Lebenszyklus in der Fassaden-TU (profile_run_facade.cpp) -- hier entsteht nur der Kontext, denn der gelockte
// Schreibweg zieht ueber bestandslog_document.hpp den ce-XML-DOM.
//
// GATE-FORMEL (verbatim uebernommen): COMDARE_BESTANDSLOG=="true" UND minio_enabled() UND die drei
// Pflicht-Variablen. minio_enabled() und NICHT !inert(), weil die vier Objekt-Verben ausnahmslos auf Ebene B
// gaten -- eine Nur-measure-drop-Konfiguration wuerde sonst einen toten Transport binden. Gate an + Pflicht-Var
// leer => harter Abbruch (der Aufrufer liefert exit 6). Gate an + kein minio => EINE WARNUNG, kein Binden.
// Gate aus => vollstaendig stumm (Byte-Neutralitaet des Vor-Zustands).
//
// BUDGET (2.4-(5)): with_object_budget(1, 10) -- EIN Versuch mit 10 s Deckel statt der 12 Versuche des Defaults.
// Ein unerreichbarer Store darf eine CI-Emission nicht minutenlang aufhalten.
//
// id (E2): owner_uuid + "/planer" -- EINE Sperre je Lauf, nicht je Sequenz. Die Eindeutigkeit traegt der
// lauf-eindeutige owner, nicht ein Zaehler. W1-Wirkung auf den Merge-Schluessel: KEINE -- die id traegt den
// owner_uuid, nicht argv[0]; dass jetzt eine ANDERE Binary reserviert, aendert den Schluessel nicht.
struct PlanerBlockGate {
    pf::PlanerBlockContext ctx;             // leer = inert
    bool                   abbruch = false; // true => exit 6
};

[[nodiscard]] PlanerBlockGate make_planer_block_gate() {
    namespace atp = ::comdare::cache_engine::builder::artifact_transport;

    PlanerBlockGate g;
    if (pln::env_trimmed("COMDARE_BESTANDSLOG") != "true") return g; // stumm inert

    // Der Emissions-Cache ist eine EIGENE, benannte Instanz -- die Mess-Lauf-Instanz des Treibers entsteht in
    // einer anderen Binary und geht diesen Weg gar nicht mehr an.
    auto const emit_ac = std::make_shared<atp::ArtifactCache const>(atp::ArtifactCache::from_env().with_object_budget(
        /*tries=*/1, /*timeout_s=*/10));
    if (!emit_ac->minio_enabled()) {
        std::cerr << "[bestandslog] WARNUNG fehlerklasse=lager_ebene_fehlt: COMDARE_BESTANDSLOG=true, aber Ebene B "
                  << "(minio) ist nicht konfiguriert (measure-drop=" << (emit_ac->drop_enabled() ? "1" : "0")
                  << ") -- planer_block bleibt AUS, Emission unveraendert.\n";
        return g;
    }

    std::string const doc_key      = pln::env_trimmed("COMDARE_BESTANDSLOG_DOC_KEY");
    std::string const owner_uuid   = pln::env_trimmed("COMDARE_BESTANDSLOG_OWNER_UUID");
    std::string const maschine     = pln::env_trimmed("COMDARE_BESTANDSLOG_MASCHINE");
    char const*       fehlende_var = nullptr;
    if (doc_key.empty())
        fehlende_var = "COMDARE_BESTANDSLOG_DOC_KEY";
    else if (owner_uuid.empty())
        fehlende_var = "COMDARE_BESTANDSLOG_OWNER_UUID";
    else if (maschine.empty())
        fehlende_var = "COMDARE_BESTANDSLOG_MASCHINE";
    if (fehlende_var != nullptr) {
        std::cerr << "[bestandslog] FEHLER fehlerklasse=konfiguration_unvollstaendig: "
                  << "COMDARE_BESTANDSLOG=true, aber " << fehlende_var << " ist leer -- Abbruch.\n";
        g.abbruch = true;
        return g;
    }

    g.ctx.cache      = emit_ac;
    g.ctx.doc_key    = doc_key;
    g.ctx.id         = owner_uuid + "/planer"; // E2: EINE Sperre je Lauf
    g.ctx.owner_uuid = owner_uuid;
    g.ctx.maschine   = maschine;
    // Thread-Budget nur, wenn es ueberhaupt erklaert ist (0 = nicht gemeldet, keine erfundene Zahl).
    if (auto const bp = pln::parse_size_env_strict("COMDARE_BUILD_PARALLEL"))
        g.ctx.threads = static_cast<unsigned>(*bp);
    std::cerr << "[bestandslog] planer_block aktiv: doc_key=" << g.ctx.doc_key << " id=" << g.ctx.id
              << " maschine=" << g.ctx.maschine << "\n";
    return g;
}

// G4b-2/2.4-(3) + A-B1 (cppcheck throwInEntryPoint): die Emissionszweige leben als BENANNTE freie Handler
// AUSSERHALB von main, mit AUSGESCHRIEBENEM try/catch-Mantel je Handler. cppcheck wertet Lambda-Koerper
// LEXIKALISCH im Kontext der umgebenden Funktion und versteht ein try, das erst im Aufgerufenen liegt, NICHT.
// Ohne Mantel loeste eine Ausnahme std::terminate OHNE Unwinding aus -- der PromiseGuard des planer_block waere
// nie gefeuert und die Reservierung 30 Minuten haengen geblieben. Rueckgabe 1, NICHT 6: exit 6 bleibt exklusiv
// fuer fehlerklasse=konfiguration_unvollstaendig.
[[nodiscard]] int emission_abgebrochen(char const* was, char const* detail) noexcept {
    std::cerr << "[bestandslog] FEHLER fehlerklasse=emission_abgebrochen: " << was << " -- " << detail << "\n";
    return 1;
}

[[nodiscard]] int run_plan_ci_guarded(std::string const& prof) noexcept {
    // G4b-2/E1: eine der beiden CEB-Compile-Strecken -> planer_block haengt hier. Gate-Erzeugung IM Mantel:
    // make_planer_block_gate() macht Bestandslog-IO und kann werfen -- nur im Mantel ist das Unwinding
    // (PromiseGuard) garantiert; exit 6 bleibt dem Gate-Abbruch vorbehalten.
    try {
        auto const gate = make_planer_block_gate();
        if (gate.abbruch) return 6;
        return pf::dump_experiment_ci_facade(prof, std::cout, gate.ctx);
    } catch (std::exception const& e) { return emission_abgebrochen("plan ci", e.what()); } catch (...) {
        return emission_abgebrochen("plan ci", "unbekannte Ausnahme");
    }
}

[[nodiscard]] int run_plan_cmake_guarded(std::string const& prof) noexcept {
    // G4b-2/E1: die zweite CEB-Compile-Strecke -- derselbe planer_block wie bei plan ci.
    try {
        auto const gate = make_planer_block_gate();
        if (gate.abbruch) return 6;
        return pf::dump_experiment_cmake_facade(prof, std::cout, gate.ctx);
    } catch (std::exception const& e) { return emission_abgebrochen("plan cmake", e.what()); } catch (...) {
        return emission_abgebrochen("plan cmake", "unbekannte Ausnahme");
    }
}

[[nodiscard]] int run_plan_dump_guarded(std::string const& prof) noexcept {
    // KEIN planer_block (plan dump stoesst keine CEB-Compile-Strecke an), aber derselbe Ausnahme-Mantel:
    // der Fassaden-Walk liest XML + Katalog und darf main nicht ohne Unwinding verlassen.
    try {
        return pf::dump_experiment_plan_facade(prof, std::cout);
    } catch (std::exception const& e) { return emission_abgebrochen("plan dump", e.what()); } catch (...) {
        return emission_abgebrochen("plan dump", "unbekannte Ausnahme");
    }
}

// Bruecke-I2 (2026-07-16): validate deckt BEIDE offiziellen Profil-Wurzeln ab. Ein Root-Tag-Sniff (rein-lesend
// ueber den common-DOM) entscheidet: <comdare_thesis_profile> -> das Achsen-/Werte-Gate (validate_profile_facade);
// <comdare_experiment> -> das 3-Phasen-Gate (validate_experiment_profile_facade) mit den per CMake einkompilierten
// STATISCHEN ce+prt-Registry-Pfaden (2-Registry-Kanon; der Host reicht sie herein, die ce-Fassade haelt keinen
// prt-art-Pfad hart vor -- Baseline-Layering). Beide Parser liefern nullopt bei Fremd-Tag, daher ist der reine
// Root-Tag-Read gefahrlos; eine unbekannte/unlesbare Wurzel -> rc 5 (kein Bau).
[[nodiscard]] int run_validate_guarded(std::string const& prof) noexcept {
    try {
        std::string root_tag;
        if (std::ifstream in{prof, std::ios::binary}; in) {
            std::ostringstream ss;
            ss << in.rdbuf();
            if (auto const root = ::comdare::common::xml::parse_document(ss.str())) root_tag = root->tag;
        }
        if (root_tag == "comdare_thesis_profile") { return pf::validate_profile_facade(prof, std::cout); }
        if (root_tag == "comdare_experiment") {
#if defined(COMDARE_CE_AXIS_REGISTRY_PATH) && defined(COMDARE_PRT_AXIS_REGISTRY_PATH)
            return pf::validate_experiment_profile_facade(prof, COMDARE_CE_AXIS_REGISTRY_PATH,
                                                          COMDARE_PRT_AXIS_REGISTRY_PATH, std::cout);
#else
            // ce-STANDALONE ohne prt-art-Klon: derselbe #ifdef-Fall wie im Treiber vor W1 (deklarierte
            // Paritaet). In der super-Welt -- dem einzigen CI-Konsumenten von validate -- sind BEIDE Pfade
            // einkompiliert, dort ist der Zweig oben aktiv.
            std::cerr << "[validate] '" << prof
                      << "': comdare_experiment erkannt, aber die statischen Registry-Pfade wurden nicht "
                         "einkompiliert (COMDARE_CE/PRT_AXIS_REGISTRY_PATH) -- ce-Klon/CMake nicht synchron?\n";
            return 5;
#endif
        }
        std::cerr << "[validate] '" << prof << "': unbekannte/unlesbare Wurzel"
                  << (root_tag.empty() ? "" : " '" + root_tag + "'")
                  << " -- weder <comdare_thesis_profile> noch <comdare_experiment>. KEIN Bau ausgefuehrt.\n";
        return 5;
    } catch (std::exception const& e) {
        std::cerr << "[Konfig-Fehler: validate] " << e.what() << "\n";
        return 5;
    } catch (...) {
        std::cerr << "[Konfig-Fehler: validate] unbekannte Ausnahme\n";
        return 5;
    }
}

// Cache-Resthygiene-2 (2026-07-21): fingerprint druckt das Chunk-Organ-Fingerprint-PRE-IMAGE (perm.dll.algos-
// Inhalte der Range-Binaries, stem-sortiert konkateniert) nach stdout -- rein aus dem Katalog, KEIN DLL-Bau.
// Die CI pipet es durch `sha256sum` -> COMDARE_GN_ALGO_SIG (== S1-F1-Marker-algo_sig -> Marker-Wache scharf).
// Range aus COMDARE_GOLDEN_N_RANGE="start:count" (leer/ungesetzt => ganze View).
[[nodiscard]] int run_fingerprint_guarded(std::string const& prof) noexcept {
    try {
        std::size_t rstart = 0, rcount = 0;
        if (auto const r = pln::parse_golden_range_env()) {
            rstart = r->start;
            rcount = r->count;
        }
        return pf::chunk_organ_fingerprint_facade(prof, rstart, rcount, std::cout);
    } catch (std::exception const& e) {
        // Fehlerklassen-Doktrin: kaputtes COMDARE_GOLDEN_N_RANGE/Profil ist ein KONFIG-Fehler -- klar melden
        // statt unhandled throw; nie stillschweigend Voll-View.
        std::cerr << "[Konfig-Fehler: fingerprint] " << e.what() << "\n";
        return 2;
    } catch (...) {
        std::cerr << "[Konfig-Fehler: fingerprint] unbekannte Ausnahme\n";
        return 2;
    }
}

// R8 (Nacht-Audit 2026-07-22): cache-key druckt den VOLLEN ce-Objekt-Cache-Key-Praefix fuer die per Env gepinnte
// GN-Zelle nach stdout (EINE Zeile). Die CI (.golden_n_build) konsumiert ihn LITERAL als PULL-Quelle/MARK_PREFIX
// -> kein bash-Key-Drift (+bt/+ceb/+mtool/+mrg Single-Source aus dieser Binary). base = "m3v2" (dieselbe
// Mess-Lauf-build_version wie die run-Pfade des Treibers).
[[nodiscard]] int run_cache_key_guarded() noexcept {
    try {
        return pf::print_cache_key_facade("m3v2", std::cout);
    } catch (std::exception const& e) {
        std::cerr << "[Konfig-Fehler: cache-key] " << e.what() << "\n";
        return 2;
    } catch (...) {
        std::cerr << "[Konfig-Fehler: cache-key] unbekannte Ausnahme\n";
        return 2;
    }
}

// Section 43.b/64: der PLANER ist die Binary mit EINER statischen Selbst-Version X.Y.Z (er permutiert, er IST
// keine Permutation -> KEINE Achsen-Arrays). DEKLARIERTE ABWEICHUNG vom Treiber: der behaelt seinen G1-Block aus
// vier Zeilen (planner/ceb-contract/build-type/build-version), weil er die CEB-Rolle traegt; hier stehen genau
// die zwei Zeilen, die den PLANER beschreiben.
[[nodiscard]] int run_version_guarded() noexcept {
    try {
        std::cout << pln::planner_version_stamp() << "\n";
        std::cout << "build-type=" << pf::g1_build_type_label() << "\n";
        return 0;
    } catch (std::exception const& e) {
        std::cerr << "[Fehler: version] " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "[Fehler: version] unbekannte Ausnahme\n";
        return 1;
    }
}

// Hilfe nach stdout (clig.dev: Hilfe ist Daten), rc 0. topic leer = Uebersicht; sonst Detail je Subcommand.
void help_for(std::string const& topic) {
    if (topic == "validate") {
        std::cout << "comdare-experiment-planner validate [<profil>]\n"
                  << "  Rein-lesende Pre-Flight-Pruefung des Profils -- baut KEINE DLL, misst NICHT.\n"
                  << "  Deckt BEIDE offiziellen Wurzeln: <comdare_thesis_profile> (Achsen-/Werte-Gate) und\n"
                  << "  <comdare_experiment> (3-Phasen-Gate gegen die einkompilierten ce+prt-Registry-Pfade).\n"
                  << "  Profil-Aufloesung: Argument > COMDARE_THESIS_PROFILE > einkompiliertes Default-Profil.\n"
                  << "  Exit: 0 ok; 1 Verstoss; 5 unbekannte/unlesbare Profil-Wurzel.\n";
        return;
    }
    if (topic == "plan") {
        std::cout << "comdare-experiment-planner plan dump|ci|cmake [<profil>]\n"
                  << "  PLANER-Rolle (Stufe 1, 40.b): der deterministische ExperimentPlanDirector-Walk in drei\n"
                  << "  Emissions-Kanaelen -- zwei Laeufe sind byte-gleich.\n"
                  << "    plan dump   Textplan nach stdout\n"
                  << "    plan ci     GitLab-Child-Pipeline-YAML: CEB-Jobs je Mess-Kombination\n"
                  << "    plan cmake  experiment_plan.cmake fuer den Bare-Metal-Bau\n"
                  << "  plan ci/cmake tragen das Bestandslog-planer_block-Gate: COMDARE_BESTANDSLOG=true +\n"
                  << "  COMDARE_BESTANDSLOG_DOC_KEY/_OWNER_UUID/_MASCHINE (Exit 6 bei unvollstaendiger Konfig).\n"
                  << "  Profil-Aufloesung: Argument > COMDARE_THESIS_PROFILE > einkompiliertes Default-Profil.\n";
        return;
    }
    if (topic == "cache-key") {
        std::cout << "comdare-experiment-planner cache-key\n"
                  << "  Druckt den vollen ce-Objekt-Cache-Key-Praefix der env-gepinnten GN-Zelle (EINE Zeile).\n"
                  << "  Env-Pins: COMDARE_GN_OPT, COMDARE_GN_SIMD, COMDARE_CXX, COMDARE_BUILD_TYPE,\n"
                  << "  COMDARE_MEASUREMENT_COMBO.\n";
        return;
    }
    if (topic == "fingerprint") {
        std::cout << "comdare-experiment-planner fingerprint [<profil>]\n"
                  << "  Druckt das Chunk-Organ-Fingerprint-PRE-IMAGE des Range-Fensters nach stdout (die CI\n"
                  << "  pipet es durch sha256sum -> COMDARE_GN_ALGO_SIG). Fenster: COMDARE_GOLDEN_N_RANGE\n"
                  << "  \"start:count\" (leer = ganze View). Exit 2 bei kaputter Range/Profil.\n";
        return;
    }
    if (topic == "version") {
        std::cout << "comdare-experiment-planner version   (kanonisches Flag: --version)\n"
                  << "  Druckt den Planer-Selbst-Stempel (Section 43.b: statische X.Y.Z + ISA/OS-Deklaration)\n"
                  << "  und die Compile-Einstellung -- zwei gelabelte non-empty Zeilen.\n";
        return;
    }
    std::cout << "comdare-experiment-planner -- die PLANER-Rolle der Mess-Kette (Stufe 1, Section 40.b/42)\n\n"
              << "Usage:\n"
              << "  comdare-experiment-planner <subcommand> [argumente]\n\n"
              << "Subcommands:\n"
              << "  validate [<profil>]     Profil rein-lesend pruefen (beide offiziellen Wurzeln)\n"
              << "  plan dump [<profil>]    deterministischer Experiment-Plan als Text\n"
              << "  plan ci [<profil>]      GitLab-Child-Pipeline-YAML der CEB-Jobs\n"
              << "  plan cmake [<profil>]   experiment_plan.cmake fuer den Bare-Metal-Bau\n"
              << "  cache-key               ce-Objekt-Cache-Key-Praefix der env-gepinnten GN-Zelle\n"
              << "  fingerprint [<profil>]  Chunk-Organ-Fingerprint-Pre-Image (COMDARE_GOLDEN_N_RANGE-Fenster)\n"
              << "  version                 Planer-Selbst-Stempel + Compile-Einstellung\n"
              << "  help [<subcommand>]     diese Uebersicht bzw. Detail-Hilfe (auch: <subcommand> --help)\n\n"
              << "Rollen-Trennung (Section 40.b/42, ZWEI MODULE): die STUFE-2-Sicht (tier ci|cmake) und der\n"
              << "Mess-Vollzug (run) gehoeren der CEB -- comdare-messung-driver. Exit 7 (Lane-Fehlrouting) gibt es\n"
              << "nur dort; diese Binary misst nicht.\n\n"
              << "Profil-Aufloesung: explizites Argument > COMDARE_THESIS_PROFILE > einkompiliertes Default-Profil.\n"
              << "Ausgaben: Daten/Emissionen -> stdout; Diagnose/Fehler -> stderr (clig.dev).\n\n"
              << "Exit-Codes:\n"
              << "  0 Erfolg | 1 Usage/Verstoss/Emission abgebrochen | 2 Konfig-Fehler (kaputte Range)\n"
              << "  5 unbekannte Profil-Wurzel | 6 Bestandslog-Gate-Abbruch\n";
}

// Profil-Aufloesung (identisch in allen Zweigen): Argument > COMDARE_THESIS_PROFILE > gebackenes Default-Profil.
// Ein mit '-' beginnendes Argument ist NIE ein Profil-Positional (Flag-Reserve, Muster des Treibers).
[[nodiscard]] std::string resolve_profile(std::string const& arg) {
    std::string prof = (!arg.empty() && arg.front() != '-') ? arg : pln::env_trimmed("COMDARE_THESIS_PROFILE");
    if (prof.empty()) prof = COMDARE_MESSUNG_DEFAULT_THESIS_PROFILE;
    return prof;
}

[[nodiscard]] int unbekanntes_subkommando(std::string const& wort) noexcept {
    std::cerr << "comdare-experiment-planner: unbekanntes Subkommando '" << wort
              << "' -- erwartet: validate | plan dump|ci|cmake | cache-key | fingerprint | version | help.\n"
              << "  (Die CEB-Rolle 'tier ci|cmake' und der Mess-Lauf 'run' leben im comdare-messung-driver.)\n";
    return 1;
}

} // namespace

int main(int argc, char* argv[]) {
    // Flacher Subkommando-Dispatcher (KEINE Alt-Flag-Kanonisierung: diese Binary ist neu und erbt keine
    // deprecated Aliase). W5-ANDOCKPUNKT: das kuenftige `status`-Subkommando haengt sich hier additiv als
    // weiterer if-Zweig ein -- die flache Kette ist genau dafuer gewaehlt.
    std::vector<std::string> args;
    args.reserve(static_cast<std::size_t>(argc));
    for (int i = 0; i < argc; ++i) args.emplace_back(argv[i]);
    std::string const a1 = args.size() > 1 ? args[1] : std::string{};
    std::string const a2 = args.size() > 2 ? args[2] : std::string{};
    std::string const a3 = args.size() > 3 ? args[3] : std::string{};

    if (a1.empty()) {
        std::cerr << "comdare-experiment-planner: kein Subkommando angegeben -- "
                  << "'comdare-experiment-planner help' zeigt die Uebersicht.\n";
        return 1;
    }
    if (a1 == "--help" || a1 == "-h") {
        help_for({});
        return 0;
    }
    if (a1 == "help") {
        help_for(a2);
        return 0;
    }
    if (a1 == "version" || a1 == "--version") {
        if (a2 == "--help" || a2 == "-h") {
            help_for("version");
            return 0;
        }
        return run_version_guarded();
    }
    if (a1 == "validate") {
        if (a2 == "--help" || a2 == "-h") {
            help_for("validate");
            return 0;
        }
        return run_validate_guarded(resolve_profile(a2));
    }
    if (a1 == "cache-key") {
        if (a2 == "--help" || a2 == "-h") {
            help_for("cache-key");
            return 0;
        }
        return run_cache_key_guarded();
    }
    if (a1 == "fingerprint") {
        if (a2 == "--help" || a2 == "-h") {
            help_for("fingerprint");
            return 0;
        }
        return run_fingerprint_guarded(resolve_profile(a2));
    }
    if (a1 == "plan") {
        if (a2 == "--help" || a2 == "-h" || a3 == "--help" || a3 == "-h") {
            help_for("plan");
            return 0;
        }
        std::string const prof = resolve_profile(a3);
        if (a2 == "dump") return run_plan_dump_guarded(prof);
        if (a2 == "ci") return run_plan_ci_guarded(prof);
        if (a2 == "cmake") return run_plan_cmake_guarded(prof);
        std::cerr << "comdare-experiment-planner: unbekanntes Unterkommando 'plan" << (a2.empty() ? "" : " ") << a2
                  << "' -- erwartet: plan dump|ci|cmake (Detail: 'comdare-experiment-planner help plan').\n";
        return 1;
    }
    return unbekanntes_subkommando(a1);
}
