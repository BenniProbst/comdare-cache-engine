#pragma once
// BinaryToCsv - Post-Experiment Conversion-Routine (REV 7 §8.2.5)
//
// PFLICHT: NIE waehrend Laufzeit aufgerufen — nur in Auswertung.

#include "../custom_allocation_1_measurements.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>

namespace comdare::benchmark_suite::conversion {

class BinaryToCsv {
public:
    void convert(std::span<MeasurementRecord32 const> records,
                 std::filesystem::path const& csv_path) const
    {
        std::ofstream out{csv_path};
        if (!out) throw std::runtime_error{"Could not open " + csv_path.string()};

        // Header
        out << "timestamp_ns,op_id,op_kind,flags,cycles_or_value\n";
        // Rows
        for (auto const& r : records) {
            out << r.timestamp_ns << ','
                << r.op_id        << ','
                << r.op_kind      << ','
                << r.flags        << ','
                << r.cycles_or_value << '\n';
        }
    }
};

}  // namespace comdare::benchmark_suite::conversion
