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
#include <limits>
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

/// Die Bericht-Zeile, die mit <marker> beginnt UND <teil> enthaelt -- MIT abschliessendem "\n", damit ein
/// Feld am Zeilenende exakt geprueft werden kann. Leer, wenn es sie nicht gibt.
///
/// WARUM ZEILENGENAU: die Bilanz-Befunde unterscheiden sich nur durch die ZEILE, in der eine Zahl steht.
/// `offen=14` gehoert in die ZELL-Zeile (16 SOLL minus die 2 Messungen dieser Zelle) und ist dort richtig;
/// in der GESAMT-Zeile ist genau dieselbe Zahl der Befund H2. Ein Volltext-find kann beides nicht trennen.
[[nodiscard]] std::string zeile_mit(std::string const& bericht, std::string const& marker,
                                    std::string const& teil = {}) {
    std::size_t pos = 0;
    while ((pos = bericht.find(marker, pos)) != std::string::npos) {
        std::size_t const ende  = bericht.find('\n', pos);
        std::string const zeile = bericht.substr(pos, (ende == std::string::npos ? bericht.size() : ende) - pos) + "\n";
        if (teil.empty() || zeile.find(teil) != std::string::npos) return zeile;
        if (ende == std::string::npos) break;
        pos = ende + 1;
    }
    return {};
}

[[nodiscard]] bool enthaelt(std::string const& heu, std::string const& nadel) {
    return heu.find(nadel) != std::string::npos;
}

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
    // T2-A/F4-NB2 (Befund 4): das DRITTE Format-Faktum -- die Generations-Marke am Kopf der Resume-Zeile.
    fakten.stamp_format = ex::kLazyResumeStampFormat;
    check("Format-Fakten vollstaendig (csv_header + rows_key + stamp_format aus der Iterator-Substanz)",
          fakten.vollstaendig());
    eq("rows_key ist die gehobene EINE Konstante", std::string{ex::kLazyResumeRowsKey}, std::string{"|rows="});
    eq("stamp_format ist die gehobene EINE Konstante", std::string{ex::kLazyResumeStampFormat},
       std::string{"resume-v6"});
    // Die Hebung muss den SCHREIBER treffen, nicht nur danebenstehen: der reale Stempel-Praefix beginnt mit
    // genau diesem Wort. Ohne diese Zeile waere die Konstante eine zweite Wahrheit statt der einen.
    check("und der REALE Schreiber beginnt mit genau dieser Marke",
          prefix.compare(0, std::string{ex::kLazyResumeStampFormat}.size(), ex::kLazyResumeStampFormat) == 0);
    // Die EINE Schema-Wahrheit literal ausgeben: so ist im ctest-Protokoll nachlesbar, GEGEN WAS der Leser die
    // Kopf-Identitaet prueft -- und ein Rauchtest am echten Binary kann seine Fixture damit exakt bauen.
    std::cout << "  (info) lazy_csv_header() = " << fakten.csv_header;
    if (fakten.csv_header.empty() || fakten.csv_header.back() != '\n') std::cout << "\n";

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
        // B4-Nachbesserung: auch die ACHSEN-LISTE ist Schreiber-Form und damit Teil der Kopplung
        // (`line << " " << c.axis_index << "->" << c.variant_index`).
        eq("Schreiber-Form Paar-Trenner", std::string{pl::kProgressPaarTrenner}, std::string{" "});
        eq("Schreiber-Form Paar-Pfeil", std::string{pl::kProgressPaarPfeil}, std::string{"->"});

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
    // (1b) M1 -- done ist NICHT sticky: die Datei ueberlebt das Fenster
    //
    // BISS AM ALT-STAND: der Alt-Leser setzte done_gesehen einmal auf true und nahm es nie zurueck. An einer
    // Perm, an der ein FOLGE-Fenster hinter das done weiterschreibt, meldete der Bericht darum "done=ja",
    // waehrend das naechste Fenster noch misst -- der kuenftige 38.b-Takt haette die Folge-CEB losgezogen.
    // ============================================================================================
    std::cout << "\n-- (1b) M1: done faellt beim naechsten laufenden Fenster zurueck --\n";
    {
        pl::CursorStand s{};
        pl::verarbeite_cursor_zeile("[progress] perm=0 axes_changed=1 2->1", s);
        pl::verarbeite_cursor_zeile("[progress] done perm=3 window-complete", s);
        check("nach der done-Zeile: done=ja", s.done_gesehen);
        eq("nach der done-Zeile: done_perm", s.done_perm, std::uint64_t{3});

        // Das FOLGE-Fenster schreibt hinter das done weiter -- ab hier laeuft wieder etwas.
        pl::verarbeite_cursor_zeile("[progress] perm=0 axes_changed=2 0->1 4->2", s);
        check("ALT-STAND-BISS: done faellt bei der spaeteren perm-Zeile zurueck (nicht sticky)", !s.done_gesehen);
        eq("done_perm faellt mit zurueck (gilt laut Vertrag nur bei done_gesehen)", s.done_perm, std::uint64_t{0});
        eq("letzte_perm folgt dem NEUEN Fenster", s.letzte_perm, std::uint64_t{0});
        eq("zeilen_done bleibt die ehrliche Historie", s.zeilen_done, std::uint64_t{1});

        // Und das zweite Fenster meldet seinerseits fertig.
        pl::verarbeite_cursor_zeile("[progress] done perm=9 window-complete", s);
        check("das zweite Fenster meldet wieder done", s.done_gesehen);
        eq("done_perm des zweiten Fensters", s.done_perm, std::uint64_t{9});
        eq("zwei Fenster in EINER Datei = zwei done-Zeilen", s.zeilen_done, std::uint64_t{2});
    }

    // ============================================================================================
    // (1c) M2 -- das Abbruch-Fragment ist KEIN Fortschritt
    //
    // BISS AM ALT-STAND: der Alt-Leser pruefte nur, ob " axes_changed=" folgt, nicht ob dahinter ein WERT
    // steht. Ein bei SIGKILL/Platte-voll halb geschriebenes "[progress] perm=7 axes_changed=" ging damit als
    // volle Fortschritts-Zeile durch und zog letzte_perm auf einen nie erreichten Cursor 7.
    // ============================================================================================
    std::cout << "\n-- (1c) M2: Abbruch-Fragment zaehlt als abgebrochene Zeile, nicht als Fortschritt --\n";
    {
        pl::CursorStand s{};
        pl::verarbeite_cursor_zeile("[progress] perm=2 axes_changed=1 3->1", s);
        pl::verarbeite_cursor_zeile("[progress] perm=7 axes_changed=", s); // der Schreiber brach hier ab
        eq("ALT-STAND-BISS: letzte_perm bleibt beim letzten VOLLSTAENDIGEN Fortschritt (2, nicht 7)", s.letzte_perm,
           std::uint64_t{2});
        eq("ALT-STAND-BISS: das Fragment zaehlt NICHT als perm-Zeile", s.zeilen_perm, std::uint64_t{1});
        eq("das Fragment wird als abgebrochene Zeile ausgewiesen", s.zeilen_abgebrochen, std::uint64_t{1});
        eq("das Fragment ist KEINE fremde Form (der Schluessel stand ja da)", s.zeilen_fremd, std::uint64_t{0});
        eq("es wird trotzdem als Zeile gezaehlt (nichts verschwindet)", s.zeilen_gesamt, std::uint64_t{2});

        // Nicht-numerischer Wert: dieselbe Klasse.
        pl::verarbeite_cursor_zeile("[progress] perm=8 axes_changed=x", s);
        eq("nicht-numerisches <K> ist dieselbe Klasse", s.zeilen_abgebrochen, std::uint64_t{2});
        eq("und bewegt letzte_perm ebenfalls nicht", s.letzte_perm, std::uint64_t{2});

        // B4: der Abbruch kann auch MITTEN IM WERT liegen. "axes_changed=1x" traegt eine gueltige Ziffer und
        // danach Muell -- ein Praefix-Parser liest die 1 und laesst die Zeile als vollen Fortschritt durch.
        // Der Rest hinter <K> MUSS darum die Schreiber-Form treffen: Zeilenende oder Trenn-Leerzeichen.
        pl::verarbeite_cursor_zeile("[progress] perm=9 axes_changed=1x", s);
        eq("ALT-STAND-BISS: <K> mit angehaengtem Muell ist eine abgebrochene Zeile", s.zeilen_abgebrochen,
           std::uint64_t{3});
        eq("ALT-STAND-BISS: sie zaehlt NICHT als perm-Zeile", s.zeilen_perm, std::uint64_t{1});
        eq("ALT-STAND-BISS: und zieht letzte_perm nicht auf 9", s.letzte_perm, std::uint64_t{2});

        // GEGENPROBE: die BEIDEN gueltigen Schreiber-Enden hinter <K> bleiben Fortschritt -- die Rest-Pruefung
        // darf die echte Form nicht miterschlagen.
        pl::verarbeite_cursor_zeile("[progress] perm=10 axes_changed=0", s); // K==0 -> Zeilenende
        eq("Gegenprobe: <K> am Zeilenende bleibt Fortschritt", s.zeilen_perm, std::uint64_t{2});
        eq("Gegenprobe: und traegt den Cursor", s.letzte_perm, std::uint64_t{10});
        pl::verarbeite_cursor_zeile("[progress] perm=11 axes_changed=2 3->1 5->2", s); // Leerzeichen zur Liste
        eq("Gegenprobe: <K> vor der Achsen-Liste bleibt Fortschritt", s.zeilen_perm, std::uint64_t{3});
        eq("Gegenprobe: und traegt den Cursor", s.letzte_perm, std::uint64_t{11});
    }

    // ============================================================================================
    // (1c2) B4-NACHBESSERUNG (Codex Zyklus 3) -- <K> ist eine ANKUENDIGUNG, und sie wird auf EINLOESUNG geprueft
    //
    // BISS AM ALT-STAND (Stand 8c4c84b0): die Rest-Pruefung hinter <K> las genau EIN Zeichen -- Zeilenende
    // oder Trenn-Leerzeichen. Damit ging jede Zeile durch, die K ankuendigte, die K Paare aber gar nicht,
    // nicht vollstaendig oder gar nicht in Schreiber-Form trug. GENAU SO reisst der Schreiber ab: er baut die
    // Zeile links nach rechts (`line << " axes_changed=" << d.changed.size();` DANN die Schleife), also steht
    // die Ankuendigung IMMER schon da, wenn die Liste fehlt. Jede dieser Formen zog letzte_perm auf einen nie
    // erreichten Cursor -- dieselbe Luege wie in M2/B4, nur ein Feld weiter rechts.
    // ============================================================================================
    std::cout << "\n-- (1c2) B4-Nachbesserung: die angekuendigten K Achsen-Paare muessen VOLLSTAENDIG dastehen --\n";
    {
        pl::CursorStand s{};
        pl::verarbeite_cursor_zeile("[progress] perm=2 axes_changed=1 3->1", s); // der letzte VOLLSTAENDIGE Stand
        eq("Vorlauf: die vollstaendige Zeile traegt den Cursor", s.letzte_perm, std::uint64_t{2});
        eq("Vorlauf: und zaehlt als perm-Zeile", s.zeilen_perm, std::uint64_t{1});

        // (a) Codex-Form 1: K angekuendigt, KEIN Paar geschrieben.
        pl::verarbeite_cursor_zeile("[progress] perm=5 axes_changed=1", s);
        eq("ALT-STAND-BISS (a): 'axes_changed=1' OHNE Paar ist eine abgebrochene Zeile", s.zeilen_abgebrochen,
           std::uint64_t{1});
        eq("ALT-STAND-BISS (a): sie zaehlt NICHT als perm-Zeile", s.zeilen_perm, std::uint64_t{1});
        eq("ALT-STAND-BISS (a): und zieht letzte_perm nicht auf 5", s.letzte_perm, std::uint64_t{2});

        // (b) Codex-Form 2: 2 angekuendigt, nur 1 geschrieben -- der Abriss zwischen den Paaren.
        pl::verarbeite_cursor_zeile("[progress] perm=6 axes_changed=2 3->1", s);
        eq("ALT-STAND-BISS (b): 'axes_changed=2 3->1' traegt zu WENIGE Paare", s.zeilen_abgebrochen, std::uint64_t{2});
        eq("ALT-STAND-BISS (b): sie zaehlt NICHT als perm-Zeile", s.zeilen_perm, std::uint64_t{1});
        eq("ALT-STAND-BISS (b): und zieht letzte_perm nicht auf 6", s.letzte_perm, std::uint64_t{2});

        // (c) Codex-Form 3: K angekuendigt, dahinter ein BELIEBIGER Anhang statt der Liste.
        pl::verarbeite_cursor_zeile("[progress] perm=7 axes_changed=1 window-complete", s);
        eq("ALT-STAND-BISS (c): '=1 <beliebiger Anhang>' ist keine Achsen-Liste", s.zeilen_abgebrochen,
           std::uint64_t{3});
        eq("ALT-STAND-BISS (c): sie zaehlt NICHT als perm-Zeile", s.zeilen_perm, std::uint64_t{1});
        eq("ALT-STAND-BISS (c): und zieht letzte_perm nicht auf 7", s.letzte_perm, std::uint64_t{2});

        // (c2) derselbe Befund eine Ebene tiefer: der Abriss liegt MITTEN IM Paar (Pfeil ohne Ziel).
        pl::verarbeite_cursor_zeile("[progress] perm=13 axes_changed=1 3->", s);
        eq("Abriss IM Paar (Pfeil ohne Varianten-Index) ist dieselbe Klasse", s.zeilen_abgebrochen, std::uint64_t{4});
        eq("und bewegt letzte_perm ebenfalls nicht", s.letzte_perm, std::uint64_t{2});

        // (c3) die Gegenrichtung: MEHR Paare als angekuendigt ist ebenfalls keine Schreiber-Form.
        pl::verarbeite_cursor_zeile("[progress] perm=14 axes_changed=1 3->1 5->2", s);
        eq("mehr Paare als angekuendigt ist ebenfalls keine Schreiber-Form", s.zeilen_abgebrochen, std::uint64_t{5});
        eq("und bewegt letzte_perm ebenfalls nicht", s.letzte_perm, std::uint64_t{2});

        // (d) GUT-FALL: K=2 mit GENAU 2 Paaren -- die neue Wache darf die echte Form nicht miterschlagen.
        pl::verarbeite_cursor_zeile("[progress] perm=8 axes_changed=2 3->1 5->2", s);
        eq("Gut-Fall: K=2 mit genau 2 Paaren bleibt voller Fortschritt", s.zeilen_perm, std::uint64_t{2});
        eq("Gut-Fall: und traegt den Cursor auf 8", s.letzte_perm, std::uint64_t{8});
        eq("Gut-Fall: er erhoeht die Abbruch-Zahl nicht", s.zeilen_abgebrochen, std::uint64_t{5});
        check("Gut-Fall: er belegt den Cursor", s.letzte_perm_belegt);
        eq("keine dieser Formen ist FREMD (der Schluessel stand ueberall da)", s.zeilen_fremd, std::uint64_t{0});
        eq("alle Zeilen bleiben gezaehlt (nichts verschwindet)", s.zeilen_gesamt, std::uint64_t{7});
    }

    // ============================================================================================
    // (1d) B5 -- "nie gemeldet" ist NICHT "steht bei Perm 0"
    //
    // BISS AM ALT-STAND: letzte_perm ist mit 0 vorbelegt, und 0 ist ein ECHTER Cursor-Wert (die erste Perm
    // eines Fensters). Eine progress.cursor, die NUR abgerissene oder fremde Zeilen traegt, meldete darum
    // "letzte_perm=0" -- ununterscheidbar von einem Fenster, das seine erste Perm gerade fertig hat.
    // ============================================================================================
    std::cout << "\n-- (1d) B5: letzte_perm_belegt trennt 'nie gemeldet' von 'Perm 0' --\n";
    {
        pl::CursorStand leer{};
        check("frischer Stand: der Cursor ist unbelegt", !leer.letzte_perm_belegt);

        pl::CursorStand nur_fragmente{};
        pl::verarbeite_cursor_zeile("[progress] perm=4 axes_changed=", nur_fragmente);
        pl::verarbeite_cursor_zeile("irgendeine fremde Zeile", nur_fragmente);
        eq("nur Fragmente/Fremdes gelesen -> zeilen_perm bleibt 0", nur_fragmente.zeilen_perm, std::uint64_t{0});
        check("ALT-STAND-BISS: der Cursor ist NICHT belegt (die 0 waere eine Behauptung)",
              !nur_fragmente.letzte_perm_belegt);

        // Die ECHTE Perm 0 ist der Gegenfall: gleiche Zahl, andere Aussage.
        pl::CursorStand echte_null{};
        pl::verarbeite_cursor_zeile("[progress] perm=0 axes_changed=0", echte_null);
        check("die ECHTE Perm 0 belegt den Cursor", echte_null.letzte_perm_belegt);
        eq("und traegt den Wert 0", echte_null.letzte_perm, std::uint64_t{0});

        // Auch die done-Zeile belegt den Cursor (sie meldet einen erreichten Stand).
        pl::CursorStand nur_done{};
        pl::verarbeite_cursor_zeile("[progress] done perm=0 window-complete", nur_done);
        check("auch die done-Zeile belegt den Cursor", nur_done.letzte_perm_belegt);

        // UND AM BERICHT: eine VORHANDENE progress.cursor, die nur ein Fragment traegt. Genau hier trennen
        // sich Alt- und Neu-Stand -- der Alt-Renderer machte `letzte_perm` an `datei_vorhanden` fest und
        // druckte darum die Default-0 als gemeldeten Cursor.
        fs::path const f_root = wurzel / "nur_fragment";
        schreibe(f_root / "_all_" / "perm0" / "progress.cursor", "[progress] perm=4 axes_changed=\n");

        pl::StatusBericht fb{};
        fb.planer_stempel  = "planer-test";
        fb.root            = f_root;
        fb.fenster         = "0:2";
        fb.fenster_bekannt = true;
        fb.fenster_start   = 0;
        fb.fenster_count   = 2;
        fb.soll.erhoben    = true;
        fb.soll.zellen.push_back(zelle("[all]", "_all_", 0, "[O3,no_extension][a1,b1,c1]", 2));
        pl::erhebe_zellen(fb, fakten);
        std::ostringstream fos;
        pl::render_status(fb, fos);
        std::string const fc = zeile_mit(fos.str(), "[status-cursor]", " perm=0 ");
        std::cout << "----- Cursor-Zeile (nur ein Fragment) -----\n"
                  << fc << "-------------------------------------------\n";
        check("die Datei IST da (die Quelle fehlt nicht)", enthaelt(fc, " cursor_datei=vorhanden "));
        check("ALT-STAND-BISS: letzte_perm=unbelegt statt der Default-0", enthaelt(fc, " letzte_perm=unbelegt "));
        check("ALT-STAND-BISS: die Zeile behauptet NICHT letzte_perm=0", !enthaelt(fc, " letzte_perm=0 "));
        check("das Fragment bleibt sichtbar", enthaelt(fc, " abgebrochene_zeile=1\n"));
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
    // (3b) T2-A/F4-NB2, BEFUND 4 -- ZWEI FORTSCHRITTS-WAHRHEITEN UEBER DIESELBE DATEI.
    //
    // DER BEFUND: dieser Leser prueft den Konfigurations-Praefix des Resume-Stempels bewusst NICHT (er
    // kennt die Lauf-Konfiguration nicht). Damit galt ihm ein Stempel aus einer AELTEREN Format-Generation
    // (resume-v5) als gueltiger Messstand, waehrend der echte Runner ihn ueber den Praefix-Vergleich
    // korrekt verwirft. Zwei Bilanzen ueber DIESELBE Datei -- und die des Lesers fiel in die gefaehrliche
    // Richtung ("fertiger als es ist"), also in die, die einen 38.b-Takt zu frueh weiterzoege.
    //
    // DER BISS IST LITERAL UND SCHARF: der v5-Stand ist mit dem v6-Stand BYTE-GLEICH bis auf das eine
    // Versions-Wort. Gleicher CSV-Kopf, gleiche Zeilenzahl, gleiche rows-Zahl im Stempel -- JEDE andere
    // Wache des Lesers geht durch. Genau deshalb hatte der Alt-Stand nichts mehr, was ihn haette fangen
    // koennen; die Zelle zaehlte als gemessen. NEU: sie zaehlt als teilweise UND wird als format_drift
    // BENANNT (nicht in einen Sammel-Eimer geworfen).
    // ============================================================================================
    std::cout << "\n-- (3b) Befund 4: ein Alt-Format-Stempel gilt dem Leser NICHT mehr als gemessen --\n";
    {
        fs::path const perm_v = wurzel / "format_drift" / "_all_" / "perm0";
        fs::path const dll_v  = perm_v / "e4_xml" / "dll";

        // Der v5-Praefix entsteht aus dem ECHTEN v6-Praefix durch Ersetzen NUR des Versions-Wortes --
        // so ist bewiesen, dass sich sonst kein Byte unterscheidet (kein von Hand gebauter Fremd-Stempel).
        std::string const marke_v6  = ex::kLazyResumeStampFormat;
        std::string const marke_v5  = "resume-v5";
        std::string const prefix_v5 = marke_v5 + prefix.substr(marke_v6.size());
        eq("Vorbedingung: der v5-Praefix unterscheidet sich NUR im Versions-Wort", prefix_v5.substr(marke_v5.size()),
           prefix.substr(marke_v6.size()));

        lege_gemessenes_binary(dll_v / "stem_v6", prefix, 3);                 // der Gut-Fall
        schreibe(dll_v / "stem_v5" / "result.csv", ex::lazy_csv_header() + "z1\nz2\nz3\n");
        schreibe(dll_v / "stem_v5" / "result.csv.stamp", prefix_v5 + ex::kLazyResumeRowsKey + "3\n");

        pl::BinaerStand const st = pl::lies_binaer_stand(perm_v, fakten);
        eq("beide Ablagen werden gesehen (die Quelle fehlt nicht)", st.csv_gesehen, std::size_t{2});
        eq("NEU: nur der v6-Stand gilt als gemessen", st.gemessen, std::size_t{1});
        eq("ALT-STAND-BISS: der v5-Stand faellt in teilweise statt in gemessen", st.teilweise, std::size_t{1});
        eq("und er wird BENANNT (format_drift), nicht in einen Sammel-Eimer geworfen", st.format_drift,
           std::size_t{1});
        // Die Gegenprobe zu jeder dieser Zahlen: es lag NICHT an einer der anderen Wachen.
        eq("Gegenprobe: es war KEINE Kopf-Drift (der CSV-Kopf ist identisch)", st.kopf_drift, std::size_t{0});
        eq("Gegenprobe: es war KEINE Zeilen-Abweichung (3 Zeilen, |rows=3)", st.zeilen_abweichung, std::size_t{0});
        eq("Gegenprobe: der Stempel FEHLT nicht (er ist nur alt)", st.ohne_stempel, std::size_t{0});

        // Die Wache TRENNT, sie SPERRT nicht: derselbe Stand mit der aktuellen Marke zaehlt sehr wohl.
        schreibe(dll_v / "stem_v5" / "result.csv.stamp", prefix + ex::kLazyResumeRowsKey + "3\n");
        pl::BinaerStand const st2 = pl::lies_binaer_stand(perm_v, fakten);
        eq("Gegenprobe: mit der AKTUELLEN Marke zaehlt derselbe Stand (die Wache trennt, sie sperrt nicht)",
           st2.gemessen, std::size_t{2});
        eq("und die Drift-Zahl faellt auf 0 zurueck", st2.format_drift, std::size_t{0});

        // DIE GLIED-GRENZE: ein reiner Praefix-Vergleich haette "resume-v60" als "resume-v6" durchgelassen.
        // Die Marke endet am Feld-Trenner, und der Leser prueft das -- sonst waere die Wache bei der naechsten
        // zweistelligen Generation still falsch. (Heute unerreichbar; genau deshalb steht der Biss hier.)
        {
            std::string const prefix_v60 = "resume-v60" + prefix.substr(marke_v6.size());
            schreibe(dll_v / "stem_v5" / "result.csv.stamp", prefix_v60 + ex::kLazyResumeRowsKey + "3\n");
            pl::BinaerStand const st_g = pl::lies_binaer_stand(perm_v, fakten);
            eq("GLIED-GRENZE: 'resume-v60' gilt NICHT als 'resume-v6'", st_g.format_drift, std::size_t{1});
            eq("und zaehlt folglich nicht als gemessen", st_g.gemessen, std::size_t{1});
            schreibe(dll_v / "stem_v5" / "result.csv.stamp", prefix + ex::kLazyResumeRowsKey + "3\n"); // zurueck
        }

        // FAIL-CLOSED der Fakten selbst: wer die Marke nicht mitgibt, bekommt kein "gemessen" geschenkt.
        pl::MessFormatFakten unvollstaendig = fakten;
        unvollstaendig.stamp_format.clear();
        check("unvollstaendige Fakten sagen es selbst", !unvollstaendig.vollstaendig());
        pl::BinaerStand const st3 = pl::lies_binaer_stand(perm_v, unvollstaendig);
        eq("FAIL-CLOSED: ohne Marke urteilt der Leser NICHT 'gemessen'", st3.gemessen, std::size_t{0});
        eq("sondern meldet beide Zellen als teilweise/format_drift", st3.format_drift, std::size_t{2});
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
        b.fenster_start                = 0;
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

        std::string const gesamt = zeile_mit(t, "[status-gesamt]");
        std::string const z_p0   = zeile_mit(t, "[status-zelle]", " perm=0 ");
        std::string const z_p1   = zeile_mit(t, "[status-zelle]", " perm=1 ");
        check("Gesamt-Zeile vorhanden", !gesamt.empty());
        check("Zell-Zeilen beider Perms vorhanden", !z_p0.empty() && !z_p1.empty());
        check("Gesamt: gebaut=3", enthaelt(gesamt, " gebaut=3 "));
        check("Gesamt: gemessen=2", enthaelt(gesamt, " gemessen=2 "));
        // H2: 14 = 16 SOLL minus die 2 Messungen DIESER Zelle -> gehoert in die ZELL-Zeile.
        // 30 = 14 (perm0) + 16 (perm1, ohne Verzeichnis) -> die SUMME gehoert in die Gesamt-Zeile.
        check("Zell-Zeile perm0: offen=14 (16 SOLL minus die 2 eigenen Messungen)", enthaelt(z_p0, " offen=14\n"));
        check("Zell-Zeile perm1: offen=16 (dort ist noch nichts gemessen)", enthaelt(z_p1, " offen=16\n"));
        check("Gesamt: offen=30 == 14+16 (SUMME der Zell-Offenstaende, H2)", enthaelt(gesamt, " offen=30\n"));
        check("ALT-STAND-BISS: die Gesamt-Zeile traegt NICHT 14 (ein count minus ALLER Messungen)",
              !enthaelt(gesamt, " offen=14\n"));
        check("Gesamt: alle Zellen liegen im Fenster (fremdfenster=0)", enthaelt(gesamt, " fremdfenster=0 "));
        check("Gesamt: keine Zelle wurde breiten-gekappt", enthaelt(gesamt, " scan_gekappt_zellen=0 "));
        check("Zell-Zeile weist die Fenster-Zugehoerigkeit aus", enthaelt(z_p0, " im_fenster=ja "));
        check("Cursor-Zeile weist abgebrochene Zeilen aus", enthaelt(t, " abgebrochene_zeile=0\n"));
        check("ceb= ist ein EIGENES Feld (Layer nie verschmolzen)", t.find("ceb=[all] zelle=[") != std::string::npos);
        check("zweite Perm ohne Verzeichnis meldet perm_dir=fehlt", t.find("perm_dir=fehlt") != std::string::npos);
        check("Cursor-Zeile der ersten Perm meldet done=ja", t.find("done=ja") != std::string::npos);
        check("Cursor-Zeile der zweiten Perm meldet den Sentinel", t.find("letzte_perm=unbelegt") != std::string::npos);
        // B1: die Fenster-Treue steht auch auf der CURSOR-Zeile, und done_fremd bleibt hier leer.
        std::string const c_p0 = zeile_mit(t, "[status-cursor]", " perm=0 ");
        std::string const c_p1 = zeile_mit(t, "[status-cursor]", " perm=1 ");
        check("Cursor-Zeile traegt die Fenster-Zugehoerigkeit", enthaelt(c_p0, " im_fenster=ja "));
        check("eine Fenster-eigene Cursor-Zeile traegt done=ja UND done_fremd=unbelegt",
              enthaelt(c_p0, " done=ja ") && enthaelt(c_p0, " done_fremd=unbelegt "));
        check("die Cursor-Zeile ohne Datei traegt beide done-Felder als Sentinel",
              enthaelt(c_p1, " done=unbelegt ") && enthaelt(c_p1, " done_fremd=unbelegt "));
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

    // ============================================================================================
    // (7) H2 -- 2-ZELLEN-BILANZ an einer Flaeche, in der BEIDE Zellen gleich weit sind
    //
    // BISS AM ALT-STAND: der Alt-Renderer bildete die Gesamt-Zeile als "EIN fenster_count minus die Summe
    // ALLER Messungen". Bei 2 Zellen x 16 SOLL und je 1 Messung meldete er offen=14 -- eine Zahl, die
    // KLEINER ist als der Offenstand EINER einzigen Zelle (15). Der Fehler waechst mit jeder weiteren Zelle
    // und faellt am Ende auf 0, also genau dann, wenn noch fast alles offen ist.
    // ============================================================================================
    std::cout << "\n-- (7) H2: Gesamt-Bilanz == SUMME der Zell-Offenstaende (2-Zellen-Fall) --\n";
    {
        fs::path const b_root = wurzel / "bilanz";
        lege_gemessenes_binary(b_root / "_all_" / "perm0" / "e4_xml" / "dll" / "stem_a", prefix, 3);
        lege_gemessenes_binary(b_root / "_all_" / "perm1" / "e4_xml" / "dll" / "stem_a", prefix, 4);

        pl::StatusBericht b{};
        b.planer_stempel  = "planer-test";
        b.root            = b_root;
        b.fenster         = "0:16";
        b.fenster_bekannt = true;
        b.fenster_start   = 0;
        b.fenster_count   = 16;
        b.soll.erhoben    = true;
        b.soll.zellen.push_back(zelle("[all]", "_all_", 0, "[O3,no_extension][search_algo,mapping,filter]", 2));
        b.soll.zellen.push_back(zelle("[all]", "_all_", 1, "[O3,no_extension][search_algo,mapping,queuing]", 2));
        pl::erhebe_zellen(b, fakten);
        std::ostringstream os;
        pl::render_status(b, os);
        std::string const t      = os.str();
        std::string const gesamt = zeile_mit(t, "[status-gesamt]");
        std::cout << "----- Bericht (2-Zellen-Bilanz) -----\n" << t << "-------------------------------------\n";

        eq("beide Zellen sind erhoben", b.zellen.size(), std::size_t{2});
        check("Zell-Zeile perm0: offen=15", enthaelt(zeile_mit(t, "[status-zelle]", " perm=0 "), " offen=15\n"));
        check("Zell-Zeile perm1: offen=15", enthaelt(zeile_mit(t, "[status-zelle]", " perm=1 "), " offen=15\n"));
        check("Gesamt: gemessen=2 (je Zelle eine)", enthaelt(gesamt, " gemessen=2 "));
        check("Gesamt: offen=30 == 15+15", enthaelt(gesamt, " offen=30\n"));
        check("ALT-STAND-BISS: NICHT offen=14 (16 minus alle 2 Messungen)", !enthaelt(gesamt, " offen=14\n"));
        check("ALT-STAND-BISS: die Gesamt-Zahl ist nie kleiner als der groesste Zell-Offenstand",
              !enthaelt(gesamt, " offen=15\n"));
    }

    // ============================================================================================
    // (8) H1 -- FREMD-FENSTER: der START aus START:COUNT filtert die Erhebung
    //
    // BISS AM ALT-STAND: der Alt-Aggregator erhob JEDES perm<idx>-Verzeichnis des Plans, gleich zu welchem
    // Chunk es gehoerte. Ein Mess-Baum, in dem ein FRUEHERES Fenster (perm7) schon drei fertige Messungen
    // liegen hat, meldete diese als Fortschritt des AKTUELLEN Fensters 0:2 -- gemessen=4 bei einem SOLL von
    // 2, also offen=0 (auf 0 geklemmt), obwohl im aktuellen Fenster noch 3 Binaries offen sind.
    // ============================================================================================
    std::cout << "\n-- (8) H1: Fremd-Fenster zaehlen NICHT in die Fenster-Bilanz --\n";
    {
        fs::path const f_root = wurzel / "fensterfilter";
        lege_gemessenes_binary(f_root / "_all_" / "perm0" / "e4_xml" / "dll" / "stem_a", prefix, 3);
        // perm1 liegt im Fenster, hat aber noch nichts gemessen -- nur ein laufender Cursor.
        schreibe(f_root / "_all_" / "perm1" / "progress.cursor", "[progress] perm=0 axes_changed=1 2->1\n");
        // perm7 ist ein FREMDES Fenster: drei fertige Messungen eines frueheren Chunks.
        lege_gemessenes_binary(f_root / "_all_" / "perm7" / "e4_xml" / "dll" / "stem_a", prefix, 3);
        lege_gemessenes_binary(f_root / "_all_" / "perm7" / "e4_xml" / "dll" / "stem_b", prefix, 4);
        lege_gemessenes_binary(f_root / "_all_" / "perm7" / "e4_xml" / "dll" / "stem_c", prefix, 5);

        auto baue = [&](std::size_t start, std::size_t count) {
            pl::StatusBericht b{};
            b.planer_stempel  = "planer-test";
            b.root            = f_root;
            b.fenster         = std::to_string(start) + ":" + std::to_string(count);
            b.fenster_bekannt = count > 0;
            b.fenster_start   = start;
            b.fenster_count   = count;
            b.soll.erhoben    = true;
            b.soll.zellen.push_back(zelle("[all]", "_all_", 0, "[O3,no_extension][a1,b1,c1]", 2));
            b.soll.zellen.push_back(zelle("[all]", "_all_", 1, "[O3,no_extension][a2,b2,c2]", 2));
            b.soll.zellen.push_back(zelle("[all]", "_all_", 7, "[O3,no_extension][a3,b3,c3]", 2));
            pl::erhebe_zellen(b, fakten);
            std::ostringstream os;
            pl::render_status(b, os);
            return os.str();
        };

        std::string const t      = baue(/*start=*/0, /*count=*/2);
        std::string const gesamt = zeile_mit(t, "[status-gesamt]");
        std::string const fremd  = zeile_mit(t, "[status-fremdfenster]");
        std::cout << "----- Bericht (Fenster 0:2, Fremd-Fenster perm7) -----\n"
                  << t << "-----------------------------------------------------\n";

        check("perm7 wird als NICHT im Fenster ausgewiesen",
              enthaelt(zeile_mit(t, "[status-zelle]", " perm=7 "), " im_fenster=nein "));
        check("perm0 liegt im Fenster", enthaelt(zeile_mit(t, "[status-zelle]", " perm=0 "), " im_fenster=ja "));
        check("die Fremd-Fenster-Zelle traegt KEINE erfundene offen-Zahl",
              enthaelt(zeile_mit(t, "[status-zelle]", " perm=7 "), " offen=unbelegt\n"));
        check("ALT-STAND-BISS: Gesamt gemessen=1 (nur das aktuelle Fenster), nicht 4",
              enthaelt(gesamt, " gemessen=1 ") && !enthaelt(gesamt, " gemessen=4 "));
        check("ALT-STAND-BISS: offen=3 == (2-1)+(2-0), nicht das geklemmte 0",
              enthaelt(gesamt, " offen=3\n") && !enthaelt(gesamt, " offen=0\n"));
        check("Gesamt weist die Fremd-Fenster-Zahl aus", enthaelt(gesamt, " fremdfenster=1 "));
        check("Gesamt weist die Fenster-Zell-Zahl aus", enthaelt(gesamt, " fenster_zellen=2 "));
        check("eigene [status-fremdfenster]-Zeile statt Verschmelzung", !fremd.empty());
        check("die Fremd-Fenster-Zeile traegt deren 3 Messungen",
              enthaelt(fremd, " zellen=1 ") && enthaelt(fremd, " gemessen=3 "));

        // START != 0: das Fenster wandert, und mit ihm die Zugehoerigkeit -- perm0 wird selbst fremd.
        std::string const t2      = baue(/*start=*/1, /*count=*/2);
        std::string const gesamt2 = zeile_mit(t2, "[status-gesamt]");
        check("Fenster 1:2 -> perm0 ist jetzt FREMD",
              enthaelt(zeile_mit(t2, "[status-zelle]", " perm=0 "), " im_fenster=nein "));
        check("Fenster 1:2 -> perm1 bleibt drin",
              enthaelt(zeile_mit(t2, "[status-zelle]", " perm=1 "), " im_fenster=ja "));
        check("Fenster 1:2 -> zwei Fremd-Fenster-Zellen (perm0 + perm7)", enthaelt(gesamt2, " fremdfenster=2 "));
        check("Fenster 1:2 -> im Fenster ist NICHTS gemessen", enthaelt(gesamt2, " gemessen=0 "));
        check("Fenster 1:2 -> offen=2 (die eine Fenster-Zelle mit SOLL 2)", enthaelt(gesamt2, " offen=2\n"));
        check("Fenster 1:2 -> die 4 Fremd-Messungen stehen in der eigenen Zeile",
              enthaelt(zeile_mit(t2, "[status-fremdfenster]"), " gemessen=4 "));

        // count == 0 deaktiviert das Fenster: dann gibt es keinen Filter und kein Binary-SOLL.
        std::string const t3 = baue(/*start=*/0, /*count=*/0);
        check("Fenster deaktiviert -> keine Zelle ist fremd",
              enthaelt(zeile_mit(t3, "[status-gesamt]"), " fremdfenster=0 "));
        check("Fenster deaktiviert -> offen bleibt der Sentinel",
              enthaelt(zeile_mit(t3, "[status-gesamt]"), " offen=unbelegt\n"));
    }

    // ============================================================================================
    // (9) M4 -- eine Transport-AUSNAHME ist BERICHTS-Inhalt, keine Konfig-Fehler-Klasse
    //     (der Wurf selbst liegt im App-Host; hier wird die Berichts-FORM festgenagelt -- kein Netz)
    // ============================================================================================
    std::cout << "\n-- (9) M4: Bestandslog-Transportfehler als eigene Berichts-Zeile --\n";
    {
        pl::StatusBericht b{};
        b.root           = root;
        b.soll.erhoben   = true;
        b.bestand.aktiv  = true;
        b.bestand.fehler = true;
        b.bestand.grund  = "minio: connection refused";
        pl::erhebe_zellen(b, fakten);
        std::ostringstream os;
        pl::render_status(b, os);
        std::string const t = os.str();
        check("die Fehler-Zeile steht literal im Bericht",
              enthaelt(t, "[status-bestand] quelle=fehler (minio: connection refused)\n"));
        check("der Transportfehler wird NICHT als blosses 'keine Daten' verschluckt",
              !enthaelt(t, "[status] quelle=bestandslog keine Daten"));
    }

    // ============================================================================================
    // (10) B1 -- der FREMD-FENSTER-CURSOR darf kein Fertig-Signal DIESES Fensters vortaeuschen
    //
    // BISS AM ALT-STAND: die [status-cursor]-Zeile druckte `fenster=<aktuelles>` und `done=ja` nebeneinander,
    // ohne zu sagen, zu welchem Fenster der Cursor gehoert. An einer Perm aus einem FRUEHEREN Chunk stand
    // damit woertlich "fenster=0:2 ... done=ja", obwohl das done aus Fenster 7:x stammt -- genau das Signal,
    // an dem der kuenftige 38.b-Takt die naechste CEB losziehen soll, waehrend 0:2 noch misst.
    // ============================================================================================
    std::cout << "\n-- (10) B1: Fremd-Fenster-Cursor traegt im_fenster=nein und KEIN done=ja --\n";
    {
        fs::path const c_root = wurzel / "fremdcursor";
        // perm0 liegt im Fenster 0:2 und laeuft noch (kein done).
        schreibe(c_root / "_all_" / "perm0" / "progress.cursor", "[progress] perm=0 axes_changed=1 2->1\n");
        // perm7 gehoert einem FRUEHEREN Fenster und hat dort fertig gemeldet.
        schreibe(c_root / "_all_" / "perm7" / "progress.cursor", "[progress] perm=1 axes_changed=1 0->1\n"
                                                                 "[progress] done perm=3 window-complete\n");

        pl::StatusBericht b{};
        b.planer_stempel  = "planer-test";
        b.root            = c_root;
        b.fenster         = "0:2";
        b.fenster_bekannt = true;
        b.fenster_start   = 0;
        b.fenster_count   = 2;
        b.soll.erhoben    = true;
        b.soll.zellen.push_back(zelle("[all]", "_all_", 0, "[O3,no_extension][a1,b1,c1]", 2));
        b.soll.zellen.push_back(zelle("[all]", "_all_", 7, "[O3,no_extension][a3,b3,c3]", 2));
        pl::erhebe_zellen(b, fakten);
        std::ostringstream os;
        pl::render_status(b, os);
        std::string const t  = os.str();
        std::string const c0 = zeile_mit(t, "[status-cursor]", " perm=0 ");
        std::string const c7 = zeile_mit(t, "[status-cursor]", " perm=7 ");
        std::cout << "----- Bericht (Fremd-Fenster-Cursor) -----\n"
                  << t << "------------------------------------------\n";

        check("beide Cursor-Zeilen vorhanden", !c0.empty() && !c7.empty());
        check("ALT-STAND-BISS: die Fremd-Cursor-Zeile weist im_fenster=nein aus", enthaelt(c7, " im_fenster=nein "));
        check("ALT-STAND-BISS: die Fremd-Cursor-Zeile traegt KEIN done=ja", !enthaelt(c7, " done=ja "));
        check("die Fremd-Cursor-Zeile traegt done=unbelegt (kein Fertig-Signal DIESES Fensters)",
              enthaelt(c7, " done=unbelegt "));
        check("das fremde Fertig-Signal geht nicht verloren: done_fremd=ja", enthaelt(c7, " done_fremd=ja "));
        check("die Fremd-Cursor-Zeile behaelt ihre ehrliche Historie (zeilen_done=1)", enthaelt(c7, " zeilen_done=1 "));
        check("die eigene Cursor-Zeile weist im_fenster=ja aus", enthaelt(c0, " im_fenster=ja "));
        check("die eigene Cursor-Zeile meldet done=nein (das Fenster laeuft)", enthaelt(c0, " done=nein "));
        check("die eigene Cursor-Zeile traegt done_fremd=unbelegt", enthaelt(c0, " done_fremd=unbelegt "));
        // Gegenprobe ueber den GANZEN Bericht: in diesem Stand darf nirgends ein done=ja stehen.
        check("ALT-STAND-BISS: der ganze Bericht traegt kein done=ja (nichts ist in 0:2 fertig)",
              !enthaelt(t, " done=ja "));

        // Wandert das Fenster auf 7:2, dreht sich die Zugehoerigkeit -- und das done wird zum EIGENEN.
        b.fenster       = "7:2";
        b.fenster_start = 7;
        pl::erhebe_zellen(b, fakten);
        std::ostringstream os2;
        pl::render_status(b, os2);
        std::string const t2 = os2.str();
        check("Fenster 7:2 -> perm7 ist jetzt das EIGENE Fenster und meldet done=ja",
              enthaelt(zeile_mit(t2, "[status-cursor]", " perm=7 "), " done=ja "));
        check("Fenster 7:2 -> perm0 ist jetzt fremd und meldet done_fremd=nein",
              enthaelt(zeile_mit(t2, "[status-cursor]", " perm=0 "), " done_fremd=nein "));
    }

    // ============================================================================================
    // (11) B2 -- eine LEERE Summe ist keine Bilanz
    //
    // BISS AM ALT-STAND: gesamt_offen_feld summierte ueber die Zell-Liste. Ist sie leer (Plan nicht erhoben,
    // Walk ohne Zelle, oder ALLE Zellen fremd), lieferte die Schleife 0 -- und `offen=0` heisst "nichts mehr
    // offen". Der Bericht behauptete damit Fertigkeit an genau der Stelle, an der er NICHTS weiss.
    // ============================================================================================
    std::cout << "\n-- (11) B2: ohne Grundlage traegt offen= den Sentinel, nicht 0 --\n";
    {
        auto mit_fenster = [&](bool erhoben, bool mit_zelle, std::size_t start) {
            pl::StatusBericht b{};
            b.planer_stempel  = "planer-test";
            b.root            = wurzel / "gibt_es_nicht";
            b.fenster         = std::to_string(start) + ":16";
            b.fenster_bekannt = true;
            b.fenster_start   = start;
            b.fenster_count   = 16;
            b.soll.erhoben    = erhoben;
            if (!erhoben) b.soll.grund = "Profil 'x.xml' nicht als bekannte Wurzel lesbar (rc 5)";
            if (mit_zelle) b.soll.zellen.push_back(zelle("[all]", "_all_", 0, "[O3,no_extension][a,b,c]", 2));
            pl::erhebe_zellen(b, fakten);
            std::ostringstream os;
            pl::render_status(b, os);
            return os.str();
        };

        std::string const t_nicht_erhoben = mit_fenster(/*erhoben=*/false, /*mit_zelle=*/false, /*start=*/0);
        std::string const g1              = zeile_mit(t_nicht_erhoben, "[status-gesamt]");
        std::cout << "----- Bericht (Plan nicht erhoben, Fenster 0:16) -----\n"
                  << t_nicht_erhoben << "-----------------------------------------------------\n";
        check("Plan nicht erhoben -> plan=nicht_erhoben steht literal",
              enthaelt(t_nicht_erhoben, "plan=nicht_erhoben"));
        check("ALT-STAND-BISS: offen=unbelegt statt offen=0", enthaelt(g1, " offen=unbelegt\n"));
        check("ALT-STAND-BISS: die Gesamt-Zeile behauptet NICHT offen=0", !enthaelt(g1, " offen=0\n"));

        std::string const t_leerer_walk = mit_fenster(/*erhoben=*/true, /*mit_zelle=*/false, /*start=*/0);
        std::string const g2            = zeile_mit(t_leerer_walk, "[status-gesamt]");
        check("erhobener Plan OHNE Zelle -> auch hier offen=unbelegt", enthaelt(g2, " offen=unbelegt\n"));
        check("ALT-STAND-BISS: auch der leere Walk behauptet NICHT offen=0", !enthaelt(g2, " offen=0\n"));
        check("die 'keine Zelle'-Aussage steht weiterhin literal da",
              enthaelt(t_leerer_walk, "[status] quelle=plan keine Daten (der Walk lieferte keine Zelle)"));

        // ALLE Zellen fremd: die Summe laeuft ueber nichts -- dieselbe Klasse, andere Ursache.
        std::string const t_alle_fremd = mit_fenster(/*erhoben=*/true, /*mit_zelle=*/true, /*start=*/5);
        std::string const g3           = zeile_mit(t_alle_fremd, "[status-gesamt]");
        check("alle Zellen fremd -> fenster_zellen=0", enthaelt(g3, " fenster_zellen=0 "));
        check("ALT-STAND-BISS: alle Zellen fremd -> offen=unbelegt statt offen=0", enthaelt(g3, " offen=unbelegt\n"));

        // GEGENPROBE: sobald EINE Zelle im Fenster liegt, steht wieder eine Zahl da.
        std::string const t_mit_zelle = mit_fenster(/*erhoben=*/true, /*mit_zelle=*/true, /*start=*/0);
        check("Gegenprobe: eine Fenster-Zelle -> offen=16 (die Zahl kehrt zurueck)",
              enthaelt(zeile_mit(t_mit_zelle, "[status-gesamt]"), " offen=16\n"));
    }

    // ============================================================================================
    // (12) B3 -- die Summe der Zell-Offenstaende saettigt statt umzuklappen
    //
    // BISS AM ALT-STAND: `summe += zell_offen(...)` ueber std::size_t. Bei zwei Zellen mit je SIZE_MAX
    // offenen Binaries klappte die Summe auf SIZE_MAX-1 um -- eine Zahl, die wie eine gueltige Bilanz aussieht
    // und KLEINER ist als der Offenstand einer EINZIGEN Zelle. Die Fehlerrichtung ist wieder "zu wenig offen".
    // ============================================================================================
    std::cout << "\n-- (12) B3: Ueberlauf der Gesamt-Summe wird als solcher ausgewiesen --\n";
    {
        constexpr std::size_t kMax = (std::numeric_limits<std::size_t>::max)();

        pl::StatusBericht b{};
        b.planer_stempel  = "planer-test";
        b.root            = wurzel / "gibt_es_nicht";
        b.fenster         = "0:" + std::to_string(kMax);
        b.fenster_bekannt = true;
        b.fenster_start   = 0;
        b.fenster_count   = kMax; // jede Zelle hat SIZE_MAX offene Binaries
        b.soll.erhoben    = true;
        b.soll.zellen.push_back(zelle("[all]", "_all_", 0, "[O3,no_extension][a1,b1,c1]", 2));
        b.soll.zellen.push_back(zelle("[all]", "_all_", 1, "[O3,no_extension][a2,b2,c2]", 2));
        pl::erhebe_zellen(b, fakten);
        std::ostringstream os;
        pl::render_status(b, os);
        std::string const t      = os.str();
        std::string const gesamt = zeile_mit(t, "[status-gesamt]");

        eq("die Zell-Zeile traegt weiterhin den echten Einzel-Offenstand", pl::offen_feld(b, /*gemessen=*/0),
           std::to_string(kMax));
        check("ALT-STAND-BISS: die Gesamt-Zeile traegt offen=uebergelaufen",
              enthaelt(gesamt, " offen=uebergelaufen\n"));
        check("ALT-STAND-BISS: sie traegt NICHT die umgeklappte Summe SIZE_MAX-1",
              !enthaelt(gesamt, " offen=" + std::to_string(kMax - 1) + "\n"));
        check("der Ueberlauf-Sentinel ist NICHT derselbe wie 'unbelegt'", !enthaelt(gesamt, " offen=unbelegt\n"));

        // GEGENPROBE: eine EINZELNE Zelle mit SIZE_MAX laeuft nicht ueber und meldet die echte Zahl.
        b.soll.zellen.pop_back();
        pl::erhebe_zellen(b, fakten);
        std::ostringstream os2;
        pl::render_status(b, os2);
        check("Gegenprobe: eine Zelle mit SIZE_MAX ist kein Ueberlauf",
              enthaelt(zeile_mit(os2.str(), "[status-gesamt]"), " offen=" + std::to_string(kMax) + "\n"));
    }

    fs::remove_all(wurzel, ec);
    std::cout << "\n==== W5 status-Rueck-Leser: " << (g_fail == 0 ? "ALLE OK" : "FEHLER") << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
