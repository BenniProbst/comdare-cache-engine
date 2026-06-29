// V42 L-74c — ObservableSerialization<Strategy> Driver-Baustein-Test (2026-06-03). 3. Exemplar des Musters.
// Build: scratch_compile_test.ps1 -Test test_d_v42_serialization_observable -Boost -Extra @("build\generated")

#define COMDARE_CE_ENABLE_STATISTICS 1

#include <axes/serialization_axis/axis_10_serialization_observable.hpp>
#include <axes/serialization_axis/axis_10_serialization_raw_binary.hpp>
#include "anatomy/observer_aggregate.hpp" // ObservableAxis

#include <cassert>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <type_traits>
#include <vector>

namespace s = comdare::cache_engine::serialization_axis;
namespace a = comdare::cache_engine::anatomy;

int main() {
    using Strat = s::RawBinarySerialization;
    using Obs   = s::ObservableSerialization<Strat>;

    static_assert(std::is_aggregate_v<Strat>, "Strategie ist Aggregat");
    static_assert(!a::ObservableAxis<Strat>, "nackte Strategie hat kein statistics()");
    static_assert(a::ObservableAxis<Obs>, "Huelle muss ObservableAxis sein");
    static_assert(!std::is_aggregate_v<Obs>, "Huelle darf kein Aggregat sein");
    static_assert(std::is_standard_layout_v<s::SerializationSnapshot> &&
                      std::is_trivially_copyable_v<s::SerializationSnapshot>,
                  "Snapshot ABI-tauglich");

    constexpr std::size_t      record_size = 8, n = 4;
    std::vector<unsigned char> buf(record_size * n, 0);
    std::uint32_t const        vals[n] = {10u, 20u, 30u, 40u}; // Summe = 100
    for (std::size_t i = 0; i < n; ++i) std::memcpy(buf.data() + i * record_size, &vals[i], sizeof(std::uint32_t));

    // static Pass-Through trackt NICHT (Drop-in fuer abi_adapter):
    assert(Obs::serialize_scan(buf.data(), n, record_size) == 100u);

    Obs ser;
    assert(ser.statistics().serialize_count == 0);
    std::uint64_t const checksum = ser.observe_serialize(buf.data(), n, record_size); // Instanz-Driver: trackt
    auto const          after    = ser.statistics();
    std::cout << "serialize_count=" << after.serialize_count << " records=" << after.records_serialized
              << " bytes=" << after.bytes_serialized << " checksum=" << after.last_checksum << "\n";

    assert(checksum == 100u);
    assert(after.serialize_count == 1u);
    assert(after.records_serialized == 4u);
    assert(after.bytes_serialized == 32u); // 4 * 8
    assert(after.last_checksum == 100u);
    assert(after.serialize_count != 0u); // Delta > 0

    ser.reset();
    assert(ser.statistics().serialize_count == 0u);

    std::cout
        << "OK: serialization-Achse ist echte getriebene ObservableAxis (serialize_scan durchgereicht + getrackt).\n";
    return 0;
}
