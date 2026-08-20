#pragma once
// HY-A1 (Dock-Schicht) -- DER GEMEINSAME DRITTE HEADER: die attach-Definition, von Array UND
// Factory gemeinsam getragen. (F-9-Schliessung, A2.5/G4/L21, 2026-08-19)
//
// ============================================================================================
// WARUM ES DIESE DATEI GIBT
// ============================================================================================
// A2.5-Fund F-9: attach war im Array-Header nur DEKLARIERT, die Definition stand in der Factory.
// Wer nur das Array inkludierte und attach rief, lief am LINKER auf, nicht am Compiler -- eine
// unmarkierte Naht. ZWEI Sentinel-Anlaeufe (Template-Spezialisierung als Sichtbarkeits-Wache,
// einmal Methode, einmal freie Funktion) scheiterten an der Sprachsemantik: "specialization of
// ... after instantiation" -- eine Abfrage im ERSTEN Header kann keine Antwort erzwingen, die
// erst der ZWEITE gibt. Das Register nennt als tragenden Weg den gemeinsamen dritten Header:
// die Definition wandert HIERHER, und beide bisherigen Einstiege liefern sie mit.
//
// DIE KETTE, UEBER DIE DIESE DATEI JEDEN NUTZER ERREICHT:
//   hybrid_dock_array.hpp   (Ende) -> hybrid_dock_factory.hpp (Ende) -> DIESE DATEI
//   hybrid_dock_factory.hpp (Ende) ----------------------------------> DIESE DATEI
// Der Expansionspunkt liegt IMMER hinter der Factory-Klasse UND hinter der Array-Klasse --
// gleich, welcher der drei Header zuerst inkludiert wird. Genau deshalb haengt der End-Include
// des Arrays an der FACTORY und nicht direkt hier: expandierte diese Datei am Array-Ende, dann
// braeche der Einstieg "Factory zuerst" (ihr Array-Include laeuft VOR ihrer Klasse; pragma-once
// liesse diese Datei dort expandieren, wo es die Factory noch nicht gibt).
//
// DIE ZUSAGE "DIE FACTORY IST DER EINZIGE KONSTRUKTIONS-ORT" IST UNBERUEHRT: sie ist eine
// Aussage darueber, WER baut (HybridDockFactory::make_dock), nicht darueber, in welcher Datei
// die attach-Zeile steht (so ausdruecklich schon der F-9-Marker). attach ruft weiterhin
// AUSSCHLIESSLICH die Factory; eine eigene Vertrags-Fallunterscheidung traegt es nicht.
//
// Die beiden Includes machen diese Datei als DRITTEN Einstieg self-contained; ueber die
// pragma-once-Marken sind sie in den Ketten oben wirkungslose Rueckverweise.
//
// @fund   FINDINGS A2.5 F-9; Deckungstest: tests/unit/test_hy_a1_attach_nur_array_include.cpp
// @design docs/architecture/20260802-hybrid_tier_stufe_soll_design.md Abschnitt 3.1 (e)

#include "hybrid_dock_array.hpp"
#include "hybrid_dock_factory.hpp"

#include <cstddef>
#include <utility>

namespace comdare::cache_engine::hybrid {

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
