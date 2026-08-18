#pragma once
// HY-A2 (2026-08-18) -- Hybrid Module ABI v1 (Factory-Makro) -- MODUL-AUTOR-Seite der Gattung
// HeuristikAdapter / Genus FunctionInterfaceReroute.
//
// Analog COMDARE_DEFINE_ANATOMY_MODULE (abi/anatomy_module_abi_v1.hpp) und
// COMDARE_DEFINE_ADAPTER_MODULE (abi/adapter_module_abi_v1.hpp): eine Hybrid-Binary materialisiert
// damit ihre .so-Export-Symbole. Die VIER extern-"C"-Pflicht-Symbole sind BUCHSTABENGLEICH zu
// denen der plain Tiere -- comdare_anatomy_abi_version / comdare_anatomy_abi_magic /
// comdare_create_anatomy / comdare_destroy_anatomy -- und genau das ist der Zweck:
//
//   DERSELBE gattungs-agnostische AnatomyModuleLoader laedt eine Hybrid-.so ohne jede Aenderung.
//   Haette die Hybrid-Gattung eigene Symbolnamen, muesste der Loader die Gattung KENNEN, bevor er
//   laedt -- er kann sie aber erst NACH dem Laden erfragen (anatomy()->genus()). Das waere ein
//   Henne-Ei-Problem und haette eine zweite Ladestrecke erzwungen. Die Gleichheit der vier Namen
//   ist deshalb keine Bequemlichkeit, sondern die Bedingung dafuer, dass es EINE Ladestrecke gibt
//   (Doc 24 Abschnitt 8.8, dieselbe Begruendung wie bei der Container-Gattung).
//
// NACHTRAG Q2/V-06 + KON101-02 (18.08.2026) -- AUS VIER SYMBOLEN SIND SECHS GEWORDEN, UND DAS HAT DIE
// OBIGE BEGRUENDUNG NICHT AUFGEWEICHT, SONDERN VOLLENDET. Neu sind comdare_anatomy_gattung und
// comdare_anatomy_genus, ebenfalls buchstabengleich ueber alle Gattungen. Das oben beschriebene
// Henne-Ei-Problem ("die Gattung ist erst NACH dem Laden erfragbar") war bis hierher nur DADURCH
// entschaerft, dass der Loader sie gar nicht wissen musste; jetzt ist es aufgeloest: er kann sie
// erfragen, BEVOR er eine Instanz baut -- ueber gattungs-agnostische NAMEN mit gattungs-spezifischen
// WERTEN. Fuer den Hybrid gilt dabei unveraendert Weg C: comdare_anatomy_genus meldet den ZIEL-Genus
// (das, was das Modul FUER den Aufrufer ist), niemals FunctionInterfaceReroute (das, was es INTERN
// tut). Der Loader verriegelt beides gegeneinander (status_identity_mismatch).
//
// ------------------------------------------------------------------------------------------------
// WARUM DIESER HEADER IN hybrid/ LIEGT UND NICHT IN abi/, WO DIE VIER ANDEREN MODUL-MAKROS STEHEN
// ------------------------------------------------------------------------------------------------
// set_/view_/sequence_/adapter_module_abi_v1.hpp liegen in abi/, weil ihre Genera ABI-SICHTBAR sind:
// sie sind Werte, die genus() zurueckgibt, und gehoeren damit in die gattungs-agnostische
// Vertragsschicht. FunctionInterfaceReroute ist genau das NICHT (anatomy_base.hpp:139-168, Owner-
// Entscheid E-1 final): er ist ein KLASSIFIKATIONS-Wert, den genus() nie liefert. Ihn in die
// ABI-Vertragsschicht zu legen, haette ihn optisch in die Reihe der ABI-sichtbaren Genera gestellt
// -- also genau die Verwechslung nahegelegt, gegen die Weg A/Weg C getrennt wurden.
//
// DIE INCLUDE-RICHTUNG BESTAETIGT DEN SCHNITT: hybrid/ -> abi/ ist die Richtung "obere Gattung kennt
// den Vertrag". Die Gegenrichtung abi/ -> hybrid/ waere "der Vertrag kennt eine einzelne Gattung"
// und existiert im Baum an keiner Stelle (gemessen 18.08.2026: 0 Treffer fuer '#include.*hybrid/'
// unter include/cache_engine/abi/). Dieser Header haelt das so.
//
// @doku docs/architecture/24 Abschnitt 8.8 (Pruef-Dock je Gattung) + anatomy_base.hpp:139-168 (Weg A/Weg C)

#include "hybrid_binary_proxy.hpp" // HybridBinaryProxy + RerouteZiel-Deklarationen

#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp> // ABI-Version/Magic/Export-Macro

#include <new>

// ------------------------------------------------------------------------------------------------
// COMDARE_DEFINE_HYBRID_MODULE(ZielGenusExpr) -- die SECHS Pflicht-extern-C-Symbole einer
// Hybrid-.so (vier Ur-Pflicht + comdare_anatomy_gattung/genus, NACHTRAG Q2/V-06 oben).
//
// EIN Argument, kein variadisches: eine Hybrid-Binary wird durch ihr REROUTE-ZIEL bestimmt, nicht
// durch eine Achsen-Liste. Sie hat keine eigene Komposition (deshalb auch keine 12 oder 18 Achsen
// im Makro) -- sie hat Docks, und deren Zahl ist der Programm-Deckel.
//
// GENAU EIN STANDARD-DOCK: der F8-Minimal-Schnitt legt MaxDocks = 1 fest und legt das Dock direkt
// in der Factory an. Wuerde die Factory es NICHT anlegen, kaeme eine Hybrid-.so ohne jede
// Aufnahmestelle aus dem Loader -- ein Objekt, das sich wie ein Tier anfuehlt und nichts
// weiterreichen kann. Schlaegt das attach fehl, wird das halb gebaute Objekt WIEDER FREIGEGEBEN und
// nullptr gemeldet: der Loader behandelt nullptr bereits als Ladefehler, waehrend ein Proxy ohne
// Dock erst beim ersten Reroute-Versuch aufgefallen waere -- also weit weg von der Ursache.
//
// DIE CT-SPERRE GREIFT HIER, NICHT ERST IM TEST: ZielGenusExpr geht als Template-Argument in
// HybridBinaryProxy, dessen static_assert ein nicht deklariertes Ziel als LESBAREN Compile-Fehler
// meldet (hybrid_binary_proxy.hpp: RerouteZiel<G> primaer undefiniert). Eine Hybrid-.so auf ein
// unzulaessiges Genus ist damit nicht baubar -- nicht "wird zur Laufzeit abgelehnt".
// ------------------------------------------------------------------------------------------------
#define COMDARE_DEFINE_HYBRID_MODULE(ZielGenusExpr)                                                                    \
    using ComdareHybridModulProxy = ::comdare::cache_engine::hybrid::HybridBinaryProxy<(ZielGenusExpr), 1>;            \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT std::uint64_t comdare_anatomy_abi_version() noexcept {                       \
        return (static_cast<std::uint64_t>(COMDARE_ANATOMY_ABI_MAJOR) << 32) |                                         \
               static_cast<std::uint64_t>(COMDARE_ANATOMY_ABI_MINOR);                                                  \
    }                                                                                                                  \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT std::uint64_t comdare_anatomy_abi_magic() noexcept {                         \
        return COMDARE_ANATOMY_ABI_MAGIC;                                                                              \
    }                                                                                                                  \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT ::comdare::cache_engine::anatomy::IAnatomyBase*                              \
    comdare_create_anatomy() noexcept {                                                                                \
        auto* proxy = new (::std::nothrow) ComdareHybridModulProxy{};                                                  \
        if (proxy == nullptr) return nullptr;                                                                          \
        if (proxy->dock_anlegen() < 0) {                                                                               \
            delete proxy;                                                                                              \
            return nullptr;                                                                                            \
        }                                                                                                              \
        return proxy;                                                                                                  \
    }                                                                                                                  \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT void comdare_destroy_anatomy(                                                \
        ::comdare::cache_engine::anatomy::IAnatomyBase* ptr) noexcept {                                                \
        delete ptr;                                                                                                    \
    }                                                                                                                  \
    /* Q2/V-06 (18.08.2026): die ZWEI IDENTITAETS-SYMBOLE. Sie beantworten "was BIST du" VOR                           \
       der Factory -- bisher ging das nur ueber create + genus(), also erst, nachdem ein Objekt                        \
       gebaut war. Die NAMEN sind gattungs-agnostisch (jedes Modul jeder Gattung traegt genau                          \
       diese zwei), die WERTE sind es nicht. Die Gattung wird NICHT getragen, sondern aus dem                          \
       Genus abgeleitet (gattung_of, total + constexpr) -- zwei unabhaengig gepflegte Quellen                          \
       koennten auseinanderlaufen, eine abgeleitete kann es nicht. */                                                  \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT std::uint8_t comdare_anatomy_gattung() noexcept {                            \
        return static_cast<std::uint8_t>(                                                                              \
            ::comdare::cache_engine::anatomy::gattung_of((ZielGenusExpr)));                                            \
    }                                                                                                                  \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT std::uint8_t comdare_anatomy_genus() noexcept {                              \
        return static_cast<std::uint8_t>((ZielGenusExpr));                                                             \
    }
