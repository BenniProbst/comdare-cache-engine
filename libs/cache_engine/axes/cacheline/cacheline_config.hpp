#pragma once
// KF-3 (2026-06-02) — cacheline_config.hpp: die PER-ORGAN Cache-Line-Unterachse (KERNTHEMA der Diplomarbeit).
//
// User-Direktive 2026-06-02: die Cache-Line-Groesse je Organ EINZELN variieren/permutieren. `cacheline` ist
// daher KEINE globale Einstellung, sondern eine eigene Compile-Time-Unterachse PRO betroffenem Organ
// (page/node/traversal/allocator). Jedes Organ traegt seine UNABHAENGIGE CacheLineConfig (45 = 3x3x5).
//
// Querschnitt-Komponente (kein eigenes Organ-Set): die betroffenen Organ-Algorithmen WEBEN sie via CRTP-Mixin
// CacheLineAware<Cfg> ein (KF-5). Compile-time only (kein Runtime-Switch im Hot-Path):
//   - line_size  → alignas/Packing-Granularitaet (Struktur an 64/128/256-B-Grenze)
//   - alignment  → none / cache_line_aligned / padded (false-sharing-Padding)
//   - sw_hint    → Software-Prefetch-Hint (_mm_prefetch T0/T1/T2/NTA), per if constexpr gebacken
// HW-Grundlage: docs/sessions/20260602-cacheline-konfigurator-design-und-hw-recherche.md §4.1.
// (Der HW-Prefetcher-MSR-Zustand ist die EINZIGE Laufzeit-Dimension dieser Unterachse → Launcher, §7.)

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>

#if defined(_MSC_VER)
  #include <intrin.h>
#elif (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))
  #include <x86intrin.h>
#endif

namespace comdare::cache_engine::cacheline {

// ── Wertraum (3 x 3 x 5 = 45) ────────────────────────────────────────────────
enum class CacheLineSize : std::uint16_t { B64 = 64, B128 = 128, B256 = 256 };
enum class CacheLineAlignment : std::uint8_t { None = 0, CacheLineAligned = 1, Padded = 2 };
enum class SwPrefetchHint : std::uint8_t { None = 0, T0 = 1, T1 = 2, T2 = 3, NTA = 4 };

/// Compile-Time-Konfiguration einer Cache-Line-Unterachse EINES Organs. Strukturell → als NTTP nutzbar.
struct CacheLineConfig {
    CacheLineSize      line_size = CacheLineSize::B64;
    CacheLineAlignment alignment = CacheLineAlignment::None;
    SwPrefetchHint     sw_hint   = SwPrefetchHint::None;

    [[nodiscard]] constexpr bool operator==(CacheLineConfig const&) const noexcept = default;
};

// ── Compile-Time-Primitive ───────────────────────────────────────────────────

/// Effektive Alignment-Granularitaet in Bytes. None → natuerliches max_align; sonst die Cache-Line-Groesse.
[[nodiscard]] constexpr std::size_t alignment_bytes(CacheLineConfig c) noexcept {
    return (c.alignment == CacheLineAlignment::None)
               ? alignof(std::max_align_t)
               : static_cast<std::size_t>(c.line_size);
}

/// Software-Prefetch mit compile-time gebackenem Hint (kein Runtime-Branch). No-op fuer None / Nicht-x86.
template <SwPrefetchHint Hint>
inline void prefetch(void const* p) noexcept {
    if constexpr (Hint == SwPrefetchHint::None) {
        (void)p;
    } else {
#if defined(_MSC_VER)
        constexpr int h = (Hint == SwPrefetchHint::T0)  ? _MM_HINT_T0
                        : (Hint == SwPrefetchHint::T1)  ? _MM_HINT_T1
                        : (Hint == SwPrefetchHint::T2)  ? _MM_HINT_T2
                                                        : _MM_HINT_NTA;
        _mm_prefetch(static_cast<char const*>(p), h);
#elif (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))
        constexpr int locality = (Hint == SwPrefetchHint::T0) ? 3
                               : (Hint == SwPrefetchHint::T1) ? 2
                               : (Hint == SwPrefetchHint::T2) ? 1
                                                              : 0;  // NTA = non-temporal
        __builtin_prefetch(p, 0, locality);
#else
        (void)p;
#endif
    }
}

// ── Concept: ein Organ unterstuetzt die Cache-Line-Unterachse ────────────────
template <typename T>
concept CacheLineConfigurable = requires {
    { T::cacheline_config() } -> std::same_as<CacheLineConfig>;
};

// ── CRTP-Mixin: betroffene Organ-Algorithmen erben dies, um die Unterachse zu tragen (KF-5) ──
/// Cfg ist die per-Organ-Konfiguration (vom Codegen je Organ gesetzt). Bietet die Pflicht-API
/// cacheline_config() + die Primitive (alignment, prefetch) compile-time gebacken.
template <CacheLineConfig Cfg>
struct CacheLineAware {
    [[nodiscard]] static constexpr CacheLineConfig cacheline_config() noexcept { return Cfg; }
    [[nodiscard]] static constexpr std::size_t cacheline_alignment() noexcept { return alignment_bytes(Cfg); }
    static void cacheline_prefetch(void const* p) noexcept { cacheline::prefetch<Cfg.sw_hint>(p); }
};

// ── Enumeration: alle 45 Konfigurationen (fuer Codegen/Registry, KF-8/KF-9) ──
[[nodiscard]] constexpr std::array<CacheLineConfig, 45> all_configs() noexcept {
    constexpr CacheLineSize      sizes[]  = {CacheLineSize::B64, CacheLineSize::B128, CacheLineSize::B256};
    constexpr CacheLineAlignment aligns[] = {CacheLineAlignment::None, CacheLineAlignment::CacheLineAligned,
                                             CacheLineAlignment::Padded};
    constexpr SwPrefetchHint     hints[]  = {SwPrefetchHint::None, SwPrefetchHint::T0, SwPrefetchHint::T1,
                                             SwPrefetchHint::T2, SwPrefetchHint::NTA};
    std::array<CacheLineConfig, 45> out{};
    std::size_t i = 0;
    for (auto s : sizes)
        for (auto a : aligns)
            for (auto h : hints) out[i++] = CacheLineConfig{s, a, h};
    return out;
}

inline constexpr std::uint32_t kCacheLineSubaxisVersion = 1;

}  // namespace comdare::cache_engine::cacheline
