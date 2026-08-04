#pragma once
// V5-I2 (2026-05-31) — IDriveableTier: funktionaler Gattungs-Antrieb (IMMER einkompiliert, ABI-stabil).
//
// Doku: docs/architecture/messarchitektur_v5_design.md §5.2/§8.1 (Interface-Split), §7 (compile-time-Disjunktheit).
//
// Trennung der Belange (User-Direktive „Anatomy-ABI ⊥ Observer-Tier-Schnittstelle"):
//   - IDriveableTier  = der reine FUNKTIONALE Gattungs-Antrieb (tier_insert/lookup/erase/clear/size, uint64-Key/Value).
//                       IMMER in jeder Tier-Binary einkompiliert — auch in der Release-/funktional-only-DLL ohne Messung.
//                       Über diese Schnittstelle treibt der host-seitige Prüfdock (abstract-factory-ähnlich) jede geladene
//                       Binary generisch + fährt das std::map-Konformitäts-Gate (§6), unabhängig von der Mess-Ausstattung.
//   - IObservableTier = NUR der Observer-Zugriff (tier_observe → Snapshot-POD); erbt IDriveableTier, wird aber NUR bei
//                       Messung-AN (COMDARE_MEASUREMENT_ON) vom ABI-Adapter vererbt/exportiert (observable_tier.hpp).
//   - IRollbackableTier (rollbackable_tier.hpp, V5-I6) = memento_all; ebenfalls NUR bei Messung-AN.
//
// ABI-SICHER nach demselben Designprinzip wie measurable_workload.hpp / observable_tier.hpp: ein eigenständiges
// Sub-Interface, das der genus-typisierte ABI-Adapter erbt; der Host fragt es via dynamic_cast<IDriveableTier*> ab.
// Der Antriebs-Teil quert die Grenze als reine vtable (uint64-Parameter, kein STL/POD-by-value) → ABI-stabil.

#include <cstdint>

namespace comdare::cache_engine::anatomy {

/// IDriveableTier -- DER DE-FACTO KERN DER MAP-GATTUNG (E-24 C7-4, benannt statt nur beschrieben).
///
/// EINORDNUNG NACH DEM FINALEN ZWEI-EBENEN-MODELL (Owner-KERN NACHTRAG 4, LEDGER:3836; C7-Auflage C7-4,
/// LEDGER:3843): Ebene 1 sind die GATTUNGEN Map / Container / Graph, Ebene 2 die GENERA. Die Map-Gattung
/// hat GENAU EIN Genus (SearchAlgorithm). Eine Schnittmenge ueber ein Element IST das Element -- deshalb
/// braucht die Map-Gattung KEIN eigenes Kopf-Framework (Manager-Entscheid, LEDGER:3844), sondern eine
/// BENENNUNG: dieses Interface, mit tier_insert(k,v)/tier_lookup/tier_erase/tier_clear/tier_size, IST der
/// Map-Gattungs-Kern. Seine <Key,Value>-Instanz ist das Typ-Paar key_type/value_type der SA-Anatomie
/// (das Gegenstueck zum Container-Gattungs-Typ-Vertrag element_type, C7-3).
/// ABGRENZUNG (deklariert, nicht still): ob die MESS-Flaeche (IObservableTier + die SA-Zusatz-
/// Subinterfaces) zum Map-Gattungs-Kern gehoert, ist NICHT kartiert -- offener Entscheid des
/// Diskrepanz-Dossiers (Abschnitt 7 Punkt 2), NICHT in diesem Fenster beantwortet.
///
/// Der funktionale Gattungs-Antrieb einer geladenen Tier-Anatomie (Pfad B, immer vorhanden).
///
/// Gemeinsamer uint64-Key/Value-Raum nach Umstufung-B (alle sezierten Such-Organe einheitlich uint64). Der Host
/// (CacheEngineBuilder/Prüfdock) treibt darüber generisch jede geladene Binary — für das Konformitäts-Gate gegen
/// std::map UND für die eigentliche Last (Lastenprofil). KEIN Observer, KEIN Memento — rein funktional.
class IDriveableTier {
public:
    virtual ~IDriveableTier() = default;

    /// Fügt (key,value) ein bzw. aktualisiert. Rückgabe: true wenn ein NEUER Schlüssel entstand.
    [[nodiscard]] virtual bool tier_insert(std::uint64_t key, std::uint64_t value) noexcept = 0;

    /// Sucht key. Bei Treffer wird *out_value gesetzt (falls out_value != nullptr) und true geliefert.
    [[nodiscard]] virtual bool tier_lookup(std::uint64_t key, std::uint64_t* out_value) const noexcept = 0;

    /// Entfernt key. Rückgabe: true wenn key vorhanden war.
    [[nodiscard]] virtual bool tier_erase(std::uint64_t key) noexcept = 0;

    /// Leert den Tier-Zustand (Schlüssel; Statistik-Reset ist separat über reset()).
    virtual void tier_clear() noexcept = 0;

    /// Logische Schlüsselzahl (Füllstand).
    [[nodiscard]] virtual std::uint64_t tier_size() const noexcept = 0;
};

} // namespace comdare::cache_engine::anatomy
