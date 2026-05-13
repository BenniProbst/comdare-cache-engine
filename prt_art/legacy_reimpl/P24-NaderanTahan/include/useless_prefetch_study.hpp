// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 BEP Venture UG (Marke Comdare)
//
// P24-NaderanTahan — Re-Implementation in PRT-ART
// Paper: Why Does Data Prefetching Not Work for Modern Workloads
//        (Naderan-Tahan/Sarbazi-Azad 2016)
// Concept: Telemetry (Achse: Measurement)
//
// Kernidee: Telemetry-Counter, der Useless Prefetches misst.
// Useless = ein gepretchter Block wird verdraengt, bevor er benutzt wird.

#pragma once

#include <cstdint>
#include <unordered_map>

namespace comdare::prt_art::legacy_reimpl::useless_prefetch {

struct UsefulnessMetrics {
    std::uint64_t total_prefetches    = 0;
    std::uint64_t useful_prefetches   = 0;   // benutzt vor evict
    std::uint64_t useless_prefetches  = 0;   // evicted ohne use
    std::uint64_t late_prefetches     = 0;   // benutzt nach evict

    [[nodiscard]] double usefulness_ratio() const noexcept {
        return total_prefetches > 0
            ? static_cast<double>(useful_prefetches) / static_cast<double>(total_prefetches)
            : 0.0;
    }
    [[nodiscard]] double waste_ratio() const noexcept {
        return total_prefetches > 0
            ? static_cast<double>(useless_prefetches) / static_cast<double>(total_prefetches)
            : 0.0;
    }
};

class UselessPrefetchTracker {
public:
    void prefetch(std::uint64_t addr) {
        pending_[addr] = false;
        ++metrics_.total_prefetches;
    }

    void use(std::uint64_t addr) {
        auto it = pending_.find(addr);
        if (it != pending_.end()) {
            if (!it->second) {
                ++metrics_.useful_prefetches;
                it->second = true;
            }
        } else {
            ++metrics_.late_prefetches;
        }
    }

    void evict(std::uint64_t addr) {
        auto it = pending_.find(addr);
        if (it == pending_.end()) return;
        if (!it->second) {
            ++metrics_.useless_prefetches;
        }
        pending_.erase(it);
    }

    [[nodiscard]] UsefulnessMetrics const& metrics() const noexcept { return metrics_; }

private:
    std::unordered_map<std::uint64_t, bool> pending_{};  // addr -> was_used
    UsefulnessMetrics                        metrics_{};
};

}  // namespace comdare::prt_art::legacy_reimpl::useless_prefetch
