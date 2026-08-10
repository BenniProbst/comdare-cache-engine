// wachen_werkbank.hpp -- die WERKBANK der Wachen-Biss-Tests.               (2026-08-10)
// =============================================================================
// WOFUER SIE DA IST
//
// Ein Selbsttest einer CI-Wache muss dreierlei koennen, und jedes davon ist eine
// Fehlerquelle, wenn es je Fall neu geschrieben wird:
//
//  (1) DEN KOEDER WUERFELN (K13). Kein Orakel schreibt eine Kennung ab. Jede Marke
//      kommt frisch aus /dev/urandom und wird WOERTLICH in der Ausgabe der Wache
//      zurueckgefordert. Steht sie dort, kann sie nur aus dem Gegenstand stammen,
//      den dieser Fall gerade angelegt hat -- aus keiner Datei dieses Repos.
//
//  (2) EINEN WEGWERF-GEGENSTAND BAUEN. Zwei der drei Wachen fragen `git` nach der
//      Repo-Wurzel und nach `git ls-files`; sie brauchen also ein echtes, eigenes
//      Repo. Der Bestand wird dabei NIE angefasst: jeder Fall arbeitet unter einem
//      eigenen Temp-Pfad und raeumt ihn wieder ab.
//
//  (3) DEN ARRANGEMENT-FEHLER VOM BEFUND TRENNEN. Jeder Schritt liefert eine
//      testing::AssertionResult. Misslingt das Arrangement, ist der Fall ROT mit
//      Fixture-Diagnose -- er wird NIE als "gefangen" verbucht. Genau daran ist die
//      abgeloeste Shell-Form am 09.08. gescheitert: ihr Orakel zaehlte jeden
//      Rueckgabewert ausser 0 und 2 als Biss, ein `tr`-Shim mit rc=127 liess alle
//      fuenf Mutanten als "gefangen" durchgehen.
//
// DIE DREI ZUSTAENDE, DIE EIN FALL UNTERSCHEIDEN MUSS:
//      (a) die Wache greift        -> erwarteter Exit MIT dem erwarteten Literal
//      (b) die Wache greift nicht  -> Mutant ueberlebt -> ROT
//      (c) das Werkzeug ist kaputt -> ROT, und ausdruecklich NICHT "gefangen"
// Ein Mutant gilt erst als getoetet, wenn er sein EIGENES Riss-Literal vorweist.
//
// WARUM HEADER-ONLY UND NICHT EIN CMake-ZIEL: das ist die Hausform DIESES Repos --
// tests/unit/support/ traegt bereits oeb_stempel_zeilen.hpp und
// std_map_equivalence_harness.hpp als reine Header. Die super-Werkbank
// (Code/ci_wachen/testwerkbank/) ist eine Bibliothek, weil sie gegen eine
// Produktions-Bibliothek linkt; hier gibt es nichts zu linken.
//
// ASCII-only, Zeilen <= 120 Byte.
// =============================================================================
#pragma once

#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#if !defined(_WIN32)
#include <sys/wait.h>
#endif

#include "../comdare_test_tmp.hpp"

namespace comdare::test::wachen {

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// (1) DER WUERFEL. /dev/urandom, nicht std::random_device und erst recht keine
// Konstante aus einer Doku: der Wert darf in KEINER Datei dieses Repos vorkommen.
// Gibt die Quelle nichts her, ist das ein ROTER Fall und kein stiller Ersatzwert --
// ohne frischen Koeder gibt es keinen Beweis.
// ---------------------------------------------------------------------------
[[nodiscard]] inline std::string koeder(std::size_t bytes = 6) {
    std::ifstream quelle{"/dev/urandom", std::ios::binary};
    EXPECT_TRUE(quelle.good()) << "/dev/urandom nicht lesbar -- ohne frischen Koeder kein Beweis";
    std::vector<unsigned char> roh(bytes, 0U);
    quelle.read(reinterpret_cast<char*>(roh.data()), static_cast<std::streamsize>(bytes));
    EXPECT_EQ(quelle.gcount(), static_cast<std::streamsize>(bytes))
        << "/dev/urandom lieferte zu wenige Bytes -- fail-closed statt Ersatzwert";
    static constexpr char kZiffern[] = "0123456789abcdef";
    std::string           marke;
    for (unsigned char const b : roh) {
        marke.push_back(kZiffern[(b >> 4U) & 0x0FU]);
        marke.push_back(kZiffern[b & 0x0FU]);
    }
    return marke;
}

// ---------------------------------------------------------------------------
// Ein Lauf des Prueflings. 'code' ist -1, wenn der Prozess nicht normal endete --
// das ist ausdruecklich NICHT dasselbe wie "irgendein Fehlercode" und wird von den
// Faellen auch nie als Biss gewertet.
// ---------------------------------------------------------------------------
struct Lauf {
    int         code{-1};
    std::string ausgabe;
};

[[nodiscard]] inline bool enthaelt(std::string const& heuhaufen, std::string_view nadel) {
    return heuhaufen.find(nadel) != std::string::npos;
}

// stdout UND stderr einsammeln: die Wachen schreiben ihre ABBRUCH-Zeilen auf stderr
// und ihre Befunde auf stdout. Ein Fall, der nur eines von beiden sieht, koennte
// einen Abbruch (Zustand c) nicht von einem Befund (Zustand a) trennen.
[[nodiscard]] inline Lauf fahre(std::string const& befehl) {
    Lauf  ergebnis;
    FILE* rohr = ::popen((befehl + " 2>&1").c_str(), "r");
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

// Ein Pfad, den die Shell woertlich nimmt. Die Temp-Wurzel enthaelt heute keine
// Sonderzeichen; das ist eine Eigenschaft der Umgebung und keine Zusicherung, deshalb
// wird sie hier nicht behauptet, sondern der Pfad wird in Anfuehrungszeichen gesetzt.
[[nodiscard]] inline std::string zitiert(fs::path const& p) { return "\"" + p.string() + "\""; }

// ---------------------------------------------------------------------------
// (2) DER WEGWERF-GEGENSTAND: ein eigenes git-Repo unter einem eindeutigen Temp-Pfad.
//
// GIT_CONFIG_GLOBAL/SYSTEM=/dev/null: die Faelle duerfen nicht davon abhaengen, was
// in der Konfiguration DIESER Maschine steht (init.defaultBranch, core.hooksPath,
// templateDir). Ein Fall, der auf einem anders konfigurierten Runner anders laeuft,
// beweist nichts.
// ---------------------------------------------------------------------------
class WegwerfRepo {
public:
    explicit WegwerfRepo(std::string const& marke)
        : wurzel_{comdare::test::user_tmp_dir() / ("wachen_werkbank_" + marke)} {
        std::error_code ec;
        fs::remove_all(wurzel_, ec);
        fs::create_directories(wurzel_, ec);
    }

    WegwerfRepo(WegwerfRepo const&)            = delete;
    WegwerfRepo& operator=(WegwerfRepo const&) = delete;
    WegwerfRepo(WegwerfRepo&&)                 = delete;
    WegwerfRepo& operator=(WegwerfRepo&&)      = delete;

    ~WegwerfRepo() {
        std::error_code ec;
        fs::remove_all(wurzel_, ec);
    }

    [[nodiscard]] fs::path const& pfad() const { return wurzel_; }

    [[nodiscard]] testing::AssertionResult init() {
        Lauf const l = fahre(umgebung() + " git init -q " + zitiert(wurzel_));
        if (l.code != 0) {
            return testing::AssertionFailure() << "git init fehlgeschlagen (Exit " << l.code << "):\n" << l.ausgabe;
        }
        return testing::AssertionSuccess();
    }

    [[nodiscard]] testing::AssertionResult schreibe(std::string const& relativ, std::string const& inhalt) const {
        fs::path const  ziel = wurzel_ / relativ;
        std::error_code ec;
        fs::create_directories(ziel.parent_path(), ec);
        std::ofstream aus{ziel, std::ios::binary | std::ios::trunc};
        if (!aus.good()) { return testing::AssertionFailure() << "konnte '" << ziel.string() << "' nicht anlegen"; }
        aus << inhalt;
        aus.close();
        if (!fs::exists(ziel)) {
            return testing::AssertionFailure() << "'" << ziel.string() << "' fehlt nach dem Schreiben";
        }
        return testing::AssertionSuccess();
    }

    // GEGENPROBE eingebaut: nach dem `git add` wird git SELBST gefragt, ob die Datei
    // im Index steht. Ohne diese Frage waere das Arrangement eine Behauptung -- und
    // ein stiller add-Fehler saehe genauso aus wie eine funktionierende Wache.
    [[nodiscard]] testing::AssertionResult schreibe_und_verfolge(std::string const& relativ,
                                                                 std::string const& inhalt) const {
        testing::AssertionResult const r = schreibe(relativ, inhalt);
        if (!r) { return r; }
        Lauf const add = fahre(umgebung() + " git -C " + zitiert(wurzel_) + " add -- " + zitiert(relativ));
        if (add.code != 0) {
            return testing::AssertionFailure()
                   << "git add '" << relativ << "' fehlgeschlagen (Exit " << add.code << "):\n"
                   << add.ausgabe;
        }
        return ist_verfolgt(relativ);
    }

    // DIE FALLE, DIE HIER STRUKTURELL AUSGESCHLOSSEN WIRD: beide Wachen bestimmen ihre
    // Repo-Wurzel per `git rev-parse --show-toplevel`. Waere das `git init` still
    // misslungen, liefe die Wache gegen das UMGEBENDE Repo -- sie wuerde messen, aber
    // den falschen Gegenstand, und der Fall saehe wie ein Befund aus. Genau diese Klasse
    // hat bei ci_diff_ascii_width_guard.sh schon einmal den falschen Baum gemessen.
    [[nodiscard]] testing::AssertionResult ist_eigene_wurzel() const {
        Lauf const l = fahre(umgebung() + " git -C " + zitiert(wurzel_) + " rev-parse --show-toplevel");
        if (l.code != 0) { return testing::AssertionFailure() << "rev-parse fehlgeschlagen:\n" << l.ausgabe; }
        std::string gemeldet = l.ausgabe;
        while (!gemeldet.empty() && (gemeldet.back() == '\n' || gemeldet.back() == '\r')) { gemeldet.pop_back(); }
        std::error_code ec;
        fs::path const  erwartet = fs::canonical(wurzel_, ec);
        fs::path const  ist      = fs::canonical(fs::path{gemeldet}, ec);
        if (ec || erwartet != ist) {
            return testing::AssertionFailure() << "git meldet die Wurzel '" << gemeldet << "', erwartet war '"
                                               << wurzel_.string() << "' -- der Fall wuerde den falschen Baum messen.";
        }
        return testing::AssertionSuccess();
    }

    [[nodiscard]] testing::AssertionResult ist_verfolgt(std::string const& relativ) const {
        Lauf const l =
            fahre(umgebung() + " git -C " + zitiert(wurzel_) + " ls-files --error-unmatch -- " + zitiert(relativ));
        if (l.code != 0) {
            return testing::AssertionFailure() << "'" << relativ << "' ist NICHT im Index (Exit " << l.code << "):\n"
                                               << l.ausgabe;
        }
        return testing::AssertionSuccess();
    }

    // Die Umgebung, in der jeder Aufruf gegen dieses Repo faehrt. Auch die Wachen
    // selbst werden darueber gestartet -- sonst laese `git rev-parse --show-toplevel`
    // in ihnen eine andere Konfiguration als die Werkbank beim Arrangieren.
    [[nodiscard]] static std::string umgebung() {
        return "GIT_CONFIG_GLOBAL=/dev/null GIT_CONFIG_SYSTEM=/dev/null GIT_TERMINAL_PROMPT=0";
    }

private:
    fs::path wurzel_;
};

// ---------------------------------------------------------------------------
// EIN PRAEPARIERTER BAU-BAUM. `ctest -N` liest ausschliesslich CTestTestfile.cmake --
// es braucht kein Compilat und keinen Generator. Damit laesst sich jede Inventur
// exakt herstellen, auch eine, die es im echten Baum nie gibt.
// ---------------------------------------------------------------------------
inline testing::AssertionResult schreibe_inventur(fs::path const& baum, std::vector<std::string> const& namen) {
    std::error_code ec;
    fs::create_directories(baum, ec);
    std::ofstream aus{baum / "CTestTestfile.cmake", std::ios::trunc};
    if (!aus.good()) { return testing::AssertionFailure() << "CTestTestfile.cmake in '" << baum.string() << "'"; }
    for (auto const& n : namen) { aus << "add_test(" << n << " \"/bin/true\")\n"; }
    aus.close();
    if (!fs::exists(baum / "CTestTestfile.cmake")) {
        return testing::AssertionFailure() << "CTestTestfile.cmake fehlt nach dem Schreiben";
    }
    return testing::AssertionSuccess();
}

// ctest in den PATH heben. Ohne das brechen die Wachen mit "ctest ist nicht im PATH"
// (Exit 2) ab -- ein zweiter, vom Befund NICHT unterscheidbarer Grund fuer ein Nicht-0.
[[nodiscard]] inline std::string mit_ctest_im_pfad(std::string const& ctest_binary) {
    fs::path const dir = fs::path{ctest_binary}.parent_path();
    return "PATH=\"" + dir.string() + ":$PATH\"";
}

} // namespace comdare::test::wachen
