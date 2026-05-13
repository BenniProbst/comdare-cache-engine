// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 BEP Venture UG (Marke Comdare)
//
// P14-Samuel — Re-Implementation in PRT-ART
// Paper: Making CSB+-Trees Processor Conscious (Samuel/Pedersen/Bonnet 2005)
// Concept: Page (Achse: Page-Type)
//
// Kernidee: Configuration Table, indexed by (key size, value size, update ratio).

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace comdare::prt_art::legacy_reimpl::samuel {

enum class UpdateRatioBin : std::uint8_t {
    ReadDominant   = 0,
    Mixed          = 1,
    WriteHeavy     = 2
};

struct ConfigKey {
    std::uint8_t   key_size_bin    = 0;
    std::uint8_t   value_size_bin  = 0;
    UpdateRatioBin update_bin      = UpdateRatioBin::ReadDominant;

    [[nodiscard]] bool operator==(ConfigKey const& o) const noexcept {
        return key_size_bin == o.key_size_bin
            && value_size_bin == o.value_size_bin
            && update_bin == o.update_bin;
    }
};

struct ConfigRecommendation {
    std::uint16_t recommended_cache_lines_per_node = 1;
    std::uint16_t recommended_fanout               = 7;
    bool          enable_sibling_clusters          = false;
    bool          enable_prefetch                  = false;
};

class ConfigurationTable {
public:
    void insert(ConfigKey k, ConfigRecommendation r) {
        table_.push_back({k, r});
    }

    [[nodiscard]] std::optional<ConfigRecommendation>
    lookup(ConfigKey k) const noexcept {
        for (auto const& [key, rec] : table_) {
            if (key == k) return rec;
        }
        return std::nullopt;
    }

    [[nodiscard]] std::size_t size() const noexcept { return table_.size(); }

private:
    std::vector<std::pair<ConfigKey, ConfigRecommendation>> table_{};
};

}  // namespace comdare::prt_art::legacy_reimpl::samuel
