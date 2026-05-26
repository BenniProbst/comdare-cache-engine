#pragma once
// V41.F.6.1.R5.C.A3 — MammalAbiAdapter Production-Header (Module-Loader-Bruecke)
//
// User-Direktive 2026-05-27 frueh (Session-Doku Teil II §18, NEXT-Action R5.C.A3):
// "AnatomyAbiAdapter Template von Test §5 in eigenen Production-Header verschieben
//  (anatomy/abi_adapter.hpp). Wird in R5.E fuer Module-Factory benoetigt."
//
// Architektur-Rolle:
// `MammalAbiAdapter<A>` bridge eine konkrete `SearchAlgorithmAnatomy<Composition>`
// (Mammal-Gattung, Compile-Time-Concept) zur Runtime-ABI-Schicht (`IAnatomyBase`
// → `IExecutionEngine`). Ein generierter Permutations-Binary (.so/.dll) wird in
// R5.D/R5.E exakt EINEN solchen Adapter via `extern "C"` Factory exportieren.
//
// Bisheriger Stand: Adapter-Pattern war in tests/ lokal dupliziert in zwei Varianten:
//   - test_v41_anatomy_base.cpp:110 AnatomyAbiAdapter (unvollstaendig, fehlte
//     Lifecycle-Override — broken seit R5.C.A2 Wurzel-Inheritance)
//   - test_v41_execution_engine.cpp:115 MammalAbiAdapter (vollstaendig, Vorlage)
//
// R5.C.A3 promoviert die vollstaendige Variante zum Production-Header und ersetzt
// beide lokalen Test-Klassen.
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §35.3 + Session-Doku Teil II §18
// @task #702 V41.F.6.1.R5.C.A3
// @related [[execution-engine-als-wurzel]] [[anatomie-gattungen]]
//          [[anatomie-nur-achsen-und-observer]]

#include "anatomy_base.hpp"
#include "../execution_engine/execution_engine_base.hpp"

#include <cstddef>
#include <string_view>

namespace comdare::cache_engine::anatomy {

// ─────────────────────────────────────────────────────────────────────────────
// SearchAlgorithmAbiAdapter — bridge AnatomyConcept-konformer SearchAlgorithm-
// Anatomie zu IAnatomyBase (Mammal-Gattung in Tier-Metapher, Doku 14 §27.2)
// ─────────────────────────────────────────────────────────────────────────────

/// SearchAlgorithmAbiAdapter<A> — generischer Runtime-ABI-Adapter fuer die
/// SearchAlgorithm-Gattung (Mammal in Tier-Metapher).
///
/// Vorbedingung: `A` erfuellt `AnatomyConcept` UND `A::genus() ==
/// AnatomyGenus::SearchAlgorithm`. Der `static_assert` im Klassen-Body validiert
/// die Gattung zur Compile-Zeit (Doku 14 §32 Gattungs-Constraint).
///
/// Verwendung (R5.E Module-Factory-Pattern):
/// ```cpp
/// // In generiertem Permutations-Binary (.so/.dll):
/// extern "C" comdare::cache_engine::anatomy::IAnatomyBase*
/// comdare_create_anatomy() {
///     using A = comdare::cache_engine::anatomy::SearchAlgorithmAnatomy<MyComposition>;
///     return new comdare::cache_engine::anatomy::SearchAlgorithmAbiAdapter<A>{};
/// }
/// ```
///
/// Lifecycle-Implementierung: der interne `state_` reflektiert die Pflicht-
/// Phasen `Uninitialized → Warming → Running → Idle → Shutdown`. Pilot R5.C.A3
/// setzt state_ in den entsprechenden Hooks. Echte Cache-Preheat/Bulk-Load
/// kommt mit R5.D (CacheEngineBuilder Workload-Treiber).
template <AnatomyConcept A>
class SearchAlgorithmAbiAdapter final : public IAnatomyBase {
    static_assert(A::genus() == AnatomyGenus::SearchAlgorithm,
                  "SearchAlgorithmAbiAdapter erwartet eine SearchAlgorithm-Gattung-"
                  "Anatomie (AnatomyGenus::SearchAlgorithm). Cross-Genus-Adapter "
                  "sind type-system-mathematisch unmoeglich — Doku 14 §32.");

public:
    // ─────────────────────────────────────────────────────────────────────
    // IExecutionEngine-Pflicht-Override (R5.C.A2 Wurzel-Schicht)
    // ─────────────────────────────────────────────────────────────────────

    [[nodiscard]] std::string_view engine_name() const noexcept override {
        return A::composition_name();
    }

    [[nodiscard]] ::comdare::cache_engine::execution_engine::EngineLifecycleState
    lifecycle_state() const noexcept override {
        return state_;
    }

    void warm_up() override {
        state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Warming;
    }

    void reset() override {
        state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Idle;
    }

    void shutdown() override {
        state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Shutdown;
    }

    // ─────────────────────────────────────────────────────────────────────
    // IAnatomyBase-Pflicht-Override (R5.C.A Anatomie-Schicht)
    // ─────────────────────────────────────────────────────────────────────

    [[nodiscard]] std::string_view composition_name() const noexcept override {
        return A::composition_name();
    }

    [[nodiscard]] std::string_view paper_id() const noexcept override {
        return A::paper_id();
    }

    [[nodiscard]] AnatomyGenus genus() const noexcept override {
        return A::genus();
    }

    [[nodiscard]] std::size_t organ_count() const noexcept override {
        return A::organ_count();
    }

private:
    ::comdare::cache_engine::execution_engine::EngineLifecycleState state_{
        ::comdare::cache_engine::execution_engine::EngineLifecycleState::Uninitialized};
};

}  // namespace comdare::cache_engine::anatomy
