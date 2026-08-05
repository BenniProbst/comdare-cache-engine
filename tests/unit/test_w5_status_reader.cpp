// W5-KERN (2026-08-05, Owner-R5) -- die Flaechen-TU des status-RUECK-LESERS.
//
// Der Status-Leser beantwortet die E-04-Ur-Frage ("wie viele Rekombinationen sind noch offen") aus dem, was
// auf der Platte und im Lager TATSAECHLICH steht. Ein solcher Leser hat genau zwei Arten zu versagen, und
// beide sind still: er kann eine Form falsch parsen (dann meldet er 0, wo Daten liegen), und er kann eine
// fehlende Quelle als Null-Ergebnis ausgeben (dann behauptet er Wissen, das er nicht hat). Diese TU prueft
// beide Richtungen.
//
// DIE FIXTURES ENTSTEHEN AUS DEN ECHTEN SCHREIB-FORMEN, NICHT AUS NACHGEBAUTEN STRINGS:
//   * der Resume-Stempel aus lazy_resume_stamp_prefix(...) + ex::kLazyResumeRowsKey (die gerade gehobene
//     EINE Konstante -- Schreiber und Leser teilen sie ab jetzt),
//   * der CSV-Kopf aus ex::lazy_csv_header() (die EINE Schema-Wahrheit),
//   * das Bestandslog aus emit_document(...) -> parse_bestandslog(...) (Roundtrip, kein Handstrick-XML),
//   * die progress.cursor-Zeilen LITERAL in beiden Formen des super-Schreibers
//     (Code/02_messung_driver/main.cpp, ProgressSinkFn) -- die deklarierte cross-repo-Format-Kopplung wird
//     hier festgenagelt, damit eine Drift ROT wird statt den Leser still zu leeren.
// KEIN NETZ: das Bestandslog kommt als lokaler String herein (bestand_sicht_aus_xml).

#include <builder/experiment_tree/cache_engine_builder_iterator.hpp> // lazy_csv_header / kLazyResumeRowsKey / Stempel
#include <profile_facade/planner/planner_status_reader.hpp>
#include <profile_facade/planner/progress_cursor_reader.hpp>

#include "comdare_test_tmp.hpp" // #278/#24 + Posten 69: per-User-/per-Build-Temp

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

namespace ex = ::comdare::cache_engine::builder::experiment;
namespace pl = ::comdare::cache_engine::planner;
namespace bl = ::comdare::cache_engine::builder::bestandslog;
namespace fs = std::filesystem;

namespace {

int  g_fail = 0;
void check(char const* was, bool ok) {
    std::cout << (ok ? "  [ ok ] " : "  [FAIL] ") << was << "\n";
    if (!ok) ++g_fail;
}
template <class A, class B>
void eq(char const* was, A const& ist, B const& soll) {
    bool const ok = (ist == soll);
    std::cout << (ok ? "  [ ok ] " : "  [FAIL] ") << was << " = " << ist;
    if (!ok) std::cout << " (erwartet " << soll << ")";
    std::cout << "\n";
    if (!ok) ++g_fail;
}

void schreibe(fs::path const& p, std::string const& inhalt) {
    std::error_code ec;
    fs::create_directories(p.parent_path(), ec);
    std::ofstream f{p, std::ios::binary | std::ios::trunc};
    f << inhalt;
}

/// Die Lauf-Konfiguration, aus der der ECHTE Schreiber seinen Stempel-Praefix bildet.
[[nodiscard]] ex::LazyRunConfig fixture_cfg() {
    ex::LazyRunConfig cfg{};
    cfg.build_version      = "m3v2";
    cfg.n_ops              = 1000;
    cfg.workload_seed      = 42;
    cfg.workload_records   = 256;
    cfg.row_series         = "A";
    cfg.row_pruefling_type = "full";
    cfg.row_fairness_mode  = "native";
    cfg.row_sweep_axis     = "-";
    cfg.row_platform       = "prod1";
    cfg.row_build_version  = "m3v2";
    return cfg;
}

/// EIN vollstaendig gemessenes Binary-Verzeichnis anlegen (CSV-Kopf + N Datenzeilen + passender Stempel).
void lege_gemessenes_binary(fs::path const& bin_dir, std::string const& stamp_prefix, std::uint64_t zeilen) {
    std::string csv = ex::lazy_csv_header();
    for (std::uint64_t i = 0; i < zeilen; ++i) csv += "zeile" + std::to_string(i) + "\n";
    schreibe(bin_dir / "result.csv", csv);
    schreibe(bin_dir / "result.csv.stamp", stamp_prefix + ex::kLazyResumeRowsKey + std::to_string(zeilen) + "\n");
}

/// EIN gebautes Binary anlegen: die (leere) Artefakt-Datei + ihr .fingerprint-Sidecar.
void lege_gebautes_binary(fs::path const& bin, std::string const& sidecar_inhalt) {
    schreibe(bin, "");
    schreibe(fs::path{bin.string() + ".fingerprint"}, sidecar_inhalt);
}

[[nodiscard]] std::string hex128(char fuellzeichen) { return std::string(128, fuellzeichen); }

/// Der SOLL-Anteil, den sonst der Director-Walk liefert -- hier direkt gesetzt, damit die TU den
/// Katalog-Walk nicht braucht (der ist eigenstaendig gegated und in dieser Flaeche nicht der Prueflings-Gegenstand).
[[nodiscard]] pl::PlanZelleSoll zelle(std::string const& ceb, std::string const& slug, std::size_t perm,
                                      std::string const& z, std::size_t schritte) {
    pl::PlanZelleSoll s{};
    s.ceb           = ceb;
    s.ceb_slug      = slug;
    s.perm_index    = perm;
    s.zelle         = z;
    s.plan_schritte = schritte;
    return s;
}

} // namespace

int main() {
    std::cout << "==== W5: status-Rueck-Leser (progress.cursor / result.csv+stamp+stale / .fingerprint / "
                 "Bestandslog) ====\n";

    fs::path const  wurzel = ::comdare::test::user_tmp_dir() / "w5_status_reader";
    std::error_code ec;
    fs::remove_all(wurzel, ec);
    fs::create_directories(wurzel, ec);

    ex::LazyRunConfig const           cfg    = fixture_cfg();
    std::vector<ex::DynamicDim> const dims   = {{"concurrency", "thread_count", {"1", "2"}, "blk"}};
    std::string const                 prefix = ex::lazy_resume_stamp_prefix(cfg, dims);
    pl::MessFormatFakten              fakten{};
    fakten.csv_header = ex::lazy_csv_header();
    fakten.rows_key   = ex::kLazyResumeRowsKey;
    check("Format-Fakten vollstaendig (csv_header + rows_key aus der Iterator-Substanz)", fakten.vollstaendig());
    eq("rows_key ist die gehobene EINE Konstante", std::string{ex::kLazyResumeRowsKey}, std::string{"|rows="});

    // ============================================================================================
    // (1) progress.cursor -- die BEIDEN Schreiber-Formen LITERAL gepinnt (cross-repo-Kopplung)
    // ============================================================================================
    std::cout << "\n-- (1) progress.cursor: die beiden Schreiber-Formen --\n";
    {
        // Die Praefix-Konstanten MUESSEN Byte fuer Byte den Formen aus super Code/02_messung_driver/main.cpp
        // entsprechen. Diese vier Vergleiche sind der eigentliche Drift-Waechter der Kopplung.
        eq("Schreiber-Form done-Praefix", std::string{pl::kProgressDonePraefix}, std::string{"[progress] done perm="});
        eq("Schreiber-Form done-Suffix", std::string{pl::kProgressDoneSuffix}, std::string{" window-complete"});
        eq("Schreiber-Form perm-Praefix", std::string{pl::kProgressPermPraefix}, std::string{"[progress] perm="});
        eq("Schreiber-Form axes-Schluessel", std::string{pl::kProgressAxesSchluessel}, std::string{" axes_changed="});

        pl::CursorStand s{};
        pl::verarbeite_cursor_zeile("[progress] perm=0 axes_changed=0", s);
        pl::verarbeite_cursor_zeile("[progress] perm=7 axes_changed=2 3->1 5->2", s);
        pl::verarbeite_cursor_zeile("[progress] perm=12 axes_changed=1 0->4", s);
        eq("letzte_perm nach drei perm-Zeilen", s.letzte_perm, std::uint64_t{12});
        eq("zeilen_perm", s.zeilen_perm, std::uint64_t{3});
        check("done noch NICHT gesehen (das Fenster laeuft)", !s.done_gesehen);
        pl::verarbeite_cursor_zeile("[progress] done perm=15 window-complete", s);
        check("done gesehen (38.b-Fertig-Signal)", s.done_gesehen);
        eq("done_perm", s.done_perm, std::uint64_t{15});
        eq("zeilen_done (Vertrag: genau eine je Fenster)", s.zeilen_done, std::uint64_t{1});
        eq("zeilen_gesamt", s.zeilen_gesamt, std::uint64_t{4});
        eq("zeilen_fremd", s.zeilen_fremd, std::uint64_t{0});

        // DRIFT-Gegenprobe: nahe, aber falsche Formen zaehlen als fremd -- sie duerfen NICHT still
        // als Fortschritt durchgehen.
        pl::CursorStand d{};
        pl::verarbeite_cursor_zeile("[progress] perm=3", d);                      // axes_changed fehlt
        pl::verarbeite_cursor_zeile("[progress] done perm=9 window-finished", d); // falscher Suffix
        pl::verarbeite_cursor_zeile("irgendeine fremde Zeile", d);
        eq("Drift-Zeilen zaehlen als fremd", d.zeilen_fremd, std::uint64_t{3});
        eq("Drift-Zeilen bewegen letzte_perm NICHT", d.letzte_perm, std::uint64_t{0});
        check("Drift-Zeilen setzen done NICHT", !d.done_gesehen);
    }

    // ============================================================================================
    // (2) LEERER STAND -- ehrlich "keine Daten", kein erfundener Nullpunkt
    // ============================================================================================
    std::cout << "\n-- (2) leerer Stand: ehrlich 'keine Daten' --\n";
    {
        pl::StatusBericht b{};
        b.planer_stempel = "planer-test";
        b.profil         = "fixture.profile.xml";
        b.root           = wurzel / "gibt_es_nicht";
        b.soll.erhoben   = true;
        b.soll.zellen.push_back(
            zelle("[all]", "_all_", 0, "[O3,no_extension][search_algo,cache_traversal,mapping]", 2));
        pl::erhebe_zellen(b, fakten);
        std::ostringstream os;
        pl::render_status(b, os);
        std::string const t = os.str();
        check("root_vorhanden=nein steht literal im Bericht", t.find("root_vorhanden=nein") != std::string::npos);
        check("quelle=messbaum meldet 'keine Daten'",
              t.find("[status] quelle=messbaum keine Daten") != std::string::npos);
        check("quelle=progress_cursor meldet 'keine Daten'",
              t.find("[status] quelle=progress_cursor keine Daten") != std::string::npos);
        check("quelle=result_csv meldet 'keine Daten'",
              t.find("[status] quelle=result_csv keine Daten") != std::string::npos);
        check("quelle=bestandslog meldet 'keine Daten'",
              t.find("[status] quelle=bestandslog keine Daten") != std::string::npos);
        check("perm_dir=fehlt in der Zell-Zeile", t.find("perm_dir=fehlt") != std::string::npos);
        check("ohne gepinntes Fenster ist offen= der Sentinel", t.find("offen=unbelegt") != std::string::npos);
        check("gemessen bleibt 0 (nichts erfunden)", t.find("gemessen=0") != std::string::npos);
        std::cout << "----- Bericht (leerer Stand) -----\n" << t << "----------------------------------\n";
    }

    // ============================================================================================
    // (3) PRAEPARIERTER MINI-STAND -- volle Bilanz aus den echten Schreib-Formen
    // ============================================================================================
    std::cout << "\n-- (3) praeparierter Mini-Stand: volle Bilanz --\n";
    fs::path const root  = wurzel / "measure_out";
    fs::path const perm0 = root / "_all_" / "perm0";
    fs::path const dll0  = perm0 / "e4_xml" / "dll";
    {
        // 2 vollstaendig gemessene Binaries, 1 mit Kopf-Drift, 1 mit Zeilen-Abweichung, 1 ohne Stempel,
        // 1 stale-Stand; 3 gueltige Sidecars + 1 kaputter.
        lege_gemessenes_binary(dll0 / "stem_a", prefix, 3);
        lege_gemessenes_binary(dll0 / "stem_b", prefix, 5);

        schreibe(dll0 / "stem_c" / "result.csv", "voellig_anderer_kopf\nz1\n");
        schreibe(dll0 / "stem_c" / "result.csv.stamp", prefix + ex::kLazyResumeRowsKey + "1\n");

        schreibe(dll0 / "stem_d" / "result.csv", ex::lazy_csv_header() + "z1\n");
        schreibe(dll0 / "stem_d" / "result.csv.stamp", prefix + ex::kLazyResumeRowsKey + "9\n");

        schreibe(dll0 / "stem_e" / "result.csv", ex::lazy_csv_header() + "z1\n"); // Stempel fehlt ganz

        schreibe(dll0 / "stem_f" / "result.csv.stale", ex::lazy_csv_header() + "alt\n");

        lege_gebautes_binary(dll0 / "stem_a" / "perm_0.so", hex128('a'));
        lege_gebautes_binary(dll0 / "stem_b" / "perm_1.so", hex128('b'));
        lege_gebautes_binary(dll0 / "stem_c" / "perm_2.so", hex128('c'));
        lege_gebautes_binary(dll0 / "stem_d" / "perm_3.so", "zu_kurz_und_nicht_hex"); // ungueltiger Sidecar

        schreibe(perm0 / "progress.cursor", "[progress] perm=0 axes_changed=0\n"
                                            "[progress] perm=4 axes_changed=1 2->1\n"
                                            "[progress] done perm=7 window-complete\n");

        pl::BinaerStand const st = pl::lies_binaer_stand(perm0, fakten);
        eq("gebaut (gueltige .fingerprint-Sidecars)", st.gebaut, std::size_t{3});
        eq("sidecar_ungueltig (sichtbar statt verschwunden)", st.sidecar_ungueltig, std::size_t{1});
        eq("csv_gesehen", st.csv_gesehen, std::size_t{5});
        eq("gemessen (csv + stamp + Kopf-Identitaet + Zeilen == rows)", st.gemessen, std::size_t{2});
        eq("teilweise", st.teilweise, std::size_t{3});
        eq("kopf_drift", st.kopf_drift, std::size_t{1});
        eq("zeilen_abweichung", st.zeilen_abweichung, std::size_t{1});
        eq("ohne_stempel", st.ohne_stempel, std::size_t{1});
        eq("stale", st.stale, std::size_t{1});

        pl::CursorStand const c = pl::read_progress_cursor(perm0);
        check("Cursor-Datei gefunden", c.datei_vorhanden);
        eq("Cursor letzte_perm", c.letzte_perm, std::uint64_t{7});
        check("Cursor done gesehen", c.done_gesehen);
    }

    // ============================================================================================
    // (4) Bestandslog-Sicht -- Roundtrip emit_document -> parse_bestandslog, KEIN Netz
    // ============================================================================================
    std::cout << "\n-- (4) Bestandslog-Sicht (Roundtrip, kein Netz) --\n";
    std::string bestand_xml;
    {
        bl::BestandslogDocument doc{};
        doc.genus        = bl::Genus::binary;
        doc.doc_revision = 17;
        doc.created_utc  = "2026-08-05T09:00:00Z";
        doc.bestand.push_back(bl::BestandEintrag{.key_sha512 = hex128('1'),
                                                 .zelle      = {.combo = "[all]", .opt = "O3", .simd = "avx2"},
                                                 .pfad       = "a/perm_0.so",
                                                 .bytes      = 1024,
                                                 .stempel    = "[O3,avx2][x,y,z]+bt=Release",
                                                 .done_utc   = "2026-08-05T08:00:00Z",
                                                 .versions   = {}});
        doc.bestand.push_back(bl::BestandEintrag{.key_sha512 = hex128('2'),
                                                 .zelle      = {.combo = "[all]", .opt = "O3", .simd = "avx2"},
                                                 .pfad       = "b/perm_1.so",
                                                 .bytes      = 2048,
                                                 .stempel    = "[O3,avx2][x,y,z]+bt=Release",
                                                 .done_utc   = "2026-08-05T08:10:00Z",
                                                 .versions   = {}});
        bl::BatchReservierung r1{};
        r1.id                    = "owner-1/0";
        r1.typ                   = bl::BatchTyp::tier;
        r1.slice_begin           = 0;
        r1.slice_count           = 4096;
        r1.maschine              = "prod1";
        r1.threads               = 32;
        r1.status                = bl::BatchStatus::offen; // ohne eta_s => "noch nicht geschaetzt"
        bl::BatchReservierung r2 = r1;
        r2.id                    = "owner-1/1";
        r2.status                = bl::BatchStatus::done;
        r2.eta_s                 = "120";
        bl::BatchReservierung r3 = r1;
        r3.id                    = "owner-2/0";
        r3.status                = bl::BatchStatus::released;
        doc.reservierungen       = {r1, r2, r3};
        bestand_xml              = bl::emit_document(doc);

        pl::BestandSicht const s = pl::bestand_sicht_aus_xml(bestand_xml);
        check("Bestandslog gelesen", s.gelesen);
        eq("doc_revision", s.doc_revision, std::uint64_t{17});
        eq("genus", s.genus, std::string{"binary"});
        eq("Bestands-Eintraege", s.eintraege, std::size_t{2});
        eq("Reservierungen offen", s.res_offen, std::size_t{1});
        eq("Reservierungen done", s.res_done, std::size_t{1});
        eq("Reservierungen released", s.res_released, std::size_t{1});
        eq("Reservierungen ohne ETA (ehrlich 'noch nicht geschaetzt')", s.res_ohne_eta, std::size_t{2});

        pl::BestandSicht const kaputt = pl::bestand_sicht_aus_xml("<kein_bestandslog/>");
        check("Fremd-Wurzel -> nicht gelesen", !kaputt.gelesen);
        check("Fremd-Wurzel -> Grund benannt (nie stumm)", !kaputt.grund.empty());
    }

    // ============================================================================================
    // (5) VOLLER BERICHT ueber den praeparierten Stand, MIT gepinntem Fenster
    // ============================================================================================
    std::cout << "\n-- (5) voller Bericht mit gepinntem Fenster --\n";
    {
        pl::StatusBericht b{};
        b.planer_stempel               = "planer-test";
        b.profil                       = "fixture.profile.xml";
        b.root                         = root;
        b.fenster                      = "0:16";
        b.fenster_bekannt              = true;
        b.fenster_count                = 16;
        b.soll.erhoben                 = true;
        b.soll.source_kind             = "thesis";
        b.soll.profile_id              = "fixture";
        b.soll.perm_count              = 2;
        b.soll.measurement_combo_count = 1;
        b.soll.zellen.push_back(
            zelle("[all]", "_all_", 0, "[O3,no_extension][search_algo,cache_traversal,mapping]", 2));
        b.soll.zellen.push_back(zelle("[all]", "_all_", 1, "[O2,avx2][search_algo,cache_traversal,mapping]", 2));
        b.bestand = pl::bestand_sicht_aus_xml(bestand_xml);
        pl::erhebe_zellen(b, fakten);

        std::ostringstream os;
        pl::render_status(b, os);
        std::string const t = os.str();
        std::cout << "----- Bericht (Mini-Stand) -----\n" << t << "--------------------------------\n";

        check("Gesamt-Zeile vorhanden", t.find("[status-gesamt]") != std::string::npos);
        check("Gesamt: gebaut=3",
              t.find("[status-gesamt]") != std::string::npos && t.find(" gebaut=3 ") != std::string::npos);
        check("Gesamt: gemessen=2", t.find(" gemessen=2 ") != std::string::npos);
        check("Gesamt: offen=14 (Fenster 16 minus 2 gemessen)", t.find(" offen=14\n") != std::string::npos);
        check("ceb= ist ein EIGENES Feld (Layer nie verschmolzen)", t.find("ceb=[all] zelle=[") != std::string::npos);
        check("zweite Perm ohne Verzeichnis meldet perm_dir=fehlt", t.find("perm_dir=fehlt") != std::string::npos);
        check("Cursor-Zeile der ersten Perm meldet done=ja", t.find("done=ja") != std::string::npos);
        check("Cursor-Zeile der zweiten Perm meldet den Sentinel", t.find("letzte_perm=unbelegt") != std::string::npos);
        check("Bestandslog-Zeile vorhanden",
              t.find("[status-bestand] genus=binary doc_revision=17") != std::string::npos);
        check("sidecar_ungueltig sichtbar", t.find("sidecar_ungueltig=1") != std::string::npos);

        // DETERMINISMUS: zwei Erhebungen desselben Standes liefern byte-gleiche Berichte.
        pl::StatusBericht b2 = b;
        pl::erhebe_zellen(b2, fakten);
        std::ostringstream os2;
        pl::render_status(b2, os2);
        check("zwei Laeufe ueber denselben Stand sind byte-gleich", os2.str() == t);
    }

    // ============================================================================================
    // (6) SOLL nicht erhoben -- der Bericht sagt es, statt eine leere Bilanz zu behaupten
    // ============================================================================================
    std::cout << "\n-- (6) SOLL nicht erhoben --\n";
    {
        pl::StatusBericht b{};
        b.root         = root;
        b.soll.erhoben = false;
        b.soll.grund   = "Profil 'x.xml' nicht als bekannte Wurzel lesbar (rc 5)";
        pl::erhebe_zellen(b, fakten);
        std::ostringstream os;
        pl::render_status(b, os);
        std::string const t = os.str();
        check("plan=nicht_erhoben literal", t.find("plan=nicht_erhoben") != std::string::npos);
        check("quelle=plan meldet 'keine Daten' MIT Grund",
              t.find("[status] quelle=plan keine Daten (Profil") != std::string::npos);
        check("keine Zell-Zeile ohne SOLL (nichts erfunden)", t.find("[status-zelle]") == std::string::npos);
    }

    fs::remove_all(wurzel, ec);
    std::cout << "\n==== W5 status-Rueck-Leser: " << (g_fail == 0 ? "ALLE OK" : "FEHLER") << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
