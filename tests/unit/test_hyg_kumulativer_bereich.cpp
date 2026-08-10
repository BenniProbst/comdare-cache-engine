// test_hyg_kumulativer_bereich -- Posten #48 (2026-08-10): DIE KUMULATIVE ACHSE DER DIFF-HYGIENE-WACHE.
//
// WAS HIER GEPRUEFT WIRD -- und warum es diesen Test ueberhaupt gibt:
//
//   scripts/ci_diff_ascii_width_guard.sh urteilt ueber einen DIFF-BEREICH. Welchen, sagt ihr
//   der Aufrufer. Die CI sagt ihr CI_COMMIT_BEFORE_SHA..HEAD -- also den PUSH. Damit
//   beantwortet die Wache die Frage "ist DIESER Push sauber?" und nur die. Die Frage, die vor
//   einem main-Fast-Forward zaehlt, ist eine andere: "ist der STAND sauber, der nach main
//   geht?" Beide Fragen sind legitim; bis heute wurde nur die erste gestellt.
//
//   Das ist die Fehlerklasse: nicht ein falsches Messgeraet, sondern ein richtiges Messgeraet
//   am zu kleinen GEGENSTAND. Solange in kleinen Schritten gepusht wird, meldet es gruen. Der
//   main-FF ist das ERSTE kumulative Gate und traegt den ganzen aufgelaufenen Bereich auf
//   einmal.
//
// AM OBJEKT BELEGT (2026-08-09, DERSELBE Baum-SHA aebc4f2c in beiden Laeufen):
//   ce-Pipeline 15498  development  Bereich 2eb310ae..HEAD    1 Commit    1 Zusatzzeile   GRUEN
//   ce-Pipeline 15501  main         Bereich 8fcf0c0e..HEAD   86 Commits  18577 Zusatzz.   ROT
//   58 Verstoesse (37 Nicht-ASCII, 21 ueber 120 Spalten) in zwoelf Dateien -- kein einziger
//   davon war je in einem Push sichtbar. Am 2026-08-10 auf diesem Repo nachgerechnet:
//   `sh scripts/ci_diff_ascii_width_guard.sh 8fcf0c0e..aebc4f2c` -> Exit 1, 37 + 21 = 58.
//
// WARUM EIN GOOGLE TEST UND KEINE WEITERE SHELL-PROBE (Owner-Entscheid 2026-08-09):
//   "Ich sehe einen Haufen shells statt vernuenftiger google tests, was soll das? Es waere
//    sauberer im cmake-Debug Modus standard google Tests zu fahren und diese in Release zu
//    wiederholen aufgrund von compile regressionen. SKRIPTE SAGEN GAR NICHTS."
//   Die Wache selbst bleibt vorerst ein sh-Skript (ihr Umbau nach C++ ist ein eigenes Paket) --
//   der BEWEIS ihres Bisses liegt ab hier hier, als ctest-Ziel, in Debug wie in Release, ohne
//   eigene Sonder-Ausfuehrung im CI. Die Shell-Menge waechst durch dieses Paket NICHT.
//
// K13 -- DER KOEDER MUSS ERST BEISSEN UND DEN RICHTIGEN RISS ZEIGEN:
//   * Jeder Fall wuerfelt seine Kennung FRISCH aus /dev/urandom. Kein Wert ist abgeschrieben;
//     taucht er in der Ausgabe der Wache auf, kann er nur aus DEM Wegwerf-Repo stammen, das
//     dieser Fall gerade gebaut hat.
//   * Der Koeder liegt im BEREICH, NICHT IM PUSH -- das ist der ganze Punkt. Der letzte
//     Commit ist sauber. Ein Koeder im letzten Commit haette auch die alte, push-lokale
//     Messung gefangen und ueber die kumulative Achse nichts bewiesen.
//   * Zu jeder Zusicherung ein Gegeneingang (T-4): derselbe Baum, derselbe Ablauf, nur ohne
//     Koeder, muss GRUEN bleiben -- sonst waere das Rot eine Konstante des Modus.
//
// T-3 NENNER, FREMD: die Erwartungswerte (Abzweigung, Endpunkt-SHAs, Commit-Zahl, Zahl der
// hinzugefuegten Zeilen) rechnet dieser Test SELBST mit git aus -- also aus einer anderen
// Quelle als dem Pruefling. Die Wache muss sie treffen, nicht bloss irgendeine Zahl drucken.
//
// GEGENPROBE GEGEN DEN EIGENEN MUTANTEN: COMDARE_HYG_WACHE_PFAD verschiebt den Prueflings-
// Pfad. Damit laesst sich dieselbe Test-Binary gegen eine praeparierte Wache fahren (z.B. den
// Stand VOR diesem Paket, der --bereich gar nicht kennt); sie MUSS dann rot werden. Welcher
// Prueflig gefahren wurde, steht in der Ausgabe jedes Falls.
//
// GRENZE, EHRLICH BENANNT: dieser Test prueft das URTEIL der Wache ueber praeparierte
// Wegwerf-Repos. Dass die CI ihr im echten Lauf die RICHTIGEN Refs uebergibt (origin/main
// gegen HEAD, in einem Klon, der beide kennt), kann er nicht sehen -- das haelt die
// Verdrahtung in .gitlab-ci.yml, und die ist von hier aus ungedeckt.
//
// ASCII-only (Leitplanke).

#include "comdare_test_tmp.hpp"

#include <gtest/gtest.h>

// Die Wache ist ein POSIX-sh-Skript und faehrt in keinem Windows-Job. Der ctest-Eintrag
// entsteht trotzdem UNBEDINGT -- ein bedingter Registrierungs-Block waere in genau diesem
// Paket die falsche Antwort. Auf Windows meldet der Fall sich als SKIP.
#if defined(_WIN32)

TEST(HygKumulativerBereich, NurPosix) {
    GTEST_SKIP() << "scripts/ci_diff_ascii_width_guard.sh ist ein POSIX-sh-Skript "
                    "(kein Windows-Job faehrt es).";
}

#else

#include <sys/wait.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace {

// ---------------------------------------------------------------------------
// Der Prueflig: die ECHTE Wache, es sei denn, jemand schiebt bewusst eine andere unter.
// ---------------------------------------------------------------------------
[[nodiscard]] std::string wachen_quelle() {
    if (char const* const ueberschrieben = std::getenv("COMDARE_HYG_WACHE_PFAD");
        ueberschrieben != nullptr && *ueberschrieben != '\0') {
        return std::string{ueberschrieben};
    }
    return std::string{COMDARE_HYG_GUARD_SH};
}

// ---------------------------------------------------------------------------
// FRISCH GEWUERFELT (K13). /dev/urandom, nicht std::random_device und erst recht keine
// Konstante aus einer Doku: der Wert darf in KEINER Datei dieses Repos vorkommen, sonst
// koennte die Ausgabe der Wache ihn aus einer anderen Quelle haben als aus unserem Repo.
// ---------------------------------------------------------------------------
[[nodiscard]] std::string koeder_marke() {
    std::ifstream quelle{"/dev/urandom", std::ios::binary};
    EXPECT_TRUE(quelle.good()) << "/dev/urandom nicht lesbar -- ohne frischen Koeder kein Beweis";
    unsigned char rohbytes[6]{};
    quelle.read(reinterpret_cast<char*>(rohbytes), sizeof rohbytes);
    static constexpr char kZiffern[] = "0123456789abcdef";
    std::string           marke;
    for (unsigned char const b : rohbytes) {
        marke.push_back(kZiffern[(b >> 4U) & 0x0FU]);
        marke.push_back(kZiffern[b & 0x0FU]);
    }
    return marke;
}

struct Lauf {
    int         code{-1};
    std::string ausgabe;
};

[[nodiscard]] Lauf schale(std::string const& befehl) {
    Lauf  ergebnis;
    FILE* rohr = ::popen(befehl.c_str(), "r");
    if (rohr == nullptr) {
        ADD_FAILURE() << "popen fehlgeschlagen: " << befehl;
        return ergebnis;
    }
    char puffer[4096];
    while (std::fgets(puffer, sizeof puffer, rohr) != nullptr) { ergebnis.ausgabe += puffer; }
    int const status = ::pclose(rohr);
    ergebnis.code    = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    return ergebnis;
}

[[nodiscard]] bool enthaelt(std::string const& heuhaufen, std::string_view nadel) {
    return heuhaufen.find(nadel) != std::string::npos;
}

[[nodiscard]] std::string ohne_zeilenende(std::string s) {
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) { s.pop_back(); }
    return s;
}

// ---------------------------------------------------------------------------
// Ein Wegwerf-Repo mit ZWEI Zweigen. Die Wache loest ihre Repo-Wurzel aus dem eigenen
// Skript-Pfad auf (dirname/..), deshalb wird sie in <repo>/scripts/ hineinkopiert. Der Test
// misst damit BYTEGLEICH dieselbe Datei, die im Repo liegt -- nur an einem Ort, an dem sie
// ueber das praeparierte Repo urteilt statt ueber dieses hier.
// ---------------------------------------------------------------------------
class WegwerfRepo {
public:
    explicit WegwerfRepo(std::string const& marke) : wurzel_{comdare::test::user_tmp_dir() / ("hyg_kum_" + marke)} {
        std::error_code ec;
        fs::remove_all(wurzel_, ec);
        fs::create_directories(wurzel_ / "scripts", ec);

        std::error_code kopier_ec;
        fs::copy_file(fs::path{wachen_quelle()}, wurzel_ / "scripts" / "ci_diff_ascii_width_guard.sh",
                      fs::copy_options::overwrite_existing, kopier_ec);
        EXPECT_FALSE(kopier_ec) << "Die Wache liess sich nicht in das Wegwerf-Repo kopieren: " << wachen_quelle()
                                << " -- " << kopier_ec.message();

        // Kein --global, keine Signatur, keine Hooks: das Repo darf nichts von der Umgebung erben.
        git("init -q -b main .");
        git("config user.email wegwerf@example.invalid");
        git("config user.name Wegwerf");
        git("config commit.gpgsign false");
        git("add scripts/ci_diff_ascii_width_guard.sh");
        git("-c core.hooksPath=/dev/null commit -q -m basis-wache");
    }

    WegwerfRepo(WegwerfRepo const&)            = delete;
    WegwerfRepo& operator=(WegwerfRepo const&) = delete;
    WegwerfRepo(WegwerfRepo&&)                 = delete;
    WegwerfRepo& operator=(WegwerfRepo&&)      = delete;

    ~WegwerfRepo() {
        std::error_code ec;
        fs::remove_all(wurzel_, ec);
    }

    // Ein git-Aufruf IM Repo. Der Rueckgabewert ist die reine Ausgabe -- Fehler fallen als
    // leere Antwort bzw. als fehlgeschlagene Erwartung im Fall selbst auf.
    std::string git(std::string const& argumente) const {
        Lauf const lauf = schale("git -C \"" + wurzel_.string() + "\" " + argumente + " 2>&1");
        return ohne_zeilenende(lauf.ausgabe);
    }

    void datei_anhaengen(std::string const& name, std::string const& zeile) const {
        std::ofstream aus{wurzel_ / name, std::ios::app};
        aus << zeile << "\n";
    }

    void datei_schreiben(std::string const& name, std::string const& inhalt) const {
        std::ofstream aus{wurzel_ / name};
        aus << inhalt;
    }

    void committen(std::string const& name, std::string const& nachricht) const {
        git("add -- \"" + name + "\"");
        git("-c core.hooksPath=/dev/null commit -q -m \"" + nachricht + "\"");
    }

    // Die Wache fahren -- IM Repo, ueber die dort liegende Kopie.
    [[nodiscard]] Lauf wache(std::string const& argumente) const {
        return schale("cd \"" + wurzel_.string() + "\" && sh scripts/ci_diff_ascii_width_guard.sh " + argumente +
                      " 2>&1");
    }

    [[nodiscard]] fs::path const& pfad() const { return wurzel_; }

private:
    fs::path wurzel_;
};

// Eine Zeile ueber 120 Byte, deren Fuellung die frisch gewuerfelte Marke traegt.
[[nodiscard]] std::string ueberlange_zeile(std::string const& marke) {
    std::string zeile = "int lang_" + marke + " = 1; // ";
    while (zeile.size() <= 160U) { zeile += marke; }
    return zeile;
}

void lauf_berichten(char const* fall, Lauf const& lauf, std::string const& marke) {
    std::cout << "  [HYG-KUM] Fall '" << fall << "' | Koeder " << marke << " | Prueflig " << wachen_quelle()
              << " | Exit " << lauf.code << "\n";
}

// Zaehlt die hinzugefuegten Zeilen eines Diffs UNABHAENGIG von der Wache (T-3): eigene,
// bewusst simple Zaehlung ueber den Zeilenanfang. Sie ist nicht die Zustandsmaschine der
// Wache -- genau deshalb taugt sie als Gegenquelle. In den Fixtures dieses Tests gibt es
// keine Inhaltszeile, die selbst mit "+++" beginnt; die Vereinfachung ist hier gedeckt.
[[nodiscard]] int zusatzzeilen_selbst_zaehlen(WegwerfRepo const& repo, std::string const& von, std::string const& bis) {
    std::string const roh = repo.git("diff -U0 --no-color --no-ext-diff " + von + " " + bis);
    int               n   = 0;
    std::size_t       pos = 0;
    while (pos <= roh.size()) {
        std::size_t const ende  = roh.find('\n', pos);
        std::string const zeile = roh.substr(pos, (ende == std::string::npos ? roh.size() : ende) - pos);
        if (!zeile.empty() && zeile[0] == '+' && zeile.rfind("+++", 0) != 0) { ++n; }
        if (ende == std::string::npos) { break; }
        pos = ende + 1;
    }
    return n;
}

// Der Standard-Aufbau aller Bereichsfaelle: main, davon abgezweigt development mit ZWEI
// Commits -- der ERSTE traegt (optional) den Koeder, der ZWEITE ist immer sauber. Damit ist
// der letzte Push garantiert sauber und der Bereich garantiert nicht.
void zwei_commits_auf_development(WegwerfRepo const& repo, std::string const& marke, bool mit_koeder,
                                  std::string const& koeder_zeile) {
    repo.datei_schreiben("q.cpp", "int basis_" + marke + " = 0;\n");
    repo.committen("q.cpp", "basis");
    repo.git("branch development");
    repo.git("checkout -q development");
    repo.datei_anhaengen("q.cpp", mit_koeder ? koeder_zeile : ("int harmlos_" + marke + " = 1;"));
    repo.committen("q.cpp", "A-im-bereich");
    repo.datei_anhaengen("q.cpp", "int sauber_" + marke + " = 2;");
    repo.committen("q.cpp", "B-der-letzte-push");
}

} // namespace

// ===========================================================================================
// (1) DER EIGENTLICHE DEFEKT, in einem Fall: derselbe Baum, zwei Bereiche, zwei Urteile.
//     Der Koeder liegt im ERSTEN der beiden development-Commits. Die push-lokale Messung
//     sieht nur den ZWEITEN und meldet GRUEN -- die kumulative sieht beide und meldet ROT.
//     Vor diesem Paket gab es die zweite Messung nicht; --bereich endete mit Exit 2.
// ===========================================================================================
TEST(HygKumulativerBereich, KoederImBereichAberNichtImPushWirdNurKumulativRot) {
    std::string const marke  = koeder_marke();
    std::string const koeder = ueberlange_zeile(marke);
    WegwerfRepo       repo{marke};
    zwei_commits_auf_development(repo, marke, /*mit_koeder=*/true, koeder);

    // Derselbe Baum fuer beide Laeufe -- das ist die Pointe und wird deshalb festgehalten.
    std::string const baum = repo.git("rev-parse development^{tree}");
    ASSERT_FALSE(baum.empty()) << "Der Baum-SHA liess sich nicht bestimmen -- Fixture kaputt.";

    Lauf const push = repo.wache("\"HEAD~1..HEAD\"");
    lauf_berichten("push-lokal (wie die CI heute misst)", push, marke);
    EXPECT_EQ(push.code, 0) << "Der letzte Commit ist sauber -- push-lokal MUSS gruen sein, "
                               "sonst misst dieser Fall etwas anderes als die Luecke.\n"
                            << push.ausgabe;
    EXPECT_TRUE(enthaelt(push.ausgabe, "DIFF-HYGIENE-WACHE: GRUEN")) << push.ausgabe;
    EXPECT_FALSE(enthaelt(push.ausgabe, marke + marke))
        << "Der Koeder darf im PUSH-Bereich gar nicht auftauchen -- er liegt einen Commit frueher.\n"
        << push.ausgabe;

    Lauf const kum = repo.wache("--bereich main development");
    lauf_berichten("kumulativ ueber main..development", kum, marke);
    EXPECT_EQ(kum.code, 1) << "Der kumulative Lauf MUSS mit 1 (Verstoss) enden. Exit 2 hiesse: die Wache "
                              "kennt --bereich nicht -- genau der Stand VOR diesem Paket.\n"
                           << kum.ausgabe;
    EXPECT_TRUE(enthaelt(kum.ausgabe, "DIFF-HYGIENE-WACHE: ROT")) << kum.ausgabe;
    EXPECT_TRUE(enthaelt(kum.ausgabe, ">120-SPALTEN  q.cpp:2"))
        << "Der Verstoss muss mit Datei UND Zeile genannt werden.\n"
        << kum.ausgabe;
    // Nicht bloss "irgendwo steht KUMULATIV" -- das steht schon im Kopf der Ausgabe und war als
    // Zusicherung hohl (von der Mutation M6 am 10.08.2026 am Objekt freigelegt: der Bereich liess
    // sich aus dem VERDIKT streichen, ohne dass dieser Fall es merkte). Geprueft wird die
    // Verdikt-ZEILE selbst.
    EXPECT_TRUE(enthaelt(kum.ausgabe, "DIFF-HYGIENE-WACHE: ROT.  KUMULATIV ueber "))
        << "Das Verdikt selbst muss seinen Modus UND seinen Bereich tragen.\n"
        << kum.ausgabe;

    std::cout << "  [HYG-KUM] BEIDE Laeufe auf demselben Baum " << baum << ": push Exit " << push.code
              << " / kumulativ Exit " << kum.code << "\n";
}

// ===========================================================================================
// (2) K13-GEGENKOEDER: derselbe Ablauf, dieselbe Zweig-Topologie, nur ist die Zeile im ersten
//     Commit kurz und ASCII. Dann MUSS auch der kumulative Lauf gruen sein. Ohne diesen Fall
//     waere (1) auch von einer Wache zu bestehen, die im --bereich-Modus immer rot meldet.
// ===========================================================================================
TEST(HygKumulativerBereich, OhneKoederBleibtDerKumulativeLaufGruen) {
    std::string const marke = koeder_marke();
    WegwerfRepo       repo{marke};
    zwei_commits_auf_development(repo, marke, /*mit_koeder=*/false, "");

    Lauf const kum = repo.wache("--bereich main development");
    lauf_berichten("Gegenkoeder: sauberer Bereich", kum, marke);
    EXPECT_EQ(kum.code, 0) << "Ein sauberer Bereich MUSS gruen sein -- sonst ist der Modus ein Daueralarm.\n"
                           << kum.ausgabe;
    EXPECT_TRUE(enthaelt(kum.ausgabe, "DIFF-HYGIENE-WACHE: GRUEN")) << kum.ausgabe;
    // Gruen mit Nenner 0 waere wertlos: der Lauf muss belegen, dass er ueberhaupt gemessen hat.
    EXPECT_TRUE(enthaelt(kum.ausgabe, "2 Commit(s) im Bereich")) << "Auch das Gruen traegt seinen Nenner.\n"
                                                                 << kum.ausgabe;
    EXPECT_TRUE(enthaelt(kum.ausgabe, "2 Zusatzzeilen in selbst verfasstem Code geprueft")) << kum.ausgabe;
}

// ===========================================================================================
// (3) DIE ZWEITE ACHSE derselben Wache: Nicht-ASCII. Ein Fall, der nur die Breite prueft,
//     liesse die Haelfte der 58 Verstoesse vom 09.08. ungedeckt (37 davon waren Nicht-ASCII).
// ===========================================================================================
TEST(HygKumulativerBereich, NichtAsciiImBereichBeisstEbenfalls) {
    std::string const marke = koeder_marke();
    WegwerfRepo       repo{marke};
    // Ein Em-Dash (U+2014, UTF-8: e2 80 94) -- genau die Klasse, die am 07.08. durch die CI fiel.
    std::string const koeder = std::string{"int dash_"} + marke + " = 1; // \xe2\x80\x94 " + marke;
    zwei_commits_auf_development(repo, marke, /*mit_koeder=*/true, koeder);

    Lauf const push = repo.wache("\"HEAD~1..HEAD\"");
    lauf_berichten("push-lokal (Nicht-ASCII eine Etage tiefer)", push, marke);
    EXPECT_EQ(push.code, 0) << push.ausgabe;

    Lauf const kum = repo.wache("--bereich main development");
    lauf_berichten("kumulativ (Nicht-ASCII)", kum, marke);
    EXPECT_EQ(kum.code, 1) << kum.ausgabe;
    EXPECT_TRUE(enthaelt(kum.ausgabe, "NICHT-ASCII   q.cpp:2")) << kum.ausgabe;
}

// ===========================================================================================
// (4) V-1: DER BEREICH IST TEIL DER AUSSAGE. Alles, was die Wache ueber ihren Bereich druckt,
//     wird hier gegen git NACHGERECHNET -- Abzweigung, beide Endpunkte, Commit-Zahl und die
//     Zahl der hinzugefuegten Zeilen. T-3: die Gegenquelle ist git, nicht der Prueflig.
//     Ohne diesen Fall koennte die Wache irgendeinen Bereich drucken und einen anderen messen.
// ===========================================================================================
TEST(HygKumulativerBereich, BereichStehtInDerAusgabeUndDecktSichMitGit) {
    std::string const marke = koeder_marke();
    WegwerfRepo       repo{marke};
    zwei_commits_auf_development(repo, marke, /*mit_koeder=*/false, "");

    // FREMDER NENNER: selbst gerechnet, nicht von der Wache uebernommen.
    std::string const mb          = repo.git("merge-base main development");
    std::string const basis_sha   = repo.git("rev-parse main^{commit}");
    std::string const spitze_sha  = repo.git("rev-parse development^{commit}");
    std::string const commit_zahl = repo.git("rev-list --count " + mb + ".." + spitze_sha);
    int const         zeilen      = zusatzzeilen_selbst_zaehlen(repo, mb, spitze_sha);

    ASSERT_EQ(mb.size(), 40U) << "merge-base lieferte keinen vollen SHA: '" << mb << "'";
    ASSERT_EQ(commit_zahl, "2") << "Fixture-Annahme verletzt -- der Bereich sollte 2 Commits haben.";
    ASSERT_EQ(zeilen, 2) << "Fixture-Annahme verletzt -- der Bereich sollte 2 Zusatzzeilen haben.";

    Lauf const kum = repo.wache("--bereich main development");
    lauf_berichten("Bereich in der Ausgabe", kum, marke);
    EXPECT_EQ(kum.code, 0) << kum.ausgabe;

    EXPECT_TRUE(enthaelt(kum.ausgabe, "ABZWEIGUNG (merge-base) = " + mb))
        << "Die Abzweigung gehoert VOLL in die Ausgabe -- 'der letzte bequeme SHA' ist kein Bereich.\n"
        << kum.ausgabe;
    EXPECT_TRUE(enthaelt(kum.ausgabe, "BASIS      main = " + basis_sha)) << kum.ausgabe;
    EXPECT_TRUE(enthaelt(kum.ausgabe, "SPITZE     development = " + spitze_sha)) << kum.ausgabe;
    EXPECT_TRUE(enthaelt(kum.ausgabe, commit_zahl + " Commit(s) im Bereich")) << kum.ausgabe;
    EXPECT_TRUE(enthaelt(kum.ausgabe, std::to_string(zeilen) + " Zusatzzeilen insgesamt im Diff")) << kum.ausgabe;
    // Und das Verdikt selbst traegt den Bereich: wer nur die letzte Zeile zitiert, zitiert
    // sonst die halbe Aussage.
    EXPECT_TRUE(enthaelt(kum.ausgabe, "KUMULATIV ueber " + mb + ".." + spitze_sha + " (2 Commits)")) << kum.ausgabe;
}

// ===========================================================================================
// (5) DER BEREICH IST DIE ABZWEIGUNG, NICHT DER ENDPUNKT (V-1, Mess-Regel 2).
//     Die BASIS heilt eine ueberlange Zeile aus dem Altbestand; die SPITZE hat sie nie
//     angefasst. Der Endpunkt-Diff rechnet der SPITZE diese fremde Zeile als "hinzugefuegt"
//     an -- der Abzweigungs-Diff nicht. Beide Richtungen stehen hier, sonst waere die
//     Zusicherung nicht von "meldet immer gruen" zu unterscheiden.
//     Am Objekt gemessen (2026-08-10, Wegwerf-Repo): zwei Punkte Exit 1 "q.cpp:1 (227 Byte)",
//     drei Punkte Exit 0.
// ===========================================================================================
TEST(HygKumulativerBereich, GemessenWirdAbDerAbzweigungNichtAbDemEndpunkt) {
    std::string const marke = koeder_marke();
    WegwerfRepo       repo{marke};

    // ALTBESTAND auf der gemeinsamen Basis: eine ueberlange Zeile, die niemand in diesem
    // Zyklus geschrieben hat.
    repo.datei_schreiben("q.cpp", ueberlange_zeile(marke) + "\n");
    repo.committen("q.cpp", "altbestand");
    repo.git("branch development");

    // Die BASIS (main) heilt sie -- die SPITZE (development) weiss davon nichts.
    repo.datei_schreiben("q.cpp", "int lang_" + marke + " = 1;\n");
    repo.committen("q.cpp", "main-heilt-die-ueberlaenge");

    repo.git("checkout -q development");
    repo.datei_anhaengen("q.cpp", "int neu_" + marke + " = 2;");
    repo.committen("q.cpp", "development-fuegt-eine-saubere-zeile-an");

    Lauf const endpunkt = repo.wache("\"main..development\"");
    lauf_berichten("Durchreich-Modus, ZWEI Punkte (Endpunkt)", endpunkt, marke);
    EXPECT_EQ(endpunkt.code, 1) << "Der Endpunkt-Diff MUSS hier anschlagen -- sonst belegt dieser Fall nicht, "
                                   "dass die Abzweigung einen Unterschied macht.\n"
                                << endpunkt.ausgabe;

    Lauf const abzweigung = repo.wache("--bereich main development");
    lauf_berichten("--bereich, ab der ABZWEIGUNG", abzweigung, marke);
    EXPECT_EQ(abzweigung.code, 0) << "Ab der Abzweigung gehoert die fremde, laengst geheilte Zeile nicht "
                                     "zur Menge der SPITZE -- der Lauf MUSS gruen sein.\n"
                                  << abzweigung.ausgabe;
    EXPECT_TRUE(enthaelt(abzweigung.ausgabe, "1 Commit(s) nur auf der BASIS (Divergenz)"))
        << "Die Divergenz gehoert benannt -- ein Fast-Forward ist in dieser Lage nicht moeglich.\n"
        << abzweigung.ausgabe;
    EXPECT_TRUE(enthaelt(abzweigung.ausgabe, "ein Fast-Forward ist damit NICHT moeglich")) << abzweigung.ausgabe;
}

// ===========================================================================================
// (6) FAIL-CLOSED: ein Ref, den dieses Repo nicht kennt (nicht geholt, flacher Klon,
//     Tippfehler), ist ABBRUCH -- nie gruen und auch nicht "1 Verstoss". Genau diese Lage
//     entsteht in der CI, wenn origin/main im Job-Klon fehlt.
//     Gegeneingang: derselbe Aufruf mit dem ECHTEN Ref darf gerade NICHT mit 2 enden.
// ===========================================================================================
TEST(HygKumulativerBereich, UnaufloesbarerRefIstAbbruchStattGruen) {
    std::string const marke = koeder_marke();
    WegwerfRepo       repo{marke};
    zwei_commits_auf_development(repo, marke, /*mit_koeder=*/false, "");

    std::string const geisterref = "kein_ref_" + marke;

    Lauf const basis_fehlt = repo.wache("--bereich " + geisterref + " development");
    lauf_berichten("BASIS unaufloesbar", basis_fehlt, marke);
    EXPECT_EQ(basis_fehlt.code, 2) << "Ein unaufloesbarer Ref ist ABBRUCH (2), nicht gruen und nicht 1.\n"
                                   << basis_fehlt.ausgabe;
    EXPECT_TRUE(enthaelt(basis_fehlt.ausgabe, "ABBRUCH")) << basis_fehlt.ausgabe;
    EXPECT_TRUE(enthaelt(basis_fehlt.ausgabe, geisterref))
        << "Der fehlende Ref muss NAMENTLICH in der Ausgabe stehen.\n"
        << basis_fehlt.ausgabe;
    // DER GRUND MUSS DER RICHTIGE SEIN, nicht bloss irgendeiner (V-8). Diese Zusicherung ist am
    // 10.08.2026 durch die Wegwerf-Mutation M5 entstanden: ein stiller Fallback auf HEAD statt des
    // Abbruchs UEBERLEBTE die erste Fassung dieses Falls. Der Grund: der Bereich HEAD..HEAD hat
    // 0 Commits, also brach die Wache doch ab -- mit Exit 2, mit dem Ref-Namen im Kopf, aus einer
    // GANZ ANDEREN Ursache. Zwei Zustaende, eine Beobachtung. Ab hier wird der Grund selbst gepinnt.
    EXPECT_TRUE(enthaelt(basis_fehlt.ausgabe, "ist in diesem Repo nicht aufloesbar"))
        << "Der Abbruch muss AM UNAUFLOESBAREN REF liegen -- ein Abbruch aus anderem Grund belegt "
           "diese Zusicherung nicht.\n"
        << basis_fehlt.ausgabe;
    EXPECT_FALSE(enthaelt(basis_fehlt.ausgabe, "0 Commit(s) im Bereich"))
        << "Ein unaufloesbarer Ref darf gar nicht erst bis zur Commit-Zaehlung kommen.\n"
        << basis_fehlt.ausgabe;
    EXPECT_FALSE(enthaelt(basis_fehlt.ausgabe, "DIFF-HYGIENE-WACHE: GRUEN")) << basis_fehlt.ausgabe;

    Lauf const spitze_fehlt = repo.wache("--bereich main " + geisterref);
    lauf_berichten("SPITZE unaufloesbar", spitze_fehlt, marke);
    EXPECT_EQ(spitze_fehlt.code, 2) << spitze_fehlt.ausgabe;
    EXPECT_TRUE(enthaelt(spitze_fehlt.ausgabe, geisterref)) << spitze_fehlt.ausgabe;
    EXPECT_TRUE(enthaelt(spitze_fehlt.ausgabe, "ist in diesem Repo nicht aufloesbar")) << spitze_fehlt.ausgabe;

    // GEGENEINGANG (T-4): mit echten Refs ist derselbe Aufruf gerade KEIN Abbruch.
    Lauf const echt = repo.wache("--bereich main development");
    lauf_berichten("Gegeneingang: echte Refs", echt, marke);
    EXPECT_NE(echt.code, 2) << "Mit echten Refs darf die Wache nicht abbrechen -- sonst waere (6) eine Konstante.\n"
                            << echt.ausgabe;
}

// ===========================================================================================
// (7) FAIL-CLOSED, zweite Haelfte: ein Bereich mit NULL Commits ist ABBRUCH. Dort waere
//     nichts geprueft worden, und ein nicht gelaufener Test ist kein bestandener. Das ist die
//     Lage einer falsch verdrahteten CI-Variablen (`--bereich HEAD`, leerer Wert, zweimal
//     derselbe Ref) -- und ohne diese Regel bekaeme sie die Quittung "GRUEN".
//     Gegeneingang: EIN Commit im Bereich genuegt und ist gerade kein Abbruch.
// ===========================================================================================
TEST(HygKumulativerBereich, LeererBereichIstAbbruchStattGruen) {
    std::string const marke = koeder_marke();
    WegwerfRepo       repo{marke};
    zwei_commits_auf_development(repo, marke, /*mit_koeder=*/false, "");

    Lauf const leer = repo.wache("--bereich development development");
    lauf_berichten("leerer Bereich", leer, marke);
    EXPECT_EQ(leer.code, 2) << "0 Commits im Bereich MUSS Abbruch sein -- gruen mit Nenner 0 ist rot.\n"
                            << leer.ausgabe;
    EXPECT_TRUE(enthaelt(leer.ausgabe, "0 Commit(s) im Bereich"))
        << "Die Null gehoert in die Ausgabe, nicht nur in den Exit-Code.\n"
        << leer.ausgabe;
    EXPECT_FALSE(enthaelt(leer.ausgabe, "DIFF-HYGIENE-WACHE: GRUEN")) << leer.ausgabe;

    // GEGENEINGANG (T-4): ein einziger Commit im Bereich reicht und ist kein Abbruch.
    Lauf const einer = repo.wache("--bereich \"development~1\" development");
    lauf_berichten("Gegeneingang: 1 Commit im Bereich", einer, marke);
    EXPECT_NE(einer.code, 2) << "Ein nicht leerer Bereich darf nicht abbrechen.\n" << einer.ausgabe;
    EXPECT_TRUE(enthaelt(einer.ausgabe, "1 Commit(s) im Bereich")) << einer.ausgabe;
}

// ===========================================================================================
// (8) BEDIENFEHLER SIND ABBRUCH, NICHT STILLE ANNAHME. `--bereich` ohne Argument und mit zu
//     vielen Argumenten. Ein Modus, der bei Unfug irgendetwas misst, misst das Falsche.
// ===========================================================================================
TEST(HygKumulativerBereich, BedienfehlerAmBereichSindAbbruch) {
    std::string const marke = koeder_marke();
    WegwerfRepo       repo{marke};
    zwei_commits_auf_development(repo, marke, /*mit_koeder=*/false, "");

    Lauf const ohne = repo.wache("--bereich");
    lauf_berichten("--bereich ohne Argument", ohne, marke);
    EXPECT_EQ(ohne.code, 2) << ohne.ausgabe;
    EXPECT_TRUE(enthaelt(ohne.ausgabe, "braucht eine BASIS")) << ohne.ausgabe;

    Lauf const zuviel = repo.wache("--bereich main development noch_eins_" + marke);
    lauf_berichten("--bereich mit drei Argumenten", zuviel, marke);
    EXPECT_EQ(zuviel.code, 2) << zuviel.ausgabe;
    EXPECT_TRUE(enthaelt(zuviel.ausgabe, "hoechstens zwei Argumente")) << zuviel.ausgabe;
}

// ===========================================================================================
// (9) T-6-SCHWESTERSTELLE, als Regression gepinnt: die beiden ALTEN Wege duerfen sich durch
//     diesen Umbau NICHT geaendert haben. Der Durchreich-Modus bleibt ohne Bereichs-Block,
//     und sein Verdikt bleibt die nackte Zeile -- sonst haette dieses Paket still eine
//     zweite Schnittstelle veraendert, an der die CI heute haengt.
// ===========================================================================================
TEST(HygKumulativerBereich, DurchreichUndStdinModusBleibenUnveraendert) {
    std::string const marke = koeder_marke();
    WegwerfRepo       repo{marke};
    zwei_commits_auf_development(repo, marke, /*mit_koeder=*/true, ueberlange_zeile(marke));

    Lauf const durchreich = repo.wache("\"main..development\"");
    lauf_berichten("Durchreich-Modus", durchreich, marke);
    EXPECT_EQ(durchreich.code, 1) << durchreich.ausgabe;
    EXPECT_TRUE(enthaelt(durchreich.ausgabe, "DIFF-HYGIENE-WACHE: ROT")) << durchreich.ausgabe;
    EXPECT_FALSE(enthaelt(durchreich.ausgabe, "KUMULATIV"))
        << "Der Durchreich-Modus darf sich nicht als kumulativ ausgeben -- er loest keine Abzweigung auf.\n"
        << durchreich.ausgabe;
    EXPECT_FALSE(enthaelt(durchreich.ausgabe, "ABZWEIGUNG (merge-base)")) << durchreich.ausgabe;

    Lauf const stdin_lauf = repo.wache("--stdin < /dev/null");
    lauf_berichten("stdin-Modus, leere Eingabe", stdin_lauf, marke);
    EXPECT_EQ(stdin_lauf.code, 0) << "Der stdin-Weg traegt seine Nenner-Verantwortung beim Aufrufer -- "
                                     "das war vor diesem Paket so und muss so bleiben.\n"
                                  << stdin_lauf.ausgabe;
    EXPECT_FALSE(enthaelt(stdin_lauf.ausgabe, "KUMULATIV")) << stdin_lauf.ausgabe;
}

#endif // !_WIN32
