#pragma once
// DecisionLambdaTreeRegistry — vereint alle Trees, die geladene Module mitbringen
// Termin 7 / 02_uml_cache_engine §2

#include <cache_engine/concepts/decision_lambda_tree_bundle.hpp>
#include <cache_engine/concepts/event.hpp>

#include <map>

namespace comdare::cache_engine {

class DecisionLambdaTreeRegistry {
public:
    void register_module_trees(ModuleId module, AnyDecisionTreeBundle bundle) { trees_per_module_[module] = bundle; }

    void unregister_module(ModuleId module) noexcept { trees_per_module_.erase(module); }

    // Routing nach Event-Typ + Modul-Kontext.
    // Gibt EXECUTE zurueck, falls fuer das Event/Modul kein Tree registriert ist
    // (Defaultverhalten: nicht filtern).
    [[nodiscard]] Decision dispatch(ModuleId module, Event const& event, DecisionContext const& ctx) const noexcept {
        auto it = trees_per_module_.find(module);
        if (it == trees_per_module_.end()) return Decision::EXECUTE;
        AnyDecisionTreeBundle const& b = it->second;
        switch (event.kind) {
            case EventKind::PageRelocation: {
                auto* t = b.page_relocation;
                if (!t) return Decision::EXECUTE;
                return t->evaluate(static_cast<PageRelocationEvent const&>(event), ctx);
            }
            case EventKind::PageTypeChange: {
                auto* t = b.page_type_change;
                if (!t) return Decision::EXECUTE;
                return t->evaluate(static_cast<PageTypeChangeEvent const&>(event), ctx);
            }
            case EventKind::PrefetchAdjustment: {
                auto* t = b.prefetch_adjustment;
                if (!t) return Decision::EXECUTE;
                return t->evaluate(static_cast<PrefetchAdjustmentEvent const&>(event), ctx);
            }
            case EventKind::HotPathRecognition: {
                auto* t = b.hot_path_recognition;
                if (!t) return Decision::EXECUTE;
                return t->evaluate(static_cast<HotPathRecognitionEvent const&>(event), ctx);
            }
            case EventKind::TelemetryUpdate: {
                // Routing-Ambiguitaet: zwei Trees lauschen auf TelemetryUpdate
                if (auto* t = b.cache_coherence_detect)
                    return t->evaluate(static_cast<TelemetryUpdateEvent const&>(event), ctx);
                if (auto* t = b.value_handle_selection)
                    return t->evaluate(static_cast<TelemetryUpdateEvent const&>(event), ctx);
                return Decision::EXECUTE;
            }
            case EventKind::Write: {
                if (auto* t = b.coherence_aware_write) return t->evaluate(static_cast<WriteEvent const&>(event), ctx);
                if (auto* t = b.concurrency_switch) return t->evaluate(static_cast<WriteEvent const&>(event), ctx);
                return Decision::EXECUTE;
            }
            case EventKind::Sampling: {
                if (auto* t = b.sampling_rate) return t->evaluate(static_cast<SamplingEvent const&>(event), ctx);
                return Decision::EXECUTE;
            }
            case EventKind::ConsolidationBarrier:
            case EventKind::Error:
            default: return Decision::EXECUTE;
        }
    }

    [[nodiscard]] std::size_t module_count() const noexcept { return trees_per_module_.size(); }

    [[nodiscard]] bool has_module(ModuleId module) const noexcept {
        return trees_per_module_.find(module) != trees_per_module_.end();
    }

    void clear() noexcept { trees_per_module_.clear(); }

private:
    std::map<ModuleId, AnyDecisionTreeBundle> trees_per_module_{};
};

} // namespace comdare::cache_engine
