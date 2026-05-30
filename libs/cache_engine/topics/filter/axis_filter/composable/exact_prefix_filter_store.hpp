#pragma once
// V41 Umstufung-A s4 (Task #43) — ExactPrefixFilterStore: exakte Filter-Correctness-Base (S1).
//
// @topic filter @achse axis_filter @schicht composable (Organ-statt-Tier)
//
// Reines Substrat OHNE Query-Logik — die EXAKTE Filter-Correctness-Base (S1, is_original=false): sortierte
// uint64-Keys (aus build_from_sorted_keys). Strukturell FP-Rate 0 (kein Suffix-Truncation), no-false-negative
// trivial — der GROUND-TRUTH-ANKER, gegen den S2 (echte succinct LOUDS-FST mit FP>0) per Kreuzbeleg
// `S2.contains(k) >= S1.contains(k)` verifiziert wird. Die contains/range-Logik lebt im ExactPrefixFilterOrgan.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace comdare::cache_engine::filter::axis_filter::composable {

class ExactPrefixFilterStore {
public:
    using key_type = std::uint64_t;

    /// Bulk-Aufbau aus AUFSTEIGEND sortierten Keys (SuRF baut single-scan aus sortierten Keys).
    /// Darf via vector werfen ([[allocation-failure-exception]]). Nicht-sortierte Eingabe wird defensiv sortiert.
    void build_from_sorted_keys(std::span<key_type const> sorted) {
        keys_.assign(sorted.begin(), sorted.end());
        if (!std::is_sorted(keys_.begin(), keys_.end())) std::sort(keys_.begin(), keys_.end());
        keys_.erase(std::unique(keys_.begin(), keys_.end()), keys_.end());
    }

    [[nodiscard]] std::size_t key_count() const noexcept { return keys_.size(); }
    [[nodiscard]] std::size_t bit_size()  const noexcept { return keys_.size() * 64u; }   // exakt: voller 64-Bit-Key
    [[nodiscard]] double bits_per_key()   const noexcept { return keys_.empty() ? 0.0 : 64.0; }

    /// Index des ersten Keys >= k (== size falls keiner). Fundament fuer contains/range.
    [[nodiscard]] std::size_t lower_bound(key_type k) const noexcept {
        return static_cast<std::size_t>(std::lower_bound(keys_.begin(), keys_.end(), k) - keys_.begin());
    }
    [[nodiscard]] key_type key_at(std::size_t i) const noexcept { return keys_[i]; }

    void clear() noexcept { keys_.clear(); }

private:
    std::vector<key_type> keys_{};   // aufsteigend sortiert, duplikatfrei
};

}  // namespace comdare::cache_engine::filter::axis_filter::composable
