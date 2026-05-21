#pragma once
// V32.EE.3 (2026-05-18) + V34.A.1 (2026-05-21) - CacheEngine als ExecutionEngine A (AA.2)
//
// @subsystem CE
// @reuse_status (b)
//
// AA.2-Korrektur: CacheEngine ist KEINE Werkzeug-Bibliothek im klassischen Sinne -
// sondern selbst eine ExecutionEngine, gleichwertig zu PRT-ART (EE-B).
// Dieser Adapter macht CacheEngine ueber IExecutingEngine-Interface ansprechbar.
//
// V34.A.1: Echtes Backend ueber Template-Param + as_engine_callable() Bridge zu
// ExecuteEngineCommand (V33.A.1 DI).

#include "../../../builder/commands/execute_engine_command.hpp"
#include "../../../builder/commands/execution_result.hpp"
#include "../../../builder/commands/workload.hpp"

#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace comdare::cache_engine::abi {

namespace cmd = comdare::cache_engine::builder::commands;

/**
 * @brief DefaultMapBackend - In-Memory std::map als Pure-SOTA-Baseline
 * @subsystem CE
 *
 * Dient als Default-Backend fuer den CacheEngineExecutionEngineAdapter wenn
 * kein expliziter Backend uebergeben wird. Repraesentiert den naivsten SOTA-
 * Baustein (std::map = Red-Black-Tree) ohne irgendwelche PRT-ART-Innovationen.
 */
template <typename Key = std::string, typename Value = std::uint64_t>
class DefaultMapBackend {
public:
    // Concurrency-Safety per Konvention: jede EE-Instanz hat ihr eigenes Backend.
    // Bei Bedarf kann ein thread-safe-Backend ueber Template-Param injiziert werden.

    [[nodiscard]] std::optional<Value> lookup(const Key& key) const {
        auto it = store_.find(key);
        if (it == store_.end()) return std::nullopt;
        return it->second;
    }

    bool insert(const Key& key, const Value& value) {
        auto [it, ok] = store_.try_emplace(key, value);
        if (!ok) it->second = value;
        return ok;
    }

    [[nodiscard]] std::size_t size() const {
        return store_.size();
    }

private:
    std::map<Key, Value> store_;
};

/**
 * @brief CacheEngineExecutionEngineAdapter - macht CacheEngine zur ExecutionEngine A
 * @subsystem CE
 * @reuse_status (b)
 *
 * Generischer Backend-Adapter. Default-Backend ist DefaultMapBackend (std::map).
 * In Produktion kann ein echter CacheEngine-Backend (z.B. ART, HOT, Masstree)
 * injiziert werden via Template-Parameter.
 *
 * Bietet:
 * - V32-API: execute(workload) -> ExecutionResult mit einfacher Simulation
 * - V34.A.1: as_engine_callable() -> EngineCallable fuer ExecuteEngineCommand DI
 * - ISearchEngine-API: lookup/insert
 */
template <typename Backend = DefaultMapBackend<std::string, std::uint64_t>>
class CacheEngineExecutionEngineAdapter {
public:
    using key_type = std::string;
    using value_type = std::uint64_t;

    CacheEngineExecutionEngineAdapter() = default;
    explicit CacheEngineExecutionEngineAdapter(Backend backend) noexcept
        : backend_{std::move(backend)} {}

    [[nodiscard]] static constexpr std::string_view engine_name() noexcept {
        return "CacheEngine-EE-A";
    }

    /// V32-API: execute eine Workload mit einfacher Simulation
    [[nodiscard]] cmd::ExecutionResult execute(const cmd::Workload& workload) {
        cmd::ExecutionResult result {};
        result.engine_name = engine_name();
        result.workload_kind = workload.kind;
        result.success = true;
        return result;
    }

    /// V34.A.1: Liefert einen EngineCallable der lookup/insert auf dem Backend macht.
    /// Wird vom ExecuteEngineCommand via DI konsumiert.
    [[nodiscard]] cmd::EngineCallable as_engine_callable() {
        return [this](std::size_t op, cmd::WorkloadKind kind, std::uint64_t seed) {
            cmd::OperationOutcome outcome {};
            const auto key = make_key(op, seed);
            switch (kind) {
                case cmd::WorkloadKind::YCSB_C_ReadOnly: {
                    auto val = this->backend_.lookup(key);
                    outcome.cache_misses_delta = val.has_value() ? 0u : 1u;
                    outcome.bytes_touched = 32u;  // key + value + node header
                    outcome.success = true;
                    break;
                }
                case cmd::WorkloadKind::YCSB_A_Read50Write50:
                case cmd::WorkloadKind::YCSB_F_ReadModifyWrite: {
                    const bool do_write = ((op + seed) % 2u == 0u);
                    if (do_write) {
                        const value_type v = static_cast<value_type>(op);
                        outcome.success = this->backend_.insert(key, v);
                        outcome.bytes_touched = 64u;
                    } else {
                        auto val = this->backend_.lookup(key);
                        outcome.cache_misses_delta = val.has_value() ? 0u : 1u;
                        outcome.bytes_touched = 32u;
                        outcome.success = true;
                    }
                    break;
                }
                default: {
                    // Generisch: Insert+Lookup-Mix
                    const value_type v = static_cast<value_type>(op);
                    this->backend_.insert(key, v);
                    outcome.bytes_touched = 64u;
                    outcome.success = true;
                    break;
                }
            }
            ops_executed_.fetch_add(1, std::memory_order_relaxed);
            return outcome;
        };
    }

    /// ISearchEngine-API: lookup
    [[nodiscard]] std::optional<value_type> lookup(const key_type& key) const {
        return backend_.lookup(key);
    }

    /// ISearchEngine-API: insert
    bool insert(const key_type& key, value_type value) {
        return backend_.insert(key, value);
    }

    [[nodiscard]] Backend& backend() noexcept { return backend_; }
    [[nodiscard]] const Backend& backend() const noexcept { return backend_; }

    [[nodiscard]] std::uint64_t ops_executed() const noexcept {
        return ops_executed_.load(std::memory_order_relaxed);
    }

private:
    static key_type make_key(std::size_t op, std::uint64_t seed) {
        return "k" + std::to_string(seed) + "_" + std::to_string(op);
    }

    Backend backend_ {};
    std::atomic<std::uint64_t> ops_executed_ {0};
};

}  // namespace comdare::cache_engine::abi
