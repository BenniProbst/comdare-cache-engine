#pragma once
// V41.F.6.1.R5.B — AnatomyExecutionContext<Composition>
//
// Wraps SearchAlgorithmAnatomy<C> + Container fuer Mess-Treiber-Operationen.
// Container (std::map als Pilot R3) wandert hierhin nach User-Direktive:
// "Anatomie enthaelt nur Achsen + Observer. Alle anderen Methoden/Tools
// gehoeren in CacheEngineBuilder."
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §17.3+§24
// @task #698 V41.F.6.1.R5.B
// @related [[anatomie-nur-achsen-und-observer]]

#include <anatomy/search_algorithm_anatomy.hpp>
#include <anatomy/composition_concept.hpp>
#include <anatomy/observer_aggregate.hpp>

#include <cstdint>
#include <map>
#include <optional>

namespace comdare::cache_engine::builder::anatomy_commands {

namespace ana = ::comdare::cache_engine::anatomy;

/// AnatomyExecutionContext — Builder-Side Container + Anatomie-Holder.
///
/// Trennung: Anatomie haelt nur Achsen + Observer (R5.A). Container (std::map
/// als R5.B Pilot, spaeter Composition::node_type/allocator-getrieben) und
/// Container-Operationen sind Builder-Verantwortung.
template <ana::IsComposition Composition>
class AnatomyExecutionContext {
public:
    using anatomy_t            = ana::SearchAlgorithmAnatomy<Composition>;
    using observer_aggregate_t = typename anatomy_t::observer_aggregate_t;
    using key_type             = std::uint64_t;
    using value_type           = std::uint64_t;

    /// Default-konstruiert — Anatomie + leeren Container
    AnatomyExecutionContext() = default;

    /// Anatomie-Referenz (read-only) fuer Observer-Aggregation
    [[nodiscard]] anatomy_t const& anatomy() const noexcept { return anatomy_; }

    // ─────────────────────────────────────────────────────────────────────
    // Container-Operationen (R5.B: gehoeren in Builder, nicht Anatomie)
    // ─────────────────────────────────────────────────────────────────────

    bool insert(key_type k, value_type v) {
        auto [it, inserted] = container_.insert_or_assign(k, v);
        return inserted;
    }

    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
        auto it = container_.find(k);
        if (it == container_.end()) return std::nullopt;
        return it->second;
    }

    bool erase(key_type k) {
        return container_.erase(k) > 0;
    }

    void clear() noexcept { container_.clear(); }

    [[nodiscard]] std::size_t size() const noexcept { return container_.size(); }
    [[nodiscard]] bool        empty() const noexcept { return container_.empty(); }

    /// Snapshot-Abruf via Anatomie-API (R5.A observe_all)
    [[nodiscard]] observer_aggregate_t observe_all() const noexcept {
        return anatomy_.observe_all();
    }

    // ─────────────────────────────────────────────────────────────────────
    // Composition-Inspection (durchgereicht von Anatomie)
    // ─────────────────────────────────────────────────────────────────────

    static constexpr std::string_view composition_name() noexcept { return anatomy_t::composition_name(); }
    static constexpr std::string_view paper_id()         noexcept { return anatomy_t::paper_id(); }
    static constexpr std::size_t      organ_count()      noexcept { return anatomy_t::organ_count(); }

private:
    anatomy_t                       anatomy_{};
    std::map<key_type, value_type>  container_{};
};

}  // namespace comdare::cache_engine::builder::anatomy_commands
