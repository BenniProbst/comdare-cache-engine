#pragma once
// HY-A1 (Dock-Schicht) -- DAS DOCK-ARRAY und die EINE erlaubte std::variant-Stelle des Hauses
// ausserhalb der CEB.
//
// ============================================================================================
// DIE VARIANT-KANTE, EXAKT
// ============================================================================================
// Owner-E1 woertlich (02.08.2026): "Es ist auch einstellbar, dass die Anzahl der ABI stabilen
// Pruefdocks an der hybrid-Tier-Binary zahlenmaessig dynamisch variieren kann, genau dafuer braucht
// es std::variant in einem wahlweise statischen oder runtime array."
//
// Und die Verbots-Seite, aus demselben Satz: "Aber in den plain Tier-Binaries ist das verboten."
//
// WARUM DAS VERBOT UEBERHAUPT EXISTIERT -- die Begruendung ist Speicher-Mathematik, keine
// Aesthetik. Im Ledger unter dem Stichwort "SPEICHER-MATHEMATIK (der Kern-Grund)" (Stand
// 17.08.2026 bei :20060, zweite Fassung bei :27831 -- Stichwort statt Zeilennummer, weil der
// Ledger waechst und Anker wandern): golden N = 131.072 Binaries; mit variant-Bloat ~60 MB je
// Binary sind das 7,5 TB gegen 6 TB verfuegbar. Bloat-reduziert (2-5 MB) bleiben 256-640 GB.
// std::variant zu vermeiden ist damit die dritte Voraussetzung dafuer, dass das golden N ueberhaupt
// baubar ist. Wer diese Datei liest und denkt "ein variant mehr schadet nicht", rechnet mit der
// falschen Zahl.
//
// ERLAUBT ist deshalb GENAU: HybridDockVariant im DockSlot dieses Arrays -- der Traeger dafuer, dass
// N Docks ZAHLENMAESSIG dynamisch variieren und TYPLICH abweichende Vertraege tragen koennen
// (soll_design 3.2 Punkt 1).
// VERBOTEN bleibt variant: in jeder plain Tier-Binary (uneingeschraenkt), in der
// Haupt-Observer-Kommunikation, als Achsen-Traeger und im Planer (soll_design 3.2 Punkt 3).
// GEDULDET bleibt er in der CEB (Owner-Duldung, unveraendert; im Design-Dokument als LEDGER:3489
// gefuehrt -- Zeilen-Anker jener Zeit sind gewandert, die Aussage nicht).
//
// EHRLICHE GRENZE DIESER ZUSAGE: der Satz "DockSlot ist die einzige variant-Ausnahme" ist eine
// ZONEN-Aussage ueber libs/cache_engine/hybrid/, KEINE System-Aussage. Systemweit gibt es die
// CEB-Duldung, zwei legitime achsenfremde Nutzungen (BuildError, PressureState) und einen
// test-only quarantaenierten Cluster. Bewacht wird hier die Zone -- der Test
// test_hy_a1_dock_contract.cpp liest die hybrid/-Quelldateien und belegt, dass "std::variant<"
// darin an genau einer Stelle vorkommt: hier.
//
// ============================================================================================
// "STATISCH ODER RUNTIME" HEISST KAPAZITAET, NICHT BELEGUNG
// ============================================================================================
// Die Policy waehlt, wo der Speicher herkommt (soll_design 3.1 (c)). Die BELEGUNG ist in BEIDEN
// Faellen dynamisch -- ein statisches Array startet leer und fuellt sich zur Laufzeit. Wer
// "statisch" als "fest belegt" liest, baut ein Array, das N Docks vortaeuscht, die es nicht gibt.
//
// DER DECKEL IST EINE ZAHL, NICHT ZWEI: kHybridNodeObergrenzeDefault (32) aus der Synthese-Matrix.
// Der alte Widerspruch 8-gegen-32 ist per KON28-03 aufgeloest -- die 32 loest die 8 als
// Dock-Obergrenze ab (Owner 12.08.: "maximal 32"). Sie ist ein PROGRAMM-DECKEL: eine statische
// Maximal-Variable, deren Wert willkuerlich gesetzt und W7-anpassbar ist -- KEIN Fach-Nenner.
// Docks sind NICHT Mess-Permutationen (KON41-03); dass dort ebenfalls eine 32 steht, ist Zufall.
// Diese Datei importiert die Konstante, statt sie zu wiederholen: zwei Konstanten waeren zwei
// Wahrheiten, und genau daran ist der Widerspruch entstanden.

#include "anatomy/anatomy_base.hpp"
#include "anatomy/observable_tier.hpp"
#include "heuristik_adapter_synthese_matrix.hpp" // kHybridNodeObergrenzeDefault -- der EINE Deckel
#include "hybrid_dock_contract.hpp"
#include "hybrid_pruef_dock.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <variant>
#include <vector>

namespace comdare::cache_engine::hybrid {

// ------------------------------------------------------------------------------------------
// (1) DIE EINE VARIANT-STELLE
// ------------------------------------------------------------------------------------------

/// HybridDockVariant -- der Traeger der Dock-TYP-Varianz. Heute genau eine Alternative, weil der
/// F8-Minimal-DoD genau einen Dock-Typ kennt.
///
/// DASS ER TROTZDEM SCHON EIN variant IST, ist Absicht und keine Vorratshaltung: die Stelle ist
/// diejenige, die der Owner ausdruecklich freigegeben hat, und sie ist die einzige. Sie jetzt als
/// nacktes StandardHybridDock zu bauen und spaeter zum variant umzubauen hiesse, den Slot-Typ, die
/// Factory-Signatur und jede Wache darueber zweimal zu schreiben -- und beim zweiten Mal waere die
/// Zonen-Wache neu zu begruenden. Additiv wachsen kann diese Zeile; entstehen muss sie einmal.
using HybridDockVariant = std::variant<StandardHybridDock /* additiv je neuem Vertrags-Dock-Typ */>;

// ------------------------------------------------------------------------------------------
// (2) DER SLOT
// ------------------------------------------------------------------------------------------

/// DockSlot -- ein belegter Dock-Platz des Arrays.
///
/// DAS antrieb_-FELD IST DER CACHE, NICHT DIE ZWEITE WAHRHEIT. Die Bindung wird per visit in das
/// Dock geschrieben (dort lebt sie als Zustand des Dock-Typs) und HIER gespiegelt. Der Op-Pfad
/// liest ausschliesslich diese Spiegelung und fasst den variant nie an -- das ist woertlich die
/// Design-Zusage: "Nach dem visit wird der monomorphisierte Antrieb im Slot GECACHED; der
/// Op-Hot-Path ist variant-frei und cast-frei" (soll_design 3.2 Punkt 2).
/// Beide Werte werden ausschliesslich durch DockArray::antrieb_binden gesetzt -- es gibt keinen
/// zweiten Schreiber, an dem sie auseinanderlaufen koennten.
///
/// WAS HIER FEHLT UND WARUM: der HybridBinaryProxy (soll_design 3.1 (d)), der das AnatomyModuleHandle
/// besitzt und den Antrieb ueberhaupt erst beschafft. Er zieht AnatomyModuleLoader + das
/// Drive-Buendel aus der BUILDER-Lib in diese Stufe -- und deren Schichten-Zuordnung ist die offene
/// Auflage K2 (soll_design Abschnitt 7: "(a) in eine stufen-neutrale Lib extrahieren -- Vorzug --
/// oder (b) die Hybrid-Stufe linkt bewusst die Builder-Loader-Lib"). Bis das entschieden ist, nimmt
/// der Slot den fertigen Zeiger ENTGEGEN. Die Naht ist damit genau eine Funktion breit.
struct DockSlot {
    HybridDockVariant             dock;
    DockContractDescriptor        desc{};
    anatomy::IObservableTier*     antrieb = nullptr; ///< Cache der Bindung -- der Op-Pfad liest DIESEN
};

// ------------------------------------------------------------------------------------------
// (3) DIE BEIDEN POLICIES
// ------------------------------------------------------------------------------------------
// Beide fuehren std::optional-Slots. Der Design-Sketch nennt fuer die Runtime-Seite einen nackten
// std::vector<DockSlot> (3.1 (c)); optional ist die genauere Form derselben Absicht, weil detach
// sonst je nach Policy etwas anderes bedeutete: beim Array wird ein Platz frei, beim Vektor
// ruecken die Nachbarn nach und ALLE Slot-Indizes dahinter wandern. Ein wanderender Index waere
// ein Fehler mit Ansage -- Aufrufer halten Slot-Indizes ueber Umschaltpunkte hinweg.

/// Statische Kapazitaet: der Speicher steht zur Compile-Zeit fest, die Belegung nicht.
template <std::size_t MaxN>
struct StatischeDockArrayPolicy {
    using storage                             = std::array<std::optional<DockSlot>, MaxN>;
    static constexpr bool        statisch     = true;
    static constexpr std::size_t kapazitaet   = MaxN;

    static_assert(MaxN > 0, "HY-A1-ARRAY: eine Kapazitaet 0 traegt kein einziges Dock.");
    static_assert(MaxN <= kHybridNodeObergrenzeDefault,
                  "HY-A1-ARRAY: die statische Kapazitaet ueberschreitet den Programm-Deckel "
                  "(KON28-03, 'maximal 32'). Der Deckel ist willkuerlich gesetzt und W7-anpassbar "
                  "-- aber er wird an EINER Stelle angehoben, nicht hier umgangen.");
};

/// Runtime-Kapazitaet: der Speicher waechst zur Laufzeit, gedeckelt auf denselben Programm-Deckel.
struct RuntimeDockArrayPolicy {
    using storage                             = std::vector<std::optional<DockSlot>>;
    static constexpr bool        statisch     = false;
    static constexpr std::size_t kapazitaet   = kHybridNodeObergrenzeDefault;
};

// ------------------------------------------------------------------------------------------
// (4) DAS ARRAY
// ------------------------------------------------------------------------------------------

/// DockArray<Policy> -- N Hybrid-Pruef-Docks, Policy-gewaehlter Speicher, IMMER dynamische Belegung.
///
/// Fehler-Konvention, bewusst gemischt und deshalb hier benannt:
///   attach() liefert einen SLOT-INDEX (>= 0) oder -status (< 0). Ein Index ist keine
///     Erfolgs-Null -- der errno-Stil (0 == ok) waere hier mehrdeutig, weil 0 ein gueltiger Index ist.
///   Alle uebrigen Funktionen liefern hybrid_status_* im reinen errno-Stil (0 == ok), wie
///     IPruefDock und der Loader.
template <class Policy>
class DockArray {
public:
    DockArray() noexcept(Policy::statisch) {
        if constexpr (!Policy::statisch) { speicher_.reserve(4); }
    }

    /// Die KAPAZITAET (Compile-Zeit-Groesse bzw. Programm-Deckel) -- nicht die Belegung.
    [[nodiscard]] static constexpr std::size_t kapazitaet() noexcept { return Policy::kapazitaet; }

    /// Die BELEGUNG: wie viele Slots tragen gerade ein Dock. Dynamisch in BEIDEN Policies.
    [[nodiscard]] std::size_t size() const noexcept {
        std::size_t n = 0;
        for (auto const& s : speicher_) {
            if (s.has_value()) ++n;
        }
        return n;
    }

    /// Legt ein Dock nach Deskriptor an. Rueckgabe: Slot-Index (>= 0) oder -hybrid_status_* (< 0).
    ///
    /// Der Dock-Wert kommt AUSSCHLIESSLICH aus der Factory -- sie ist der einzige Konstruktions-Ort
    /// der variant-Alternativen (Paragraf-49-KORREKTUR: "per Abstract-Factory-Methode gelesen und
    /// verarbeitet"). Deshalb steht hier keine eigene Fallunterscheidung ueber Vertraege; sie waere
    /// die zweite Stelle, an der Vertrag zu Typ wird, und die beiden liefen auseinander.
    /// Die Deklaration steht in hybrid_dock_factory.hpp; die Definition unten ist bewusst
    /// out-of-line, damit diese Datei nicht von der Factory abhaengt und die Factory nicht von ihr.
    [[nodiscard]] int attach(DockContractDescriptor const& desc) noexcept;

    /// Gibt einen Slot frei. Loescht die Antriebs-Bindung MIT -- ein Slot darf nie einen Zeiger in
    /// ein Modul ueberleben lassen, das gleich entladen wird (soll_design 3.3: destroy-vor-dlclose,
    /// Quiesce am Op-Rand).
    [[nodiscard]] int detach(std::size_t slot) noexcept {
        if (slot >= speicher_.size() || !speicher_[slot].has_value()) return hybrid_status_slot_leer;
        // Erst die Bindung loesen (im Dock UND im Cache), dann den Slot raeumen. Die Reihenfolge
        // ist die kleine Schwester der destroy-vor-dlclose-Ordnung: ein halb geraeumter Slot mit
        // lebendem Zeiger ist genau der Zustand, den es nie geben darf.
        std::visit([](auto& d) noexcept { d.antrieb_binden(nullptr); }, speicher_[slot]->dock);
        speicher_[slot]->antrieb = nullptr;
        speicher_[slot].reset();
        return hybrid_status_ok;
    }

    /// DER UMSCHALTPUNKT: hier laeuft der einzige visit des Lebenszyklus (neben attach/detach).
    /// Danach liest der Op-Pfad nur noch den Cache.
    [[nodiscard]] int antrieb_binden(std::size_t slot, anatomy::IObservableTier* p) noexcept {
        if (slot >= speicher_.size() || !speicher_[slot].has_value()) return hybrid_status_slot_leer;
        std::visit([p](auto& d) noexcept { d.antrieb_binden(p); }, speicher_[slot]->dock);
        speicher_[slot]->antrieb = p;
        return hybrid_status_ok;
    }

    /// DER OP-PFAD. Kein visit, kein Cast -- ein Zeiger-Read aus dem Slot-Cache.
    /// Ein unbelegter oder ungebundener Slot liefert nullptr statt UB: der Aufrufer bekommt eine
    /// Antwort, die er pruefen kann, nicht einen Absturz an einer anderen Stelle.
    [[nodiscard]] anatomy::IObservableTier* antrieb_von(std::size_t slot) const noexcept {
        if (slot >= speicher_.size() || !speicher_[slot].has_value()) return nullptr;
        return speicher_[slot]->antrieb;
    }

    /// Traegt der Slot ein gebundenes Ziel? (Belegt und gebunden sind zwei verschiedene Zustaende.)
    [[nodiscard]] bool slot_ist_bereit(std::size_t slot) const noexcept { return antrieb_von(slot) != nullptr; }

    /// Lesender Slot-Zugriff (Deskriptor, Cache, Dock). nullptr fuer leere/ungueltige Indizes.
    [[nodiscard]] DockSlot const* slot(std::size_t slot_index) const noexcept {
        if (slot_index >= speicher_.size() || !speicher_[slot_index].has_value()) return nullptr;
        return &speicher_[slot_index].value();
    }

private:
    /// Der erste freie Platz -- oder kapazitaet(), wenn keiner frei ist.
    /// Freie Plaetze WERDEN WIEDERVERWENDET (nicht nur angehaengt): sonst waere ein Array nach
    /// N attach/detach-Paaren voll, obwohl es leer ist.
    [[nodiscard]] std::size_t erster_freier_platz() noexcept {
        for (std::size_t i = 0; i < speicher_.size(); ++i) {
            if (!speicher_[i].has_value()) return i;
        }
        if constexpr (Policy::statisch) {
            return Policy::kapazitaet; // voll
        } else {
            if (speicher_.size() >= Policy::kapazitaet) return Policy::kapazitaet; // Programm-Deckel
            speicher_.emplace_back();
            return speicher_.size() - 1;
        }
    }

    typename Policy::storage speicher_{};
};

// Die Kapazitaets-Zusage, an der Quelle statt nur im Test: die Runtime-Policy nimmt DENSELBEN
// Deckel wie die Synthese-Matrix. Zwei Konstanten waeren der alte 8-gegen-32-Widerspruch zurueck.
static_assert(RuntimeDockArrayPolicy::kapazitaet == kHybridNodeObergrenzeDefault,
              "HY-A1-DECKEL: Dock-Array und Synthese-Matrix teilen EINE Obergrenze (KON28-03). "
              "Wer hier eine eigene Zahl setzt, holt den 8-gegen-32-Widerspruch zurueck.");

} // namespace comdare::cache_engine::hybrid
