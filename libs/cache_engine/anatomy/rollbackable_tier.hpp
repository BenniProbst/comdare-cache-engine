#pragma once
// V5-I6 — IRollbackableTier: das ABI-stabile memento_all-Sub-Interface (Tier-Binary-Seite).
//
// User-Direktive 2026-05-31: memento_all existiert PARALLEL zu observer_all, NUR bei Messung-AN einkompiliert
// (reine Compile-Time-Metaprogrammierung). Über dieses Sub-Interface triggert der host-seitige Zwei-Phasen-
// Treiber (V5-I7) die Rollback-Mechanik der geladenen Tier-Binary:
//
//   pro Op:  tier_save_all()  →  op (warmup, kalt)  →  tier_rollback_all()  →  op (measure)
//
// Der Zustand lebt IN der Binary (search_organ_/ComposedStore/Disk-Files); der Host sieht den Memento NIE —
// er steuert nur save/rollback. (G5-Audit w289llo0o: das frühere Etikett „hybrider Visitor / die Memento-Klassen
// besuchen den Host" ist NICHT umgesetzt und gestrichen — es gibt KEIN accept()/HostSink; real ist dies ein reines
// Memento über zwei void-vtable-Methoden, der Host ruft save/rollback, kein Element-Besuch.) „Einfacher Snapshot
// reicht NICHT" → memento_all kapselt die im abi_adapter real gedeckten stateful Achsen (search_organ_ + container_,
// inkl. IO/Disk-Persistenz). Realisiert im abi_adapter über Copy-on-Write Rev.2 (cow_materialize_copy_) + den per-Achsen-
// MementoAxis-Fallback (save_axis/restore_axis, memento_aggregate.hpp, V5-I5). (K10-PMAJOR-02, 2026-06-18:
// der frühere 19-Slot-Aggregat-Halter MementoAggregate war toter Rev.1-Apparat und wurde entfernt — der
// kanonische Pfad lief nie über ihn.)
//
// ABI-SICHER nach demselben Designprinzip wie IDriveableTier/IObservableTier/IMeasurableWorkload: eigenständiges
// Sub-Interface, das der ABI-Adapter NUR bei COMDARE_MEASUREMENT_ON zusätzlich erbt; in der Release-/funktional-
// only-DLL ist es compile-time restlos entfernt (kein vtable-Slot, kein Overhead). Quert die Grenze als reine
// vtable (void-Methoden, keine POD-by-value) → ABI-stabil; der Memento selbst quert NICHT.
//
// @doku docs/architecture/messarchitektur_v5_design.md §3.4 (dort als „hybrider Visitor" betitelt, aber NICHT
//       umgesetzt — G5-Audit w289llo0o; real = Memento) + §4 (Zwei-Phasen-Op-Schleife)

namespace comdare::cache_engine::anatomy {

/// IRollbackableTier — optionales memento_all-Sub-Interface einer geladenen Tier-Anatomie (nur Messung-AN).
///
/// Der Host treibt damit die Zwei-Phasen-Messung: tier_save_all() kapselt den im abi_adapter REAL gedeckten
/// Tier-Zustand — die beiden materialisierten stateful Träger search_organ_ + container_ (CoW Rev.2) plus den
/// per-Achsen-MementoAxis-Fallback (NICHT pauschal „alle" Achsen/„inkl. Disk": disk-persistierende Organe sind
/// nur abgedeckt, soweit sie über search_organ_/container_ bzw. ihren MementoAxis materialisiert sind — G5-Audit
/// w289llo0o); tier_rollback_all() rollt diesen Stand nach dem Warmup-Op exakt zurück, sodass der gemessene Op
/// gegen DENSELBEN Vor-Zustand läuft (eliminiert Pfad-Abhängigkeit der Latenz).
class IRollbackableTier {
public:
    virtual ~IRollbackableTier() = default;

    /// Kapselt den real gedeckten stateful Zustand (search_organ_ + container_ via CoW Rev.2, plus per-Achsen-
    /// MementoAxis-Fallback) in den binary-internen Memento — NICHT pauschal „alle Achsen + Disk" (G5-Audit w289llo0o).
    /// noexcept: reines Kapseln; eine OOM/IO-Störung muss intern abgefangen werden (Mess-Robustheit).
    virtual void tier_save_all() noexcept = 0;

    /// Rollt den gedeckten Zustand exakt auf den letzten tier_save_all()-Stand zurück (Disk nur soweit über
    /// search_organ_/container_ bzw. MementoAxis materialisiert — kein pauschaler Disk-Flush/Checkpoint, G5-Audit).
    /// noexcept. Idempotent bzgl. mehrfachem Aufruf nach EINEM save (rollt stets auf denselben Stand).
    virtual void tier_rollback_all() noexcept = 0;
};

} // namespace comdare::cache_engine::anatomy
