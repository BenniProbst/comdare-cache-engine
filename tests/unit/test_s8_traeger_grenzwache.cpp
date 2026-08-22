// test_s8_traeger_grenzwache -- DIE GRENZ-WACHE der Traeger-Stufenkette (DESIGN #29 Par. 4.4).
// =============================================================================================
// ZIEHT MIT DEM ERSTEN S-8-COMMIT EIN (gleiches Paket -- Design-Auflage). Bau-Muster:
// test_vs_taxonomie_klassen_grep (Quelltext-Scanner als Google-Test, kein Shell; KON6-05).
//
// DIE ZWEI ZUSICHERUNGEN (Par. 4.4, woertlich):
//   (a) KEINE Include-Kante Stufe N -> N+1 innerhalb libs/traeger/ (KON43-01: Stufe N+1 haengt
//       NUR an Stufe N, nie umgekehrt -- eine niedrige Stufe, die eine hoehere zieht, ist die
//       Richtungs-Inversion, gegen die die ganze Zielstruktur gebaut ist).
//   (b) JEDE traeger -> builder/-Kante traegt den Bruecken-Marker "TRAEGER-BRUECKE(#88): <Zielort>"
//       (Par. 4.3: Wiederverwendung des Monolith-Bestands ist ERLAUBT, aber nur als gezaehlter
//       #88-Wanderposten, nie als stille Neuverschmutzung -- Risiko R1).
//
// WARUM ROT HIER BILLIG IST: heute (S-8-Kopf) besteht libs/traeger/ aus drei std-only-Headern --
// jede kuenftige Kante faellt als EINZELNE Datei+Zeile auf, solange die Menge klein ist. Die
// Wache meldet IDENTITAET (Datei + Zeile + Grund), nicht nur "irgendwo rot".
//
// K13 -- KOEDER BEIDSEITIG (der Biss wird IM Test vorgefuehrt, nicht behauptet):
//   KOEDER A (Stufen-Inversion): Wegwerf-Baum mit planner-Datei, die <traeger/ceb/...> zieht --
//     MUSS als genau DIESE Datei auffallen; die bereinigte Kopie MUSS gruen sein.
//   KOEDER B (Bruecke ohne Marker): Wegwerf-Baum mit traeger-Datei, die builder/ zieht --
//     ohne Marker MUSS sie auffallen, MIT Marker (Zeile davor) MUSS sie durchgehen.
//
// TESTKRITIK (T-9), benannt: der Scanner ist zeilenbasiert und parst kein C++. Ein #include in
// einem Blockkommentar wird MITGEZAEHLT (billige Richtung: kostet einen Marker/eine Bereinigung,
// uebersieht nie eine echte Kante); bedingte Includes (#ifdef) zaehlen als Kante (streng richtig:
// auch eine bedingte Kante ist eine Kante). Der Marker darf auf der Include-Zeile ODER der
// Zeile unmittelbar davor stehen. Nicht gedeckt: Kanten via Makro-Expansion des Include-Pfads
// (im Haus nicht in Gebrauch; wuerde am Review auffallen).
//
// STILLE-NULL-GEGENPROBE (Fallen-Register ugrep): der Lauf gegen den ECHTEN Baum muss
// mindestens die drei S-8-Kopf-Header GESEHEN haben -- ein leerer Scan ist ROT, nie gruen.
//
// ASCII-only, Zeilen <= 120.

#include <gtest/gtest.h>

#include <cctype>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#ifndef COMDARE_TRAEGER_WURZEL
#error "COMDARE_TRAEGER_WURZEL must point to libs/traeger of the source tree"
#endif

namespace {

namespace fs = std::filesystem;

struct Verstoss {
    std::string datei;
    std::size_t zeile = 0;
    std::string grund; // "stufen_inversion" | "bruecke_ohne_marker"
    std::string include_ziel;
};

struct ScanErgebnis {
    std::vector<Verstoss> verstoesse;
    std::size_t           quell_dateien  = 0; // gesehene Quelldateien (Stille-Null-Gegenprobe)
    std::size_t           include_zeilen = 0;
};

constexpr std::string_view kMarker = "TRAEGER-BRUECKE(#88):";

// Stufen-Karte der vier Unterprojekte (DESIGN #29 Par. 4.2 / KON43-01).
int stufe_von_verzeichnis(std::string_view name) {
    if (name == "planner") { return 1; }
    if (name == "ceb") { return 2; }
    if (name == "tier") { return 3; }
    if (name == "hybrid") { return 4; }
    return 0; // keine Stufe (z. B. Wurzel-CMakeLists)
}

bool ist_quelldatei(fs::path const& p) {
    auto const e = p.extension().string();
    return e == ".hpp" || e == ".cpp" || e == ".h" || e == ".cc" || e == ".hh" || e == ".cxx" || e == ".ipp";
}

// Extrahiert das Include-Ziel einer Zeile oder leer, wenn die Zeile kein #include ist.
std::string include_ziel_von_zeile(std::string const& zeile) {
    std::size_t i = 0;
    while (i < zeile.size() && (std::isspace(static_cast<unsigned char>(zeile[i])) != 0)) { ++i; }
    if (i >= zeile.size() || zeile[i] != '#') { return {}; }
    ++i;
    while (i < zeile.size() && (std::isspace(static_cast<unsigned char>(zeile[i])) != 0)) { ++i; }
    if (zeile.compare(i, 7, "include") != 0) { return {}; }
    i += 7;
    while (i < zeile.size() && zeile[i] != '<' && zeile[i] != '"') { ++i; }
    if (i >= zeile.size()) { return {}; }
    char const        schluss = (zeile[i] == '<') ? '>' : '"';
    std::size_t const beginn  = i + 1;
    std::size_t const ende    = zeile.find(schluss, beginn);
    if (ende == std::string::npos) { return {}; }
    return zeile.substr(beginn, ende - beginn);
}

// Ziel-Stufe eines Include-Pfads INNERHALB der Traeger-Welt; 0 = kein Traeger-Include.
// Deckt <traeger/NAME/...> ebenso wie relative Formen "../NAME/...".
int traeger_ziel_stufe(std::string const& ziel) {
    for (std::string_view const name : {"planner", "ceb", "tier", "hybrid"}) {
        std::string const absolut = "traeger/" + std::string(name) + "/";
        std::string const relativ = "../" + std::string(name) + "/";
        if (ziel.find(absolut) != std::string::npos || ziel.rfind(relativ, 0) == 0) {
            return stufe_von_verzeichnis(name);
        }
    }
    return 0;
}

bool ist_builder_include(std::string const& ziel) {
    return ziel.rfind("builder/", 0) == 0 || ziel.find("/builder/") != std::string::npos ||
           ziel.rfind("../builder/", 0) == 0;
}

ScanErgebnis scanne_traeger_baum(fs::path const& wurzel) {
    ScanErgebnis erg;
    if (!fs::exists(wurzel)) { return erg; }
    for (auto const& eintrag : fs::recursive_directory_iterator(wurzel)) {
        if (!eintrag.is_regular_file() || !ist_quelldatei(eintrag.path())) { continue; }
        ++erg.quell_dateien;

        // Datei-Stufe = erste Pfad-Komponente unter der Wurzel.
        auto const rel   = fs::relative(eintrag.path(), wurzel);
        int const  stufe = rel.begin() != rel.end() ? stufe_von_verzeichnis(rel.begin()->string()) : 0;

        std::ifstream in(eintrag.path());
        std::string   zeile;
        std::string   vorzeile;
        std::size_t   nr = 0;
        while (std::getline(in, zeile)) {
            ++nr;
            std::string const ziel = include_ziel_von_zeile(zeile);
            if (!ziel.empty()) {
                ++erg.include_zeilen;
                // Regel (a): Stufe N zieht Stufe M > N -- die Richtungs-Inversion.
                int const ziel_stufe = traeger_ziel_stufe(ziel);
                if (stufe != 0 && ziel_stufe != 0 && ziel_stufe > stufe) {
                    erg.verstoesse.push_back({eintrag.path().string(), nr, "stufen_inversion", ziel});
                }
                // Regel (b): traeger -> builder nur mit Bruecken-Marker (Zeile selbst oder davor).
                if (ist_builder_include(ziel) && zeile.find(kMarker) == std::string::npos &&
                    vorzeile.find(kMarker) == std::string::npos) {
                    erg.verstoesse.push_back({eintrag.path().string(), nr, "bruecke_ohne_marker", ziel});
                }
            }
            vorzeile = zeile;
        }
    }
    return erg;
}

// Wegwerf-Baum fuer die Koeder (Posten-69-Konvention: getaggter Name unter dem System-Temp).
class WegwerfBaum {
public:
    WegwerfBaum() {
#ifdef COMDARE_TEST_TMP_BUILD_TAG
        std::string const tag = COMDARE_TEST_TMP_BUILD_TAG;
#else
        std::string const tag = "dev";
#endif
        wurzel_ = fs::temp_directory_path() / ("comdare_test_s8grenz_" + tag);
        fs::remove_all(wurzel_);
        fs::create_directories(wurzel_ / "planner");
    }
    ~WegwerfBaum() {
        std::error_code ec;
        fs::remove_all(wurzel_, ec); // Aufraeumen best effort; Name ist build-eindeutig
    }
    [[nodiscard]] fs::path const& wurzel() const { return wurzel_; }

    void schreibe(fs::path const& rel, std::string const& inhalt) const {
        fs::create_directories((wurzel_ / rel).parent_path());
        std::ofstream out(wurzel_ / rel);
        out << inhalt;
    }

private:
    fs::path wurzel_;
};

// -- DER ECHTE BAUM: 0 Verstoesse, und der Scan hat NACHWEISLICH etwas gesehen. --
TEST(S8TraegerGrenzwache, EchterBaumIstStufenreinUndMarkiert) {
    auto const erg = scanne_traeger_baum(fs::path{COMDARE_TRAEGER_WURZEL});

    // Stille-Null-Gegenprobe: die drei S-8-Kopf-Header MUESSEN im Nenner sein.
    EXPECT_GE(erg.quell_dateien, 3u) << "Scan sah <3 Quelldateien -- falsche Wurzel? " << COMDARE_TRAEGER_WURZEL;
    EXPECT_GE(erg.include_zeilen, 3u) << "Scan sah keine Include-Zeilen -- Scanner defekt?";

    for (auto const& v : erg.verstoesse) {
        ADD_FAILURE() << v.grund << ": " << v.datei << ":" << v.zeile << " -> " << v.include_ziel;
    }
    EXPECT_TRUE(erg.verstoesse.empty()) << erg.verstoesse.size() << " Grenz-Verstoesse in libs/traeger/ (Details oben)";
    std::cout << "[grenzwache] gescannt: " << erg.quell_dateien << " Quelldateien, " << erg.include_zeilen
              << " Include-Zeilen, " << erg.verstoesse.size() << " Verstoesse\n";
}

// -- KOEDER A: die Stufen-Inversion beisst -- mit IDENTITAET der gemeldeten Datei. --
TEST(S8TraegerGrenzwache, KoederStufenInversionBeisst) {
    WegwerfBaum const baum;
    baum.schreibe("planner/probe_inversion.hpp",
                  "#pragma once\n"
                  "#include <string>\n"
                  "#include <traeger/ceb/irgendwas.hpp>\n"); // Stufe 1 zieht Stufe 2 = Inversion

    auto const rot = scanne_traeger_baum(baum.wurzel());
    ASSERT_EQ(rot.verstoesse.size(), 1u) << "genau EIN Verstoss (die Inversions-Zeile)";
    EXPECT_EQ(rot.verstoesse[0].grund, "stufen_inversion");
    EXPECT_EQ(rot.verstoesse[0].zeile, 3u);
    EXPECT_NE(rot.verstoesse[0].datei.find("probe_inversion.hpp"), std::string::npos)
        << "IDENTITAET: gemeldet wird die Koeder-Datei, nicht irgendwas";

    // GEGENPROBE: dieselbe Datei ohne die Inversions-Zeile ist gruen; die GEGENRICHTUNG
    // (hoehere Stufe zieht niedrigere, ceb -> planner) ist ERLAUBT und bleibt gruen.
    baum.schreibe("planner/probe_inversion.hpp", "#pragma once\n#include <string>\n");
    baum.schreibe("ceb/probe_richtung.hpp", "#pragma once\n#include <traeger/planner/naht_nachrichten.hpp>\n");
    auto const gruen = scanne_traeger_baum(baum.wurzel());
    EXPECT_TRUE(gruen.verstoesse.empty()) << "bereinigt + erlaubte N+1->N-Kante muss gruen sein";
    EXPECT_GE(gruen.quell_dateien, 2u);
}

// -- KOEDER B: die Bruecke ohne Marker beisst; MIT Marker (Zeile davor) geht sie durch. --
TEST(S8TraegerGrenzwache, KoederBrueckeOhneMarkerBeisst) {
    WegwerfBaum const baum;
    baum.schreibe("planner/probe_bruecke.hpp",
                  "#pragma once\n"
                  "#include <builder/experiment_tree/progress_delta.hpp>\n"); // ohne Marker

    auto const rot = scanne_traeger_baum(baum.wurzel());
    ASSERT_EQ(rot.verstoesse.size(), 1u);
    EXPECT_EQ(rot.verstoesse[0].grund, "bruecke_ohne_marker");
    EXPECT_EQ(rot.verstoesse[0].zeile, 2u);
    EXPECT_NE(rot.verstoesse[0].datei.find("probe_bruecke.hpp"), std::string::npos);

    // GEGENPROBE: derselbe Include MIT Marker auf der Vorzeile ist eine GEZAEHLTE Bruecke -> gruen.
    baum.schreibe("planner/probe_bruecke.hpp",
                  "#pragma once\n"
                  "// TRAEGER-BRUECKE(#88): progress_delta wandert nach traeger/planner (Par. 3c K1)\n"
                  "#include <builder/experiment_tree/progress_delta.hpp>\n");
    auto const gruen = scanne_traeger_baum(baum.wurzel());
    EXPECT_TRUE(gruen.verstoesse.empty()) << "markierte Bruecke ist erlaubt (gezaehlter #88-Posten)";
}

} // namespace
