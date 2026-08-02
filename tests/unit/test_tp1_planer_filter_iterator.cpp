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

#include <cstddef>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
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

// Gemeinsame Lauf-Konfiguration (frisches Ausgabe-Verzeichnis je Fall; leere build_version =>
// dll_is_current skippt nie => die EINZIGE Skip-Quelle ist der Bau-Filter -> saubere Zurechnung).
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
        check_eq("(2) Buchung: skipped == 3 (nur Bestand; dll_is_current war aus)", agg.skipped, std::size_t{3});
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

        std::size_t konfig_deltas = 0;
        std::size_t done_cursor   = 0;
        std::size_t done_count    = 0;
        cfg.progress_sink         = [&](ex::ProgressDelta const& d) {
            if (d.done) {
                ++done_count;
                done_cursor = d.cursor;
            } else {
                ++konfig_deltas;
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

    std::cout << (g_fail == 0 ? "TP1_ANKER_OK\n" : "TP1_ANKER_FAIL\n");
    return g_fail == 0 ? 0 : 1;
}
