#pragma once
// mess/pilot_suche_impl.hpp -- (3) GENUS_impl als ABSTRACT FACTORY und (4) DER HAUPTALGORITHMUS.
//
// == WAS DIESE DATEI IM SCHICHTENBILD IST ==========================================================
// Sie ist die Schicht UNTER dem Genus-Interface und UEBER den Achsen:
//     GENUS_impl        <- SucheImplFabrik: ABSTRACT FACTORY, eigene Datei (Owner-Schichtung)
//     je Funktion EINE  <- SucheHauptalgo: EIN Hauptalgorithmus
//     Klasse
//     Achsen-Interfaces <- werden ORCHESTRIERT, nicht veraendert
//
// == DER HAUPTALGORITHMUS RECHNET NICHT ============================================================
// Owner 09.08.2026: "Der Hauptalgorithmus RECHNET NICHT -- er ORCHESTRIERT Achsen; deshalb sind die
// Achsen ueberhaupt messbar, und deshalb sitzt die Mess-Naht am Genus-Interface und nicht tiefer."
//
// Genau so ist `kern()` unten gebaut: er ruft der Reihe nach die Achsen der Komposition und klammert
// JEDEN Aufruf mit einem Mikro-Paar; den ganzen Durchlauf klammert er mit einem Makro-Paar. Er
// enthaelt selbst keine Rechenschleife. Was gerechnet wird, steht in den Achsen -- und deshalb misst
// das Mikro-Paar die ACHSE und nicht den Algorithmus, der sie ruft.
//
// == EIN KOERPER, ZWEI AUSPRAEGUNGEN ===============================================================
// `kern()` ist EINMAL geschrieben und nimmt die Naht als Template-Parameter. Bei aktiver Messung ist
// das NahtAn<MK>, bei deaktivierter StillNaht -- ein leerer Typ, dessen Methoden constexpr-leer sind.
//
// Die Alternative (zwei Koerper, einer mit und einer ohne Messstellen) ist die Abschrift-Falle: zwei
// Fassungen, die dasselbe tun sollen, driften auseinander, und die Abweichung sitzt ausgerechnet im
// UNGEMESSENEN Zweig -- also dort, wo sie per Konstruktion niemandem auffaellt.
//
// == SELBSTCHECK ===================================================================================
//   ZUSICHERT: dass zwischen zwei Achsen-Aufrufen geklammert wird und der ACHSEN-Rueckgabewert als
//              Messwert reist (nicht eine Zeit, die der Test nicht nachrechnen koennte).
//   ZUSICHERT NICHT: dass die Klammer den Overhead der Klammer selbst aussen vor laesst -- das ist
//              die "mit Fuehler/ohne Fuehler"-Differenz und Gegenstand der Ordnungs-Frage D-3.
//
// ASCII-only.

#include <mess/genus_kaskade.hpp>
#include <mess/konfiguration.hpp>
#include <mess/mess_naht.hpp>

#include <cstddef>
#include <concepts>
#include <cstdint>

namespace comdare::cache_engine::mess::pilot {

namespace ms = ::comdare::cache_engine::builder::measure_storage;

// ==================================================================================================
// DIE STATIONEN -- der statische Deskriptor je Messort (SOLL-Design OP-1 = JA)
// ==================================================================================================
//
// EIN Deskriptor je (Gattung, Genus, Funktion, Station). Der Index ist constexpr; der Name wird NUR
// nach dem Fenster aufgeloest (R1: kein Text im heissen Pfad).
//
// Die Indizes sind hier von Hand vergeben, weil der Pilot drei Stationen hat. Im Vollausbau faltet
// der Planer sie aus der Komposition -- die Faltung ist unten (stationen_je_op) bereits angelegt, sie
// bekommt dort nur mehr Eingang.

struct StationDurchlauf {
    static constexpr std::uint32_t deskriptor_ix = 0;
    static constexpr char const*   name() noexcept { return "suche/durchlauf"; }
};

struct StationAchseVorbereitung {
    static constexpr std::uint32_t deskriptor_ix = 1;
    static constexpr char const*   name() noexcept { return "suche/achse/vorbereitung"; }
};

/// DIE KOEDER-STATION. Ihr Messwert ist der Rueckgabewert der Koeder-Achse -- eine Zahl, die die
/// Abnahme UNABHAENGIG nachrechnen kann. Deshalb reist hier ausdruecklich KEINE Zeit: eine Zeit
/// laesst sich nicht nachrechnen, und ein Test, der seinen Sollwert nicht kennt, kann nur die
/// ANWESENHEIT eines Wertes pruefen. Anwesenheit ist keine Aussage (T-2).
struct StationAchseKoeder {
    static constexpr std::uint32_t deskriptor_ix = 2;
    static constexpr char const*   name() noexcept { return "suche/achse/koeder"; }
};

// ==================================================================================================
// (3) GENUS_impl -- DIE ABSTRACT FACTORY
// ==================================================================================================
//
// Sie ist die Stelle, an der der durchgereichte Typparameter zum monomorphisierten Objekt wird --
// "HINTER die Gattung+Interface Kaskade GEBAUT", woertlich der Owner-Auftrag.
//
// SELBSTCHECK: die Fabrik entscheidet, WELCHE Klasse zu welcher Funktion gehoert -- und sonst nichts.
// Sie misst nicht, sie rechnet nicht, sie kennt keine Achse. Wer hier Logik einbaut, hat aus der
// Fabrik einen zweiten Hauptalgorithmus gemacht.
template <class Komposition>
struct SucheImplFabrik;

// ==================================================================================================
// (4) DER HAUPTALGORITHMUS -- je Funktion EINE Klasse
// ==================================================================================================

/// Was ein Lauf berichtet. Die Zahlen sind ECHTE Ergebnisse der Achsen, keine Mess-Artefakte -- die
/// Abnahme vergleicht sie gegen unabhaengig nachgerechnete Sollwerte.
struct LaufErgebnis {
    std::uint64_t vorbereitung_ergebnis = 0;
    std::uint64_t koeder_ergebnis       = 0;
    std::uint64_t achsen_gerufen        = 0; // der NENNER: wie viele Achsen orchestriert wurden
};

/// Die Last eines Laufs.
struct Last {
    std::uint64_t saat   = 0; // der gewuerfelte Koeder R
    std::uint64_t runden = 1;
};

template <KonfigConcept MK, class Komposition>
class SucheHauptalgo {
public:
    // ----------------------------------------------------------------------------------------------
    // WARUM HIER `template <class MK2 = MK>` STEHT UND NICHT NUR `requires aktiv<MK>`
    // ----------------------------------------------------------------------------------------------
    // BEFUND AM OBJEKT (09.08.2026, beim Uebersetzen dieser Datei gefunden, nicht vermutet): eine
    // gewoehnliche Mitgliedsfunktion mit `MessVisitor<MK>&` im Parameter und `requires aktiv<MK>`
    // dahinter UEBERSETZT DEN DEAKTIVIERTEN BAU NICHT. Der Grund ist die Reihenfolge: bei der
    // Instanziierung der KLASSE werden alle Mitglieds-DEKLARATIONEN instanziiert, und dabei entsteht
    // `MessVisitor<Leer>` -- ein Typ, der seine eigene Bedingung verletzt. Die requires-Klausel
    // kommt zu spaet; sie waehlt zwischen Ueberladungen, aber der Parametertyp ist da schon gebildet.
    //
    // Gemessen: g++ 15.3 meldet "template constraint failure for ... class MessVisitor" an der
    // DEKLARATION, ohne dass die Funktion je gerufen wird.
    //
    // Der Ausweg ist, den Parametertyp VOM FUNKTIONS-Template abhaengig zu machen: `MessVisitor<MK2>`
    // wird erst gebildet, wenn die Funktion wirklich gebraucht wird. `std::same_as<MK2, MK>` nagelt
    // MK2 auf MK fest, damit der Vorgabewert nicht umgangen werden kann -- sonst waere aus dem
    // Pflichtparameter eine Einladung geworden, eine fremde Konfiguration hereinzureichen.
    //
    // DAS IST DER UNTERSCHIED ZWISCHEN "der Zweig wird nicht genommen" UND "der Zweig existiert
    // nicht". Ohne diesen Griff gaebe es die stille Form gar nicht -- und die Owner-Zusage
    // "oder bei deaktiviert eben nicht" waere unerfuellbar.

    /// MESSENDE Form: Visitor ist Pflichtparameter (kein Default, kein Zeiger, kein optional).
    template <class MK2 = MK>
        requires(std::same_as<MK2, MK> && aktiv<MK2>)
    [[nodiscard]] LaufErgebnis fahren_impl(Last const& last, MessVisitor<MK2>& visitor) const {
        NahtAn<MK2> naht{visitor};
        return kern(last, naht);
    }

    /// STILLE Form: der Visitor-Parameter EXISTIERT NICHT.
    template <class MK2 = MK>
        requires(std::same_as<MK2, MK> && !aktiv<MK2>)
    [[nodiscard]] LaufErgebnis fahren_impl(Last const& last) const {
        StillNaht naht{};
        return kern(last, naht);
    }

private:
    /// DER EINE KOERPER. Er orchestriert -- er rechnet nicht.
    ///
    /// Die Klammerung ist geschachtelt und genau so gemeint:
    ///     Makro-EIN  (ganzer Durchlauf)
    ///       Mikro-EIN/AUS  Achse "vorbereitung"
    ///       Mikro-EIN/AUS  Achse "koeder"
    ///     Makro-AUS
    /// Die Stapel-Arena bildet diese Schachtelung ab; ihr Hoechststand ist damit 2, und genau das
    /// prueft die Abnahme -- eine Tiefe, die niemand nachrechnet, ist keine Aussage.
    template <class Naht>
    [[nodiscard]] LaufErgebnis kern(Last const& last, Naht& naht) const {
        LaufErgebnis erg{};

        // MAKRO-Klammer um den ganzen Durchlauf. Der EIN-Wert traegt die Saat, damit sich die Zeile
        // spaeter dem Lauf zuordnen laesst, ohne dass ein zweiter Schluessel gefuehrt werden muss.
        naht.template ein<ms::MessEbene::Macro, StationDurchlauf>(last.saat);

        // --- Achse 1: Vorbereitung. Der Hauptalgorithmus RUFT sie, er rechnet sie nicht. ----------
        naht.template ein<ms::MessEbene::Micro, StationAchseVorbereitung>(last.saat);
        erg.vorbereitung_ergebnis = Komposition::vorbereitung(last.saat, last.runden);
        naht.template aus<ms::MessEbene::Micro, StationAchseVorbereitung>(erg.vorbereitung_ergebnis);
        ++erg.achsen_gerufen;

        // --- Achse 2: die KOEDER-Achse. Ihr Rueckgabewert ist f(R) und reist als Messwert. --------
        naht.template ein<ms::MessEbene::Micro, StationAchseKoeder>(last.saat);
        erg.koeder_ergebnis = Komposition::koeder(last.saat);
        naht.template aus<ms::MessEbene::Micro, StationAchseKoeder>(erg.koeder_ergebnis);
        ++erg.achsen_gerufen;

        naht.template aus<ms::MessEbene::Macro, StationDurchlauf>(erg.achsen_gerufen);
        return erg;
    }
};

template <class Komposition>
struct SucheImplFabrik {
    /// Die Fabrik-Methode: MK kommt von oben, das Objekt entsteht hier.
    template <KonfigConcept MK>
    [[nodiscard]] static constexpr auto bauen() noexcept {
        return SucheHauptalgo<MK, Komposition>{};
    }
};

/// Wie viele Mess-ZEILEN ein Durchlauf erzeugt -- die Faltung, aus der der Planer die Kapazitaet
/// dimensioniert.
///
/// Sie steht hier und nicht im Planer, weil nur der Hauptalgorithmus weiss, wie oft er klammert. Der
/// Planer NIMMT die Zahl (kapazitaet_zeilen_rechnen), er raet sie nicht -- und `--check-size` rechnet
/// damit ab jetzt mit einer gefalteten statt einer geschaetzten Zeilenzahl.
///
/// EHRLICH: die Zahl gilt fuer die VOLLE Konfiguration. Faellt die Mikro-Ebene weg, faellt der
/// Mikro-Anteil mit -- deshalb ist sie ueber MK parametrisiert und keine nackte Konstante.
template <KonfigConcept MK>
[[nodiscard]] consteval std::uint64_t zeilen_je_op() noexcept {
    std::uint64_t n = 0;
    if constexpr (EbeneEingebaut<MK, ms::MessEbene::Macro>) { n += 2u; }      // ein Makro-Paar
    if constexpr (EbeneEingebaut<MK, ms::MessEbene::Micro>) { n += 2u * 2u; } // zwei Achsen-Paare
    return n;
}

} // namespace comdare::cache_engine::mess::pilot
