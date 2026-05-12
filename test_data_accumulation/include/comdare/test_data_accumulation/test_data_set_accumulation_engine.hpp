#pragma once
// TestDataSetAccumulationEngine - Klasse fuer Deserialization + Parsing + fair-aligned
// Bereitstellung der Test-Daten (REV 7 §7)
//
// Pflicht-Eigenschaften:
//   - Bei SearchEngine-Initialisierung mit-konstruiert
//   - Cache-line-aligned (oder Hugepage / NUMA-local) Buffer pro Dataset
//   - Test-Algo-Interface-View ueber Buffer
//   - Reset zwischen Experimenten ohne Buffer-Realloc

#include "alignment_strategy.hpp"

#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace comdare::test_data_accumulation {

struct DatasetView {
    std::string                 name;
    std::span<std::byte const>  bytes;
    std::size_t                 cursor    = 0;
    std::size_t                 record_size = 0;
};

template <typename SearchAlgo>
class TestDataSetAccumulationEngine {
public:
    using algo_t = SearchAlgo;

    explicit TestDataSetAccumulationEngine(SearchAlgo& bound_algo,
                                            AlignmentMode mode = AlignmentMode::CacheLine) noexcept
        : algo_{bound_algo}, mode_{mode} {}

    ~TestDataSetAccumulationEngine() {
        for (auto& [name, buf] : aligned_buffers_) {
            if (buf.ptr) aligned_free(buf.ptr);
        }
    }

    TestDataSetAccumulationEngine(TestDataSetAccumulationEngine const&) = delete;
    TestDataSetAccumulationEngine& operator=(TestDataSetAccumulationEngine const&) = delete;

    // Lade Dataset per Deserialization in cache-line-aligned Buffer
    void load_dataset(std::string const& name, std::filesystem::path const& file) {
        if (!std::filesystem::exists(file)) {
            throw std::runtime_error{"Dataset file not found: " + file.string()};
        }
        std::size_t const file_size = std::filesystem::file_size(file);
        void* buffer = aligned_alloc_for_mode(file_size, mode_);
        if (!buffer) throw std::bad_alloc{};

        std::ifstream in{file, std::ios::binary};
        in.read(static_cast<char*>(buffer), static_cast<std::streamsize>(file_size));
        if (!in) {
            aligned_free(buffer);
            throw std::runtime_error{"Failed to read dataset: " + file.string()};
        }

        AlignedBuffer ab;
        ab.ptr  = buffer;
        ab.size = file_size;
        aligned_buffers_[name] = ab;
    }

    // In-Memory-Variante: lade Dataset aus User-Buffer (z.B. generated YCSB)
    void load_dataset_from_memory(std::string const& name,
                                   std::span<std::byte const> source,
                                   std::size_t record_size = 0) {
        void* buffer = aligned_alloc_for_mode(source.size(), mode_);
        if (!buffer) throw std::bad_alloc{};
        std::memcpy(buffer, source.data(), source.size());
        AlignedBuffer ab;
        ab.ptr         = buffer;
        ab.size        = source.size();
        ab.record_size = record_size;
        aligned_buffers_[name] = ab;
    }

    [[nodiscard]] bool all_loaded() const noexcept {
        for (auto const& [name, buf] : aligned_buffers_) {
            if (!buf.ptr) return false;
        }
        return !aligned_buffers_.empty();
    }

    [[nodiscard]] std::span<std::byte const> get_dataset(std::string_view name) const noexcept {
        auto it = aligned_buffers_.find(std::string{name});
        if (it == aligned_buffers_.end()) return {};
        return std::span<std::byte const>{
            reinterpret_cast<std::byte const*>(it->second.ptr),
            it->second.size
        };
    }

    [[nodiscard]] DatasetView make_view(std::string_view name) const {
        DatasetView view;
        view.name = name;
        auto it = aligned_buffers_.find(std::string{name});
        if (it != aligned_buffers_.end()) {
            view.bytes       = std::span<std::byte const>{
                reinterpret_cast<std::byte const*>(it->second.ptr),
                it->second.size
            };
            view.record_size = it->second.record_size;
        }
        return view;
    }

    void reset_cursors() noexcept {
        // Cursor-Reset waere DatasetView-Property; hier no-op
    }

    [[nodiscard]] std::size_t loaded_count() const noexcept { return aligned_buffers_.size(); }

private:
    struct AlignedBuffer {
        void*       ptr        = nullptr;
        std::size_t size       = 0;
        std::size_t record_size = 0;
    };

    SearchAlgo&                                       algo_;
    AlignmentMode                                     mode_;
    std::unordered_map<std::string, AlignedBuffer>     aligned_buffers_;
};

}  // namespace comdare::test_data_accumulation
