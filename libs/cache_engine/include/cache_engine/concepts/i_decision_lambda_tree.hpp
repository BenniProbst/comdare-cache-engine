#pragma once
// IDecisionLambdaTree — F-EXTRA-6 PRO Baustein
// Termin 7 / 02_uml_cache_engine §2

#include <cache_engine/concepts/event.hpp>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace comdare::cache_engine {

// Decision-Enum (per Lambda-Tree evaluate-Resultat)
enum class Decision : std::uint8_t {
    EXECUTE = 0,
    DELAY   = 1,
    SKIP    = 2,
};

// Kontext fuer evaluate-Aufruf — gemeinsamer State, von der CacheEngine bereitgestellt
struct DecisionContext {
    double        cost_threshold           = 0.0;
    std::uint64_t recent_relocations_count = 0;
    std::uint64_t epoch                    = 0;
    void*         user_payload             = nullptr;
};

// Konfigurations-Knoten fuer Lambda-Tree (NodeTreeConfig)
struct NodeTreeConfig {
    std::vector<std::string> lambda_node_names;
    double                   cost_threshold   = 0.0;
    std::uint64_t            max_per_interval = 0;
};

struct NodeTreeState {
    std::uint64_t total_evaluations = 0;
    std::uint64_t total_executes    = 0;
    std::uint64_t total_delays      = 0;
    std::uint64_t total_skips       = 0;
};

// Wurzel-Concept (template-Form fuer EventT)
template <typename EventT>
class IDecisionLambdaTree {
public:
    virtual ~IDecisionLambdaTree() = default;

    [[nodiscard]] virtual Decision evaluate(EventT const& event, DecisionContext const& ctx) const noexcept = 0;

    virtual void configure(NodeTreeConfig const& config) = 0;

    [[nodiscard]] virtual NodeTreeState save_state() const noexcept                        = 0;
    virtual void                        restore_state(NodeTreeState const& state) noexcept = 0;
};

} // namespace comdare::cache_engine
