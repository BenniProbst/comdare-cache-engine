// SPDX-License-Identifier: Apache-2.0
// V41.E11 Skeleton (2026-05-25) — Abstract-Factory-Slot fuer Prueflinge.
//
// User-Direktive: "PermutationsModul nimmt einen Pruefling wie den prt-art
// als abstract factory pattern zusaetzlich zu den eigenen Achsen auf und
// damit HAT die cache-engine per cmake konfigurierte Pruefling(e)."
//
// Status: API-Skelett. Implementation Phase 7+.

#pragma once

#include <cstddef>
#include <memory>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::api {

// Eine Pruefling-Instanz (z.B. prt-art-Algorithmus mit Achsen-Konfiguration).
class IPruefling {
public:
    virtual ~IPruefling() = default;

    [[nodiscard]] virtual std::string_view name() const          = 0;  // z.B. "prt-art"
    [[nodiscard]] virtual std::string_view version() const       = 0;  // z.B. "0.1.0"
    [[nodiscard]] virtual std::string_view axes_signature() const = 0;  // z.B. "node=compact,pc=lazy,..."

    // Hot-Path: pro Run die Operation laufen lassen.
    // Returns 0 = ok, sonst Fehler. micros_per_op = Mikrosekunden pro Operation.
    [[nodiscard]] virtual int run(std::size_t n_ops, double& out_micros_per_op) = 0;
};

// Factory: erzeugt eine Pruefling-Instanz fuer eine konkrete Achsen-Konfiguration.
class IPrueflingFactory {
public:
    virtual ~IPrueflingFactory() = default;

    [[nodiscard]] virtual std::string_view pruefling_name() const = 0;

    // Liste aller Achsen-Kombinationen die dieser Pruefling unterstuetzt.
    // (Pro Eintrag eine konkrete Permutation, z.B. "compact|lazy|binary|leaf_only".)
    [[nodiscard]] virtual std::vector<std::string_view> available_axes_combinations() const = 0;

    // Instanziiert einen Pruefling fuer eine konkrete Achsen-Kombination.
    [[nodiscard]] virtual std::unique_ptr<IPruefling> create(std::string_view axes) = 0;
};

// Registry: cache-engine fragt zur Laufzeit alle registrierten Prueflinge ab.
// Konkrete Implementations registrieren sich beim Bootstrap (CMake-konfiguriert).
class IPrueflingRegistry {
public:
    virtual ~IPrueflingRegistry() = default;

    virtual void register_factory(std::unique_ptr<IPrueflingFactory> factory) = 0;
    [[nodiscard]] virtual std::vector<IPrueflingFactory*> all_factories() = 0;
    [[nodiscard]] virtual IPrueflingFactory* find(std::string_view pruefling_name) = 0;
};

[[nodiscard]] IPrueflingRegistry& get_pruefling_registry();

}  // namespace comdare::cache_engine::api
