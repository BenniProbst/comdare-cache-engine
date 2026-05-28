#pragma once
// V41.F.6.1.F.6 axis_07_prefetch PathOrientedImpl (2026-05-29)
//
// @topic prefetch @achse 07 @impl path_oriented
//
// **Herkunft:** F.6-Migration aus prt-art `prefetch/path_oriented_prefetch.hpp`
// (PathOrientedPrefetch, REV 6 §5.17 + REV 7.6 V11.1 Hot-Path-Hints) — der
// Algorithmus-Kern der Diplomarbeit (pfad-orientiertes Prefetching). Der
// metadaten-only-Wrapper PathOrientedPrefetch (PF3 granularity) bekommt hierueber
// echte Logik: verfolgt den aktiven Suchpfad, extrapoliert die naechste erwartete
// Adresse und integriert Hot-Path-Hints aus rohen Schluessel-Bytes.
//
// HotPathMixin-Aspekt (V11.1): note_hot_path_bytes() interpretiert die ersten
// 8 Byte eines Schluessels (z.B. binary_key_t aus dem SearchEngine-ABI) als
// uint64-Adresse und speist sie in die Pfad-Trajektorie ein.
//
// Allocation: std::vector waechst dynamisch (bounded auf kMaxTrackedSlots) —
// [[allocation-failure-exception]] (push_back kann std::bad_alloc werfen).

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace comdare::cache_engine::prefetch::axis_07_prefetch::impl {

/// Pfad-orientierter Prefetch-Tracker (prt-art REV 6 §5.17, V11.1 Hot-Path).
class PathOrientedImpl {
public:
    static constexpr std::size_t kMaxTrackedSlots = 16;

    PathOrientedImpl() = default;

    /// Reiht die naechste erwartete Adresse in die Pfad-Trajektorie ein (FIFO, bounded).
    void enqueue(std::uint64_t addr) {
        recent_path_.push_back(addr);
        if (recent_path_.size() > kMaxTrackedSlots) {
            recent_path_.erase(recent_path_.begin(),
                               recent_path_.begin() + (recent_path_.size() - kMaxTrackedSlots));
        }
        ++total_enqueued_;
    }

    [[nodiscard]] std::uint64_t total_enqueued() const noexcept { return total_enqueued_; }
    [[nodiscard]] std::size_t   queue_depth()    const noexcept { return recent_path_.size(); }
    [[nodiscard]] std::vector<std::uint64_t> const& path() const noexcept { return recent_path_; }

    /// Empfehlung fuer die naechste Prefetch-Adresse via linearer Schritt-Extrapolation.
    [[nodiscard]] std::uint64_t suggest_next() const noexcept {
        if (recent_path_.size() < 2) return recent_path_.empty() ? 0u : recent_path_.back();
        std::uint64_t a    = recent_path_[recent_path_.size() - 2];
        std::uint64_t b    = recent_path_.back();
        std::uint64_t step = (b > a) ? (b - a) : 0u;
        return b + step;
    }

    void reset() noexcept {
        recent_path_.clear();
        total_enqueued_ = 0;
    }

    /// V11.1 Hot-Path-Hint: erste min(8, bytes) Byte als uint64 interpretieren + einreihen.
    void note_hot_path_bytes(std::byte const* data, std::size_t bytes) noexcept {
        if (data == nullptr || bytes == 0) return;
        std::uint64_t addr = 0;
        std::size_t   copy = bytes < sizeof(addr) ? bytes : sizeof(addr);
        std::memcpy(&addr, data, copy);
        enqueue(addr);
        ++total_hot_path_hints_;
    }
    [[nodiscard]] std::uint64_t total_hot_path_hints() const noexcept { return total_hot_path_hints_; }

private:
    std::vector<std::uint64_t> recent_path_{};
    std::uint64_t              total_enqueued_       = 0;
    std::uint64_t              total_hot_path_hints_ = 0;  // V11.1
};

}  // namespace comdare::cache_engine::prefetch::axis_07_prefetch::impl
