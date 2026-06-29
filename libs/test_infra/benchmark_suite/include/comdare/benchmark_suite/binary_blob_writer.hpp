#pragma once
// BinaryBlobWriter - End-of-Experiment Konsolidierung (REV 7 §8.2.4)
//
// Header + Records + Footer Layout:
//   [magic:4B "CDB1"] [version:4B] [record_count:8B] [state_log_bytes:8B]
//   [records: record_count * 32B]
//   [state_log: state_log_bytes]
//   [footer: CRC64 4B + magic_end:4B "END!"]

#include "benchmark_runner.hpp"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <span>

namespace comdare::benchmark_suite {

inline constexpr std::uint32_t kBlobMagic    = 0x31424443; // "CDB1"
inline constexpr std::uint32_t kBlobVersion  = 1;
inline constexpr std::uint32_t kBlobMagicEnd = 0x21444E45; // "END!"

class BinaryBlobWriter {
public:
    void write(std::filesystem::path const& path, BenchmarkRunner const& runner) const {
        std::ofstream out{path, std::ios::binary};
        if (!out) throw std::runtime_error{"Could not open " + path.string()};

        auto records   = runner.measurements().snapshot();
        auto state_log = runner.state_log().snapshot();

        // Header
        write_u32(out, kBlobMagic);
        write_u32(out, kBlobVersion);
        write_u64(out, records.size());
        write_u64(out, state_log.size());

        // Records (32B aligned writes)
        if (!records.empty()) {
            out.write(reinterpret_cast<char const*>(records.data()),
                      static_cast<std::streamsize>(records.size_bytes()));
        }
        // State log
        if (!state_log.empty()) {
            out.write(reinterpret_cast<char const*>(state_log.data()),
                      static_cast<std::streamsize>(state_log.size_bytes()));
        }
        // Footer (placeholder CRC = 0; Phase 7 ergaenzt echte CRC64)
        write_u32(out, 0);
        write_u32(out, kBlobMagicEnd);
    }

private:
    static void write_u32(std::ostream& os, std::uint32_t v) { os.write(reinterpret_cast<char const*>(&v), sizeof(v)); }
    static void write_u64(std::ostream& os, std::uint64_t v) { os.write(reinterpret_cast<char const*>(&v), sizeof(v)); }
};

} // namespace comdare::benchmark_suite

namespace comdare::benchmark_suite {

inline void BenchmarkRunner::flush_to_binary_blob(std::filesystem::path const& output) const {
    BinaryBlobWriter writer;
    writer.write(output, *this);
}

} // namespace comdare::benchmark_suite
