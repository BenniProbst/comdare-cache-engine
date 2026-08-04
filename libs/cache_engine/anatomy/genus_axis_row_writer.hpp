#pragma once
// E-24 C6 (2026-08-04, b-Teil / ABI-EREIGNIS-SERIE) -- write_genus_axis_row(): der EINE Schreiber, der
// EINEN Achsen-Snapshot in EINE 8-Spalten-Zeile einer Gattungs-Wire-Form (GenusObserverAggregate<G, N>)
// projiziert.
//
// WARUM EIN SCHREIBER STATT VIER: die Achsen-THEMEN sind gattungs-uebergreifend dieselben Bausteine
// (Owner-KERN NACHTRAG 5, LEDGER:3838: "Die Achsen sind also die Bausteine, die unter der Haupt-
// Implementierung der Genus und natuerlich auch cross Gattung und cross Genus [...] gebaut werden").
// Ein Snapshot-Typ hat deshalb GENAU EINE Spalten-Belegung -- unabhaengig davon, in welcher Gattung sein
// Slot sitzt. Vier Schreiber waeren vier Gelegenheiten, dieselbe Achse verschieden zu verspalten.
//
// ERKENNUNG UEBER DIE REALEN MEMBER, NICHT UEBER DIE SLOT-POSITION (Muster fill_observer_v3,
// abi_adapter.hpp: `if constexpr (requires { a.bytes_allocated; ... })`). Das haelt den Schreiber frei
// von Includes der Achsen-Header und damit frei von Include-Zyklen; und es ist die einzige Form, die
// bei einer CROSS-GENUS-Belegung eines Slots (ObservableOrgan-Huelle, organ_concept.hpp) noch traegt.
//
// E1 -- SKALIERUNGS-KONVENTION double -> uint64 (Manager-Entscheid, LEDGER:3828): MILLI-FIXPUNKT (x1000,
// Suffix _milli), Praezedenz avg_density_milli/frag_milli (measurement_snapshot.hpp:54-55). Der Gattungs-
// Wire darf den SA-double-DROP (observable_tier.hpp:66-68 laesst die double-Felder schlicht weg) NICHT
// wiederholen: AllocationStatistics traegt zwei double-Felder (external/internal_fragmentation,
// axis_06_allocator_cache_engine_permutation_concept.hpp:61-62), und genau die wuerden sonst im
// Gattungs-Wire fehlen. Sie werden hier als _milli-uint64 gefuehrt -- der Wert geht nicht verloren,
// er wechselt die Darstellung.
//
// D2-DOKTRIN (leere Spalte MIT Grund, nie stille 0): der Rueckgabewert ist die Zahl der REAL
// geschriebenen Spalten. 0 heisst "fuer diesen Snapshot-Typ existiert KEINE Spalten-Belegung" -- der
// Aufrufer zaehlt die Zeile dann NICHT in filled_axis_count, statt eine genullte Zeile als Messwert
// auszugeben. Das Concept GenusAxisRowWritable<S> macht dieselbe Aussage compile-time pruefbar.
//
// ABGRENZUNG (deklarierte Luecke, kein stiller Rest): abgedeckt sind die FUENF C6-V-Achsen-Formen
// (Katalog Sektion 2 Punkt 2) + AllocationStatistics (die vom Katalog Punkt 3 woertlich benannte
// double-Naht). Andere Achsen-Statistics-Formen liefern 0 und bleiben cross-boundary stumm -- ihre
// Wire-Anbindung ist A8-S3-Gegenstand (SA-Member-Nachruestung, Katalog Sektion 4), NICHT C6. Die
// in-process-Sicht observe_axes() traegt sie schon heute vollstaendig.

#include "genus_observer_aggregate.hpp" // kGenusAxisFieldCount + die Wire-Form
#include "observer_aggregate.hpp"       // EmptyAxisSnapshot (der Fallback-Typ ohne statistics())

#include <cstdint>
#include <type_traits>

namespace comdare::cache_engine::anatomy {

/// to_axis_milli() -- E1-Fixpunkt: ein double-Anteil wird als Tausendstel gefuehrt (x1000, kaufmaennisch
/// gerundet). Negatives und NaN werden zu 0 -- eine negative Fragmentierung gibt es nicht, und ein NaN
/// waere auf dem Draht ein Phantom-Wert. Beides ist eine EHRLICHE 0 (der Wert war nicht bestimmbar),
/// keine stille Kuerzung.
[[nodiscard]] constexpr std::uint64_t to_axis_milli(double v) noexcept {
    if (!(v > 0.0)) return 0; // faengt 0, negativ UND NaN in EINER Bedingung
    return static_cast<std::uint64_t>((v * 1000.0) + 0.5);
}

// ---------------------------------------------------------------------------
// Die Spalten-Belegungen (je Snapshot-Form eine; Erkennung strukturell)
// ---------------------------------------------------------------------------

/// GenusAxisRowWritable<S> -- traegt der Schreiber fuer diese Snapshot-Form eine Belegung?
/// Compile-time-Aussage fuer die Test-TUs: was hier false ist, bleibt cross-boundary DEKLARIERT stumm.
template <class S>
concept GenusAxisRowWritable = requires(S const& s) {
    s.growth_events;
    s.peak_slack;
} || requires(S const& s) {
    s.substrate_ops;
    s.underflow_count;
} || requires(S const& s) {
    s.bounds_checks_performed;
    s.rebind_count;
} || requires(S const& s) {
    s.index_translations;
    s.max_offset_jump;
} || requires(S const& s) {
    s.access_count;
    s.unaligned_accesses;
} || requires(S const& s) {
    s.total_bytes_allocated;
    s.external_fragmentation;
};

/// write_genus_axis_row() -- projiziert EINEN Achsen-Snapshot in EINE Zeile. Rueckgabe = Zahl der real
/// geschriebenen Spalten (0 == keine Belegung, s. D2-Doktrin im Kopf).
///
/// Die Reihenfolge der Zweige ist DISJUNKT gewaehlt (je zwei Member, die nur EINE Form gemeinsam hat) --
/// eine spaetere Achse mit ueberlappender Member-Menge waere ein Schema-Konflikt und faellt in der
/// Test-TU auf, nicht still im Feld.
template <class S>
constexpr std::size_t write_genus_axis_row(std::uint64_t (&row)[kGenusAxisFieldCount], S const& s) noexcept {
    if constexpr (std::is_same_v<S, EmptyAxisSnapshot>) {
        // Ein getragener Slot (CarriedAxis / Achse ohne statistics()) ist KEIN Messwert. Die Zeile bleibt
        // 0 und wird NICHT als befuellt gemeldet -- genau die Unterscheidung 'ohne' vs. 'leer' aus dem
        // Owner-KERN NACHTRAG 3 (b) (LEDGER:3814, Katalog 1N.1).
        (void)row;
        (void)s;
        return 0;
    } else if constexpr (requires {
                             s.growth_events;
                             s.peak_slack;
                         }) {
        // GrowthStatistics (growth_policy_observable.hpp:42-52) -- 7 Felder. Spalte [7] bleibt dem
        // E1-Detail-Entscheid growth_factor_milli vorbehalten; sie schreibt der Gattungs-Adapter, weil
        // der Wert an der HUELLE haengt und nicht im Snapshot steht (s. sequence_tier_v2.hpp).
        row[0] = s.growth_events;
        row[1] = s.elements_copied;
        row[2] = s.bytes_copied;
        row[3] = s.requested_total;
        row[4] = s.granted_total;
        row[5] = s.final_capacity;
        row[6] = s.peak_slack;
        return 7;
    } else if constexpr (requires {
                             s.substrate_ops;
                             s.underflow_count;
                         }) {
        // InnerContainerStatistics (inner_container_observable.hpp:43-54) -- 8 Felder, Vollbelegung.
        row[0] = s.push_count;
        row[1] = s.pop_count;
        row[2] = s.front_reads;
        row[3] = s.back_reads;
        row[4] = s.current_occupancy;
        row[5] = s.peak_occupancy;
        row[6] = s.underflow_count;
        row[7] = s.substrate_ops;
        return 8;
    } else if constexpr (requires {
                             s.bounds_checks_performed;
                             s.rebind_count;
                         }) {
        // ExtentStatistics (view_policies_observable.hpp:38-44) -- 3 Felder. bounds_checks_performed == 0
        // bei statischer Ausdehnung IST der on-wire-Elisions-Beweis (Katalog C-B) und deshalb ein
        // geschriebener Wert, keine ausgelassene Spalte.
        row[0] = s.bounds_checks_performed;
        row[1] = s.oob_rejects;
        row[2] = s.rebind_count;
        return 3;
    } else if constexpr (requires {
                             s.index_translations;
                             s.max_offset_jump;
                         }) {
        // LayoutStatistics (view_policies_observable.hpp:136-142) -- 3 Felder.
        row[0] = s.index_translations;
        row[1] = s.non_contiguous_steps;
        row[2] = s.max_offset_jump;
        return 3;
    } else if constexpr (requires {
                             s.access_count;
                             s.unaligned_accesses;
                         }) {
        // AccessorStatistics (view_policies_observable.hpp:198-204) -- 3 Felder.
        row[0] = s.access_count;
        row[1] = s.unaligned_accesses;
        row[2] = s.conversion_ops;
        return 3;
    } else if constexpr (requires {
                             s.total_bytes_allocated;
                             s.external_fragmentation;
                         }) {
        // AllocationStatistics (axis_06_allocator_cache_engine_permutation_concept.hpp:55-63) -- 5 uint64
        // + die BEIDEN double-Felder als E1-Milli. Das ist die vom Katalog Punkt 3 woertlich verlangte
        // Schliessung des SA-double-Drops auf der Gattungs-Seite.
        row[0] = s.total_bytes_allocated;
        row[1] = s.total_bytes_in_use;
        row[2] = s.allocation_count;
        row[3] = s.deallocation_count;
        row[4] = s.failure_count;
        row[5] = to_axis_milli(s.external_fragmentation);
        row[6] = to_axis_milli(s.internal_fragmentation);
        return 7;
    } else {
        // DEKLARIERTE Luecke (s. Kopf-Abgrenzung): keine Belegung -> Zeile bleibt 0 UND wird nicht als
        // befuellt gemeldet. Der Aufrufer darf daraus keinen Messwert machen.
        (void)row;
        (void)s;
        return 0;
    }
}

} // namespace comdare::cache_engine::anatomy
