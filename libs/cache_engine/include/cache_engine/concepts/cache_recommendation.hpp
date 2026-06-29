#pragma once
// cache_recommendation.hpp - REV 5.2 CacheEngine-Antwort an ISearchPageStrategy
// Quelle: U09 (UML) - Visitor mit Rueckgabe, nicht void!

#include <cstdint>
#include <optional>
#include <string>

namespace comdare::cache_engine {

/// MemoryAllocationHint - Tier-Wahl fuer neue Allocation
struct MemoryAllocationHint {
    enum class TierKind : std::uint8_t {
        Dram,
        Hbm,
        Nvram,
        VCache, ///< 3D V-Cache (Block AO Ryzen 9 9950X3D)
        PageCache,
        Heap
    };
    TierKind    tier            = TierKind::Dram;
    std::size_t alignment_bytes = 64;
    std::size_t size_bytes      = 0;
};

/// LayoutChangeProposal - Knoten-Geometrie aendern (P02 HOT k-Wechsel, P05 START Span)
struct LayoutChangeProposal {
    std::uint32_t target_node_size_bytes = 0;
    std::uint32_t target_fanout          = 0;
    bool          change_encoding        = false;
    std::string   new_encoding_name; // z.B. "louds_sparse", "hot_compound"
};

/// PrefetchAdvisory - Latenz-Hiding-Empfehlung (P21 Chen, P23 Khan, P25 Mahling)
struct PrefetchAdvisory {
    enum class Mode : std::uint8_t { Off, Static, WarmupCalibrated, OnlineAdaptive };
    Mode          mode          = Mode::Off;
    std::uint16_t distance      = 0;     ///< 0 = off, 1-8 typisch
    bool          use_coroutine = false; ///< P25 Mahling
};

/// MigrationDirective - Tier-Wechsel (P19 Saikkonen, P33 VAMPIR)
struct MigrationDirective {
    void*                          source_addr = nullptr;
    void*                          target_addr = nullptr;
    MemoryAllocationHint::TierKind target_tier = MemoryAllocationHint::TierKind::Dram;
};

/// PinningDirective - Core-Bindung (Block AO Hybrid)
struct PinningDirective {
    enum class Policy : std::uint8_t { None, LargestL3Ccd, HighIpcCore, NumaLocal, RoundRobin, FirstTouch };
    Policy       policy         = Policy::None;
    std::int32_t target_core_id = -1; ///< -1 = unspecified
};

/// PressureSummary - kompakte Telemetry-Snapshot (fuer Caller + Audit)
struct PressureSummary {
    double        cache_miss_rate = 0.0;
    double        bandwidth_util  = 0.0;
    std::uint32_t hot_path_score  = 0;
    std::uint64_t timestamp_ns    = 0;
};

// -----------------------------------------------------------------------------
// CacheRecommendation - Rueckgabe an ISearchPageStrategy
// -----------------------------------------------------------------------------
struct CacheRecommendation {
    enum class Verdict : std::uint8_t {
        DoNothing, ///< Naderan-Tahan negative-state
        Hint,      ///< nur Prefetch-Distance aktualisieren
        Reshape,   ///< Layout-Geometrie aendern (P20 adaptive)
        Migrate,   ///< Tier-Wechsel
        Allocate,  ///< neue Allocation noetig
        Abort      ///< Operation abbrechen (Notfall)
    };

    Verdict verdict = Verdict::DoNothing;

    std::optional<MemoryAllocationHint> alloc_hint;
    std::optional<LayoutChangeProposal> layout_proposal;
    std::optional<PrefetchAdvisory>     prefetch_advice;
    std::optional<MigrationDirective>   migration_directive;
    std::optional<PinningDirective>     pinning_directive;

    PressureSummary pressure_summary{};
    double          confidence = 0.0; ///< 0.0-1.0

    /// Factory: DoNothing-Antwort
    static CacheRecommendation DoNothing() noexcept { return CacheRecommendation{}; }
};

} // namespace comdare::cache_engine
