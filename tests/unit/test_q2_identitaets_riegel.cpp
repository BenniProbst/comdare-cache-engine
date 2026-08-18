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

#include <cstdint>
#include <iostream>
#include <string>

#if !defined(_WIN32)
#include <dlfcn.h> // Zaehler-Sonden des Luegner-Moduls: eigenes Handle haelt die .so resident
#endif

namespace cea    = comdare::cache_engine::anatomy;
namespace loader = comdare::cache_engine::builder::anatomy_loader;

namespace {

struct Fall {
    char const*       pfad;
    cea::AnatomyGenus erwartetes_genus;
    // Review #15 Fix 8: UNABHAENGIGE Erwartungs-Spalte statt Selbstvergleich. Der fruehere Check
    // verglich gattung_of(anatomie->genus()) mit gattung_of(f.erwartetes_genus) -- nach bestandenem
    // Genus-Check ist das gattung_of(x) == gattung_of(x), eine Tautologie, die JEDE Zuordnungstabelle
    // in gattung_of gruen liesse. Die Spalte bindet gattung_of an ein LITERAL je Fixture-Klasse.
    cea::AnatomyGattung erwartete_gattung;
    char const*         etikett;
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
    {COMDARE_Q2_MODUL_SET, cea::AnatomyGenus::Set, cea::AnatomyGattung::Container, "plain set"},
    {COMDARE_Q2_MODUL_SEQUENCE, cea::AnatomyGenus::Sequence, cea::AnatomyGattung::Container, "plain sequence"},
    {COMDARE_Q2_MODUL_VIEW, cea::AnatomyGenus::View, cea::AnatomyGattung::Container, "plain view"},
    {COMDARE_Q2_MODUL_ADAPTER, cea::AnatomyGenus::Adapter, cea::AnatomyGattung::Container, "plain adapter"},
    {COMDARE_Q2_MODUL_HYBRID_SEARCH, cea::AnatomyGenus::SearchAlgorithm, cea::AnatomyGattung::Map,
     "hybrid -> searchalgorithm"},
    {COMDARE_Q2_MODUL_HYBRID_SET, cea::AnatomyGenus::Set, cea::AnatomyGattung::Container, "hybrid -> set"},
};

} // namespace

TEST(Q2IdentitaetsRiegel, SymbolWertGleichGenusUndGattungAbgeleitet) {
    for (auto const& f : kFaelle) {
        loader::AnatomyModuleHandle handle;
        int const                   st = loader::AnatomyModuleLoader::load(f.pfad, handle);

        // (1) Die zwei neuen Pflicht-Symbole sind da UND der Riegel im Loader hat gehalten. Faellt der
        //     Riegel, ist status_name die Diagnose -- identity_mismatch heisst: das Modul weist eine
        //     andere Identitaet aus, als seine eigene Factory liefert.
        ASSERT_EQ(st, loader::status_ok) << f.etikett << ": load == " << loader::status_name(st) << " (Pfad " << f.pfad
                                         << ")";

        auto* anatomie = handle.anatomy();
        ASSERT_NE(anatomie, nullptr) << f.etikett << ": Factory lieferte nullptr";

        // (2) Der Genus des GELADENEN Objekts ist der deklarierte. Beim Hybrid ist das der ZIEL-Genus
        //     (Weg C) -- nie der Reroute-Wert.
        EXPECT_EQ(anatomie->genus(), f.erwartetes_genus)
            << f.etikett << ": das geladene Modul meldet ein anderes Genus als deklariert";
        EXPECT_NE(anatomie->genus(), cea::AnatomyGenus::FunctionInterfaceReroute)
            << f.etikett << ": genus() liefert den Reroute-Wert -- Weg C gebrochen";

        // (3) Die Gattung ist ABGELEITET, nicht getragen -- und die Erwartung ist ein LITERAL aus der
        //     Tabelle (Review #15 Fix 8): Map fuer den Hybrid-Search-Fall, Container fuer die vier
        //     plain-Klassen und hybrid-set. Der fruehere Vergleich gegen gattung_of(f.erwartetes_genus)
        //     war nach bestandenem (2) ein Selbstvergleich und haette JEDE gattung_of-Tabelle gedeckt.
        auto const gattung = cea::gattung_of(anatomie->genus());
        EXPECT_EQ(gattung, f.erwartete_gattung)
            << f.etikett
            << ": Ebene-1-Zuordnung weicht vom Tabellen-Literal ab (gattung_of=" << static_cast<int>(gattung)
            << ", erwartet " << static_cast<int>(f.erwartete_gattung) << ")";

        std::cout << "  RIEGEL OK: " << f.etikett << "  genus=" << static_cast<int>(anatomie->genus())
                  << "  gattung=" << static_cast<int>(gattung) << "\n";
    }
}

TEST(Q2IdentitaetsRiegel, AltModuleScheiternWeiterVORDerSymbolStufe) {
    // DER NEGATIV-BELEG. Die ersten beiden Fixtures TRAGEN die zwei neuen Symbole (mit Absicht -- sonst
    // waere ihre Ablehnung mehrdeutig). Sie muessen trotzdem an ihrem jeweiligen ALTEN Schloss
    // scheitern, weil der Loader die Identitaets-Symbole erst NACH Magic und Version zieht.
    // Die DRITTE Fixture (Review #15 Fix 3) traegt die Symbole ABSICHTLICH NICHT: erst an ihr ist die
    // Reihenfolge beobachtbar. Ein Loader, der die Identitaets-Symbole VOR Magic/Version zoege, wuerde
    // hier gattung_symbol_missing statt magic_mismatch melden -- also die Diagnose des haeufigsten
    // Realfalls (Modul gegen alte ABI gebaut, hat die Symbole schlicht nicht) verfaelschen.
    struct AltFall {
        char const* pfad;
        int         erwarteter_status;
        char const* etikett;
    };
    AltFall const alt[] = {
        {COMDARE_Q2_MODUL_ALT_MAJOR7, loader::status_magic_mismatch, "alt_major7 (Schloss 1: Magic)"},
        {COMDARE_Q2_MODUL_MAJOR7_NEUE_MAGIC, loader::status_abi_major_mismatch, "major7_neue_magic (Schloss 2: Major)"},
        {COMDARE_Q2_MODUL_ALT_MAGIC_OHNE_SYMBOLE, loader::status_magic_mismatch,
         "alt_magic_ohne_symbole (Schloss 1 schlaegt VOR der Symbol-Stufe zu -- Reihenfolge-Pin)"},
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

TEST(Q2IdentitaetsRiegel, RiegelUndSymbolPflichtBeissenAmEchtenLadeweg) {
    // Review #15 Fix 1 (KRITISCH, K13): die drei neuen Statuscodes werden hier ERZEUGT, nicht nur
    // benannt. Ohne diese Koeder bliebe ein invertierter oder toter Riegel bei ALLEN Tests gruen --
    // der namensgebende Mechanismus des Q2/V-06-Bruchs waere unbewiesen.
    //
    // (a)+(b) Die zwei Symbol-Schloesser, je EINZELN: den Fixtures fehlt GENAU EIN Symbol, alles
    // andere ist korrekt (aktuelle Magic/Version, echte Factory). Die exakte Status-Erwartung pinnt
    // zugleich die Zuordnung 9<->gattung / 10<->genus gegen Vertauschung.
    struct SymbolFall {
        char const* pfad;
        int         erwarteter_status;
        char const* etikett;
    };
    SymbolFall const symbol_faelle[] = {
        {COMDARE_Q2_MODUL_OHNE_GATTUNG, loader::status_gattung_symbol_missing, "ohne comdare_anatomy_gattung"},
        {COMDARE_Q2_MODUL_OHNE_GENUS, loader::status_genus_symbol_missing, "ohne comdare_anatomy_genus"},
    };
    for (auto const& s : symbol_faelle) {
        loader::AnatomyModuleHandle handle;
        int const                   st = loader::AnatomyModuleLoader::load(s.pfad, handle);
        EXPECT_EQ(st, s.erwarteter_status)
            << s.etikett << ": erwartet " << loader::status_name(s.erwarteter_status) << ", bekommen "
            << loader::status_name(st) << " -- das fehlende Pflicht-Symbol muss EINZELN benannt werden";
        EXPECT_FALSE(handle.valid()) << s.etikett << ": abgelehntes Modul darf keine gueltige Handle tragen";
        std::cout << "  NEGATIV OK: " << s.etikett << " -> " << loader::status_name(st) << "\n";
    }

    // (c) DER LUEGNER: Symbol behauptet Sequence, die Factory liefert eine Set-Instanz. Beide
    // Wertklassen-Gates passieren (bekanntes, ABI-sichtbares Genus; Gattung konsistent abgeleitet) --
    // NUR der Konsistenz-Riegel am geladenen Objekt kann diese Luege sehen.
#if !defined(_WIN32)
    // Zaehler-Sonden VOR dem Loader-Lauf verdrahten: das eigene dlopen haelt das Modul resident,
    // damit die Zaehler den Loader-Lauf UEBERLEBEN (der Loader schliesst sein Handle im Fehlerpfad).
    void* sonde = ::dlopen(COMDARE_Q2_MODUL_LUEGNER, RTLD_NOW | RTLD_LOCAL);
    ASSERT_NE(sonde, nullptr) << "Sonden-dlopen der Luegner-.so scheiterte: " << ::dlerror();
    using PfnZaehler    = std::uint64_t (*)();
    auto* pfn_create_n  = reinterpret_cast<PfnZaehler>(::dlsym(sonde, "comdare_q2_testonly_create_count"));
    auto* pfn_destroy_n = reinterpret_cast<PfnZaehler>(::dlsym(sonde, "comdare_q2_testonly_destroy_count"));
    ASSERT_NE(pfn_create_n, nullptr);
    ASSERT_NE(pfn_destroy_n, nullptr);
    ASSERT_EQ(pfn_create_n(), 0u) << "Vorbelasteter Zaehler -- die Sonde selbst darf nichts bauen";
#endif

    loader::AnatomyModuleHandle handle;
    int const                   st = loader::AnatomyModuleLoader::load(COMDARE_Q2_MODUL_LUEGNER, handle);
    EXPECT_EQ(st, loader::status_identity_mismatch)
        << "Luegner-Modul: erwartet identity_mismatch, bekommen " << loader::status_name(st)
        << " -- das Symbol behauptet Sequence, die Factory liefert Set; nur der Riegel trennt das";
    EXPECT_FALSE(handle.valid())
        << "Luegner-Modul: der Riegel hat abgelehnt, aber die Handle traegt trotzdem ein Modul";

#if !defined(_WIN32)
    // destroy-vor-dlclose am Fehlerpfad: die Factory lief GENAU EINMAL, und die gebaute Instanz
    // wurde ueber comdare_destroy_anatomy DES MODULS freigegeben (ErwerbsGuard A-F4). Ein Leck
    // (destroy==0) waere hier rot -- und unter ASan/LSan zusaetzlich als Leak sichtbar.
    EXPECT_EQ(pfn_create_n(), 1u) << "Die Luegner-Factory lief nicht genau einmal";
    EXPECT_EQ(pfn_destroy_n(), 1u)
        << "destroy-vor-dlclose verletzt: die im Fehlerpfad gebaute Instanz wurde nicht freigegeben";
    std::cout << "  LUEGNER OK: identity_mismatch, create=" << pfn_create_n() << " destroy=" << pfn_destroy_n()
              << " (destroy-vor-dlclose am Fehlerpfad belegt)\n";
    ::dlclose(sonde);
#else
    std::cout << "  LUEGNER OK: identity_mismatch (Zaehler-Sonde nur auf POSIX verdrahtet)\n";
#endif
}

TEST(Q2IdentitaetsRiegel, StatusNamenSindVollstaendig) {
    // Ein neuer Status-Code ohne status_name-Zeile ist ein Code, den niemand lesen kann -- der Fehler
    // waere dann "unknown", und die ganze Diagnose-Absicht der drei neuen Codes waere dahin.
    EXPECT_STREQ(loader::status_name(loader::status_gattung_symbol_missing), "gattung_symbol_missing");
    EXPECT_STREQ(loader::status_name(loader::status_genus_symbol_missing), "genus_symbol_missing");
    EXPECT_STREQ(loader::status_name(loader::status_identity_mismatch), "identity_mismatch");
    // Review #15 Fix 2: die WERTKLASSEN-Haelfte des Riegels traegt ihren eigenen Code. Ein Modul,
    // dessen genus-Symbol ein Nicht-Enum-Byte (z.B. 250) oder den Reroute-Wert (5, NIE aus genus())
    // liefert, wird VOR der Factory abgewiesen -- und der Ablehnungsgrund muss lesbar sein.
    EXPECT_STREQ(loader::status_name(loader::status_genus_not_abi_visible), "genus_not_abi_visible");
    // Und die Gegenprobe: ein Code, den es NICHT gibt, muss weiterhin "unknown" liefern. Ohne sie
    // wuerde ein default-Zweig, der versehentlich einen Namen zurueckgibt, unbemerkt bleiben.
    EXPECT_STREQ(loader::status_name(99), "unknown");
}
