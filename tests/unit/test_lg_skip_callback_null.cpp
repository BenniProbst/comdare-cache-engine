// tests/unit/test_lg_skip_callback_null.cpp -- F2-5 Posten 1: LG-SkipCallback==0 (Designplan par.2/K2,
// woertliches F2-Abnahmekriterium; Abnahme-Formel par.4-W1: "SKIP-Zweitlauf ruft den Mess-Callback 0-mal").
//
// GEGENSTAND (am Objekt gesucht, nicht am Konsumenten): die Lagerhaltungs-Doktrin "gueltiger Bestand =>
// SKIP" lebt im Builder-Iterator als Mess-RESUME (#139): measure_one_binary kehrt bei gueltigem Bestand
// (b.skipped + result.csv + result.csv.stamp via lazy_try_resume_binary) VOR Laden/Messen/Sink zurueck
// (cache_engine_builder_iterator.hpp, Resume-Zweig; "return oc; // Resume-Skip"). Der Mess-Callback ist
// die injizierte Naht LazyRunConfig::measurement_sink (MeasurementSinkFn, artifact_transport) mit GENAU
// EINER Aufrufstelle im Iterator -- sie liegt an der per-Binary-Naht HINTER dem Resume-Zweig.
//
// WAS DIESER TEST NEU BEWEIST (Anrechnung statt Doppelung, A2.2):
//   * test_a9s4_skip_manifest.cpp:113 prueft die xlsx-Skip-Seite (Byte-Identitaet+mtime des Zweitlaufs),
//   * test_lazy_resume_binary.cpp prueft die Resume-ENTSCHEIDUNGSFUNKTION isoliert,
//   * HIER NEU: die CALLBACK-ZAEHLUNG am ECHTEN Lauf (run_lazy_static_then_dynamic): der SKIP-Zweitlauf
//     ruft den Mess-Callback exakt 0-mal, und zwar NICHT als Falsch-Null (der Nenner belegt, dass es
//     etwas zu skippen GAB: resumed_binaries == N).
//
// ZAEHL-NENNER AUS FREMDER QUELLE (T-3): N = 3 stammt aus den Achsen-LITERALEN dieses Tests (die eine
// statische Achse traegt wortwoertlich drei Werte) -- nicht aus einem Zaehler des Prueflings. Jede
// gepruefte Zahl unten traegt diesen Nenner.
//
// GEGENEINGANG (T-4): gegen FEHLENDEN Bestand (Lauf 1) und gegen UNGUELTIGEN Bestand (Lauf 3,
// Zeilenzahl-Luege im Stamp) wird NICHT resumiert (resumed_binaries == 0) und der Lauf betritt die
// Mess-Kette je Zelle real (load_failed == N: jede Zelle hat die Lade-Naht erreicht und ist erst DORT
// gescheitert -- der Stub baut bewusst keine ladbare .so).
//
// EHRLICHE GRENZE + KLEINSTE TEST-SEITIGE SONDE (als solche kommentiert, F2-5-Auflage): die ">0"-Seite
// des Callbacks selbst (Sink feuert fuer eine WIRKLICH gemessene Zelle) verlangt eine ladbare
// Voll-Mess-Tier-Binary durch Loader + Mess-Konsistenz-Gate + Dock -- eine solche Unit-Fixture existiert
// im Baum nicht (die genus-/naht1-Fixtures tragen 2-arg-Stempel OHNE Mess-Deklaration, das r3-Modul ist
// stempel-only). Dieselbe deklarierte Grenze halten test_t15_drift_gate_messschleife Abschnitt (8)
// ("die produktive Mess-Schleife ... laesst sich in einem Unit-Test nicht fahren") und der Kommentar an
// lazy_row_mess_ausstattung_uebernehmen fest. Deshalb deckt Abschnitt (S) die Erreichbarkeits-Ordnung
// als QUELLEN-WACHE auf der Iterator-Quelle (T-15-Praezedenzmuster): genau EINE
// cfg.measurement_sink-Aufrufstelle, und sie steht NACH dem Resume-Skip-Return -- ein geskippter
// Zweitlauf KANN sie nicht erreichen, eine real gemessene Zelle erreicht sie.
//
// Deterministisch, ohne echte DLLs/Messung (Stub-Compile erzeugt die Datei, dll_is_current skippt im
// Zweitlauf ueber den Fingerprint-Anker -- Muster test_a2_sha512_skip_gate (f)); Registrierung nach dem
// test_f3_lager_key_provider_iterator-Muster (schwerer Host-Treiber-Header, plain main, Boost::mp11).
// ASCII-only.

#include <builder/experiment_tree/cache_engine_builder_iterator.hpp>

#include "comdare_test_tmp.hpp" // #278/#24: per-User-Temp gegen CI-Kollisionen

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace ex = ::comdare::cache_engine::builder::experiment;
namespace fs = std::filesystem;

namespace {

int g_fail = 0;

void tr(std::string const& was, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << was << "\n";
    if (!ok) ++g_fail;
}

template <class A, class B>
void eq(std::string const& was, A const& ist, B const& soll) {
    bool const ok = (ist == static_cast<A>(soll));
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << was << " = " << ist;
    if (!ok) std::cout << "  (erwartet " << soll << ")";
    std::cout << "\n";
    if (!ok) ++g_fail;
}

[[nodiscard]] std::string datei_inhalt(fs::path const& p) {
    std::ifstream     f{p, std::ios::binary};
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

void schreibe(fs::path const& p, std::string const& inhalt) {
    std::ofstream f{p, std::ios::binary | std::ios::trunc};
    f.write(inhalt.data(), static_cast<std::streamsize>(inhalt.size()));
}

// Der SPY am ECHTEN Haken: LazyRunConfig::measurement_sink ist der injizierte Mess-Callback des
// Iterators (No-Op-Naht Storage #51). Der Spy ZAEHLT nur -- er behauptet nichts. measure_parallelism
// bleibt 0 (strikt sequentiell), also braucht der Zaehler keine Synchronisation.
struct SinkSpy {
    std::size_t              aufrufe = 0;
    std::vector<std::string> ziele;

    [[nodiscard]] ex::MeasurementSinkFn fn() {
        return [this](fs::path const& /*local_file*/, std::string const& relative_dest) {
            ++aufrufe;
            ziele.push_back(relative_dest);
        };
    }
    void reset() {
        aufrufe = 0;
        ziele.clear();
    }
};

} // namespace

int main() {
    std::cout << "==== F2-5 Posten 1: LG-SkipCallback==0 -- SKIP-Zweitlauf ruft den Mess-Callback 0-mal ====\n";

    // T-3 FREMDER NENNER: die DREI Werte stehen als Literale HIER im Test; kSollBlaetter ist aus ihnen
    // abgezaehlt, nicht aus einem Zaehler des Prueflings uebernommen.
    constexpr std::size_t kSollBlaetter = 3;
    auto                  factory       = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree    tree{factory};
    tree.build({
        ex::AxisLevel{"traversal", {"ART"}, true, "", ""},
        ex::AxisLevel{"node.cl_line", {"1", "2", "3"}, true, "", ""}, // drei Literale -> N = 3
    });
    ex::StaticBinaryView view = tree.static_binary_view();
    eq("Vorbedingung: statische Blaetter (Nenner aus den Achsen-Literalen dieses Tests)", view.size(), kSollBlaetter);

    fs::path const  base = ::comdare::test::user_tmp_dir() / "comdare_lg_skip_callback_null";
    std::error_code ec;
    fs::remove_all(base, ec);

    // Stub-Compile ERZEUGT die Ausgabedatei (bewusst KEINE ladbare .so): dll_is_current verlangt die
    // Existenz, der Fingerprint-Anker traegt den Skip des Zweitlaufs (Muster test_a2_sha512_skip_gate).
    auto compile_touch = [](ex::BuildJob const& j) -> int {
        std::ofstream f{j.output, std::ios::binary | std::ios::trunc};
        f << "STUB-DLL-NICHT-LADBAR";
        return 0;
    };
    auto gen_stub = [](std::string const&) { return std::string{"// lg-skipcb-stub\n"}; };
    auto ram_stub = []() -> std::uint64_t { return ~std::uint64_t{0}; };

    // Fingerprint-Provider (128-hex je Blatt, 'a'/'b'/'c' sind Hex-Alphabet): schreibt in Lauf 1 die
    // .fingerprint-Sidecars und ist in Lauf 2/3 die dll_is_current-Erwartung (=> b.skipped).
    std::map<std::string, std::string> fp_by_id;
    SinkSpy                            spy;

    ex::LazyRunConfig cfg;
    cfg.source_dir           = base / "src";
    cfg.output_dir           = base / "dll";
    cfg.build_version        = "1.0.0.c";
    cfg.per_binary_subdirs   = true; // Traeger der per-Binary-Ablage (result.csv+stamp) UND der Sink-Naht
    cfg.build_parallelism    = 1;
    cfg.max_binaries         = kSollBlaetter;
    cfg.erwartete_mess_zeile = "f2-5-nicht-erreicht (Zellen scheitern VOR dem Gate an der Lade-Naht)";
    cfg.measurement_sink     = spy.fn();

    for (std::size_t i = 0; i < view.size(); ++i)
        fp_by_id[view[i].binary_id] = std::string(128, static_cast<char>('a' + i));
    cfg.bestand_fingerprint_fn = [&fp_by_id](std::string const& id) -> std::string {
        auto const it = fp_by_id.find(id);
        return (it == fp_by_id.end()) ? std::string{} : it->second;
    };

    ex::BuildSelection sel;
    sel.indices    = {0, 1, 2};
    sel.provenance = "explicit";

    // == LAUF 1 (T-4a): FEHLENDER Bestand -> kein Skip, die Mess-Kette wird je Zelle real betreten ====
    {
        ex::LazyRunResult const r1 =
            ex::run_lazy_static_then_dynamic(tree, sel, compile_touch, gen_stub, ram_stub, cfg);
        std::cout << "-- Lauf 1: fehlender Bestand (Nenner N=" << kSollBlaetter << ") --\n";
        eq("Lauf 1: frisch gebaut (built_new) von N", r1.built_new, kSollBlaetter);
        eq("Lauf 1: resumed_binaries von N (fehlender Bestand skippt NICHT)", r1.resumed_binaries, std::size_t{0});
        eq("Lauf 1: load_failed von N (jede Zelle hat die Lade-Naht REAL erreicht)", r1.load_failed, kSollBlaetter);
        eq("Lauf 1: Mess-Callback-Aufrufe (Stub-.so nicht ladbar -> Naht liegt HINTER dem Load)", spy.aufrufe,
           std::size_t{0});
        // Der Lade-Fehler-Zweig schreibt KEINE result.csv -- deshalb ist das Seeden des gueltigen
        // Bestands unten ehrliche Test-Arbeit und keine Doppelung eines Lauf-1-Artefakts.
        for (std::size_t i = 0; i < view.size(); ++i) {
            fs::path const bin_dir = cfg.output_dir / ex::orch_make_stem(view[i].binary_id, view[i].index);
            tr("Lauf 1: keine result.csv in " + bin_dir.filename().string(), !fs::exists(bin_dir / "result.csv", ec));
        }
    }

    // == SEED: GUELTIGER Bestand je Blatt (die Lagerhaltung, gegen die der Zweitlauf skippen MUSS) ====
    // Stamp-Form = <lazy_resume_stamp_prefix><|fpr=hex128>|rows=<N>\n (keine AlgoSigFn -> kein |algos=;
    // dyn_dims sind leer, der Baum traegt nur statische Achsen -- exakt die Form des Iterator-Schreibers).
    std::string const              praefix = ex::lazy_resume_stamp_prefix(cfg, std::vector<ex::DynamicDim>{});
    std::vector<std::string> const seed_marker{"lg_seed_zeile_a", "lg_seed_zeile_b", "lg_seed_zeile_c"};
    for (std::size_t i = 0; i < view.size(); ++i) {
        fs::path const bin_dir = cfg.output_dir / ex::orch_make_stem(view[i].binary_id, view[i].index);
        schreibe(bin_dir / "result.csv", ex::lazy_csv_header() + seed_marker[i] + "\n");
        schreibe(bin_dir / "result.csv.stamp", praefix + "|fpr=" + fp_by_id[view[i].binary_id] + "|rows=1\n");
    }

    // == LAUF 2: DIE ABNAHME-FORMEL (par.4-W1) -- SKIP-Zweitlauf ruft den Mess-Callback 0-mal =========
    {
        spy.reset();
        ex::LazyRunResult const r2 =
            ex::run_lazy_static_then_dynamic(tree, sel, compile_touch, gen_stub, ram_stub, cfg);
        std::cout << "-- Lauf 2: SKIP-Zweitlauf gegen gueltigen Bestand (Nenner N=" << kSollBlaetter << ") --\n";
        eq("Lauf 2: built_skip von N (dll_is_current, nichts neu kompiliert)", r2.built_skip, kSollBlaetter);
        eq("Lauf 2: built_new von N", r2.built_new, std::size_t{0});
        eq("Lauf 2: resumed_binaries von N (der Nenner der 0 -- es GAB etwas zu skippen)", r2.resumed_binaries,
           kSollBlaetter);
        eq("Lauf 2: load_failed von N (der Skip kehrt VOR der Lade-Naht zurueck)", r2.load_failed, std::size_t{0});
        eq("Lauf 2: ABNAHME par.4-W1 -- Mess-Callback-Aufrufe im SKIP-Zweitlauf", spy.aufrufe, std::size_t{0});
        std::size_t uebernommen = 0;
        for (auto const& m : seed_marker)
            if (r2.resumed_csv_rows.find(m) != std::string::npos) ++uebernommen;
        eq("Lauf 2: uebernommene Seed-Zeilen von N (Bestand uebernommen statt neu gemessen)", uebernommen,
           kSollBlaetter);
    }

    // == LAUF 3 (T-4b): UNGUELTIGER Bestand (Zeilenzahl-Luege im Stamp) -> kein Skip =================
    {
        for (std::size_t i = 0; i < view.size(); ++i) {
            fs::path const bin_dir = cfg.output_dir / ex::orch_make_stem(view[i].binary_id, view[i].index);
            schreibe(bin_dir / "result.csv.stamp", praefix + "|fpr=" + fp_by_id[view[i].binary_id] + "|rows=2\n");
        }
        spy.reset();
        ex::LazyRunResult const r3 =
            ex::run_lazy_static_then_dynamic(tree, sel, compile_touch, gen_stub, ram_stub, cfg);
        std::cout << "-- Lauf 3: UNGUELTIGER Bestand (rows=2 gegen 1 echte Zeile; Nenner N=" << kSollBlaetter
                  << ") --\n";
        eq("Lauf 3: resumed_binaries von N (ungueltiger Bestand skippt NICHT)", r3.resumed_binaries, std::size_t{0});
        eq("Lauf 3: load_failed von N (die Mess-Kette wurde je Zelle WIEDER real betreten)", r3.load_failed,
           kSollBlaetter);
        eq("Lauf 3: Mess-Callback-Aufrufe (Fixture-Grenze: Stub-.so nicht ladbar, s. Kopf + Abschnitt S)", spy.aufrufe,
           std::size_t{0});
    }

    // == (S) SONDE: QUELLEN-WACHE der Erreichbarkeits-Ordnung (T-15-Praezedenz, s. Kopf-Kommentar) ====
    // Kleinste test-seitige Sonde fuer die ">0"-Seite: eine real gemessene Zelle erreicht die EINE
    // Sink-Aufrufstelle, ein Resume-Skip kehrt textlich DAVOR zurueck. Anker sind CODE-Literale des
    // Resume-Gates, des Skip-Returns und des Sink-Aufrufs -- keine eigens eingebauten Marker.
    {
        std::cout << "-- (S) Quellen-Wache auf der Iterator-Quelle --\n";
        std::string const quelle = datei_inhalt(fs::path{COMDARE_LG_SKIPCB_ITERATOR_QUELLE});
        tr("(S) Iterator-Quelle lesbar und nicht leer", !quelle.empty());

        std::string const gate = "if (cfg.resume_completed_binaries && cfg.per_binary_subdirs && b.skipped";
        std::string const ret  = "return oc; // Resume-Skip";
        std::string const sink = "cfg.measurement_sink(";

        std::size_t sink_vorkommen = 0;
        for (std::size_t p = quelle.find(sink); p != std::string::npos; p = quelle.find(sink, p + sink.size()))
            ++sink_vorkommen;
        eq("(S) Aufrufstellen von cfg.measurement_sink( im Iterator (Nenner: ganze Quelldatei)", sink_vorkommen,
           std::size_t{1});

        std::size_t const p_gate = quelle.find(gate);
        std::size_t const p_ret  = quelle.find(ret);
        std::size_t const p_sink = quelle.find(sink);
        tr("(S) Resume-Gate-Literal gefunden", p_gate != std::string::npos);
        tr("(S) Resume-Skip-Return gefunden", p_ret != std::string::npos);
        tr("(S) Ordnung: Gate VOR Skip-Return VOR Sink-Aufruf (Skip kann den Callback nicht erreichen)",
           p_gate != std::string::npos && p_ret != std::string::npos && p_sink != std::string::npos && p_gate < p_ret &&
               p_ret < p_sink);
    }

    std::cout << (g_fail == 0 ? "ALLE PRUEFUNGEN GRUEN" : "FEHLER: " + std::to_string(g_fail) + " Pruefung(en) rot")
              << "\n";
    return g_fail == 0 ? 0 : 1;
}
