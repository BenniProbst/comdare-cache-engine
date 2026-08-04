#pragma once
// E-24 C6 (2026-08-04, b-Teil / ABI-EREIGNIS-SERIE, Bauplan docs/sessions/20260803-DOSSIER-e24-fenster-bauplan.md
// Paragraf 3.2-C6) -- GenusObserverAggregate<G, N>: die GEMEINSAME Wire-LAYOUT-Form der per-Achsen-Snapshots
// der vier Container-Genus-Ebenen. Die vier benannten Formen SetObserverAggregate<13> /
// SequenceObserverAggregate<9> / AdapterObserverAggregate<11> / ViewObserverAggregate<5> sind Alias-Templates
// darauf (je in set_/sequence_/adapter_/view_tier_v2.hpp) -- EIN Layout, VIER genus-getaggte Typen.
//
// WARUM UEBERHAUPT (Katalog docs/sessions/20260804-DOSSIER-achsen-qualitaets-parameter-katalog.md Sektion 2):
// Bis C5 querte je Genus NUR ein flacher Hand-POD die ABI-Grenze (SetObserverSnapshotV1 & Co.) -- die
// per-Achsen-Sicht observe_axes() der C3-Anatomien blieb IN-PROCESS und war cross-boundary unsichtbar. Ein
// Freeze in diesem Zustand waere die woertliche "stille Qualitaets-Luecke" (Owner-Terminierung LEDGER:3812
// Punkt 3). C6 hebt die per-Achsen-Sicht auf die Wire-Ebene.
//
// APPEND-ONLY-V2-DOKTRIN (Auflage 5, E24-DOSSIER:267 / Bauplan 3.2-C6) -- die drei Regeln, die dieses
// Layout traegt:
//   (1) NEBEN, NICHT STATT: die flachen V1-PODs (set_tier.hpp:17-30, sequence_tier.hpp:12-23,
//       view_tier.hpp:12-22, adapter_tier.hpp:26-39) bleiben BYTE-IDENTISCH eingefroren. Diese Form tritt
//       DANEBEN, sie ersetzt keinen einzigen V1-Feld-Offset.
//   (2) KEIN VTABLE-ANHANG: die Abfrage laeuft ueber EIGENE Sub-Interfaces (ISetTierV2 & Co.), die der
//       genus-typisierte ABI-Adapter ZUSAETZLICH erbt -- die vtables von ISetTier/ISequenceTier/
//       IAdapterTier/IViewTier bleiben unberuehrt. Der Host fragt 1x KALT je Modul per dynamic_cast ab
//       (Praezedenz IObservableTier observable_tier.hpp:21-27, IMigratableTier :211-213); eine Alt-DLL
//       liefert nullptr -> sauberes Degradieren statt Absturz.
//   (3) FLACH + KOMPOSITION-UNABHAENGIG: nur uint64/int64, feste Kantenlaengen, kein STL/kein vtable
//       -> standard_layout + trivially_copyable -> memcpy ueber die Modul-Grenze. Die Composition-
//       abhaengigen In-Process-Formen (SetAxisObservation<C> & Co.) koennen das NICHT und bleiben
//       in-process; diese Form ist ihre Wire-PROJEKTION.
//
// SCHEMA-VERTRAG (Praezedenz kV3AxisSchema, observable_tier.hpp:69-105): WELCHES Feld in axis_stats[T][f]
// steht, sagt eine per-Genus-Schema-Tabelle vom Typ GenusAxisFieldNames. Sie ist die SINGLE SOURCE zwischen
// dem Schreiber (im Modul-Binary) und den CSV-Spaltennamen (Host) -- ein Wert ohne Namen ist auf dieser
// Ebene verboten (D2-Doktrin: leere Spalte MIT Grund, nie stille 0).
//
// E13 (LEDGER:3828, Owner-KERN Programm-Messbaeume): seg_ns[N] traegt das PER-SLOT-TIMING der Gattung --
// das Analogon zu ComdareTierObserverSnapshot::seg_ns[18] (observable_tier.hpp:134). Ohne per-Slot-Timing
// waeren Container-Messbaeume blind; Nachruesten NACH dem Freeze waere ein zweites Wire-/Major-Ereignis.
//
// REKURSIONS-INVARIANTE (Owner-KERN NACHTRAG 3 (d), LEDGER:3815; Katalog Sektion 2 Punkt 11): die
// Einsammlung ist FLACH und IMPERATIV je Ebene -- kein rekursiver Laufzeit-Abstieg durch geschachtelte
// Aggregate. Diese Form hat deshalb bewusst KEIN geschachteltes Aggregat-Member: eine Ebene, ein
// rechteckiges Feld, eine Schleifen-freie `if constexpr`-Einsammlung im ABI-Adapter.
//
// @related [[feedback_recursive_dock_planer_ceb_tier_abi_stable_so]] [[feedback_hybrid_tier_stufe_hinter_ceb_variant_ausnahme]]

#include "anatomy_base.hpp" // AnatomyGenus (das Genus-Tag dieser Form)

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace comdare::cache_engine::anatomy {

// ---------------------------------------------------------------------------
// Kanten-Konstanten des Gattungs-Wire-Layouts
// ---------------------------------------------------------------------------

/// Feld-Spalten je Achsen-Slot. 8 ist NICHT geraten, sondern der gemessene Bedarf der C6-V-Formen:
/// InnerContainerStatistics traegt 8 Felder (inner_container_observable.hpp:43-54) und GrowthStatistics
/// 7 + growth_factor_milli (E1-Detail-Entscheid, s. sequence_tier_v2.hpp) = ebenfalls 8. Damit deckt die
/// Kante den breitesten Bedarf EXAKT und traegt zugleich die SA-Praezedenz kV3FieldCount == 8
/// (observable_tier.hpp:53) -- eine Kante, ein Wert ueber alle Gattungen.
inline constexpr std::size_t kGenusAxisFieldCount = 8;

/// Flache GATTUNGS-Zaehler (nicht achsen-gebunden) je Wire-Form. Hier stehen genau die Groessen, die
/// EHRLICHKEIT verlangt, die aber im eingefrorenen V1-POD keinen Platz mehr haben -- z. B. die Set-Versuchs-
/// Zaehler insert_attempt_count/erase_attempt_count (Katalog Sektion 2 Punkt 4). Auch sie sind
/// schema-benannt; ein unbenannter Slot ist ein LEERER Slot, kein stiller Wert.
inline constexpr std::size_t kGenusStatFieldCount = 4;

/// Format-Version der Gattungs-Aggregat-Familie. Die Loader-Kompatibilitaet laeuft ueber den ABI-Major
/// (anatomy_module_abi_v1_decl.hpp) -- diese Konstante dient Diagnose/Tests, exakt wie
/// kTierObserverSnapshotVersionUnified (observable_tier.hpp:166).
inline constexpr std::uint32_t kGenusObserverAggregateVersion = 1;

// ---------------------------------------------------------------------------
// Schema-Tabellen-Typ
// ---------------------------------------------------------------------------

/// GenusAxisFieldNames -- die Namen der 8 Spalten EINES Achsen-Slots. nullptr = Spalte ungenutzt.
/// Muster/Praezedenz: V3AxisFieldNames (observable_tier.hpp:61-63).
struct GenusAxisFieldNames {
    char const* names[kGenusAxisFieldCount] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
};

/// GenusStatFieldNames -- die Namen der flachen Gattungs-Zaehler-Spalten. nullptr = Spalte ungenutzt.
struct GenusStatFieldNames {
    char const* names[kGenusStatFieldCount] = {nullptr, nullptr, nullptr, nullptr};
};

/// genus_count_filled_axes() -- Zahl der Achsen-Slots mit MINDESTENS EINEM benannten Schema-Feld, aus der
/// Schema-Tabelle ABGELEITET statt hartkodiert (Praezedenz v3_count_filled_axes, observable_tier.hpp:110-116).
/// So zieht die Diagnose automatisch mit, sobald ein bisher stummer Slot seine Schema-Zeile bekommt.
template <std::size_t N>
[[nodiscard]] constexpr std::size_t genus_count_filled_axes(GenusAxisFieldNames const (&schema)[N]) noexcept {
    std::size_t n = 0;
    for (std::size_t t = 0; t < N; ++t) {
        if (schema[t].names[0] != nullptr) ++n;
    }
    return n;
}

// ---------------------------------------------------------------------------
// GenusObserverAggregate<G, N> -- das Layout
// ---------------------------------------------------------------------------

/// GenusObserverAggregate<G, N> -- die per-Achsen-Wire-Form EINER Container-Gattungs-Ebene.
///
/// G = das Genus-Tag (Ebene 2). Es macht die vier Formen zu VERSCHIEDENEN Typen, obwohl sie dasselbe
/// Layout tragen -- ein Set-Aggregat kann so nicht versehentlich an einer Sequence-Flaeche landen (die
/// Cross-Genus-Unmoeglichkeit set_abi_adapter.hpp:19-21 gilt auch fuer die Mess-Flaeche).
/// N = die LIVE Slot-Zahl des Genus (13/9/11/5). Sie ist Template-Parameter und KEIN Laufzeit-Wert:
/// die Kante des Feldes IST die Slot-Zahl, damit ein Slot-Schnitt compile-hart auffaellt.
///
/// LAYOUT (alle Member 8 Byte breit, kein Padding):
///   axis_stats[N][8]  N*64 Byte   Per-Achsen-Observer-Felder (Schema = die genus-eigene Tabelle)
///   seg_ns[N]         N*8  Byte   E13: Per-Slot-Timing der Gattung (ns; negativ = nicht erhoben)
///   genus_stats[4]    32   Byte   flache Gattungs-Zaehler (schema-benannt)
///   Meta              40   Byte   5 Felder, s. u.
template <AnatomyGenus G, std::size_t N>
struct GenusObserverAggregate {
    static_assert(N > 0, "Eine Gattungs-Ebene ohne Achsen-Slot gibt es nicht");

    /// Das Genus-Tag dieser Form (compile-time; quert die Grenze NICHT als Feld -- es IST der Typ).
    static constexpr AnatomyGenus genus = G;
    /// Die Slot-Zahl als Typ-Eigenschaft (Kreuz-Wache gegen Composition::slot_count in den Test-TUs).
    static constexpr std::size_t slot_count = N;

    /// Per-Achsen-Observer-Felder. axis_stats[T][f] == der Wert der Spalte, die die genus-eigene
    /// Schema-Tabelle an (T, f) benennt. Reihenfolge T == axis_names()-Reihenfolge des Genus
    /// (GenusBindingTraits<G>::axis_names(); Kreuz-Wache in den Test-TUs, hier waere sie ein Include-Zyklus).
    std::uint64_t axis_stats[N][kGenusAxisFieldCount] = {};

    /// E13: Per-Slot-Timing entlang der Aufrufpfade der Gattung (Owner-KERN Programm-Messbaeume,
    /// LEDGER:3813/:3828). Analogon zu ComdareTierObserverSnapshot::seg_ns (observable_tier.hpp:134);
    /// int64 wie dort, damit "nicht erhoben" (negativ) von "gemessen 0" (0) unterscheidbar bleibt.
    std::int64_t seg_ns[N] = {};

    /// Flache Gattungs-Zaehler, die NICHT an einer Achse haengen (Schema = die genus-eigene Tabelle).
    std::uint64_t genus_stats[kGenusStatFieldCount] = {};

    // -- Meta ----------------------------------------------------------------
    /// # Achsen-Slots, die real einen Snapshot liefern (in-process erhoben, ehrlich; R5.B-Grenze).
    /// Diese Spalte HEILT zugleich die Adapter-Asymmetrie: AdapterObserverSnapshotV1 war der einzige
    /// V1-POD ohne observable_axis_count (adapter_tier.hpp:26-39, Katalog Sektion 2 Punkt 5).
    std::uint64_t observable_axis_count = 0;
    /// Slot-Zahl, aus der LIVE Composition::slot_count abgeleitet -- NICHT aus einer frozen Konstanten
    /// (Katalog Sektion 2 Punkt 6; Negativ-Praezedenz kAdapterCompositionSlotCount == 13 vs. live 11).
    /// Muss zur Typ-Kante N passen; die Wache dafuer steht in den Test-TUs und im ABI-Adapter.
    std::uint64_t total_slots = 0;
    /// # Achsen mit mindestens einem benannten Schema-Feld (aus der Schema-Tabelle abgeleitet).
    std::uint64_t filled_axis_count = 0;
    /// E13-Coverage: benannter Rest des Segment-Laufs (Praezedenz P-MD3, observable_tier.hpp:145).
    std::int64_t seg_framework_ns = 0;
    /// E13-Coverage: aeussere Wall-Clock des Segment-Laufs (Nenner; Summe seg_ns + framework == total).
    std::int64_t seg_run_total_ns = 0;

    [[nodiscard]] constexpr bool operator==(GenusObserverAggregate const&) const noexcept = default;

    /// Erwartete Groesse in Byte, aus den Kanten GERECHNET (die Test-TUs pinnen sizeof gegen DIESEN
    /// Ausdruck UND gegen die literale Zahl -- rechnet jemand die Kante um, faellt beides zusammen auf).
    [[nodiscard]] static constexpr std::size_t expected_size_bytes() noexcept {
        return (N * kGenusAxisFieldCount * sizeof(std::uint64_t)) // axis_stats
               + (N * sizeof(std::int64_t))                       // seg_ns
               + (kGenusStatFieldCount * sizeof(std::uint64_t))   // genus_stats
               + (5 * sizeof(std::uint64_t));                     // Meta (3 uint64 + 2 int64)
    }
};

} // namespace comdare::cache_engine::anatomy
