// tools/overlay_source_hash_gen/main.cpp -- E-E (07.08.2026): DER PRE-BUILD-CODEGEN des Overlay-Glieds [7].
//
// -- WAS ER TUT ----------------------------------------------------------------------------------------
//   1. Er ermittelt die Dateimenge in der Ordnung Organ -> System -> Mess -> Tier-Substanz aus der EINEN
//      Quelle builder/overlay_source_set.hpp (dort stehen der Schnitt, die Ordnungs-Wachen und die
//      Begruendung des Owner-Entscheids).
//   2. Er KONKATENIERT die Bytes aller Dateien in genau dieser Ordnung und rechnet EINMAL SHA-512
//      darueber (Owner-Entscheid (1): Konkatenation, nicht Hash je Datei).
//   3. Er schreibt das Ergebnis als Compile-Define-Header, aus dem
//      -DCOMDARE_OVERLAY_SOURCE_HASH="<128-hex>" in jede Uebersetzung reist.
//
// -- WARUM PRE-BUILD UND NICHT ZUR LAUFZEIT ------------------------------------------------------------
// Owner (26.07., verbatim): "dann muss er nicht zur Laufzeit berechnet werden und die binary ist
// eindeutig." Ein consteval-Hash kann keine Dateien lesen; jede Laufzeit-Variante braeche zusaetzlich die
// Doktrin "compile-time only". Das Muster ist im Haus etabliert: is_original_validator (cmake/
// is_original_codegen.cmake) ist derselbe Bau -- C++-Werkzeug, add_custom_command, generierter Header.
//
// -- DETERMINISMUS, UND WIE ER BEWIESEN WIRD -----------------------------------------------------------
// Dieselbe Quelle MUSS denselben Hash liefern -- unabhaengig von Dateisystem-Reihenfolge, Zeitstempeln
// oder Locale. Drei Massnahmen, je eine pro Fehlerquelle:
//   * REIHENFOLGE: recursive_directory_iterator liefert eine UNSPEZIFIZIERTE Ordnung. Die Dateien eines
//     Eintrags werden deshalb gesammelt und ueber ihren relativen Pfad BYTE-WEISE sortiert
//     (std::string::operator<, kein locale-abhaengiger Vergleich, kein std::collate).
//   * ZEITSTEMPEL/METADATEN gehen gar nicht erst ein -- nur der INHALT.
//   * LOCALE: es wird nichts formatiert und nichts verglichen, was von ihr abhinge; die Hex-Ausgabe
//     benutzt eine eigene Ziffern-Tabelle.
// Symbolische Links werden NICHT verfolgt (sonst kaeme dieselbe Datei doppelt oder ein Zyklus).
//
// -- FAIL-LOUD, UEBERALL -------------------------------------------------------------------------------
// Ein stiller Ausfall waere hier das Schlimmste: ein leeres oder unvollstaendiges Glied wuerde eine
// FALSCHE Identitaet zementieren, und zwar genau an der Stelle, an der niemand mehr nachsieht. Deshalb
// bricht dieses Werkzeug mit benannter Fehlerklasse bei:
//   * einem Schnitt-Pfad, den es nicht gibt (eine Umbenennung darf den Schnitt nicht still verkleinern),
//   * einem Praefix-Eintrag, der KEINE Datei trifft (dito),
//   * einer Datei-Endung, die weder in kQuellEndungen noch in kNichtQuellEndungen steht (eine neue
//     Endung MUSS eine Entscheidung erzwingen, statt still in den Hash zu rutschen oder herauszufallen),
//   * einer Datei, die sich nicht lesen laesst,
//   * einer leeren Gesamtmenge.
//
// ASCII-only (Umlaute als ae/oe/ue/ss).

#include <builder/overlay_source_set.hpp>

#include <sha512/ctsha512.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace fs  = std::filesystem;
namespace ovl = comdare::cache_engine::builder::overlay;

namespace {

[[noreturn]] void fehler(std::string const& text) { throw std::runtime_error("fehlerklasse=overlay_schnitt: " + text); }

/// Gehoert die Endung in den Hash? Fail-loud bei einer Endung, die in KEINER der beiden Listen steht.
[[nodiscard]] bool ist_quelldatei(fs::path const& p) {
    std::string const endung = p.extension().string();
    for (auto const& e : ovl::kQuellEndungen)
        if (endung == e) return true;
    for (auto const& e : ovl::kNichtQuellEndungen)
        if (endung == e) return false;
    fehler("die Datei '" + p.string() + "' traegt die Endung '" + endung +
           "', die WEDER in kQuellEndungen NOCH in kNichtQuellEndungen steht (builder/overlay_source_set.hpp). "
           "Das ist Absicht: eine neue Endung muss eine Entscheidung erzwingen -- gehoert ihr Inhalt in die "
           "Identitaet der Tier-Binary oder nicht? Endung in eine der beiden Listen eintragen; ein Nachtrag in "
           "kQuellEndungen bewegt den Fingerprint und ist damit ein deklariertes Byte-Ereignis.");
}

/// Die Dateien EINES Schnitt-Eintrags, byte-weise nach relativem Pfad sortiert.
[[nodiscard]] std::vector<std::string> dateien_des_eintrags(fs::path const& wurzel, ovl::Eintrag const& e) {
    fs::path const basis = wurzel / fs::path{std::string{e.pfad}};
    if (!fs::exists(basis) || !fs::is_directory(basis))
        fehler("der Schnitt-Pfad '" + std::string{e.pfad} + "' (Achse '" + std::string{e.achse} +
               "') existiert nicht als Verzeichnis unter '" + wurzel.string() +
               "'. Wurde er umbenannt oder verschoben, MUSS builder/overlay_source_set.hpp nachgezogen werden -- "
               "ein stilles Ueberspringen wuerde den Schnitt verkleinern, ohne dass es jemand merkt.");

    std::vector<std::string> aus;
    auto                     aufnehmen = [&](fs::path const& p) {
        if (!fs::is_regular_file(fs::symlink_status(p)) && !fs::is_regular_file(p)) return;
        if (fs::is_symlink(fs::symlink_status(p))) return; // s. Kopf: keine Zweitzaehlung ueber Links
        if (!ist_quelldatei(p)) return;
        aus.push_back(fs::relative(p, wurzel).generic_string());
    };

    if (e.form == ovl::Form::verzeichnis) {
        for (auto const& d : fs::recursive_directory_iterator(basis, fs::directory_options::none)) {
            if (d.is_directory()) continue;
            aufnehmen(d.path());
        }
    } else {
        // Praefix-Form: FLACH im genannten Verzeichnis, Datei-Name beginnt mit dem Praefix.
        std::string const praefix{e.praefix};
        for (auto const& d : fs::directory_iterator(basis)) {
            if (d.is_directory()) continue;
            std::string const name = d.path().filename().string();
            if (name.rfind(praefix, 0) != 0) continue;
            aufnehmen(d.path());
        }
        if (aus.empty())
            fehler("das Datei-Praefix '" + praefix + "' (Achse '" + std::string{e.achse} + "') trifft in '" +
                   std::string{e.pfad} +
                   "' KEINE Quelldatei. Entweder wurde die Namens-Konvention gebrochen oder die Achse ist "
                   "umgezogen -- in beiden Faellen ist der Schnitt nachzuziehen, nicht der Treffer zu ignorieren.");
    }

    std::sort(aus.begin(), aus.end()); // byte-weise, locale-frei -- die Determinismus-Zusage des Kopfes
    return aus;
}

/// Liest eine Datei vollstaendig als Bytes. Fail-loud, wenn sie sich nicht oeffnen/lesen laesst.
void anhaengen(std::vector<std::uint8_t>& senke, fs::path const& datei) {
    std::ifstream in{datei, std::ios::binary};
    if (!in) fehler("die Quelldatei '" + datei.string() + "' laesst sich nicht oeffnen.");
    std::vector<char> const bytes{std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
    if (in.bad()) fehler("beim Lesen von '" + datei.string() + "' ist ein E/A-Fehler aufgetreten.");
    senke.reserve(senke.size() + bytes.size());
    for (char const c : bytes) senke.push_back(static_cast<std::uint8_t>(c));
}

/// Schreibt die Datei NUR, wenn ihr Inhalt sich aendert. Das ist kein Feinschliff, sondern noetig: der
/// generierte Header haengt an jeder Uebersetzungseinheit, die den Fingerprint traegt. Wuerde er bei
/// jedem Bau neu geschrieben, loeste ein unveraenderter Quellbaum eine Voll-Neuuebersetzung aus.
[[nodiscard]] bool schreibe_wenn_anders(fs::path const& ziel, std::string const& inhalt) {
    if (fs::exists(ziel)) {
        std::ifstream     alt{ziel, std::ios::binary};
        std::string const vorher{std::istreambuf_iterator<char>{alt}, std::istreambuf_iterator<char>{}};
        if (vorher == inhalt) return false;
    }
    fs::create_directories(ziel.parent_path());
    std::ofstream aus{ziel, std::ios::binary | std::ios::trunc};
    if (!aus) fehler("der generierte Header '" + ziel.string() + "' laesst sich nicht schreiben.");
    aus << inhalt;
    if (!aus) fehler("beim Schreiben von '" + ziel.string() + "' ist ein E/A-Fehler aufgetreten.");
    return true;
}

[[nodiscard]] std::string kategorie_text(ovl::Kategorie k) {
    switch (k) {
        case ovl::Kategorie::organ: return "organ";
        case ovl::Kategorie::system: return "system";
        case ovl::Kategorie::mess: return "mess";
        case ovl::Kategorie::tier_substanz: return "tier_substanz";
    }
    return "?";
}

void hilfe() {
    std::cout << "overlay_source_hash_gen -- Pre-Build-Codegen des Overlay-Glieds [7]\n"
                 "  --root <verzeichnis>   Wurzel der cache_engine (libs/cache_engine), PFLICHT\n"
                 "  --output <header>      Ziel des generierten Compile-Define-Headers\n"
                 "  --listing <datei>      optional: das Datei-Manifest (eine Zeile je Datei)\n"
                 "  --print                den 128-hex auf stdout ausgeben\n";
}

} // namespace

int main(int argc, char** argv) try {
    fs::path wurzel;
    fs::path ausgabe;
    fs::path listing;
    bool     drucken = false;

    for (int i = 1; i < argc; ++i) {
        std::string_view const a{argv[i]};
        auto                   wert = [&](char const* name) -> std::string {
            if (i + 1 >= argc) fehler(std::string{"zu "} + name + " fehlt der Wert.");
            return std::string{argv[++i]};
        };
        if (a == "--root")
            wurzel = wert("--root");
        else if (a == "--output")
            ausgabe = wert("--output");
        else if (a == "--listing")
            listing = wert("--listing");
        else if (a == "--print")
            drucken = true;
        else if (a == "--help" || a == "-h") {
            hilfe();
            return 0;
        } else
            fehler("unbekanntes Argument '" + std::string{a} + "'.");
    }
    if (wurzel.empty()) fehler("--root fehlt (die Wurzel der cache_engine).");
    if (!fs::is_directory(wurzel)) fehler("--root '" + wurzel.string() + "' ist kein Verzeichnis.");

    // -- (1) die Dateimenge in der KANONISCHEN Ordnung, Eintrag fuer Eintrag --------------------------
    std::vector<std::string> alle;     // relative Pfade, in Preimage-Ordnung
    std::vector<std::string> manifest; // dieselbe Ordnung, mit Kategorie/Achse fuer das Listing
    for (auto const& e : ovl::kOverlaySourceSet) {
        for (auto const& rel : dateien_des_eintrags(wurzel, e)) {
            manifest.push_back(kategorie_text(e.kategorie) + "\t" + std::string{e.achse} + "\t" + rel);
            alle.push_back(rel);
        }
    }
    if (alle.empty()) fehler("die Dateimenge des Schnitts ist LEER -- das kann nicht stimmen.");

    // Dieselbe Datei darf nicht zweimal in den Hash. Die Pfad-Eintraege sind compile-time paarweise
    // verschieden (static_assert im Schnitt-Header), aber zwei Eintraege koennten sich UEBERLAPPEN
    // (etwa ein Verzeichnis und ein Unterverzeichnis davon). Das faellt hier auf, nicht spaeter.
    {
        std::vector<std::string> sortiert = alle;
        std::sort(sortiert.begin(), sortiert.end());
        auto const dopp = std::adjacent_find(sortiert.begin(), sortiert.end());
        if (dopp != sortiert.end())
            fehler("die Datei '" + *dopp +
                   "' steht ZWEIMAL im Schnitt -- zwei Eintraege in builder/overlay_source_set.hpp "
                   "ueberlappen sich. Ihre Bytes gingen doppelt ins Preimage.");
    }

    // -- (2) KONKATENATION + EINMAL SHA-512 ------------------------------------------------------------
    std::vector<std::uint8_t> bytes;
    for (auto const& rel : alle) anhaengen(bytes, wurzel / rel);

    auto const digest =
        ::comdare::cache_engine::sha512::sha512(std::span<std::uint8_t const>{bytes.data(), bytes.size()});
    auto const        hexa = ::comdare::cache_engine::sha512::to_hex(digest);
    std::string const hex(hexa.data(), hexa.size());

    // -- (3) der Compile-Define-Header -----------------------------------------------------------------
    // Das #ifndef ist Absicht und traegt eine Rangfolge: ein per -D gereichter Wert GEWINNT. Auf dem
    // produktiven Tier-Pfad haengt die CEB -DCOMDARE_OVERLAY_SOURCE_HASH an die Uebersetzung (siehe
    // profile_facade/overlay_source_hash_naht.hpp) und reicht damit IHREN Wert weiter -- den, gegen den
    // sie die Binary spaeter auch wiedererkennt. Der Header ist der Weg fuer alles, was CMake selbst
    // uebersetzt (die CEB, die Tests, die Werkzeuge).
    std::string kopf;
    kopf += "#pragma once\n";
    kopf += "// GENERIERT von tools/overlay_source_hash_gen -- NICHT von Hand editieren.\n";
    kopf += "// E-E: das Overlay-Glied [7] des Tier-Fingerprints (abi/anatomy_fingerprint.hpp).\n";
    kopf += "// EINMAL SHA-512 ueber die KONKATENATION aller Quelldateien des Schnitts, in der Ordnung\n";
    kopf += "// Organ -> System -> Mess -> Tier-Substanz (builder/overlay_source_set.hpp).\n";
    kopf += "// Dateien im Schnitt: " + std::to_string(alle.size()) + "\n";
    kopf += "// Bytes im Preimage:  " + std::to_string(bytes.size()) + "\n";
    kopf += "//\n";
    kopf += "// Ein per -D gereichter Wert GEWINNT (das #ifndef unten): auf dem Tier-Pfad reicht die CEB\n";
    kopf += "// ihren eigenen Wert herein -- den, gegen den sie die Binary spaeter wiedererkennt.\n";
    kopf += "#ifndef COMDARE_OVERLAY_SOURCE_HASH\n";
    kopf += "#define COMDARE_OVERLAY_SOURCE_HASH \"" + hex + "\"\n";
    kopf += "#endif\n";

    if (!ausgabe.empty()) {
        bool const neu = schreibe_wenn_anders(ausgabe, kopf);
        std::cout << "[overlay_source_hash_gen] " << alle.size() << " Dateien, " << bytes.size()
                  << " Bytes, sha512=" << hex.substr(0, 16) << "... "
                  << (neu ? "(Header NEU geschrieben)" : "(unveraendert)") << "\n";
    }
    if (!listing.empty()) {
        std::string text;
        text += "# GENERIERT von tools/overlay_source_hash_gen -- das Manifest des Overlay-Schnitts.\n";
        text += "# Spalten: kategorie<TAB>achse<TAB>pfad (relativ zu libs/cache_engine/).\n";
        text += "# Die Reihenfolge IST die Preimage-Reihenfolge.\n";
        text += "# sha512 ueber die Konkatenation: " + hex + "\n";
        for (auto const& z : manifest) text += z + "\n";
        (void)schreibe_wenn_anders(listing, text);
    }
    if (drucken || (ausgabe.empty() && listing.empty())) std::cout << hex << "\n";
    return 0;
} catch (std::exception const& ex) {
    std::cerr << "overlay_source_hash_gen: " << ex.what() << "\n";
    return 1;
}
