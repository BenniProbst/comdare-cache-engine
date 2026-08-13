// S-14a BUMP-WACHE TEIL 1 -- DER RIEGEL (Task #33; Plan 13.3 #90; Owner KON17-03 'Wache Modular
// erweitern und in Detail-Klassen splitten'). Nachfolger des PAKET-W3-C-Tools (GN-8/O-4, Ledger
// Sec.32-F4): Content-Digest-Tripwire ueber die algo_version-Traeger des Baums.
//
// WAS SICH GEGENUEBER v1 AENDERT (und warum):
//   * DISCOVERY STATT ARGV-LISTE. v1 pruefte genau die Header, die ihm der Aufrufer nannte -- die
//     hartkodierte 6-Pfad-Liste in der CI-YAML war Mitursache des Drei-Wochen-Ausfalls, und ein
//     verwaister Lock-Eintrag blieb prinzipiell unsichtbar (do_check iterierte nur ueber argv, nie
//     ueber Lock-Eintraege; am 13.08.2026 literal belegt: Phantom-Eintrag -> 'GRUEN', Exit 0).
//     v2 erhebt seine Grundgesamtheit SELBST unter --root und prueft BIDIREKTIONAL.
//   * KATEGORIE-DETAIL-KLASSEN (KON17-03): HeuristikDetail traegt den heutigen Mechanismus
//     UNVERAENDERT weiter, OrganDetail ist neu (axes/ + topics/queuing/). EINE Wache, EIN Lock
//     (KON27-01/KON17-03) -- die Kategorien sind Spalten desselben Locks, kein zweites Lockfile.
//   * ZWEI RIEGEL-SCHAERFUNGEN: (i) Lock-Eintrag ohne Datei => ROT (verwaist); (ii) je Kategorie
//     ein discovered-Zaehler MIT NENNER in der Ausgabe (BESTAND-Zeilen), damit die Zahl gefahren
//     wird statt behauptet.
//
// BENANNTE LEERSTELLE (kein stiller Default): S-14a Teil 1 legt genau ZWEI Detail-Klassen an.
// SYSTEM- und MESS-Detail-Klassen werden BEWUSST NICHT angelegt -- ihre Homes existieren im Baum
// noch nicht (#16 Umgliederung kommt erst; queuing ist hier im HEUTIGEN Pfad festgeschrieben,
// golden VOR Umgliederung). Wer die Homes anlegt, ergaenzt die Detail-Klasse UND haengt sie in
// write_lock()/check_lock() ein; die Struktur (discover/version_of/bump_ok) ist darauf ausgelegt.
//
// KATEGORIEN UND IHRE REGELN:
//   heuristik  Home libs/cache_engine/heuristik/, jede *.hpp ist Traeger. Version = Integer aus dem
//              ASCII-Marker '// AXIS_ALGO_VERSION: <N>' (fehlt -> 0 + sichtbare WARNUNG, wie v1).
//              bump_ok = Integer echt groesser. Mechanismus UNVERAENDERT aus v1.
//   organ      Homes libs/cache_engine/axes/ + libs/cache_engine/topics/queuing/, rekursiv *.hpp.
//              DISCOVERED ist jede Datei, die den Substring 'algo_version' enthaelt -- exakt die
//              golden-Erhebung (grep -l) der Baseline vom 13.08.2026, damit deren 158er-Schnitt
//              digest-stabil weiterlebt. TRAEGER ist darunter jede Datei mit einer echten
//              algo_version-String-Literal-Zuweisung IM CODE (kommentar-/string-bewusster Scan mit
//              Wortgrenzen; Forwarder wie '= Strategy::algo_version' und Prosa/Concept-Erwaehnungen
//              fallen automatisch heraus und stehen als version='-' digest-only im Lock).
//              declared_version = ERSTES Literal 'X.Y.Z[.flags]'. bump_ok = X.Y.Z-Tripel echt
//              groesser, geparst ueber den BESTANDS-Parser measurement/algo_semver.hpp (EIN Parser,
//              keine Zweitgrammatik); unparsbares Literal ist Sentinel => ROT.
//
// LOCK-FORMAT v2 (v1 wird mit klarer Meldung abgewiesen, kein stilles Weiterlesen):
//   # format: v2
//   ZWEIZEILEN-RECORD je Datei:  '<category> <version> <relative-path>' + Folgezeile
//   '    <sha256-hex>' (vier Leerzeichen Einrueckung). WARUM ZWEI ZEILEN: 64 Hex-Zeichen + Pfad
//   passen strukturell nicht in die 120-Spalten-Diff-Hygiene (ci_diff_ascii_width_guard); das
//   Zweizeilen-Format haelt JEDE Zeile der Datei wachen-gedeckt, statt eine Ausnahme-Klasse in
//   der Wache zu eroeffnen. Records global nach Pfad sortiert; version bei organ = Literal-STRING
//   bzw. '-'. Eine Kopfzeile ohne Digest-Folgezeile ist ein LAUTER Formatfehler (kein Ueberlesen).
//
// AUFRUF (CI ruft ohne Datei-Liste; die Grundgesamtheit erhebt das Tool):
//   axis_version_lock --write <lockfile> [--root <repo-root>]
//   axis_version_lock --check <lockfile> [--root <repo-root>]
// Exit: 0 = gruen, 1 = ROT (Tripwire), 2 = Usage/Umgebungsfehler (z.B. Home fehlt unter --root).
//
// BETRIEBSFOLGE (deklariert): ab Landung erzwingt JEDE Aenderung an einem organ-Traeger einen
// Version-Bump ODER einen bewussten Lock-Regen-Commit (--write); Forwarder-/Prosa-Dateien haben
// keinen Bump-Pfad und verlangen IMMER den bewussten Regen-Commit. Das ist gewollt (trifft S-6/S-7).
//
// SHA-256: WIEDERVERWENDUNG der vorhandenen consteval/constexpr-Implementation
// (libs/cache_engine/src/sha256/ctsha.hpp, RFC 6234). KEINE zweite SHA-Implementation.
// Der CT-Zaehler kAllRegisteredOrganVariantCount wird ABSICHTLICH NICHT hier eingebunden (die
// Varianten-Tabelle ist eine schwere Registry-TU; dieses Tool bleibt eine schlanke Datei-Wache).
// Der ctest test_s14_axis_version_lock_tripwire loggt ihn NEBEN den BESTAND-Zaehlern -- nur loggen,
// nie gleichsetzen (Datei != Variante).
//
// INERT-by-default (Anti-Phantom): EXCLUDE_FROM_ALL; gebaut vom CI-Job contract:axis-version-lock
// und als Fixture-Dependency des ctest.

#include <sha256/ctsha.hpp> // comdare::cache_engine::sha256::{sha256, to_hex, Digest}

#include <cache_engine/measurement/algo_semver.hpp> // BESTANDS-Parser parse_algo_semver (X.Y.Z[.flags])

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

namespace fs   = std::filesystem;
namespace sha  = ::comdare::cache_engine::sha256;
namespace meas = ::comdare::cache_engine::measurement;

/// Rohe Bytes einer Datei. false = nicht lesbar.
[[nodiscard]] bool read_file_bytes(fs::path const& path, std::vector<std::uint8_t>& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string const s = ss.str();
    out.assign(s.begin(), s.end());
    return true;
}

[[nodiscard]] std::string digest_hex_of(std::vector<std::uint8_t> const& bytes) {
    sha::Digest const d = sha::sha256(std::span<const std::uint8_t>{bytes.data(), bytes.size()});
    auto const        h = sha::to_hex(d);
    return std::string(h.begin(), h.end());
}

[[nodiscard]] bool ist_ident_zeichen(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

// ================================================================================================
// KATEGORIE-DETAIL: heuristik -- der v1-Mechanismus, UNVERAENDERT weitergetragen.
// ================================================================================================
struct HeuristikDetail {
    static constexpr std::string_view kName = "heuristik";

    /// Jede regulaere *.hpp unter dem Home ist Traeger (rekursiv; heute 6 flache Dateien).
    [[nodiscard]] static std::vector<std::string> discover(fs::path const& root, bool& home_ok) {
        std::vector<std::string> rels;
        fs::path const           home = root / "libs/cache_engine/heuristik";
        home_ok                       = fs::is_directory(home);
        if (!home_ok) return rels;
        for (auto const& de : fs::recursive_directory_iterator(home)) {
            if (!de.is_regular_file() || de.path().extension() != ".hpp") continue;
            rels.push_back(fs::relative(de.path(), root).generic_string());
        }
        std::sort(rels.begin(), rels.end());
        return rels;
    }

    /// Version = Integer aus '// AXIS_ALGO_VERSION: <N>'. Fehlt der Marker: "0" + sichtbare WARNUNG
    /// (kein stiller Default) -- exakt die v1-Diagnose.
    [[nodiscard]] static std::string version_of(std::vector<std::uint8_t> const& bytes, std::string const& rel) {
        static constexpr std::string_view kMarker = "AXIS_ALGO_VERSION:";
        std::string_view const            text(reinterpret_cast<char const*>(bytes.data()), bytes.size());
        std::size_t const                 pos = text.find(kMarker);
        if (pos != std::string_view::npos) {
            std::size_t i = pos + kMarker.size();
            while (i < text.size() && (text[i] == ' ' || text[i] == '\t')) ++i;
            std::uint64_t v     = 0;
            bool          digit = false;
            while (i < text.size() && text[i] >= '0' && text[i] <= '9') {
                v     = v * 10 + static_cast<std::uint64_t>(text[i] - '0');
                digit = true;
                ++i;
            }
            if (digit) return std::to_string(v);
        }
        std::fprintf(stderr, "axis_version_lock: WARNUNG kein AXIS_ALGO_VERSION-Marker in %s -> version=0\n",
                     rel.c_str());
        return "0";
    }

    /// bump_ok = Integer echt groesser (v1-Regel).
    [[nodiscard]] static bool bump_ok(std::string const& alt, std::string const& neu) {
        char*                    ende_a = nullptr;
        char*                    ende_n = nullptr;
        unsigned long long const a      = std::strtoull(alt.c_str(), &ende_a, 10);
        unsigned long long const n      = std::strtoull(neu.c_str(), &ende_n, 10);
        if (ende_a == alt.c_str() || *ende_a != '\0') return false;
        if (ende_n == neu.c_str() || *ende_n != '\0') return false;
        return n > a;
    }
};

// ================================================================================================
// KATEGORIE-DETAIL: organ -- NEU (S-14a). Discovery ueber axes/ + topics/queuing/.
// ================================================================================================
struct OrganDetail {
    static constexpr std::string_view kName = "organ";

    /// discovered = *.hpp unter den zwei Homes, die den Substring 'algo_version' enthalten
    /// (golden-kompatible Erhebung, s. Kopf). Sortiert, '/'-Separatoren.
    [[nodiscard]] static std::vector<std::string> discover(fs::path const& root, bool& home_ok) {
        std::vector<std::string> rels;
        home_ok = true;
        for (char const* home_rel : {"libs/cache_engine/axes", "libs/cache_engine/topics/queuing"}) {
            fs::path const home = root / home_rel;
            if (!fs::is_directory(home)) {
                home_ok = false;
                std::fprintf(stderr, "axis_version_lock: FEHLER organ-Home fehlt unter --root: %s\n", home_rel);
                continue;
            }
            for (auto const& de : fs::recursive_directory_iterator(home)) {
                if (!de.is_regular_file() || de.path().extension() != ".hpp") continue;
                std::vector<std::uint8_t> bytes;
                if (!read_file_bytes(de.path(), bytes)) continue; // unlesbar faellt beim Digest-Schritt laut auf
                std::string_view const text(reinterpret_cast<char const*>(bytes.data()), bytes.size());
                if (text.find("algo_version") == std::string_view::npos) continue;
                rels.push_back(fs::relative(de.path(), root).generic_string());
            }
        }
        std::sort(rels.begin(), rels.end());
        return rels;
    }

    /// declared_version = ERSTES algo_version-String-Literal IM CODE. Kommentar-/string-bewusster
    /// Scan (Zeilen-/Blockkommentare, "..."-Strings und '...'-Chars werden uebersprungen; Wortgrenzen
    /// am Token). Kein Literal -> "-" (Forwarder/Prosa; digest-only). Der Scan kennt KEINE
    /// Raw-Strings -- in den Traeger-Baeumen gibt es keine; taucht einer auf, faellt er als
    /// Digest-Aenderung auf, nie als stilles Gruen.
    [[nodiscard]] static std::string version_of(std::vector<std::uint8_t> const& bytes, std::string const&) {
        std::string_view const            text(reinterpret_cast<char const*>(bytes.data()), bytes.size());
        static constexpr std::string_view kTok = "algo_version";
        enum class Z { code, zeilen_kommentar, block_kommentar, string_lit, char_lit };
        Z           z = Z::code;
        std::size_t i = 0;
        while (i < text.size()) {
            char const c = text[i];
            switch (z) {
                case Z::zeilen_kommentar:
                    if (c == '\n') z = Z::code;
                    ++i;
                    continue;
                case Z::block_kommentar:
                    if (c == '*' && i + 1 < text.size() && text[i + 1] == '/') {
                        z = Z::code;
                        i += 2;
                    } else {
                        ++i;
                    }
                    continue;
                case Z::string_lit:
                    if (c == '\\') {
                        i += 2;
                    } else {
                        if (c == '"') z = Z::code;
                        ++i;
                    }
                    continue;
                case Z::char_lit:
                    if (c == '\\') {
                        i += 2;
                    } else {
                        if (c == '\'') z = Z::code;
                        ++i;
                    }
                    continue;
                case Z::code: break;
            }
            if (c == '/' && i + 1 < text.size() && text[i + 1] == '/') {
                z = Z::zeilen_kommentar;
                i += 2;
                continue;
            }
            if (c == '/' && i + 1 < text.size() && text[i + 1] == '*') {
                z = Z::block_kommentar;
                i += 2;
                continue;
            }
            if (c == '"') {
                z = Z::string_lit;
                ++i;
                continue;
            }
            if (c == '\'') {
                z = Z::char_lit;
                ++i;
                continue;
            }
            if (c == 'a' && text.compare(i, kTok.size(), kTok) == 0) {
                bool const        grenze_vor  = (i == 0) || !ist_ident_zeichen(text[i - 1]);
                std::size_t const nach        = i + kTok.size();
                bool const        grenze_nach = (nach >= text.size()) || !ist_ident_zeichen(text[nach]);
                if (grenze_vor && grenze_nach) {
                    std::size_t j = nach;
                    while (j < text.size() && (text[j] == ' ' || text[j] == '\t' || text[j] == '\n' || text[j] == '\r'))
                        ++j;
                    if (j < text.size() && text[j] == '=') {
                        ++j;
                        while (j < text.size() &&
                               (text[j] == ' ' || text[j] == '\t' || text[j] == '\n' || text[j] == '\r'))
                            ++j;
                        if (j < text.size() && text[j] == '"') {
                            std::size_t const anfang = j + 1;
                            std::size_t       ende   = anfang;
                            while (ende < text.size() && text[ende] != '"' && text[ende] != '\n') ++ende;
                            if (ende < text.size() && text[ende] == '"')
                                return std::string(text.substr(anfang, ende - anfang));
                        }
                    }
                }
                i += kTok.size();
                continue;
            }
            ++i;
        }
        return "-";
    }

    /// bump_ok = X.Y.Z-Tripel echt groesser (lexikographisch), beide Seiten ueber den BESTANDS-Parser.
    /// '-' (kein Literal) hat KEINEN Bump-Pfad; Sentinel (unparsbar) ist nie ein gueltiger Bump.
    [[nodiscard]] static bool bump_ok(std::string const& alt, std::string const& neu) {
        if (alt == "-" || neu == "-") return false;
        meas::AlgoSemVer const a = meas::parse_algo_semver(alt);
        meas::AlgoSemVer const n = meas::parse_algo_semver(neu);
        if (a.is_sentinel() || n.is_sentinel()) return false;
        if (n.x != a.x) return n.x > a.x;
        if (n.y != a.y) return n.y > a.y;
        return n.z > a.z;
    }

    /// Sichtbare Sentinel-Diagnose (Koeder E): unparsbares Literal wird BEIM NAMEN genannt.
    [[nodiscard]] static bool literal_unparsbar(std::string const& version) {
        return version != "-" && meas::parse_algo_semver(version).is_sentinel();
    }
};

// ================================================================================================
// Lock-Datei v2: Lesen/Schreiben + der bidirektionale Check.
// ================================================================================================
struct Befund {
    std::string category;
    std::string version;
    std::string digest_hex;
};

using BefundMap = std::map<std::string, Befund>; // key = relativer Pfad (sortiert => deterministisch)

/// Erhebt die Grundgesamtheit beider Kategorien unter root. false = Home-/Lesefehler (Usage-Klasse).
[[nodiscard]] bool erhebe_bestand(fs::path const& root, BefundMap& out, int& heuristik_n, int& organ_n,
                                  int& organ_traeger_n) {
    bool ok      = true;
    bool home_ok = false;

    std::vector<std::string> const h_rels = HeuristikDetail::discover(root, home_ok);
    if (!home_ok) {
        std::fprintf(stderr, "axis_version_lock: FEHLER heuristik-Home fehlt unter --root: %s\n",
                     "libs/cache_engine/heuristik");
        ok = false;
    }
    for (std::string const& rel : h_rels) {
        std::vector<std::uint8_t> bytes;
        if (!read_file_bytes(root / rel, bytes)) {
            std::fprintf(stderr, "axis_version_lock: FEHLER Datei nicht lesbar: %s\n", rel.c_str());
            ok = false;
            continue;
        }
        out[rel] =
            Befund{std::string(HeuristikDetail::kName), HeuristikDetail::version_of(bytes, rel), digest_hex_of(bytes)};
    }
    heuristik_n = static_cast<int>(h_rels.size());

    std::vector<std::string> const o_rels = OrganDetail::discover(root, home_ok);
    if (!home_ok) ok = false;
    organ_traeger_n = 0;
    for (std::string const& rel : o_rels) {
        std::vector<std::uint8_t> bytes;
        if (!read_file_bytes(root / rel, bytes)) {
            std::fprintf(stderr, "axis_version_lock: FEHLER Datei nicht lesbar: %s\n", rel.c_str());
            ok = false;
            continue;
        }
        std::string const version = OrganDetail::version_of(bytes, rel);
        if (version != "-") ++organ_traeger_n;
        out[rel] = Befund{std::string(OrganDetail::kName), version, digest_hex_of(bytes)};
    }
    organ_n = static_cast<int>(o_rels.size());
    return ok;
}

/// Die Riegel-Schaerfung (ii): discovered-Zaehler MIT NENNER, je Kategorie eine BESTAND-Zeile.
void drucke_bestand(int heuristik_n, int organ_n, int organ_traeger_n) {
    std::fprintf(stdout,
                 "axis_version_lock: BESTAND heuristik discovered=%d traeger=%d "
                 "(nenner: alle *.hpp unter libs/cache_engine/heuristik)\n",
                 heuristik_n, heuristik_n);
    std::fprintf(stdout,
                 "axis_version_lock: BESTAND organ discovered=%d traeger=%d forwarder_prosa=%d "
                 "(nenner: *.hpp mit 'algo_version' unter libs/cache_engine/axes + "
                 "libs/cache_engine/topics/queuing)\n",
                 organ_n, organ_traeger_n, organ_n - organ_traeger_n);
}

int do_write(fs::path const& root, std::string const& lockfile) {
    BefundMap bestand;
    int       h_n = 0, o_n = 0, o_t = 0;
    if (!erhebe_bestand(root, bestand, h_n, o_n, o_t)) return 2;

    int           rc = 0;
    std::ofstream out(lockfile, std::ios::trunc);
    if (!out) {
        std::fprintf(stderr, "axis_version_lock: FEHLER kann Lock-Datei nicht schreiben: %s\n", lockfile.c_str());
        return 2;
    }
    out << "# axis_version.lock -- content-digest tripwire ueber die algo_version-Traeger\n";
    out << "# (S-14a Riegel, Task #33/KON17-03; vormals PAKET W3-C GN-8/O-4)\n";
    out << "# format: v2\n";
    out << "# record (ZWEI Zeilen je Datei): '<category> <version> <relative-path>' + Folgezeile\n";
    out << "#   '    <sha256-hex>' (vier Leerzeichen Einrueckung).\n";
    out << "#   heuristik: version = Integer aus dem '// AXIS_ALGO_VERSION: <N>'-Marker (0 = kein Marker)\n";
    out << "#   organ:     version = erstes algo_version-String-Literal 'X.Y.Z[.flags]' im Code;\n";
    out << "#              '-' = kein Literal (Forwarder/Prosa; digest-only, Aenderung verlangt\n";
    out << "#              bewussten Lock-Regen-Commit via --write)\n";
    out << "# Formatwahl v2: Kategorie-Spalte + Versions-STRING statt v1-Integer-Spalte; v1-Locks\n";
    out << "# werden beim --check mit klarer Meldung abgewiesen (kein stilles Weiterlesen).\n";
    out << "# Zweizeilen-Record, damit jede Zeile <=120 Spalten bleibt (Diff-Hygiene-Wache voll\n";
    out << "# gedeckt, keine Ausnahme-Klasse); Kopfzeile ohne Digest-Folgezeile = lauter Formatfehler.\n";
    for (auto const& [rel, b] : bestand) {
        out << b.category << ' ' << b.version << ' ' << rel << '\n';
        out << "    " << b.digest_hex << '\n';
        std::fprintf(stdout, "axis_version_lock: LOCK %s %s %s %s\n", b.category.c_str(), b.version.c_str(),
                     b.digest_hex.c_str(), rel.c_str());
        if (b.category == OrganDetail::kName && OrganDetail::literal_unparsbar(b.version)) {
            std::fprintf(stderr, "axis_version_lock: ROT algo_version-Literal unparsbar (Sentinel): '%s' %s\n",
                         b.version.c_str(), rel.c_str());
            rc = 1;
        }
    }
    drucke_bestand(h_n, o_n, o_t);
    if (rc != 0)
        std::fprintf(stderr, "axis_version_lock: ROT --write hat unparsbare Literale vermerkt -- "
                             "Lock NICHT committen, Literale reparieren\n");
    return rc;
}

/// Parst die Lock-Datei v2. Verlangt die Formatzeile '# format: v2' VOR dem ersten Eintrag.
[[nodiscard]] bool parse_lock_v2(std::string const& path, BefundMap& out) {
    std::ifstream f(path);
    if (!f) {
        std::fprintf(stderr, "axis_version_lock: ROT Lock-Datei nicht lesbar: %s\n", path.c_str());
        return false;
    }
    bool        format_ok = false;
    bool        have_kopf = false;
    Befund      kopf;
    std::string kopf_rel;
    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("# format:", 0) == 0) {
            if (line == "# format: v2") {
                format_ok = true;
                continue;
            }
            std::fprintf(stderr,
                         "axis_version_lock: ROT Lock-Format unbekannt/veraltet ('%s', erwartet '# format: v2') "
                         "-- mit --write regenerieren: %s\n",
                         line.c_str(), path.c_str());
            return false;
        }
        if (line.empty() || line[0] == '#') continue;
        if (!format_ok) {
            std::fprintf(stderr, "axis_version_lock: ROT Lock ohne '# format: v2'-Kopf vor dem ersten Eintrag: %s\n",
                         path.c_str());
            return false;
        }
        if (line[0] == ' ' || line[0] == '\t') {
            // Digest-Folgezeile des Zweizeilen-Records (vier Leerzeichen + 64 Hex).
            std::istringstream ds(line);
            std::string        digest;
            ds >> digest;
            if (!have_kopf || digest.size() != 64) {
                std::fprintf(stderr, "axis_version_lock: ROT unparsbare Digest-Zeile im Lock (Record-Kopf %s): '%s'\n",
                             have_kopf ? kopf_rel.c_str() : "FEHLT", line.c_str());
                return false;
            }
            kopf.digest_hex = digest;
            out[kopf_rel]   = kopf;
            have_kopf       = false;
            continue;
        }
        if (have_kopf) {
            std::fprintf(stderr, "axis_version_lock: ROT Lock-Record ohne Digest-Folgezeile: %s\n", kopf_rel.c_str());
            return false;
        }
        std::istringstream ls(line);
        if (!(ls >> kopf.category >> kopf.version >> kopf_rel)) {
            std::fprintf(stderr, "axis_version_lock: ROT unparsbare Lock-Zeile: '%s'\n", line.c_str());
            return false;
        }
        have_kopf = true;
    }
    if (have_kopf) {
        std::fprintf(stderr, "axis_version_lock: ROT Lock-Record ohne Digest-Folgezeile am Dateiende: %s\n",
                     kopf_rel.c_str());
        return false;
    }
    if (!format_ok) std::fprintf(stderr, "axis_version_lock: ROT Lock ohne '# format: v2'-Kopf: %s\n", path.c_str());
    return format_ok;
}

int do_check(fs::path const& root, std::string const& lockfile) {
    BefundMap lock;
    if (!parse_lock_v2(lockfile, lock)) return 1;

    BefundMap bestand;
    int       h_n = 0, o_n = 0, o_t = 0;
    if (!erhebe_bestand(root, bestand, h_n, o_n, o_t)) return 2;

    int red = 0;
    for (auto const& [rel, ist] : bestand) {
        auto const it = lock.find(rel);
        if (it == lock.end()) {
            std::fprintf(stderr, "axis_version_lock: ROT Datei nicht im Lock verzeichnet (unlocked): %s\n",
                         rel.c_str());
            red = 1;
            continue;
        }
        Befund const& soll = it->second;
        if (ist.digest_hex == soll.digest_hex) continue; // unveraendert -- still gruen, Zaehler unten
        bool bump = false;
        if (ist.category == HeuristikDetail::kName)
            bump = HeuristikDetail::bump_ok(soll.version, ist.version);
        else
            bump = OrganDetail::bump_ok(soll.version, ist.version);
        if (bump) {
            std::fprintf(stdout, "axis_version_lock: OK Digest geaendert MIT Version-Bump %s -> %s %s\n",
                         soll.version.c_str(), ist.version.c_str(), rel.c_str());
            std::fprintf(stdout, "axis_version_lock: HINWEIS Lock erneuern (--write) fuer %s\n", rel.c_str());
            continue;
        }
        if (ist.category == OrganDetail::kName &&
            (OrganDetail::literal_unparsbar(ist.version) || OrganDetail::literal_unparsbar(soll.version))) {
            std::fprintf(stderr,
                         "axis_version_lock: ROT algo_version-Literal unparsbar (Sentinel): "
                         "'%s' -> '%s' %s\n",
                         soll.version.c_str(), ist.version.c_str(), rel.c_str());
        } else {
            std::fprintf(stderr, "axis_version_lock: ROT Digest geaendert OHNE gueltigen Version-Bump (%s -> %s) %s\n",
                         soll.version.c_str(), ist.version.c_str(), rel.c_str());
        }
        std::fprintf(stderr, "axis_version_lock:     erwartet %s\n", soll.digest_hex.c_str());
        std::fprintf(stderr, "axis_version_lock:     ist      %s\n", ist.digest_hex.c_str());
        std::fprintf(stderr, "axis_version_lock:     HINWEIS Version bumpen ODER bewussten Lock-Regen-Commit "
                             "(--write) fahren\n");
        red = 1;
    }
    // Riegel-Schaerfung (i), schliesst Koeder D: jeder Lock-Eintrag braucht seine Datei im Bestand.
    for (auto const& [rel, soll] : lock) {
        if (bestand.find(rel) != bestand.end()) continue;
        std::fprintf(stderr, "axis_version_lock: ROT Lock-Eintrag ohne Datei (verwaist): %s\n", rel.c_str());
        std::fprintf(stderr, "axis_version_lock:     HINWEIS Datei geloescht/umgezogen/ohne 'algo_version'? "
                             "Bewussten Lock-Regen-Commit (--write) fahren\n");
        red = 1;
    }
    drucke_bestand(h_n, o_n, o_t);
    if (red == 0)
        std::fprintf(stdout, "axis_version_lock: GRUEN bestand konsistent -- %zu Dateien (heuristik=%d, organ=%d)\n",
                     bestand.size(), h_n, o_n);
    return red;
}

void usage() {
    std::fprintf(stderr, "usage: axis_version_lock (--write|--check) <lockfile> [--root <repo-root>]\n");
    std::fprintf(stderr, "       (v2: die Grundgesamtheit erhebt das Tool selbst unter --root; die\n");
    std::fprintf(stderr, "        v1-Form mit expliziter Header-Liste ist entfallen)\n");
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 3 && argc != 5) {
        usage();
        return 2;
    }
    std::string const mode     = argv[1];
    std::string const lockfile = argv[2];
    fs::path          root     = ".";
    if (argc == 5) {
        if (std::string_view(argv[3]) != "--root") {
            usage();
            return 2;
        }
        root = argv[4];
    }
    if (mode == "--write") return do_write(root, lockfile);
    if (mode == "--check") return do_check(root, lockfile);
    usage();
    return 2;
}
