#pragma once
// DecisionLambdaTreeBundle — Komposition aller Trees pro Permutation (F-EXTRA-6)
// Termin 7 / 02_uml_cache_engine §2

#include <cache_engine/concepts/decision_trees/allocator_rebalance_tree.hpp>
#include <cache_engine/concepts/decision_trees/cache_coherence_detection_tree.hpp>
#include <cache_engine/concepts/decision_trees/coherence_aware_write_decision_tree.hpp>
#include <cache_engine/concepts/decision_trees/concurrency_discipline_switch_tree.hpp>
#include <cache_engine/concepts/decision_trees/hot_path_recognition_tree.hpp>
#include <cache_engine/concepts/decision_trees/page_relocation_tree.hpp>
#include <cache_engine/concepts/decision_trees/page_type_change_tree.hpp>
#include <cache_engine/concepts/decision_trees/prefetch_adjustment_tree.hpp>
#include <cache_engine/concepts/decision_trees/sampling_rate_adjustment_tree.hpp>
#include <cache_engine/concepts/decision_trees/value_handle_selection_tree.hpp>

#include <memory>

namespace comdare::cache_engine {

// Ein Bundle pro geladenem Permutations-Modul. Alle Trees optional —
// nur belegt, wenn der jeweilige Baustein aktiviert ist.
template <typename PageT, typename TraversalT, typename AllocatorT, typename DisciplineT, typename HandleT,
          typename TelemetryT>
struct DecisionLambdaTreeBundle {
    std::unique_ptr<PageRelocationTree<PageT>>                    page_relocation;
    std::unique_ptr<PageTypeChangeTree<PageT>>                    page_type_change;
    std::unique_ptr<PrefetchAdjustmentTree<TraversalT>>           prefetch_adjustment;
    std::unique_ptr<HotPathRecognitionTree<TraversalT>>           hot_path_recognition;
    std::unique_ptr<AllocatorRebalanceTree<AllocatorT>>           allocator_rebalance;
    std::unique_ptr<ConcurrencyDisciplineSwitchTree<DisciplineT>> concurrency_switch;
    std::unique_ptr<ValueHandleSelectionTree<HandleT>>            value_handle_selection;
    std::unique_ptr<SamplingRateAdjustmentTree<TelemetryT>>       sampling_rate;
    std::unique_ptr<CoherenceAwareWriteDecisionTree>              coherence_aware_write;
    std::unique_ptr<CacheCoherenceDetectionTree>                  cache_coherence_detect;
};

// Type-erased Bundle — fuer DecisionLambdaTreeRegistry-Verwendung
struct AnyDecisionTreeBundle {
    // Pro Tree-Slot ein erased Tree (per virtueller Wurzel-Klasse referenziert)
    IDecisionLambdaTree<PageRelocationEvent>*     page_relocation        = nullptr;
    IDecisionLambdaTree<PageTypeChangeEvent>*     page_type_change       = nullptr;
    IDecisionLambdaTree<PrefetchAdjustmentEvent>* prefetch_adjustment    = nullptr;
    IDecisionLambdaTree<HotPathRecognitionEvent>* hot_path_recognition   = nullptr;
    IDecisionLambdaTree<PageRelocationEvent>*     allocator_rebalance    = nullptr;
    IDecisionLambdaTree<WriteEvent>*              concurrency_switch     = nullptr;
    IDecisionLambdaTree<TelemetryUpdateEvent>*    value_handle_selection = nullptr;
    IDecisionLambdaTree<SamplingEvent>*           sampling_rate          = nullptr;
    IDecisionLambdaTree<WriteEvent>*              coherence_aware_write  = nullptr;
    IDecisionLambdaTree<TelemetryUpdateEvent>*    cache_coherence_detect = nullptr;

    [[nodiscard]] std::size_t populated_count() const noexcept {
        std::size_t n = 0;
        if (page_relocation) ++n;
        if (page_type_change) ++n;
        if (prefetch_adjustment) ++n;
        if (hot_path_recognition) ++n;
        if (allocator_rebalance) ++n;
        if (concurrency_switch) ++n;
        if (value_handle_selection) ++n;
        if (sampling_rate) ++n;
        if (coherence_aware_write) ++n;
        if (cache_coherence_detect) ++n;
        return n;
    }
};

} // namespace comdare::cache_engine
