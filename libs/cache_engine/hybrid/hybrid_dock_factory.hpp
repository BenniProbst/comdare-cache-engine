#pragma once
// HY-A1 (Dock-Schicht) -- DIE ABSTRACT FACTORY: der EINZIGE Konstruktions-Ort der
// variant-Alternativen.
//
// ============================================================================================
// DER AUFTRAG, WOERTLICH (Owner-KORREKTUR Paragraf 49, 20.07.2026 -- Ledger-Stichwort
// "49-KORREKTUR"; die im Design-Dokument genannte Zeile :2663 ist gewandert, s.
// hybrid_dock_contract.hpp Kopf)
// ============================================================================================
// variant NUR als Traeger abweichender Unter-Pruef-Dock-Typen/-Vertraege, "gelesen und verarbeitet
// per Abstract-Factory-Methode"; die Haupt-Kommunikation zu den Tier-Binary-Observern bleibt
// statisch (IObservableTier, zero-cost).
//
// ============================================================================================
// WARUM "EINZIGER KONSTRUKTIONS-ORT" MEHR IST ALS EINE STIL-REGEL
// ============================================================================================
// Die Abbildung Vertrag -> Dock-Typ ist die einzige Stelle, an der aus DATEN (einer contract_id aus
// der Konfiguration) CODE (ein konkreter Dock-Typ) wird. Gaebe es sie zweimal -- etwa zusaetzlich
// als Fallunterscheidung im Dock-Array --, liefen die beiden auseinander, sobald ein neuer
// Vertrags-Dock dazukommt: die eine Stelle kennte ihn, die andere nicht, und welche greift, haenge
// vom Aufrufweg ab. Deshalb traegt DockArray::attach keine eigene Vertrags-Logik, sondern ruft
// hierher; und deshalb steht seine Definition in DIESER Datei.
//
// FOLGE FUER DEN LESER, ausdruecklich: wer DockArray::attach benutzt, MUSS diese Datei inkludieren.
// Das ist Absicht und keine Nachlaessigkeit -- ein Dock-Array ohne Factory kann keine Docks bauen,
// und die fehlende Definition sagt genau das, statt still eine zweite Abbildung zu erlauben.
// Alle uebrigen Funktionen des Arrays (detach, antrieb_binden, antrieb_von, slot, size) sind ohne
// diese Datei vollstaendig nutzbar; nur die KONSTRUKTION haengt an ihr.
//
// @design docs/architecture/20260802-hybrid_tier_stufe_soll_design.md Abschnitt 3.1 (e)

#include "hybrid_dock_array.hpp"
#include "hybrid_dock_contract.hpp"
#include "hybrid_pruef_dock.hpp"

#include <cstddef>
#include <utility>

namespace comdare::cache_engine::hybrid {

// ------------------------------------------------------------------------------------------
// (1) DIE FACTORY
// ------------------------------------------------------------------------------------------

/// HybridDockFactory -- bildet einen Vertrags-Deskriptor auf die passende variant-Alternative ab.
class HybridDockFactory {
public:
    /// make_dock -- Deskriptor rein, konstruierte Alternative raus. errno-Stil (0 == ok).
    ///
    /// BEI FEHLSCHLAG BLEIBT `out` UNVERAENDERT. Das ist die Zusage, auf die sich der Aufrufer
    /// verlassen koennen muss: ein halb gebautes Dock in einem Slot waere schlimmer als gar keins,
    /// weil es sich wie ein fertiges anfuehlt. (Dieselbe Konvention faehrt der Loader:
    /// "Bei Misserfolg: errno-style status, handle_out unveraendert", anatomy_module_loader.hpp.)
    ///
    /// DREI GRUENDE ZU SCHEITERN, und sie sind ausdruecklich VERSCHIEDEN benannt:
    ///   hybrid_status_unbekannter_vertrag     -- die contract_id zeigt in keine Registry-Zeile.
    ///   hybrid_status_kein_zielfaehiges_genus -- das Ziel ist ein Klassifikations-Genus (Gate S1).
    ///   hybrid_status_contract_ohne_dock_typ  -- der Vertrag existiert, sein Dock-Typ noch nicht.
    /// Der dritte ist der heute haeufigste und der einzige, der KEIN Fehler des Aufrufers ist,
    /// sondern der Stand des Baus: vier Vertraege stehen im Angebot, einer traegt einen Typ. Ihn
    /// mit dem ersten zusammenzuwerfen ("unbekannt") wuerde einen Bau-Stand als Eingabefehler
    /// ausgeben und den Anwender an der falschen Stelle suchen lassen.
    [[nodiscard]] static int make_dock(DockContractDescriptor const& desc, HybridDockVariant& out) noexcept {
        if (int const status = pruefe_dock_deskriptor(desc); status != hybrid_status_ok) return status;

        // Die Abbildung selbst. Sie ist bewusst ein switch ueber den ENUM-Wert und keine Tabelle
        // von Funktionszeigern: der Ziel-TYP je Zweig ist compile-time verschieden, und genau das
        // soll sichtbar bleiben. Ein fehlender Zweig faellt in den default und meldet den Stand des
        // Baus -- er kann nicht still den Standard liefern.
        switch (static_cast<HybridDockContract>(desc.contract_id)) {
            case HybridDockContract::Standard: out.emplace<StandardHybridDock>(); return hybrid_status_ok;
            // KEIN default-Zweig, der still etwas baut: die drei uebrigen Vertraege sind oben
            // bereits durch pruefe_dock_deskriptor (dock_typ_gebaut == false) abgefangen. Kaeme ein
            // fuenfter Enum-Wert dazu, ohne dass jemand hier einen Zweig ergaenzt, faellt er auf die
            // Zeile darunter -- sichtbar, benannt, nicht still.
            case HybridDockContract::Rollback:
            case HybridDockContract::Scan:
            case HybridDockContract::ResourceControl: return hybrid_status_contract_ohne_dock_typ;
        }
        return hybrid_status_unbekannter_vertrag;
    }
};

// ------------------------------------------------------------------------------------------
// (2) DIE ATTACH-DEFINITION -- hier, weil sie die Factory braucht
// ------------------------------------------------------------------------------------------

/// Siehe Deklaration in hybrid_dock_array.hpp. Rueckgabe: Slot-Index (>= 0) oder -status (< 0).
///
/// REIHENFOLGE, und warum sie so herum ist: erst wird das Dock GEBAUT, dann der Platz belegt.
/// Andersherum bliebe bei einem Factory-Fehlschlag ein leerer, aber belegt gezaehlter Slot zurueck
/// -- ein Array, das voll meldet und nichts traegt. Der Platz wird deshalb erst reserviert, wenn
/// das Dock steht; bei der Runtime-Policy heisst das, dass ein fehlgeschlagener attach den Vektor
/// nicht wachsen laesst.
template <class Policy>
int DockArray<Policy>::attach(DockContractDescriptor const& desc) noexcept {
    HybridDockVariant gebaut{};
    if (int const status = HybridDockFactory::make_dock(desc, gebaut); status != hybrid_status_ok) return -status;

    std::size_t const platz = erster_freier_platz();
    // F-4: gegen den LAUFZEIT-Deckel pruefen, nicht gegen die Policy-Groesse.
    if (platz >= kapazitaet()) return -hybrid_status_array_voll;

    speicher_[platz].emplace();
    speicher_[platz]->dock    = std::move(gebaut);
    speicher_[platz]->desc    = desc;
    speicher_[platz]->antrieb = nullptr; // der Proxy bindet ihn spaeter (HY-A2)
    return static_cast<int>(platz);
}

} // namespace comdare::cache_engine::hybrid
