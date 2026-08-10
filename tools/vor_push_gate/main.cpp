// main.cpp -- VOR-PUSH-GATE, die CLI. argv -> Kern -> Exit-Abbildung.        (2026-08-10)
//
// Die Begruendung, der Gegenstand und die Grenze dieses Werkzeugs stehen im Kopf von
// vor_push_gate.hpp. Hier steht NUR, was diese Datei zusaetzlich traegt: die Laeufer.
//
// ===================================================================================================
// DIE LAEUFER -- und warum es zwei Sorten gibt
// ===================================================================================================
// SORTE A, VERBATIM. Jobs, deren script: vollstaendig in .gitlab-ci.yml steht und kein cmake/ctest
// enthaelt. Ihr Kommando wird bei jedem Lauf AUS DER DATEI GELESEN und unveraendert ausgefuehrt.
// Diese Sorte kann nicht driften: aendert sich das Skript, aendert sich der Lauf. Ein NEUER Job
// dieser Bauart wird automatisch mitgefahren, ohne dass hier eine Zeile dazukommt.
//
// SORTE B, NACHBAU. lint:format, lint:static und lint:secrets erben ihr Skript aus dem FREMDEN
// Projekt ci-templates (base-pipeline.yml). Dieses Repo sieht das Skript nicht, also wird es hier
// nachgebaut -- und das ist die gefaehrliche Sorte, weil ein Nachbau still vom Original wegdriften
// kann. Zwei Gegenmittel:
//   (1) Die Optionen sind am 10.08.2026 Zeichen fuer Zeichen aus base-pipeline.yml abgeschrieben
//       (Zeilen 165-181 fuer .lint-format, 181-220 fuer .lint-static, 152-162 fuer .lint-secrets);
//       die Fundstellen stehen an jedem Laeufer.
//   (2) Vor jedem Lauf wird geprueft, ob der Job noch DIESE Vorlage erbt. Erbt er sie nicht mehr,
//       gilt der Job als 'abweichend gefahren' und zaehlt NICHT im Zaehler.
//
// ===================================================================================================
// DER BEFUND, DER SORTE B UEBERHAUPT NOETIG MACHT -- Diff-Menge gegen ls-files-Menge
// ===================================================================================================
// Die bisherige lokale Vorpruefung (scripts/vor_push_alle_wachen.sh) faehrt clang-format ueber die
// im Diff BERUEHRTEN Dateien. Der CI-Job lint:format faehrt es ueber `git ls-files -- $LINT_PATHS`,
// also ueber den GANZEN versionierten Bestand -- am 10.08. waren das 1849 Dateien gegen 55 im Diff.
// Beide Mengen koennen gleichzeitig gruen und rot sein. Dieser Laeufer bildet die CI-Menge nach.
//
// ===================================================================================================
// EXIT-CODES (fail-closed: ein nicht gelaufener Test ist KEIN bestandener)
// ===================================================================================================
//   0  alle HIER gefahrenen Jobs gruen. Sagt NICHTS ueber die nicht gefahrenen.
//   1  mindestens ein hier gefahrener Job ROT.
//   2  ABBRUCH: kein Repo, keine .gitlab-ci.yml, oder der Nenner waere 0. Ein Nenner 0 ist
//      niemals ein Gruen -- er heisst, das Werkzeug hat seinen Gegenstand nicht gefunden.
//
// ASCII-only, Zeilen <= 120 Byte.

#include "vor_push_gate.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

namespace vpg = comdare::vor_push_gate;
namespace fs  = std::filesystem;

namespace {

// ---------------------------------------------------------------------------------------------
// Prozesse. Zwei Wege, mit Absicht.
// ---------------------------------------------------------------------------------------------

struct Ergebnis {
    int         rc = -1;
    std::string ausgabe;
};

// Wichtig und leicht zu verlieren: pclose() liefert den WAIT-STATUS, nicht den Exit-Code.
// Ohne WEXITSTATUS misst man 256*rc und jeder Vergleich gegen 0/1/2 geht schief.
Ergebnis lauf_shell(const std::string& kommando) {
    Ergebnis   e;
    std::FILE* p = ::popen((kommando + " 2>&1").c_str(), "r");
    if (p == nullptr) {
        e.rc      = -1;
        e.ausgabe = "popen fehlgeschlagen";
        return e;
    }
    std::array<char, 4096> puffer{};
    while (std::fgets(puffer.data(), static_cast<int>(puffer.size()), p) != nullptr) { e.ausgabe += puffer.data(); }
    const int status = ::pclose(p);
    e.rc             = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    return e;
}

// Ohne Shell: fuer Kommandos mit sehr vielen Argumenten (clang-format ueber 1849 Dateien) und
// ueberall dort, wo Anfuehrungszeichen-Quoting eine stille Fehlerquelle waere.
Ergebnis lauf_argv(const std::vector<std::string>& argv) {
    Ergebnis e;
    int      rohr[2];
    if (::pipe(rohr) != 0) {
        e.ausgabe = "pipe fehlgeschlagen";
        return e;
    }
    const pid_t kind = ::fork();
    if (kind < 0) {
        ::close(rohr[0]);
        ::close(rohr[1]);
        e.ausgabe = "fork fehlgeschlagen";
        return e;
    }
    if (kind == 0) {
        ::close(rohr[0]);
        ::dup2(rohr[1], STDOUT_FILENO);
        ::dup2(rohr[1], STDERR_FILENO);
        ::close(rohr[1]);
        std::vector<char*> roh;
        roh.reserve(argv.size() + 1);
        for (const auto& a : argv) { roh.push_back(const_cast<char*>(a.c_str())); }
        roh.push_back(nullptr);
        ::execvp(roh[0], roh.data());
        ::_exit(127); // execvp kehrt nur im Fehlerfall zurueck
    }
    ::close(rohr[1]);
    std::array<char, 4096> puffer{};
    ssize_t                gelesen = 0;
    while ((gelesen = ::read(rohr[0], puffer.data(), puffer.size())) > 0) {
        e.ausgabe.append(puffer.data(), static_cast<std::size_t>(gelesen));
    }
    ::close(rohr[0]);
    int status = 0;
    ::waitpid(kind, &status, 0);
    e.rc = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    return e;
}

std::vector<std::string> zeilen_aus(const std::string& text) {
    std::vector<std::string> z;
    std::string              aktuell;
    for (const char c : text) {
        if (c == '\n') {
            z.push_back(aktuell);
            aktuell.clear();
            continue;
        }
        if (c != '\r') { aktuell += c; }
    }
    if (!aktuell.empty()) { z.push_back(aktuell); }
    return z;
}

// Zerlegt eine leerzeichengetrennte Liste ("libs apps tests") in ihre Worte. Genau das macht
// die Shell in der Vorlage beim unquotierten ${COMDARE_LINT_PATHS}.
std::vector<std::string> worte(const std::string& text) {
    std::vector<std::string> w;
    std::string              aktuell;
    for (const char c : text) {
        if (c == ' ' || c == '\t' || c == '\n') {
            if (!aktuell.empty()) {
                w.push_back(aktuell);
                aktuell.clear();
            }
            continue;
        }
        aktuell += c;
    }
    if (!aktuell.empty()) { w.push_back(aktuell); }
    return w;
}

std::optional<std::vector<std::string>> lies_datei(const fs::path& datei) {
    std::ifstream ein(datei);
    if (!ein) {
        return std::nullopt; // fail-closed: "Datei weg" ist nicht dasselbe wie "Datei leer"
    }
    std::vector<std::string> z;
    std::string              zeile;
    while (std::getline(ein, zeile)) {
        if (!zeile.empty() && zeile.back() == '\r') { zeile.pop_back(); }
        z.push_back(zeile);
    }
    return z;
}

// Erstes ausfuehrbares aus einer Kandidatenliste. Meldet zurueck, WO gesucht wurde -- ein
// blosses "nicht gefunden" ist eine Behauptung ohne Beleg (an genau der ist die alte Wache
// schon einmal gescheitert: sie erklaerte ein vorhandenes cppcheck fuer fehlend).
struct Werkzeug {
    std::string pfad;
    std::string gesucht;
};

Werkzeug finde_werkzeug(const char* umgebungsvariable, const std::vector<std::string>& kandidaten) {
    Werkzeug w;
    if (const char* aus_umgebung = std::getenv(umgebungsvariable); aus_umgebung != nullptr && *aus_umgebung != 0) {
        w.pfad    = aus_umgebung;
        w.gesucht = std::string(umgebungsvariable) + "=" + w.pfad;
        return w;
    }
    for (const auto& k : kandidaten) {
        w.gesucht += (w.gesucht.empty() ? "" : " ") + k;
        const Ergebnis e = lauf_shell("command -v " + k);
        if (e.rc == 0) {
            w.pfad = k;
            return w;
        }
    }
    return w;
}

// ---------------------------------------------------------------------------------------------
// Sorte B: die drei Nachbauten
// ---------------------------------------------------------------------------------------------

struct LaufErgebnis {
    vpg::Deckung deckung = vpg::Deckung::kein_laeufer;
    std::string  bemerkung;
    std::string  details;
};

// ci-templates/base-pipeline.yml:165-181 (.lint-format), abgeschrieben am 2026-08-10:
//   mapfile -t F < <(git ls-files -- ${COMDARE_LINT_PATHS} | grep -E '\.(c|cc|...)$'
//                    | grep -vE "${COMDARE_LINT_EXCLUDE_RE}")
//   clang-format --dry-run -Werror "${F[@]}"
LaufErgebnis fahre_lint_format(const std::vector<std::string>& yml, const fs::path& wurzel) {
    LaufErgebnis   r;
    const Werkzeug cf = finde_werkzeug("COMDARE_CLANG_FORMAT", {"/home/comdare/tools/cf22/usr/bin/clang-format-22",
                                                                "clang-format-22", "clang-format"});
    if (cf.pfad.empty()) {
        r.deckung   = vpg::Deckung::werkzeug_fehlt;
        r.bemerkung = "kein clang-format gefunden. Gesucht: " + cf.gesucht;
        return r;
    }

    std::string pfade        = vpg::variable(yml, "COMDARE_LINT_PATHS").value_or("");
    std::string quelle_pfade = "aus .gitlab-ci.yml";
    if (pfade.empty()) {
        pfade        = "libs apps tests"; // Vorgabe der Vorlage, base-pipeline.yml:58
        quelle_pfade = "Vorgabe der Vorlage (steht nicht in .gitlab-ci.yml)";
    }
    std::string aus        = vpg::variable(yml, "COMDARE_LINT_EXCLUDE_RE").value_or("");
    std::string quelle_aus = "aus .gitlab-ci.yml";
    if (aus.empty()) {
        aus        = R"((^|/)(ext|build|_archive_code_pre_migration|cmake-build-[^/]*|modules)/)";
        quelle_aus = "Vorgabe der Vorlage (steht nicht in .gitlab-ci.yml)";
    }

    const Ergebnis ls = lauf_shell("cd " + wurzel.string() + " && git ls-files -- " + pfade);
    if (ls.rc != 0) {
        r.deckung   = vpg::Deckung::werkzeug_fehlt;
        r.bemerkung = "git ls-files fehlgeschlagen (rc=" + std::to_string(ls.rc) + ")";
        return r;
    }
    const std::vector<std::string> dateien = vpg::lint_dateien(zeilen_aus(ls.ausgabe), aus);
    if (dateien.empty()) {
        // Der Leer-Guard der Vorlage (base-pipeline.yml:199) ist ehrlich leer-OK. Hier ist er
        // ein ABBRUCH-Verdacht: dieses Repo HAT Quellen. Nenner 0 ist kein Gruen.
        r.deckung   = vpg::Deckung::werkzeug_fehlt;
        r.bemerkung = "0 Dateien in der Lint-Menge -- Nenner 0 ist kein Gruen. Pfade: " + pfade;
        return r;
    }

    std::vector<std::string> argv{cf.pfad, "--dry-run", "-Werror"};
    argv.insert(argv.end(), dateien.begin(), dateien.end());
    const Ergebnis e = lauf_argv(argv);
    r.deckung        = (e.rc == 0) ? vpg::Deckung::gruen : vpg::Deckung::rot;
    r.bemerkung      = std::to_string(dateien.size()) + " Datei(en) geprueft (Pfade " + quelle_pfade + ", Ausschluss " +
                       quelle_aus + "), Werkzeug " + cf.pfad + ", Exit " + std::to_string(e.rc);
    r.details        = e.ausgabe;
    return r;
}

// ci-templates/base-pipeline.yml:181-220 (.lint-static), abgeschrieben am 2026-08-10:
//   cppcheck --enable=warning,portability --inline-suppr --library=googletest
//            --error-exitcode=2 --std=c++23 --language=c++ -q ${COMDARE_LINT_PATHS} $IGN
// IGN entsteht aus COMDARE_CPPCHECK_IGNORE_DIRS, per find auf real existierende Verzeichnisse
// expandiert; ein nackter -i-Name wuerde als SUBSTRING auch gegen den absoluten Pfad matchen.
LaufErgebnis fahre_lint_static(const std::vector<std::string>& yml, const fs::path& wurzel) {
    LaufErgebnis   r;
    const Werkzeug cc =
        finde_werkzeug("COMDARE_CPPCHECK", {"/home/comdare/tools/cppcheck-2.21.0/bin/cppcheck", "cppcheck"});
    if (cc.pfad.empty()) {
        r.deckung   = vpg::Deckung::werkzeug_fehlt;
        r.bemerkung = "kein cppcheck gefunden. Gesucht: " + cc.gesucht;
        return r;
    }
    const std::string pfade = vpg::variable(yml, "COMDARE_LINT_PATHS").value_or("libs apps tests");
    const std::string ign_dirs =
        vpg::variable(yml, "COMDARE_CPPCHECK_IGNORE_DIRS").value_or("ext build _archive_code_pre_migration modules");

    std::vector<std::string>       argv{cc.pfad,
                                        "--enable=warning,portability",
                                        "--inline-suppr",
                                        "--library=googletest",
                                        "--error-exitcode=2",
                                        "--std=c++23",
                                        "--language=c++",
                                        "-q"};
    const std::vector<std::string> lint_pfade = worte(pfade);
    argv.insert(argv.end(), lint_pfade.begin(), lint_pfade.end());

    argv.push_back("-i");
    argv.push_back("./.citools");
    std::size_t                    ign_anzahl = 1;
    const std::vector<std::string> namen      = worte(ign_dirs);
    std::error_code                fehler;
    for (fs::recursive_directory_iterator it(wurzel, fehler), ende; it != ende; it.increment(fehler)) {
        if (fehler) { break; }
        if (!it->is_directory(fehler)) { continue; }
        const std::string dname = it->path().filename().string();
        if (dname == ".git" || dname == ".citools") {
            it.disable_recursion_pending();
            continue;
        }
        if (std::find(namen.begin(), namen.end(), dname) != namen.end()) {
            argv.push_back("-i");
            argv.push_back("./" + fs::relative(it->path(), wurzel).string());
            ++ign_anzahl;
            it.disable_recursion_pending();
        }
    }

    // cppcheck bekommt RELATIVE Pfade (so wie die CI im Workspace steht) -- deshalb der
    // Verzeichniswechsel um den Lauf herum, und zurueck auch im Fehlerfall.
    const fs::path  zurueck = fs::current_path();
    std::error_code wechsel_fehler;
    fs::current_path(wurzel, wechsel_fehler);
    const Ergebnis e = lauf_argv(argv);
    fs::current_path(zurueck, wechsel_fehler);

    r.deckung   = (e.rc == 0) ? vpg::Deckung::gruen : vpg::Deckung::rot;
    r.bemerkung = std::to_string(lint_pfade.size()) + " Lint-Pfad(e), " + std::to_string(ign_anzahl) +
                  " Ignore-Eintraege, Werkzeug " + cc.pfad + ", Exit " + std::to_string(e.rc);
    r.details   = e.ausgabe;
    return r;
}

// ci-templates/base-pipeline.yml:152-162 (.lint-secrets):
//   gitleaks dir "${CI_PROJECT_DIR}" --no-banner --redact
LaufErgebnis fahre_lint_secrets(const fs::path& wurzel) {
    LaufErgebnis   r;
    const Werkzeug gl = finde_werkzeug("COMDARE_GITLEAKS", {"gitleaks"});
    if (gl.pfad.empty()) {
        r.deckung   = vpg::Deckung::werkzeug_fehlt;
        r.bemerkung = "kein gitleaks gefunden. Gesucht: " + gl.gesucht +
                      " -- COMDARE_GITLEAKS=<pfad> setzen. Dieses Gate bleibt OFFEN.";
        return r;
    }
    const Ergebnis e = lauf_argv({gl.pfad, "dir", wurzel.string(), "--no-banner", "--redact"});
    r.deckung        = (e.rc == 0) ? vpg::Deckung::gruen : vpg::Deckung::rot;
    r.bemerkung = "gitleaks dir ueber " + wurzel.string() + ", Werkzeug " + gl.pfad + ", Exit " + std::to_string(e.rc);
    r.details   = e.ausgabe;
    return r;
}

// ---------------------------------------------------------------------------------------------
// Sorte A: verbatim aus der Datei
// ---------------------------------------------------------------------------------------------

LaufErgebnis fahre_verbatim(const vpg::JobBlock& block, const fs::path& wurzel) {
    LaufErgebnis                   r;
    const std::vector<std::string> kommandos = vpg::script_kommandos(block);
    if (kommandos.empty()) {
        r.deckung   = vpg::Deckung::kein_laeufer;
        r.bemerkung = "script: nicht verbatim fahrbar (mehrzeiliges Blockskript oder leer)";
        return r;
    }
    std::string sammel;
    int         schlimmster = 0;
    for (const auto& k : kommandos) {
        const Ergebnis e = lauf_shell("cd " + wurzel.string() + " && " + k);
        sammel += "$ " + k + "\n" + e.ausgabe;
        if (e.rc != 0) { schlimmster = e.rc; }
    }
    r.deckung   = (schlimmster == 0) ? vpg::Deckung::gruen : vpg::Deckung::rot;
    r.bemerkung = std::to_string(kommandos.size()) + " Kommando(s) verbatim aus .gitlab-ci.yml, Exit " +
                  std::to_string(schlimmster);
    r.details   = sammel;
    return r;
}

// ---------------------------------------------------------------------------------------------
// Die Zuordnung Job -> Laeufer
// ---------------------------------------------------------------------------------------------

struct NachbauBindung {
    std::string_view job;
    std::string_view vorlage; // die Vorlage, FUER DIE der Nachbau geschrieben wurde
};

constexpr std::array<NachbauBindung, 3> kNachbauten{{
    {"lint:format", ".lint-format"},
    {"lint:static", ".lint-static"},
    {"lint:secrets", ".lint-secrets"},
}};

} // namespace

int main(int argc, char** argv) {
    bool     nur_nenner   = false;
    bool     ausfuehrlich = false;
    fs::path wurzel;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--nenner") {
            nur_nenner = true;
        } else if (a == "--ausfuehrlich") {
            ausfuehrlich = true;
        } else if (a == "--wurzel" && i + 1 < argc) {
            wurzel = argv[++i];
        } else if (a == "--hilfe" || a == "-h") {
            std::cout << "vor-push-gate [--wurzel <repo>] [--nenner] [--ausfuehrlich]\n"
                      << "  Nennt IMMER beide Mengen: gefahrene und nicht gefahrene CI-Jobs.\n"
                      << "  Exit 0 = hier gefahrene Jobs gruen (keine Aussage ueber die anderen)\n"
                      << "  Exit 1 = mindestens ein hier gefahrener Job ROT\n"
                      << "  Exit 2 = ABBRUCH (kein Repo / keine .gitlab-ci.yml / Nenner 0)\n";
            return 0;
        }
    }

    if (wurzel.empty()) {
        const Ergebnis e = lauf_shell("git rev-parse --show-toplevel");
        if (e.rc != 0) {
            std::cerr << "VOR-PUSH-GATE: ABBRUCH -- kein git-Repository (git rev-parse rc=" << e.rc << ")\n";
            return 2;
        }
        const std::vector<std::string> z = zeilen_aus(e.ausgabe);
        if (z.empty()) {
            std::cerr << "VOR-PUSH-GATE: ABBRUCH -- Repo-Wurzel nicht bestimmbar\n";
            return 2;
        }
        wurzel = z.front();
    }

    const fs::path ci_datei = wurzel / ".gitlab-ci.yml";
    const auto     yml      = lies_datei(ci_datei);
    if (!yml) {
        std::cerr << "VOR-PUSH-GATE: ABBRUCH -- " << ci_datei.string() << " nicht lesbar.\n"
                  << "  Ohne diese Datei gibt es keinen Nenner, und ohne Nenner kein Urteil.\n";
        return 2;
    }

    const std::vector<vpg::JobBlock> jobs = vpg::job_bloecke(*yml);
    if (jobs.empty()) {
        std::cerr << "VOR-PUSH-GATE: ABBRUCH -- 0 Jobs in " << ci_datei.string() << " erkannt.\n"
                  << "  Ein Nenner 0 ist niemals ein Gruen: das Werkzeug hat seinen Gegenstand\n"
                  << "  nicht gefunden (Datei umgebaut? Scanner-Regel gebrochen?).\n";
        return 2;
    }

    std::cout << "=============================================================================\n"
              << " VOR-PUSH-GATE -- welche CI-Jobs sind lokal gefahren, welche nicht?\n"
              << "=============================================================================\n"
              << "Repo:    " << wurzel.string() << "\n"
              << "Quelle:  " << ci_datei.string() << " (der Nenner kommt AUS DIESER DATEI,\n"
              << "         nicht aus einer Liste im Werkzeug -- kommt ein Job dazu, waechst er)\n"
              << "Jobs:    " << jobs.size() << " erkannt\n\n";

    if (nur_nenner) {
        for (const auto& j : jobs) {
            const vpg::Befund b   = vpg::bauwunsch(j);
            const char*       art = (b.wunsch == vpg::Bauwunsch::ohne_bau)      ? "ohne Bau  "
                                    : (b.wunsch == vpg::Bauwunsch::braucht_bau) ? "braucht Bau"
                                                                                : "unbekannt ";
            std::cout << "  " << art << "  " << j.name << "   (Zeile " << j.erste_zeile << ")\n";
        }
        std::cout << "\nNENNER: " << jobs.size() << " Jobs.\n";
        return 0;
    }

    std::vector<vpg::Lauf> laeufe;
    laeufe.reserve(jobs.size());
    std::string protokoll;

    for (const auto& job : jobs) {
        vpg::Lauf lauf;
        lauf.name = job.name;

        // Sorte B zuerst: gibt es fuer diesen Job einen erklaerten Nachbau?
        const NachbauBindung* bindung = nullptr;
        for (const auto& n : kNachbauten) {
            if (n.job == job.name) {
                bindung = &n;
                break;
            }
        }

        if (bindung != nullptr) {
            const std::string erbt = vpg::extends_ziel(job);
            if (erbt.find(bindung->vorlage) == std::string::npos) {
                // DRIFT. Der Nachbau wurde fuer eine andere Vorlage geschrieben als die, die der
                // Job heute erbt. Ihn trotzdem zu fahren hiesse, ueber einen anderen Lauf zu reden.
                lauf.deckung   = vpg::Deckung::abweichend;
                lauf.bemerkung = "Nachbau gilt fuer Vorlage '" + std::string(bindung->vorlage) +
                                 "', der Job erbt heute '" + (erbt.empty() ? "(nichts)" : erbt) +
                                 "' -- NICHT gefahren, Bindung pruefen";
                laeufe.push_back(lauf);
                continue;
            }
            LaufErgebnis r;
            if (job.name == "lint:format") {
                r = fahre_lint_format(*yml, wurzel);
            } else if (job.name == "lint:static") {
                r = fahre_lint_static(*yml, wurzel);
            } else {
                r = fahre_lint_secrets(wurzel);
            }
            lauf.deckung   = r.deckung;
            lauf.bemerkung = "Nachbau nach ci-templates " + std::string(bindung->vorlage) + "; " + r.bemerkung;
            if (!r.details.empty() && (r.deckung == vpg::Deckung::rot || ausfuehrlich)) {
                protokoll += "--- " + job.name + " ---\n" + r.details + "\n";
            }
            laeufe.push_back(lauf);
            continue;
        }

        // Sorte A: eigenes Skript in dieser Datei, kein Bau darin.
        const vpg::Befund b = vpg::bauwunsch(job);
        if (b.wunsch == vpg::Bauwunsch::braucht_bau) {
            lauf.deckung   = vpg::Deckung::braucht_bau;
            lauf.bemerkung = "Bau-Beleg: " + b.beleg;
            laeufe.push_back(lauf);
            continue;
        }
        if (b.wunsch == vpg::Bauwunsch::unbekannt) {
            lauf.deckung   = vpg::Deckung::kein_laeufer;
            lauf.bemerkung = b.beleg;
            laeufe.push_back(lauf);
            continue;
        }
        const LaufErgebnis r = fahre_verbatim(job, wurzel);
        lauf.deckung         = r.deckung;
        lauf.bemerkung       = r.bemerkung;
        if (!r.details.empty() && (r.deckung == vpg::Deckung::rot || ausfuehrlich)) {
            protokoll += "--- " + job.name + " ---\n" + r.details + "\n";
        }
        laeufe.push_back(lauf);
    }

    std::cout << "-----------------------------------------------------------------------------\n"
              << " JE JOB\n"
              << "-----------------------------------------------------------------------------\n";
    for (const auto& l : laeufe) {
        std::cout << "  " << (vpg::zaehlt_als_gefahren(l.deckung) ? "[x] " : "[ ] ") << l.name << "\n"
                  << "        " << vpg::deckung_text(l.deckung) << "\n"
                  << "        " << l.bemerkung << "\n";
    }

    if (!protokoll.empty()) {
        std::cout << "\n-----------------------------------------------------------------------------\n"
                  << " AUSGABE DER GEFAHRENEN JOBS\n"
                  << "-----------------------------------------------------------------------------\n"
                  << protokoll;
    }

    const vpg::Zaehlung z = vpg::zaehle(laeufe);
    std::cout << "\n=============================================================================\n"
              << " " << vpg::nenner_satz(laeufe) << "\n"
              << "=============================================================================\n";

    if (z.rot > 0) {
        std::cout << "VOR-PUSH-GATE: ROT -- " << z.rot << " von " << z.gefahren
                  << " gefahrenen Job(s) rot. NICHT pushen.\n";
        return 1;
    }
    std::cout << "VOR-PUSH-GATE: die " << z.gefahren << " HIER gefahrenen Job(s) sind gruen.\n"
              << "Das ist KEINE Aussage ueber die " << (z.nenner - z.gefahren)
              << " oben namentlich genannten, nicht gefahrenen Jobs.\n"
              << "Dieses Werkzeug ersetzt die CI nicht -- es faengt die Klasse, die ohne Bau\n"
              << "erkennbar ist (am 10.08.2026 waren das 2 der 4 roten Jobs).\n";
    return 0;
}
