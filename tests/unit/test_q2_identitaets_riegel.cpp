// Q2/V-06 + KON101-02 (18.08.2026) -- DER IDENTITAETS-RIEGEL, am GELADENEN Modul.
//
// WAS HIER GEPRUEFT WIRD UND WARUM NICHT MIT `nm`: dass ein Symbol im Export-Verzeichnis STEHT, sagt
// nichts darueber, ob es das Richtige antwortet. Geprueft wird deshalb ueber den echten Ladeweg
// (AnatomyModuleLoader::load, dlopen), und die Aussage ist staerker als eine Namensliste: der Loader
// zieht comdare_anatomy_gattung und comdare_anatomy_genus als PFLICHT, verriegelt ihre Werte gegen die
// Instanz, die die Factory desselben Moduls liefert, und erreicht status_ok nur, wenn beides passt.
//
// DIE DREI BEHAUPTUNGEN, DIE DIESER TEST TRENNT:
//   (1) Das Symbol ist da und antwortet          -> load == status_ok (sonst nennt status_name, WAS fehlt)
//   (2) Symbol-Wert == genus() der Instanz       -> die statische und die dynamische Antwort sind EINE
//   (3) gattung_of(genus) == Gattungs-Symbol     -> die Ebene-1-Zuordnung ist abgeleitet, nicht behauptet
// (2) und (3) verriegelt der Loader bereits selbst (status_identity_mismatch); dieser Test belegt, dass
// die Verriegelung fuer JEDE Fixture-Klasse HAELT -- also dass sie kein Sonderweg fuer eine Gattung ist.
//
// DIE NEGATIV-SEITE gehoert dazu, sonst ist die Positiv-Seite wertlos: die beiden Alt-Major-Fixtures
// scheitern weiterhin VOR der Symbol-Stufe (Magic bzw. Major). Das ist der Beleg, dass die neue
// Pflicht die BESTEHENDEN Fehlerbilder nicht verdraengt hat -- ein Alt-Modul muss "magic_mismatch"
// bzw. "abi_major_mismatch" melden, nicht "gattung_symbol_missing", sonst waere die Diagnose des
// haeufigsten Realfalls (jemand baut gegen eine alte ABI) durch den Umbau schlechter geworden.
//
// @related [[gattung-genus-sind-interface-hierarchie]] [[genus-impl-abstract-factory-ein-tier-binary]]

#include <anatomy/anatomy_base.hpp>
#include <builder/anatomy_module_loader/anatomy_module_loader.hpp>

#include <gtest/gtest.h>

#include <string>

namespace cea    = comdare::cache_engine::anatomy;
namespace loader = comdare::cache_engine::builder::anatomy_loader;

namespace {

struct Fall {
    char const*       pfad;
    cea::AnatomyGenus erwartetes_genus;
    char const*       etikett;
};

// Die Fixture-Klassen, jede einmal: plain-Tier ueber die vier Gattungs-Makros und Hybrid ueber
// COMDARE_DEFINE_HYBRID_MODULE. Damit sind BEIDE Bauwege gedeckt, ueber die im Haus eine ladbare
// Anatomy-.so entsteht -- und mit den vier Gattungs-Makros zugleich beide Gattungen (Map ueber den
// Hybrid-Search-Fall, Container ueber set/sequence/view/adapter).
//
// WARUM DIE BUILDVARIANT-PAARE HIER NICHT STEHEN (am Objekt gepruft, 18.08.2026, nicht aus der Karte
// uebernommen): perm_buildvariant_avx512/-avx2 sind KEINE Anatomy-Module. Ihre Quellen
// (genus_buildvariant_avx*.cpp) rufen COMDARE_DEFINE_BUILD_VARIANT_INSPECTION und exportieren GENAU EIN
// Symbol -- comdare_build_variant_inspect. `nm -D` auf die gebaute .so zeigt keines der Anatomy-
// ABI-Symbole, und der Loader lehnt sie folgerichtig mit symbol_not_found ab; das galt vor diesem Bruch
// genauso. Sie gehoeren zum Build-Varianten-Pull-Pfad (L-74a), nicht zur Ladestrecke. Sie hier
// aufzunehmen haette den Test rot gemacht und dabei etwas geprueft, was diese .so nie behauptet hat.
// Der Bauweg COMDARE_DEFINE_ANATOMY_MODULE_ADHOC_BUILDVARIANT ist trotzdem gedeckt: er delegiert an
// COMDARE_DEFINE_ANATOMY_MODULE_ADHOC und dieses an COMDARE_DEFINE_ANATOMY_MODULE -- die zwei neuen
// Symbole erbt er also, ohne eigene Makro-Stelle zu sein.
Fall const kFaelle[] = {
    {COMDARE_Q2_MODUL_SET, cea::AnatomyGenus::Set, "plain set"},
    {COMDARE_Q2_MODUL_SEQUENCE, cea::AnatomyGenus::Sequence, "plain sequence"},
    {COMDARE_Q2_MODUL_VIEW, cea::AnatomyGenus::View, "plain view"},
    {COMDARE_Q2_MODUL_ADAPTER, cea::AnatomyGenus::Adapter, "plain adapter"},
    {COMDARE_Q2_MODUL_HYBRID_SEARCH, cea::AnatomyGenus::SearchAlgorithm, "hybrid -> searchalgorithm"},
    {COMDARE_Q2_MODUL_HYBRID_SET, cea::AnatomyGenus::Set, "hybrid -> set"},
};

} // namespace

TEST(Q2IdentitaetsRiegel, SymbolWertGleichGenusUndGattungAbgeleitet) {
    for (auto const& f : kFaelle) {
        loader::AnatomyModuleHandle handle;
        int const                   st = loader::AnatomyModuleLoader::load(f.pfad, handle);

        // (1) Die zwei neuen Pflicht-Symbole sind da UND der Riegel im Loader hat gehalten. Faellt der
        //     Riegel, ist status_name die Diagnose -- identity_mismatch heisst: das Modul weist eine
        //     andere Identitaet aus, als seine eigene Factory liefert.
        ASSERT_EQ(st, loader::status_ok)
            << f.etikett << ": load == " << loader::status_name(st) << " (Pfad " << f.pfad << ")";

        auto* anatomie = handle.anatomy();
        ASSERT_NE(anatomie, nullptr) << f.etikett << ": Factory lieferte nullptr";

        // (2) Der Genus des GELADENEN Objekts ist der deklarierte. Beim Hybrid ist das der ZIEL-Genus
        //     (Weg C) -- nie der Reroute-Wert.
        EXPECT_EQ(anatomie->genus(), f.erwartetes_genus)
            << f.etikett << ": das geladene Modul meldet ein anderes Genus als deklariert";
        EXPECT_NE(anatomie->genus(), cea::AnatomyGenus::FunctionInterfaceReroute)
            << f.etikett << ": genus() liefert den Reroute-Wert -- Weg C gebrochen";

        // (3) Die Gattung ist ABGELEITET, nicht getragen: gattung_of ist total und constexpr, also gibt
        //     es zu jedem Genus genau eine Gattung. Der Test rechnet sie hier NOCH EINMAL aus und
        //     vergleicht mit der Zuordnung, die das Modul ueber seinen Genus impliziert.
        auto const gattung = cea::gattung_of(anatomie->genus());
        EXPECT_EQ(gattung, cea::gattung_of(f.erwartetes_genus))
            << f.etikett << ": Ebene-1-Zuordnung weicht ab";

        std::cout << "  RIEGEL OK: " << f.etikett << "  genus=" << static_cast<int>(anatomie->genus())
                  << "  gattung=" << static_cast<int>(gattung) << "\n";
    }
}

TEST(Q2IdentitaetsRiegel, AltModuleScheiternWeiterVORDerSymbolStufe) {
    // DER NEGATIV-BELEG. Beide Fixtures TRAGEN die zwei neuen Symbole (mit Absicht -- sonst waere ihre
    // Ablehnung mehrdeutig). Sie muessen trotzdem an ihrem jeweiligen ALTEN Schloss scheitern, weil der
    // Loader die Identitaets-Symbole erst NACH Magic und Version zieht.
    struct AltFall {
        char const* pfad;
        int         erwarteter_status;
        char const* etikett;
    };
    AltFall const alt[] = {
        {COMDARE_Q2_MODUL_ALT_MAJOR7, loader::status_magic_mismatch, "alt_major7 (Schloss 1: Magic)"},
        {COMDARE_Q2_MODUL_MAJOR7_NEUE_MAGIC, loader::status_abi_major_mismatch,
         "major7_neue_magic (Schloss 2: Major)"},
    };

    for (auto const& a : alt) {
        loader::AnatomyModuleHandle handle;
        int const                   st = loader::AnatomyModuleLoader::load(a.pfad, handle);
        EXPECT_EQ(st, a.erwarteter_status)
            << a.etikett << ": erwartet " << loader::status_name(a.erwarteter_status) << ", bekommen "
            << loader::status_name(st)
            << " -- die neue Symbol-Pflicht darf das bestehende Fehlerbild NICHT verdraengen";
        std::cout << "  NEGATIV OK: " << a.etikett << " -> " << loader::status_name(st) << "\n";
    }
}

TEST(Q2IdentitaetsRiegel, StatusNamenSindVollstaendig) {
    // Ein neuer Status-Code ohne status_name-Zeile ist ein Code, den niemand lesen kann -- der Fehler
    // waere dann "unknown", und die ganze Diagnose-Absicht der drei neuen Codes waere dahin.
    EXPECT_STREQ(loader::status_name(loader::status_gattung_symbol_missing), "gattung_symbol_missing");
    EXPECT_STREQ(loader::status_name(loader::status_genus_symbol_missing), "genus_symbol_missing");
    EXPECT_STREQ(loader::status_name(loader::status_identity_mismatch), "identity_mismatch");
    // Und die Gegenprobe: ein Code, den es NICHT gibt, muss weiterhin "unknown" liefern. Ohne sie
    // wuerde ein default-Zweig, der versehentlich einen Namen zurueckgibt, unbemerkt bleiben.
    EXPECT_STREQ(loader::status_name(99), "unknown");
}
