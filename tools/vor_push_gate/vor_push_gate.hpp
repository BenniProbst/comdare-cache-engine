// vor_push_gate.hpp -- VOR-PUSH-GATE: der KERN. Beantwortet EINE Frage, und zwar mit
// beiden Mengen: WELCHE CI-Jobs habe ich lokal gefahren, und welche NICHT? (2026-08-10)
//
// ===================================================================================================
// DER BEFUND, GEGEN DEN DIESES WERKZEUG GEBAUT IST -- eine Zahl vom 10.08.2026
// ===================================================================================================
// Die ce-Sammellandung (34 Commits, 9 Pakete) fiel mit 4 von 24 Jobs rot. Vor dem Push waren drei
// Pruefungen gefahren worden -- Diff-Hygiene-Wache, gitleaks, eine Eigenpruefung auf ASCII und
// Zeilenbreite -- und alle drei meldeten GRUEN. Danach wurde gezaehlt, welche der 24 CI-Jobs diese
// Vorpruefung ueberhaupt abdeckt:
//
//     Jobs in der ce-Pipeline:                     24
//     davon vor dem Push gefahren:                  0
//
// NULL. Die Diff-Hygiene-Wache ist nicht einmal ein eigener CI-Job (sie laeuft in
// test:coverage-guard mit), gitleaks deckt allenfalls lint:secrets indirekt.
//
// ZWEI der vier roten Jobs waren OHNE JEDEN BAU vermeidbar gewesen:
//     lint:format    8 clang-format-Verstoesse   -- lokal in Sekunden pruefbar
//     lint:static    3 cppcheck-Warnungen        -- lokal in Minuten pruefbar
// Sie sind trotzdem in die CI gegangen, haben eine Pipeline auf einem Blech verbrannt und
// 'development' rot hinterlassen, waehrend sieben weitere Pakete darauf warteten.
//
// ===================================================================================================
// WARUM EIN WERKZEUG UND KEIN VORSATZ
// ===================================================================================================
// Der Wellenplan misst es (Paragraph 11.3): von zehn Verschaerfungen hielten 7 von 10 -- genau die,
// die in ein WERKZEUG gebrannt wurden; die drei rein disziplinaeren kamen zurueck. Und: "Zufall,
// Disziplin und Maschine sehen im Rueckblick identisch gruen aus. Unterscheidbar nur an: was
// erzwingt das Halten?"
// Der Lead hat die Regel "ein gruenes Gate deckt nur seinen eigenen Gegenstand" am 10.08. DREIMAL
// zitiert und ist trotzdem hineingelaufen. Wissen traegt hier nicht. Ein Nenner traegt.
//
// ===================================================================================================
// WAS DIESES WERKZEUG NICHT TUT -- die Grenze steht hier, nicht in einem Bericht
// ===================================================================================================
// Es ERSETZT DIE CI NICHT und behauptet das nirgends. Es verhindert genau die Fehlerklasse, die
// ohne Bau erkennbar gewesen waere -- am 10.08. waren das 2 von 4 roten Jobs. Alles, was ein
// Bauverzeichnis braucht (test:unit, test:coverage-guard, contract:*, sanitize:*, pmc:*), wird
// NICHT heimlich uebersprungen, sondern namentlich als "braucht Bau -- nicht gefahren" gemeldet.
// Ein gruener Gesamtbefund dieses Werkzeugs heisst NIE "landefaehig". Er heisst: die HIER
// gefahrenen Jobs sind gruen, und die anderen stehen daneben, mit Namen.
//
// ===================================================================================================
// DIE DREI TRAGENDEN ENTSCHEIDUNGEN
// ===================================================================================================
// (1) DER NENNER IST FREMD (T-3). Die Job-Liste wird bei JEDEM Lauf aus .gitlab-ci.yml gelesen --
//     es gibt in diesem Werkzeug KEINE Job-Liste. Kommt ein Job in die Datei, waechst der Nenner
//     von allein, und der neue Job landet automatisch in der Menge "nicht gefahren". Ein Nenner,
//     den man nicht bewegen kann, ist eine Konstante und kein Nenner.
//
// (2) FAIL-CLOSED BEI UNKENNTNIS. Ein Job, dessen Skript NICHT in dieser Datei steht (er erbt von
//     einer Vorlage im Projekt ci-templates), gilt als NICHT gefahren -- nicht als harmlos. Die
//     einzige Ausnahme sind die drei Jobs mit einem ausdruecklich erklaerten NACHBAU, und selbst
//     der ist an eine Bedingung geknuepft, siehe (3).
//
// (3) EIN JOB MIT ANDEREN OPTIONEN IST NICHT DERSELBE JOB. Fuer lint:format/lint:static/lint:secrets
//     liegt das Skript in einem FREMDEN Projekt; hier laeuft ein NACHBAU. Damit der Nachbau nicht
//     unbemerkt vom Original wegdriftet, prueft das Werkzeug bei jedem Lauf, ob der Job noch die
//     Vorlage erbt, fuer die der Nachbau geschrieben wurde. Erbt er sie nicht mehr, faellt der Job
//     auf "abweichend gefahren" zurueck und zaehlt NICHT als gedeckt.
//
// ===================================================================================================
// DAS PARSEN: ein strenger ZEILEN-Scanner, kein YAML-Parser (T-9-Kritik, benannt statt versteckt)
// ===================================================================================================
// Uebernommen aus super Code/ci_wachen/ci_yml_scanner.hpp, einschliesslich der dort teuer gelernten
// Regel: EINE ZEILE IN SPALTE 0 BEENDET DEN BLOCK, KOMMENTARZEILEN AUSDRUECKLICH EINGESCHLOSSEN.
// Nimmt man '#' vom Blockende aus, laeuft der Block in den Kommentarkopf des NAECHSTEN Jobs und
// erbt dessen Inhalt. Genau das ist beim Kalibrieren dieses Werkzeugs am 10.08. passiert: ein
// awk-Probelauf hielt lint:xml-wellformed faelschlich fuer einen Bau-Job, weil der Kommentarkopf
// von build:clang das Wort 'ctest' traegt. Ein eigener Testfall friert die Regel ein.
// NICHT gesehen werden: YAML-Anker, include-Dateien, rules-Gates (ein per Variable abgeschalteter
// Job zaehlt hier im Nenner mit). Das ist benannt, nicht behoben.
//
// ASCII-only, Zeilen <= 120 Byte.

#ifndef COMDARE_VOR_PUSH_GATE_HPP
#define COMDARE_VOR_PUSH_GATE_HPP

#include <algorithm>
#include <cstddef>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::vor_push_gate {

// ---------------------------------------------------------------------------------------------
// Grundlagen
// ---------------------------------------------------------------------------------------------

inline bool ist_kommentarzeile(std::string_view zeile) {
    const auto erste = zeile.find_first_not_of(" \t");
    return erste != std::string_view::npos && zeile[erste] == '#';
}

// Die reservierten GitLab-Schluesselwoerter auf oberster Ebene. Sie sehen syntaktisch aus wie
// Jobs, sind aber keine. Diese Liste ist NICHT der Nenner -- sie ist sein Komplement, und sie
// ist kurz und stabil (GitLab-Schluesselwoerter aendern sich in Jahren, Jobs woechentlich).
inline bool ist_reserviert(std::string_view schluessel) {
    static constexpr std::string_view kReserviert[] = {
        "include",  "workflow",      "stages",       "variables", "default", "image",
        "services", "before_script", "after_script", "cache",     "spec",
    };
    return std::find(std::begin(kReserviert), std::end(kReserviert), schluessel) != std::end(kReserviert);
}

// Ist diese Zeile der Kopf eines Job-Blocks? Bedingungen, alle noetig:
//   * Spalte 0 (keine Einrueckung)  * beginnt mit Buchstabe/Unterstrich -- damit fallen die
//   Vorlagen '.bare_metal'/'.ccache-pull' heraus, die per Punkt-Praefix versteckt sind
//   * endet mit ':' plus optionalem Kommentar -- ein Schluessel MIT Wert ('image: x') oeffnet
//     keinen Job-Block  * ist kein reserviertes Schluesselwort
inline std::optional<std::string> job_kopf(std::string_view zeile) {
    static const std::regex kKopf(R"(^([A-Za-z_][A-Za-z0-9_.:-]*):[ \t]*(#.*)?$)");
    std::cmatch             treffer;
    if (!std::regex_match(zeile.data(), zeile.data() + zeile.size(), treffer, kKopf)) { return std::nullopt; }
    std::string name = treffer[1].str();
    if (ist_reserviert(name)) { return std::nullopt; }
    return name;
}

struct JobBlock {
    std::string              name;
    std::size_t              erste_zeile = 0; // 1-basiert, wie ein Editor zaehlt
    std::vector<std::string> zeilen;          // OHNE die Kopfzeile: nur der eingerueckte Rumpf
};

// DER NENNER. Die einzige Stelle, an der die Job-Menge entsteht -- und sie entsteht aus der
// fremden Datei, nicht aus einer Liste hier.
inline std::vector<JobBlock> job_bloecke(const std::vector<std::string>& zeilen) {
    std::vector<JobBlock> jobs;
    bool                  im_block = false;
    for (std::size_t i = 0; i < zeilen.size(); ++i) {
        const std::string& z           = zeilen[i];
        const bool         spalte_null = !z.empty() && z[0] != ' ' && z[0] != '\t';
        if (spalte_null) {
            // SPALTE 0 BEENDET DEN BLOCK -- auch eine Kommentarzeile. Siehe Kopf dieser Datei.
            im_block = false;
            if (auto name = job_kopf(z)) {
                jobs.push_back(JobBlock{*name, i + 1, {}});
                im_block = true;
            }
            continue;
        }
        if (im_block && !jobs.empty()) { jobs.back().zeilen.push_back(z); }
    }
    return jobs;
}

// ---------------------------------------------------------------------------------------------
// Klassifikation: braucht dieser Job ein Bauverzeichnis?
// ---------------------------------------------------------------------------------------------

enum class Bauwunsch {
    ohne_bau,    // der Job faehrt reinen Text/Parser-Kram, kein Compiler noetig
    braucht_bau, // im Job-Block steht cmake/ctest -- ohne Bauverzeichnis geht er nicht
    unbekannt,   // der Job erbt sein Skript von einer Vorlage AUSSERHALB dieser Datei
};

struct Befund {
    Bauwunsch   wunsch = Bauwunsch::unbekannt;
    std::string beleg; // die Zeile bzw. die Vorlage, auf die sich das Urteil stuetzt
};

inline std::string extends_ziel(const JobBlock& block) {
    static const std::regex kExtends(R"(^[ \t]+extends:[ \t]*(.*)$)");
    for (const auto& z : block.zeilen) {
        if (ist_kommentarzeile(z)) { continue; }
        std::smatch treffer;
        if (std::regex_match(z, treffer, kExtends)) {
            std::string wert  = treffer[1].str();
            const auto  raute = wert.find('#');
            if (raute != std::string::npos) { wert.erase(raute); }
            while (!wert.empty() && (wert.back() == ' ' || wert.back() == '\t')) { wert.pop_back(); }
            return wert;
        }
    }
    return {};
}

inline bool hat_eigenes_script(const JobBlock& block) {
    static const std::regex kScript(R"(^[ \t]+script:[ \t]*$)");
    for (const auto& z : block.zeilen) {
        if (!ist_kommentarzeile(z) && std::regex_match(z, kScript)) { return true; }
    }
    return false;
}

// Die Bau-Verraeter. Sie werden NUR in Nicht-Kommentarzeilen des Job-Blocks gesucht -- ein
// 'ctest' in einem Kommentarkopf ist kein Bau (genau dieser Fehlgriff ist beim Kalibrieren
// passiert und steht als Testfall fest).
inline Befund bauwunsch(const JobBlock& block) {
    static constexpr std::string_view kVerraeter[] = {"cmake", "ctest", "ce_ctest", "--build"};
    for (const auto& z : block.zeilen) {
        if (ist_kommentarzeile(z)) { continue; }
        for (const auto v : kVerraeter) {
            if (z.find(v) != std::string::npos) {
                std::string beleg  = z;
                const auto  anfang = beleg.find_first_not_of(" \t");
                if (anfang != std::string::npos) { beleg.erase(0, anfang); }
                if (beleg.size() > 90) { beleg = beleg.substr(0, 87) + "..."; }
                return Befund{Bauwunsch::braucht_bau, beleg};
            }
        }
    }
    if (hat_eigenes_script(block)) { return Befund{Bauwunsch::ohne_bau, "eigenes script:, kein cmake/ctest darin"}; }
    const std::string ziel = extends_ziel(block);
    return Befund{Bauwunsch::unbekannt, ziel.empty()
                                            ? std::string("weder eigenes script: noch extends:")
                                            : ("Skript liegt in der Vorlage " + ziel + " (Projekt ci-templates)")};
}

// Die einzeiligen Kommandos eines eigenen script:-Blocks. MEHRZEILIGE Kommandos ('- |') werden
// bewusst NICHT zurueckgegeben: ein Blockskript verbatim nachzufahren ist eine Quelle stiller
// Abweichung. Wer nichts bekommt, faellt auf "kein Laeufer" -- fail-closed.
inline std::vector<std::string> script_kommandos(const JobBlock& block) {
    static const std::regex  kScript(R"(^[ \t]+script:[ \t]*$)");
    static const std::regex  kEintrag(R"(^[ \t]+-[ \t]+(.*)$)");
    std::vector<std::string> kommandos;
    bool                     im_script = false;
    for (const auto& z : block.zeilen) {
        if (ist_kommentarzeile(z)) { continue; }
        if (std::regex_match(z, kScript)) {
            im_script = true;
            continue;
        }
        if (!im_script) { continue; }
        std::smatch treffer;
        if (std::regex_match(z, treffer, kEintrag)) {
            std::string k = treffer[1].str();
            if (k == "|" || k == ">" || k.rfind("|", 0) == 0) {
                return {}; // Blockskript: nicht verbatim fahrbar
            }
            kommandos.push_back(k);
            continue;
        }
        // eine andere Schluesselzeile auf script:-Ebene beendet den script:-Block
        static const std::regex kSchluessel(R"(^[ \t]+[A-Za-z_][A-Za-z0-9_-]*:.*$)");
        if (std::regex_match(z, kSchluessel)) { im_script = false; }
    }
    return kommandos;
}

// ---------------------------------------------------------------------------------------------
// Variablen aus der .gitlab-ci.yml (COMDARE_LINT_PATHS und Geschwister)
// ---------------------------------------------------------------------------------------------

inline std::optional<std::string> variable(const std::vector<std::string>& zeilen, std::string_view name) {
    // Eigener Rohstring-Trenner RX: das Muster enthaelt selbst die Folge )" und wuerde einen
    // nackten R"( ... )"-Literal vorzeitig beenden.
    const std::regex kVar("^[ \t]+" + std::string(name) + R"RX(:[ \t]*"?([^"#]*?)"?[ \t]*(#.*)?$)RX");
    for (const auto& z : zeilen) {
        if (ist_kommentarzeile(z)) { continue; }
        std::smatch treffer;
        if (std::regex_match(z, treffer, kVar)) { return treffer[1].str(); }
    }
    return std::nullopt;
}

// ---------------------------------------------------------------------------------------------
// Die Dateimenge von lint:format / lint:static
// ---------------------------------------------------------------------------------------------
// WARUM DAS HIER STEHT UND NICHT IM AUFRUFER: es ist die Stelle, an der die bisherige lokale
// Vorpruefung vom CI-Job ABWICH und deshalb gruen sein konnte, waehrend die CI rot war. Die CI
// prueft `git ls-files -- $COMDARE_LINT_PATHS` -- den GANZEN versionierten Bestand. Die
// bisherige Wache (scripts/vor_push_alle_wachen.sh) prueft nur die im Diff BERUEHRTEN Dateien.
// Ein Verstoss in einer unberuehrten Datei ist damit lokal unsichtbar und in der CI rot.
// Diese Funktion bildet die CI-Menge nach, nicht die Diff-Menge.
inline std::vector<std::string> lint_dateien(const std::vector<std::string>& versionierte,
                                             std::string_view                ausschluss_re) {
    static const std::regex  kEndung(R"(\.(c|cc|cxx|cpp|h|hh|hxx|hpp)$)");
    const std::regex         kAus(std::string{ausschluss_re});
    std::vector<std::string> treffer;
    for (const auto& d : versionierte) {
        if (d.empty()) { continue; }
        if (!std::regex_search(d, kEndung)) { continue; }
        if (!ausschluss_re.empty() && std::regex_search(d, kAus)) { continue; }
        treffer.push_back(d);
    }
    return treffer;
}

// ---------------------------------------------------------------------------------------------
// Der Bericht -- und hier liegt der eigentliche Gegenstand des Werkzeugs
// ---------------------------------------------------------------------------------------------

enum class Deckung {
    gruen,         // lokal gefahren, Ergebnis gruen
    rot,           // lokal gefahren, Ergebnis rot
    abweichend,    // gefahren, aber nicht mit den Optionen der CI -> zaehlt NICHT als gedeckt
    braucht_bau,   // nicht gefahren: braucht ein Bauverzeichnis
    kein_laeufer,  // nicht gefahren: kein lokaler Laeufer bekannt
    werkzeug_fehlt // nicht gefahren: Laeufer da, Werkzeug nicht auffindbar
};

// GEDECKT ist nur, was WIRKLICH mit den Optionen der CI gelaufen ist. 'abweichend' zaehlt
// ausdruecklich nicht -- sonst waere die Zahl im Zaehler eine Behauptung ueber einen anderen
// Lauf als den, der zaehlt.
inline bool zaehlt_als_gefahren(Deckung d) { return d == Deckung::gruen || d == Deckung::rot; }

inline std::string_view deckung_text(Deckung d) {
    switch (d) {
        case Deckung::gruen: return "GEFAHREN, gruen";
        case Deckung::rot: return "GEFAHREN, ROT";
        case Deckung::abweichend: return "abweichend gefahren -- zaehlt NICHT als gedeckt";
        case Deckung::braucht_bau: return "braucht Bau -- nicht gefahren";
        case Deckung::kein_laeufer: return "kein lokaler Laeufer -- nicht gefahren";
        case Deckung::werkzeug_fehlt: return "Werkzeug fehlt -- nicht gefahren";
    }
    return "unbekannt -- nicht gefahren";
}

struct Lauf {
    std::string name;
    Deckung     deckung = Deckung::kein_laeufer;
    std::string bemerkung;
};

struct Zaehlung {
    std::size_t nenner   = 0; // ALLE Jobs aus .gitlab-ci.yml
    std::size_t gefahren = 0; // davon lokal gefahren (gruen ODER rot)
    std::size_t rot      = 0;
};

inline Zaehlung zaehle(const std::vector<Lauf>& laeufe) {
    Zaehlung z;
    z.nenner = laeufe.size();
    for (const auto& l : laeufe) {
        if (zaehlt_als_gefahren(l.deckung)) { ++z.gefahren; }
        if (l.deckung == Deckung::rot) { ++z.rot; }
    }
    return z;
}

// DIE ZWEITE ZEILE IST DIE HAUPTSACHE. Ohne die namentliche Nennung der NICHT gefahrenen Jobs
// waere dieses Werkzeug selbst ein gruenes Gate ohne Gegenstand -- also genau der Fehler, den es
// verhindern soll, eine Ebene hoeher.
inline std::string nenner_satz(const std::vector<Lauf>& laeufe) {
    const Zaehlung           z = zaehle(laeufe);
    std::vector<std::string> offen;
    for (const auto& l : laeufe) {
        if (!zaehlt_als_gefahren(l.deckung)) { offen.push_back(l.name); }
    }
    std::string s = std::to_string(z.gefahren) + " von " + std::to_string(z.nenner) +
                    " CI-Jobs lokal gefahren; die uebrigen " + std::to_string(offen.size()) + " namentlich: ";
    if (offen.empty()) {
        s += "(keine)";
        return s;
    }
    for (std::size_t i = 0; i < offen.size(); ++i) {
        s += offen[i];
        if (i + 1 < offen.size()) { s += ", "; }
    }
    return s;
}

} // namespace comdare::vor_push_gate

#endif // COMDARE_VOR_PUSH_GATE_HPP
