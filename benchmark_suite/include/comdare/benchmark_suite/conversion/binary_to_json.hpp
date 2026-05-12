#pragma once
// BinaryToJson - Post-Experiment Conversion-Routine (REV 7 §8.2.5)

#include "../custom_allocation_1_measurements.hpp"

#include <filesystem>
#include <fstream>
#include <span>

namespace comdare::benchmark_suite::conversion {

class BinaryToJson {
public:
    void convert(std::span<MeasurementRecord32 const> records,
                 std::filesystem::path const& json_path) const
    {
        std::ofstream out{json_path};
        if (!out) throw std::runtime_error{"Could not open " + json_path.string()};

        out << "{\n  \"records\": [\n";
        bool first = true;
        for (auto const& r : records) {
            if (!first) out << ",\n";
            first = false;
            out << "    {\"timestamp_ns\":" << r.timestamp_ns
                << ",\"op_id\":"            << r.op_id
                << ",\"op_kind\":"          << r.op_kind
                << ",\"flags\":"            << r.flags
                << ",\"cycles_or_value\":"  << r.cycles_or_value
                << "}";
        }
        out << "\n  ]\n}\n";
    }
};

}  // namespace comdare::benchmark_suite::conversion
