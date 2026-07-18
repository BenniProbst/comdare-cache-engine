// V41.F.6.1.R5.C.A2 — ExecutionEngine Wurzel-Tests (Lebewesen vs Viren)
//
// Beweist:
// 1. ExecutionEngineKind enum hat 3 Werte (Anatomy/Virus/Hybrid)
// 2. engine_kind_name() liefert korrekte Strings
// 3. EngineLifecycleState enum hat 5 Phasen
// 4. ExecutionEngineConcept Conformance fuer Anatomie + Virus
// 5. IExecutionEngine ist abstract + has_virtual_destructor
// 6. IAnatomyBase erbt von IExecutionEngine (Lebewesen-Spezialisierung)
// 7. Sample VirusExecutionEngine (GraphBFS-Stub) als Beispiel Nicht-Lebewesen
// 8. AbiAdapter Pattern fuer Anatomy + Virus (Module-Loader R5.E Vorbereitung)
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §33-§40
// @task #701 V41.F.6.1.R5.C.A2

#include <gtest/gtest.h>

#include <execution_engine/execution_engine_base.hpp>
#include <anatomy/abi_adapter.hpp>
#include <anatomy/anatomy_base.hpp>
#include <anatomy/known_algorithms.hpp>

#include <type_traits>

namespace ee  = ::comdare::cache_engine::execution_engine;
namespace ana = ::comdare::cache_engine::anatomy;

// ─────────────────────────────────────────────────────────────────────────────
// §1 — ExecutionEngineKind + engine_kind_name() Helper
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5CA2_ExecutionEngineKind, EnumHasThreeValues) {
    static_assert(static_cast<int>(ee::ExecutionEngineKind::Anatomy) == 0);
    static_assert(static_cast<int>(ee::ExecutionEngineKind::Virus) == 1);
    static_assert(static_cast<int>(ee::ExecutionEngineKind::Hybrid) == 2);
    SUCCEED();
}

TEST(R5CA2_ExecutionEngineKind, KindNameHelperReturnsStrings) {
    static_assert(ee::engine_kind_name(ee::ExecutionEngineKind::Anatomy) == std::string_view{"Anatomy"});
    static_assert(ee::engine_kind_name(ee::ExecutionEngineKind::Virus) == std::string_view{"Virus"});
    static_assert(ee::engine_kind_name(ee::ExecutionEngineKind::Hybrid) == std::string_view{"Hybrid"});
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §2 — EngineLifecycleState (5 Phasen wie prt_art_legacy IExecutingEngine)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5CA2_EngineLifecycleState, EnumHasFivePhases) {
    static_assert(static_cast<int>(ee::EngineLifecycleState::Uninitialized) == 0);
    static_assert(static_cast<int>(ee::EngineLifecycleState::Warming) == 1);
    static_assert(static_cast<int>(ee::EngineLifecycleState::Running) == 2);
    static_assert(static_cast<int>(ee::EngineLifecycleState::Idle) == 3);
    static_assert(static_cast<int>(ee::EngineLifecycleState::Shutdown) == 4);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §3 — ExecutionEngineConcept Conformance (Compile-Time)
// ─────────────────────────────────────────────────────────────────────────────

// Sample Anatomie-Engine die Concept erfuellt
struct SampleAnatomyEngine {
    struct measurement_snapshot_t {};
    static constexpr std::string_view        engine_name() noexcept { return "SampleAnatomyEngine"; }
    static constexpr ee::ExecutionEngineKind engine_kind() noexcept { return ee::ExecutionEngineKind::Anatomy; }
};

// Sample Virus-Engine die Concept erfuellt (Beispiel Graph-BFS)
struct SampleGraphBfsEngine {
    struct measurement_snapshot_t {
        std::uint64_t nodes_visited{0};
        std::uint64_t edges_traversed{0};
    };
    static constexpr std::string_view        engine_name() noexcept { return "GraphBFS"; }
    static constexpr ee::ExecutionEngineKind engine_kind() noexcept { return ee::ExecutionEngineKind::Virus; }
};

// Counter-Beispiel: leere Klasse
struct NotAnEngine {};

TEST(R5CA2_ExecutionEngineConcept, AnatomyAndVirusBothConform) {
    static_assert(ee::ExecutionEngineConcept<SampleAnatomyEngine>);
    static_assert(ee::ExecutionEngineConcept<SampleGraphBfsEngine>);
    SUCCEED();
}

TEST(R5CA2_ExecutionEngineConcept, EmptyClassDoesNotConform) {
    static_assert(!ee::ExecutionEngineConcept<NotAnEngine>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §4 — IExecutionEngine Virtual Interface (Compile-Only-Verifikation)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5CA2_IExecutionEngine, IsAbstractClass) {
    static_assert(std::is_abstract_v<ee::IExecutionEngine>);
    static_assert(std::has_virtual_destructor_v<ee::IExecutionEngine>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §5 — IAnatomyBase erbt von IExecutionEngine (R5.C.A2 Wurzel-Update)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5CA2_AnatomyInheritance, IAnatomyBaseDerivesFromIExecutionEngine) {
    static_assert(std::is_base_of_v<ee::IExecutionEngine, ana::IAnatomyBase>);
    static_assert(std::is_abstract_v<ana::IAnatomyBase>);
    SUCCEED();
}

// R5.C.A3: Lokale Sample-Adapter-Klasse entfernt. Wir nutzen jetzt den
// Production-Header `anatomy/abi_adapter.hpp` mit `ana::SearchAlgorithmAbiAdapter<A>`.

TEST(R5CA2_AnatomyInheritance, SearchAlgorithmAdapterBridgeToBothInterfaces) {
    ana::SearchAlgorithmAbiAdapter<ana::Art> adapter;
    // Als IExecutionEngine
    ee::IExecutionEngine const& exec = adapter;
    EXPECT_EQ(exec.engine_name(), std::string_view{"ArtComposition"});
    EXPECT_EQ(exec.engine_kind(), ee::ExecutionEngineKind::Anatomy);
    EXPECT_EQ(exec.lifecycle_state(), ee::EngineLifecycleState::Uninitialized);

    // Als IAnatomyBase (Subtype-Polymorphismus)
    ana::IAnatomyBase const& anat = adapter;
    EXPECT_EQ(anat.composition_name(), std::string_view{"ArtComposition"});
    EXPECT_EQ(anat.genus(), ana::AnatomyGenus::SearchAlgorithm);
    EXPECT_EQ(anat.organ_count(), 17u);

    // Lifecycle-Transitionen (R5.C.A4: vollstaendiger Roundtrip mit run())
    adapter.warm_up();
    EXPECT_EQ(adapter.lifecycle_state(), ee::EngineLifecycleState::Warming);
    adapter.run();
    EXPECT_EQ(adapter.lifecycle_state(), ee::EngineLifecycleState::Running);
    adapter.reset();
    EXPECT_EQ(adapter.lifecycle_state(), ee::EngineLifecycleState::Idle);
    adapter.shutdown();
    EXPECT_EQ(adapter.lifecycle_state(), ee::EngineLifecycleState::Shutdown);
}

// ─────────────────────────────────────────────────────────────────────────────
// §6 — Sample Virus-ExecutionEngine (GraphBFS-Stub als Nicht-Lebewesen)
// ─────────────────────────────────────────────────────────────────────────────

/// IVirusExecutionEngine — Skelett-Interface fuer Viren (parallel zu IAnatomyBase).
/// Wird in R6/V42 in eigenen Header verschoben (virus/virus_execution_engine.hpp).
class IVirusExecutionEngine : public ee::IExecutionEngine {
public:
    [[nodiscard]] ee::ExecutionEngineKind  engine_kind() const noexcept final { return ee::ExecutionEngineKind::Virus; }
    [[nodiscard]] virtual std::string_view algorithm_family() const noexcept = 0;
    [[nodiscard]] virtual std::string_view algorithm_paper() const noexcept  = 0;
};

class GraphBfsVirusStub final : public IVirusExecutionEngine {
public:
    [[nodiscard]] std::string_view         engine_name() const noexcept override { return "GraphBFS"; }
    [[nodiscard]] ee::EngineLifecycleState lifecycle_state() const noexcept override { return state_; }
    void                                   warm_up() override { state_ = ee::EngineLifecycleState::Warming; }
    void                                   run() override { state_ = ee::EngineLifecycleState::Running; }
    void                                   reset() override { state_ = ee::EngineLifecycleState::Idle; }
    void                                   shutdown() override { state_ = ee::EngineLifecycleState::Shutdown; }

    [[nodiscard]] std::string_view algorithm_family() const noexcept override { return "GraphTraversal"; }
    [[nodiscard]] std::string_view algorithm_paper() const noexcept override {
        return "Moore 1959 — Shortest path through a maze";
    }

private:
    ee::EngineLifecycleState state_{ee::EngineLifecycleState::Uninitialized};
};

TEST(R5CA2_VirusEngine, GraphBfsStubAsVirusExecutionEngine) {
    GraphBfsVirusStub     bfs;
    ee::IExecutionEngine& exec = bfs;
    EXPECT_EQ(exec.engine_name(), std::string_view{"GraphBFS"});
    EXPECT_EQ(exec.engine_kind(), ee::ExecutionEngineKind::Virus); // NICHT Anatomy!
    // Virus hat keine Composition/Achsen, aber lifecycle Pflicht-API
    bfs.warm_up();
    EXPECT_EQ(bfs.lifecycle_state(), ee::EngineLifecycleState::Warming);
    // R5.C.A4: run() Hook auch fuer Virus pflicht
    bfs.run();
    EXPECT_EQ(bfs.lifecycle_state(), ee::EngineLifecycleState::Running);
    EXPECT_EQ(bfs.algorithm_family(), std::string_view{"GraphTraversal"});
}

TEST(R5CA2_VirusEngine, IVirusExecutionEngineDoesNotInheritIAnatomyBase) {
    static_assert(!std::is_base_of_v<ana::IAnatomyBase, IVirusExecutionEngine>);
    static_assert(std::is_base_of_v<ee::IExecutionEngine, IVirusExecutionEngine>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §7 — Polymorpher Mess-Loop (Anatomy + Virus uniform behandelbar)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5CA2_PolymorphicMeasurement, AnatomyAndVirusUniformLifecycle) {
    ana::SearchAlgorithmAbiAdapter<ana::Hot> anatomy;
    GraphBfsVirusStub                        virus;

    // Polymorpher Mess-Loop (CacheEngineBuilder-Pattern fuer R5.D)
    // R5.C.A4: vollstaendige Lifecycle-Sequenz mit run()
    ee::IExecutionEngine* engines[] = {&anatomy, &virus};
    for (auto* e : engines) {
        e->warm_up();
        EXPECT_EQ(e->lifecycle_state(), ee::EngineLifecycleState::Warming);
        e->run();
        EXPECT_EQ(e->lifecycle_state(), ee::EngineLifecycleState::Running);
        e->reset();
        EXPECT_EQ(e->lifecycle_state(), ee::EngineLifecycleState::Idle);
        e->shutdown();
        EXPECT_EQ(e->lifecycle_state(), ee::EngineLifecycleState::Shutdown);
    }

    // Type-discrimination via engine_kind() (Doku 14 §34.1 Drei-Ebenen-Taxonomie)
    EXPECT_EQ(anatomy.engine_kind(), ee::ExecutionEngineKind::Anatomy);
    EXPECT_EQ(virus.engine_kind(), ee::ExecutionEngineKind::Virus);
}
