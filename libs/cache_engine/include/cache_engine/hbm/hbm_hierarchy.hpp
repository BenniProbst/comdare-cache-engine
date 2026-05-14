// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 BEP Venture UG (Marke Comdare)
//
// comdare::hbm — HBM-Hybrid-Cache-Hierarchie via Abstract Factory +
// Plattform-Adapter (Aufgabe #106).
//
// Designziel: einheitliche Abstraktion ueber:
//   - klassische CPU-Cache-Hierarchien (L1/L2/L3)
//   - NUMA-Knoten + DDR-RAM
//   - HBM (High-Bandwidth Memory, z. B. Intel MCDRAM, NVIDIA HBM3, AMD HBM)
//   - PMEM (Intel Optane) als 4. Tier
//
// Abstract Factory Pattern: pro Plattform ein Adapter, der zur Laufzeit
// `create_hierarchy()` aufruft. Cache-Engine konsumiert IHbmHierarchy
// fuer Pool-Affinity-Entscheidungen.

#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace comdare::hbm {

enum class TierKind : std::uint8_t {
    L1Cache       = 0,
    L2Cache       = 1,
    L3Cache       = 2,
    DRAM          = 3,
    HBM           = 4,    // High-Bandwidth Memory (MCDRAM, HBM3, ...)
    PMEM          = 5,    // Persistent Memory (Optane, ...)
    Disk          = 6,
};

struct TierInfo {
    TierKind      kind            = TierKind::DRAM;
    std::uint64_t size_bytes      = 0;
    std::uint32_t latency_ns      = 0;
    std::uint32_t bandwidth_gb_s  = 0;
    std::uint32_t numa_node       = 0;       // Physical NUMA node id
    std::string   label           = {};       // Human-readable name
};

// Abstract Interface fuer Plattform-spezifische Cache-Hierarchien
class IHbmHierarchy {
public:
    virtual ~IHbmHierarchy() = default;

    [[nodiscard]] virtual std::size_t tier_count() const noexcept = 0;
    [[nodiscard]] virtual TierInfo    tier_info(std::size_t idx) const = 0;
    [[nodiscard]] virtual bool        hbm_available() const noexcept = 0;
    [[nodiscard]] virtual bool        pmem_available() const noexcept = 0;
    [[nodiscard]] virtual std::string platform_name() const = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Konkrete Adapter
// ─────────────────────────────────────────────────────────────────────────────

class X86HbmHierarchy : public IHbmHierarchy {
public:
    X86HbmHierarchy() {
        tiers_ = {
            TierInfo{TierKind::L1Cache, 48 * 1024,         1,    1500, 0, "L1d"},
            TierInfo{TierKind::L2Cache, 1280 * 1024,       4,    600,  0, "L2"},
            TierInfo{TierKind::L3Cache, 18 * 1024 * 1024,  12,   300,  0, "L3"},
            TierInfo{TierKind::DRAM,    32ULL * (1ULL<<30), 100,  50,   0, "DDR5"},
            TierInfo{TierKind::HBM,     16ULL * (1ULL<<30), 60,   400,  1, "HBM2e"},
        };
    }

    [[nodiscard]] std::size_t tier_count() const noexcept override { return tiers_.size(); }

    [[nodiscard]] TierInfo tier_info(std::size_t idx) const override {
        return idx < tiers_.size() ? tiers_[idx] : TierInfo{};
    }

    [[nodiscard]] bool hbm_available() const noexcept override {
        for (auto const& t : tiers_) if (t.kind == TierKind::HBM) return true;
        return false;
    }
    [[nodiscard]] bool pmem_available() const noexcept override {
        for (auto const& t : tiers_) if (t.kind == TierKind::PMEM) return true;
        return false;
    }
    [[nodiscard]] std::string platform_name() const override { return "x86_64-hbm-numa"; }

private:
    std::vector<TierInfo> tiers_;
};

class ArmDdrHierarchy : public IHbmHierarchy {
public:
    ArmDdrHierarchy() {
        tiers_ = {
            TierInfo{TierKind::L1Cache, 64 * 1024,         2,    1200, 0, "L1d"},
            TierInfo{TierKind::L2Cache, 1024 * 1024,       6,    500,  0, "L2"},
            TierInfo{TierKind::L3Cache, 8 * 1024 * 1024,   18,   250,  0, "L3"},
            TierInfo{TierKind::DRAM,    16ULL * (1ULL<<30), 120,  35,   0, "LPDDR5"},
        };
    }

    [[nodiscard]] std::size_t tier_count() const noexcept override { return tiers_.size(); }
    [[nodiscard]] TierInfo    tier_info(std::size_t idx) const override {
        return idx < tiers_.size() ? tiers_[idx] : TierInfo{};
    }
    [[nodiscard]] bool        hbm_available() const noexcept override { return false; }
    [[nodiscard]] bool        pmem_available() const noexcept override { return false; }
    [[nodiscard]] std::string platform_name() const override { return "arm64-ddr"; }

private:
    std::vector<TierInfo> tiers_;
};

class GenericFallback : public IHbmHierarchy {
public:
    GenericFallback() {
        tiers_ = {
            TierInfo{TierKind::L1Cache, 32 * 1024,         3,    1000, 0, "L1"},
            TierInfo{TierKind::L2Cache, 256 * 1024,        10,   400,  0, "L2"},
            TierInfo{TierKind::DRAM,    4ULL * (1ULL<<30),  150,  20,   0, "DDR4"},
        };
    }

    [[nodiscard]] std::size_t tier_count() const noexcept override { return tiers_.size(); }
    [[nodiscard]] TierInfo    tier_info(std::size_t idx) const override {
        return idx < tiers_.size() ? tiers_[idx] : TierInfo{};
    }
    [[nodiscard]] bool        hbm_available() const noexcept override { return false; }
    [[nodiscard]] bool        pmem_available() const noexcept override { return false; }
    [[nodiscard]] std::string platform_name() const override { return "generic-fallback"; }

private:
    std::vector<TierInfo> tiers_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Abstract Factory
// ─────────────────────────────────────────────────────────────────────────────

enum class PlatformChoice : std::uint8_t {
    AutoDetect = 0,
    X86_Hbm    = 1,
    Arm_Ddr    = 2,
    Generic    = 3,
};

class HbmHierarchyFactory {
public:
    [[nodiscard]] static std::unique_ptr<IHbmHierarchy>
    create(PlatformChoice choice = PlatformChoice::AutoDetect) {
        if (choice == PlatformChoice::AutoDetect) choice = detect_platform();
        switch (choice) {
            case PlatformChoice::X86_Hbm: return std::make_unique<X86HbmHierarchy>();
            case PlatformChoice::Arm_Ddr: return std::make_unique<ArmDdrHierarchy>();
            case PlatformChoice::Generic:
            default:                       return std::make_unique<GenericFallback>();
        }
    }

    [[nodiscard]] static PlatformChoice detect_platform() noexcept {
        // Compile-time-Detection (Phase 5+ ersetzt durch Runtime via CPUID + /sys-Scan)
#if defined(__x86_64__) || defined(_M_X64) || defined(_M_AMD64)
        return PlatformChoice::X86_Hbm;
#elif defined(__aarch64__) || defined(_M_ARM64)
        return PlatformChoice::Arm_Ddr;
#else
        return PlatformChoice::Generic;
#endif
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Hilfsfunktionen
// ─────────────────────────────────────────────────────────────────────────────

[[nodiscard]] inline std::string tier_kind_name(TierKind k) noexcept {
    switch (k) {
        case TierKind::L1Cache: return "L1Cache";
        case TierKind::L2Cache: return "L2Cache";
        case TierKind::L3Cache: return "L3Cache";
        case TierKind::DRAM:    return "DRAM";
        case TierKind::HBM:     return "HBM";
        case TierKind::PMEM:    return "PMEM";
        case TierKind::Disk:    return "Disk";
    }
    return "Unknown";
}

}  // namespace comdare::hbm
