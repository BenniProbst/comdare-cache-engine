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
    static constexpr Key           kEmpty    = static_cast<Key>(0);
    static constexpr double        kMaxLoad  = 0.7;

    LinearProbeHashSet() = default;

    explicit LinearProbeHashSet(std::size_t hint) { reserve(hint); }

    void reserve(std::size_t hint) {
        std::size_t cap = 16;
        while (static_cast<double>(hint) > static_cast<double>(cap) * kMaxLoad) { cap *= 2; }
        buckets_.assign(cap, kEmpty);
        capacity_mask_ = cap - 1; // cap is power-of-2
        // has_zero_ wird NICHT zurueckgesetzt (sonst verloere der lazy reserve(16) im
        // Nicht-Null-insert ein zuvor per insert(0) gesetztes Flag); den Sentinel-Eintrag
        // aber in size_ beruecksichtigen, da reserve() hier mit leeren buckets_ laeuft.
        size_ = has_zero_ ? std::size_t{1} : std::size_t{0};
    }

    // Returns true wenn neu eingefuegt, false wenn schon vorhanden
    bool insert(Key k) {
        if (k == kEmpty) {
            // V41.A5-fix (2026-06-01): Der als Empty-Marker reservierte Wert 0 wird
            // separat via has_zero_ gefuehrt (Standardloesung fuer value-sentinel
            // open-addressing), damit ALLE Key-Werte inkl. 0 speicherbar sind
            // (std::unordered_set-Kompatibilitaet, vgl. Header-Doku). Vorher wurde 0
            // still verworfen und war nie auffindbar — das liess scalar-Permutationen
            // mit mix(0)=0*kPrime=0 systematisch um 1 Treffer danebenliegen (run()=-2).
            if (has_zero_) return false; // duplicate
            has_zero_ = true;
            ++size_;
            return true;
        }
        if (buckets_.empty()) { reserve(16); }
        if (static_cast<double>(size_ + 1) > static_cast<double>(buckets_.size()) * kMaxLoad) { grow(); }
        std::size_t idx = hash_index(k);
        while (buckets_[idx] != kEmpty) {
            if (buckets_[idx] == k) return false; // duplicate
            idx = (idx + 1) & capacity_mask_;
        }
        buckets_[idx] = k;
        ++size_;
        return true;
    }

    // Returns true wenn gefunden
    [[nodiscard]] bool contains(Key k) const {
        if (k == kEmpty) return has_zero_;
        if (buckets_.empty()) return false;
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
        std::vector<Key> old     = std::move(buckets_);
        std::size_t      old_cap = old.size();
        std::size_t      new_cap = old_cap * 2;
        buckets_.assign(new_cap, kEmpty);
        capacity_mask_ = new_cap - 1;
        // has_zero_ bleibt erhalten (nicht in buckets_ gespeichert); den Sentinel-Eintrag
        // im size_-Reset beruecksichtigen, sonst zaehlt size() nach grow() um 1 zu wenig.
        size_ = has_zero_ ? std::size_t{1} : std::size_t{0};
        for (std::size_t i = 0; i < old_cap; ++i) {
            if (old[i] != kEmpty) { insert(old[i]); }
        }
    }

    std::vector<Key> buckets_;
    std::size_t      capacity_mask_{0};
    std::size_t      size_{0};
    bool             has_zero_{false}; // V41.A5-fix: speichert den Sentinel-Key 0 separat
};

} // namespace comdare::cache_engine::indexes
