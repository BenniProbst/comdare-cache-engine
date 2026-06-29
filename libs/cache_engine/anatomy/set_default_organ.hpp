#pragma once
// D9.3 / L-76a (2026-06-02) — SortedArrayKeySet: ein leichtgewichtiges, ECHTES search_algo-API-Organ (sorted-array
// Mengen-Algorithmus mit Binary-Search) als Default-Kern der Set-Gattung. Erfüllt die search_algo-Pflicht-API
// (key_type + insert(k,v)/lookup(k)→optional/erase(k)/occupied_count()/clear()), die SetAnatomy K=V treibt.
//
// Zweck: macht die Set-DLL OHNE das schwere Achsen-Umbrella (Boost/generated) baubar — analog wie Sequence/View
// interne Organe (std::vector / externer Puffer) nutzen. Der Vollausbau mit den ~30 echten search_algo-Wrappern
// (Array256/Patricia/…) als Set-Kern ist die R5.B-Operativitäts-Erweiterung (separat, #74-artig). Header-only.

#include <algorithm>
#include <cstdint>
#include <optional>
#include <vector>

namespace comdare::cache_engine::anatomy {

/// SortedArrayKeySet — sorted-array K-Set (Binary-Search). search_algo-API-konform (K=V von SetAnatomy genutzt).
struct SortedArrayKeySet {
    using key_type = std::uint64_t;

    void insert(std::uint64_t k, std::uint64_t /*v*/) {
        auto it = std::lower_bound(keys_.begin(), keys_.end(), k);
        if (it == keys_.end() || *it != k) keys_.insert(it, k); // Set: keine Duplikate
    }
    [[nodiscard]] std::optional<std::uint64_t> lookup(std::uint64_t k) const {
        auto it = std::lower_bound(keys_.begin(), keys_.end(), k);
        return (it != keys_.end() && *it == k) ? std::optional<std::uint64_t>{k} : std::nullopt;
    }
    void erase(std::uint64_t k) {
        auto it = std::lower_bound(keys_.begin(), keys_.end(), k);
        if (it != keys_.end() && *it == k) keys_.erase(it);
    }
    [[nodiscard]] std::size_t occupied_count() const noexcept { return keys_.size(); }
    void                      clear() noexcept { keys_.clear(); }

private:
    std::vector<std::uint64_t> keys_; // aufsteigend sortiert
};

} // namespace comdare::cache_engine::anatomy
