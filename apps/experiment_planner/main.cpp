// SPDX-License-Identifier: LicenseRef-Comdare-Research-1.0
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
//   validate [<profil>] | plan dump|ci|cmake [<profil>] | cache-key | fingerprint [<profil>] |
//   status [<profil>] [--root=<dir>] | check-size [<profil>] [--max-bytes=N] [--max-tage=F]
//   [--sekunden-je-op=F] | version | help [<subcommand>]
// W5 (Owner-R5, 05.08.2026): `status` ist der ON-DEMAND-RUECK-LESER -- die einzige Rolle dieser Binary, die
// nach dem Lauf FRAGT statt ihn zu planen. Er steuert nichts (kein watch, kein Block, keine Reservierung).
// Der Treiber behaelt tier ci|cmake (CEB-Rolle) und run (Mess-Vollzug). KEINE Alt-Flags in dieser NEUEN
// Oberflaeche (die deprecated Aliase des Treibers wandern bewusst NICHT mit -- Aufraeumpass-Doktrin).
//
// Ausgaben (clig.dev): Daten/Emissionen -> stdout; Diagnose/Fehler -> stderr.

#include <profile_facade/g1_binary_version_stamp.hpp>       // version: g1_build_type_label() (EINE Build-Typ-Wahrheit)
#include <profile_facade/planner/planner_cli_env.hpp>       // W1-Hoist: env_trimmed/GoldenRange/parse_size_env_strict
#include <profile_facade/planner/planner_status_reader.hpp> // W5: der ON-DEMAND-Rueck-Leser (status)
#include <profile_facade/planner/planner_version.hpp>       // Section 43.b/64: planner_version_stamp()
#include <profile_facade/profile_run_facade.hpp>            // die EINE Schnitt-Naht (kuenftige #35-.so-Schnittstelle)

#include <builder/artifact_transport/artifact_cache.hpp>    // planer_block-Kontext: ArtifactCache::from_env
#include <builder/bestandslog/artifact_cache_transport.hpp> // W5: make_bestand_transport (NUR LESEND, kein Binden)

#include <cache_engine/measurement/axis_error.hpp> // VL-3: AdmissionStatus + debug_flag_admission (Sperre)

#include "xml_config_parser/xml_reader.hpp" // Bruecke-I2: Root-Tag-Sniff des validate-Profils (common-DOM)

#include <cerrno> // check-size: ERANGE der strtoull-Bereichspruefung des Byte-Deckels
#include <cstddef>
#include <cstdint> // check-size: uint64_t (der Byte-Deckel)
#include <cstdlib> // check-size: strtoull -- der Byte-Deckel ist eine GANZE Zahl, kein double
#include <exception>
#include <filesystem> // W5: der aufgeloeste Mess-Ausgabe-Root (status --root)
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view> // W5: --root=-Flag-Praefix ohne Laengen-Literal
#include <vector>

namespace {

namespace pln = ::comdare::cache_engine::planner;
namespace pf  = ::comdare::cache_engine::builder::profile_facade;
namespace cem = ::comdare::cache_engine::measurement; // VL-3: die Zulassungs-Taxonomie der --debug-Sperre

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

// ---------------------------------------------------------------------------------------------------------------
// check-size (2026-08-09): die MENGEN-VORSCHAU. Der Planer rechnet die Mess-Menge, BEVOR gemessen wird.
//
// WARUM ES AN DIESER BINARY HAENGT: der Planer ist die Stelle, die FRAGT (Laufzeit); die CEB TRAEGT die Zahl
// (Uebersetzungszeit). Die Menge entscheidet vor dem Lauf ueber Zeitfenster, Arena-Groesse und Rueckschrieb --
// alle drei sind nach dem Start nicht mehr billig zu korrigieren.
//
// DIE AUSGABE IST DAS PRODUKT, nicht die interne Rechnung: gedruckt werden die Mengen JE FAKTOR mit ihrem
// NENNER. Wer nur eine Endzahl sieht, kann einen fehlenden Faktor (z.B. die 18-fache Wiederholung durch das
// Drift-Gate) nicht bemerken -- ein zu kleines Produkt sieht aus wie ein richtiges.
//
// STEUERT NICHTS, wie `status`: kein Bau, keine Messung, keine Reservierung, kein planer_block (also nie 6).
// Die Aufrufer-Zutaten-Parser -- DATEI-WEIT, weil check-size UND simulate (S-19) dieselben Regeln
// brauchen (ein Ort, eine Wahrheit). KONFIG-FEHLER SIND HART (rc 2 beim Aufrufer), nicht
// stillschweigend 0: ein vertippter Deckel, der als "kein Deckel" durchrutscht, waere genau der
// Gruen-ohne-Pruefung-Fall.
[[nodiscard]] double zahl_oder_fehler(std::string const& s, char const* flag, bool& kaputt) {
    if (s.empty()) return 0.0;
    try {
        std::size_t  gelesen = 0;
        double const w       = std::stod(s, &gelesen);
        if (gelesen != s.size() || !(w > 0.0)) {
            std::cerr << "comdare-experiment-planner: " << flag << " erwartet eine positive Zahl, bekam '" << s
                      << "'.\n";
            kaputt = true;
            return 0.0;
        }
        return w;
    } catch (...) {
        std::cerr << "comdare-experiment-planner: " << flag << " erwartet eine positive Zahl, bekam '" << s << "'.\n";
        kaputt = true;
        return 0.0;
    }
}
// Der Byte-Deckel ist eine GANZE Zahl und wird als solche geparst (strtoull, Ganz-String + errno).
// NICHT als double: static_cast<uint64_t>(0.5) waere 0 == "kein Deckel" (stiller Deckel-Verlust),
// und der Cast von inf/1e30/2^64 nach uint64 ist UB. Nur [0-9]+ ist zugelassen -- strtoull selbst
// wuerde "-1" kommentarlos nach 2^64-1 wickeln und Hex/Leerraum schlucken. 0 ist ABGELEHNT: ein
// Deckel von null Bytes ist ein Tippfehler, kein Auftrag "nicht pruefen" (Flag weglassen heisst das).
[[nodiscard]] std::uint64_t ganzzahl_oder_fehler(std::string const& s, char const* flag, bool& kaputt) {
    if (s.empty()) return 0u; // Flag nicht gesetzt == kein Deckel
    bool nur_ziffern = true;
    for (char const c : s) {
        if (c < '0' || c > '9') {
            nur_ziffern = false;
            break;
        }
    }
    errno                         = 0;
    char*                    ende = nullptr;
    unsigned long long const w    = std::strtoull(s.c_str(), &ende, 10);
    if (!nur_ziffern || ende != s.c_str() + s.size() || errno == ERANGE || w == 0ull) {
        std::cerr << "comdare-experiment-planner: " << flag
                  << " erwartet eine positive GANZE Zahl (dezimal, ohne Vorzeichen), bekam '" << s << "'.\n";
        kaputt = true;
        return 0u;
    }
    return static_cast<std::uint64_t>(w);
}

[[nodiscard]] int run_check_size_guarded(std::string const& profil, std::string const& max_bytes_arg,
                                         std::string const& max_tage_arg, std::string const& sek_je_op_arg) noexcept {
    try {
        // Die drei Aufrufer-Zutaten -- geparst VOR dem Profil-Walk (Parser datei-weit, s. oben). Die
        // Reihenfolge (erst Flags, dann Profil) ist Absicht: ein kaputtes Flag ist ohne Profil pruefbar
        // und faellt auch dann als 2, wenn zusaetzlich das Profil unlesbar ist.
        bool                kaputt       = false;
        std::uint64_t const deckel_bytes = ganzzahl_oder_fehler(max_bytes_arg, "--max-bytes", kaputt);
        double const        deckel_tage  = zahl_oder_fehler(max_tage_arg, "--max-tage", kaputt);
        double const        sek_je_op    = zahl_oder_fehler(sek_je_op_arg, "--sekunden-je-op", kaputt);
        if (kaputt) return 2;

        pln::MengenEingang eingang{};
        if (int const rc = pf::collect_mess_menge_facade(std::filesystem::path{profil}, eingang, std::cerr); rc != 0) {
            return rc; // 5 = unbekannte/unlesbare Profil-Wurzel (Diagnose steht schon auf stderr)
        }
        eingang.deckel_bytes   = deckel_bytes;
        eingang.deckel_tage    = deckel_tage;
        eingang.sekunden_je_op = sek_je_op;

        auto const sicht = pln::mengen_rechnen(eingang);
        std::cout << pln::mengen_bericht(sicht, eingang);

        // Nicht erhoben ist KEIN Erfolg: ohne Zahlen gibt es keine Freigabe.
        if (!sicht.erhoben) return 1;
        // Ein gerissener oder unbestimmbarer Deckel lehnt ab -- fail-closed, weil ein Deckel, der mangels Zahl
        // gruen meldet, keine Pruefung ist.
        return sicht.abgelehnt() ? 1 : 0;
    } catch (std::exception const& e) {
        std::cerr << "[Fehler: check-size] " << e.what() << "\n";
        return 2;
    } catch (...) {
        std::cerr << "[Fehler: check-size] unbekannte Ausnahme\n";
        return 2;
    }
}

// ---------------------------------------------------------------------------------------------------------------
// simulate -- S-19 PLANUNGS-SIMULATION (#7, 2026-08-20): die B-4-/Mess-Mengen-Rechnung VOR dem Lauf.
//
// PLANER-ETAPPE (D.1/O1 ausdruecklich): der Owner-Auftrag verlangt die Berechnung ZUSAETZLICH auf der
// CEB; deren Rechen-Vertrag (S-10/S-22) ist nicht gebaut -- diese Etappe rechnet im Planer und SAGT es.
// Die geparsten Aufrufer-Zutaten sind exakt die check-size-Regeln (Parser datei-weit, eine Wahrheit).
//
// MEHRERE Profile == EINE Kampagne (Owner: "Paper = genau EIN Experiment-XML"; FULL JOIN je Achse
// ueber alle Paper). Je Profil ein eigener Bericht, danach die Aggregation mit der B-4-Zahl.
//
// STEUERT NICHTS, wie check-size: kein Bau, keine Messung, keine Reservierung, kein planer_block.
struct SimulateArgs {
    std::vector<std::string> profile;
    std::string              sek_je_op, bau_sek_je_dll, bytes_je_dll, lager_budget, t3_fenster;
    std::string              mess_teilmenge, kandidaten, fremde_lane;
};

[[nodiscard]] int run_simulate_guarded(SimulateArgs const& args) noexcept {
    try {
        bool                kaputt       = false;
        double const        sek_je_op    = zahl_oder_fehler(args.sek_je_op, "--sekunden-je-op", kaputt);
        double const        bau_sek      = zahl_oder_fehler(args.bau_sek_je_dll, "--bau-sekunden-je-dll", kaputt);
        double const        t3_fenster   = zahl_oder_fehler(args.t3_fenster, "--t3-fenster-tage", kaputt);
        std::uint64_t const bytes_je_dll = ganzzahl_oder_fehler(args.bytes_je_dll, "--bytes-je-dll", kaputt);
        std::uint64_t const lager_budget = ganzzahl_oder_fehler(args.lager_budget, "--lager-budget-bytes", kaputt);
        std::uint64_t const teilmenge    = ganzzahl_oder_fehler(args.mess_teilmenge, "--mess-teilmenge", kaputt);
        std::vector<std::uint64_t> kandidaten;
        if (!args.kandidaten.empty()) {
            std::string rest = args.kandidaten;
            while (!rest.empty()) {
                auto const        komma = rest.find(',');
                std::string const token = rest.substr(0, komma);
                rest                    = (komma == std::string::npos) ? std::string{} : rest.substr(komma + 1);
                kandidaten.push_back(ganzzahl_oder_fehler(token, "--kandidaten", kaputt));
            }
        }
        // Die fremde Lane ist eine DEKLARATION (nicht gemessen) -- nur die drei Lage-Woerter sind zulaessig,
        // damit ein Tippfehler nicht stumm als "unbrauchbar" durchgeht (fail-closed heisst LAUT, nicht still).
        if (!args.fremde_lane.empty() && args.fremde_lane != "amd" && args.fremde_lane != "intel" &&
            args.fremde_lane != "unbrauchbar") {
            std::cerr << "comdare-experiment-planner: --fremde-lane erwartet amd|intel|unbrauchbar, bekam '"
                      << args.fremde_lane << "'.\n";
            kaputt = true;
        }
        if (kaputt) return 2;

        std::vector<pln::KampagnenPosten> posten;
        int                               schlechtester = 0;
        for (auto const& prof : args.profile) {
            pln::SimulationsEingang eingang{};
            if (int const rc = pf::collect_simulation_eingang_facade(std::filesystem::path{prof}, eingang, std::cerr);
                rc != 0) {
                return rc; // 5 = unbekannte/unlesbare Profil-Wurzel (Diagnose steht schon auf stderr)
            }
            eingang.mengen.sekunden_je_op = sek_je_op;
            eingang.mess_teilmenge        = teilmenge;
            eingang.bau_sekunden_je_dll   = bau_sek;
            eingang.bau_sekunden_art      = pln::MengenArt::Geschaetzt;
            eingang.bau_sekunden_nenner   = "--bau-sekunden-je-dll vom Aufrufer -- Trace-Anker, NICHT gemessen@16W";
            eingang.bytes_je_dll          = bytes_je_dll;
            eingang.bytes_je_dll_art      = pln::MengenArt::Geschaetzt;
            eingang.bytes_je_dll_nenner   = "--bytes-je-dll vom Aufrufer (GN-9-Kalibrierwert)";
            eingang.lager_budget_bytes    = lager_budget;
            eingang.t3_fenster_tage       = t3_fenster;
            eingang.fremde_lane_label     = args.fremde_lane;
            eingang.fremde_lane_pmc       = (args.fremde_lane == "amd" || args.fremde_lane == "intel");

            auto sicht = pln::simulation_rechnen(eingang);
            std::cout << pln::simulations_bericht(sicht, eingang);
            // Nicht erhoben ist KEIN Erfolg -- und eine Kampagne mit einem Loch ist keine Kampagne.
            if (!sicht.erhoben) return 1;
            if (sicht.abgelehnt()) schlechtester = 1;
            posten.push_back(pln::KampagnenPosten{std::filesystem::path{prof}.filename().string(), std::move(eingang),
                                                  std::move(sicht)});
        }
        auto const kampagne = pln::kampagnen_aggregation(posten);
        std::cout << pln::kampagnen_bericht(kampagne, kandidaten);
        if (!kampagne.erhoben) return 1;
        return schlechtester;
    } catch (std::exception const& e) {
        std::cerr << "[Fehler: simulate] " << e.what() << "\n";
        return 2;
    } catch (...) {
        std::cerr << "[Fehler: simulate] unbekannte Ausnahme\n";
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

// ---------------------------------------------------------------------------------------------------------------
// W5 (Owner-R5): `status` -- der ON-DEMAND-RUECK-LESER. Er berichtet den Stand von CEB-Bauten, Tier-Binaries und
// Messwerten und STEUERT NICHTS: kein Bau, keine Messung, keine Reservierung, kein planer_block (deshalb gibt es
// hier auch kein exit 6). Kein watch/follow -- jeder Aufruf ist ein Schnappschuss.
//
// Die SUBSTANZ liegt in profile_facade/planner/planner_status_reader.hpp (Leser-je-Quelle -> Aggregator ->
// Renderer); dieser Host tut nur, was ein Host tut: Argumente aufloesen, die vier Quellen anstoepseln, rendern.
//
// FEHLENDE QUELLEN SIND BERICHTS-INHALT, KEIN FEHLER: ein nicht existierender Mess-Baum, eine fehlende
// progress.cursor, ein abgeschaltetes Bestandslog -> je EINE ehrliche "keine Daten"-Zeile und rc 0. Nur ein
// kaputtes COMDARE_GOLDEN_N_RANGE (Konfig-Fehler) liefert rc 2 -- dieselbe Klasse wie bei `fingerprint`.

/// Der Mess-Ausgabe-Wurzelpfad. Default = der EIGENE Emissions-Kanon des Planers
/// ($CI_PROJECT_DIR/Code/measure_out/<slug>/perm<idx>, experiment_plan_director), mit der Bare-Metal-Probe
/// `measure_out` als Zweit-Kandidat. Die Binary kannte bis W5 KEINEN measure_out-Pfad -- `--root=` ist die eine
/// neue, deklarierte Zutat (ein Flag, KEINE neue Pflicht-Env). Der aufgeloeste Wert steht IMMER in der Kopfzeile.
[[nodiscard]] std::filesystem::path resolve_status_root(std::string const& root_arg) {
    if (!root_arg.empty()) return std::filesystem::path{root_arg};
    std::error_code             ec;
    std::filesystem::path const kanon{"Code/measure_out"};
    if (std::filesystem::exists(kanon, ec) && !ec) return kanon;
    std::filesystem::path const baremetal{"measure_out"};
    if (std::filesystem::exists(baremetal, ec) && !ec) return baremetal;
    return kanon; // nichts gefunden -> den Kanon NENNEN (der Bericht sagt root_vorhanden=nein)
}

/// Die Bestandslog-Sicht -- GENAU dasselbe Gate wie make_planer_block_gate, aber NUR LESEND: kein Lock, keine
/// Reservierung, kein PromiseGuard. Dasselbe Objekt-Budget (1 Versuch, 10 s Deckel): ein unerreichbarer Store
/// darf ein on-demand-Kommando nicht minutenlang aufhalten. Jeder Ausfall ist eine benannte "keine Daten"-Zeile.
[[nodiscard]] pln::BestandSicht lies_bestand_sicht() {
    namespace atp = ::comdare::cache_engine::builder::artifact_transport;
    namespace bl  = ::comdare::cache_engine::builder::bestandslog;

    pln::BestandSicht s{};
    if (pln::env_trimmed("COMDARE_BESTANDSLOG") != "true") return s; // aktiv=false -> "nicht aktiv"-Zeile
    s.aktiv = true;

    // Der Transport ist die EINZIGE Quelle dieses Kommandos, die ueber ein Netz geht -- und die einzige, die
    // WIRFT statt leer zu liefern (Env-Fehlform, Endpunkt weg, TLS/CA, Timeout). Faenge man sie nicht HIER,
    // liefe sie in den run_status_guarded-Rahmen und machte aus dem Bericht rc 2 -- also genau die
    // Konfig-Fehler-Klasse, die dem Anwender sagt "dein Aufruf war falsch", obwohl nur eine von vier Quellen
    // stumm blieb. Die Zusage des Kommandos lautet: fehlende Quellen sind BERICHTS-Inhalt, rc 0.
    try {
        auto const cache = atp::ArtifactCache::from_env().with_object_budget(/*tries=*/1, /*timeout_s=*/10);
        if (!cache.minio_enabled()) {
            s.grund = "Ebene B (minio) ist nicht konfiguriert";
            return s;
        }
        std::string const doc_key = pln::env_trimmed("COMDARE_BESTANDSLOG_DOC_KEY");
        if (doc_key.empty()) {
            s.grund = "COMDARE_BESTANDSLOG_DOC_KEY ist leer";
            return s;
        }
        auto const xml = bl::make_bestand_transport(cache).fetch(doc_key);
        if (!xml) {
            s.grund = "Dokument '" + doc_key + "' im Objekt-Store nicht gefunden/nicht lesbar";
            return s;
        }
        return pln::bestand_sicht_aus_xml(*xml);
    } catch (std::exception const& e) {
        s.fehler = true;
        s.grund  = e.what(); // der WORTLAUT des Transports, nicht eine nachgebaute Vermutung
    } catch (...) {
        s.fehler = true;
        s.grund  = "unbekannte Ausnahme im Bestandslog-Transport";
    }
    return s;
}

[[nodiscard]] int run_status_guarded(std::string const& prof, std::string const& root_arg) noexcept {
    try {
        pln::StatusBericht bericht{};
        bericht.planer_stempel = pln::planner_version_stamp();
        bericht.profil         = prof;
        bericht.root           = resolve_status_root(root_arg);

        // Das Fenster kennt nur die Env (COMDARE_GOLDEN_N_RANGE) -- der Plan traegt es nicht (die Emission
        // reicht es als Shell-Variable durch). Fehlform => Konfig-Fehler, NIE stille Voll-View.
        if (auto const r = pln::parse_golden_range_env()) {
            bericht.fenster         = std::to_string(r->start) + ":" + std::to_string(r->count);
            bericht.fenster_bekannt = r->count > 0; // count==0 deaktiviert das Fenster (syntaktisch gueltig)
            bericht.fenster_start   = r->start;     // FILTERT die Erhebung (H1), nicht nur den Anzeigestring
            bericht.fenster_count   = r->count;
        }

        // SOLL: derselbe deterministische Director-Walk wie plan dump/ci/cmake. rc != 0 ist KEIN Abbruch --
        // der Bericht sagt dann "plan=nicht_erhoben" MIT Grund und berichtet die uebrigen Quellen weiter.
        (void)pf::collect_plan_soll_facade(prof, bericht.soll, std::cerr);

        pln::erhebe_zellen(bericht, pf::mess_format_fakten_facade());
        bericht.bestand = lies_bestand_sicht();
        pln::render_status(bericht, std::cout);
        return 0;
    } catch (std::exception const& e) {
        std::cerr << "[Konfig-Fehler: status] " << e.what() << "\n";
        return 2;
    } catch (...) {
        std::cerr << "[Konfig-Fehler: status] unbekannte Ausnahme\n";
        return 2;
    }
}

// Hilfe nach stdout (clig.dev: Hilfe ist Daten), rc 0. topic leer = Uebersicht; sonst Detail je Subcommand.
void help_for(std::string const& topic) {
    if (topic == "status") {
        std::cout << "comdare-experiment-planner status [<profil>] [--root=<dir>]\n"
                  << "  ON-DEMAND-Rueck-Leser: berichtet den Stand von CEB-Bauten, Tier-Binaries und Messwerten.\n"
                  << "  Steuert NICHTS (kein Bau, keine Messung, keine Reservierung, kein planer_block).\n"
                  << "  Quellen: progress.cursor (Fenster-Cursor) | result.csv/.stamp/.stale (Mess-Resume) |\n"
                  << "           .fingerprint-Sidecars (gebaute Binaries) | Bestandslog-XML (Aggregat).\n"
                  << "  --root=<dir>  Wurzel des Mess-Ausgabe-Baums. Default: Code/measure_out, sonst measure_out;\n"
                  << "                der aufgeloeste Wert steht IMMER in der Kopfzeile (kein stilles Raten).\n"
                  << "  Fenster: COMDARE_GOLDEN_N_RANGE \"start:count\" -- start FILTERT: nur Perms in\n"
                  << "           [start,start+count) zaehlen in die Bilanz; alles andere gehoert einem anderen\n"
                  << "           Fenster und steht als eigene [status-fremdfenster]-Zeile. offen= ist die SUMME\n"
                  << "           der Zell-Offenstaende. Ohne Fenster gibt es kein Binary-SOLL --\n"
                  << "           offen= traegt dann den Sentinel 'unbelegt' statt einer erfundenen Zahl;\n"
                  << "           ebenso, wenn es nichts zu summieren gibt (kein Plan / keine Zelle im Fenster).\n"
                  << "           Sprengt die Summe den Wertebereich: 'uebergelaufen', nie eine kleine Zahl.\n"
                  << "           Auch die [status-cursor]-Zeile traegt im_fenster=; done= gilt NUR fuer das\n"
                  << "           eigene Fenster, ein fremdes Fertig-Signal steht als done_fremd= daneben.\n"
                  << "  Bestandslog: COMDARE_BESTANDSLOG=true + _DOC_KEY + minio; sonst EINE 'keine Daten'-Zeile.\n"
                  << "  Fehlende Quellen sind BERICHTS-Inhalt, kein Fehler. Exit: 0 Bericht; 2 Konfig-Fehler\n"
                  << "  (kaputte COMDARE_GOLDEN_N_RANGE). KEIN watch/follow -- jeder Aufruf ist ein Schnappschuss.\n";
        return;
    }
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
                  << "  Emissions-Kanaelen -- zwei Laeufe sind byte-gleich.\n" // (GEMESSEN 19.08.2026: cmp identisch)
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
    if (topic == "check-size") {
        std::cout << "comdare-experiment-planner check-size [<profil>] [--max-bytes=N] [--max-tage=F]\n"
                  << "                                      [--sekunden-je-op=F]   (kanonisches Flag: --check-size)\n"
                  << "  MENGEN-VORSCHAU vor dem Lauf: rechnet die Mess-Menge aus dem deterministischen Plan-Walk\n"
                  << "  und druckt sie AUFGESCHLUESSELT -- jede Menge mit ihrem Nenner, jede geschaetzte Groesse\n"
                  << "  als solche markiert. Baut KEINE DLL, misst NICHT, schreibt nichts.\n"
                  << "    --max-bytes=N        Deckel auf die Arena EINES Mess-Prozesses; N = positive GANZE Zahl\n"
                  << "                         in Bytes (dezimal). Flag weglassen = nur berichten; 0 ist rc 2.\n"
                  << "    --max-tage=F         Deckel auf die geschaetzte Dauer in Maschinentagen\n"
                  << "    --sekunden-je-op=F   Kalibrierung des Aufrufers. OHNE sie bleibt die Dauer n/a --\n"
                  << "                         der Bestand liefert VOR dem Lauf keine gemessene Zeit.\n"
                  << "  FAIL-CLOSED: ein verlangter Deckel, dessen Menge nicht berechenbar ist, gilt als NICHT\n"
                  << "  bestanden (Exit 1). Ein Deckel, der mangels Zahl gruen meldet, ist keine Pruefung.\n"
                  << "  Exit: 0 haelt; 1 Deckel gerissen/unbestimmbar oder nicht erhoben; 2 kaputtes Flag oder\n"
                  << "        ueberzaehliges Argument; 5 unbekannte Profil-Wurzel.\n";
        return;
    }
    if (topic == "simulate") {
        std::cout << "comdare-experiment-planner simulate [<profil>...] [--sekunden-je-op=F]\n"
                  << "    [--bau-sekunden-je-dll=F] [--bytes-je-dll=N] [--lager-budget-bytes=N]\n"
                  << "    [--t3-fenster-tage=F] [--mess-teilmenge=N] [--kandidaten=N,N,...]\n"
                  << "    [--fremde-lane=amd|intel|unbrauchbar]\n"
                  << "  S-19 PLANUNGS-SIMULATION (#7): rechnet aus den XML-FREIGABEN die Bau-Menge (B-4-Zahl =\n"
                  << "  Organ-Freigabe-Produkt x System-Perms je XML, Summe ueber die Kampagne) und die Mess-Seite\n"
                  << "  (dynamic_dims-Produkt, T-15 x KF-10, Messgeraete-Rekombination als AUSGANG der gemessenen\n"
                  << "  PMC-Torlage -- nie als Vorgabe). MEHRERE Profile = EINE Kampagne (FULL JOIN je Achse).\n"
                  << "  Baut KEINE DLL, misst NICHT, schreibt nichts. PLANER-ETAPPE: der CEB-Rechen-Ort folgt\n"
                  << "  mit dem CEB-Rechen-Vertrag (S-10/S-22); dieser Lauf rechnet im Planer und sagt es.\n"
                  << "    --sekunden-je-op=F       Mess-Kalibrierung (GESCHAETZT; ohne sie bleibt Mess-ETA n/a)\n"
                  << "    --bau-sekunden-je-dll=F  Bau-Trace-Anker je DLL (GESCHAETZT; ohne ihn Bau-ETA n/a)\n"
                  << "    --bytes-je-dll=N         GN-9-Kalibrierwert fuer den Lager-Bedarf\n"
                  << "    --lager-budget-bytes=N   verlangt das Lager-Gate (fail-closed ohne --bytes-je-dll)\n"
                  << "    --t3-fenster-tage=F      verlangt den OV-4-Deckel = f(T-3) als AUSGANG (fail-closed\n"
                  << "                             ohne BEIDE Kalibrierungen -- O4/GN-9)\n"
                  << "    --mess-teilmenge=N       Interims-Eingang der Mess-Teilmenge (kein XML-Traeger; ohne\n"
                  << "                             sie ist die Mess-Gesamtzahl unbestimmbar, nur die Schranke)\n"
                  << "    --kandidaten=N,N,...     Gegenprobe: gerechnete B-4-Zahl gegen diese Kandidaten stellen\n"
                  << "    --fremde-lane=..         DEKLARIERTE Lage der zweiten Lane (nicht gemessen); ohne\n"
                  << "                             Deklaration wird keine zweite Lane erfunden\n"
                  << "  Exit: 0 ok; 1 verlangter Deckel gerissen/unbestimmbar oder nicht erhoben; 2 kaputtes\n"
                  << "        Flag; 5 unbekannte Profil-Wurzel.\n";
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
              << "  status [<profil>] [--root=<dir>]\n"
              << "                          Rueck-Leser: Stand von CEB-Bauten, Tier-Binaries und Messwerten\n"
              << "  check-size [<profil>] [--max-bytes=N] [--max-tage=F] [--sekunden-je-op=F]\n"
              << "                          Mengen-Vorschau VOR dem Lauf: Faktoren, Nenner, Arena, Dauer\n"
              << "  simulate [<profil>...] [Flags, s. 'help simulate']\n"
              << "                          S-19 Planungs-Simulation: B-4-Bau-Zahl + Mess-Menge + Deckel-AUSGANG\n"
              << "  version                 Planer-Selbst-Stempel + Compile-Einstellung\n"
              << "  help [<subcommand>]     diese Uebersicht bzw. Detail-Hilfe (auch: <subcommand> --help)\n\n"
              << "Globales Flag (mit JEDEM Subkommando kombinierbar, Stellung frei):\n"
              << "  --debug                 Wartungs-Modus: nicht-regelkonformes, PARALLELES Messen -- prueft die\n"
              << "                          KETTE (entsteht eine xlsx, wird sie abgelegt?), NICHT die Zahl. Seine\n"
              << "                          Messwerte sind AUSSCHUSS und gehoeren nie ins Messwertlager.\n"
              << "                          FUER ANWENDER GESPERRT (Integritaetsregel): ohne den internen Kontext\n"
              << "                          COMDARE_DEBUG_FREIGABE=true wird der Aufruf mit Exit 8 abgewiesen.\n"
              << "                          Verglichen wird der Wert 'true'; umgebende Leerzeichen werden ignoriert\n"
              << "                          (Haus-Idiom; ein Newline aus der CI sperrt so nicht stumm).\n"
              << "                          Das Flag nimmt KEINEN Wert: '--debug=true' ist ein Fehler, kein Synonym.\n"
              << "                          Das Flag aendert nie die Reihenfolge oder Abhaengigkeiten der Modi,\n"
              << "                          nur ihre Auspraegung.\n\n"
              << "Rollen-Trennung (Section 40.b/42, ZWEI MODULE): die STUFE-2-Sicht (tier ci|cmake) und der\n"
              << "Mess-Vollzug (run) gehoeren der CEB -- comdare-messung-driver. Exit 7 (Lane-Fehlrouting) gibt es\n"
              << "nur dort; diese Binary misst nicht.\n\n"
              << "Profil-Aufloesung: explizites Argument > COMDARE_THESIS_PROFILE > einkompiliertes Default-Profil.\n"
              << "Ausgaben: Daten/Emissionen -> stdout; Diagnose/Fehler -> stderr (clig.dev).\n\n"
              << "Exit-Codes:\n"
              << "  0 Erfolg | 1 Usage/Verstoss/Emission abgebrochen | 2 Konfig-Fehler (kaputte Range)\n"
              << "  5 unbekannte Profil-Wurzel | 6 Bestandslog-Gate-Abbruch | 8 --debug-Zulassung gesperrt\n";
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
              << "' -- erwartet: validate | plan dump|ci|cmake | cache-key | fingerprint | status | check-size | "
                 "simulate | version | help.\n"
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

    // -- A-05/VL-3: DAS --debug-FLAG DIESER SHELL (Ledger 09.08.2026, geltende work_mode-Fassung) ---
    // Owner verbatim: "Damit ist Debug auch eher als Flag entkoppelt, als dass es als State gleichrangig
    // einsortiert wird, denn es beeinflusst zwar die AUSPRAEGUNG der States, aber nicht ihr Verhalten der
    // Reihenfolge oder Abhaengigkeiten. Es ist also ein CLI-Flag auf der Planer-Shell." Und, im selben
    // Abschluss: "Debug ist fuer normale Benutzer gesperrt."
    //
    // WARUM DIE AUSWERTUNG VOR DEM DISPATCH STEHT -- und das ist keine Stil-Frage:
    //   (a) ORTHOGONALITAET. Das Flag hat keine Stellung in der Kette build->measure->compare->release;
    //       es ist mit JEDEM Subkommando kombinierbar. Wer es je Subkommando parst, hat es beim
    //       naechsten vergessen -- genau die Ausnahmenliste, die eine Zulassungsregel aufloest.
    //   (b) DIE SPERRE MUSS ZUERST GREIFEN. Laege sie hinter dem Dispatch, meldete ein gesperrter Lauf
    //       "unbekanntes Subkommando" statt der Sperre, und die Regel waere unbeobachtbar (Wachen-
    //       Doktrin: ein verdeckter Zweig ist kein Zweig).
    // Das Flag wird HIER aus argv entfernt; die Sub-Parser sehen es nie und brauchen keine Kenntnis
    // davon (ihre '-'-Reserve bleibt unveraendert gueltig).
    //
    // WAS ES HEUTE TUT: es traegt sich ein und meldet sich. Die WIRKUNG (maximale Thread-Zahl in jeder
    // Factory, Jitter-Pruefer aus) haengt am work_mode-Umbau und am Drift-Gate-Paket T-15+D4 und wird
    // dort verdrahtet -- Ledger-Bauliste Punkte 9 und 10. Dieses Glied ist der VORBAU: es muss stehen,
    // BEVOR Debug das WorkMode-Enum verlaesst, sonst verliert der einzige debug-Token in XML
    // (m3_smoke_coverage.profile.xml) seinen Traeger ersatzlos.
    bool debug_flag = false;
    {
        std::vector<std::string> ohne_debug;
        ohne_debug.reserve(args.size());
        // '--' beendet die Optionen (POSIX-Konvention). Ohne diese Marke frisst die Schleife jedes
        // `--debug` ab Position 1 -- heute folgenlos, weil ALLE Optionen dieser Shell '='-gefuegt sind
        // (--root=, --max-bytes=, --max-tage=, --sekunden-je-op=) und die Positionals Profil-Pfade
        // sind: ein Argument-WERT kann also nie der Token `--debug` sein. Diese Garantie ruht damit
        // aber allein auf der heutigen Options-Flaeche und nicht auf einer Regel -- die erste
        // leerzeichen-getrennte Wert-Option wuerde sie still brechen. WER HIER EINE SOLCHE OPTION
        // EINFUEHRT, muss nichts aendern; wer das '--' entfernt, schon.
        bool optionen_beendet = false;
        for (std::size_t i = 0; i < args.size(); ++i) {
            if (i > 0 && !optionen_beendet && args[i] == "--") {
                optionen_beendet = true; // die Marke selbst reist nicht weiter
                continue;
            }
            if (i > 0 && !optionen_beendet && args[i] == "--debug") {
                debug_flag = true; // mehrfache Nennung ist kein Fehler, nur Redundanz
                continue;
            }
            // `--debug=<wert>` ist KEINE gueltige Schreibweise: das Flag ist ein Schalter, kein
            // Wert-Paar. Ohne diesen Zweig zerfiel die Form in zwei Verhalten, je nach Stellung
            // (am Objekt gemessen): HINTER dem Subkommando wurde sie STILL geschluckt --
            // `version --debug=true` lief mit rc 0, ohne Debug-Modus und ohne jede Meldung, weil
            // version sein a2 nur gegen --help prueft und resolve_profile jedes '-'-Argument
            // verwirft; VOR dem Subkommando fiel sie in "unbekanntes Subkommando", also laut, aber
            // mit der falschen Ursache. Die stille Haelfte ist die gefaehrliche: wer --debug=true
            // schreibt, glaubt im Debug-Modus zu fahren und faehrt im normalen.
            if (i > 0 && !optionen_beendet && args[i].rfind("--debug=", 0) == 0) {
                std::cerr << "comdare-experiment-planner: FEHLER fehlerklasse=flag_syntax: "
                          << "--debug nimmt keinen Wert -- '" << args[i] << "' ist keine gueltige "
                          << "Schreibweise.\n"
                          << "  --debug ist ein SCHALTER: entweder er steht da oder nicht. Die Zulassung "
                          << "entscheidet allein\n"
                          << "  der interne Freigabe-Kontext (COMDARE_DEBUG_FREIGABE=true), nie ein Wert "
                          << "am Flag.\n";
                return 1;
            }
            ohne_debug.push_back(args[i]);
        }
        args.swap(ohne_debug);
    }
    if (debug_flag) {
        // Die Zulassung entsteht AUSSCHLIESSLICH aus dieser Funktion (measurement/axis_error.hpp) --
        // fail-closed, weil AdmissionStatus::Zugelassen Ordinal 0 ist und eine Default-Initialisierung
        // die Freigabe sonst verschenken wuerde. Das Gate-Idiom ist das des Hauses: exakter Vergleich
        // gegen "true" (wie COMDARE_BESTANDSLOG oben); jeder andere Wert -- "1", "TRUE", leer, nicht
        // gesetzt -- faellt auf gesperrt.
        auto const zulassung = cem::debug_flag_admission(pln::env_trimmed("COMDARE_DEBUG_FREIGABE") == "true");
        if (zulassung != cem::AdmissionStatus::Zugelassen) {
            // WARUM DIE SPERRE EINE INTEGRITAETSREGEL IST, KEIN KOMFORT-GATE (Ledger): debug ist der
            // einzige work_mode, der MISST und dabei PARALLEL laeuft. Parallel gemessene Latenzen sind
            // nicht run-to-run-stabil -- genau die Stabilitaet, die `measure` zusichert. "Ein Anwender,
            // der debug waehlen koennte, bekaeme Zahlen, die wie Messwerte AUSSEHEN und keine sind.
            // Das ist die unheilbare Klasse: kontaminierte Daten."
            std::cerr << "comdare-experiment-planner: FEHLER fehlerklasse=debug_zulassung_gesperrt: "
                      << "--debug ist fuer Anwender GESPERRT (Zulassung: " << cem::admission_status_token(zulassung)
                      << ").\n"
                      << "  GRUND (Integritaetsregel, kein Komfort-Gate): der Debug-Modus misst PARALLEL. "
                      << "Seine Zahlen sehen wie\n"
                      << "  Messwerte aus und sind keine -- sie gehoeren nie ins Messwertlager, nie in "
                      << "eine Auswertung, nie in die Thesis.\n"
                      << "  FREIGABE nur im internen/CI-Kontext: COMDARE_DEBUG_FREIGABE=true (genau dieser "
                      << "Wert, umgebende\n"
                      << "  Leerzeichen werden ignoriert; jeder andere Inhalt -- auch \"1\" oder \"TRUE\" -- "
                      << "bleibt gesperrt).\n";
            return 8;
        }
        // SICHTBAR, nicht still: ein unbemerkter Debug-Lauf ist der Weg, auf dem seine Zahl spaeter fuer
        // einen Messwert gehalten wird. Der Vermerk geht auf stderr, damit stdout emissions-rein bleibt
        // (clig.dev: Daten -> stdout, Diagnose -> stderr). GEMESSEN seit B04 (2026-08-21, VL-3-Rest
        // geschlossen): die stdout-Byte-Gleichheit mit/ohne --debug ist Prozess-Probe in
        // test_vl3_debug_stdout_bytegleich.cpp -- 4 Subkommandos (plan dump|ci|cmake, validate) je als
        // Byte-Paar, dazu Exit-6-/Exit-8-Zweige und ein Determinismus-Anker; T-11c-Biss: EIN
        // zusaetzliches stdout-Byte an dieser Stelle bricht den Vergleich literal (1824 vs 1825).
        std::cerr << "[debug] AKTIV (COMDARE_DEBUG_FREIGABE=true): nicht-regelkonformes Messen freigegeben. "
                  << "Zahlen aus diesem Lauf sind AUSSCHUSS\n"
                  << "        und duerfen nicht ins Messwertlager -- geprueft wird die KETTE, nicht die "
                  << "Zahl.\n";
    }

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
    // W5-ANDOCKPUNKT EINGELOEST (Owner-R5): der reservierte additive if-Zweig. `status` ist rein lesend --
    // es bindet KEINEN planer_block und kann darum nie 6 liefern.
    if (a1 == "status") {
        if (a2 == "--help" || a2 == "-h" || a3 == "--help" || a3 == "-h") {
            help_for("status");
            return 0;
        }
        // Flag und Positional in beliebiger Reihenfolge: --root=<dir> irgendwo, das erste Nicht-Flag ist
        // das Profil (dieselbe '-'-Reserve wie resolve_profile).
        std::string root_arg, prof_arg;
        for (std::size_t i = 2; i < args.size(); ++i) {
            std::string const& a = args[i];
            if (a.rfind("--root=", 0) == 0) {
                root_arg = a.substr(std::string_view{"--root="}.size());
            } else if (!a.empty() && a.front() == '-') {
                std::cerr << "comdare-experiment-planner: unbekanntes Flag '" << a
                          << "' fuer status -- erwartet: --root=<dir> (Detail: 'help status').\n";
                return 1;
            } else if (prof_arg.empty()) {
                prof_arg = a;
            }
        }
        return run_status_guarded(resolve_profile(prof_arg), root_arg);
    }
    // check-size: DERSELBE additive if-Zweig wie `status`. Die Doppelstrich-Schreibweise `--check-size` ist der
    // KANONISCHE FLAG-ALIAS des Subkommandos -- exakt das Muster, das diese Datei schon fuer `version` /
    // `--version` fuehrt (s. Hilfe-Text dort). Das ist KEIN Alt-Flag im Sinne des Datei-Kopfes: die dort
    // ausgeschlossenen Aliase sind die DEPRECATED Treiber-Flags (--dump-plan & Co.), die bewusst nicht
    // mitwandern. Der Alias existiert, weil die Anforderung und die Entwurfs-Doku
    // (docs/architecture/20260808-checkpoint_measure_soll_design.md:283) das Kommando `--check-size` nennen.
    if (a1 == "check-size" || a1 == "--check-size") {
        if (a2 == "--help" || a2 == "-h") {
            help_for("check-size");
            return 0;
        }
        std::string prof_arg, max_bytes_arg, max_tage_arg, sek_arg;
        for (std::size_t i = 2; i < args.size(); ++i) {
            std::string const& a = args[i];
            if (a == "--help" || a == "-h") {
                help_for("check-size");
                return 0;
            }
            if (a.rfind("--max-bytes=", 0) == 0) {
                max_bytes_arg = a.substr(std::string_view{"--max-bytes="}.size());
            } else if (a.rfind("--max-tage=", 0) == 0) {
                max_tage_arg = a.substr(std::string_view{"--max-tage="}.size());
            } else if (a.rfind("--sekunden-je-op=", 0) == 0) {
                sek_arg = a.substr(std::string_view{"--sekunden-je-op="}.size());
            } else if (!a.empty() && a.front() == '-') {
                // rc 2, nicht 1: die Hilfe verspricht "2 kaputtes Flag", und rc 1 ist schon das Urteil
                // "Deckel gerissen" -- eine kaputte Kommandozeile muss davon UNTERSCHEIDBAR sein.
                std::cerr << "comdare-experiment-planner: unbekanntes Flag '" << a
                          << "' fuer check-size -- erwartet: --max-bytes=N | --max-tage=F | --sekunden-je-op=F "
                             "(Detail: 'help check-size').\n";
                return 2;
            } else if (prof_arg.empty()) {
                prof_arg = a;
            } else {
                // Ein NICHT verbrauchtes Positional wird ABGELEHNT, nie verworfen: "max-bytes=1000" (Striche
                // vergessen) saehe sonst wie ein Zweit-Profil aus und verschwaende stumm -- der Deckel gleich mit.
                std::cerr << "comdare-experiment-planner: ueberzaehliges Argument '" << a
                          << "' fuer check-size -- das Profil ist schon '" << prof_arg
                          << "'. Vertipptes Flag ohne '--'? (Detail: 'help check-size').\n";
                return 2;
            }
        }
        return run_check_size_guarded(resolve_profile(prof_arg), max_bytes_arg, max_tage_arg, sek_arg);
    }
    // simulate: additiver if-Zweig wie check-size; --simulate ist der kanonische Flag-Alias (Muster
    // --version/--check-size). MEHRERE Positionale sind hier ERWUENSCHT (eine Kampagne aus N Papers) --
    // der check-size-Schutz gegen ein Zweit-Positional gilt darum bewusst NICHT.
    if (a1 == "simulate" || a1 == "--simulate") {
        if (a2 == "--help" || a2 == "-h") {
            help_for("simulate");
            return 0;
        }
        SimulateArgs sim;
        for (std::size_t i = 2; i < args.size(); ++i) {
            std::string const& a = args[i];
            if (a == "--help" || a == "-h") {
                help_for("simulate");
                return 0;
            }
            if (a.rfind("--sekunden-je-op=", 0) == 0) {
                sim.sek_je_op = a.substr(std::string_view{"--sekunden-je-op="}.size());
            } else if (a.rfind("--bau-sekunden-je-dll=", 0) == 0) {
                sim.bau_sek_je_dll = a.substr(std::string_view{"--bau-sekunden-je-dll="}.size());
            } else if (a.rfind("--bytes-je-dll=", 0) == 0) {
                sim.bytes_je_dll = a.substr(std::string_view{"--bytes-je-dll="}.size());
            } else if (a.rfind("--lager-budget-bytes=", 0) == 0) {
                sim.lager_budget = a.substr(std::string_view{"--lager-budget-bytes="}.size());
            } else if (a.rfind("--t3-fenster-tage=", 0) == 0) {
                sim.t3_fenster = a.substr(std::string_view{"--t3-fenster-tage="}.size());
            } else if (a.rfind("--mess-teilmenge=", 0) == 0) {
                sim.mess_teilmenge = a.substr(std::string_view{"--mess-teilmenge="}.size());
            } else if (a.rfind("--kandidaten=", 0) == 0) {
                sim.kandidaten = a.substr(std::string_view{"--kandidaten="}.size());
            } else if (a.rfind("--fremde-lane=", 0) == 0) {
                sim.fremde_lane = a.substr(std::string_view{"--fremde-lane="}.size());
            } else if (!a.empty() && a.front() == '-') {
                // rc 2, nicht 1 -- dieselbe Unterscheidbarkeits-Regel wie bei check-size.
                std::cerr << "comdare-experiment-planner: unbekanntes Flag '" << a
                          << "' fuer simulate (Detail: 'help simulate').\n";
                return 2;
            } else {
                sim.profile.push_back(a);
            }
        }
        // Kein Positional => dieselbe Aufloesung wie ueberall (env > einkompiliertes Default-Profil).
        if (sim.profile.empty()) sim.profile.push_back(resolve_profile(std::string{}));
        return run_simulate_guarded(sim);
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
