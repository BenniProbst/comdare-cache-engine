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
//        (Ist: gebaut_neu/sidecar_skip/lager_skip/fehl) -- die Zahlen stammen aus DEMSELBEN
//        slice_stats/Filter-Ergebnis wie die geprueften Aggregat-Zahlen (Single-Source-Beleg).
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
        check_true("(1m) IST: gebaut_neu=5 sidecar_skip=0 lager_skip=3 fehl=0",
                   spur.find(" gebaut_neu=5 sidecar_skip=0 lager_skip=3 fehl=0") != std::string::npos);
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
        auto const            dims          = tree.dynamic_filter();
        ex::BinarySpec const  spec0         = view[0];
        std::string const     stem0         = ex::orch_make_stem(spec0.binary_id, spec0.index);
        constexpr char const* kAltMarke     = "ALTER-ERFOLGS-STAND";
        // T2-A/K2: `extra` traegt die per-Binary-Felder, die der Lauf an den Config-Praefix haengt
        // (heute "|fpr=<128hex>"). Der Stamp wird weiterhin ueber DIESELBEN Funktionen erzeugt, die der
        // Resume-Check liest -- kein handkopierter Stempel-String.
        auto const            lege_altstand = [&](ex::LazyRunConfig const& c, std::string const& extra = {}) {
            fs::path const  bin_dir = c.output_dir / stem0;
            std::error_code lec;
            fs::create_directories(bin_dir, lec);
            { std::ofstream{bin_dir / "result.csv", std::ios::trunc} << ex::lazy_csv_header() << kAltMarke << "\n"; }
            {
                std::ofstream{bin_dir / "result.csv.stamp", std::ios::trunc}
                    << ex::lazy_resume_stamp_prefix(c, dims) << extra << "|rows=1\n";
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
        std::string const kFpAlt = std::string(128, 'a');
        std::string const kFpNeu = std::string(128, 'b');
        ex::LazyRunConfig cfg6n  = mach_cfg(base / "b10_skip");
        cfg6n.bestand_fingerprint_fn = [kFpAlt](std::string const&) { return kFpAlt; };
        fs::path const bin_dir_skip = lege_altstand(cfg6n, "|fpr=" + kFpAlt);
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
                                     " sidecar_skip=" + std::to_string(r.built_skip) + " lager_skip=0 fehl=0";
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

    std::cout << (g_fail == 0 ? "TP1_ANKER_OK\n" : "TP1_ANKER_FAIL\n");
    return g_fail == 0 ? 0 : 1;
}
