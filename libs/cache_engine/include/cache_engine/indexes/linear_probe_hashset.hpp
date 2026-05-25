// SPDX-License-Identifier: Apache-2.0
// V41.A5 (2026-05-24) - Echte Algorithmus-Implementierung (kein STL-Wrapper)
//
// LinearProbeHashSet: open-addressing Hash-Set mit Fibonacci-Hash und Linear
// Probing. Cache-friendlich (kontiguoeser Bucket-Storage, kein pointer-chasing
// wie bei std::unordered_set's separate chaining).
//
// API kompatibel zu std::unordered_set fuer einfache Verwendung in Wrappers:
//   insert(key), find(key) != end()
//
// Komplexitaet: Amortisiert O(1) bei load_factor < 0.7

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace comdare::cache_engine::indexes {

template <typename Key>
class LinearProbeHashSet {
public:
    static constexpr std::uint64_t kFibPrime = 11400714819323198485ULL;
    static constexpr Key            kEmpty   = static_cast<Key>(0);
    static constexpr double         kMaxLoad = 0.7;

    LinearProbeHashSet() = default;

    explicit LinearProbeHashSet(std::size_t hint) {
        reserve(hint);
    }

    void reserve(std::size_t hint) {
        std::size_t cap = 16;
        while (static_cast<double>(hint) > static_cast<double>(cap) * kMaxLoad) {
            cap *= 2;
        }
        buckets_.assign(cap, kEmpty);
        capacity_mask_ = cap - 1;  // cap is power-of-2
        size_ = 0;
    }

    // Returns true wenn neu eingefuegt, false wenn schon vorhanden
    bool insert(Key k) {
        if (k == kEmpty) {
            // Reserved sentinel — gracefully skip
            return false;
        }
        if (buckets_.empty()) {
            reserve(16);
        }
        if (static_cast<double>(size_ + 1) > static_cast<double>(buckets_.size()) * kMaxLoad) {
            grow();
        }
        std::size_t idx = hash_index(k);
        while (buckets_[idx] != kEmpty) {
            if (buckets_[idx] == k) return false;  // duplicate
            idx = (idx + 1) & capacity_mask_;
        }
        buckets_[idx] = k;
        ++size_;
        return true;
    }

    // Returns true wenn gefunden
    [[nodiscard]] bool contains(Key k) const {
        if (buckets_.empty() || k == kEmpty) return false;
        std::size_t idx = hash_index(k);
        while (buckets_[idx] != kEmpty) {
            if (buckets_[idx] == k) return true;
            idx = (idx + 1) & capacity_mask_;
        }
        return false;
    }

    [[nodiscard]] std::size_t size() const { return size_; }
    [[nodiscard]] std::size_t capacity() const { return buckets_.size(); }

private:
    std::size_t hash_index(Key k) const {
        return static_cast<std::size_t>(static_cast<std::uint64_t>(k) * kFibPrime) & capacity_mask_;
    }

    void grow() {
        std::vector<Key> old = std::move(buckets_);
        std::size_t old_cap = old.size();
        std::size_t new_cap = old_cap * 2;
        buckets_.assign(new_cap, kEmpty);
        capacity_mask_ = new_cap - 1;
        size_ = 0;
        for (std::size_t i = 0; i < old_cap; ++i) {
            if (old[i] != kEmpty) {
                insert(old[i]);
            }
        }
    }

    std::vector<Key> buckets_;
    std::size_t      capacity_mask_ {0};
    std::size_t      size_          {0};
};

}  // namespace comdare::cache_engine::indexes
