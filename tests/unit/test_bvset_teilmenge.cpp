// test_bvset_teilmenge -- Task #59: der RICHTUNGS-BEHAFTETE Vergleich zweier bvset-Mengen-Signaturen.
//
// WAS DIESE TU BEWEIST (und was ausdruecklich nicht):
//   (1) PARSER   -- die Grammatik wird VOLLSTAENDIG gelesen, nicht ueberflogen. Der Beweis ist die
//                   REASSEMBLIERUNG: aus den geparsten Teilen wird der Original-String Byte fuer Byte
//                   wieder zusammengesetzt. Ein Parser, der etwas verschluckt, faellt daran durch --
//                   "parst ohne Fehler" waere keine Zusicherung (T-2).
//   (2) RICHTUNG -- A TEILMENGE B ist true, B TEILMENGE A ist false. BEIDE Richtungen, an DEMSELBEN
//                   Paar, im selben Test. Genau diese Asymmetrie ist der Gegenstand von #59; ein Test,
//                   der nur die true-Richtung prueft, waere von einer Implementierung, die immer true
//                   liefert, nicht zu unterscheiden.
//   (3) PIN      -- die ECHTE Treiber-Signatur (kDriverBuildVariantSignature) ist parsebar und traegt je
//                   Achse mindestens ein Element. Damit haengt der Parser an dem String, den die Flotte
//                   wirklich fuehrt, nicht nur an synthetischen Beispielen.
//
// DER KOEDER IST ZUFAELLIG (V-2, K13). Das zusaetzliche Element X wird je Lauf frisch aus /dev/urandom
// erzeugt. Grund: eine Implementierung, die eine hartkodierte Element-Tabelle mitfuehrt oder auf
// Laengen-Heuristik ausweicht, wuerde einen festen Koeder ueberleben. Sie kann einen Namen nicht kennen,
// den es beim Schreiben des Tests noch nicht gab.
//
// GEGENEINGANG ZU JEDER ZUSICHERUNG (T-4): zu "Teilmenge -> true" steht die Einschraenkungs-Richtung
// (-> false), zu "gleiches Element -> gefunden" steht dasselbe Element mit anderen Feldwerten
// (-> false), zu "wohlgeformt -> parsebar" stehen sechs Verstuemmelungen (-> nullopt).

#include "builder/bvset_teilmenge.hpp"                  // der Prueflig
#include "builder/driver_build_variant_signature.hpp"   // (3): die ECHTE Signatur dieses Treibers

#include <cstddef>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

namespace ex = comdare::cache_engine::builder::experiment;

namespace {

// -- DER ZUFALLS-KOEDER ------------------------------------------------------------------------------
// 12 Hex-Zeichen aus /dev/urandom. Kein Fallback auf eine Konstante: waere die Quelle nicht lesbar,
// liefe der Test mit einem VORHERSAGBAREN Koeder weiter und behauptete dann eine Schaerfe, die er nicht
// mehr hat. Ein nicht erzeugbarer Koeder ist ein Testfehler, kein Sonderfall.
[[nodiscard]] std::string koeder_token() {
    std::ifstream q{"/dev/urandom", std::ios::binary};
    EXPECT_TRUE(q.good()) << "/dev/urandom nicht lesbar -- ohne Zufall ist der Koeder keiner (V-2)";
    unsigned char roh[6]{};
    q.read(reinterpret_cast<char*>(roh), sizeof(roh));
    EXPECT_EQ(q.gcount(), static_cast<std::streamsize>(sizeof(roh)));
    static constexpr char kHex[] = "0123456789abcdef";
    std::string           s      = "koeder_";
    for (unsigned char const b : roh) {
        s.push_back(kHex[b >> 4]);
        s.push_back(kHex[b & 0x0F]);
    }
    return s;
}

// -- SYNTHETISCHE SIGNATUREN IN DER ECHTEN GRAMMATIK --------------------------------------------------
// Abgelesen am Emitter (build_variant_set_signature.hpp::ct_put_variant_set_signature und die drei
// ct_put_*_element): Kopf `bvset=<setver>;bv=<podver>`, dann je Achse `<name>[<elem>...]`, Element
// `{<name>;<feld>=<zahl>;...}`. Die Feldnamen sind exakt die des Emitters -- ein Test in einer
// Fantasie-Grammatik wuerde einen Parser abnehmen, der die echte nie sieht.
constexpr char const* kKopf = "bvset=1;bv=3";

[[nodiscard]] std::string simd_elem(std::string const& name, unsigned bits, unsigned avx512) {
    return "{" + name + ";simd_width_bits=" + std::to_string(bits) + ";simd_avx512=" + std::to_string(avx512) +
           ";simd_avx10_version=0}";
}

// Baut eine vollstaendige Signatur mit den gegebenen simd-Elementen (page/hw bleiben konstant -- die
// Richtung wird an EINER Achse gezeigt, das genuegt und haelt den Fall lesbar).
[[nodiscard]] std::string sig_mit_simd(std::string const& simd_elemente) {
    return std::string{kKopf} + ";page_type[{pt_basis;page_kind=1}]" + ";simd_extension[" + simd_elemente + "]" +
           ";general_hardware[{hw_basis;hw_cache_line=64;hw_numa_capable=1}]";
}

} // namespace

// ===========================================================================
// (1) PARSER -- Reassemblierung statt "parst ohne Fehler"
// ===========================================================================
TEST(BvsetTeilmenge, ParserReassembliertDenOriginalStringByteGenau) {
    auto const  x   = koeder_token();
    std::string sig = sig_mit_simd(simd_elem("se_a", 256, 0) + simd_elem(x, 512, 1));

    auto const g = ex::bvset_parse(sig);
    ASSERT_TRUE(g.has_value()) << "die wohlgeformte Signatur wurde nicht geparst: " << sig;

    // T-3: die Grundgesamtheit kommt aus einer ANDEREN Quelle als dem Prueflig -- die Zahl der
    // top-level-Elemente wird unabhaengig durch Zaehlen der '{' bestimmt (die Grammatik kennt keine
    // Verschachtelung). ASSERT auf die Zahl VOR der Schleife, damit ein leerer Parse nicht still als
    // "alles geprueft" durchgeht.
    std::size_t elemente_fremd = 0;
    for (char const c : sig)
        if (c == '{') ++elemente_fremd;
    ASSERT_EQ(elemente_fremd, 4u) << "NENNER (fremd gezaehlt): erwartet 4 Elemente (1 page + 2 simd + 1 hw)";

    std::size_t elemente_parser = 0;
    for (auto const& a : g->achsen) elemente_parser += a.elemente.size();
    EXPECT_EQ(elemente_parser, elemente_fremd)
        << "der Parser sieht " << elemente_parser << " von " << elemente_fremd << " Elementen";

    // DER EIGENTLICHE BEWEIS: aus den Teilen wieder das Ganze.
    std::string wieder{g->kopf};
    for (auto const& a : g->achsen) {
        wieder += ";";
        wieder += a.name;
        wieder += "[";
        for (auto const& e : a.elemente) wieder += e;
        wieder += "]";
    }
    EXPECT_EQ(wieder, sig) << "Reassemblierung weicht ab -- der Parser hat etwas verschluckt oder erfunden";
}

TEST(BvsetTeilmenge, UnparsebaresIstFailClosed) {
    // T-4: zu jeder Zusicherung ein Eingang, bei dem sie NICHT gilt. Sechs Verstuemmelungen, jede an
    // einer ANDEREN Stelle der Grammatik -- ein Parser, der nur den Praefix prueft, faellt hier durch.
    struct Fall {
        char const* was;
        char const* text;
    };
    Fall const faelle[] = {
        {"leer", ""},
        {"falscher Kopf", "bvsetX=1;bv=3;page_type[];simd_extension[];general_hardware[]"},
        {"Achse fehlt", "bvset=1;bv=3;page_type[{a;page_kind=1}];simd_extension[]"},
        {"Achsen-Reihenfolge vertauscht",
         "bvset=1;bv=3;page_type[{a;page_kind=1}];general_hardware[];simd_extension[]"},
        {"unbalancierte Klammer", "bvset=1;bv=3;page_type[{a;page_kind=1];simd_extension[];general_hardware[]"},
        {"Rest hinter der letzten Achse",
         "bvset=1;bv=3;page_type[];simd_extension[];general_hardware[]MUELL"},
    };
    std::size_t const nenner = sizeof(faelle) / sizeof(faelle[0]);
    ASSERT_EQ(nenner, 6u) << "NENNER: 6 Verstuemmelungen erwartet";
    for (auto const& f : faelle) {
        EXPECT_FALSE(ex::bvset_parse(f.text).has_value()) << "unparsebar erwartet bei: " << f.was;
        // Und die Folge davon am eigentlichen Prueflig: unparsebar heisst NIE Teilmenge.
        EXPECT_FALSE(ex::bvset_ist_teilmenge(f.text, sig_mit_simd(simd_elem("se_a", 256, 0)))) << f.was;
        EXPECT_FALSE(ex::bvset_ist_teilmenge(sig_mit_simd(simd_elem("se_a", 256, 0)), f.text)) << f.was;
    }
}

// ===========================================================================
// (2) RICHTUNG -- der Kern von #59: beide Richtungen an DEMSELBEN Paar
// ===========================================================================
TEST(BvsetTeilmenge, ErweiterungIstTeilmengeEinschraenkungNicht) {
    auto const        x = koeder_token();
    std::string const A = sig_mit_simd(simd_elem("se_a", 256, 0));
    std::string const B = sig_mit_simd(simd_elem("se_a", 256, 0) + simd_elem(x, 512, 1)); // B = A + X

    ASSERT_NE(A, B) << "der Zufalls-Koeder hat die Signatur nicht veraendert -- dann beisst er nicht (K13)";

    // ERWEITERUNG: die alte Binary (A) bleibt unter dem heutigen Treiber (B) gueltig.
    EXPECT_TRUE(ex::bvset_ist_teilmenge(A, B)) << "Erweiterung muss als Teilmenge gelten (Owner 26.07.)";
    // EINSCHRAENKUNG, DERSELBE KOEDER, UMGEKEHRTE RICHTUNG: die alte Binary traegt X, der Treiber nicht mehr.
    EXPECT_FALSE(ex::bvset_ist_teilmenge(B, A)) << "Einschraenkung darf NIE als Teilmenge gelten (Owner 10.08.)";
    // GEGENKOEDER: Gleichheit ist der Sonderfall, der in BEIDE Richtungen gilt.
    EXPECT_TRUE(ex::bvset_ist_teilmenge(A, A));
    EXPECT_TRUE(ex::bvset_ist_teilmenge(B, B));
}

TEST(BvsetTeilmenge, GleicherNameAndereFelderIstKeineTeilmenge) {
    // T-4 zur Zusicherung "Element gefunden": der Vergleich ist BYTE-GENAU, nicht ueber den Namen. Zwei
    // Varianten mit gleichem Namen aber anderer Breite sind VERSCHIEDENE Faehigkeiten -- ein
    // Namens-Vergleich wuerde die Feld-Aenderung still durchwinken.
    auto const        x  = koeder_token();
    std::string const A  = sig_mit_simd(simd_elem(x, 256, 0));
    std::string const A2 = sig_mit_simd(simd_elem(x, 512, 1)); // gleicher Name, andere Felder
    EXPECT_FALSE(ex::bvset_ist_teilmenge(A, A2));
    EXPECT_FALSE(ex::bvset_ist_teilmenge(A2, A));
}

TEST(BvsetTeilmenge, KopfUngleichheitSchlaegtJedeTeilmengeAus) {
    // Der Kopf traegt Format- UND POD-Version. Ueber einen Bump hinweg bedeuten gleiche Element-Bytes
    // nicht mehr dasselbe -- ein Element-Vergleich waere dort eine Behauptung ohne Grundlage.
    std::string const A     = sig_mit_simd(simd_elem("se_a", 256, 0));
    std::string       A_bv4 = A;
    auto const        pos   = A_bv4.find("bv=3");
    ASSERT_NE(pos, std::string::npos) << "NENNER: der Kopf traegt kein bv=3 -- Testvoraussetzung gebrochen";
    A_bv4.replace(pos, 4, "bv=4");
    EXPECT_NE(A, A_bv4);
    EXPECT_FALSE(ex::bvset_ist_teilmenge(A, A_bv4)) << "POD-Versions-Bump darf nicht als Teilmenge durchgehen";
    EXPECT_FALSE(ex::bvset_ist_teilmenge(A_bv4, A));
}

// ===========================================================================
// (3) PIN auf die ECHTE Treiber-Signatur
// ===========================================================================
TEST(BvsetTeilmenge, EchteTreiberSignaturIstParsebarUndTraegtJeAchseElemente) {
    std::string const echt{ex::kDriverBuildVariantSignature};
    ASSERT_FALSE(echt.empty()) << "NENNER 0 waere hier Abbruch: die Treiber-Signatur ist leer";

    auto const g = ex::bvset_parse(echt);
    ASSERT_TRUE(g.has_value()) << "die REALE Signatur dieses Treibers ist nicht parsebar: " << echt;

    // Die Elementzahlen je Achse NENNEN (V-1) -- sie sind die Grundgesamtheit dieses Pins und schwanken
    // mit der CMake-Enable-Menge dieser Maschine. Eine feste Erwartung waere hier falsch; die Zusicherung
    // ist "mindestens eins je Achse", und die Zahl gehoert trotzdem in die Ausgabe.
    std::size_t gesamt = 0;
    for (std::size_t a = 0; a < ex::kBvsetAchsenNamen.size(); ++a) {
        EXPECT_EQ(g->achsen[a].name, ex::kBvsetAchsenNamen[a]);
        EXPECT_GE(g->achsen[a].elemente.size(), 1u)
            << "Achse " << ex::kBvsetAchsenNamen[a] << " ist leer -- das waere `name[]`";
        std::cerr << "[NENNER] Achse " << ex::kBvsetAchsenNamen[a] << ": " << g->achsen[a].elemente.size()
                  << " enabled Element(e)\n";
        gesamt += g->achsen[a].elemente.size();
    }
    std::cerr << "[NENNER] Treiber-Signatur gesamt: " << gesamt << " Element(e), " << echt.size() << " Byte\n";
    ASSERT_GE(gesamt, 3u) << "drei Achsen, je mindestens ein Element";

    // Reassemblierung auch hier -- der Pin prueft nicht bloss, DASS geparst wurde, sondern dass nichts
    // verloren ging.
    std::string wieder{g->kopf};
    for (auto const& a : g->achsen) {
        wieder += ";";
        wieder += a.name;
        wieder += "[";
        for (auto const& e : a.elemente) wieder += e;
        wieder += "]";
    }
    EXPECT_EQ(wieder, echt);

    // Und die Selbst-Teilmenge der echten Signatur: ein Lauf gegen sich selbst muss immer gelten, sonst
    // wuerde der Skip-Pfad in der Flotte NIE greifen.
    EXPECT_TRUE(ex::bvset_ist_teilmenge(echt, echt));
}

TEST(BvsetTeilmenge, EchteSignaturPlusKoederIstEinseitig) {
    // Der Erweiterungs-Fall am REALEN String: ein zufaelliges Element in die simd-Achse der echten
    // Signatur einhaengen. Das ist der Fall, den die Flotte trifft, wenn eine SIMD-Variante hinzukommt.
    std::string const echt{ex::kDriverBuildVariantSignature};
    auto const        x = koeder_token();
    auto const        marke = std::string{";simd_extension["};
    auto const        pos   = echt.find(marke);
    ASSERT_NE(pos, std::string::npos) << "NENNER: die echte Signatur traegt keine simd_extension-Achse";
    std::string const erweitert =
        echt.substr(0, pos + marke.size()) + simd_elem(x, 512, 1) + echt.substr(pos + marke.size());
    ASSERT_NE(echt, erweitert);

    EXPECT_TRUE(ex::bvset_ist_teilmenge(echt, erweitert)) << "die heutige Flotte bleibt unter Erweiterung gueltig";
    EXPECT_FALSE(ex::bvset_ist_teilmenge(erweitert, echt)) << "eine Binary mit Zusatz-Variante ist NICHT gueltig";
}
