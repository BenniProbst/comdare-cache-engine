#pragma once
// IObserver + ObserverRegistry — F2 Push synchron mit Lambda-Tree-Filter
// Termin 7 / 02_uml_cache_engine §5

#include <cache_engine/concepts/event.hpp>

#include <map>
#include <vector>

namespace comdare::cache_engine {

class IObserver {
public:
    virtual ~IObserver() = default;

    virtual void               on_event(Event const& event) noexcept  = 0;
    [[nodiscard]] virtual bool accepts(EventKind kind) const noexcept = 0;
};

class ObserverRegistry {
public:
    void register_observer(ModuleId module, IObserver* observer) { module_observers_[module].push_back(observer); }

    void unregister_module(ModuleId module) noexcept { module_observers_.erase(module); }

    // F2 Push synchron: Algorithmus ruft direkt dispatch(event)
    void dispatch(Event const& event) noexcept {
        auto it = module_observers_.find(event.module_id);
        if (it == module_observers_.end()) return;
        for (IObserver* obs : it->second) {
            if (obs && obs->accepts(event.kind)) obs->on_event(event);
        }
    }

    [[nodiscard]] std::size_t observer_count(ModuleId module) const noexcept {
        auto it = module_observers_.find(module);
        return it == module_observers_.end() ? 0 : it->second.size();
    }

    void clear() noexcept { module_observers_.clear(); }

private:
    std::map<ModuleId, std::vector<IObserver*>> module_observers_{};
};

} // namespace comdare::cache_engine
