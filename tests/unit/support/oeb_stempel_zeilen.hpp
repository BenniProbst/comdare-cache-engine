#pragma once
// support/oeb_stempel_zeilen.hpp -- Test-Support fuer das OE-B-DUMMY-LAGER (LB-6).
//
// OWNER-ENTSCHEID OE-B (01.08.2026): "Dummy-Lager im temp-Verzeichnis, Binaries als TEXTDATEIEN mit
// Stempel-String, Test als GOOGLE-TEST." VERSCHAERFUNG (06.08.2026): "aus simulierten Textdokumenten
// die Stempel auszulesen und JEDE ZEILE VERBATIM auszuwerten."
//
// DREI Bausteine, jeder genau EINMAL im Haus:
//
//   (a) split_lines(block) -- der Zeilen-Zerleger. Er stand bis 2026-08-06 als TU-lokale Funktion im
//       anonymen Namensraum von test_g1_binary_version_stamp.cpp (dort Zeilen 28-40) und ist von da
//       VERBATIM hierher gewandert: gleiche Signatur, gleiche Schleife, gleiche defensive
//       Rest-Behandlung. AUSGELAGERT statt kopiert -- eine zweite Kopie waere "zwei Quellen fuer
//       dieselbe Wahrheit", die Fehlerklasse, an der dieses Haus schon mehrfach haengengeblieben ist.
//
//   (b) der MEHRZEILIGE OE-B-Stempel-Fixture. Der bis 2026-08-06 in beiden Lager-TUs benutzte
//       Blattinhalt "[vereint,O2,avx2][a,b,c]+bt=Release" war EINZEILIG; an einer einzigen Zeile ist
//       "jede Zeile verbatim" gar nicht belegbar -- eine verlorene Zeile hat dort keinen Ort, an dem
//       sie auffallen koennte. Der Fixture hier hat dieselbe FORM wie der echte Binary-Stempel
//       profile_facade::g1_binary_version_block(): vier gelabelte, '\n'-terminierte Zeilen in der
//       Reihenfolge planner@ / ceb-contract= / build-type= / build-version=.
//
//   (c) lies_blatt_datei(pfad) -- der RUECKLESE-Weg ueber std::ifstream.
//
// WARUM LITERALE UND NICHT g1_binary_version_block() SELBST: dessen erste Zeile ist der
// PLANER-Selbst-Stempel der uebersetzenden Maschine (planner@... isa=... os=...), also ein
// maschinenabhaengiger Wert. Ein Lager-Blatt, das byte-genau zurueckgelesen und ZEILENWEISE
// verglichen wird, braucht einen maschinen-UNABHAENGIGEN Inhalt -- sonst waere der Vergleich in der
// 8er-Docker-Matrix je Distro ein anderer und ein eingefrorener Erwartungswert unmoeglich (dieselbe
// Begruendung, die die Frozen-Vektoren in test_m_w12_stamp_bausteine.cpp tragen). Damit der Fixture
// trotzdem nicht von der echten Stempel-FORM wegdriftet, prueft test_g1_binary_version_stamp die
// Form-Gleichheit (Zeilenzahl + Label je Position) gegen den echten Block -- in genau der TU, die den
// echten Erzeuger ueberhaupt sieht. Die Lager-TUs (bestandslog-Schicht) muessen dafuer keine
// profile_facade-Kante ziehen.
//
// WARUM DIE RUECKLESE NICHT UEBER BaumAblage::datei_lesen LAEUFT: Schreiben und Lesen kaemen dann aus
// demselben Baustein (make_filesystem_ablage); ein symmetrischer Fehler darin hoebe sich im Vergleich
// selbst auf. Der zweite, unabhaengige Weg (std::ifstream) ist der Sinn der Sache, keine Verdopplung.
//
// DOKTRIN: header-only C++23, ASCII-Kommentare, nur stdlib.

#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::test {

/// Zerlegt den "\n"-terminierten Block in seine Zeilen (ohne die abschliessende Leer-Zeile nach dem
/// letzten "\n"). VERBATIM aus test_g1_binary_version_stamp.cpp:28-40 ausgelagert.
[[nodiscard]] inline std::vector<std::string> split_lines(std::string const& block) {
    std::vector<std::string> lines;
    std::string              cur;
    for (char const c : block) {
        if (c == '\n') {
            lines.push_back(cur);
            cur.clear();
        } else {
            cur += c;
        }
    }
    if (!cur.empty()) lines.push_back(cur); // (defensiv: nicht-terminierter Rest)
    return lines;
}

// ---------------------------------------------------------------------------
// Der mehrzeilige OE-B-Stempel-Fixture. Jede Zeile EINZELN benannt, damit ein Test sie EINZELN
// nennen kann statt nur "den Stempel" -- genau das verlangt die Owner-Verschaerfung.
// ---------------------------------------------------------------------------
inline constexpr std::string_view kOeBStempelZeile0 = "planner@oeb-dummy isa=amd64_v3 os=linux";
inline constexpr std::string_view kOeBStempelZeile1 = "ceb-contract=8.0";
inline constexpr std::string_view kOeBStempelZeile2 = "build-type=Release";
inline constexpr std::string_view kOeBStempelZeile3 = "build-version=+cxx=gcc+opt=O2+ext=avx2+ceb=8.0";

/// Die vier Zeilen in der bindenden Reihenfolge -- die EINE Liste, gegen die zurueckgelesen wird.
inline constexpr std::array<std::string_view, 4> kOeBStempelZeilen{kOeBStempelZeile0, kOeBStempelZeile1,
                                                                   kOeBStempelZeile2, kOeBStempelZeile3};

/// Das Label je Position. Es ist der Anker der Form-Gleichheit mit g1_binary_version_block(): ein
/// umbenanntes oder umgestelltes Label des echten Blocks macht den Fixture rot, nicht still falsch.
inline constexpr std::array<std::string_view, 4> kOeBStempelLabels{"planner@",
                                                                   "ceb-contract=", "build-type=", "build-version="};

inline constexpr std::size_t kOeBStempelZeilenZahl = kOeBStempelZeilen.size();

/// Der Blattinhalt eines OE-B-Dummy-"Binary": vier Zeilen, jede '\n'-terminiert (wie der echte Block).
[[nodiscard]] inline std::string oeb_stempel_block() {
    std::string out;
    for (auto const z : kOeBStempelZeilen) {
        out += z;
        out += '\n';
    }
    return out;
}

/// Ein Blatt vom DATEISYSTEM zurueckholen -- der zweite, vom Schreib-Weg unabhaengige Pfad (s. Kopf).
/// nullopt == nicht oeffenbar; NIE ein stiller leerer String (der waere von "leere Datei" nicht zu
/// unterscheiden und wuerde eine fehlende Datei als bestandene Ruecklese durchgehen lassen).
[[nodiscard]] inline std::optional<std::string> lies_blatt_datei(std::filesystem::path const& pfad) {
    std::ifstream is(pfad, std::ios::binary);
    if (!is) return std::nullopt;
    return std::string{std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>()};
}

} // namespace comdare::test
