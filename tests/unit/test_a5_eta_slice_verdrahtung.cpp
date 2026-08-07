// test_a5_eta_slice_verdrahtung.cpp -- A5 / F5-ETA-Paket, der WIRK-NACHWEIS am echten Bau-Weg.
//
// WOZU DIESER ZWEITE TEST, wo test_a5_eta_kalibrierung die Arithmetik schon literal abdeckt:
// Die Arithmetik war NIE das Problem. eta_estimator.hpp rechnete seit der B4-Scheibe korrekt -- er hatte
// nur keinen produktiven Aufrufer. project_slice_eta_s wurde von genau NULL Produktionsstellen gerufen,
// und apply_calibration ausschliesslich unmittelbar VOR mark_done: die Reservierung bekam ihre ETA also
// erst, als sie nicht mehr offen und damit nie mehr uebernehmbar war. Ein Test der reinen Funktionen
// haette diese Luecke nie gezeigt -- er war ja gruen.
//
// DESHALB PRUEFT DIESER TEST GENAU EINE SACHE, UND ZWAR AM OBJEKT: steht die ETA im Bestandslog,
// WAEHREND der Slice laeuft? Das ist der Beweis-Anker der F5-Spez ("Bestandslog-Reservierung traegt
// eta_s VOR Slice-Ende, nicht erst bei Done"). Weil der Endzustand des Dokuments diese Frage NICHT
// beantworten kann (am Ende steht dort ein done-Record), fuehrt der FakeStore eine HISTORIE aller
// Schreibvorgaenge. Der Beleg ist ein Dokument-Stand, in dem eine Reservierung GLEICHZEITIG
// status="offen" und ein gefuelltes eta_s traegt.
//
// UND DER GEGENFALL: ein Fenster, das nicht groesser ist als sein eigener Mini-Batch, erreicht die
// Belastbarkeits-Schwelle erst, wenn nichts mehr offen ist. Dann wird NICHTS geschrieben -- kein
// Rest-ETA, keine 0, kein Platzhalter. Ohne diesen zweiten Fall waere der erste unbewiesen.
//
// Deterministisch, ohne minio/mc: FakeTransport (in-Memory), Stub-Compile mit kontrollierter Dauer.
// Build: plain main (KEIN gtest) -- Muster test_tp1_planer_filter_iterator (schwerer Host-Treiber-Header).

#include <builder/experiment_tree/cache_engine_builder_iterator.hpp>

#include "comdare_test_tmp.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace ex = ::comdare::cache_engine::builder::experiment;
namespace bl = ::comdare::cache_engine::builder::bestandslog;
namespace fs = std::filesystem;

namespace {

int g_fail = 0;

void check_true(char const* what, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!ok) ++g_fail;
}
template <typename A, typename B>
void check_eq(char const* what, A const& got, B const& want) {
    bool const ok = (got == static_cast<A>(want));
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) {
        std::cout << "  (erwartet: " << want << ")";
        ++g_fail;
    }
    std::cout << "\n";
}

// In-Memory-Transport MIT HISTORIE. Die Historie ist der ganze Punkt: sie haelt jeden Zwischenstand fest,
// den der Endzustand ueberschreibt.
struct HistorienStore {
    std::map<std::string, std::string> objs;
    std::vector<std::string>           historie; // jeder store()-Inhalt, in Schreib-Reihenfolge

    bl::BestandTransport transport() {
        bl::BestandTransport t;
        t.fetch = [this](std::string const& k) -> std::optional<std::string> {
            auto it = objs.find(k);
            if (it == objs.end()) return std::nullopt;
            return it->second;
        };
        t.store = [this](std::string const& k, std::string const& c) -> bool {
            objs[k] = c;
            historie.push_back(c);
            return true;
        };
        t.remove = [this](std::string const& k) -> bool {
            objs.erase(k);
            return true;
        };
        t.stat = [this](std::string const& k) -> std::optional<bl::ObjectStat> {
            auto it = objs.find(k);
            if (it == objs.end()) return std::nullopt;
            return bl::ObjectStat{.size = it->second.size(), .mtime_epoch_s = 0};
        };
        return t;
    }
};

constexpr char const* kDocKey = "bestandslog/test_a5_eta.xml";

class CerrCapture {
public:
    CerrCapture() : alt_{std::cerr.rdbuf(puffer_.rdbuf())} {}
    CerrCapture(CerrCapture const&)            = delete;
    CerrCapture& operator=(CerrCapture const&) = delete;
    ~CerrCapture() { std::cerr.rdbuf(alt_); }
    [[nodiscard]] std::string text() const { return puffer_.str(); }

private:
    std::ostringstream puffer_;
    std::streambuf*    alt_;
};

// 8 statische Blaetter (1x4x2) -- dasselbe Muster wie test_tp1_planer_filter_iterator.
ex::ExperimentTree make_tree(std::shared_ptr<ex::ExperimentNodeFactory> const& f) {
    ex::ExperimentTree t{f};
    t.build({
        ex::AxisLevel{"traversal", {"ART"}, true, ""},
        ex::AxisLevel{"node", {"N4", "N16", "N48", "N256"}, true, ""},
        ex::AxisLevel{"node.cl_line", {"64", "128"}, true, ""},
        ex::AxisLevel{"concurrency", {"1", "2"}, false, "thread_count"},
    });
    return t;
}

ex::LazyRunConfig make_cfg(HistorienStore& store, fs::path const& out, std::size_t parallel) {
    ex::LazyRunConfig cfg;
    cfg.source_dir         = out / "src";
    cfg.output_dir         = out / "dll";
    cfg.per_binary_subdirs = true;
    cfg.build_parallelism  = parallel;
    cfg.provision_only     = true;
    cfg.bestand_transport  = store.transport();
    cfg.bestand_doc_key    = kDocKey;
    cfg.bestand_owner_uuid = "uuid-a5-eta";
    cfg.bestand_maschine   = "prodX";
    cfg.marker_kontext     = ex::MarkerKontext{"amd", "[O2,avx2][a,b,c]", "[all]"};
    return cfg;
}

// DER BELEG-SUCHER: gibt es in der Historie einen Dokument-Stand, in dem eine Reservierung
// GLEICHZEITIG offen ist UND eine brauchbare ETA traegt? Genau diese Gleichzeitigkeit ist die Aussage
// "die Zahl war da, als sie noch jemandem genuetzt haette".
struct OffenMitEta {
    bool        gefunden = false;
    std::string eta_s;
    std::string avg_size_bytes;
};

// Ein Feld aus der Testat-Zeile ziehen ("... vorhersage_s=0.067 gemessen_s=0.076 ...").
[[nodiscard]] std::optional<double> testat_feld(std::string const& spur, std::string const& name) {
    auto const p = spur.find(name);
    if (p == std::string::npos) return std::nullopt;
    auto const anfang = p + name.size();
    auto const ende   = spur.find_first_of(" \n", anfang);
    return bl::parse_seconds(spur.substr(anfang, ende - anfang));
}

[[nodiscard]] OffenMitEta suche_offen_mit_eta(std::vector<std::string> const& historie) {
    OffenMitEta fund;
    for (auto const& xml : historie) {
        auto const doc = bl::parse_bestandslog(xml);
        if (!doc) continue;
        for (auto const& r : doc->reservierungen) {
            if (r.status != bl::BatchStatus::offen) continue;
            if (!bl::has_usable_eta(r.eta_s)) continue;
            fund.gefunden       = true;
            fund.eta_s          = r.eta_s;
            fund.avg_size_bytes = r.avg_size_bytes;
            return fund; // der ERSTE solche Stand genuegt als Beleg
        }
    }
    return fund;
}

} // namespace

int main() {
    std::cout << "==== A5/F5: ETA-Kalibrierung WAEHREND des Slices (Wirk-Nachweis am Bau-Weg) ====\n";

    auto                 factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree   tree    = make_tree(factory);
    ex::StaticBinaryView view    = tree.static_binary_view();
    check_eq("Vorbedingung: 8 statische Blaetter", view.size(), std::size_t{8});

    fs::path const  base = ::comdare::test::user_tmp_dir() / "comdare_a5_eta";
    std::error_code ec;
    fs::remove_all(base, ec);

    // Ein Compile mit MESSBARER Dauer -- ohne sie waere jede t_i null und die Kalibrierung haette
    // (korrekterweise) nichts zu buchen. 25 ms je Bau: gross genug fuer jede Uhr, klein genug fuer die CI.
    auto compile_stub = [](ex::BuildJob const&) -> int {
        std::this_thread::sleep_for(std::chrono::milliseconds{25});
        return 0;
    };
    auto gen_stub = [](std::string const&) { return std::string{"// a5-eta-stub\n"}; };

    std::vector<std::size_t> const alle{0, 1, 2, 3, 4, 5, 6, 7};

    // ===================================================================================
    // FALL A -- DER TREFFER: Fenster (8) groesser als der Mini-Batch (3 Worker).
    // Nach 3 fertigen Compiles sind 5 Binaries offen -> die Projektion traegt und wird GESCHRIEBEN,
    // waehrend der Slice noch laeuft.
    // ===================================================================================
    {
        HistorienStore    store;
        ex::LazyRunConfig cfg = make_cfg(store, base / "treffer", 3);
        ex::BuildConfig   bcfg;
        bcfg.cores_per_build    = 1;
        bcfg.source_dir         = cfg.source_dir;
        bcfg.output_dir         = cfg.output_dir;
        bcfg.per_binary_subdirs = true;
        bcfg.build_parallelism  = 3;
        ex::BuildOrchestrator orch{bcfg, compile_stub, gen_stub};

        ex::BuildStats agg;
        std::string    spur;
        {
            CerrCapture fang;
            (void)ex::run_planer_driven_provision(orch, view, alle, cfg, agg, bl::PresenceFn{});
            spur = fang.text();
        }

        check_eq("(A) alle 8 gebaut", agg.built, std::size_t{8});

        auto const fund = suche_offen_mit_eta(store.historie);
        check_true("(A) BEWEIS-ANKER: ein Dokument-Stand traegt status=offen UND eine brauchbare eta_s", fund.gefunden);
        if (fund.gefunden) {
            std::cout << "        gefundene eta_s=\"" << fund.eta_s << "\" avg_size_bytes=\"" << fund.avg_size_bytes
                      << "\"\n";
            auto const wert = bl::parse_seconds(fund.eta_s);
            check_true("(A) die eta_s ist strikt parsbar", wert.has_value());
            // UNTERGRENZE: die Projektion darf nie unter EINEN Compile fallen. Ein Bau dauert hier >= 25 ms,
            // und es sind mindestens 5 Binaries offen -- bei 3 Workern also >= 2 Wellen a 25 ms.
            if (wert) check_true("(A) eta_s >= 0.025 s (mindestens ein Compile)", *wert >= 0.025);
            check_true("(A) eta_s > 0", wert && *wert > 0.0);
            // HONEST-EMPTY JE FELD, am realen Weg: der Stub-Compile legt keine Datei an, also ist keine
            // Binary vermessen worden -- und avg_size_bytes bleibt LEER statt "0" zu behaupten. Genau
            // dieser Fall (Zeit-Grundlage da, Groessen-Grundlage nicht) unterscheidet
            // uebertrage_kalibrierung von apply_calibration.
            check_eq("(A) avg_size_bytes ist LEER (kein Artefakt vermessen), nicht \"0\"", fund.avg_size_bytes,
                     std::string{});
        }

        // DER ZAHLEN-BISSBEWEIS AM INTEGRIERTEN WEG: die Vorhersage des Blocks gegen seine eigene
        // Wanduhr. Das Band ist bewusst weit (CI-Uhren rauschen), aber es faengt genau die Fehlerklasse,
        // die eine ETA wertlos macht -- eine Einheiten- oder Faktor-Verwechslung waere sofort draussen.
        auto const vorhersage = testat_feld(spur, "vorhersage_s=");
        auto const gemessen   = testat_feld(spur, "gemessen_s=");
        check_true("(A) Testat traegt beide Zahlen", vorhersage.has_value() && gemessen.has_value());
        if (vorhersage && gemessen && *gemessen > 0.0) {
            double const quotient = *vorhersage / *gemessen;
            std::cout << "        Vorhersage/Messung = " << quotient << "\n";
            check_true("(A) Vorhersage liegt im Band [0,5 ; 1,5] x Messung", quotient >= 0.5 && quotient <= 1.5);
        }

        // Der Endzustand ist ein done-Record MIT der IST-Wanduhr -- das bisherige Verhalten bleibt als
        // Abschluss-Testat erhalten (F5-Spez Baupunkt 4).
        auto const ende = bl::parse_bestandslog(store.objs[kDocKey]);
        check_true("(A) Endzustand parsbar", ende.has_value());
        if (ende) {
            bool done_mit_eta = false;
            for (auto const& r : ende->reservierungen)
                if (r.status == bl::BatchStatus::done && bl::has_usable_eta(r.eta_s)) done_mit_eta = true;
            check_true("(A) am Ende steht der done-Record mit der IST-Wanduhr (unveraendertes Verhalten)",
                       done_mit_eta);
        }

        // Das Testat ist nicht stumm und traegt seinen eigenen Nenner.
        check_true("(A) Kalibrier-Testat im Log", spur.find("[bestandslog] eta-kalibrierung:") != std::string::npos);
        check_true("(A) Testat traegt punkte=<ist>/<soll>", spur.find(" punkte=") != std::string::npos);
        check_true("(A) Testat traegt die Zahl der Mid-Slice-Schriebe", spur.find(" schriebe=") != std::string::npos);
        std::cout << "        Testat-Zeile: ";
        if (auto const p = spur.find("[bestandslog] eta-kalibrierung:"); p != std::string::npos)
            std::cout << spur.substr(p, spur.find('\n', p) - p);
        std::cout << "\n";
    }

    // ===================================================================================
    // FALL B -- DER GEGENFALL: Fenster (8) NICHT groesser als der Mini-Batch (8 Worker).
    // Die Schwelle wird erst erreicht, wenn nichts mehr offen ist. Ergebnis: KEINE Zahl.
    // Ein Schaetzer, der hier etwas geschrieben haette, haette aus 8 Punkten auf 0 Rest hochgerechnet.
    // ===================================================================================
    {
        HistorienStore    store;
        ex::LazyRunConfig cfg = make_cfg(store, base / "zu_klein", 8);
        ex::BuildConfig   bcfg;
        bcfg.cores_per_build    = 1;
        bcfg.source_dir         = cfg.source_dir;
        bcfg.output_dir         = cfg.output_dir;
        bcfg.per_binary_subdirs = true;
        bcfg.build_parallelism  = 8;
        ex::BuildOrchestrator orch{bcfg, compile_stub, gen_stub};

        ex::BuildStats agg;
        std::string    spur;
        {
            CerrCapture fang;
            (void)ex::run_planer_driven_provision(orch, view, alle, cfg, agg, bl::PresenceFn{});
            spur = fang.text();
        }

        check_eq("(B) alle 8 gebaut", agg.built, std::size_t{8});
        auto const fund = suche_offen_mit_eta(store.historie);
        check_true("(B) EHRLICH: KEIN offener Record traegt eine ETA -- das Fenster war zu klein", !fund.gefunden);
        check_true("(B) und es wurde auch nichts geschrieben (schriebe=0)",
                   spur.find(" schriebe=0 ") != std::string::npos);
        std::cout << "        Testat-Zeile: ";
        if (auto const p = spur.find("[bestandslog] eta-kalibrierung:"); p != std::string::npos)
            std::cout << spur.substr(p, spur.find('\n', p) - p);
        std::cout << "\n";
    }

    // ===================================================================================
    // FALL C -- DER PUSH-HOOK LEBT NOCH. add_on_binary_done statt set_on_binary_done: ohne diese
    // Unterscheidung haette die ETA-Verdrahtung den bestehenden Completion-Hook STILL verdraengt.
    // Hier wird ein eigener Hook VORHER registriert und nachgewiesen, dass er weiterhin feuert.
    // ===================================================================================
    {
        HistorienStore    store;
        ex::LazyRunConfig cfg = make_cfg(store, base / "hook", 3);
        ex::BuildConfig   bcfg;
        bcfg.cores_per_build    = 1;
        bcfg.source_dir         = cfg.source_dir;
        bcfg.output_dir         = cfg.output_dir;
        bcfg.per_binary_subdirs = true;
        bcfg.build_parallelism  = 3;
        ex::BuildOrchestrator orch{bcfg, compile_stub, gen_stub};

        std::atomic<std::size_t> erster_hook{0};
        orch.set_on_binary_done([&erster_hook](ex::BuildResult const&) { ++erster_hook; });

        ex::BuildStats agg;
        {
            CerrCapture fang;
            (void)ex::run_planer_driven_provision(orch, view, alle, cfg, agg, bl::PresenceFn{});
        }
        check_eq("(C) der ZUERST registrierte Hook feuerte fuer alle 8 Binaries", erster_hook.load(), std::size_t{8});
        check_true("(C) und die ETA-Kalibrierung lief trotzdem", suche_offen_mit_eta(store.historie).gefunden);
    }

    // ===================================================================================
    // FALL D -- PER-BINARY-TIMING liegt im Ergebnis (Baupunkt 1), und ein NICHT kompilierter Job
    // traegt KEINE Dauer statt einer 0.
    // ===================================================================================
    {
        HistorienStore    store;
        ex::LazyRunConfig cfg = make_cfg(store, base / "timing", 2);
        ex::BuildConfig   bcfg;
        bcfg.cores_per_build    = 1;
        bcfg.source_dir         = cfg.source_dir;
        bcfg.output_dir         = cfg.output_dir;
        bcfg.per_binary_subdirs = true;
        bcfg.build_parallelism  = 2;
        ex::BuildOrchestrator orch{bcfg, compile_stub, gen_stub};

        ex::BuildStats               agg;
        std::vector<ex::BuildResult> builds;
        {
            CerrCapture fang;
            builds = ex::run_planer_driven_provision(orch, view, alle, cfg, agg, bl::PresenceFn{});
        }
        std::size_t mit_dauer    = 0;
        bool        alle_positiv = true;
        for (auto const& b : builds) {
            if (!b.dauer_s) continue;
            ++mit_dauer;
            if (!(*b.dauer_s >= 0.025)) alle_positiv = false;
        }
        check_eq("(D) alle 8 gebauten Binaries tragen eine gemessene Dauer", mit_dauer, std::size_t{8});
        check_true("(D) jede gemessene Dauer >= 25 ms (die Stub-Bauzeit)", alle_positiv);
    }

    std::cout << (g_fail == 0 ? "==== A5/F5-VERDRAHTUNG: ALLE PRUEFUNGEN BESTANDEN ====\n"
                              : "==== A5/F5-VERDRAHTUNG: FEHLER ====\n");
    return g_fail == 0 ? 0 : 1;
}
