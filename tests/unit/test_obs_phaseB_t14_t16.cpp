// Phase B (2026-06-04) STANDALONE-COMPILE-VERIFIKATION der 3 vom Hauptagenten entschiedenen Achsen-Hüllen:
//   T14 ObservableIoDispatch<S>  (IoDispatchSnapshot)
//   T15 ObservableMigration<S>   (MigrationSnapshot)
//   T16 ObservableFilter<S>      (FilterStatistics)
// Prüft je: (a) static Pass-Through-Scan ruft die Strategie, (b) der observe_*-Instanz-Driver trackt unter
// COMDARE_CE_ENABLE_STATISTICS, (c) ObservableAxis<Hülle> == true (snapshot_t + statistics()), (d) der Snapshot
// ist standard_layout + trivially_copyable (Cross-ABI-POD-Pflicht). Reine Header-Verifikation (keine abi_adapter-
// Abhängigkeit) — bestätigt, dass die Hüllen die Achsen-Concepts erfüllen und die Decorator-Mechanik kompiliert.

#include <axes/io_dispatch/axis_io_dispatch_observable.hpp>
#include <axes/io_dispatch/axis_io_in_memory_only.hpp>
#include <axes/io_dispatch/axis_io_buffered.hpp>

#include <axes/migration_policy/axis_migration_observable.hpp>
#include <axes/migration_policy/axis_migration_none.hpp>
#include <axes/migration_policy/axis_migration_hot_cold.hpp>

#include <axes/filter_axis/axis_filter_observable.hpp>
#include <axes/filter_axis/axis_filter_bloom.hpp>
#include <axes/filter_axis/axis_filter_cuckoo.hpp>

#include <anatomy/observer_aggregate.hpp> // ObservableAxis-Concept

#include <array>
#include <cstdint>
#include <iostream>
#include <type_traits>

namespace io = ::comdare::cache_engine::io_dispatch;
namespace mg = ::comdare::cache_engine::migration_policy;
namespace ft = ::comdare::cache_engine::filter_axis;
namespace an = ::comdare::cache_engine::anatomy;

static int  g_fail = 0;
static void tr(char const* w, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n";
    if (!c) ++g_fail;
}

// ── ObservableAxis muss für ALLE Hüllen gelten (sonst fiele observe_all auf EmptyAxisSnapshot zurück) ──
static_assert(an::ObservableAxis<io::ObservableIoDispatch<io::InMemoryOnly>>);
static_assert(an::ObservableAxis<io::ObservableIoDispatch<io::BufferedIo>>);
static_assert(an::ObservableAxis<mg::ObservableMigration<mg::NoMigration>>);
static_assert(an::ObservableAxis<mg::ObservableMigration<mg::HotColdMigration>>);
static_assert(an::ObservableAxis<ft::ObservableFilter<ft::BloomFilter>>);
static_assert(an::ObservableAxis<ft::ObservableFilter<ft::CuckooFilter>>);

// ── Cross-ABI-POD-Pflicht ──
static_assert(std::is_standard_layout_v<io::IoDispatchSnapshot> &&
              std::is_trivially_copyable_v<io::IoDispatchSnapshot>);
static_assert(std::is_standard_layout_v<mg::MigrationSnapshot> && std::is_trivially_copyable_v<mg::MigrationSnapshot>);
static_assert(std::is_standard_layout_v<ft::FilterStatistics> && std::is_trivially_copyable_v<ft::FilterStatistics>);

int main() {
    constexpr std::size_t                    kRecords = 2048, kRecordSize = 48, kQueries = 256;
    std::array<unsigned char, kRecords * 64> buf{};
    for (std::size_t i = 0; i < buf.size(); ++i) buf[i] = static_cast<unsigned char>(i * 31u + 7u);
    std::array<unsigned char, kQueries> q{};
    for (std::size_t i = 0; i < kQueries; ++i) q[i] = static_cast<unsigned char>(i * 53u + 11u);

    // T14 io_dispatch — Buffered (alignment_adjusts > 0, da !is_in_memory_only)
    {
        io::ObservableIoDispatch<io::BufferedIo> h;
        std::uint64_t const                      a =
            io::ObservableIoDispatch<io::BufferedIo>::io_dispatch_scan(buf.data(), kRecords, kRecordSize);
        std::uint64_t const b = h.observe_dispatch(buf.data(), kRecords, kRecordSize);
        auto const          s = h.statistics();
        tr("T14 static==driver", a == b);
        tr("T14 dispatch_rounds==1", s.dispatch_rounds == 1);
        tr("T14 total_dispatch_count==kRecords", s.total_dispatch_count == kRecords);
        tr("T14 bytes_dispatched==kRecords*recsize", s.bytes_dispatched == std::uint64_t{kRecords} * kRecordSize);
        tr("T14 alignment_adjusts>0 (non-in-memory)", s.alignment_adjusts == kRecords);
    }
    // T14 InMemoryOnly — alignment_adjusts honest 0
    {
        io::ObservableIoDispatch<io::InMemoryOnly> h;
        (void)h.observe_dispatch(buf.data(), kRecords, kRecordSize);
        tr("T14 InMemory alignment_adjusts==0", h.statistics().alignment_adjusts == 0);
    }
    // T15 migration — HotCold (active → votes), None (baseline → 0)
    {
        mg::ObservableMigration<mg::HotColdMigration> h;
        h.observe_decide(buf.data(), kRecords, kRecordSize);
        auto const s = h.statistics();
        tr("T15 total_decisions==kRecords", s.total_decisions == kRecords);
        tr("T15 hot+cold==total (active)", s.hot_votes + s.cold_votes == s.total_decisions);
        tr("T15 tier_moves==0 (honest)", s.tier_moves == 0);
        tr("T15 migrations_triggered>0 (active)", s.migrations_triggered == kRecords);

        mg::ObservableMigration<mg::NoMigration> hn;
        hn.observe_decide(buf.data(), kRecords, kRecordSize);
        auto const sn = hn.statistics();
        tr("T15 None migrations_triggered==0", sn.migrations_triggered == 0);
        tr("T15 None votes==0 (baseline)", sn.hot_votes == 0 && sn.cold_votes == 0);
        tr("T15 None total_decisions==kRecords", sn.total_decisions == kRecords);
    }
    // T16 filter — Bloom (positive+negative==q)
    {
        ft::ObservableFilter<ft::BloomFilter> h;
        std::uint64_t const                   a =
            ft::ObservableFilter<ft::BloomFilter>::filter_probe_scan(buf.data(), kRecords, q.data(), kQueries);
        std::uint64_t const b = h.observe_probe(buf.data(), kRecords, q.data(), kQueries);
        auto const          s = h.statistics();
        tr("T16 static==driver", a == b);
        tr("T16 probe_count==1", s.probe_count == 1);
        tr("T16 pos+neg==q", s.queries_positive + s.queries_negative == kQueries);
        tr("T16 hash_probes_total>0", s.hash_probes_total > 0);
    }

    std::cout << (g_fail == 0 ? "ALL-GREEN\n" : "FAILURES\n");
    return g_fail == 0 ? 0 : 1;
}
