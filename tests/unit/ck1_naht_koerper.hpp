#pragma once
// ck1_naht_koerper.hpp -- DER GEMEINSAME KOERPER der beiden Beleg-Uebersetzungseinheiten.
//
// == WOZU =========================================================================================
// Die Zero-Cost-Zusage der Naht ("bei deaktiviert eben nicht") wird AM OBJEKT belegt, nicht behauptet:
// derselbe Quelltext wird ZWEIMAL uebersetzt, einmal mit voller Mess-Konfiguration und einmal mit der
// leeren. Danach werden die Symboltabellen verglichen.
//
// DASS ES EIN GEMEINSAMER KOERPER IST, IST DER GANZE PUNKT. Zwei getrennt geschriebene Fassungen
// haetten den Vergleich wertlos gemacht: jeder Unterschied im Objekt liesse sich dann auch mit einem
// Unterschied im Quelltext erklaeren. So unterscheiden sich die beiden Uebersetzungseinheiten in
// GENAU EINER ZEILE -- der Wahl von MK. Alles andere ist Zeichen fuer Zeichen dasselbe.
//
// Der Aufruf ist ausdruecklich NICHT `static` und NICHT `inline`: er muss als Symbol im Objekt
// stehen, sonst prueft der Vergleich nichts.
//
// ASCII-only.

#include <mess/genus_kaskade.hpp>
#include <mess/konfiguration.hpp>
#include <mess/mess_naht.hpp>
#include <mess/pilot_achsen.hpp>
#include <mess/pilot_suche_impl.hpp>

#include <cstdint>

namespace ck1 {

namespace mk = ::comdare::cache_engine::mess;
namespace pl = ::comdare::cache_engine::mess::pilot;
namespace ms = ::comdare::cache_engine::builder::measure_storage;

/// Die Genus-Flaeche, die das Pruefdock sieht -- je Konfiguration eine Monomorphisierung.
template <class MK>
using PilotGenus = mk::GenusInterface<pl::SucheImplFabrik<pl::PilotKomposition>, MK>;

/// Faehrt den Piloten. Die Doppelform steckt HIER: bei aktiver Konfiguration wird ein Instrument
/// aufgebaut und ein Visitor uebergeben, bei leerer gibt es beides nicht -- und zwar nicht als
/// uebersprungener Zweig, sondern als nicht vorhandener Code.
template <class MK>
[[nodiscard]] std::uint64_t koerper_fahren(std::uint64_t saat) noexcept {
    PilotGenus<MK> genus{};
    pl::Last       last{saat, 3u};

    if constexpr (mk::aktiv<MK>) {
        ms::CheckpointMeasure<MK> instrument{};
        ms::MessMasse             masse{4096u, 64u, ms::VorabBeruehrung::Ja};
        auto const                aufbau = instrument.init(masse);
        if (!aufbau.steht()) return 0u;

        mk::MessVisitor<MK> visitor{instrument, 0u};
        auto const          erg = genus.fahren(last, visitor); // Visitor ist PFLICHTPARAMETER
        return erg.koeder_ergebnis;
    } else {
        auto const erg = genus.fahren(last); // der Visitor-Parameter EXISTIERT NICHT
        return erg.koeder_ergebnis;
    }
}

} // namespace ck1
