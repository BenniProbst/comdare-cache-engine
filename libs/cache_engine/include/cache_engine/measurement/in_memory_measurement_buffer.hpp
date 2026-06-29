#pragma once
// InMemoryMeasurementBuffer — F5 zentrale Komponente, append-only, kein Lock im Hot-Path
// Termin 7 / 06_uml_persistence §1 + §4 (Disk-Dump-Format)

#include <cache_engine/measurement/measurement_record.hpp>
#include <cache_engine/measurement/thread_arena.hpp>

#include <chrono>
#include <map>
#include <mutex>
#include <thread>

namespace comdare::cache_engine::measurement {

enum class SerializationFormat : std::uint8_t {
    Binary = 0,
    Csv    = 1,
};

struct DumpHeader {
    char          magic[24]    = {'C', 'O', 'M', 'D', 'A', 'R', 'E', '-', 'M', 'E', 'A',  'S',
                                  'U', 'R', 'E', 'M', 'E', 'N', 'T', '-', 'V', '1', '\0', '\0'};
    std::uint32_t version      = 1;
    std::uint64_t run_id       = 0;
    std::uint64_t timestamp_ns = 0;
    std::uint64_t platform_sig = 0;
    std::uint64_t record_count = 0;
    std::uint32_t record_size  = sizeof(MeasurementRecord);
};

struct DumpFooter {
    std::uint64_t checksum       = 0;
    char          end_marker[12] = {'E', 'N', 'D', '-', 'C', 'O', 'M', 'D', 'A', 'R', 'E', '\0'};
};

using RunId = std::uint64_t;

class InMemoryMeasurementBuffer {
public:
    explicit InMemoryMeasurementBuffer(SerializationFormat fmt = SerializationFormat::Binary) : format_(fmt) {}

    // Hot-Path: pro-Thread-Arena, kein Lock
    void append_record(MeasurementRecord const& record) noexcept {
        ThreadArena& a = arena_for_current_thread();
        a.append(record);
    }

    [[nodiscard]] SerializationFormat format() const noexcept { return format_; }

    [[nodiscard]] std::size_t total_records() const noexcept {
        std::size_t total = 0;
        for (auto const& [_, arena] : arenas_) total += arena.size();
        return total;
    }

    [[nodiscard]] std::size_t arena_count() const noexcept { return arenas_.size(); }

    void reset_for_run(RunId run_id) noexcept {
        run_id_ = run_id;
        for (auto& [_, arena] : arenas_) arena.reset();
    }

    [[nodiscard]] RunId run_id() const noexcept { return run_id_; }

    [[nodiscard]] DumpHeader make_dump_header(std::uint64_t platform_sig) const noexcept {
        DumpHeader h{};
        h.run_id       = run_id_;
        h.platform_sig = platform_sig;
        h.record_count = total_records();
        h.timestamp_ns = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch())
                .count());
        return h;
    }

    // Pro-Thread Arena (Lazy-Allocation; Lock NUR fuer Map-Insert, NICHT fuer Append)
    [[nodiscard]] ThreadArena& arena_for_current_thread() noexcept {
        std::thread::id tid = std::this_thread::get_id();
        {
            std::lock_guard<std::mutex> g(arena_map_mutex_);
            auto                        it = arenas_.find(tid);
            if (it == arenas_.end()) { it = arenas_.emplace(tid, ThreadArena{tid}).first; }
            return it->second;
        }
    }

    [[nodiscard]] std::map<std::thread::id, ThreadArena> const& arenas() const noexcept { return arenas_; }

private:
    SerializationFormat                    format_;
    RunId                                  run_id_ = 0;
    std::map<std::thread::id, ThreadArena> arenas_{};
    mutable std::mutex                     arena_map_mutex_{};
};

} // namespace comdare::cache_engine::measurement
