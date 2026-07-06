#pragma once
// sysfs_cache_probe.hpp — #265-b (AP-3-Follow-up, 2026-07-06): Linux-sysfs-Laufzeitprobe der Cache-Hierarchie.
// ABGELEITETER ce-Baustein zur vendored foundation-Zelle comdare::platform (Matrix-Reuse-Direktive: bestehende
// Module wiederverwenden, Erweiterungen als EIGENE abgeleitete Bausteine — der Vendor-Spiegel unter
// libs/common/platform/vendor/ bleibt byte-treu und wird hier NICHT angefasst).
// Zweck: Auf ARM (und exotischen x86) liefert CPUID keine L1/L2/L3-Groessen — die vendored CPUFeatures tragen
// dann 0. Diese Probe liest die REALE Hierarchie aus /sys/devices/system/cpu/cpu0/cache/index*/ (level, type,
// size, coherency_line_size) als LAUFZEIT-Fallback. Kein Modell, keine Kompile-Zeit-Annahme.

#include <cstdint>
#include <fstream>
#include <string>

namespace comdare::cache_engine::platform_probe {

struct SysfsCacheInfo {
    bool          available  = false; // true, sobald mindestens ein index*-Eintrag lesbar war
    std::uint32_t line_bytes = 0;     // coherency_line_size des ersten Daten-/Unified-Levels
    std::uint32_t l1_data_kb = 0;     // level==1 && type in {Data, Unified}
    std::uint32_t l2_kb      = 0;     // level==2 (Data/Unified)
    std::uint32_t l3_kb      = 0;     // level==3 (Data/Unified)
};

namespace detail {

[[nodiscard]] inline std::string read_sysfs_line(std::string const& path) {
    std::ifstream in{path};
    std::string   line;
    if (in && std::getline(in, line)) return line;
    return {};
}

// sysfs-"size" ist "32K"/"1024K"/"8M"/"36864K" — nach KiB normalisieren; unlesbar -> 0.
[[nodiscard]] inline std::uint32_t parse_size_kb(std::string const& s) {
    if (s.empty()) return 0;
    std::uint64_t value = 0;
    std::size_t   i     = 0;
    while (i < s.size() && s[i] >= '0' && s[i] <= '9') {
        value = value * 10u + static_cast<std::uint64_t>(s[i] - '0');
        ++i;
    }
    if (i >= s.size()) return 0;
    if (s[i] == 'K' || s[i] == 'k') return static_cast<std::uint32_t>(value);
    if (s[i] == 'M' || s[i] == 'm') return static_cast<std::uint32_t>(value * 1024u);
    if (s[i] == 'G' || s[i] == 'g') return static_cast<std::uint32_t>(value * 1024u * 1024u);
    return 0;
}

[[nodiscard]] inline std::uint32_t parse_u32(std::string const& s) {
    std::uint64_t value = 0;
    for (char c : s) {
        if (c < '0' || c > '9') break;
        value = value * 10u + static_cast<std::uint64_t>(c - '0');
    }
    return static_cast<std::uint32_t>(value);
}

} // namespace detail

/// Liest die Cache-Hierarchie von cpu0 aus sysfs (Linux). Auf Nicht-Linux bzw. ohne sysfs:
/// available=false, alle Werte 0 — der Aufrufer behandelt das als "keine Zweitquelle".
[[nodiscard]] inline SysfsCacheInfo probe_sysfs_cache() {
    SysfsCacheInfo info{};
#if defined(__linux__)
    std::string const base = "/sys/devices/system/cpu/cpu0/cache/index";
    for (int idx = 0; idx < 16; ++idx) {
        std::string const dir   = base + std::to_string(idx) + "/";
        std::string const level = detail::read_sysfs_line(dir + "level");
        if (level.empty()) continue; // Cross-Review 265-b: Luecken tolerieren statt abbrechen (idx-Obergrenze deckelt)
        std::string const type = detail::read_sysfs_line(dir + "type");
        if (type != "Data" && type != "Unified") continue; // Cross-Review: exotische Typen NICHT als Cache zaehlen
        std::uint32_t const lvl     = detail::parse_u32(level);
        std::uint32_t const size_kb = detail::parse_size_kb(detail::read_sysfs_line(dir + "size"));
        std::uint32_t const line    = detail::parse_u32(detail::read_sysfs_line(dir + "coherency_line_size"));
        if (size_kb == 0 && line == 0) continue; // Cross-Review: available nur bei VERWERTBAREM Treffer
        info.available = true;
        if (info.line_bytes == 0) info.line_bytes = line;
        if (lvl == 1 && info.l1_data_kb == 0) info.l1_data_kb = size_kb;
        if (lvl == 2 && info.l2_kb == 0) info.l2_kb = size_kb;
        if (lvl == 3 && info.l3_kb == 0) info.l3_kb = size_kb;
    }
#endif
    return info;
}

} // namespace comdare::cache_engine::platform_probe
