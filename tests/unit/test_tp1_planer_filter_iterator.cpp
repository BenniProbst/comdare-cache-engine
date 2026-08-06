// test_tp1_planer_filter_iterator.cpp -- Lager-TP1-Nachbesserung N3 (B-2/B-3): der ITERATOR-EBENE-
// Testanker fuer den Bau-Filter, der bis dahin nur auf Bausteine-Ebene belegt war.
//
// WAS HIER BEWIESEN WIRD (gegen den ECHTEN run_planer_driven_provision + BuildOrchestrator mit
// Stub-Compile, Muster test_w6_parallel_provision):
//   (1) builds-SCHRUMPFUNG: mit Praesenz-Praedikat traegt der builds-Vektor NUR die gebauten
//       (fehlenden) Binaries -- die Lager-Treffer fehlen darin und reisen als Zahl im out-Param.
//   (2) BUCHUNG: Bestand-Skips zaehlen wie dll_is_current-Hits (succeeded+skipped+total_jobs);
//       built_new bleibt die Zahl der Fehlenden, built_skip die der Vorhandenen.
//   (3) RESERVIERUNG: der Slice-Claim beansprucht das GANZE Fenster (slice_count == Fenster), wird
//       Done geschrieben und traegt eine Kalibrierung.
//   (4) OHNE Praedikat: byte-identischer Ist-Pfad (volles Fenster, 0 Skips) -- der Anker, an dem die
//       Golden-Neutralitaet des inaktiven Filters haengt.
//   (5) VOLLER run_lazy_static_then_dynamic (provision_only): das done-Delta des Paragraf-38-Kanals
//       traegt die VOLLE bereitgestellte Menge (gebaut + Lager-Bestand), je gebauter Binary genau
//       ein Konfigurations-Delta (B-2b), und LazyRunResult.bestand_lager_skips liefert die
//       Differenzierung der zwei Skip-Quellen (B-3).
//
// ADDITIV E-04-P1 (Marker-Familie v2, Slice-Kanal):
//   (1m) je Fenster genau EIN [PLAN-TESTAT] (Soll: gesamt/lager/zu_bauen) und EIN [BILANZ-TESTAT]
//        (Ist: gebaut_neu/sidecar_skip/lager_skip/plan_skip/fehl) -- die Zahlen stammen aus DEMSELBEN
//        slice_stats/Filter-Ergebnis wie die geprueften Aggregat-Zahlen (Single-Source-Beleg).
//        plan_skip= kam mit T2-A/F4-BILANZ (2026-08-06) dazu und steht in JEDER Zeile der Marke.
//   (6m) FALLBACK-KANAL ohne aktives Bestandslog: der Live-Kanal bleibt beziffert (nicht stumm), und
//        die Bilanz macht built_new/built_skip zu Konsumenten. Die Pflichtfelder lane=/zelle=/fenster=
//        reisen aus cfg.marker_kontext in JEDE Zeile -- das Substrat des (zelle, fenster)-Keys.
//
// Deterministisch, ohne minio/mc: FakeTransport (in-Memory), Stub-Compile ohne echte DLLs.
// Build: plain main (KEIN gtest), Return 0/1 -- Registrierung nach dem test_lazy_resume_binary-Muster
// (schwerer Host-Treiber-Header + Boost::mp11).

#include <builder/experiment_tree/cache_engine_builder_iterator.hpp>

#include "comdare_test_tmp.hpp" // #278/#24: per-User-Temp gegen CI-Kollisionen

#include <algorithm> // CX-W1: std::any_of ueber die Dokument-Eintraege
#include <atomic>    // T2-A/F4-NB2 (11h): das deterministische Alt-Fenster der Push-Barriere
#include <cstddef>
#include <filesystem>
#include <fstream> // Alt-Staende der Faelle (6)/(9) legen und zurueckliegen
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept> // CX-W1: std::runtime_error als simulierter Push-Wurf
#include <string>
#include <thread> // T2-A/F4-NB2 (11h): yield() statt sleep -- die Barriere wird beobachtet, nicht gehofft
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

// 8 statische Blaetter (1x4x2) + eine dynamische Dimension -- exakt das w6-Muster.
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

// In-Memory-Transport (Muster test_g3_takeover_sweep, ohne Fehler-Skript).
struct FakeStore {
    std::map<std::string, std::string> objs;

    bl::BestandTransport transport() {
        bl::BestandTransport t;
        t.fetch = [this](std::string const& k) -> std::optional<std::string> {
            auto it = objs.find(k);
            if (it == objs.end()) return std::nullopt;
            return it->second;
        };
        t.store = [this](std::string const& k, std::string const& c) -> bool {
            objs[k] = c;
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

constexpr char const* kDocKey = "bestandslog/test_bestand.xml";

// cerr-Fang fuer die Testat-Zeilen (Muster test_g3_takeover_sweep).
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

// Gemeinsame Lauf-Konfiguration (frisches Ausgabe-Verzeichnis je Fall; dll_is_current skippt nie => die
// EINZIGE Skip-Quelle ist der Bau-Filter -> saubere Zurechnung). A2-EICHUNG (GATE 5, F7, 2026-08-05): das
// Nie-Skippen haengt jetzt daran, dass hier KEIN Fingerprint-Provider gesetzt ist (kein expected =>
// fail-closed false); vorher trug es die leere build_version. Zurechnung und Erwartungswerte unveraendert.
ex::LazyRunConfig make_cfg(FakeStore& store, fs::path const& out) {
    ex::LazyRunConfig cfg;
    cfg.source_dir         = out / "src";
    cfg.output_dir         = out / "dll";
    cfg.per_binary_subdirs = true;
    cfg.build_parallelism  = 1;
    cfg.provision_only     = true;
    cfg.bestand_transport  = store.transport();
    cfg.bestand_doc_key    = kDocKey;
    cfg.bestand_owner_uuid = "uuid-tp1-anker";
    cfg.bestand_maschine   = "prodX";
    // E-04-P1: die Pflicht-Koordinaten der Marker-Familie (im Betrieb aus COMDARE_LANE/GN_OPT/GN_SIMD/
    // MEASUREMENT_COMBO; hier fest, damit die Zeilen-Pins unabhaengig von der Test-Umgebung greifen).
    cfg.marker_kontext = ex::MarkerKontext{"amd", "[O2,avx2][a,b,c]", "[all]"};
    return cfg;
}

// Zaehlt nicht-ueberlappende Vorkommen (Marker-Zeilen-Pins; Muster count_occurrences der Director-Tests).
[[nodiscard]] std::size_t zaehle(std::string const& heu, std::string const& nadel) {
    if (nadel.empty()) return 0;
    std::size_t n = 0;
    for (std::size_t p = heu.find(nadel); p != std::string::npos; p = heu.find(nadel, p + nadel.size())) ++n;
    return n;
}

} // namespace

int main() {
    std::cout << "==== TP1-N3: Iterator-Anker Bau-Filter (Buchung + builds-Schrumpfung + done-Menge) ====\n";

    auto                 factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree   tree    = make_tree(factory);
    ex::StaticBinaryView view    = tree.static_binary_view();
    check_eq("Vorbedingung: 8 statische Blaetter", view.size(), std::size_t{8});

    fs::path const  base = ::comdare::test::user_tmp_dir() / "comdare_tp1_anker";
    std::error_code ec;
    fs::remove_all(base, ec);

    auto compile_stub = [](ex::BuildJob const&) -> int { return 0; };
    auto gen_stub     = [](std::string const&) { return std::string{"// tp1-anker-stub\n"}; };

    std::vector<std::size_t> const alle{0, 1, 2, 3, 4, 5, 6, 7};
    // Praesenz-Praedikat der Faelle (1)-(3): Indizes 0..2 liegen "im Lager".
    bl::PresenceFn const drei_im_bestand = [](std::size_t i) { return i < 3; };

    // -- Faelle (1)-(3): run_planer_driven_provision DIREKT, mit Filter --------------------------
    {
        FakeStore         store;
        ex::LazyRunConfig cfg = make_cfg(store, base / "gefiltert");
        ex::BuildConfig   bcfg;
        bcfg.cores_per_build    = 1;
        bcfg.source_dir         = cfg.source_dir;
        bcfg.output_dir         = cfg.output_dir;
        bcfg.per_binary_subdirs = true;
        bcfg.build_parallelism  = 1;
        ex::BuildOrchestrator orch{bcfg, compile_stub, gen_stub};

        ex::BuildStats agg;
        std::size_t    skips = 0;
        CerrCapture    cerr_fang;
        auto const     builds = ex::run_planer_driven_provision(orch, view, alle, cfg, agg, drei_im_bestand, &skips);

        check_eq("(1) builds-Schrumpfung: nur die 5 Fehlenden gebaut", builds.size(), std::size_t{5});
        bool nur_fehlende = builds.size() == 5;
        for (auto const& b : builds)
            if (b.index < 3) nur_fehlende = false;
        check_true("(1) kein Lager-Treffer im builds-Vektor (alle index >= 3)", nur_fehlende);
        check_eq("(1) out-Param bestand_skips", skips, std::size_t{3});

        check_eq("(2) Buchung: total_jobs == Fenster (8)", agg.total_jobs, std::size_t{8});
        check_eq("(2) Buchung: succeeded == 8 (gebaut + Bestand)", agg.succeeded, std::size_t{8});
        check_eq("(2) Buchung: skipped == 3 (nur Bestand; dll_is_current fail-closed ohne Provider)", agg.skipped,
                 std::size_t{3});
        check_eq("(2) Buchung: built == 5 (die Fehlenden)", agg.built, std::size_t{5});
        check_true("(2) Testat-Zeile 'bau-filter: 3' vorhanden",
                   cerr_fang.text().find("bau-filter: 3") != std::string::npos);

        // -- (1m) E-04-P1: die Marker-Familie v2 am realen Slice-Pfad --------------------------------
        std::string const spur = cerr_fang.text();
        check_eq("(1m) genau EIN [PLAN-TESTAT] je Fenster", zaehle(spur, "[PLAN-TESTAT] "), std::size_t{1});
        check_eq("(1m) genau EIN [BILANZ-TESTAT] je Fenster", zaehle(spur, "[BILANZ-TESTAT] "), std::size_t{1});
        // Pflichtfelder: die Zeile ist FUER SICH zuordenbar (kein Rueckgriff auf den Zeilen-Kontext).
        check_eq("(1m) lane= in jeder Marker-Zeile", zaehle(spur, " lane=amd "), std::size_t{2});
        check_eq("(1m) zelle= in jeder Marker-Zeile", zaehle(spur, " zelle=[O2,avx2][a,b,c] "), std::size_t{2});
        check_eq("(1m) ceb= in jeder Marker-Zeile (eigener Layer)", zaehle(spur, " ceb=[all] "), std::size_t{2});
        // fenster= traegt die GLOBALEN View-Indizes des Fensters (hier das volle 0:8).
        check_eq("(1m) fenster= in jeder Marker-Zeile", zaehle(spur, " fenster=0:8 "), std::size_t{2});
        // Die Zaehler stammen aus DEMSELBEN Filter-/slice_stats-Ergebnis wie die Aggregat-Pruefungen oben.
        check_true("(1m) SOLL: gesamt=8 lager=3 zu_bauen=5",
                   spur.find(" gesamt=8 lager=3 zu_bauen=5") != std::string::npos);
        // T2-A/F4-BILANZ (2026-08-06): plan_skip= ist neu und steht in JEDER Zeile der Marke (EINE
        // Grammatik). Hier ist es 0 und das ist die Wahrheit -- dieses Fenster hat den Slice-Loop
        // erreicht, ist also per Definition nicht plan-resumiert.
        check_true("(1m) IST: gebaut_neu=5 sidecar_skip=0 lager_skip=3 plan_skip=0 fehl=0",
                   spur.find(" gebaut_neu=5 sidecar_skip=0 lager_skip=3 plan_skip=0 fehl=0") != std::string::npos);
        check_true("(1m) die Bilanz-Zeile traegt die Fenster-Wall-Clock", spur.find(" dauer_s=") != std::string::npos);
        // TP1FK1-B1 (Codex-Befund CX-W2), GEGENPROBE zu Fall (7): ein ZUSAMMENHAENGENDES Fenster loest die
        // konservative Weitung NICHT aus. Diese Zeile ist der Beleg, dass die Spannen-Form im Regelfall
        // (dichte Selektion) byte-identisch zur frueheren (front(), size())-Form bleibt -- daran haengt die
        // Golden-Neutralitaet des Fixes, nicht nur seine Korrektheit im Sonderfall.
        check_true("(3) dichtes Fenster: KEINE Weitungs-Warnung (byte-identisch zur (front,size)-Form)",
                   spur.find("Slice-Fenster ist NICHT zusammenhaengend") == std::string::npos);

        auto const raw = store.objs.find(kDocKey);
        check_true("(3) Reservierung im Store", raw != store.objs.end());
        if (raw != store.objs.end()) {
            auto const doc = bl::parse_bestandslog(raw->second);
            check_true("(3) Dokument parsebar", doc.has_value());
            if (doc) {
                check_eq("(3) genau EINE Slice-Reservierung", doc->reservierungen.size(), std::size_t{1});
                if (doc->reservierungen.size() == 1) {
                    auto const& r = doc->reservierungen[0];
                    check_eq("(3) Claim beansprucht das GANZE Fenster (slice_count)", r.slice_count, std::uint64_t{8});
                    check_true("(3) Reservierung ist Done", r.status == bl::BatchStatus::done);
                    check_true("(3) Kalibrierung eingetragen (eta_s nicht leer)", !r.eta_s.empty());
                }
            }
        }
    }

    // -- Fall (4): OHNE Praedikat -- der byte-identische Ist-Pfad ---------------------------------
    {
        FakeStore         store;
        ex::LazyRunConfig cfg = make_cfg(store, base / "voll");
        ex::BuildConfig   bcfg;
        bcfg.cores_per_build    = 1;
        bcfg.source_dir         = cfg.source_dir;
        bcfg.output_dir         = cfg.output_dir;
        bcfg.per_binary_subdirs = true;
        bcfg.build_parallelism  = 1;
        ex::BuildOrchestrator orch{bcfg, compile_stub, gen_stub};

        ex::BuildStats agg;
        std::size_t    skips  = 42; // muss auf 0 gesetzt werden
        auto const     builds = ex::run_planer_driven_provision(orch, view, alle, cfg, agg, bl::PresenceFn{}, &skips);

        check_eq("(4) ohne Praedikat: volles Fenster gebaut", builds.size(), std::size_t{8});
        check_eq("(4) ohne Praedikat: 0 Bestand-Skips", skips, std::size_t{0});
        check_eq("(4) ohne Praedikat: skipped == 0", agg.skipped, std::size_t{0});
    }

    // -- Fall (5): VOLLER run_lazy_static_then_dynamic -- done-Menge + bestand_lager_skips --------
    {
        FakeStore         store;
        ex::LazyRunConfig cfg = make_cfg(store, base / "voller_lauf");
        // bestandslog_active braucht zusaetzlich den Key-Provider; nullopt = keine Registrierung
        // (hier zaehlt der Kanal, nicht der Bestand-Rueckschrieb).
        cfg.bestand_key_of = [](fs::path const&) -> std::optional<std::string> { return std::nullopt; };
        // Host-Injektion der Praesenz (Vorrang-Naht) -- dieselben 3 Lager-Treffer.
        cfg.bestand_present = drei_im_bestand;
        cfg.max_binaries    = 8;

        std::size_t              konfig_deltas = 0;
        std::size_t              done_cursor   = 0;
        std::size_t              done_count    = 0;
        std::vector<std::size_t> konfig_cursor; // TP1FK1-B5: die gemeldeten Fenster-Positionen, in Reihenfolge
        cfg.progress_sink = [&](ex::ProgressDelta const& d) {
            if (d.done) {
                ++done_count;
                done_cursor = d.cursor;
            } else {
                ++konfig_deltas;
                konfig_cursor.push_back(d.cursor);
            }
        };

        ex::BuildSelection sel;
        sel.indices    = alle;
        sel.provenance = "explicit";

        CerrCapture cerr_fang; // faengt die Testat-Zeilen des Laufs (nicht Teil der Assertions hier)
        auto const  ram_stub = []() -> std::uint64_t { return ~std::uint64_t{0}; }; // RAM nie das Gate
        auto const  r        = ex::run_lazy_static_then_dynamic(tree, sel, compile_stub, gen_stub, ram_stub, cfg);

        check_eq("(5) bestand_lager_skips == 3 (B-3-Feld)", r.bestand_lager_skips, std::size_t{3});
        check_eq("(5) built == 8 (bereitgestellt: gebaut + Bestand)", r.built, std::size_t{8});
        check_eq("(5) built_new == 5 (die Fehlenden)", r.built_new, std::size_t{5});
        check_eq("(5) built_skip == 3 (Summe der Skip-Quellen)", r.built_skip, std::size_t{3});
        check_eq("(5) je gebauter Binary EIN Konfig-Delta", konfig_deltas, std::size_t{5});
        check_eq("(5) genau EIN done-Delta", done_count, std::size_t{1});
        check_eq("(5) done-Cursor == VOLLE bereitgestellte Menge (5+3)", done_cursor, std::size_t{8});

        // -- TP1FK1-B5 (Codex-Befund): der Cursor ist der FENSTER-Index, nicht die builds-Laufvariable.
        //    Lage: die Indizes 0..2 liegen im Lager, gebaut werden 3..7. Der Vertrag von ProgressDelta
        //    (progress_delta.hpp) nennt cursor "fenster-relativer Perm-Index" -- erwartet sind also 3,4,5,6,7.
        //    VOR dem Fix meldete der Kanal 0,1,2,3,4 (Cursor-KOMPRESSION um die Zahl der Lager-Skips) und
        //    danach ein done bei 8: Positionen, die es im Fenster so nie gab, und ein Sprung am Ende.
        //    Die Erwartung ist ABGELEITET (die drei Lager-Treffer stehen in drei_im_bestand), nicht gezaehlt.
        std::vector<std::size_t> erwartete_cursor;
        for (std::size_t i : alle)
            if (!drei_im_bestand(i)) erwartete_cursor.push_back(i); // gebaut => sein Fenster-Index ist i
        check_eq("(5/B5) Zahl der Konfig-Cursor", konfig_cursor.size(), erwartete_cursor.size());
        check_true("(5/B5) Cursor sind die FENSTER-Indizes der gebauten Binaries (keine Kompression)",
                   konfig_cursor == erwartete_cursor);
        check_true("(5/B5) kein Cursor faellt in den Lager-Bereich 0..2",
                   konfig_cursor.empty() || konfig_cursor.front() >= 3);
        check_true("(5/B5) der letzte Konfig-Cursor liegt unter dem done-Cursor (kein Sprung ueber das Fenster)",
                   !konfig_cursor.empty() && konfig_cursor.back() < done_cursor);
    }

    // -- Fall (6): TP1FK1-B10 (Codex-Befund) -- der Bau-Fehler-Zweig INVALIDIERT die per-Binary-Ablage ----
    //    LAGE: eine Binary hat aus einem frueheren Lauf eine VOLLSTAENDIGE, konfigurations-AKTUELLE
    //    result.csv + result.csv.stamp. Heute laesst sie sich NICHT MEHR BAUEN.
    //    VOR dem Fix lief der Resume-Check VOR dem b.ok()-Gate: die Erfolgs-CSV wurde uebernommen, der
    //    A15/FK-1-Marker "nicht_gebaut" kam nie zustande, und weil weder CSV noch Stamp angefasst wurden,
    //    faende der NAECHSTE Lauf denselben Stand erneut -- ein Bau-Fehler konnte beliebig lange hinter
    //    alten Messwerten verschwinden.
    //    Der "alte Stand" wird ueber DIESELBEN Funktionen erzeugt, die der Resume-Check liest
    //    (lazy_csv_header + lazy_resume_stamp_prefix) -- kein handkopierter Stempel-String.
    {
        auto const            dims      = tree.dynamic_filter();
        ex::BinarySpec const  spec0     = view[0];
        std::string const     stem0     = ex::orch_make_stem(spec0.binary_id, spec0.index);
        constexpr char const* kAltMarke = "ALTER-ERFOLGS-STAND";
        // T2-A/K2: `extra` traegt die per-Binary-Felder, die der Lauf an den Config-Praefix haengt
        // (heute "|fpr=<128hex>"). Der Stamp wird weiterhin ueber DIESELBEN Funktionen erzeugt, die der
        // Resume-Check liest -- kein handkopierter Stempel-String.
        auto const lege_altstand = [&](ex::LazyRunConfig const& c, std::string const& extra = {}) {
            fs::path const  bin_dir = c.output_dir / stem0;
            std::error_code lec;
            fs::create_directories(bin_dir, lec);
            { std::ofstream{bin_dir / "result.csv", std::ios::trunc} << ex::lazy_csv_header() << kAltMarke << "\n"; }
            {
                std::ofstream{bin_dir / "result.csv.stamp", std::ios::trunc} << ex::lazy_resume_stamp_prefix(c, dims)
                                                                             << extra << "|rows=1\n";
            }
            return bin_dir;
        };
        auto const datei_text = [](fs::path const& p) {
            std::ifstream     f{p};
            std::stringstream ss;
            ss << f.rdbuf();
            return ss.str();
        };
        auto const mach_cfg = [&](fs::path const& out) {
            FakeStore         dummy;
            ex::LazyRunConfig c         = make_cfg(dummy, out);
            c.provision_only            = false; // MESS-Modus: hier lebt der Resume-Kurzschluss
            c.bestand_transport         = {};    // Bestandslog inaktiv -> reiner Resume-/Bau-Pfad
            c.resume_completed_binaries = true;
            c.max_binaries              = 1;
            return c;
        };
        ex::BuildSelection sel1;
        sel1.indices        = {0};
        sel1.provenance     = "explicit";
        auto const ram_stub = []() -> std::uint64_t { return ~std::uint64_t{0}; };

        // (6a) T2-A/F4 -- DIE ZUSAGE DIESES FALLS IST GEDREHT, UND ZWAR ABSICHTLICH. Bis hierher stand
        //      hier die Gegenprobe "auch eine frisch GEBAUTE Binary resumiert ihren alten Stand"; sie hat
        //      die TP1FK1-B10-Umstellung abgesichert und war unter der damaligen Regel richtig. Genau diese
        //      Regel ist der Codex-Befund K2: eine in DIESEM Lauf real kompilierte .so ist ein ANDERES
        //      Artefakt als das, welches die alten Zeilen erzeugt hat -- alte Messwerte auf sie zu buchen
        //      ist der "neue DLL / alte Messwerte"-Bug. Der Resume-Anspruch haengt seither an b.skipped.
        //      LAGE: kein Fingerprint-Provider => dll_is_current ist fail-closed false => die Binary wird
        //      gebaut => b.skipped ist falsch => KEIN Resume.
        {
            ex::LazyRunConfig cfg6a   = mach_cfg(base / "b10_gruen");
            fs::path const    bin_dir = lege_altstand(cfg6a);
            CerrCapture       fang;
            auto const        r = ex::run_lazy_static_then_dynamic(tree, sel1, compile_stub, gen_stub, ram_stub, cfg6a);
            check_eq("(6a) frisch GEBAUTE Binary: KEIN Resume (K2/F4)", r.resumed_binaries, std::size_t{0});
            check_true("(6a) die alten Zeilen reisen NICHT in die globale CSV",
                       r.resumed_csv_rows.find(kAltMarke) == std::string::npos);
            check_true("(6a) result.csv.stamp bleibt stehen", fs::exists(bin_dir / "result.csv.stamp", ec));
            check_true("(6a) der Alt-Stand bleibt liegen (ent-wertet wird NUR im Bau-Fehler-Zweig)",
                       datei_text(bin_dir / "result.csv").find(kAltMarke) != std::string::npos);
            check_true("(6a) kein result.csv.stale -- ent-wertet wird NUR im Bau-Fehler-Zweig",
                       !fs::exists(bin_dir / "result.csv.stale", ec));
        }

        // (6a-2) DIE GEGENPROBE ZUR DREHUNG: der Resume ist nicht abgeschafft, er haengt an der richtigen
        //        Bedingung. LAGE: perm.dll existiert, ihr .fingerprint-Sidecar deckt sich mit der Erwartung
        //        des Providers => dll_is_current true => b.skipped => der alte Stand gilt weiter. Der Stamp
        //        traegt dabei das neue "|fpr="-Feld -- geschrieben ueber dieselbe Quelle, die der Lauf liest.
        std::string const kFpAlt     = std::string(128, 'a');
        std::string const kFpNeu     = std::string(128, 'b');
        ex::LazyRunConfig cfg6n      = mach_cfg(base / "b10_skip");
        cfg6n.bestand_fingerprint_fn = [kFpAlt](std::string const&) { return kFpAlt; };
        fs::path const bin_dir_skip  = lege_altstand(cfg6n, "|fpr=" + kFpAlt);
        {
            { std::ofstream{bin_dir_skip / "perm.dll", std::ios::trunc} << "nicht-ladbar-aber-vorhanden\n"; }
            { std::ofstream{bin_dir_skip / "perm.dll.fingerprint", std::ios::trunc} << kFpAlt; }
            CerrCapture fang;
            auto const  r = ex::run_lazy_static_then_dynamic(tree, sel1, compile_stub, gen_stub, ram_stub, cfg6n);
            check_eq("(6a-2) versions-AKTUELLE Binary (b.skipped): resumed_binaries == 1", r.resumed_binaries,
                     std::size_t{1});
            check_true("(6a-2) die alten Zeilen reisen unveraendert weiter",
                       r.resumed_csv_rows.find(kAltMarke) != std::string::npos);
        }

        // (6d) K2 AM OBJEKT -- DER "NEUE DLL / ALTE MESSWERTE"-FALL. Dieselbe Ablage, dieselbe Config, nur
        //      die Toolchain hat sich geaendert: der Provider liefert einen ANDEREN Fingerprint (das ist
        //      genau das Szenario g++-16 16.0.1 -> 16.3 aus dem Befund). Unter resume-v5 kannte der Stamp
        //      nur die algo_sig -- die Organ-Achsen sind unveraendert, also passte er weiter, und die alten
        //      Messwerte wurden auf die neu gebaute DLL gebucht. Ab resume-v6 trennt der Stamp die beiden
        //      Staende SELBST, unabhaengig von der b.skipped-Wache.
        {
            ex::LazyRunConfig cfg6d      = cfg6n;
            cfg6d.bestand_fingerprint_fn = [kFpNeu](std::string const&) { return kFpNeu; };
            std::string const stamp_alt  = ex::lazy_resume_stamp_prefix(cfg6n, dims) + "|fpr=" + kFpAlt;
            std::string const stamp_neu  = ex::lazy_resume_stamp_prefix(cfg6d, dims) + "|fpr=" + kFpNeu;
            check_true("(6d) der Stamp-Vergleich ALLEIN trennt alten und neuen Fingerprint",
                       ex::lazy_try_resume_binary(bin_dir_skip, stamp_alt, nullptr) &&
                           !ex::lazy_try_resume_binary(bin_dir_skip, stamp_neu, nullptr));
            check_true("(6d) die Config-Praefixe sind identisch -- es trennt WIRKLICH nur der Fingerprint",
                       ex::lazy_resume_stamp_prefix(cfg6n, dims) == ex::lazy_resume_stamp_prefix(cfg6d, dims));
            CerrCapture fang;
            auto const  r = ex::run_lazy_static_then_dynamic(tree, sel1, compile_stub, gen_stub, ram_stub, cfg6d);
            check_eq("(6d) neuer Fingerprint -> Neubau -> KEIN Resume", r.resumed_binaries, std::size_t{0});
            check_true("(6d) die alten Messwerte werden NICHT auf die neue DLL gebucht",
                       r.resumed_csv_rows.find(kAltMarke) == std::string::npos);
        }

        // (6e) T2-A/K2-NB (Codex-Auflage "fehlende Tests"): DIE v5-INVALIDIERUNG AM LITERALEN ON-DISK-STAMP.
        //      Der Praefix-Bump v5->v6 war bewiesen, aber UNGETESTET -- bewiesen nur ueber die Konstruktion
        //      "der Praefix beginnt mit resume-v6". Hier liegt ein Stamp auf der Platte, wie ihn ein Lauf
        //      VOR dem Bump geschrieben haette: dieselbe Konfiguration, dieselbe Zeilenzahl, nur die
        //      Format-Marke ist die alte. Er darf NICHT gelten -- sonst uebernaehme ein v6-Lauf Messwerte,
        //      die unter der schwaecheren v5-Zusage entstanden sind (ohne Fingerprint-Kopplung).
        {
            ex::LazyRunConfig cfg6e   = mach_cfg(base / "k2_v5");
            fs::path const    bin_dir = lege_altstand(cfg6e); // legt einen v6-Stamp an
            std::string const v6      = ex::lazy_resume_stamp_prefix(cfg6e, dims);
            check_true("(6e) Vorbedingung: der heutige Praefix ist resume-v6", v6.rfind("resume-v6|", 0) == 0);
            // Derselbe Stamp, EIN Zeichen anders: die Format-Marke der Vorgaenger-Generation.
            std::string v5 = v6;
            v5.replace(0, std::string{"resume-v6"}.size(), "resume-v5");
            { std::ofstream{bin_dir / "result.csv.stamp", std::ios::trunc} << v5 << "|rows=1\n"; }
            check_true("(6e) der LITERALE v5-Stamp auf der Platte gilt fuer einen v6-Lauf NICHT",
                       !ex::lazy_try_resume_binary(bin_dir, v6, nullptr));
            check_true("(6e) und der v5-Lauf selbst haette ihn akzeptiert (die Wache trennt, sie sperrt nicht)",
                       ex::lazy_try_resume_binary(bin_dir, v5, nullptr));
        }

        // (6f) T2-A/K2-NB (Codex-Haertung (b)): DIE FORM DES "|fpr="-FELDES WIRD GEPRUEFT.
        //      DER BEFUND: der Provider-Wert reiste ROH in eine Ein-Zeilen-Datei. Enthaelt er ein '\n',
        //      zerfaellt die Datei; der Leser sieht nur die ERSTE Zeile -- und die kann fuer einen
        //      KUERZEREN Fingerprint ein vollstaendiger, gueltiger Stamp sein. Genau das wird hier am
        //      Objekt vorgefuehrt (rot), und danach die Wache, die es beendet (gruen).
        {
            std::string const kFpKurz = std::string(128, 'd');
            // ROT AM OBJEKT: ein Provider-Wert, der den kurzen Fingerprint + den Schwanz enthaelt und
            // danach umbricht. Ein Lauf mit dem KURZEN Wert liest Zeile 1 -- und sie passt.
            std::string const boeser_wert = kFpKurz + "|rows=1\nweiterer-muell";
            ex::LazyRunConfig cfg6f       = mach_cfg(base / "k2_form");
            fs::path const    bin_dir     = lege_altstand(cfg6f, "|fpr=" + kFpKurz);
            { std::ofstream{bin_dir / "result.csv.stamp", std::ios::trunc}
                  << ex::lazy_resume_stamp_prefix(cfg6f, dims) << "|fpr=" << boeser_wert << "|rows=99\n"; }
            std::string const stamp_kurz = ex::lazy_resume_stamp_prefix(cfg6f, dims) + "|fpr=" + kFpKurz;
            check_true("(6f) ROT: die aufgetrennte Ablage passt als Stamp fuer den KUERZEREN Fingerprint",
                       ex::lazy_try_resume_binary(bin_dir, stamp_kurz, nullptr));
            // GRUEN: ein Provider, der so etwas liefert, kommt gar nicht mehr bis zur Ablage. Der Lauf
            // deaktiviert Resume UND Stamp fuer diese Binary und sagt es literal.
            cfg6f.bestand_fingerprint_fn = [boeser_wert](std::string const&) { return boeser_wert; };
            { std::ofstream{bin_dir / "perm.dll", std::ios::trunc} << "nicht-ladbar-aber-vorhanden\n"; }
            { std::ofstream{bin_dir / "perm.dll.fingerprint", std::ios::trunc} << boeser_wert; }
            std::string log6f;
            ex::LazyRunResult r6f;
            {
                CerrCapture fang;
                r6f   = ex::run_lazy_static_then_dynamic(tree, sel1, compile_stub, gen_stub, ram_stub, cfg6f);
                log6f = fang.text();
            }
            check_eq("(6f) GRUEN: kein Resume auf einem form-verletzenden Fingerprint", r6f.resumed_binaries,
                     std::size_t{0});
            check_true("(6f) GRUEN: der Verstoss steht literal im Log",
                       log6f.find("Fingerprint ist nicht 128-hex") != std::string::npos);
            check_true("(6f) GRUEN: es bleibt KEIN Stamp liegen, der einen fremden Stand zertifizieren koennte",
                       !fs::exists(bin_dir / "result.csv.stamp", ec));
        }

        // (6g) F8 -- DIE DOKTRIN WIRD FESTGESCHRIEBEN, NICHT VERBOTEN (Ledger 06.08. mittag-12, OFFENE
        //      OWNER-FRAGE). Codex nannte das Cross-Run-Szenario "SCHWER": DLL weg -> Neubau mit
        //      UNVERAENDERTEM Fingerprint -> ein SPAETERER Lauf resumiert die alten Zeilen. Das ist KEIN
        //      Leck, sondern der ZWECK des Fingerprints: er IST die Bau-Identitaet. Ist er unveraendert,
        //      ist die neu gebaute DLL aequivalent, und die alten Messwerte gelten weiter.
        //      Dieser Test HAELT diese Erwartung fest -- schlaegt er eines Tages um, ist das ein
        //      DOKTRIN-Wechsel und muss als solcher entschieden werden (und nicht als Bugfix passieren).
        {
            std::string const kFp8   = std::string(128, 'e');
            ex::LazyRunConfig cfg6g  = mach_cfg(base / "f8_doktrin");
            cfg6g.bestand_fingerprint_fn = [kFp8](std::string const&) { return kFp8; };
            fs::path const bin_dir       = lege_altstand(cfg6g, "|fpr=" + kFp8);
            // Lauf 1: die DLL FEHLT (Ordner geraeumt) -> Neubau -> b.skipped falsch -> KEIN Resume.
            ex::LazyRunResult r1;
            {
                CerrCapture fang;
                r1 = ex::run_lazy_static_then_dynamic(tree, sel1, compile_stub, gen_stub, ram_stub, cfg6g);
            }
            check_eq("(6g) Neubau im selben Lauf: KEIN Resume (b.skipped falsch -- korrekt)", r1.resumed_binaries,
                     std::size_t{0});
            // Lauf 2: die DLL liegt jetzt da, ihr Sidecar traegt DENSELBEN Fingerprint -> b.skipped ->
            // Resume der alten Zeilen. GEWOLLT: gleicher Fingerprint == gleiche Bau-Identitaet.
            { std::ofstream{bin_dir / "perm.dll", std::ios::trunc} << "nicht-ladbar-aber-vorhanden\n"; }
            { std::ofstream{bin_dir / "perm.dll.fingerprint", std::ios::trunc} << kFp8; }
            (void)lege_altstand(cfg6g, "|fpr=" + kFp8); // der Stand, den Lauf 1 nicht angetastet hat
            ex::LazyRunResult r2;
            {
                CerrCapture fang;
                r2 = ex::run_lazy_static_then_dynamic(tree, sel1, compile_stub, gen_stub, ram_stub, cfg6g);
            }
            check_eq("(6g) F8-DOKTRIN: gleicher Fingerprint -> der spaetere Lauf resumiert (Fingerprint == Identitaet)",
                     r2.resumed_binaries, std::size_t{1});
            check_true("(6g) F8-DOKTRIN: und uebernimmt genau die alten Zeilen",
                       r2.resumed_csv_rows.find(kAltMarke) != std::string::npos);
        }

        // (6h) T2-A/K2-NB (Codex-Haertung (d)): DER PROVIDER WIRD EINMAL GELESEN, NICHT ZWEIMAL.
        //      DER BEFUND: das DLL-Gate (provision_core) und der Mess-Resume-Stamp riefen
        //      cfg.bestand_fingerprint_fn GETRENNT auf. Dieselbe std::function garantiert keinen
        //      identischen Rueckgabewert -- ein ZUSTANDSABHAENGIGER Provider prueft dann mit X und
        //      stempelt mit Y. Der Stamp bezeugt eine Identitaet, die das Gate nie gesehen hat.
        //      LAGE: ein Provider, der beim ERSTEN Aufruf kFpA liefert und danach kFpB. Die Ablage
        //      (Sidecar + Stamp) traegt kFpA.
        //      ALT (zwei Lesepunkte): Gate liest kFpA -> b.skipped; der Stamp entsteht mit kFpB -> er
        //      passt NICHT auf die Ablage -> kein Resume, und der Lauf haette eine Marke mit kFpB
        //      geschrieben, waehrend das Sidecar der DLL kFpA sagt.
        //      NEU (ein Lesepunkt): b.fingerprint traegt kFpA in beide Konsumenten -> Resume greift,
        //      und der Zaehler steht literal auf 1 Aufruf.
        {
            std::string const kFpA = std::string(128, '1');
            std::string const kFpB = std::string(128, '2');
            auto const        rufe = std::make_shared<int>(0);
            ex::LazyRunConfig cfg6h = mach_cfg(base / "k2_einmal");
            cfg6h.bestand_fingerprint_fn = [rufe, kFpA, kFpB](std::string const&) {
                return ((*rufe)++ == 0) ? kFpA : kFpB; // zustandsabhaengig -- genau der Fehlerfall
            };
            fs::path const bin_dir = lege_altstand(cfg6h, "|fpr=" + kFpA);
            { std::ofstream{bin_dir / "perm.dll", std::ios::trunc} << "nicht-ladbar-aber-vorhanden\n"; }
            { std::ofstream{bin_dir / "perm.dll.fingerprint", std::ios::trunc} << kFpA; }
            ex::LazyRunResult r6h;
            {
                CerrCapture fang;
                r6h = ex::run_lazy_static_then_dynamic(tree, sel1, compile_stub, gen_stub, ram_stub, cfg6h);
            }
            check_eq("(6h) der Fingerprint-Provider wird GENAU EINMAL je Binary befragt", *rufe, 1);
            check_eq("(6h) und Gate wie Stamp speisen sich aus demselben Wert -> Resume greift",
                     r6h.resumed_binaries, std::size_t{1});
            check_true("(6h) die alten Zeilen reisen unveraendert weiter",
                       r6h.resumed_csv_rows.find(kAltMarke) != std::string::npos);
        }

        // (6b) DER BEFUND: derselbe alte Stand, aber der Compile scheitert -> KEIN Resume, sondern der
        //      nicht_gebaut-Marker TRITT AN DIE STELLE der stale Erfolgs-CSV, und der Stamp faellt (kein
        //      Resume-Anspruch im Folgelauf mehr).
        //      F-B10 Owner-Default (b) (02.08.), Doktrin "Messdaten nie loeschen": der alte Stand ist
        //      dabei NICHT weg, sondern liegt ent-wertet als result.csv.stale -- geprueft wird also
        //      BEIDES: er ist aus der gelesenen result.csv verschwunden UND in der .stale erhalten.
        {
            ex::LazyRunConfig cfg6b        = mach_cfg(base / "b10_fehler");
            fs::path const    bin_dir      = lege_altstand(cfg6b);
            auto const        compile_fail = [](ex::BuildJob const&) -> int { return 1; }; // Compiler lehnt ab (D1)
            CerrCapture       fang;
            auto const        r = ex::run_lazy_static_then_dynamic(tree, sel1, compile_fail, gen_stub, ram_stub, cfg6b);
            std::string const log = fang.text();

            check_eq("(6b) KEIN Resume auf einer nicht baubaren Binary", r.resumed_binaries, std::size_t{0});
            check_true("(6b) die stale Erfolgs-CSV fliesst NICHT in die globale CSV",
                       r.resumed_csv_rows.find(kAltMarke) == std::string::npos);
            check_eq("(6b) genau EINE Marker-Zeile im Ergebnis", r.csv_rows.size(), std::size_t{1});
            check_true("(6b) und sie traegt den Bau-Status nicht_gebaut",
                       r.csv_rows.size() == 1 &&
                           r.csv_rows[0].build_status ==
                               ::comdare::cache_engine::measurement::BuildCellStatus::NichtGebaut);
            // Der Kern des Befunds: die ABLAGE, die der naechste Lauf liest.
            check_true("(6b) result.csv.stamp ist WEG (kein Resume-Anspruch mehr)",
                       !fs::exists(bin_dir / "result.csv.stamp", ec));
            std::string const csv_neu = datei_text(bin_dir / "result.csv");
            check_true("(6b) die per-Binary-CSV traegt jetzt die nicht_gebaut-Marker-Zeile",
                       csv_neu.find("nicht_gebaut") != std::string::npos);
            check_true("(6b) der alte Erfolgs-Stand ist aus der resume-gelesenen result.csv geraeumt",
                       csv_neu.find(kAltMarke) == std::string::npos);
            check_true("(6b) er ist aber ERHALTEN: result.csv.stale existiert (Messdaten nie loeschen)",
                       fs::exists(bin_dir / "result.csv.stale", ec));
            check_true("(6b) und traegt genau den alten Erfolgs-Stand",
                       datei_text(bin_dir / "result.csv.stale").find(kAltMarke) != std::string::npos);
            check_true("(6b) der Bau-Fehler ist klassifiziert geloggt (Nie-stumm)",
                       log.find("Compiler-Compiler-Fehler") != std::string::npos);

            // (6c) FOLGELAUF-BEWEIS: derselbe Zustand ein zweites Mal -- der Resume-Check darf die
            //      Marker-Datei NICHT als Erfolgs-Stand annehmen (ohne Stamp gibt es keinen Anspruch).
            auto const r2 = ex::run_lazy_static_then_dynamic(tree, sel1, compile_fail, gen_stub, ram_stub, cfg6b);
            check_eq("(6c) Folgelauf resumiert ebenfalls NICHT", r2.resumed_binaries, std::size_t{0});
            check_eq("(6c) Folgelauf meldet wieder genau die Marker-Zeile", r2.csv_rows.size(), std::size_t{1});
        }
    }

    // -- Fall (6m): E-04-P1 FALLBACK-KANAL -- provision_only OHNE aktives Bestandslog ---------------
    //    Ohne Transport/Doc-Key ist planer_driven_active falsch: es gibt keinen Slice-Loop und keine
    //    Done-Records. Genau dann darf der Live-Kanal NICHT stumm sein (Nie-stumm-Doktrin) -- die
    //    Invocation IST das eine Fenster, und die Bilanz zieht ihre Zahlen aus built_new/built_skip.
    {
        FakeStore         store;
        ex::LazyRunConfig cfg = make_cfg(store, base / "fallback_kanal");
        cfg.bestand_transport = {}; // Storage INERT -> kein Bestandslog -> kein planer-getriebener Bau
        cfg.bestand_doc_key.clear();
        cfg.max_binaries = 8;

        ex::BuildSelection sel;
        sel.indices    = alle;
        sel.provenance = "explicit";

        CerrCapture cerr_fang;
        auto const  ram_stub = []() -> std::uint64_t { return ~std::uint64_t{0}; };
        auto const  r        = ex::run_lazy_static_then_dynamic(tree, sel, compile_stub, gen_stub, ram_stub, cfg);

        std::string const spur = cerr_fang.text();
        check_eq("(6m) Fallback: genau EIN [PLAN-TESTAT]", zaehle(spur, "[PLAN-TESTAT] "), std::size_t{1});
        check_eq("(6m) Fallback: genau EIN [BILANZ-TESTAT]", zaehle(spur, "[BILANZ-TESTAT] "), std::size_t{1});
        check_eq("(6m) Fallback: Pflichtfeld lane= in beiden Zeilen", zaehle(spur, " lane=amd "), std::size_t{2});
        check_eq("(6m) Fallback: Pflichtfeld fenster= in beiden Zeilen", zaehle(spur, " fenster=0:8 "), std::size_t{2});
        check_eq("(6m) Fallback: KEINE Reservierung geschrieben", store.objs.size(), std::size_t{0});
        // Die Bilanz-Zahlen sind exakt die LazyRunResult-Felder -> built_new/built_skip haben einen Leser.
        std::string const erwartet = " gebaut_neu=" + std::to_string(r.built_new) +
                                     " sidecar_skip=" + std::to_string(r.built_skip) +
                                     " lager_skip=0 plan_skip=0 fehl=0";
        check_true("(6m) Fallback-Bilanz konsumiert built_new/built_skip", spur.find(erwartet) != std::string::npos);
        check_eq("(6m) Fallback: 8 frisch gebaut (kein Bestand, kein Sidecar)", r.built_new, std::size_t{8});
    }

    // -- Fall (7): TP1FK1-B1 (Codex-Befund CX-W2) -- die Slice-Identitaet wird als SPANNE gespeichert --
    //    LAGE: eine LUECKENHAFTE Fenster-Menge {0,2}. Die frueher geschriebene (front(), size())-Form legte
    //    (0,2) ab, also das Intervall {0,1}: eine FREMDE Selektion {0,1} bestand damit die Deckungspruefung
    //    und enteignete den Claim -- Index 2 baute danach niemand (die B1-Fehlermode). slice_window_bounds
    //    legt die SPANNE (0,3) ab, die die reale Menge UMFASST; freigegeben wird erst, wenn die
    //    uebernehmende Selektion das ganze Intervall baut. Konservativ, ohne jede Draht-Aenderung.
    {
        // (7a) die Bounds-Funktion selbst: Spanne statt Kardinalitaet -- und unveraendert bei Lueckenfreiheit.
        check_eq("(7a) gappy {0,2}: begin == min == 0", bl::slice_window_bounds({0, 2}).begin, std::uint64_t{0});
        check_eq("(7a) gappy {0,2}: count == SPANNE 3 (nicht size 2)", bl::slice_window_bounds({0, 2}).count,
                 std::uint64_t{3});
        check_eq("(7a) zusammenhaengend {0,1,2,3}: count == 4 (== size, unveraendert)",
                 bl::slice_window_bounds({0, 1, 2, 3}).count, std::uint64_t{4});
        check_eq("(7a) leeres Fenster: count 0 (nichts zu beanspruchen)", bl::slice_window_bounds({}).count,
                 std::uint64_t{0});

        // (7b) die WIRKUNG am Sweep -- die eigentliche Negativ-Probe: niemand wird faelschlich enteignet.
        std::vector<std::size_t> const teil{0, 1};
        std::vector<std::size_t> const voll{0, 1, 2};
        auto const                     bounds = bl::slice_window_bounds({0, 2});
        check_true("(7b) fremde Selektion {0,1} released das gappy Fenster {0,2} NICHT",
                   !bl::scope_covers_slice(bl::make_sweep_scope(bl::BatchTyp::tier, teil), bounds.begin, bounds.count));
        check_true("(7b) wer {0,1,2} baut, deckt {0,2} voll ab -> Uebernahme erlaubt",
                   bl::scope_covers_slice(bl::make_sweep_scope(bl::BatchTyp::tier, voll), bounds.begin, bounds.count));
        // Der VERLUST literal: mit der alten (begin=0, count=size=2)-Form haette {0,1} gedeckt.
        check_true("(7b) Beleg des Schreib-Verlusts: (0, size=2) HAETTE {0,1} faelschlich freigegeben",
                   bl::scope_covers_slice(bl::make_sweep_scope(bl::BatchTyp::tier, teil), 0, 2));

        // (7c) END-TO-END am REALEN Schreibweg: run_planer_driven_provision mit gappy {0,2} legt die
        //      Reservierung mit slice_count == SPANNE 3 ab (VOR dem Fix stand dort die 2).
        FakeStore         store;
        ex::LazyRunConfig cfg = make_cfg(store, base / "cxw2_gappy");
        ex::BuildConfig   bcfg;
        bcfg.cores_per_build    = 1;
        bcfg.source_dir         = cfg.source_dir;
        bcfg.output_dir         = cfg.output_dir;
        bcfg.per_binary_subdirs = true;
        bcfg.build_parallelism  = 1;
        ex::BuildOrchestrator          orch{bcfg, compile_stub, gen_stub};
        ex::BuildStats                 agg;
        std::size_t                    skips = 0;
        CerrCapture                    fang;
        std::vector<std::size_t> const gappy{0, 2};
        auto const builds      = ex::run_planer_driven_provision(orch, view, gappy, cfg, agg, bl::PresenceFn{}, &skips);
        std::string const spur = fang.text();
        check_eq("(7c) beide Indizes des gappy Fensters gebaut", builds.size(), std::size_t{2});
        check_true("(7c) die konservative Weitung ist NICHT stumm",
                   spur.find("Slice-Fenster ist NICHT zusammenhaengend") != std::string::npos);
        auto const raw = store.objs.find(kDocKey);
        check_true("(7c) Reservierung im Store", raw != store.objs.end());
        if (raw != store.objs.end()) {
            auto const doc = bl::parse_bestandslog(raw->second);
            check_true("(7c) Dokument parsebar", doc.has_value());
            check_true("(7c) genau EINE Slice-Reservierung", doc && doc->reservierungen.size() == 1);
            if (doc && doc->reservierungen.size() == 1) {
                check_eq("(7c) slice_begin == min == 0", doc->reservierungen[0].slice_begin, std::uint64_t{0});
                check_eq("(7c) slice_count == SPANNE 3 (nicht size 2) -- kein Schreib-Verlust",
                         doc->reservierungen[0].slice_count, std::uint64_t{3});
            }
        }
    }

    // -- Fall (8): TP1FK1-B2 (Codex-Befund CX-W1) -- der SYNCHRONE Mess-Pfad-Faenger schliesst den ------
    //    Bestandslog-Eintrag aus, wenn der Push warf. Der Eintrag ist (wie in der Bau-Phase) per observe()
    //    vorgemerkt; wirft der synchrone Push, hat der Store den Satz NIE erhalten -- nach dem unbedingten
    //    flush am Lauf-Ende darf er deshalb NICHT im geteilten Dokument stehen (sonst skippt der Bau-Filter
    //    des Folgelaufs eine nirgends existierende Binary). VOR dem Fix loggte der Faenger nur.
    //    Im SELBEN Dokument sitzt eine zweite Binary, deren Push GELANG: sie muss registriert werden --
    //    der Ausschluss ist gezielt, keine Pauschal-Enteignung.
    {
        auto const                hexkey = [](char c) { return std::string(128, c); };
        bl::ZellKoordinaten const zelle{.combo = "default", .opt = "O2", .simd = "avx2"};
        fs::path const            out = base / "cxw1" / "dll";
        fs::create_directories(out / "sX", ec);
        fs::create_directories(out / "sY", ec);

        bl::LagerRunState lager;
        check_true("(8) observe merkt den Eintrag der werfenden Binary vor",
                   lager.observe(hexkey('a'), zelle, "sX/perm.dll", 10, "[a,b,c]", "2026-08-03T10:00:00Z") ==
                       bl::DedupOutcome::fresh_register);
        check_true("(8) observe merkt den Eintrag der gelingenden Binary vor",
                   lager.observe(hexkey('b'), zelle, "sY/perm.dll", 20, "[a,b,c]", "2026-08-03T10:00:01Z") ==
                       bl::DedupOutcome::fresh_register);

        ex::CachePushFn const werfen = [](fs::path const&, std::string const&) {
            // cppcheck-suppress throwInEntryPoint // FP: Negativprobe (8) -- der Wurf IST der Testfall, mess_pfad_synchron_push faengt ihn
            throw std::runtime_error("Store-Push simuliert fehlgeschlagen");
        };
        bool                  gepusht = false;
        ex::CachePushFn const geht    = [&gepusht](fs::path const&, std::string const&) { gepusht = true; };

        std::string log;
        {
            CerrCapture fang;
            ex::mess_pfad_synchron_push(werfen, out / "sX", "v1.0.0c", "binid-X", out, &lager);
            ex::mess_pfad_synchron_push(geht, out / "sY", "v1.0.0c", "binid-Y", out, &lager);
            log = fang.text();
        }
        check_true("(8) der gelungene Push lief wirklich", gepusht);
        check_true("(8) der Wurf ist klassifiziert geloggt und der Lauf MISST WEITER",
                   log.find("MISST WEITER") != std::string::npos);
        check_true("(8) der Ausschluss ist beziffert (Nie-stumm)", log.find("NICHT registriert") != std::string::npos);
        check_eq("(8) genau EIN Eintrag bleibt vorgemerkt", lager.pending_fresh(), std::size_t{1});

        FakeStore  store;
        auto const reg =
            lager.flush(store.transport(), kDocKey, "2026-08-03T10:05:00Z", bl::make_lock_owner("uuid-cxw1", "prodX"));
        check_true("(8) der flush hat geschrieben", reg.has_value() && *reg == 1);
        auto const doc = bl::parse_bestandslog(store.objs[kDocKey]);
        check_true("(8) Dokument parsebar", doc.has_value());
        if (doc) {
            bool const drin_x = std::any_of(doc->bestand.begin(), doc->bestand.end(),
                                            [](bl::BestandEintrag const& e) { return e.pfad == "sX/perm.dll"; });
            bool const drin_y = std::any_of(doc->bestand.begin(), doc->bestand.end(),
                                            [](bl::BestandEintrag const& e) { return e.pfad == "sY/perm.dll"; });
            check_true("(8) CX-W1: die Binary mit geworfenem Push steht NICHT im geteilten Dokument", !drin_x);
            check_true("(8) die Binary mit gelungenem Push steht sehr wohl drin (kein Fehl-Ausschluss)", drin_y);
        }
    }

    // -- Fall (9): TP1FK1-B10 (Codex-Befund CX-W4) -- ein fehlgeschlagenes Entfernen von result.csv.stamp
    //    wird nicht mehr verschluckt. LAGE: result.csv.stamp ist ein NICHT-LEERES VERZEICHNIS, damit
    //    std::filesystem::remove deterministisch scheitert (ENOTEMPTY -- auch als root, kein chmod noetig).
    //    VOR dem Fix lief remove() ohne jede Pruefung: der Alt-Stamp blieb liegen, die Marker-Zeile wurde
    //    trotzdem als neue result.csv geschrieben (der Alt-Stamp haette sie im Folgelauf als gueltigen
    //    Messstand ZERTIFIZIERT) und die Diagnose behauptete "der Stamp ist entfernt".
    {
        constexpr char const* kAltMarke = "ALTER-ERFOLGS-STAND";
        ex::BinarySpec const  spec0     = view[0];
        std::string const     stem0     = ex::orch_make_stem(spec0.binary_id, spec0.index);

        FakeStore         dummy;
        ex::LazyRunConfig cfg9 = make_cfg(dummy, base / "cxw4");
        cfg9.provision_only    = false; // MESS-Modus: dort lebt der Bau-Fehler-Zweig mit der Invalidierung
        cfg9.bestand_transport = {};    // Bestandslog inaktiv -> reiner Bau-/Resume-Pfad
        cfg9.bestand_doc_key.clear();
        cfg9.resume_completed_binaries = true;
        cfg9.max_binaries              = 1;

        fs::path const bin_dir = cfg9.output_dir / stem0;
        fs::create_directories(bin_dir, ec);
        { std::ofstream{bin_dir / "result.csv", std::ios::trunc} << ex::lazy_csv_header() << kAltMarke << "\n"; }
        fs::create_directories(bin_dir / "result.csv.stamp", ec); // Stamp ALS Verzeichnis ...
        { std::ofstream{bin_dir / "result.csv.stamp" / "blocker", std::ios::trunc} << "x\n"; } // ... nicht leer

        ex::BuildSelection sel9;
        sel9.indices            = {0};
        sel9.provenance         = "explicit";
        auto const compile_fail = [](ex::BuildJob const&) -> int { return 1; }; // Compiler lehnt ab -> Fehler-Zweig
        auto const ram_stub     = []() -> std::uint64_t { return ~std::uint64_t{0}; };

        std::string       log;
        ex::LazyRunResult r9;
        {
            CerrCapture fang;
            r9  = ex::run_lazy_static_then_dynamic(tree, sel9, compile_fail, gen_stub, ram_stub, cfg9);
            log = fang.text();
        }
        auto const datei_text = [](fs::path const& p) {
            std::ifstream     f{p};
            std::stringstream ss;
            ss << f.rdbuf();
            return ss.str();
        };
        check_true("(9) CX-W4: der Entfern-Fehler ist klassifiziert sichtbar",
                   log.find("result.csv.stamp NICHT entfernt") != std::string::npos);
        check_true("(9) keine unbelegte 'der Stamp ist entfernt'-Aussage",
                   log.find("der Stamp ist entfernt") == std::string::npos);
        check_true("(9) der blockierte Alt-Stamp liegt noch (remove scheiterte wirklich)",
                   fs::exists(bin_dir / "result.csv.stamp", ec));
        // Der KERN: mit liegendem Alt-Stamp darf keine Marker-Zeile als Messstand zertifiziert werden.
        std::string const csv = datei_text(bin_dir / "result.csv");
        check_true("(9) die Marker-Zeile wurde NICHT unter den Alt-Stamp geschrieben",
                   csv.find("nicht_gebaut") == std::string::npos);
        check_true("(9) der Alt-Stand blieb unangetastet (Messdaten nie loeschen)",
                   csv.find(kAltMarke) != std::string::npos);
        check_true("(9) kein result.csv.stale -- die Ablage wurde gar nicht erst angefasst",
                   !fs::exists(bin_dir / "result.csv.stale", ec));
        // Sichtbar bleibt der Befund trotzdem: die globale CSV traegt die nicht_gebaut-Zeile.
        check_eq("(9) der Bau-Fehler reist als Marker-Zeile in die globale CSV", r9.csv_rows.size(), std::size_t{1});
        check_true("(9) und traegt den Bau-Status nicht_gebaut",
                   r9.csv_rows.size() == 1 && r9.csv_rows[0].build_status ==
                                                  ::comdare::cache_engine::measurement::BuildCellStatus::NichtGebaut);
    }

    // -- Fall (10): Review-Befund Z-01/GA-02 -- ein fehlgeschlagenes SICHERN der Alt-result.csv nach
    //    result.csv.stale darf die Alt-Mess-CSV nicht dem trunc des Marker-Writes ausliefern.
    //    LAGE: result.csv.stale ist ein NICHT-LEERES VERZEICHNIS, damit fs::rename deterministisch
    //    scheitert (ENOTEMPTY/EEXIST -- auch als root, kein chmod/ro-Mount noetig). Der Stamp ist eine
    //    normale Datei und faellt sauber: der Bau-Fehler-Zweig erreicht also GENAU den else-Zweig mit
    //    dem rename (Fall (9) prueft den anderen, davorliegenden Zweig).
    //    VOR dem Fix lief das rename mit einem error_code, den danach niemand mehr ansah, und der
    //    unmittelbar folgende std::ofstream{result.csv, trunc} LOESCHTE den Alt-Stand, den es gerade
    //    nicht sichern konnte -- "Messdaten nie loeschen", gebrochen vom Sicherungs-Versuch selbst.
    {
        constexpr char const* kAltMarke = "ALTER-ERFOLGS-STAND-Z01";
        auto const            dims      = tree.dynamic_filter();
        ex::BinarySpec const  spec0     = view[0];
        std::string const     stem0     = ex::orch_make_stem(spec0.binary_id, spec0.index);

        FakeStore         dummy;
        ex::LazyRunConfig cfg10 = make_cfg(dummy, base / "z01");
        cfg10.provision_only    = false; // MESS-Modus: dort lebt der Bau-Fehler-Zweig mit der Sicherung
        cfg10.bestand_transport = {};    // Bestandslog inaktiv -> reiner Bau-/Resume-Pfad
        cfg10.bestand_doc_key.clear();
        cfg10.resume_completed_binaries = true;
        cfg10.max_binaries              = 1;

        fs::path const bin_dir = cfg10.output_dir / stem0;
        fs::create_directories(bin_dir, ec);
        { std::ofstream{bin_dir / "result.csv", std::ios::trunc} << ex::lazy_csv_header() << kAltMarke << "\n"; }
        { // Stamp als normale Datei -> sein Entfernen GELINGT, der Lauf erreicht das rename
            std::ofstream{bin_dir / "result.csv.stamp", std::ios::trunc} << ex::lazy_resume_stamp_prefix(cfg10, dims)
                                                                         << "|rows=1\n";
        }
        fs::create_directories(bin_dir / "result.csv.stale" / "belegt", ec); // .stale ALS Verzeichnis ...
        {
            std::ofstream{bin_dir / "result.csv.stale" / "belegt" / "blocker", std::ios::trunc} << "x\n";
        } // ... nicht leer

        auto const datei_text = [](fs::path const& p) {
            std::ifstream     f{p};
            std::stringstream ss;
            ss << f.rdbuf();
            return ss.str();
        };
        std::string const alt_vorher = datei_text(bin_dir / "result.csv");

        ex::BuildSelection sel10;
        sel10.indices           = {0};
        sel10.provenance        = "explicit";
        auto const compile_fail = [](ex::BuildJob const&) -> int { return 1; }; // Compiler lehnt ab -> Fehler-Zweig
        auto const ram_stub     = []() -> std::uint64_t { return ~std::uint64_t{0}; };

        std::string       log;
        ex::LazyRunResult r10;
        {
            CerrCapture fang;
            r10 = ex::run_lazy_static_then_dynamic(tree, sel10, compile_fail, gen_stub, ram_stub, cfg10);
            log = fang.text();
        }
        // (a) DER KERN: die Alt-Mess-CSV ist BYTE-IDENTISCH erhalten (kein trunc auf einen ungesicherten Stand).
        check_true("(10) Z-01: die Alt-result.csv ist byte-identisch erhalten (Messdaten nie loeschen)",
                   datei_text(bin_dir / "result.csv") == alt_vorher);
        // (b) und sie ist NICHT von der Marker-Zeile ersetzt worden.
        check_true("(10) keine Marker-Zeile in der per-Binary-Ablage",
                   datei_text(bin_dir / "result.csv").find("nicht_gebaut") == std::string::npos);
        // (c) der Fehlschlag ist klassifiziert sichtbar (Nie-stumm) -- und behauptet nichts Unbelegtes.
        check_true("(10) der Sicherungs-Fehler ist klassifiziert sichtbar",
                   log.find("result.csv NICHT nach result.csv.stale gesichert") != std::string::npos);
        check_true("(10) klassifiziert als Compiler-Compiler-Fehler des Bau-Zweigs",
                   log.find("Compiler-Compiler-Fehler") != std::string::npos);
        check_true("(10) keine unbelegte 'der Alt-Stand liegt als result.csv.stale'-Aussage",
                   log.find("der Alt-Stand liegt als result.csv.stale") == std::string::npos);
        // (d) sichtbar bleibt der Bau-Fehler trotzdem: die globale CSV traegt die Marker-Zeile.
        check_eq("(10) der Bau-Fehler reist als Marker-Zeile in die globale CSV", r10.csv_rows.size(), std::size_t{1});
        check_true("(10) und traegt den Bau-Status nicht_gebaut",
                   r10.csv_rows.size() == 1 && r10.csv_rows[0].build_status ==
                                                   ::comdare::cache_engine::measurement::BuildCellStatus::NichtGebaut);
        // Der Resume-Anspruch ist trotzdem weg (der Stamp fiel VOR dem Sicherungs-Versuch): ein Folgelauf
        // uebernimmt den Alt-Stand nicht, er misst neu -- erhalten bleibt er als Rohdatum.
        check_true("(10) der Stamp ist gefallen -> kein Resume-Anspruch auf den Alt-Stand",
                   !fs::exists(bin_dir / "result.csv.stamp", ec));
        check_true("(10) das blockierende result.csv.stale-Verzeichnis blieb unangetastet",
                   fs::is_directory(bin_dir / "result.csv.stale", ec) &&
                       fs::exists(bin_dir / "result.csv.stale" / "belegt" / "blocker", ec));
    }

    // -- Fall (11): T2-A/F4 -- DER BATCH-PLAN LIEGT VOR DEM LAUF, UND DER ZAEHLER STEHT GEGEN IHN.
    //    Owner-KERN (Ledger abend-10): "Batch-Plan [Reihenfolge+Faecher] PERSISTENT VOR dem Lauf,
    //    Resume = Zaehler je Phase [kompiliert/separat gemessen] gegen den Plan". Codex-K1 hielt dagegen
    //    fest, dass der Planer bis dahin SOFORT streamte und die einzige Ordnungs-Zahl vor Bau und
    //    Messung hochlief -- Fehlversuche eingeschlossen. Hier am ECHTEN run_planer_driven_provision:
    //      (a) nach dem Lauf liegt der Plan als Dokument auf der Platte -- in der Ordnung des
    //          Resume-Stempels (Kopf-Glieder, dann "|rows=" mit der Fach-Zahl),
    //      (b) der Bau-Zaehler traegt die ATOME des vollzogenen Fensters (und die Mess-Front 0 -- dieser
    //          Lauf hat nicht gemessen und behauptet es auch nicht),
    //      (c) ein ZWEITER Lauf gegen dieselbe Ablage setzt hinter dem gedeckten Fach auf: er baut nichts
    //          mehr und sagt das literal,
    //      (d) ein Zaehler, der gegen eine ANDERE Selektion geschrieben wurde, traegt keinen Anspruch,
    //      (e) und ein Fenster MIT Bau-Fehler laesst den Zaehler stehen (kein Ueberspringen des Lochs).
    {
        auto const mach_bau_cfg = [&](FakeStore& store, fs::path const& out, fs::path const& plan) {
            ex::LazyRunConfig c = make_cfg(store, out);
            c.batch_plan_datei  = plan;
            return c;
        };
        auto const datei_text = [](fs::path const& p) {
            std::ifstream     f{p};
            std::stringstream ss;
            ss << f.rdbuf();
            return ss.str();
        };
        auto const mach_orch = [&](ex::LazyRunConfig const& c, ex::CompileFn const& compile) {
            ex::BuildConfig bcfg;
            bcfg.cores_per_build    = 1;
            bcfg.source_dir         = c.source_dir;
            bcfg.output_dir         = c.output_dir;
            bcfg.per_binary_subdirs = true;
            bcfg.build_parallelism  = 1;
            return ex::BuildOrchestrator{bcfg, compile, gen_stub};
        };

        fs::path const    plan_datei = base / "f4" / "batch_plan.txt";
        fs::path const    z_datei    = fs::path{plan_datei.string() + ".zaehler"};
        FakeStore         store11;
        ex::LazyRunConfig cfg11 = mach_bau_cfg(store11, base / "f4" / "lauf1", plan_datei);

        // (a)+(b) ERSTER LAUF: alles fehlt (kein Praedikat) -> ein Fach mit 8 Atomen, alle gebaut.
        {
            ex::BuildOrchestrator orch = mach_orch(cfg11, compile_stub);
            ex::BuildStats        agg;
            CerrCapture           fang;
            auto const builds = ex::run_planer_driven_provision(orch, view, alle, cfg11, agg, bl::PresenceFn{});
            check_eq("(11a) Lauf 1 baut die volle Selektion", builds.size(), std::size_t{8});
        }
        std::string const plan_stamp = bl::slice_plan_stamp(alle, bl::kBuildSliceGrain);
        check_true("(11a) der Plan liegt als Dokument auf der Platte", fs::exists(plan_datei, ec));
        check_eq("(11a) und traegt Kopf-Glieder + den Schwanz-Schluessel + die Fach-Zeile", datei_text(plan_datei),
                 plan_stamp + "|rows=1\n0;8;8\n");
        check_eq("(11b) der Bau-Zaehler traegt die Atome des Fensters, die Mess-Front bleibt 0", datei_text(z_datei),
                 plan_stamp + "|kompiliert=8|gemessen=0|rows=1\n");
        // Gegenprobe zur Grammatik: der Leser nimmt genau dieses Dokument an.
        auto const faecher11 = bl::read_batch_plan(plan_datei, plan_stamp, ex::kLazyResumeRowsKey);
        check_true("(11b) der Plan-Leser nimmt das geschriebene Dokument an", faecher11.has_value());
        check_true("(11b) und der Zaehler weist sich gegen GENAU diesen Plan aus",
                   faecher11.has_value() &&
                       bl::read_phasen_zaehler(z_datei, plan_stamp, *faecher11, ex::kLazyResumeRowsKey)
                               .value_or(bl::PhasenZaehler{99, 99}) == bl::PhasenZaehler{8, 0});

        // (c) ZWEITER LAUF gegen dieselbe Ablage -- frisches Ausgabe-Verzeichnis, damit ein etwaiger Bau
        //     SICHTBAR waere. Der Plan-Zaehler deckt das einzige Fach -> es wird uebersprungen.
        {
            ex::LazyRunConfig            cfg11b = mach_bau_cfg(store11, base / "f4" / "lauf2", plan_datei);
            ex::BuildOrchestrator        orch   = mach_orch(cfg11b, compile_stub);
            ex::BuildStats               agg;
            std::string                  log;
            std::vector<ex::BuildResult> builds;
            std::size_t                  plan_skips = 99; // Vorbelegung != 0: eine Nicht-Zuweisung faellt auf
            {
                CerrCapture fang;
                builds = ex::run_planer_driven_provision(orch, view, alle, cfg11b, agg, bl::PresenceFn{},
                                                         /*bestand_skips_out=*/nullptr, &plan_skips);
                log    = fang.text();
            }
            check_eq("(11c) Lauf 2 setzt hinter dem gedeckten Fach auf -- nichts mehr zu bauen", builds.size(),
                     std::size_t{0});
            check_true("(11c) und sagt das literal (nie stumm)",
                       log.find("plan-resume: 1 von 1 Faechern bereits kompiliert") != std::string::npos);
            // T2-A/F4-BILANZ (2026-08-06): DIE GEERBTEN ATOME WERDEN GEBUCHT. Bis zur Heilung erreichten die
            // uebersprungenen FUEHRENDEN Faecher den Slice-Loop nie, also buchte sie niemand -- agg blieb in
            // JEDEM Feld 0, LazyRunResult::built damit ebenfalls, und ein voll plan-resumierter
            // provision-only-Lauf fiel eine Ebene hoeher auf exit 1. Jetzt gilt fuer den Plan-Resume exakt
            // die Buchung des Lager-Skips: succeeded + skipped + total_jobs, `built` unberuehrt.
            check_eq("(11c) BILANZ: die 8 geerbten Atome zaehlen als bereitgestellt", agg.succeeded, std::size_t{8});
            check_eq("(11c) BILANZ: und als uebersprungen", agg.skipped, std::size_t{8});
            check_eq("(11c) BILANZ: und als gezaehlte Jobs", agg.total_jobs, std::size_t{8});
            check_eq("(11c) BILANZ: NICHTS wurde neu kompiliert", agg.built, std::size_t{0});
            check_eq("(11c) BILANZ: kein Fehlschlag", agg.failed, std::size_t{0});
            check_eq("(11c) die dritte Skip-Quelle reist einzeln zum Aufrufer", plan_skips, std::size_t{8});
            // DAS TESTAT: die Bilanz stimmt nicht stumm. Ohne diese Zeile haette der voll resumierte Lauf im
            // planer-getriebenen Pfad UEBERHAUPT KEINE [BILANZ-TESTAT]-Zeile und meldete zugleich Erfolg.
            check_eq("(11c) genau EIN [BILANZ-TESTAT] -- das des resumierten Praefixes",
                     zaehle(log, "[BILANZ-TESTAT] "), std::size_t{1});
            check_true("(11c) TESTAT: gebaut_neu=0 sidecar_skip=0 lager_skip=0 plan_skip=8 fehl=0",
                       log.find(" gebaut_neu=0 sidecar_skip=0 lager_skip=0 plan_skip=8 fehl=0") != std::string::npos);
            check_true("(11c) und das Fenster benennt das gedeckte Praefix von indices",
                       log.find(" fenster=0:8 ") != std::string::npos);
            check_true("(11c) das Testat nennt den Beleg (Zaehler, nicht Platte)",
                       log.find("plan-bilanz: 8 Atome aus 1 geplanten Faechern") != std::string::npos);
            // GEGENPROBE zur Ehrlichkeit von dauer_s: dieser Lauf hat an den geerbten Atomen KEINE Zeit
            // verbracht, also traegt seine Bilanz-Zeile auch keine (Muster der Fallback-Zeile).
            check_true("(11c) keine erfundene Fenster-Zeit an der Resume-Bilanz",
                       log.find(" dauer_s=") == std::string::npos);
        }

        // (c2) TEILWEISES RESUME -- der Fall zwischen (11a) und (11c): der Zaehler deckt EINEN Teil der
        //      Faecher, der Rest wird gebaut. Die Bilanz muss BEIDE Anteile tragen, sonst waere die
        //      Heilung nur fuer den Voll-Fall richtig. Korn 4 ueber 8 Indizes => zwei Faecher; ein
        //      vorgelegter Zaehlerstand von 4 Atomen deckt genau das erste.
        {
            fs::path const    plan_t = base / "f4t" / "batch_plan.txt";
            fs::path const    z_t    = fs::path{plan_t.string() + ".zaehler"};
            FakeStore         store_t;
            ex::LazyRunConfig cfg_t1 = mach_bau_cfg(store_t, base / "f4t" / "lauf1", plan_t);
            // T2-A/F4-NB2: das Korn reist ab jetzt in der Konfiguration (cfg.batch_plan_korn) statt als
            // Funktions-Parameter -- EINE Quelle fuer Bau-Weg und Mess-Weg (s. plan_slice_korn).
            cfg_t1.batch_plan_korn = 4;
            // Lauf 1 mit Korn 4: er baut beide Faecher und hinterlaesst den Zaehler bei 8.
            {
                ex::BuildOrchestrator orch = mach_orch(cfg_t1, compile_stub);
                ex::BuildStats        agg;
                CerrCapture           fang;
                auto const            builds =
                    ex::run_planer_driven_provision(orch, view, alle, cfg_t1, agg, bl::PresenceFn{}, nullptr, nullptr);
                check_eq("(11c2) Vorlauf baut die volle Selektion in zwei Faechern", builds.size(), std::size_t{8});
            }
            std::string const stamp_t = bl::slice_plan_stamp(alle, 4);
            check_eq("(11c2) der Plan traegt ZWEI Faecher", datei_text(plan_t), stamp_t + "|rows=2\n0;4;4\n4;4;4\n");
            // Den Zaehler auf das ERSTE Fach zuruecksetzen -- der Zustand nach einem Abbruch mitten im Lauf.
            { std::ofstream{z_t, std::ios::trunc} << stamp_t << "|kompiliert=4|gemessen=0|rows=2\n"; }

            ex::LazyRunConfig cfg_t2 = mach_bau_cfg(store_t, base / "f4t" / "lauf2", plan_t);
            cfg_t2.batch_plan_korn   = 4;
            ex::BuildStats    agg2;
            std::string       log2;
            std::size_t       plan_skips2 = 99;
            std::size_t       gebaut2     = 0;
            {
                CerrCapture           fang;
                ex::BuildOrchestrator orch = mach_orch(cfg_t2, compile_stub);
                gebaut2 =
                    ex::run_planer_driven_provision(orch, view, alle, cfg_t2, agg2, bl::PresenceFn{}, nullptr,
                                                    &plan_skips2)
                        .size();
                log2 = fang.text();
            }
            check_eq("(11c2) genau das UNGEDECKTE Fach wird gebaut", gebaut2, std::size_t{4});
            check_eq("(11c2) und genau das gedeckte wird geerbt", plan_skips2, std::size_t{4});
            // DIE BILANZ GEHT AUF: 4 geerbte + 4 gebaute == die 8 Atome der Selektion. Genau diese Summe
            // ist es, an der eine Ebene hoeher der Erfolg des provision-only-Laufs haengt.
            check_eq("(11c2) BILANZ: geerbt + gebaut == die volle Selektion", agg2.succeeded, std::size_t{8});
            check_eq("(11c2) BILANZ: 4 neu kompiliert", agg2.built, std::size_t{4});
            check_eq("(11c2) BILANZ: 4 uebersprungen (die geerbten)", agg2.skipped, std::size_t{4});
            check_eq("(11c2) BILANZ: 8 gezaehlte Jobs", agg2.total_jobs, std::size_t{8});
            check_eq("(11c2) BILANZ: kein Fehlschlag", agg2.failed, std::size_t{0});
            // ZWEI Testat-Zeilen: eine fuer das geerbte Praefix, eine fuer das gebaute Fenster -- und die
            // Zahlen der beiden addieren sich zur Selektion, ohne dass ein Atom doppelt erscheint.
            check_eq("(11c2) ZWEI [BILANZ-TESTAT]: das gebaute Fenster und das geerbte Praefix",
                     zaehle(log2, "[BILANZ-TESTAT] "), std::size_t{2});
            check_true("(11c2) TESTAT gebautes Fenster: gebaut_neu=4 ... plan_skip=0",
                       log2.find(" gebaut_neu=4 sidecar_skip=0 lager_skip=0 plan_skip=0 fehl=0") != std::string::npos);
            check_true("(11c2) TESTAT geerbtes Praefix: gebaut_neu=0 ... plan_skip=4",
                       log2.find(" gebaut_neu=0 sidecar_skip=0 lager_skip=0 plan_skip=4 fehl=0") != std::string::npos);
            check_true("(11c2) das Praefix-Fenster benennt die ersten 4 Indizes",
                       log2.find(" fenster=0:4 ") != std::string::npos);
            check_true("(11c2) das gebaute Fenster benennt die zweiten 4",
                       log2.find(" fenster=4:4 ") != std::string::npos);
            // Und die Fortschreibung bleibt korrekt: der Zaehler steht danach wieder auf allen 8 Atomen.
            check_eq("(11c2) der Zaehler ist wieder voll fortgeschrieben", datei_text(z_t),
                     stamp_t + "|kompiliert=8|gemessen=0|rows=2\n");
        }

        // (d) EIN ZAEHLER GEGEN EINE ANDERE SELEKTION TRAEGT KEINEN ANSPRUCH: derselbe Ablage-Pfad, aber
        //     der Lauf selektiert 4 statt 8 Indizes -> anderer Stempel -> voller Bau.
        {
            std::vector<std::size_t> const vier{0, 1, 2, 3};
            ex::LazyRunConfig              cfg11c = mach_bau_cfg(store11, base / "f4" / "lauf3", plan_datei);
            ex::BuildOrchestrator          orch   = mach_orch(cfg11c, compile_stub);
            ex::BuildStats                 agg;
            CerrCapture                    fang;
            auto const builds = ex::run_planer_driven_provision(orch, view, vier, cfg11c, agg, bl::PresenceFn{});
            check_eq("(11d) fremder Stempel -> kein Resume-Anspruch -> voller Bau", builds.size(), std::size_t{4});
            check_eq("(11d) und der Plan wird auf die neue Selektion umgeschrieben", datei_text(plan_datei),
                     bl::slice_plan_stamp(vier, bl::kBuildSliceGrain) + "|rows=1\n0;4;4\n");
        }

        // (e) EIN FENSTER MIT BAU-FEHLER LAESST DEN ZAEHLER STEHEN. Frische Ablage, damit (d) nicht
        //     hineinspielt; der Compiler lehnt ab -> 8 Fehlschlaege -> der Zaehler bleibt bei 0 und der
        //     Folgelauf faengt genau dort wieder an (Praefix-Resume, kein Ueberspringen des Lochs).
        {
            fs::path const        plan_e = base / "f4e" / "batch_plan.txt";
            FakeStore             store_e;
            ex::LazyRunConfig     cfg11e         = mach_bau_cfg(store_e, base / "f4e" / "lauf", plan_e);
            auto const            compile_fail_e = [](ex::BuildJob const&) -> int { return 1; };
            ex::BuildOrchestrator orch           = mach_orch(cfg11e, compile_fail_e);
            ex::BuildStats        agg;
            std::string           log;
            {
                CerrCapture fang;
                (void)ex::run_planer_driven_provision(orch, view, alle, cfg11e, agg, bl::PresenceFn{});
                log = fang.text();
            }
            check_true("(11e) der Plan liegt trotzdem (er beschreibt das SOLL, nicht den Erfolg)",
                       fs::exists(plan_e, ec));
            check_true("(11e) aber KEIN Bau-Zaehler -- ein Fehl-Fenster behauptet keine Arbeit",
                       !fs::exists(fs::path{plan_e.string() + ".zaehler"}, ec));
            check_true("(11e) und der Halt ist beziffert sichtbar",
                       log.find("Bau-Fehlern -- der Zaehler bleibt bei 0 Atomen stehen") != std::string::npos);
        }

        // (f) DIE ZWEITE HAELFTE DES KERN: "kompiliert/SEPARAT gemessen". Der Bau-Lauf schreibt die
        //     Bau-Front, der MESS-Lauf schreibt die Mess-Front in DIESELBE Ablage -- und laesst das
        //     Bau-Feld unangetastet stehen. Die gemessene Zelle ist hier eine RESUMIERTE (b.skipped +
        //     passender Stamp, s. Fall (6a-2)); genau so zaehlt die Front: "diese Binary hat ihre Zeilen".
        {
            fs::path const                 plan_f = base / "f4f" / "batch_plan.txt";
            fs::path const                 z_f    = fs::path{plan_f.string() + ".zaehler"};
            std::vector<std::size_t> const eins{0};
            std::string const              kFpF = std::string(128, 'c');

            FakeStore         store_f;
            ex::LazyRunConfig cfg_bau = mach_bau_cfg(store_f, base / "f4f" / "bau", plan_f);
            // T2-A/F4-NB2 (Befund 3): BEIDE Laeufe tragen DIESELBE Bau-Identitaet. Das ist keine
            // Test-Bequemlichkeit, sondern der produktive Ist: profile_run_entry.hpp belegt
            // bestand_fingerprint_fn UNBEDINGT in der EINEN make_cfg, die Bau- wie Mess-Pass baut. Seit
            // NB2 traegt der Plan-Stempel diese Identitaet mit -- ein Bau-Lauf OHNE Anker und ein
            // Mess-Lauf MIT Anker finden einander deshalb nicht mehr (fail-closed). Genau darauf beisst
            // (11j); hier wird die REGULAERE Lage geprueft, in der beide Seiten denselben Anker fuehren.
            cfg_bau.bestand_fingerprint_fn = [kFpF](std::string const&) { return kFpF; };
            std::string const plan_f_stamp =
                bl::slice_plan_stamp(eins, bl::kBuildSliceGrain, ex::plan_identitaet_of(view, cfg_bau));
            {
                ex::BuildOrchestrator orch = mach_orch(cfg_bau, compile_stub);
                ex::BuildStats        agg;
                CerrCapture           fang;
                (void)ex::run_planer_driven_provision(orch, view, eins, cfg_bau, agg, bl::PresenceFn{});
            }
            check_eq("(11f) der BAU-Lauf schreibt allein die Bau-Front", datei_text(z_f),
                     plan_f_stamp + "|kompiliert=1|gemessen=0|rows=1\n");

            // Der MESS-Lauf: eigene Ablage, vorbereiteter Resume-Stand, derselbe Plan.
            auto const           dims_f = tree.dynamic_filter();
            ex::BinarySpec const spec_f = view[0];
            std::string const    stem_f = ex::orch_make_stem(spec_f.binary_id, spec_f.index);
            FakeStore            dummy_f;
            ex::LazyRunConfig    cfg_mess = make_cfg(dummy_f, base / "f4f" / "mess");
            cfg_mess.provision_only       = false;
            cfg_mess.bestand_transport    = {};
            cfg_mess.bestand_doc_key.clear();
            cfg_mess.resume_completed_binaries = true;
            cfg_mess.max_binaries              = 1;
            cfg_mess.batch_plan_datei          = plan_f;
            cfg_mess.bestand_fingerprint_fn    = [kFpF](std::string const&) { return kFpF; };
            fs::path const bin_dir_f           = cfg_mess.output_dir / stem_f;
            fs::create_directories(bin_dir_f, ec);
            { std::ofstream{bin_dir_f / "perm.dll", std::ios::trunc} << "nicht-ladbar-aber-vorhanden\n"; }
            { std::ofstream{bin_dir_f / "perm.dll.fingerprint", std::ios::trunc} << kFpF; }
            {
                std::ofstream{bin_dir_f / "result.csv", std::ios::trunc} << ex::lazy_csv_header()
                                                                         << "ALTER-ERFOLGS-STAND-F4F\n";
            }
            {
                std::ofstream{bin_dir_f / "result.csv.stamp", std::ios::trunc}
                    << ex::lazy_resume_stamp_prefix(cfg_mess, dims_f) << "|fpr=" << kFpF << "|rows=1\n";
            }
            ex::BuildSelection sel_f;
            sel_f.indices                = {0};
            sel_f.provenance             = "explicit";
            auto const        ram_stub_f = []() -> std::uint64_t { return ~std::uint64_t{0}; };
            ex::LazyRunResult r_f;
            {
                CerrCapture fang;
                r_f = ex::run_lazy_static_then_dynamic(tree, sel_f, compile_stub, gen_stub, ram_stub_f, cfg_mess);
            }
            check_eq("(11f) Vorbedingung: die Zelle wurde resumiert (also gemessen im Sinn der Front)",
                     r_f.resumed_binaries, std::size_t{1});
            check_eq("(11f) der MESS-Lauf setzt die Mess-Front und laesst die Bau-Front stehen", datei_text(z_f),
                     plan_f_stamp + "|kompiliert=1|gemessen=1|rows=1\n");
            check_true("(11f) der Plan selbst ist unberuehrt (der Mess-Lauf plant nicht)",
                       datei_text(plan_f) == plan_f_stamp + "|rows=1\n0;1;1\n");
        }

        // ==========================================================================================
        // (11g) T2-A/F4-NB2, BEFUND 1 -- DER BAU-ZAEHLER LIEF VOR DEM PUSH-DRAIN HOCH.
        //
        // DIE LAGE: der asynchrone Push-Pump (W11) arbeitet NEBEN dem Bau und wurde erst HINTER der
        // ganzen Slice-Schleife drainiert (push_pump->close()). Der Plan-Zaehler dagegen lief am ENDE
        // JEDES FENSTERS hoch, mit der Bedingung `slice_stats.failed == 0`. Ein Fenster, dessen Pushes
        // samt und sonders warfen, erfuellte diese Bedingung: gebaut war alles, angekommen nichts.
        // Der Folgelauf uebersprang es dann als gedecktes Praefix -- ohne dass im Store je eine Binary
        // lag. Das ist das GEGENTEIL dessen, was der Lager-Inventar-Cache leisten soll.
        //
        // DER BISS BRAUCHT KEINEN ALT-BINAERBAU: die ALT-Bedingung war WOERTLICH `slice_stats.failed == 0`.
        // Der Lauf unten weist genau das literal aus ("gebaut_neu=8 ... fehl=0" in der [BILANZ-TESTAT]-
        // Zeile) -- unter dem Alt-Stand WAERE der Zaehler damit hochgelaufen. NEU: er tut es nicht, und
        // die Zeile sagt warum. Dazu die Gegenprobe mit EINEM einzigen geaenderten Bit (der Push wirft
        // nicht mehr): derselbe Lauf, gegenteiliges Ergebnis.
        // ==========================================================================================
        {
            std::cout << "\n-- (11g) Befund 1: der Bau-Zaehler wartet auf den Push-Vollzug --\n";
            ex::BuildSelection sel_g;
            sel_g.indices         = alle;
            sel_g.provenance      = "explicit";
            auto const ram_stub_g = []() -> std::uint64_t { return ~std::uint64_t{0}; };
            // Der planer-getriebene Zweig (und damit die Plan-Ablage) haengt an bestandslog_active UND
            // provision_only. bestandslog_active verlangt zusaetzlich den Key-Provider -- nullopt heisst
            // "kein Bestand-Rueckschrieb", das Gate selbst ist damit erfuellt (Muster Fall (5)).
            auto const kein_key = [](fs::path const&) -> std::optional<std::string> { return std::nullopt; };

            // (11g-1) DER FEHL-FALL: jeder Push wirft.
            fs::path const plan_g = base / "f4g" / "wirft" / "batch_plan.txt";
            fs::path const z_g    = fs::path{plan_g.string() + ".zaehler"};
            FakeStore      store_g;
            std::string    log_g;
            {
                ex::LazyRunConfig cfg_g = mach_bau_cfg(store_g, base / "f4g" / "wirft" / "out", plan_g);
                cfg_g.bestand_key_of    = kein_key;
                cfg_g.max_binaries      = 8;
                cfg_g.cache_push        = [](fs::path const&, std::string const&) {
                    // cppcheck-suppress throwInEntryPoint // FP: Negativprobe (11g) -- der Wurf IST der Testfall
                    throw std::runtime_error("Store-Push simuliert fehlgeschlagen");
                };
                CerrCapture fang;
                (void)ex::run_lazy_static_then_dynamic(tree, sel_g, compile_stub, gen_stub, ram_stub_g, cfg_g);
                log_g = fang.text();
            }
            check_true("(11g) der Plan selbst liegt (er beschreibt das SOLL, nicht den Vollzug)",
                       fs::exists(plan_g, ec));
            check_true("(11g) ALT-BEDINGUNG literal erfuellt: der Bau war fehlerfrei (fehl=0)",
                       log_g.find("gebaut_neu=8 sidecar_skip=0 lager_skip=0 plan_skip=0 fehl=0") != std::string::npos);
            check_true("(11g) ALT-STAND-BISS: TROTZDEM kein Bau-Zaehler -- die Artefakte sind nicht im Store",
                       !fs::exists(z_g, ec));
            check_true("(11g) und der Halt ist beziffert sichtbar (nie stumm)",
                       log_g.find("Push-Fehler bis hierher") != std::string::npos);
            check_true("(11g) mit der EIGENEN Begruendung (Store-Transport, nicht Compiler)",
                       log_g.find("Fenster fehlerfrei GEBAUT") != std::string::npos);

            // (11g-2) DIE GEGENPROBE -- EIN Bit anders: der Push gelingt. Alles Uebrige ist identisch.
            fs::path const plan_g2 = base / "f4g" / "geht" / "batch_plan.txt";
            fs::path const z_g2    = fs::path{plan_g2.string() + ".zaehler"};
            FakeStore      store_g2;
            std::size_t    pushes  = 0;
            {
                ex::LazyRunConfig cfg_g2 = mach_bau_cfg(store_g2, base / "f4g" / "geht" / "out", plan_g2);
                cfg_g2.bestand_key_of    = kein_key;
                cfg_g2.max_binaries      = 8;
                cfg_g2.cache_push        = [&pushes](fs::path const&, std::string const&) { ++pushes; };
                CerrCapture fang;
                (void)ex::run_lazy_static_then_dynamic(tree, sel_g, compile_stub, gen_stub, ram_stub_g, cfg_g2);
            }
            check_eq("(11g) Gegenprobe: alle 8 Pushes sind wirklich gelaufen", pushes, std::size_t{8});
            check_true("(11g) Gegenprobe: JETZT steht der Bau-Zaehler da", fs::exists(z_g2, ec));
            check_true("(11g) Gegenprobe: und er traegt die volle Fenster-Menge",
                       datei_text(z_g2).find("|kompiliert=8|gemessen=0|") != std::string::npos);
        }

        // ==========================================================================================
        // (11h) T2-A/F4-NB2, BEFUND 1 (die BARRIERE selbst) -- AsyncPushPump::drain().
        //
        // Die Zwischen-Barriere ist das Bauteil, das (11g) traegt: warten, bis alles bislang Eingereihte
        // ABGEARBEITET ist, ohne den Pump zu schliessen. Der kritische Punkt ist der Zustand ZWISCHEN
        // "aus der Queue gezogen" und "gebucht" -- ohne die in_flight_-Marke saehe drain() eine leere
        // Queue und meldete "vollzogen", waehrend der Push noch laeuft. Genau dieses Fenster wird hier
        // DETERMINISTISCH aufgespannt (der Push haelt an, bis der Test ihn freigibt) statt erhofft.
        // ==========================================================================================
        {
            std::cout << "\n-- (11h) Befund 1: die Zwischen-Barriere haelt den in-flight-Push --\n";
            std::atomic<bool> gestartet{false};
            std::atomic<bool> freigeben{false};
            ex::AsyncPushPump pump{[&gestartet, &freigeben](fs::path const&, std::string const&) {
                                       gestartet.store(true);
                                       while (!freigeben.load()) std::this_thread::yield();
                                       // cppcheck-suppress throwInEntryPoint // FP: Negativprobe (11h)
                                       throw std::runtime_error("Push wirft nach Freigabe");
                                   },
                                   "v-test", {}, 0};
            pump.enqueue(fs::path{"/tmp/comdare-11h-eins"});
            while (!gestartet.load()) std::this_thread::yield();
            // DAS ALT-FENSTER, deterministisch: der Eintrag ist aus der Queue, aber noch nicht gebucht.
            check_eq("(11h) ALT-FENSTER: der Push laeuft, gebucht ist noch NICHTS", pump.failed_count(),
                     std::size_t{0});
            check_eq("(11h) und auch kein Erfolg", pump.pushed_count(), std::size_t{0});
            freigeben.store(true);
            pump.drain(); // die Barriere: sie MUSS bis zur Buchung warten
            check_eq("(11h) NACH der Barriere ist der Fehl-Push gebucht", pump.failed_count(), std::size_t{1});
            check_eq("(11h) drain() schliesst den Pump NICHT -- er nimmt weiter an", pump.pushed_count(),
                     std::size_t{0});
            // Der Pump lebt weiter: ein zweiter Eintrag laeuft durch denselben Thread.
            freigeben.store(false);
            gestartet.store(false);
            pump.enqueue(fs::path{"/tmp/comdare-11h-zwei"});
            while (!gestartet.load()) std::this_thread::yield();
            freigeben.store(true);
            pump.drain();
            check_eq("(11h) der zweite Eintrag ist ebenfalls gebucht (der Pump lief weiter)", pump.failed_count(),
                     std::size_t{2});
            pump.close();
            check_eq("(11h) nach close() bleibt die Bilanz stehen", pump.failed_count(), std::size_t{2});
            pump.drain(); // idempotent + kehrt nach close() sofort zurueck (kein Haenger)
            check_eq("(11h) drain() nach close() haengt nicht und aendert nichts", pump.failed_count(),
                     std::size_t{2});
        }

        // ==========================================================================================
        // (11i) T2-A/F4-NB2, BEFUND 2 -- DIE MESS-FRONT WAR EINE BILANZ, KEIN PRAEFIX.
        //
        // DER BEFUND WOERTLICH: die Zahl lief bei JEDER irgendwo erfolgreichen oder resumierten Zelle
        // hoch (`oc.measured > 0 || oc.resumed_binaries > 0`). Fuer [Fehler, Erfolg, Erfolg] stand dort
        // `gemessen=2`, obwohl das gedeckte Praefix 0 ist. Wer diese Zahl als Front liest -- und genau
        // dazu ist eine Ablage da, die "wo stand ich" beantwortet -- ueberspringt die kaputte Zelle.
        //
        // DIE LAGE HIER IST EXAKT [Fehler, Erfolg, Erfolg]: Index 0 laesst sich nicht mehr bauen
        // (Compile-Stub gibt fuer ihn != 0 zurueck), die Indizes 1 und 2 tragen eine vollstaendige,
        // konfigurations-aktuelle Ablage und resumieren. r.resumed_binaries == 2 IST die Alt-Zahl --
        // sie steht literal im Protokoll neben der neuen Front 0.
        // ==========================================================================================
        {
            std::cout << "\n-- (11i) Befund 2: die Mess-Front ist ein Praefix --\n";
            auto const                     dims_i = tree.dynamic_filter();
            std::vector<std::size_t> const drei{0, 1, 2};
            std::string const              kFpI   = std::string(128, 'd');
            auto const                     ram_i  = []() -> std::uint64_t { return ~std::uint64_t{0}; };

            // (a) Der BAU-Lauf legt Plan + Bau-Front an (kompiliert=3). Dieselbe Identitaet wie unten.
            fs::path const plan_i = base / "f4i" / "batch_plan.txt";
            fs::path const z_i    = fs::path{plan_i.string() + ".zaehler"};
            FakeStore      store_i;
            std::string    stamp_i;
            {
                ex::LazyRunConfig cfg_bau_i      = mach_bau_cfg(store_i, base / "f4i" / "bau", plan_i);
                cfg_bau_i.bestand_fingerprint_fn = [kFpI](std::string const&) { return kFpI; };
                stamp_i = bl::slice_plan_stamp(drei, bl::kBuildSliceGrain, ex::plan_identitaet_of(view, cfg_bau_i));
                ex::BuildOrchestrator orch = mach_orch(cfg_bau_i, compile_stub);
                ex::BuildStats        agg;
                CerrCapture           fang;
                (void)ex::run_planer_driven_provision(orch, view, drei, cfg_bau_i, agg, bl::PresenceFn{});
            }
            check_eq("(11i) Vorbedingung: der Bau-Lauf hat die Bau-Front auf 3 gesetzt", datei_text(z_i),
                     stamp_i + "|kompiliert=3|gemessen=0|rows=1\n");

            // (b) Der MESS-Lauf. Die Zellen 1 und 2 bekommen ihre vollstaendige Ablage; Zelle 0 nicht --
            //     und ihr Bau scheitert, damit sie die Front an Position 0 bricht.
            FakeStore         dummy_i;
            ex::LazyRunConfig cfg_mess_i         = make_cfg(dummy_i, base / "f4i" / "mess");
            cfg_mess_i.provision_only            = false;
            cfg_mess_i.bestand_transport         = {};
            cfg_mess_i.bestand_doc_key.clear();
            cfg_mess_i.resume_completed_binaries = true;
            cfg_mess_i.max_binaries              = 3;
            cfg_mess_i.batch_plan_datei          = plan_i;
            cfg_mess_i.bestand_fingerprint_fn    = [kFpI](std::string const&) { return kFpI; };
            std::string const stem_i0 =
                ex::orch_make_stem(view[0].binary_id, view[0].index); // die Zelle, die brechen soll
            for (std::size_t i : {std::size_t{1}, std::size_t{2}}) {
                std::string const stem    = ex::orch_make_stem(view[i].binary_id, view[i].index);
                fs::path const    bin_dir = cfg_mess_i.output_dir / stem;
                fs::create_directories(bin_dir, ec);
                { std::ofstream{bin_dir / "perm.dll", std::ios::trunc} << "nicht-ladbar-aber-vorhanden\n"; }
                { std::ofstream{bin_dir / "perm.dll.fingerprint", std::ios::trunc} << kFpI; }
                {
                    std::ofstream{bin_dir / "result.csv", std::ios::trunc}
                        << ex::lazy_csv_header() << "ALTER-ERFOLGS-STAND-F4I\n";
                }
                {
                    std::ofstream{bin_dir / "result.csv.stamp", std::ios::trunc}
                        << ex::lazy_resume_stamp_prefix(cfg_mess_i, dims_i) << "|fpr=" << kFpI << "|rows=1\n";
                }
            }
            // Der Compile-Stub, der GENAU die erste Zelle scheitern laesst -- die anderen beiden werden
            // ohnehin nicht kompiliert (b.skipped ueber das Fingerprint-Sidecar).
            auto const compile_bricht_null = [&stem_i0](ex::BuildJob const& j) -> int {
                return j.output.parent_path().filename().string() == stem_i0 ? 7 : 0;
            };
            ex::BuildSelection sel_i;
            sel_i.indices    = drei;
            sel_i.provenance = "explicit";
            ex::LazyRunResult r_i;
            {
                CerrCapture fang;
                r_i = ex::run_lazy_static_then_dynamic(tree, sel_i, compile_bricht_null, gen_stub, ram_i, cfg_mess_i);
            }
            check_eq("(11i) Vorbedingung: die erste Zelle ist NICHT gebaut", r_i.build_stats.failed, std::size_t{1});
            check_eq("(11i) ALT-ZAHL literal: zwei Zellen HABEN resumiert", r_i.resumed_binaries, std::size_t{2});
            check_eq("(11i) ALT-STAND-BISS: die Mess-Front bleibt trotzdem 0 (das gedeckte Praefix ist leer)",
                     datei_text(z_i), stamp_i + "|kompiliert=3|gemessen=0|rows=1\n");

            // (c) DIE GEGENPROBE: dieselbe Ablage, aber die erste Zelle traegt ihren Stand ebenfalls --
            //     jetzt ist das Praefix voll und die Front zaehlt alle drei. Die Wache TRENNT, sie SPERRT nicht.
            {
                fs::path const bin0 = cfg_mess_i.output_dir / stem_i0;
                fs::create_directories(bin0, ec);
                { std::ofstream{bin0 / "perm.dll", std::ios::trunc} << "nicht-ladbar-aber-vorhanden\n"; }
                { std::ofstream{bin0 / "perm.dll.fingerprint", std::ios::trunc} << kFpI; }
                {
                    std::ofstream{bin0 / "result.csv", std::ios::trunc}
                        << ex::lazy_csv_header() << "ALTER-ERFOLGS-STAND-F4I\n";
                }
                {
                    std::ofstream{bin0 / "result.csv.stamp", std::ios::trunc}
                        << ex::lazy_resume_stamp_prefix(cfg_mess_i, dims_i) << "|fpr=" << kFpI << "|rows=1\n";
                }
                ex::LazyRunResult r_i2;
                {
                    CerrCapture fang;
                    r_i2 = ex::run_lazy_static_then_dynamic(tree, sel_i, compile_bricht_null, gen_stub, ram_i,
                                                            cfg_mess_i);
                }
                check_eq("(11i) Gegenprobe: jetzt resumieren alle drei", r_i2.resumed_binaries, std::size_t{3});
                check_eq("(11i) Gegenprobe: und die Front zaehlt sie auch alle drei", datei_text(z_i),
                         stamp_i + "|kompiliert=3|gemessen=3|rows=1\n");
            }
        }

        // ==========================================================================================
        // (11j) T2-A/F4-NB2, BEFUND 3 -- DER ZAEHLER WAR EINE ZWEITE, VOM FINGERPRINT UNABHAENGIGE
        //       RESUME-AUTORITAET.
        //
        // LEITSATZ (bindend): DER ZAEHLER-RESUME DARF NIE MEHR BEHAUPTEN, ALS DER FINGERPRINT DECKT.
        // Der Plan-Zaehler entfernt ganze FAECHER aus dem Strom (SlicePlanner, Schritt (4)) -- also VOR
        // dem Bau und damit VOR dll_is_current, dem EINEN Vergleich, der die Bau-Identitaet prueft. Ein
        // Zaehler aus einem Lauf mit Bau-Identitaet A galt fuer einen Lauf mit Bau-Identitaet B weiter:
        // Selektion, Groesse, Korn und Indexfolge sind identisch, wenn sich nur die Toolchain aendert.
        // Die Fingerprint-Pruefung wurde nicht ueberstimmt -- sie wurde NIE GEFRAGT.
        //
        // Der ALT-Stempel wird hier NACHGEBAUT (die v2-Form ohne |bau=), damit der Defekt literal
        // dasteht statt behauptet zu werden -- dasselbe Vorgehen wie bei der Indexfolge-Bindung.
        // ==========================================================================================
        {
            std::cout << "\n-- (11j) Befund 3: die Bau-Identitaet bindet den Zaehler --\n";
            std::string const kFpA = std::string(128, '1');
            std::string const kFpB = std::string(128, '2');
            bl::PlanIdentitaetFn const idA{[kFpA](std::size_t) { return kFpA; }};
            bl::PlanIdentitaetFn const idB{[kFpB](std::size_t) { return kFpB; }};

            // (a) DER DEFEKT LITERAL: die v2-Form (Format + start + indizes + korn + idx, KEIN |bau=).
            auto const alt_v2 = [](std::vector<std::size_t> const& idx, std::size_t korn) {
                return std::string{"batchplan-v2"} +
                       "|art=bau-slice|start=" + std::to_string(idx.empty() ? std::size_t{0} : idx.front()) +
                       "|indizes=" + std::to_string(idx.size()) + "|korn=" + std::to_string(korn) +
                       "|idx=" + bl::slice_index_digest(idx);
            };
            check_true("(11j) ALT-STAND-BISS: ohne |bau= sind zwei VERSCHIEDENE Bau-Staende stempel-GLEICH",
                       alt_v2(alle, bl::kBuildSliceGrain) == alt_v2(alle, bl::kBuildSliceGrain));
            std::string const neu_a = bl::slice_plan_stamp(alle, bl::kBuildSliceGrain, idA);
            std::string const neu_b = bl::slice_plan_stamp(alle, bl::kBuildSliceGrain, idB);
            check_true("(11j) NEU: verschiedene Bau-Staende tragen verschiedene Stempel", neu_a != neu_b);
            check_true("(11j) und der Unterschied sitzt AUSSCHLIESSLICH im |bau=-Glied",
                       neu_a.substr(0, neu_a.rfind("|bau=")) == neu_b.substr(0, neu_b.rfind("|bau=")));
            check_true("(11j) derselbe Bau-Stand ist reproduzierbar derselbe Stempel",
                       bl::slice_plan_stamp(alle, bl::kBuildSliceGrain, idA) == neu_a);
            check_true("(11j) OHNE Anker sagt der Stempel das, statt eine Deckung zu behaupten",
                       bl::slice_plan_stamp(alle, bl::kBuildSliceGrain).find("|bau=ohne-anker") != std::string::npos);
            check_true("(11j) und 'ohne Anker' ist NICHT dasselbe wie 'mit Anker'", neu_a.find("ohne-anker") ==
                                                                                        std::string::npos);

            // (b) DIE WIRKUNG am ECHTEN Bau-Weg: derselbe Plan, andere Bau-Identitaet -> der Alt-Zaehler
            //     greift NICHT und der Folgelauf baut ehrlich neu.
            fs::path const plan_j = base / "f4j" / "batch_plan.txt";
            FakeStore      store_j;
            {
                ex::LazyRunConfig cfg_a      = mach_bau_cfg(store_j, base / "f4j" / "lauf_a", plan_j);
                cfg_a.bestand_fingerprint_fn = [kFpA](std::string const&) { return kFpA; };
                ex::BuildOrchestrator orch   = mach_orch(cfg_a, compile_stub);
                ex::BuildStats        agg;
                CerrCapture           fang;
                auto const builds = ex::run_planer_driven_provision(orch, view, alle, cfg_a, agg, bl::PresenceFn{});
                check_eq("(11j) Lauf A baut die volle Selektion", builds.size(), std::size_t{8});
            }
            {
                ex::LazyRunConfig cfg_b      = mach_bau_cfg(store_j, base / "f4j" / "lauf_b", plan_j);
                cfg_b.bestand_fingerprint_fn = [kFpB](std::string const&) { return kFpB; };
                ex::BuildOrchestrator orch   = mach_orch(cfg_b, compile_stub);
                ex::BuildStats        agg;
                std::size_t           plan_skips = 99;
                CerrCapture           fang;
                auto const            builds =
                    ex::run_planer_driven_provision(orch, view, alle, cfg_b, agg, bl::PresenceFn{}, nullptr,
                                                    &plan_skips);
                check_eq("(11j) ALT-STAND-BISS: ein FREMDER Bau-Stand erbt den Zaehler NICHT", builds.size(),
                         std::size_t{8});
                check_eq("(11j) und beansprucht auch kein Plan-Resume", plan_skips, std::size_t{0});
            }
            {
                // Gegenprobe: DERSELBE Bau-Stand wie Lauf A erbt sehr wohl -- die Bindung sperrt nicht,
                // sie unterscheidet. (Lauf B hat die Ablage inzwischen auf seinen eigenen Stempel gesetzt,
                // deshalb laeuft die Gegenprobe gegen eine EIGENE Ablage mit zwei A-Laeufen.)
                fs::path const    plan_j2 = base / "f4j" / "gegenprobe" / "batch_plan.txt";
                FakeStore         store_j2;
                ex::LazyRunConfig cfg_a1      = mach_bau_cfg(store_j2, base / "f4j" / "gp_1", plan_j2);
                cfg_a1.bestand_fingerprint_fn = [kFpA](std::string const&) { return kFpA; };
                {
                    ex::BuildOrchestrator orch = mach_orch(cfg_a1, compile_stub);
                    ex::BuildStats        agg;
                    CerrCapture           fang;
                    (void)ex::run_planer_driven_provision(orch, view, alle, cfg_a1, agg, bl::PresenceFn{});
                }
                ex::LazyRunConfig cfg_a2      = mach_bau_cfg(store_j2, base / "f4j" / "gp_2", plan_j2);
                cfg_a2.bestand_fingerprint_fn = [kFpA](std::string const&) { return kFpA; };
                ex::BuildOrchestrator orch    = mach_orch(cfg_a2, compile_stub);
                ex::BuildStats        agg;
                std::size_t           plan_skips = 99;
                CerrCapture           fang;
                auto const            builds =
                    ex::run_planer_driven_provision(orch, view, alle, cfg_a2, agg, bl::PresenceFn{}, nullptr,
                                                    &plan_skips);
                check_eq("(11j) Gegenprobe: DERSELBE Bau-Stand erbt den Zaehler (nichts wird neu gebaut)",
                         builds.size(), std::size_t{0});
                check_eq("(11j) Gegenprobe: und die geerbten Atome sind gebucht", plan_skips, std::size_t{8});
            }
        }

        // ==========================================================================================
        // (11k) T2-A/F4-NB2, KORN-DIVERGENZ (Vorwellen-Notiz mittag-20) -- EINE QUELLE FUER BEIDE WEGE.
        //
        // DER BEFUND: der BAU-Weg fuehrte das Korn als Funktions-Parameter, der MESS-Weg schrieb
        // bestandslog::kBuildSliceGrain HART hin. Bei abweichendem Korn trugen die beiden Stempel
        // verschiedene "|korn="-Glieder -- der Mess-Lauf fand den Plan seines EIGENEN Bau-Laufs nicht
        // und schrieb die Mess-Front nicht fort. Fail-closed, aber still falsch: der Plan IST da.
        // ==========================================================================================
        {
            std::cout << "\n-- (11k) Korn-Divergenz: eine Quelle fuer Bau- und Mess-Weg --\n";
            ex::LazyRunConfig probe;
            check_eq("(11k) 0 heisst 'die Konstante' -- der produktive Lauf ist unveraendert",
                     ex::plan_slice_korn(probe), bl::kBuildSliceGrain);
            probe.batch_plan_korn = 4;
            check_eq("(11k) ein gesetztes Korn kommt durch", ex::plan_slice_korn(probe), std::size_t{4});

            auto const                     dims_k = tree.dynamic_filter();
            std::vector<std::size_t> const eins_k{0};
            std::string const              kFpK = std::string(128, 'e');
            fs::path const                 plan_k = base / "f4k" / "batch_plan.txt";
            fs::path const                 z_k    = fs::path{plan_k.string() + ".zaehler"};

            FakeStore         store_k;
            ex::LazyRunConfig cfg_bau_k      = mach_bau_cfg(store_k, base / "f4k" / "bau", plan_k);
            cfg_bau_k.batch_plan_korn        = 4;
            cfg_bau_k.bestand_fingerprint_fn = [kFpK](std::string const&) { return kFpK; };
            std::string const stamp_k = bl::slice_plan_stamp(eins_k, 4, ex::plan_identitaet_of(view, cfg_bau_k));
            {
                ex::BuildOrchestrator orch = mach_orch(cfg_bau_k, compile_stub);
                ex::BuildStats        agg;
                CerrCapture           fang;
                (void)ex::run_planer_driven_provision(orch, view, eins_k, cfg_bau_k, agg, bl::PresenceFn{});
            }
            check_true("(11k) Vorbedingung: der Plan traegt das gesetzte Korn",
                       datei_text(plan_k).find("|korn=4|") != std::string::npos);
            check_eq("(11k) und die Bau-Front steht", datei_text(z_k), stamp_k + "|kompiliert=1|gemessen=0|rows=1\n");

            // Die Mess-Fixture (identisch zu (11f), nur mit Korn).
            std::string const stem_k  = ex::orch_make_stem(view[0].binary_id, view[0].index);
            auto const        mach_mess_cfg = [&](fs::path const& out, std::size_t korn) {
                FakeStore         dummy;
                ex::LazyRunConfig c         = make_cfg(dummy, out);
                c.provision_only            = false;
                c.bestand_transport         = {};
                c.bestand_doc_key.clear();
                c.resume_completed_binaries = true;
                c.max_binaries              = 1;
                c.batch_plan_datei          = plan_k;
                c.batch_plan_korn           = korn;
                c.bestand_fingerprint_fn    = [kFpK](std::string const&) { return kFpK; };
                return c;
            };
            auto const lege_mess_stand = [&](ex::LazyRunConfig const& c) {
                fs::path const bin_dir = c.output_dir / stem_k;
                fs::create_directories(bin_dir, ec);
                { std::ofstream{bin_dir / "perm.dll", std::ios::trunc} << "nicht-ladbar-aber-vorhanden\n"; }
                { std::ofstream{bin_dir / "perm.dll.fingerprint", std::ios::trunc} << kFpK; }
                {
                    std::ofstream{bin_dir / "result.csv", std::ios::trunc}
                        << ex::lazy_csv_header() << "ALTER-ERFOLGS-STAND-F4K\n";
                }
                {
                    std::ofstream{bin_dir / "result.csv.stamp", std::ios::trunc}
                        << ex::lazy_resume_stamp_prefix(c, dims_k) << "|fpr=" << kFpK << "|rows=1\n";
                }
            };
            ex::BuildSelection sel_k;
            sel_k.indices       = {0};
            sel_k.provenance    = "explicit";
            auto const ram_k    = []() -> std::uint64_t { return ~std::uint64_t{0}; };

            // (11k-1) ALT-STAND-BISS: der Mess-Weg mit dem HART verdrahteten Default (4096) findet den
            //         Plan seines eigenen Bau-Laufs NICHT -- genau die Divergenz, die der Befund benennt.
            {
                ex::LazyRunConfig cfg_alt = mach_mess_cfg(base / "f4k" / "mess_4096", 0);
                lege_mess_stand(cfg_alt);
                std::string log;
                {
                    CerrCapture fang;
                    (void)ex::run_lazy_static_then_dynamic(tree, sel_k, compile_stub, gen_stub, ram_k, cfg_alt);
                    log = fang.text();
                }
                check_true("(11k) ALT-STAND-BISS: mit dem Default-Korn findet der Mess-Lauf den Plan NICHT",
                           log.find("kein zu dieser Selektion passender Batch-Plan") != std::string::npos);
                check_eq("(11k) ALT-STAND-BISS: und die Mess-Front bleibt liegen", datei_text(z_k),
                         stamp_k + "|kompiliert=1|gemessen=0|rows=1\n");
            }
            // (11k-2) NEU: dasselbe Korn aus DERSELBEN Quelle -> der Mess-Lauf findet seinen Plan.
            {
                ex::LazyRunConfig cfg_neu = mach_mess_cfg(base / "f4k" / "mess_4", 4);
                lege_mess_stand(cfg_neu);
                ex::LazyRunResult r_k;
                {
                    CerrCapture fang;
                    r_k = ex::run_lazy_static_then_dynamic(tree, sel_k, compile_stub, gen_stub, ram_k, cfg_neu);
                }
                check_eq("(11k) Vorbedingung: die Zelle hat resumiert", r_k.resumed_binaries, std::size_t{1});
                check_eq("(11k) NEU: EIN Korn fuer beide Wege -> die Mess-Front wird fortgeschrieben",
                         datei_text(z_k), stamp_k + "|kompiliert=1|gemessen=1|rows=1\n");
            }
        }
    }

    std::cout << (g_fail == 0 ? "TP1_ANKER_OK\n" : "TP1_ANKER_FAIL\n");
    return g_fail == 0 ? 0 : 1;
}
