// test_vs_taxonomie_klassen_grep -- DER RIEGEL UNTER DER SELBSTWARTENDEN VERSIONS-TAXONOMIE.
//
// GEGENSTAND: die Klassen-Liste (a)..(j) im Kopf von
//   libs/cache_engine/include/cache_engine/measurement/algo_semver.hpp
// Sie fuehrt ALLE ce-eigenen Versions-Quellen und traegt dort ihre eigene Doktrin (algo_semver.hpp:119-120):
//   "diese Liste ist AUS DEM DIFF/GREP ABZULEITEN, NIE HANDGEPFLEGT"
// und ihre eigene Warnung (:117-118):
//   "Jede KUENFTIGE ce-Versions-Quelle gehoert hier hinein ... sonst entsteht wieder eine Luecke wie bei
//    (e)/(f)."
//
// WARUM ES DIESEN TEST GIBT -- die Luecke ist EINGETRETEN, zweimal, und niemand hat es gemerkt:
//   Gemessen am 10.08.2026 auf origin/development (95cb3039) trugen DREI produktive Dateien beide
//   B12-Politik-Wachen, ohne in der Liste zu stehen:
//     builder/pruef_dock/pruef_dock_version.hpp   10 Aufrufe, FUENF eigene Literale  -> jetzt Klasse (i)
//     abi/toolchain_stamp_glied.hpp                6 Aufrufe, DREI eigene Literale   -> jetzt Klasse (j)
//     measurement/hardware_meta_meta_axis.hpp      2 Aufrufe, Heimat der (d)-Wachen  -> jetzt in (d) benannt
//   Genau das ist die Klasse "(e)/(f)", vor der die Liste warnt: eine Selbstwartung, die BEHAUPTET wird,
//   statt erzwungen zu sein. Eine Doktrin ohne Riegel ist eine Absichtserklaerung.
//
// DIE ZUSICHERUNG, die dieser Test macht (und nur diese):
//   JEDE produktive Datei unter libs/, die eine der beiden ce-Politik-Wachen AUFRUFT, ist in der
//   Klassen-Liste NAMENTLICH genannt. Faellt eine Quelle aus der Liste -- oder kommt eine neue hinzu,
//   ohne eingetragen zu werden -- wird dieser Test ROT und nennt die Datei beim Namen.
//
// WARUM DER NENNER AN DER AUFRUF-FORM HAENGT UND NICHT AM BLOSSEN NAMEN:
//   Das bindende Kommando des Headers (Absatz "DAS ERSTE, BINDENDE KOMMANDO", heute algo_semver.hpp:145 --
//   die Zeilennummer wandert, der Absatz-Titel nicht) sucht den Bezeichner OHNE Klammer und faengt deshalb
//   auch reine Prosa mit. Am Objekt gemessen: libs/cache_engine/axes/alloc/axis_06_allocator_strategy_base.hpp
//   nennt "ce_owned_version_satisfies_cpu_enforce (ENFORCE ist scharf)" zweimal in KOMMENTAREN -- mit
//   Leerzeichen vor der Klammer. Das ist keine Versions-Quelle, sondern ein Verweis. Der Riegel verlangt
//   deshalb die AUFRUF-Form "<name>(" und zaehlt genau die Stellen, an denen die Politik wirklich
//   angewandt wird. Die Erhebung ist bewusst SYNTAKTISCH und parst kein C++: wer die Aufruf-Form in einen
//   Kommentar schreibt, wird mitgezaehlt und kostet eine Zeile in der Liste. Das ist die billige Richtung
//   des Irrtums -- die teure waere, eine echte Quelle zu uebersehen.
//
// WAS DIESER TEST AUSDRUECKLICH NICHT DECKT (ein gruenes Gate deckt nur seinen Gegenstand):
//   * Die absolute Trefferzahl des generischen Wachen-Greps (heute 130) wird BERICHTET, aber NICHT
//     gepinnt. Der Header sagt selbst (:122): "die genannten Zahlen sind Momentaufnahmen und altern, die
//     Kommandos nicht." Ein Pin waere eine zweite Wahrheit, die bei jedem Kommentar-Zuwachs falsch-rot
//     wird.
//   * Aufruf-Stellen unter tests/, tools/, apps/ (heute 8 Dateien) sind KEIN Gegenstand. Ein Test, der
//     eine Wache PRUEFT, ist ihr Verbraucher, nicht ihre Quelle. Die Zahl steht im Protokoll, damit die
//     nicht gepruefte Menge sichtbar bleibt statt unsichtbar.
//   * Ob eine gelistete Klasse INHALTLICH richtig beschrieben ist, kann kein Grep sehen. Der Riegel
//     haelt die VOLLSTAENDIGKEIT, nicht die Wahrheit der Prosa.
//   * Klasse (b) (builder/ceb_version_stamp.hpp) und die (d)-Instanz external_utils_family_axis.hpp
//     tragen KEINE eigene Aufruf-Stelle und stehen deshalb nicht im Nenner. Sie sind trotzdem gelistet --
//     die Richtung der Zusicherung ist "jede Aufruf-Stelle ist genannt", nicht "jede Nennung ruft auf".
//
// K13 -- DIE KOEDER, BEIDSEITIG, AUS /dev/urandom:
//   KOEDER A (neue Quelle faellt durch): ein erfundener Header mit gewuerfeltem Namen, der die Aufruf-Form
//     traegt, wird in einen Wegwerf-Baum gelegt. Gegen die ECHTE Liste MUSS er als unbenannt auffallen --
//     und gegen eine Liste, die genau diesen gewuerfelten Namen enthaelt, MUSS er es NICHT.
//   KOEDER B (bestehende Quelle wird gestrichen): eine der real gelisteten Quellen wird gewuerfelt und
//     aus einer Kopie der Liste entfernt. Der Riegel MUSS genau diese eine Datei melden -- und gegen die
//     unveraenderte Kopie derselben Datei MUSS er gruen sein.
//   Beide Koeder pruefen nicht "irgendwie rot", sondern die IDENTITAET der gemeldeten Datei. Ein Mutant
//   "melde immer alles" faellt am Gegeneingang, ein Mutant "melde nie etwas" am Koeder selbst.
//
// UMLEITUNG FUER DIE KOEDER: COMDARE_TAX_QUELLE_PFAD (die Liste) und COMDARE_TAX_LIBS_PFAD (der Baum).
// Ohne sie stehen die eincompilierten Pfade des echten Quellbaums. Welcher Prueflig gefahren wurde, steht
// in der Ausgabe jedes Falls.
//
// ASCII-only (Leitplanke). Zeilen <= 120 Byte (Diff-Hygiene-Wache).

// Der Header ist zugleich PRUEFLING dieser TU: er wird hier uebersetzt, damit ein Kommentar-Defekt in der
// Klassen-Liste (z.B. ein '\' am Zeilenende -> -Wcomment, algo_semver.hpp:128-132) hier bricht und nicht
// erst irgendwo in der Flotte.
#include <cache_engine/measurement/algo_semver.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

namespace fs = std::filesystem;

// Der Bezeichner-Stamm des bindenden Kommandos (algo_semver.hpp, Absatz "DAS ERSTE, BINDENDE KOMMANDO").
constexpr std::string_view kWacheWohlgeformt = "ce_owned_version_is_wellformed";
constexpr std::string_view kWacheCpuEnforce  = "ce_owned_version_satisfies_cpu_enforce";

// Die zwei EINDEUTIGEN Anker des Listen-Blocks. Am Objekt gemessen: je genau 1 Vorkommen in der Datei.
constexpr std::string_view kBlockAnfang = "MIGRATIONS-NAHT-LISTE";
constexpr std::string_view kBlockEnde   = "#ifndef COMDARE_VERSION_HW_FLAG_ENFORCE";

// Die definierende Datei selbst ist nie ihre eigene Quelle -- dort STEHEN die Wachen.
constexpr std::string_view kDefinitionsDatei = "algo_semver.hpp";

[[nodiscard]] std::string pfad_aus_umgebung(char const* variable, char const* vorgabe) {
    char const* p = std::getenv(variable);
    if (p != nullptr && *p != '\0') return std::string{p};
    return std::string{vorgabe};
}

[[nodiscard]] std::string quelle_pfad() { return pfad_aus_umgebung("COMDARE_TAX_QUELLE_PFAD", COMDARE_TAX_QUELLE); }
[[nodiscard]] std::string libs_pfad() { return pfad_aus_umgebung("COMDARE_TAX_LIBS_PFAD", COMDARE_TAX_LIBS); }
[[nodiscard]] std::string baum_pfad() { return pfad_aus_umgebung("COMDARE_TAX_BAUM_PFAD", COMDARE_TAX_BAUM); }

[[nodiscard]] std::string datei_lesen(fs::path const& p) {
    std::ifstream in(p, std::ios::binary);
    if (!in) return std::string{};
    std::ostringstream puffer;
    puffer << in.rdbuf();
    return puffer.str();
}

[[nodiscard]] std::vector<std::string> zeilen(std::string const& text) {
    std::vector<std::string> raus;
    std::string             zeile;
    std::istringstream      in(text);
    while (std::getline(in, zeile)) raus.push_back(zeile);
    return raus;
}

[[nodiscard]] std::size_t zaehle(std::string_view heu, std::string_view nadel) {
    if (nadel.empty()) return 0;
    std::size_t n = 0;
    for (std::size_t i = heu.find(nadel); i != std::string_view::npos; i = heu.find(nadel, i + nadel.size())) ++n;
    return n;
}

// -- DIE LISTE -------------------------------------------------------------------------------------

struct Liste {
    bool                     gefunden = false;
    std::string              block;   ///< der Text zwischen den beiden Ankern
    std::vector<char>        klassen; ///< die Klassen-Buchstaben in Auftretens-Reihenfolge
    std::string              fehler;  ///< nicht leer, wenn die Anker nicht sitzen
};

[[nodiscard]] Liste liste_lesen(fs::path const& quelle) {
    Liste       raus;
    std::string const text = datei_lesen(quelle);
    if (text.empty()) {
        raus.fehler = "Quelle nicht lesbar oder leer: " + quelle.string();
        return raus;
    }
    std::vector<std::string> const alle = zeilen(text);

    std::size_t anfang = alle.size();
    std::size_t ende   = alle.size();
    for (std::size_t i = 0; i < alle.size(); ++i) {
        if (anfang == alle.size() && alle[i].find(kBlockAnfang) != std::string::npos) anfang = i;
        if (ende == alle.size() && alle[i].rfind(kBlockEnde, 0) == 0) ende = i;
    }
    if (anfang == alle.size() || ende == alle.size() || anfang >= ende) {
        raus.fehler = "Listen-Anker sitzen nicht (Anfang/Ende nicht gefunden oder verdreht) in " + quelle.string();
        return raus;
    }

    std::string block;
    for (std::size_t i = anfang; i < ende; ++i) {
        block += alle[i];
        block += '\n';
        // Klassen-Marke: exakt "///   (x) " mit einem Kleinbuchstaben. Die Ziffern-Marken "(1)"/"(2)"
        // weiter unten in der Datei liegen ausserhalb des Blocks und werden von der Bereichs-Grenze
        // ausgeschlossen -- deshalb reicht hier die Form-Pruefung.
        std::string const& z = alle[i];
        if (z.size() >= 10 && z.rfind("///   (", 0) == 0 && z[8] == ')' && z[9] == ' ' && z[7] >= 'a' &&
            z[7] <= 'z') {
            raus.klassen.push_back(z[7]);
        }
    }
    raus.block    = std::move(block);
    raus.gefunden = true;
    return raus;
}

// -- DIE AUFRUF-STELLEN ----------------------------------------------------------------------------

struct Stelle {
    std::string relpfad;
    std::string basisname;
    std::size_t treffer = 0;
};

/// Alle Dateien unter `wurzel` (*.hpp/*.cpp), die mindestens eine der beiden Wachen in AUFRUF-Form
/// enthalten. `nur_aufrufform=false` liefert stattdessen die Zaehlung des generischen Kommandos (ohne
/// Klammer) -- das ist der Nenner, der nur berichtet und nie gepinnt wird.
void stellen_in(fs::path const& wurzel, fs::path const& relativ_zu, bool nur_aufrufform,
                bool definitionsdatei_ausnehmen, std::vector<Stelle>& raus) {
    std::error_code ec;
    if (!fs::exists(wurzel, ec)) return;

    std::string const w1 = std::string{kWacheWohlgeformt} + (nur_aufrufform ? "(" : "");
    std::string const w2 = std::string{kWacheCpuEnforce} + (nur_aufrufform ? "(" : "");

    fs::recursive_directory_iterator it(wurzel, fs::directory_options::skip_permission_denied, ec);
    fs::recursive_directory_iterator ende;
    for (; !ec && it != ende; it.increment(ec)) {
        if (!it->is_regular_file(ec)) continue;
        fs::path const&   p   = it->path();
        std::string const ext = p.extension().string();
        if (ext != ".hpp" && ext != ".cpp") continue;
        std::string const basis = p.filename().string();
        if (definitionsdatei_ausnehmen && basis == kDefinitionsDatei) continue;

        std::string const inhalt = datei_lesen(p);
        std::size_t const n      = zaehle(inhalt, w1) + zaehle(inhalt, w2);
        if (n == 0) continue;

        Stelle s;
        std::error_code rel_ec;
        fs::path const  rel = fs::relative(p, relativ_zu, rel_ec);
        s.relpfad           = rel_ec ? p.string() : rel.generic_string();
        s.basisname         = basis;
        s.treffer           = n;
        raus.push_back(std::move(s));
    }
}

[[nodiscard]] std::vector<Stelle> stellen_erheben(fs::path const& wurzel, bool nur_aufrufform,
                                                  bool definitionsdatei_ausnehmen) {
    std::vector<Stelle> raus;
    stellen_in(wurzel, wurzel, nur_aufrufform, definitionsdatei_ausnehmen, raus);
    std::sort(raus.begin(), raus.end(), [](Stelle const& a, Stelle const& b) { return a.relpfad < b.relpfad; });
    return raus;
}

/// Die VIER Zweige des bindenden Kommandos (algo_semver.hpp:127): "libs tests tools apps". Ausdruecklich
/// NICHT der ganze Quellbaum -- ein Bau-Verzeichnis oder ext/ wuerde den berichteten Nenner mit fremdem
/// Code auffuellen und ihn damit unvergleichbar zum Kommando machen.
[[nodiscard]] std::vector<Stelle> stellen_im_baum(fs::path const& baum, bool nur_aufrufform,
                                                  bool definitionsdatei_ausnehmen) {
    static constexpr std::string_view kZweige[] = {"libs", "tests", "tools", "apps"};
    std::vector<Stelle>               raus;
    for (std::string_view const zweig : kZweige) {
        stellen_in(baum / std::string{zweig}, baum, nur_aufrufform, definitionsdatei_ausnehmen, raus);
    }
    std::sort(raus.begin(), raus.end(), [](Stelle const& a, Stelle const& b) { return a.relpfad < b.relpfad; });
    return raus;
}

/// Die eine Urteils-Funktion: welche Aufruf-Stellen stehen NICHT in der Liste?
[[nodiscard]] std::vector<Stelle> unbenannte(std::vector<Stelle> const& alle, std::string const& block) {
    std::vector<Stelle> raus;
    for (Stelle const& s : alle) {
        if (block.find(s.basisname) == std::string::npos) raus.push_back(s);
    }
    return raus;
}

// -- WUERFEL ---------------------------------------------------------------------------------------

/// `bytes` Byte aus /dev/urandom als Hex. Leer, wenn die Quelle fehlt (dann wird der Fall uebersprungen --
/// ein Koeder mit vorhersagbarem Namen waere kein Koeder).
[[nodiscard]] std::string wuerfel_hex(std::size_t bytes) {
    std::ifstream ur("/dev/urandom", std::ios::binary);
    if (!ur) return std::string{};
    std::string raus;
    for (std::size_t i = 0; i < bytes; ++i) {
        unsigned char b = 0;
        ur.read(reinterpret_cast<char*>(&b), 1);
        if (!ur) return std::string{};
        char const ziffern[] = "0123456789abcdef";
        raus += ziffern[(b >> 4) & 0x0Fu];
        raus += ziffern[b & 0x0Fu];
    }
    return raus;
}

[[nodiscard]] std::size_t wuerfel_index(std::size_t obergrenze) {
    if (obergrenze == 0) return 0;
    std::ifstream ur("/dev/urandom", std::ios::binary);
    if (!ur) return 0;
    unsigned short roh = 0;
    ur.read(reinterpret_cast<char*>(&roh), sizeof(roh));
    if (!ur) return 0;
    return static_cast<std::size_t>(roh) % obergrenze;
}

[[nodiscard]] fs::path wegwerf_wurzel(std::string const& kennung) {
    fs::path     p = fs::temp_directory_path() / ("comdare_tax_" + kennung);
    std::error_code ec;
    fs::create_directories(p, ec);
    return p;
}

void schreiben(fs::path const& p, std::string const& inhalt) {
    std::ofstream out(p, std::ios::binary | std::ios::trunc);
    out << inhalt;
}

} // namespace

// Diese TU ist selbst ein Verbraucher der Wachen -- und liegt unter tests/, gehoert also ausdruecklich
// NICHT in den Nenner von libs/. Der static_assert belegt, dass die Symbole erreichbar sind; er ist die
// Gegenprobe dazu, dass der Riegel oben nur Text liest und nicht die Politik selbst.
static_assert(::comdare::cache_engine::measurement::ce_owned_version_is_wellformed("1.0.0.c"),
              "die ce-Politik-Wache ist aus dieser TU nicht erreichbar -- dann prueft der Riegel Text "
              "ueber eine Politik, die er nicht sieht");

// ==================================================================================================
// FALL 1 -- die Klassen-Leiter ist lueckenlos und schrumpft nicht
// ==================================================================================================
TEST(VsTaxonomieKlassenGrep, KlassenLeiterIstLueckenlosUndWaechstNurNachHinten) {
    Liste const l = liste_lesen(quelle_pfad());
    ASSERT_TRUE(l.gefunden) << l.fehler;

    std::vector<char> const& k = l.klassen;
    ASSERT_FALSE(k.empty()) << "die Klassen-Liste ist leer -- der Block-Anker sitzt falsch";

    // Die Ratsche: heute stehen (a)..(j) = 10 Klassen. Die Liste DARF wachsen (jede kuenftige Quelle
    // gehoert hinein), sie darf nur nicht schrumpfen. Eine gestrichene Klasse ist ein Fund, keine
    // Aufraeumung.
    constexpr std::size_t kKlassenUntergrenze = 10;

    std::cout << "[TAXONOMIE] Liste: " << quelle_pfad() << "\n";
    std::cout << "[TAXONOMIE] klassen gefunden " << k.size() << ", untergrenze " << kKlassenUntergrenze
              << ", erste '" << k.front() << "', letzte '" << k.back() << "'\n";

    EXPECT_GE(k.size(), kKlassenUntergrenze)
        << "die Klassen-Liste ist unter die Ratsche gefallen -- eine Klasse wurde gestrichen";
    EXPECT_EQ(k.front(), 'a') << "die Liste beginnt nicht bei (a)";

    std::size_t luecken = 0;
    for (std::size_t i = 0; i < k.size(); ++i) {
        char const soll = static_cast<char>('a' + static_cast<int>(i));
        if (k[i] != soll) {
            ++luecken;
            ADD_FAILURE() << "Klassen-Leiter unterbrochen an Position " << i << ": erwartet '" << soll
                          << "', gefunden '" << k[i] << "'";
        }
    }
    std::cout << "[TAXONOMIE] leiter zusammenhaengend: " << (k.size() - luecken) << "/" << k.size()
              << " positionen, " << luecken << " luecken\n";
    EXPECT_EQ(luecken, 0u);
}

// ==================================================================================================
// FALL 2 -- DER RIEGEL: jede produktive Aufruf-Stelle steht in der Liste
// ==================================================================================================
TEST(VsTaxonomieKlassenGrep, JedeProduktiveAufrufStelleStehtInDerListe) {
    Liste const l = liste_lesen(quelle_pfad());
    ASSERT_TRUE(l.gefunden) << l.fehler;

    std::vector<Stelle> const alle  = stellen_erheben(libs_pfad(), true, true);
    std::vector<Stelle> const offen = unbenannte(alle, l.block);

    std::cout << "[TAXONOMIE] baum: " << libs_pfad() << "\n";
    ASSERT_FALSE(alle.empty()) << "keine einzige Aufruf-Stelle unter " << libs_pfad()
                               << " gefunden -- die Erhebung selbst ist kaputt, nicht der Bestand";

    std::size_t treffer_summe = 0;
    for (Stelle const& s : alle) treffer_summe += s.treffer;

    std::cout << "[TAXONOMIE] produktive aufruf-stellen (libs/, *.hpp+*.cpp, ohne " << kDefinitionsDatei
              << "): " << (alle.size() - offen.size()) << "/" << alle.size() << " in der liste benannt, "
              << offen.size() << " unbenannt, " << treffer_summe << " aufrufe gesamt\n";
    for (Stelle const& s : alle) {
        std::cout << "[TAXONOMIE]   " << (l.block.find(s.basisname) == std::string::npos ? "UNBENANNT" : "benannt ")
                  << "  " << s.treffer << "x  " << s.relpfad << "\n";
    }

    for (Stelle const& s : offen) {
        ADD_FAILURE() << "ce-Versions-Quelle NICHT in der Klassen-Liste: libs/" << s.relpfad << " (" << s.treffer
                      << " Aufrufe der Politik-Wachen). Die Liste in algo_semver.hpp fordert selbst: 'Jede "
                         "KUENFTIGE ce-Versions-Quelle gehoert hier hinein'. Eintragen -- oder, wenn es keine "
                         "Quelle ist, die Aufruf-Form aus dem Text nehmen.";
    }
    EXPECT_EQ(offen.size(), 0u);
}

// ==================================================================================================
// FALL 3 -- Nenner-Bericht: der generische Wachen-Grep und die NICHT geprueften Mengen
// ==================================================================================================
TEST(VsTaxonomieKlassenGrep, GenerischerGrepNennerWirdBerichtetNichtGepinnt) {
    std::vector<Stelle> const generisch = stellen_im_baum(baum_pfad(), false, false);
    std::size_t               summe     = 0;
    for (Stelle const& s : generisch) summe += s.treffer;

    // Ausserhalb libs/: der ausdruecklich NICHT gepruefte Teil.
    std::size_t ausserhalb_dateien = 0;
    std::size_t ausserhalb_treffer = 0;
    for (Stelle const& s : stellen_im_baum(baum_pfad(), true, true)) {
        if (s.relpfad.rfind("libs/", 0) == 0) continue;
        ++ausserhalb_dateien;
        ausserhalb_treffer += s.treffer;
    }

    std::cout << "[TAXONOMIE] generischer wachen-grep (ohne Klammer, ganzer Baum " << baum_pfad()
              << "): " << summe << " treffer in " << generisch.size() << " dateien\n";
    std::cout << "[TAXONOMIE] NICHT gegenstand dieses riegels: " << ausserhalb_dateien
              << " aufruf-dateien ausserhalb libs/ mit " << ausserhalb_treffer << " aufrufen\n";
    std::cout << "[TAXONOMIE] die trefferzahl wird BERICHTET, nie gepinnt (algo_semver.hpp:122: "
                 "'Momentaufnahmen ... altern')\n";

    EXPECT_GT(summe, 0u) << "der generische Grep findet nichts -- dann misst dieser Test am falschen Baum";
    EXPECT_GT(generisch.size(), 0u);
}

// ==================================================================================================
// FALL 4 -- KOEDER A, beidseitig: eine NEUE Quelle ohne Listen-Eintrag MUSS auffallen
// ==================================================================================================
TEST(VsTaxonomieKlassenGrep, KoederNeueQuelleOhneEintragBeisstBeidseitig) {
    std::string const kennung = wuerfel_hex(8);
    if (kennung.empty()) GTEST_SKIP() << "kein /dev/urandom -- ein Koeder mit festem Namen waere keiner";

    Liste const echt = liste_lesen(quelle_pfad());
    ASSERT_TRUE(echt.gefunden) << echt.fehler;

    fs::path const    baum       = wegwerf_wurzel(kennung);
    std::string const koedername = "koeder_" + kennung + ".hpp";
    schreiben(baum / koedername, std::string{"// erfundene ce-Versions-Quelle, Kennung "} + kennung + "\n" +
                                     "static_assert(" + std::string{kWacheWohlgeformt} + "(kErfunden" + kennung +
                                     "));\n" + "static_assert(" + std::string{kWacheCpuEnforce} + "(kErfunden" +
                                     kennung + "));\n");

    std::vector<Stelle> const gefunden = stellen_erheben(baum, true, true);
    ASSERT_EQ(gefunden.size(), 1u) << "der Wegwerf-Baum traegt genau eine Datei";
    EXPECT_EQ(gefunden.front().basisname, koedername);
    EXPECT_EQ(gefunden.front().treffer, 2u);

    // KOEDER-SEITE: gegen die ECHTE Liste ist der erfundene Name unbekannt -> Biss.
    std::vector<Stelle> const biss = unbenannte(gefunden, echt.block);
    std::cout << "[KOEDER-A] kennung " << kennung << ", baum " << baum.string() << "\n";
    std::cout << "[KOEDER-A] gegen die ECHTE liste: " << biss.size() << "/1 unbenannt (erwartet 1)\n";
    ASSERT_EQ(biss.size(), 1u) << "der Koeder hat NICHT gebissen -- eine erfundene Quelle blieb unbemerkt";
    EXPECT_EQ(biss.front().basisname, koedername) << "gebissen hat er, aber an der falschen Datei";

    // GEGENEINGANG: dieselbe Datei gegen eine Liste, die genau diesen gewuerfelten Namen nennt -> kein Biss.
    std::string const block_mit = echt.block + "///   (z) KOEDER-GEGENEINGANG: " + koedername + "\n";
    std::vector<Stelle> const kein_biss = unbenannte(gefunden, block_mit);
    std::cout << "[KOEDER-A] gegen eine liste MIT dem eintrag: " << kein_biss.size() << "/1 unbenannt (erwartet 0)\n";
    EXPECT_EQ(kein_biss.size(), 0u)
        << "der Riegel meldet auch dann, wenn die Datei genannt IST -- dann prueft er nicht die Liste, "
           "sondern nur die Existenz der Datei";

    std::error_code ec;
    fs::remove_all(baum, ec);
}

// ==================================================================================================
// FALL 5 -- KOEDER B, beidseitig: faellt eine BESTEHENDE Quelle aus der Liste, wird er rot
// ==================================================================================================
TEST(VsTaxonomieKlassenGrep, KoederGestricheneQuelleBeisstBeidseitig) {
    std::string const kennung = wuerfel_hex(8);
    if (kennung.empty()) GTEST_SKIP() << "kein /dev/urandom -- ohne Wuerfel keine unabhaengige Auswahl";

    Liste const echt = liste_lesen(quelle_pfad());
    ASSERT_TRUE(echt.gefunden) << echt.fehler;

    std::vector<Stelle> const alle = stellen_erheben(libs_pfad(), true, true);
    ASSERT_FALSE(alle.empty());
    std::vector<Stelle> benannt;
    for (Stelle const& s : alle) {
        if (echt.block.find(s.basisname) != std::string::npos) benannt.push_back(s);
    }
    ASSERT_FALSE(benannt.empty()) << "keine einzige Quelle ist gelistet -- dann ist Fall 2 der eigentliche Befund";

    // GEGENEINGANG ZUERST (T-4): die unveraenderte Liste darf NICHTS melden.
    std::vector<Stelle> const vorher = unbenannte(alle, echt.block);
    std::cout << "[KOEDER-B] gegeneingang, unveraenderte liste: " << vorher.size() << " unbenannt (erwartet 0)\n";
    ASSERT_EQ(vorher.size(), 0u) << "schon ohne Koeder unbenannte Quellen -- Fall 2 ist rot, hier ist kein "
                                    "sauberer Gegeneingang moeglich";

    // Der Wuerfel entscheidet, WELCHE gelistete Quelle gestrichen wird.
    std::size_t const  idx      = wuerfel_index(benannt.size());
    std::string const& gestrich = benannt[idx].basisname;

    std::string block_ohne = echt.block;
    std::string const ersatz = "<gestrichen-durch-koeder-" + kennung + ">";
    for (std::size_t i = block_ohne.find(gestrich); i != std::string::npos; i = block_ohne.find(gestrich, i)) {
        block_ohne.replace(i, gestrich.size(), ersatz);
        i += ersatz.size();
    }
    ASSERT_EQ(block_ohne.find(gestrich), std::string::npos) << "die Streichung war unvollstaendig";

    std::vector<Stelle> const nachher = unbenannte(alle, block_ohne);
    std::cout << "[KOEDER-B] kennung " << kennung << ", gewuerfelt " << (idx + 1) << "/" << benannt.size()
              << " -> gestrichen: " << gestrich << "\n";
    std::cout << "[KOEDER-B] nach der streichung: " << nachher.size() << " unbenannt (erwartet genau 1)\n";

    ASSERT_EQ(nachher.size(), 1u) << "eine aus der Liste gefallene Quelle bleibt unbemerkt -- genau die "
                                     "Selbstwartung, die dieser Riegel erzwingen soll";
    EXPECT_EQ(nachher.front().basisname, gestrich) << "gebissen hat er, aber an der falschen Datei";
}
