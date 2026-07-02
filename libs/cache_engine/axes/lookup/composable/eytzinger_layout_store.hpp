#pragma once
// #188-4a (2026-07-02) -- EytzingerLayoutStore: sortierte Basis + lazy BFS-Layout in EINEM Store.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Option (b) (User-Entscheid 2026-07-02): sortierte Basis = Quelle der Wahrheit; BFS-Layout wird lazy im ersten
// lookup nach Mutation neu gebaut (O(n) assign + rekursives Fill). Diese Rebuild-Kosten sind eine DOKUMENTIERTE
// Mess-Eigenschaft des Eytzinger-Tiers (erster Lookup nach Mutations-Burst traegt den Rebuild), NICHT
// wegzuoptimieren. Kein thread-safe const-lookup (mutable-Rebuild; is_thread_safe()==false wie der Wrapper).
//
// Der Store ist kopierbar/movebar per Default. Eine dirty-Kopie rebuildert ihren eigenen BFS-Puffer beim naechsten
// Lookup; die Vektoren sind unabhaengig und damit Memento-/CoW-tauglich fuer den Adapter-Pfad.
// Allocation: std::vector dynamisch -- [[allocation-failure-exception]]: insert_slot_at/append_slot und
// rebuild_if_dirty koennen std::bad_alloc werfen.

#include "eytzinger_layout_pool_concept.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace comdare::cache_engine::lookup::composable {

/// Eytzinger-Layout-Substrat: sortierter Primaerzustand plus abgeleiteter 1-indexed BFS-Puffer.
class EytzingerLayoutStore {
public:
    using key_type   = std::uint64_t;
    using value_type = std::uint64_t;

    [[nodiscard]] std::size_t slot_count() const noexcept { return keys_.size(); }
    [[nodiscard]] key_type    key_at(std::size_t i) const noexcept { return keys_[i]; }
    [[nodiscard]] value_type  value_at(std::size_t i) const noexcept { return values_[i]; }

    void set_value_at(std::size_t i, value_type v) noexcept {
        values_[i] = v;
        dirty_     = true;
    }
    void insert_slot_at(std::size_t i, key_type k, value_type v) {
        keys_.insert(keys_.begin() + static_cast<std::ptrdiff_t>(i), k);
        values_.insert(values_.begin() + static_cast<std::ptrdiff_t>(i), v);
        dirty_ = true;
    }
    void erase_slot_at(std::size_t i) {
        keys_.erase(keys_.begin() + static_cast<std::ptrdiff_t>(i));
        values_.erase(values_.begin() + static_cast<std::ptrdiff_t>(i));
        dirty_ = true;
    }
    void append_slot(key_type k, value_type v) {
        keys_.push_back(k);
        values_.push_back(v);
        dirty_ = true;
    }
    void clear() noexcept {
        keys_.clear();
        values_.clear();
        eyt_keys_.clear();
        eyt_vals_.clear();
        dirty_ = true;
    }

    /// Baut bei Bedarf das 1-indizierte Eytzinger-Layout aus der sortierten Quelle.
    void rebuild_if_dirty() const {
        if (!dirty_) return;
        std::size_t const n = keys_.size();
        eyt_keys_.assign(n + 1u, key_type{});
        eyt_vals_.assign(n + 1u, value_type{});
        std::size_t pos = 0;
        fill_eytzinger(1u, n, pos);
        dirty_ = false;
    }

    [[nodiscard]] key_type   eyt_key_at(std::size_t i) const noexcept { return eyt_keys_[i]; }
    [[nodiscard]] value_type eyt_value_at(std::size_t i) const noexcept { return eyt_vals_[i]; }
    [[nodiscard]] bool       dirty() const noexcept { return dirty_; }

private:
    void fill_eytzinger(std::size_t k, std::size_t n, std::size_t& pos) const {
        if (k <= n) {
            fill_eytzinger(2u * k, n, pos);
            eyt_keys_[k] = keys_[pos];
            eyt_vals_[k] = values_[pos];
            ++pos;
            fill_eytzinger(2u * k + 1u, n, pos);
        }
    }

    std::vector<key_type>           keys_{};
    std::vector<value_type>         values_{};
    mutable std::vector<key_type>   eyt_keys_{};
    mutable std::vector<value_type> eyt_vals_{};
    mutable bool                    dirty_ = false;
};

static_assert(EytzingerLayoutPool<EytzingerLayoutStore>);

} // namespace comdare::cache_engine::lookup::composable
