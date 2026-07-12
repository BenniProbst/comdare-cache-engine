#pragma once
// F-B (GO4/#8, 2026-07-12) — alloc_hw_config.hpp: die NUMA/Page->allocator-Unterachse (Achse-12-Kante).
//
// Grundlage (Dossier GO4 §1/F-B, super docs/sessions/backups/20260712-go4-f-abc-dossier/DOSSIER.md): die
// general_hardware-Eigenschaften `numa_capable` / `memory_page_size` / `huge_page_capable` (axis_12-Profile,
// topics/hardware/axis_12_general_hardware/) fliessen als COMPILE-TIME-Inputs in die Allokator-Achse
// (axis_06) — NUR an der Allocator-Kante, HW-als-Typ-Parameter, `if constexpr`-Gating (numa_capable=false
// -> NUMA-Pinning wegkompiliert). Thesis-Motivation: AA5/NUMAlloc A09 (anhang D_building_block_matrix) +
// huge pages/dTLB-Reach (kapitel 02_fundamentals; FF3 dTLB-Misses). Vor F-B waren memory_page_size/
// huge_page_capable NIRGENDS konsumiert (honest-0, Dossier §2.1) — diese Kante ist ihr erster Konsument.
//
// Muster = die node_width-Unterachse (C2/FF2, axes/cacheline/node_width_config.hpp), exakt gespiegelt:
//   - NTTP-faehige Config (strukturell) + CRTP-Mixin AllocHwAware<Cfg> + Concept AllocHwConfigurable.
//   - Compile-time only (kein Runtime-Switch im Hot-Path); Default = Auto/Native -> Verhalten byte-identisch
//     (NUMAlloc kDefaultNumaNode -1 = kernel-Default; kein Page-Hint).
//   - Binary-id-relevant NUR profil-aktiviert (profile_to_tree.hpp: ref=="alloc_hw" -> statische Sub-Ebenen
//     alloc_hw.numa_node {auto,0,1} + alloc_hw.page {4k,2m}); ohne Profil-Aktivierung KEINE binary_id-Aenderung.
//   - Realer Konsum: NUMAllocAllocatorBody (numa_node_ = compile-time Parameter statt Hartcode -1,
//     axis_06_allocator_numalloc.hpp) + PoolResourceAllocatorBody (Page-Hint -> pmr::pool_options,
//     axis_06_allocator_pool_resource.hpp).
//
// HW-GATE-GRENZE (ehrlich dokumentiert): der NUMA-EFFEKT braucht Multi-Socket-Hardware (HW-gated ~Sep);
// auf Single-Socket-prod1 ist der Effekt klein/null. Der KNOPF existiert + ist dokumentiert — Zweck lt.
// User-Entscheid: "als Permutation und dokumentiert den Effekt-Unterschied bewerten".

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::alloc {

// ── Wertraum 1: NUMA-Origin (3 Messwerte {auto,0,1}; Auto = kernel-Default = bisheriges -1) ──────────────
// Auto IST ein Messwert (Baseline der Effekt-Differenz auto-vs-gepinnt) UND der Aus-Default der Unterachse —
// dadurch bleiben alle bestehenden Organe/Binaries byte-identisch (NUMAlloc verhielt sich immer wie Auto).
enum class AllocNumaNode : std::int8_t { Auto = -1, Node0 = 0, Node1 = 1 };

// ── Wertraum 2: Page-Hint (2 Messwerte {4k,2m}; Native = KEIN Messwert, Aus-Default wie node_width) ──────
// Page4k = Basis-Page der Plattform (aufgeloest via HW::memory_page_size(), heute 4096 auf allen 3 Profilen);
// Page2m = 2-MiB-Huge-Page-Hint (nur wenn HW::huge_page_capable(), sonst weggegated auf Page4k).
enum class AllocPageHint : std::uint8_t { Native = 0, Page4k = 1, Page2m = 2 };

/// 2-MiB-Huge-Page in Bytes (Linux MAP_HUGETLB-Standardgroesse; thesis 02_fundamentals "2 MiB/1 GiB").
inline constexpr std::size_t kAllocHugePage2mBytes = 2u * 1024u * 1024u;

/// Compile-Time-Konfiguration der NUMA/Page-Unterachse EINES Allocator-Organs. Strukturell -> als NTTP nutzbar.
struct AllocHwConfig {
    AllocNumaNode numa_node = AllocNumaNode::Auto;
    AllocPageHint page      = AllocPageHint::Native;

    [[nodiscard]] constexpr bool operator==(AllocHwConfig const&) const noexcept = default;
};

// ── Concept: ein Organ traegt die NUMA/Page-Unterachse ───────────────────────────────────────────────────
template <typename T>
concept AllocHwConfigurable = requires {
    { T::alloc_hw_config() } -> std::same_as<AllocHwConfig>;
};

// ── Concept: die axis_12-Eigenschafts-Quelle (HW-als-Typ-Parameter, F-B-Kante) ───────────────────────────
// Strukturell (kein Include der hardware-topic-Header noetig -> Layering bleibt flach wie node_width_config);
// erfuellt von GeneralHardwareStrategy-Profilen (Generic/X86_64/Aarch64, axis_12_general_hardware_*.hpp).
template <typename HW>
concept AllocHwPlatformProfile = requires {
    { HW::memory_page_size() } -> std::convertible_to<std::size_t>;
    { HW::numa_capable() } -> std::convertible_to<bool>;
    { HW::huge_page_capable() } -> std::convertible_to<bool>;
};

// ── CRTP-Mixin: Allocator-Organe erben dies (via AllocatorStrategyBase), um die Unterachse zu tragen ─────
/// Cfg ist die per-Organ-Konfiguration (vom Codegen/Profil je Organ gesetzt). Auto/Native = keine Vorgabe.
template <AllocHwConfig Cfg>
struct AllocHwAware {
    [[nodiscard]] static constexpr AllocHwConfig alloc_hw_config() noexcept { return Cfg; }
    /// NUMA-Node der Allokations-Bindung (-1 = Auto = kernel-Default, byte-identisch zum Ist-Stand).
    [[nodiscard]] static constexpr int alloc_hw_numa_node() noexcept { return static_cast<int>(Cfg.numa_node); }
    /// Page-Hint der Unterachse (Native = kein Hint).
    [[nodiscard]] static constexpr AllocPageHint alloc_hw_page() noexcept { return Cfg.page; }
    /// Page-Hint in Bytes (0 = Native/kein Hint). base_page_bytes-Default 4096 = x86-64-Bezugsgroesse
    /// (Muster node_width_bytes(line_bytes = 64)); HW-treu aufloesen: alloc_hw_page_bytes_for<HW>().
    [[nodiscard]] static constexpr std::size_t alloc_hw_page_bytes(std::size_t base_page_bytes = 4096) noexcept {
        return Cfg.page == AllocPageHint::Native   ? std::size_t{0}
               : Cfg.page == AllocPageHint::Page4k ? base_page_bytes
                                                   : kAllocHugePage2mBytes;
    }
};

// ── HW-Gate (DIE F-B-Mechanik, Dossier §1/F-B "Mechanismus"): if constexpr ueber die axis_12-Properties ──
/// Reduziert eine angeforderte Konfiguration auf das, was die Plattform compile-time hergibt:
/// numa_capable()==false  -> Node-Pinning WEGKOMPILIERT (numa_node := Auto);
/// huge_page_capable()==false -> 2m-Hint WEGKOMPILIERT (page := Page4k, Basis-Page).
/// Konsumiert damit `numa_capable` + `huge_page_capable` als Typ-Parameter an der Allocator-Kante.
template <AllocHwPlatformProfile HW>
[[nodiscard]] constexpr AllocHwConfig gate_alloc_hw_for(AllocHwConfig requested) noexcept {
    AllocHwConfig c = requested;
    if constexpr (!HW::numa_capable()) c.numa_node = AllocNumaNode::Auto;
    if constexpr (!HW::huge_page_capable()) {
        if (c.page == AllocPageHint::Page2m) c.page = AllocPageHint::Page4k;
    }
    return c;
}

/// Page-Hint HW-treu in Bytes aufloesen — konsumiert `memory_page_size` (+ huge_page-Gate) als Typ-Parameter.
template <AllocHwPlatformProfile HW>
[[nodiscard]] constexpr std::size_t alloc_hw_page_bytes_for(AllocPageHint p) noexcept {
    if (p == AllocPageHint::Native) return 0;
    if (p == AllocPageHint::Page4k) return HW::memory_page_size();
    if constexpr (HW::huge_page_capable()) {
        return kAllocHugePage2mBytes;
    } else {
        return HW::memory_page_size(); // 2m ohne huge-page-Faehigkeit -> Basis-Page (weggegated)
    }
}

// ── Enumeration: die Messwerte der F-B-Unterachse (fuer Codegen/Registry) ────────────────────────────────
[[nodiscard]] constexpr std::array<AllocNumaNode, 3> all_alloc_numa_nodes() noexcept {
    return {AllocNumaNode::Auto, AllocNumaNode::Node0, AllocNumaNode::Node1};
}
[[nodiscard]] constexpr std::array<AllocPageHint, 2> all_alloc_page_hints() noexcept {
    return {AllocPageHint::Page4k, AllocPageHint::Page2m}; // Native gehoert NICHT dazu (Aus-Default)
}

/// Compile-Time-Factory aus dem Profil-Wert (Bruecke fuer Codegen/Tests, Muster make_node_width).
/// v in {"auto","0","1"}; alles andere -> Auto (konservativer Aus-Default, kein Raten).
[[nodiscard]] constexpr AllocNumaNode make_alloc_numa_node(std::string_view v) noexcept {
    return (v == "0") ? AllocNumaNode::Node0 : (v == "1") ? AllocNumaNode::Node1 : AllocNumaNode::Auto;
}
/// v in {"4k","2m"}; alles andere -> Native (konservativer Aus-Default, kein Raten).
[[nodiscard]] constexpr AllocPageHint make_alloc_page_hint(std::string_view v) noexcept {
    return (v == "4k") ? AllocPageHint::Page4k : (v == "2m") ? AllocPageHint::Page2m : AllocPageHint::Native;
}

inline constexpr std::uint32_t kAllocHwSubaxisVersion = 1;

} // namespace comdare::cache_engine::alloc
