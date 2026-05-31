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
// er steuert nur save/rollback (hybrider Visitor: die einkompilierten Memento-Klassen besuchen den Host zur
// Steuerung). „Einfacher Snapshot reicht NICHT" → memento_all kapselt ALLE stateful Achsen (inkl. IO/Disk-
// Persistenz) via MementoAggregate + save_axis/restore_axis (memento_aggregate.hpp, V5-I5).
//
// ABI-SICHER nach demselben Designprinzip wie IDriveableTier/IObservableTier/IMeasurableWorkload: eigenständiges
// Sub-Interface, das der ABI-Adapter NUR bei COMDARE_MEASUREMENT_ON zusätzlich erbt; in der Release-/funktional-
// only-DLL ist es compile-time restlos entfernt (kein vtable-Slot, kein Overhead). Quert die Grenze als reine
// vtable (void-Methoden, keine POD-by-value) → ABI-stabil; der Memento selbst quert NICHT.
//
// @doku docs/architecture/messarchitektur_v5_design.md §3.4 (hybrider Visitor) + §4 (Zwei-Phasen-Op-Schleife)

namespace comdare::cache_engine::anatomy {

/// IRollbackableTier — optionales memento_all-Sub-Interface einer geladenen Tier-Anatomie (nur Messung-AN).
///
/// Der Host treibt damit die Zwei-Phasen-Messung: tier_save_all() kapselt den GESAMTEN Tier-Zustand über alle
/// stateful Achsen (Warmup-Vor-Zustand); tier_rollback_all() rollt ihn nach dem Warmup-Op exakt zurück, sodass
/// der gemessene Op gegen DENSELBEN Vor-Zustand läuft (eliminiert Pfad-Abhängigkeit der Latenz).
class IRollbackableTier {
public:
    virtual ~IRollbackableTier() = default;

    /// Kapselt den aktuellen Gesamt-Zustand (alle stateful Achsen + ggf. Disk) in den binary-internen Memento.
    /// noexcept: reines Kapseln; eine OOM/IO-Störung muss intern abgefangen werden (Mess-Robustheit).
    virtual void tier_save_all() noexcept = 0;

    /// Rollt den Gesamt-Zustand exakt auf den letzten tier_save_all()-Stand zurück (inkl. Disk-Flush/Checkpoint).
    /// noexcept. Idempotent bzgl. mehrfachem Aufruf nach EINEM save (rollt stets auf denselben Stand).
    virtual void tier_rollback_all() noexcept = 0;
};

}  // namespace comdare::cache_engine::anatomy
