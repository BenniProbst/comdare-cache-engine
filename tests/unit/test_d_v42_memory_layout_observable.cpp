// V42 L-74c — ObservableMemoryLayout<Strategy> Driver-Baustein-Test (2026-06-03). Belegt LITERAL (MIT
// COMDARE_CE_ENABLE_STATISTICS), dass die memory_layout-Achse jetzt eine echte, getriebene ObservableAxis ist.
// Enthaelt die Pflicht-Probe (reale Strategie CacheLineAlignedMemoryLayout) + den Mechanik-Delta-Test.
// Build: scratch_compile_test.ps1 -Test test_d_v42_memory_layout_observable -Boost -Extra @("build\generated")

#define COMDARE_CE_ENABLE_STATISTICS 1

#include <axes/layout/axis_05_memory_layout_observable.hpp>
#include <axes/layout/axis_05_memory_layout_cache_line_aligned.hpp>
#include "anatomy/observer_aggregate.hpp" // ObservableAxis

#include <cassert>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <type_traits>
#include <vector>

namespace l = comdare::cache_engine::layout;
namespace a = comdare::cache_engine::anatomy;

// M-CE-22 / NDEBUG-No-Op-Fix (2026-07-13, Muster-F): harte Checks statt assert(). assert() ist unter NDEBUG
// (Release) ein No-Op -> dieser Test degenerierte im Release-Build zu `return 0` (kein echter Beweis). CE_CHECK
// prueft NDEBUG-unabhaengig und liefert bei Verletzung `return 1` (rot). static_assert() bleibt (compile-time).
#define CE_CHECK(cond)                                                                                                 \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            std::cerr << "[FAIL] " #cond " @ " << __FILE__ << ":" << __LINE__ << "\n";                                 \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)

int main() {
    using Strat = l::CacheLineAlignedMemoryLayout;
    using Obs   = l::ObservableMemoryLayout<Strat>;

    // ── Pflicht-Probe (realer Wrapper): Aggregat + nicht observable; Huelle observable + kein Aggregat ──
    static_assert(std::is_aggregate_v<Strat>, "Strategie ist (wie telemetry) ein Aggregat");
    static_assert(!a::ObservableAxis<Strat>, "nackte Strategie hat kein statistics()");
    static_assert(a::ObservableAxis<Obs>, "Huelle muss ObservableAxis sein (STATISTICS)");
    static_assert(!std::is_aggregate_v<Obs>, "Huelle darf kein Aggregat sein");
    static_assert(std::is_standard_layout_v<l::MemoryLayoutSnapshot> &&
                      std::is_trivially_copyable_v<l::MemoryLayoutSnapshot>,
                  "Snapshot ABI-tauglich");

    // ── Mechanik: 4 Datensaetze a 64 Byte (AoS), je ein uint32 am Record-Start ──
    constexpr std::size_t      record_size = 64, n = 4;
    std::vector<unsigned char> buf(record_size * n, 0);
    std::uint32_t const        vals[n] = {10u, 20u, 30u, 40u}; // Summe = 100
    for (std::size_t i = 0; i < n; ++i) std::memcpy(buf.data() + i * record_size, &vals[i], sizeof(std::uint32_t));

    // static Pass-Through (Drop-in-Kompatibilität) trackt NICHT:
    CE_CHECK(Obs::scan_field_sum(buf.data(), n, record_size) == 100u);

    Obs        layout;
    auto const before = layout.statistics();
    CE_CHECK(before.scan_count == 0);

    std::uint64_t const checksum = layout.observe_scan(buf.data(), n, record_size); // Instanz-Driver: trackt
    auto const          after    = layout.statistics();
    std::cout << "scan_count=" << after.scan_count << " records=" << after.records_scanned
              << " field_bytes=" << after.field_bytes_read << " cache_lines=" << after.cache_lines_touched
              << " checksum=" << after.last_checksum << "\n";

    CE_CHECK(checksum == 100u); // Korrektheit der durchgereichten Strategie-Methode
    CE_CHECK(after.scan_count == 1u);
    CE_CHECK(after.records_scanned == 4u);
    // P-MD1-ERDUNG #167: der generische Raw-Buffer-Pfad (observe_scan ohne realen Store) bucht n * sizeof(uint64)
    // Key-Bytes; die layout-distinkte REALE CLU kommt seitdem aus observe_real_footprint (Store-Pfad).
    CE_CHECK(after.field_bytes_read == 32u);   // 4 * 8 Byte (generischer Key-Footprint, s.o.)
    CE_CHECK(after.cache_lines_touched == 4u); // 4 * ceil(64/64) = 4 (AoS strided, 1 Cache-Line/Record)
    CE_CHECK(after.last_checksum == 100u);
    CE_CHECK(!(after == before)); // Delta > 0 (kein Stub)

    // zweiter Scan akkumuliert
    (void)layout.observe_scan(buf.data(), n, record_size);
    CE_CHECK(layout.statistics().scan_count == 2u);
    CE_CHECK(layout.statistics().records_scanned == 8u);

    layout.reset();
    CE_CHECK(layout.statistics().scan_count == 0u);

    std::cout
        << "OK: memory_layout-Achse ist echte getriebene ObservableAxis (scan_field_sum durchgereicht + getrackt).\n";
    return 0;
}
