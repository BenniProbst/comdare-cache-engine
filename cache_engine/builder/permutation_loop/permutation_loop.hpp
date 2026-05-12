#pragma once
// PermutationLoop - Master-Enumeration aller gueltigen PermutationDescriptors
// (REV 7 §5.1 Phase 1 ENUMERATION + Phase 6 MEASURE)

#include "../xml_config_parser/xml_config_parser.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace comdare::builder::loop {

struct PermutationDescriptor {
    std::string   id;
    std::uint64_t fingerprint = 0;

    xml::PermutationEntry cache_engine_perm;
    xml::PermutationEntry search_algorithm_perm;
    xml::PermutationEntry allocator_perm;
    xml::PermutationEntry test_data_set;
};

class PermutationLoop {
public:
    [[nodiscard]] std::vector<PermutationDescriptor>
    enumerate(xml::CacheEngineConfig const& cfg) const;

    [[nodiscard]] static std::uint64_t compute_fingerprint(
        std::string_view ce_id, std::string_view sa_id,
        std::string_view alloc_id, std::string_view tds_id) noexcept;
};

}  // namespace comdare::builder::loop
