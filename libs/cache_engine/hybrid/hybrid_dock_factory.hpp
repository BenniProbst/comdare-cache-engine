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
// FOLGE FUER DEN LESER -- FORTGESCHRIEBEN (F-9-Schliessung, G4/L21, 2026-08-19): der Satz "wer
// DockArray::attach benutzt, MUSS diese Datei inkludieren" galt bis zum F-9-Fix und war dessen
// Fund: die Pflicht stand nur in Prosa, der Verstoss fiel erst am LINKER. Seitdem reist die
// attach-Definition ueber den gemeinsamen dritten Header (hybrid_dock_attach.hpp, End-Include
// unten) mit BEIDEN Einstiegen mit -- der Array-Header genuegt. Unveraendert wahr bleibt der
// Kern: ein Dock-Array ohne Factory kann keine Docks bauen; die Abbildung Vertrag -> Typ lebt
// weiter NUR hier (make_dock), attach ruft sie und traegt keine zweite.
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
// (2) DIE ATTACH-DEFINITION -- seit F-9 (G4/L21) im gemeinsamen dritten Header
// ------------------------------------------------------------------------------------------
// Sie stand HIER (Begruendung damals: "hier, weil sie die Factory braucht") und ist nach
// hybrid_dock_attach.hpp gewandert, damit auch der Array-Header sie mitliefert. Der End-Include
// unten expandiert sie hinter dieser Klasse -- vor jedem Nutzer-Code dieses Einstiegs.

} // namespace comdare::cache_engine::hybrid

// F-9 (G4/L21): die attach-Definition expandiert HIER -- hinter der Factory-Klasse. Beide
// Einstiege (Array-Header ueber SEINEN End-Include auf diese Datei, diese Datei direkt) liefern
// sie damit mit; der Konstruktions-Ort bleibt unveraendert die Factory oben.
#include "hybrid_dock_attach.hpp"
