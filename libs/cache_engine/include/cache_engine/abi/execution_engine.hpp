#pragma once
// execution_engine - Stufe 2 der Drei-Schichten-Hierarchie (REV 7 §4.2(a)+(f))
//
// erbt von CacheEngine (Visitor-Pattern Basisklasse).
// STELLT mithilfe der CacheEngine experimentelle OS-Primitiven BEREIT
// (Provider-Rolle): cache-line-aligned-Alloc, NUMA-konformes Read-Pin,
// Coherence-aware-Write etc. Reicht die CacheEngine-Limits in primitiver
// Form weiter; permutiert sie compile-time-statisch ueber processing_strategy.
//
// REV 7.6 V8.5 (User-Direktive 2026-05-13/14):
//   "Dadurch gehoert der result Aggregator in die abstrakte ExecutionEngine,
//    wird dort mithilfe der SearchEngine als spezielles Messinterface
//    implementiert und ist, wenn wir im experiment Modus kompilieren, immer
//    fester Bestandteil der result binary."
// → Bei COMDARE_EXPERIMENT_MODE_ON: ResultAggregator-Member, sonst nichts.

#include "configuration_permutation.hpp"
#include "processing_strategy.hpp"
#include "../concepts/i_cache_engine.hpp"
#include "../concepts/request_context.hpp"
#include "../concepts/cache_recommendation.hpp"

#include <cstddef>
#include <memory>

#ifdef COMDARE_EXPERIMENT_MODE_ON
#include <comdare/experiment/result_aggregator.hpp>
#endif

namespace comdare {

// Forward-Declaration der CacheEngine-Wurzel-Klasse aus dem bestehenden Stack.
namespace cache_engine {
class CacheEngine;
}

// Execution-Engine: erbt CacheEngine als Visitor-Pattern-Basis (REV 7 §4.2(d))
template <typename ProcessingStrategy>
class execution_engine {
public:
    using strategy_t = ProcessingStrategy;

    explicit execution_engine(std::shared_ptr<cache_engine::CacheEngine> ce,
                              strategy_t strategy = {}) noexcept
        : cache_engine_{std::move(ce)}, strategy_{std::move(strategy)} {}

    // Visitor-Hook: bei Algorithmus-Entscheidung um Rat fragen
    [[nodiscard]] cache_engine::CacheRecommendation advise(
        cache_engine::RequestContext const& ctx) const;

    // Provider-Rolle: OS-Primitiven (REV 7 §4.2(f))
    [[nodiscard]] void* cache_line_aligned_alloc(std::size_t bytes);
    void                cache_line_aligned_free(void* p, std::size_t bytes) noexcept;

    [[nodiscard]] void* numa_local_read_pin(void* p);
    void                numa_local_read_unpin(void* p) noexcept;

    void                coherence_aware_write(void* dest, void const* src, std::size_t bytes);

    [[nodiscard]] strategy_t&       strategy() noexcept       { return strategy_; }
    [[nodiscard]] strategy_t const& strategy() const noexcept { return strategy_; }

    [[nodiscard]] std::shared_ptr<cache_engine::CacheEngine> const& cache_engine_ref() const noexcept {
        return cache_engine_;
    }

#ifdef COMDARE_EXPERIMENT_MODE_ON
    // REV 7.6 V8.5: ResultAggregator als integraler Bestandteil im Experiment-Modus.
    [[nodiscard]] comdare::experiment::ResultAggregator& result_aggregator() noexcept {
        return result_aggregator_;
    }
    [[nodiscard]] comdare::experiment::ResultAggregator const& result_aggregator() const noexcept {
        return result_aggregator_;
    }
#endif

protected:
    std::shared_ptr<cache_engine::CacheEngine> cache_engine_;
    strategy_t                                 strategy_;

#ifdef COMDARE_EXPERIMENT_MODE_ON
    comdare::experiment::ResultAggregator result_aggregator_{};
#endif
};

}  // namespace comdare
