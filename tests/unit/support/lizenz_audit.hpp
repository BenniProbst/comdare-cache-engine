// lizenz_audit.hpp -- die reinen Erhebungen der Lizenz-Wache.            (2026-08-10)
// =============================================================================
// WARUM ES DIESE DATEI GIBT:
// Am 10.08.2026 stand in EINEM Repo an ZWEI Stellen ZWEIERLEI: NOTICE:7 sagte
// "Licensed under the Apache License, Version 2.0", waehrend LICENSE daneben die
// Dual-Lizenz vom 02.08. trug. NOTICE ist die Datei, auf die Apache selbst
// verweist. Ein Scanner, der eine Quelldatei einzeln liest, meldete Apache-2.0
// fuer 36 Dateien, deren Lizenz seit dem 02.08. eine andere war.
//
// Ein Lizenzwechsel ist kein einmaliger Textvorgang, sondern ein Zustand, der
// auseinanderlaufen kann -- und der genau dann auseinanderlaeuft, wenn niemand
// misst. Diese Datei stellt die Messung bereit; test_lizenz_konsistenz.cpp faellt
// das Urteil.
//
// WARUM GOOGLE TEST UND KEIN SKRIPT (Owner-KERN 09.08.2026, woertlich):
//   "Es waere sauberer im cmake-Debug Modus standard google Tests zu fahren und
//    diese in Release zu wiederholen ... SKRIPTE SAGEN GAR NICHTS."
//
// TRENNUNG: hier steht NUR das Erheben (reine Funktionen ueber einem Wurzelpfad),
// dort das Urteilen. Nur so kann der Koeder dieselbe Funktion ueber einen
// praeparierten Wegwerf-Baum fahren wie ueber den echten -- ein Orakel, das nur
// im echten Baum laufen kann, ist nicht widerlegbar.
//
// ASCII-only, Zeilen <= 120 Byte.
// =============================================================================
#pragma once

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::test::lizenz {

namespace fs = std::filesystem;

// Der EINE Bezeichner. Er steht in LICENSE, in NOTICE und in jedem Quelldatei-Kopf.
// Genau deshalb ist er hier eine einzige Konstante: drei Orte, eine Wahrheit.
inline constexpr std::string_view kBezeichner = "LicenseRef-Comdare-Research-1.0";

// Owner-Entscheid C-2 vom 10.08.2026, woertlich: "Ja, cache engine nach 5 Jahren
// frei verfuegbar ab heute." 10.08.2026 + 5 Jahre = 10.08.2031.
inline constexpr std::string_view kChangeDate = "2031-08-10";

struct Fund {
    std::string datei;
    std::size_t zeile = 0;
    std::string text;
};

// -----------------------------------------------------------------------------
// (1) SPDX-ERNTE -- welche Lizenz behaupten die Dateien selbst?
// -----------------------------------------------------------------------------
struct SpdxErnte {
    std::size_t       dateien_gesehen  = 0; // Nenner: durchsuchte Kandidaten
    std::size_t       dateien_mit_spdx = 0;
    std::size_t       eigen            = 0; // traegt kBezeichner
    std::vector<Fund> fremd;                // traegt etwas anderes  -> Verstoss
    std::vector<Fund> doku_ausnahme;        // dasselbe, aber unter docs/ -> benannt, kein Verstoss
};

namespace detail {

[[nodiscard]] inline bool beginnt_mit(std::string const& s, std::string_view prefix) {
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

// Welche Verzeichnisse die Wache NICHT betritt -- und warum jedes einzelne:
//   ext/       Fremdcode. Seine Lizenzen sind GEWOLLT fremd (LICENSE Abschnitt 8).
//   .git/      keine Quelle.
//   build*/    Erzeugnisse; ihre Koepfe stammen aus den Vorlagen, die geprueft werden.
//   .idea/     Werkzeugkonfiguration.
//
// DIE FALLE, DIE HIER SCHON ZUGESCHNAPPT IST (10.08.2026, im ersten Lauf dieser Wache):
// Die erste Fassung schrieb `beginnt_mit(name, "build")`. Das frisst
// `libs/cache_engine/builder/` mit -- 3 SPDX-Koepfe, der gesamte Bau-Zweig und
// ausgerechnet `COMDARE_WRITER_BACKEND` (builder/CMakeLists.txt:10) verschwanden
// lautlos aus dem Nenner. Es ist dieselbe Falle, die im FALLEN-REGISTER als
// "`grep -v /build` frisst `/builder/`" steht, nur diesmal in C++.
// Deshalb: EXAKTE Namen und ein Trenner, nie ein blosses Praefix.
[[nodiscard]] inline bool ist_uebersprungenes_verzeichnis(std::string const& name) {
    return name == "ext" || name == ".git" || name == ".idea" || name == "node_modules" || name == "build" ||
           beginnt_mit(name, "build-") || beginnt_mit(name, "build_") || beginnt_mit(name, "cmake-build");
}

[[nodiscard]] inline bool ist_kandidat(fs::path const& p) {
    if (p.filename() == "CMakeLists.txt") { return true; }
    std::string const                  e = p.extension().string();
    static std::set<std::string> const kEndungen{".c",  ".cc", ".cxx",   ".cpp", ".h",    ".hpp", ".hxx", ".in",
                                                 ".py", ".sh", ".cmake", ".yml", ".yaml", ".md",  ".txt"};
    return kEndungen.count(e) != 0;
}

[[nodiscard]] inline std::string lies(fs::path const& p) {
    std::ifstream ein{p, std::ios::binary};
    if (!ein.good()) { return {}; }
    std::ostringstream puffer;
    puffer << ein.rdbuf();
    return puffer.str();
}

// WARUM DIE GANZE DATEI GELESEN WIRD UND NICHT NUR DER KOPF:
// Die erste Fassung las 40 Zeilen. Das genuegt fuer Datei-Koepfe -- und uebersieht
// genau die gefaehrlichste Stelle: `_generate_legacy_reimpl.py:147` traegt den
// SPDX-Bezeichner in einer VORLAGE, aus der neue Dateien gestempelt werden. Eine
// Wache, die nur Koepfe liest, laesst die Quelle laufen und raeumt ewig hinterher.
// Also: ganze Datei. Der Preis ist, dass auch Erwaehnungen im Fliesstext zaehlen --
// das ist gewollt, denn in Code- und Bau-Dateien gibt es keinen harmlosen Ort fuer
// eine fremde Lizenz-Behauptung. Fuer Dokumentation gilt die benannte Ausnahme.
} // namespace detail

[[nodiscard]] inline SpdxErnte ernte_spdx(fs::path const& wurzel) {
    SpdxErnte              ernte;
    std::string_view const marke = "SPDX-License-Identifier:";

    std::error_code                  ec;
    fs::recursive_directory_iterator it{wurzel, fs::directory_options::skip_permission_denied, ec};
    fs::recursive_directory_iterator ende;
    for (; it != ende; it.increment(ec)) {
        if (ec) { break; }
        if (it->is_directory(ec)) {
            if (detail::ist_uebersprungenes_verzeichnis(it->path().filename().string())) {
                it.disable_recursion_pending();
            }
            continue;
        }
        if (!it->is_regular_file(ec)) { continue; }
        if (!detail::ist_kandidat(it->path())) { continue; }

        ernte.dateien_gesehen++;
        std::string const inhalt = detail::lies(it->path());
        if (inhalt.find(marke) == std::string::npos) { continue; }

        std::string const rel  = fs::relative(it->path(), wurzel, ec).generic_string();
        bool const        doku = detail::beginnt_mit(rel, "docs/");

        std::istringstream zeilen{inhalt};
        std::string        zeile;
        std::size_t        nr       = 0;
        bool               gezaehlt = false;
        while (std::getline(zeilen, zeile)) {
            nr++;
            std::size_t const pos = zeile.find(marke);
            if (pos == std::string::npos) { continue; }
            std::string wert = zeile.substr(pos + marke.size());
            // fuehrende Leerzeichen und ein evtl. schliessendes Anfuehrungszeichen weg
            std::size_t const erst = wert.find_first_not_of(" \t");
            wert.erase(0, erst == std::string::npos ? wert.size() : erst);
            std::size_t const schluss = wert.find_first_of(" \t\"'*/");
            if (schluss != std::string::npos) { wert.erase(schluss); }
            // Die Marke OHNE Wert ist keine Lizenz-Behauptung, sondern ein Vorkommen der
            // Marke selbst -- z.B. in dieser Datei, die nach ihr sucht. Wer das nicht
            // ausnimmt, baut eine Wache, die sich selbst anzeigt.
            if (wert.empty()) { continue; }
            if (!gezaehlt) {
                ernte.dateien_mit_spdx++;
                gezaehlt = true;
            }
            if (wert == kBezeichner) {
                ernte.eigen++;
            } else if (doku) {
                ernte.doku_ausnahme.push_back({rel, nr, wert});
            } else {
                ernte.fremd.push_back({rel, nr, wert});
            }
        }
    }
    return ernte;
}

// -----------------------------------------------------------------------------
// (2) SCHALTER-ERNTE -- welche Bau-Schalter koennen Fremdcode hereinholen?
//
// Die Vorbedingung KON2-24 sprach von VIER Schaltern, ein Nachtrag von FUENF.
// Beide Zahlen sind am Objekt zu klein: gezaehlt werden hier ALLE, und die
// Wache entscheidet ueber den Hilfetext, nicht ueber eine gemerkte Liste.
// Eine Wache, die eine auswendig gelernte Zahl prueft, uebersieht den Schalter,
// den morgen jemand dazuschreibt.
// -----------------------------------------------------------------------------
struct Schalter {
    std::string name;
    std::string vorgabe; // "ON"/"OFF" bei option(), der Wert bei set(... CACHE ...)
    std::string hilfe;
    std::string datei;
    std::size_t zeile    = 0;
    bool        copyleft = false; // Hilfetext nennt GPL oder LGPL
};

namespace detail {

// option(...) und set(...) duerfen ueber mehrere Zeilen gehen -- CMakeLists.txt:550
// tut es. Ein zeilenweiser Parser haette genau den fuenften Schalter uebersehen.
[[nodiscard]] inline std::string sammle_aufruf(std::string const& inhalt, std::size_t start, std::size_t& ende_aus) {
    int         tiefe = 0;
    std::size_t i     = start;
    for (; i < inhalt.size(); ++i) {
        if (inhalt[i] == '(') { tiefe++; }
        if (inhalt[i] == ')') {
            tiefe--;
            if (tiefe == 0) {
                i++;
                break;
            }
        }
    }
    ende_aus = i;
    return inhalt.substr(start, i - start);
}

[[nodiscard]] inline std::vector<std::string> zerlege(std::string const& aufruf) {
    std::vector<std::string> teile;
    std::string              akt;
    bool                     in_zeichenkette = false;
    for (char const c : aufruf) {
        if (c == '"') {
            in_zeichenkette = !in_zeichenkette;
            if (!in_zeichenkette) {
                teile.push_back(akt);
                akt.clear();
            }
            continue;
        }
        if (in_zeichenkette) {
            akt.push_back(c);
            continue;
        }
        if (std::isspace(static_cast<unsigned char>(c)) != 0 || c == '(' || c == ')') {
            if (!akt.empty()) {
                teile.push_back(akt);
                akt.clear();
            }
            continue;
        }
        akt.push_back(c);
    }
    if (!akt.empty()) { teile.push_back(akt); }
    return teile;
}

[[nodiscard]] inline std::size_t zeilennummer(std::string const& inhalt, std::size_t pos) {
    return static_cast<std::size_t>(std::count(inhalt.begin(), inhalt.begin() + static_cast<long>(pos), '\n')) + 1;
}

[[nodiscard]] inline bool nennt_copyleft(std::string const& s) {
    // "LGPL" enthaelt "GPL" -- beide sind Copyleft, ein Treffer genuegt.
    return s.find("GPL") != std::string::npos;
}

} // namespace detail

[[nodiscard]] inline std::vector<Schalter> ernte_schalter(fs::path const& wurzel) {
    std::vector<Schalter> alle;
    std::error_code       ec;

    fs::recursive_directory_iterator it{wurzel, fs::directory_options::skip_permission_denied, ec};
    fs::recursive_directory_iterator ende;
    for (; it != ende; it.increment(ec)) {
        if (ec) { break; }
        if (it->is_directory(ec)) {
            if (detail::ist_uebersprungenes_verzeichnis(it->path().filename().string())) {
                it.disable_recursion_pending();
            }
            continue;
        }
        if (!it->is_regular_file(ec)) { continue; }
        if (it->path().filename() != "CMakeLists.txt" && it->path().extension() != ".cmake") { continue; }

        std::string const inhalt = detail::lies(it->path());
        std::string const rel    = fs::relative(it->path(), wurzel, ec).generic_string();

        for (std::size_t pos = 0; pos < inhalt.size();) {
            std::size_t const o       = inhalt.find("option(", pos);
            std::size_t const s       = inhalt.find("set(", pos);
            std::size_t const treffer = std::min(o, s);
            if (treffer == std::string::npos) { break; }
            bool const ist_option = (treffer == o);
            // nur Aufrufe am Zeilenanfang (evtl. eingerueckt) -- sonst faengt man Kommentare
            std::size_t const zeilenanfang = inhalt.find_last_of('\n', treffer);
            std::string const davor =
                inhalt.substr(zeilenanfang == std::string::npos ? 0 : zeilenanfang + 1,
                              treffer - (zeilenanfang == std::string::npos ? 0 : zeilenanfang + 1));
            bool const sauber = davor.find_first_not_of(" \t") == std::string::npos;

            std::size_t       nach   = 0;
            std::string const aufruf = detail::sammle_aufruf(inhalt, treffer, nach);
            pos                      = (nach > treffer) ? nach : treffer + 1;
            if (!sauber) { continue; }

            std::vector<std::string> const teile = detail::zerlege(aufruf);
            if (teile.size() < 3) { continue; }
            std::string const& name = teile[1];
            if (name.rfind("COMDARE", 0) != 0) { continue; }

            Schalter sch;
            sch.name  = name;
            sch.datei = rel;
            sch.zeile = detail::zeilennummer(inhalt, treffer);
            if (ist_option) {
                sch.hilfe   = teile[2];
                sch.vorgabe = (teile.size() > 3) ? teile[3] : std::string{"OFF"};
            } else {
                // set(NAME "wert" CACHE TYP "hilfe")
                bool ist_cache = false;
                for (auto const& t : teile) {
                    if (t == "CACHE") { ist_cache = true; }
                }
                if (!ist_cache) { continue; }
                sch.vorgabe = teile[2];
                sch.hilfe   = teile.back();
            }
            sch.copyleft = detail::nennt_copyleft(sch.hilfe);
            alle.push_back(sch);
        }
    }
    return alle;
}

// -----------------------------------------------------------------------------
// (3) VENDOR-EINZUEGE -- welcher Fremdcode wird tatsaechlich in den Bau gezogen?
//
// Das ist die Frage, die eine Schalter-Liste NICHT beantwortet: liburing hat gar
// keinen option()-Schalter, wird aber von zwei CMakeLists per add_subdirectory
// hereingeholt. Wer nur Schalter zaehlt, sieht es nie -- und genau deshalb fehlte
// es bis zum 10.08.2026 im NOTICE.
// -----------------------------------------------------------------------------
struct VendorEinzug {
    std::string komponente; // z.B. "liburing"
    std::string datei;
    std::size_t zeile = 0;
};

[[nodiscard]] inline std::vector<VendorEinzug> ernte_vendor_einzuege(fs::path const& wurzel) {
    std::vector<VendorEinzug> gefunden;
    std::error_code           ec;

    fs::recursive_directory_iterator it{wurzel, fs::directory_options::skip_permission_denied, ec};
    fs::recursive_directory_iterator ende;
    for (; it != ende; it.increment(ec)) {
        if (ec) { break; }
        if (it->is_directory(ec)) {
            if (detail::ist_uebersprungenes_verzeichnis(it->path().filename().string())) {
                it.disable_recursion_pending();
            }
            continue;
        }
        if (!it->is_regular_file(ec)) { continue; }
        if (it->path().filename() != "CMakeLists.txt" && it->path().extension() != ".cmake") { continue; }

        std::string const inhalt = detail::lies(it->path());
        std::string const rel    = fs::relative(it->path(), wurzel, ec).generic_string();

        std::size_t pos = 0;
        while (true) {
            std::size_t const t = inhalt.find("add_subdirectory(", pos);
            if (t == std::string::npos) { break; }
            std::size_t       nach   = 0;
            std::string const aufruf = detail::sammle_aufruf(inhalt, t, nach);
            pos                      = (nach > t) ? nach : t + 1;

            std::size_t const e = aufruf.find("/ext/");
            if (e == std::string::npos) { continue; }
            std::string       rest    = aufruf.substr(e + 5);
            std::size_t const schluss = rest.find_first_of("\"' \t)\n");
            if (schluss != std::string::npos) { rest.erase(schluss); }
            if (rest.empty()) { continue; }
            // letztes Pfadglied ist der Komponentenname (ext/io/liburing -> liburing)
            std::size_t const strich = rest.find_last_of('/');
            std::string const komp   = (strich == std::string::npos) ? rest : rest.substr(strich + 1);
            if (komp.empty()) { continue; }
            gefunden.push_back({komp, rel, detail::zeilennummer(inhalt, t)});
        }
    }
    return gefunden;
}

[[nodiscard]] inline std::string lies_datei(fs::path const& p) { return detail::lies(p); }

} // namespace comdare::test::lizenz
