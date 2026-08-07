// tests/unit/test_d4_ceb_schluessel_wahl.cpp -- M-1/D-4 (06.08.2026): DER BISS.
//
// BEFUND, den dieser Test einklemmt: bis D-4 iterierte ceb_measurement_stamp_array() consteval ueber die
// KOMPLETTE kMeasurementToolingRegistry. kCebFingerprint hing damit an nichts, was die einkompilierte
// Mess-Combo (COMDARE_MEASUREMENT_COMBO_CT) kennt -- vier CEBs, gebaut mit [wallclock] / [macro] / [micro] /
// [all], trugen DENSELBEN ceb_key_sha512. Das ist eine Injektivitaets-Verletzung an der Naht, ueber die eine
// Bestandslog-Reservierung ihrer emittierenden CEB zugeordnet wird (profile_run_facade.cpp ->
// bestandslog_document.hpp ceb_key_sha512), und bricht Owner-KERN F6 auf der CEB-Ebene.
// Paragraf 58-V (LEDGER:3120) verlangt ein Mess-Array "je EINKOMPILIERTER Mess-Achse".
//
// WARUM DER BISS IN EINER EINZIGEN TU MOEGLICH IST: kCebFingerprint ist pro Bau EINE Konstante -- ein Test,
// der nur sie liest, kann Injektivitaet grundsaetzlich nicht zeigen (er saehe genau einen Wert). Deshalb ist
// der Renderer seit D-4 ueber den Legenden-Traeger CebComboLegend parametrisiert: kCebFingerprintFor<L> ist
// fuer JEDE Legende zur Compile-Zeit ausrechenbar, und die einkompilierte Konstante ist nur noch die
// Spezialisierung an EINER davon. Der Beweis braucht damit keine N Baue mehr -- und ist deshalb ueberhaupt
// eine dauerhafte Wache statt einer einmaligen Handmessung.
//
// ERGAENZEND ZUM OBJEKT-NACHWEIS: dieselbe Aussage ist zusaetzlich ueber N GETRENNTE UEBERSETZUNGEN einer
// Probe-TU gemessen worden (Session-Protokoll M-1/D-4); dieser Test ist die Wache, die sie festhaelt.
//
// ASCII-only.

#include "builder/ceb_version_stamp.hpp"            // D-4: kCebFingerprintFor<L> / kCebMeasurementStampFor<L>
#include <cache_engine/abi/anatomy_fingerprint.hpp> // anatomy_fingerprint_hex (Zweitweg-Nachrechnung)
// measurement_stamp_line_from_combo_legend -- der Runtime-Zwilling des consteval-Renderers
#include <cache_engine/abi/anatomy_version_stamp.hpp>
#include <cache_engine/measurement/measurement_tooling_registry.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

namespace {

namespace ceb  = ::comdare::cache_engine::builder;
namespace cabi = ::comdare::cache_engine::abi;
namespace cm   = ::comdare::cache_engine::measurement;

/// Die SIEBEN nicht-leeren Teilmengen der drei Toolings, in Registry-Reihenfolge notiert -- plus die zwei
/// Schreibweisen der Vollmenge. Handgeschrieben und NICHT aus der Registry generiert: der Legenden-Traeger
/// ist ein NTTP, seine Werte muessen Literale sein. Die Zahl 7 wird unten gegen 2^kMeasurementToolingCount-1
/// geprueft, damit ein viertes Tooling diesen Test rot macht statt ihn still unvollstaendig zu lassen.
struct LegendeUndSchluessel {
    std::string_view legende;
    std::string_view schluessel;
    std::string_view zeile;
};

/// runtime_ceb_key(mess) -- der ZWEITE, UNABHAENGIGE Ableitungsweg desselben Schluessels.
/// Er benutzt bewusst NICHT anatomy_fingerprint_hex (das ist der consteval-Weg, den wir pruefen wollen),
/// sondern die LAUFZEIT-Kette anatomy_fingerprint_glieder -> anatomy_fingerprint_preimage -> sha512 -> hex,
/// also genau den Weg, den auch der Lager-Index faehrt (bestandslog_index.hpp derive_key_from_lines).
/// Kaemen beide Wege auseinander, waere der CEB-Schluessel im Log-Kopf ein anderer als im Lager.
[[nodiscard]] std::string runtime_ceb_key(std::string const& mess) {
    auto const        glieder = cabi::anatomy_fingerprint_glieder("", "", mess);
    std::string const pre =
        cabi::anatomy_fingerprint_preimage(std::span<std::string_view const>{glieder.data(), glieder.size()});
    auto const digest = ::comdare::cache_engine::sha512::sha512(
        std::span<std::uint8_t const>{reinterpret_cast<std::uint8_t const*>(pre.data()), pre.size()});
    auto const hex = ::comdare::cache_engine::sha512::to_hex(digest);
    return std::string(hex.data(), 128);
}

/// Die Tabelle. Jede Zeile ist EINE Compile-Zeit-Auswertung des CEB-Schluessels an einer Legende.
[[nodiscard]] std::vector<LegendeUndSchluessel> alle_teilmengen() {
    return {
        {"[wallclock]", ceb::kCebFingerprintFor<ceb::CebComboLegend{"[wallclock]"}>,
         ceb::kCebMeasurementStampFor<ceb::CebComboLegend{"[wallclock]"}>},
        {"[macro]", ceb::kCebFingerprintFor<ceb::CebComboLegend{"[macro]"}>,
         ceb::kCebMeasurementStampFor<ceb::CebComboLegend{"[macro]"}>},
        {"[micro]", ceb::kCebFingerprintFor<ceb::CebComboLegend{"[micro]"}>,
         ceb::kCebMeasurementStampFor<ceb::CebComboLegend{"[micro]"}>},
        {"[wallclock,macro]", ceb::kCebFingerprintFor<ceb::CebComboLegend{"[wallclock,macro]"}>,
         ceb::kCebMeasurementStampFor<ceb::CebComboLegend{"[wallclock,macro]"}>},
        {"[wallclock,micro]", ceb::kCebFingerprintFor<ceb::CebComboLegend{"[wallclock,micro]"}>,
         ceb::kCebMeasurementStampFor<ceb::CebComboLegend{"[wallclock,micro]"}>},
        {"[macro,micro]", ceb::kCebFingerprintFor<ceb::CebComboLegend{"[macro,micro]"}>,
         ceb::kCebMeasurementStampFor<ceb::CebComboLegend{"[macro,micro]"}>},
        {"[wallclock,macro,micro]", ceb::kCebFingerprintFor<ceb::CebComboLegend{"[wallclock,macro,micro]"}>,
         ceb::kCebMeasurementStampFor<ceb::CebComboLegend{"[wallclock,macro,micro]"}>},
    };
}

} // namespace

// -- BISS-SEITE 1: VERSCHIEDENE COMBO => VERSCHIEDENER SCHLUESSEL -------------------------------------
//
// Der Kern-Nachweis. Er zaehlt Nenner UND Kollisionen aus und gibt beide aus -- eine nackte Null waere
// kein Befund (Fallen-Kanon): ein Test, der nur "keine Kollision" behauptet, ohne zu sagen, ueber wie
// viele Paare er geschaut hat, kann auch bei einer leeren Tabelle gruen sein.
TEST(D4CebSchluesselWahl, VerschiedeneEinkompilierteComboLiefertVerschiedenenSchluessel) {
    auto const tab = alle_teilmengen();

    // Vollstaendigkeits-Wache: die Tabelle MUSS alle 2^N-1 nicht-leeren Teilmengen fuehren.
    std::size_t erwartet = 1;
    for (std::size_t i = 0; i < cm::kMeasurementToolingCount; ++i) erwartet *= 2;
    erwartet -= 1;
    ASSERT_EQ(tab.size(), erwartet)
        << "Die Tooling-Registry hat " << cm::kMeasurementToolingCount
        << " Eintraege -- diese Tabelle deckt nicht mehr alle nicht-leeren Teilmengen ab. Ein neues Tooling "
           "muss hier eingetragen werden, sonst prueft der Biss still weniger als er behauptet.";

    std::size_t paare       = 0;
    std::size_t kollisionen = 0;
    for (std::size_t i = 0; i < tab.size(); ++i)
        for (std::size_t j = i + 1; j < tab.size(); ++j) {
            ++paare;
            if (tab[i].schluessel == tab[j].schluessel) {
                ++kollisionen;
                ADD_FAILURE() << "KOLLISION: '" << tab[i].legende << "' und '" << tab[j].legende
                              << "' tragen denselben ceb_key_sha512 (" << tab[i].schluessel.substr(0, 24)
                              << "...) -- das ist genau der Vor-D-4-Zustand (Paragraf 58-V / Owner-KERN F6).";
            }
            // Die Kollision der SCHLUESSEL kann nur aus einer Kollision der ZEILEN kommen (SHA-512).
            // Beide getrennt zu pruefen zeigt, WO ein kuenftiger Bruch sitzt.
            EXPECT_NE(tab[i].zeile, tab[j].zeile)
                << "Mess-Zeile identisch fuer '" << tab[i].legende << "' und '" << tab[j].legende << "'";
        }

    std::cout << "[D-4 BISS] nicht-leere Tooling-Teilmengen=" << tab.size() << ", verglichene Paare=" << paare
              << ", Kollisionen=" << kollisionen << "\n";
    EXPECT_EQ(paare, std::size_t{21}) << "Nenner-Wache: 7 Teilmengen -> 21 Paare";
    EXPECT_EQ(kollisionen, std::size_t{0});
}

// -- BISS-SEITE 2: DIESELBE COMBO => DERSELBE SCHLUESSEL ----------------------------------------------
//
// Die Rueckseite von F6. Sie ist NICHT tautologisch nachgewiesen (zwei identische NTTPs sind dieselbe
// Variable, das zeigt nichts), sondern ueber einen ZWEITEN, UNABHAENGIGEN Ableitungsweg: die Mess-Zeile
// wird vom RUNTIME-Renderer (cabi::measurement_stamp_line_from_combo_legend) neu gebaut und der SHA-512
// aus ihr neu gerechnet. Beide Wege muessen auf dasselbe Byte kommen.
TEST(D4CebSchluesselWahl, GleicheComboLiefertDenselbenSchluesselUeberZweiUnabhaengigeWege) {
    auto const tab = alle_teilmengen();
    for (auto const& e : tab) {
        // (a) Zeile: consteval-Zwilling == Runtime-Renderer an derselben Legende (der Drift-Guard, jetzt
        //     ueber ALLE sieben Teilmengen statt nur ueber die Vollmenge).
        EXPECT_EQ(std::string{e.zeile}, cabi::measurement_stamp_line_from_combo_legend(e.legende))
            << "Drift zwischen consteval-CEB-Zeile und Runtime-Renderer bei '" << e.legende << "'";
        // (b) Schluessel: aus der Zeile neu gerechnet == der gespeicherte Schluessel.
        std::string const preimage_hash = runtime_ceb_key(std::string{e.zeile});
        EXPECT_EQ(std::string{e.schluessel}, preimage_hash)
            << "SHA-512-Nachrechnung weicht ab bei '" << e.legende << "'";
        EXPECT_EQ(e.schluessel.size(), std::size_t{128});
    }
    std::cout << "[D-4 BISS] Zweitweg-Nachrechnung fuer " << tab.size() << " Legenden bestanden\n";
}

// -- BYTE-ANKER: DER ALT-BESTAND WIRD NICHT ENTWERTET -------------------------------------------------
//
// Auftrag Punkt 4. Jeder heutige Bestandslog-Eintrag stammt von einer CEB OHNE einkompilierte Combo
// (die Cache-Variable ist per Default leer, und der Emissionsweg loescht sie fuer [all]-Laeufe aktiv per
// -U, experiment_plan_director.hpp F-B1). Der no-define-Zweig rendert die Vollmenge -- byte-identisch zum
// Vor-D-4-Stand. Dieser Anker haelt genau das fest: waere die Vollmengen-Zeile je gewandert, waeren alle
// bestehenden ceb_key_sha512 entwertet, und das MUSS ein deklariertes Byte-Ereignis sein, kein Nebeneffekt.
TEST(D4CebSchluesselWahl, VollmengeIstByteStabilZumVorD4Stand) {
    // FLAG-GRAMMATIK v2 (Owner-KERN 07.08.2026) -- NEU-ANKER ALS DAS DEKLARIERTE BYTE-EREIGNIS, DAS
    // DIESER TEST FORDERT.
    //
    // Die CEB-Mess-Array-Zeile rendert die Tooling-Versionen; deren Schreibweise wandert von "@1.0.0c"
    // (Q3) auf "@1.0.0.c" (v2, Punkt vor jedem Flag). kCebFingerprintFor rechnet ueber
    // anatomy_fingerprint_hex ueber genau diese Zeile, also zwangslaeufig mit. DAS WAR VORHERGESAGT UND
    // IST GEWOLLT: dieser Test ist die Wache, die verhindert, dass es STILL geschieht -- er ist am
    // v2-Bau ROT geworden, und der neue Wert ist aus dem literalen Testlauf uebernommen, nicht
    // vorausberechnet.
    //   HISTORIE: Format 3 004251f467c004a88f...f76c8d8; Format 4 (R-3) db7bac00b3de6eef05...aa832234
    //
    // WARUM DAS TRAGBAR IST: ceb_key_sha512 hat GEMESSEN null Lese-Stellen (ceb_version_stamp.hpp -- reine
    // Provenienz; die Suche nach einem Vergleich auf dem Feld liefert 0 Treffer in libs/, apps/ und
    // tests/). Alt-Bestandslog-Zeilen behalten ihren alten Schluessel und werden von niemandem
    // gegengerechnet. Der Alt-Bestand wird nicht ENTWERTET, er wird nur nicht mehr fortgeschrieben.
    constexpr std::string_view kFormat4Vollmenge =
        "9f8802514f7a5e5ee4a4229844d83eaabf0b8860652691a2238819e346bcd48b223b297f057b7ea747cf6cac993cd7ad"
        "a926af71de1fd9ea188e09176f5c4fe8";
    EXPECT_EQ((ceb::kCebFingerprintFor<ceb::CebComboLegend{"[all]"}>), kFormat4Vollmenge)
        << "Der Vollmengen-Schluessel ist gewandert -- damit sind ALLE bestehenden Bestandslog-Eintraege "
           "entwertet. Das ist erlaubt, aber nur als deklariertes Byte-Ereignis mit Owner-Entscheid, nie "
           "als Nebenprodukt.";
    // "[all]" und die ausgeschriebene Vollmenge sind derselbe Schluessel (Section 64-D1-B).
    EXPECT_EQ((ceb::kCebFingerprintFor<ceb::CebComboLegend{"[all]"}>),
              (ceb::kCebFingerprintFor<ceb::CebComboLegend{"[wallclock,macro,micro]"}>));
    // Die leere Legende ist die dritte Schreibweise derselben Vollmenge (innen-leer -> [all]).
    EXPECT_EQ((ceb::kCebFingerprintFor<ceb::CebComboLegend{"[all]"}>),
              (ceb::kCebFingerprintFor<ceb::CebComboLegend{"[]"}>));
}

// -- DIE EINKOMPILIERTE KONSTANTE IST DIE SPEZIALISIERUNG, NICHT EIN ZWEITER WEG ----------------------
//
// Der Log-Kopf (apps/cache_engine_builder/main.cpp), --version und die Bestandslog-Zelle
// (profile_run_facade.cpp:929) lesen ALLE kCebFingerprint. Dieser Test haelt fest, dass diese Konstante
// nichts anderes ist als kCebFingerprintFor<einkompilierte Legende> -- es gibt keinen zweiten
// Ableitungsweg, an dem Log-Kopf und Bestandslog-Zelle auseinanderlaufen koennten.
TEST(D4CebSchluesselWahl, EinkompilierteKonstanteIstSpezialisierungDerVorlage) {
    EXPECT_EQ(ceb::kCebFingerprint, (ceb::kCebFingerprintFor<ceb::kCebCtLegend>));
    EXPECT_EQ(ceb::kCebMeasurementStamp, (ceb::kCebMeasurementStampFor<ceb::kCebCtLegend>));
    // Und die Runtime-Ausgabe traegt genau diese beiden Teile -- keine dritte Ableitung.
    std::string const stamp = ceb::ceb_version_stamp();
    EXPECT_EQ(stamp, "ceb-measurement=" + std::string{ceb::kCebMeasurementStamp} +
                         ";sha512=" + std::string{ceb::kCebFingerprint});
    std::cout << "[D-4 BISS] einkompilierte Legende=" << ceb::kCebCtComboLegend
              << ", Schluessel=" << ceb::kCebFingerprint.substr(0, 24) << "...\n";
}
