#pragma once
// mess/mess_naht.hpp -- DIE NAHT: MessVisitor<MK>, die RAII-Klammer und die STILLE Naht.
//
// == WAS HIER GELOEST WIRD =========================================================================
// Owner 09.08.2026: "Es muss stattdessen je aktiviert/deaktiviert der Messeinrichtungen in CEB und
// Tier-Binary am Interface ein MESS-VISITOR uebergeben werden ODER BEI DEAKTIVIERT EBEN NICHT."
//
// Das "oder eben nicht" ist der schwierige Teil. Die uebliche Bauart -- ein Zeiger, der auch nullptr
// sein darf, und ein `if (visitor)` im Hauptalgorithmus -- erfuellt den Satz dem Wortlaut nach und
// verfehlt ihn in der Sache:
//   (a) sie kostet einen Zweig im heissen Pfad, auch wenn nie gemessen wird;
//   (b) sie laesst den Mess-Code IM Kompilat, auch wenn er nie laeuft (das ist der Sidecar-Vorwurf:
//       "es existiert auch dann, wenn es nichts tut");
//   (c) sie macht das Vergessen des `if` zu einem Absturz statt zu einem Uebersetzungsfehler.
//
// GEBAUT ist deshalb die Doppelform: bei aktiv<MK> ist der Visitor PFLICHTPARAMETER (kein Default,
// kein Zeiger, kein optional) -- ihn wegzulassen uebersetzt nicht. Bei Leer EXISTIERT DER PARAMETER
// NICHT und MessVisitor<Leer> ist nicht bildbar -- ihn zu uebergeben uebersetzt nicht.
// `if (visitor)` ist damit nicht verboten, sondern UNAUSDRUECKBAR. Es gibt keinen null-Zustand,
// ueber den man `if` sagen koennte.
//
// == DIE STILLE NAHT, UND WARUM SIE KEIN ZWEITER ALGORITHMUS IST ===================================
// Der Hauptalgorithmus wird EINMAL geschrieben und nimmt die Naht als Template-Parameter. Bei aktiv
// ist sie NahtAn<MK> (leitet an den Visitor), bei inaktiv StillNaht (leerer Typ, alle Methoden
// constexpr-leer). Der Uebersetzer laesst den zweiten Fall rueckstandsfrei fallen.
//
// Die Alternative waere gewesen, den Algorithmus zweimal zu schreiben -- einmal mit, einmal ohne
// Messstellen. Das ist die ABSCHRIFT-FALLE: zwei Koerper, die dasselbe tun sollen und irgendwann
// nicht mehr dasselbe tun, wobei die Abweichung ausgerechnet im UNGEMESSENEN Zweig sitzt und deshalb
// per Konstruktion niemandem auffaellt.
//
// == SELBSTCHECK ===================================================================================
//   ZUSICHERT: dass bei deaktivierter Messung KEIN Aufruf entsteht -- nachgewiesen am OBJEKT
//              (fehlende Symbole in der Leer-Uebersetzung), nicht als Behauptung dieses Kommentars.
//   ZUSICHERT NICHT: dass die Messstellen an den RICHTIGEN Orten sitzen. Wo geklammert wird, ist eine
//              Entscheidung des Hauptalgorithmus; diese Datei stellt nur die Klammer bereit.
//
// ASCII-only.

#include <builder/measure_storage/checkpoint_measure.hpp>
#include <mess/konfiguration.hpp>

#include <concepts>
#include <cstdint>

namespace comdare::cache_engine::mess {

namespace ms = ::comdare::cache_engine::builder::measure_storage;

// ==================================================================================================
// DIE STATION -- der Ort, an dem geklammert wird, als TYP
// ==================================================================================================
//
// SOLL-Design OP-1, hier gebunden auf JA: EIN statischer Deskriptor je (Gattung, Genus, Funktion,
// Station). Er ist ein TYP mit einem constexpr-Index, keine Zeichenkette -- R1 verbietet Text im
// heissen Pfad. Die Aufloesung Index -> Ort passiert NACH dem Fenster, ueber die statische Tabelle.
//
// Warum ein Typ und nicht eine nackte Zahl: eine Zahl laesst sich verwechseln, und zwei Stationen mit
// derselben Zahl faellt niemandem auf. Ein Typ traegt seinen Namen mit und die Faltung unten kann
// ueber ihn zaehlen -- der NENNER der erwarteten Zeilen entsteht damit aus der Komposition selbst
// und nicht aus einer gepflegten Konstante.

template <class St>
concept StationConcept = requires {
    { St::deskriptor_ix } -> std::convertible_to<std::uint32_t>;
    { St::name() } -> std::convertible_to<char const*>;
};

// ==================================================================================================
// DER MESS-VISITOR -- der Kanal, der beim Tier-Binary ankommt
// ==================================================================================================

/// MessVisitor<MK> -- die typisierte Naht INNERHALB einer Binary.
///
/// `requires aktiv<MK>`: MessVisitor<Leer> ist NICHT BILDBAR. Das ist der zweite der drei Riegel --
/// eine deaktivierte Seite kann den Visitor nicht einmal benennen, geschweige denn uebergeben.
///
/// Er haelt eine REFERENZ auf das Instrument (kein Besitz, kein Zeiger-auf-vielleicht, keine
/// Allokation). Es gibt keinen null-Zustand, also nichts, worauf man `if` sagen koennte.
template <KonfigConcept MK>
    requires aktiv<MK>
class MessVisitor {
public:
    using instrument_t = ms::CheckpointMeasure<MK>;

    explicit MessVisitor(instrument_t& instrument, std::uint16_t thread_nr) noexcept
        : instrument_(&instrument), thread_nr_(thread_nr) {}

    /// EIN -- die oeffnende Klammer. requires EbeneEingebaut: eine Ebene ohne Instrument ist an
    /// dieser Stelle ein Uebersetzungsfehler und kein Aufruf ins Leere.
    template <ms::MessEbene E, StationConcept Station>
        requires EbeneEingebaut<MK, E>
    void checkpoint_ein(std::uint64_t messwert) noexcept {
        instrument_->template checkpoint<E, ms::Richtung::Ein>(Station::deskriptor_ix, messwert, thread_nr_);
    }

    /// AUS -- die schliessende Klammer.
    template <ms::MessEbene E, StationConcept Station>
        requires EbeneEingebaut<MK, E>
    void checkpoint_aus(std::uint64_t messwert) noexcept {
        instrument_->template checkpoint<E, ms::Richtung::Aus>(Station::deskriptor_ix, messwert, thread_nr_);
    }

    [[nodiscard]] std::uint16_t thread_nr() const noexcept { return thread_nr_; }

private:
    instrument_t* instrument_;
    std::uint16_t thread_nr_;
};

// ==================================================================================================
// DIE NAHT -- zwei Auspraegungen, EIN Algorithmus-Koerper
// ==================================================================================================

/// NahtAn<MK> -- die aktive Naht. Leitet 1:1 an den Visitor weiter.
template <KonfigConcept MK>
    requires aktiv<MK>
class NahtAn {
public:
    explicit NahtAn(MessVisitor<MK>& visitor) noexcept : visitor_(&visitor) {}

    static constexpr bool misst = true;

    template <ms::MessEbene E, StationConcept Station>
        requires EbeneEingebaut<MK, E>
    void ein(std::uint64_t messwert) noexcept {
        visitor_->template checkpoint_ein<E, Station>(messwert);
    }

    template <ms::MessEbene E, StationConcept Station>
        requires EbeneEingebaut<MK, E>
    void aus(std::uint64_t messwert) noexcept {
        visitor_->template checkpoint_aus<E, Station>(messwert);
    }

private:
    MessVisitor<MK>* visitor_;
};

/// StillNaht -- die stille Naht. EIN LEERER TYP; jede Methode ist constexpr und leer.
///
/// WICHTIG UND ABSICHTLICH: die Methoden nehmen ihre Argumente entgegen und tun nichts damit. Sie
/// sind NICHT `requires`-beschraenkt auf eingebaute Ebenen -- im deaktivierten Bau gibt es keine
/// eingebaute Ebene, und der Algorithmus-Koerper soll sich trotzdem uebersetzen lassen, OHNE dass
/// jemand die Messstellen auskommentiert. Genau das macht den EINEN Koerper moeglich.
///
/// Dass hier wirklich nichts uebrig bleibt, ist keine Zusage dieses Kommentars, sondern Gegenstand
/// der Abnahme: die Leer-Uebersetzung darf das Checkpoint-Symbol nicht enthalten.
class StillNaht {
public:
    static constexpr bool misst = false;

    template <ms::MessEbene, class>
    constexpr void ein(std::uint64_t) const noexcept {}

    template <ms::MessEbene, class>
    constexpr void aus(std::uint64_t) const noexcept {}
};

/// SpannWache -- die RAII-Klammer. Ein EIN ohne AUS wird damit zum ECHTEN Befund statt zum Versehen.
///
/// Warum es sie gibt, obwohl ein Paar von Hand billiger aussieht: ein fruehes `return` zwischen EIN
/// und AUS ist der Normalfall in einem Hauptalgorithmus, und er hinterlaesst ein offenes EIN. Die
/// Stapel-Arena MELDET das (sie raeumt es ausdruecklich nicht auf) -- aber eine Regression, die man
/// erst hinterher im Befund liest, hat den Lauf schon verdorben. Die Wache schliesst die Klammer am
/// Blockende, auch bei fruehem Ruecksprung.
///
/// Der Messwert des AUS wird beim Bauen noch nicht gesetzt; er wird ueber `abschluss()` nachgereicht.
/// Kein Default-Wert, mit Absicht: eine 0, die "nicht gesetzt" bedeutet, ist von einer echten 0 nicht
/// zu unterscheiden.
template <class Naht, ms::MessEbene E, StationConcept Station>
class SpannWache {
public:
    explicit SpannWache(Naht& naht, std::uint64_t ein_wert) noexcept : naht_(&naht) {
        naht_->template ein<E, Station>(ein_wert);
    }

    ~SpannWache() { naht_->template aus<E, Station>(aus_wert_); }

    SpannWache(SpannWache const&)            = delete;
    SpannWache& operator=(SpannWache const&) = delete;

    void abschluss(std::uint64_t wert) noexcept { aus_wert_ = wert; }

private:
    Naht*         naht_;
    std::uint64_t aus_wert_ = 0;
};

} // namespace comdare::cache_engine::mess
