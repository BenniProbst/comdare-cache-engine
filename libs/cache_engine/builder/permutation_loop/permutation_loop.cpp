#include "permutation_loop.hpp"
#include "permutation_sampling.hpp"

#include <functional>

namespace comdare::builder::loop {

std::vector<PermutationDescriptor> PermutationLoop::enumerate(xml::CacheEngineConfig const& cfg,
                                                              std::uint32_t                 sample_rate,
                                                              std::uint64_t                 sample_seed) const {
    std::vector<PermutationDescriptor> result;
    result.reserve(cfg.cache_engine_permutations.size() * cfg.search_algorithm_permutations.size() *
                   cfg.allocator_permutations.size() * cfg.test_data_sets.size());

    for (auto const& ce : cfg.cache_engine_permutations) {
        for (auto const& sa : cfg.search_algorithm_permutations) {
            for (auto const& al : cfg.allocator_permutations) {
                for (auto const& tds : cfg.test_data_sets) {
                    PermutationDescriptor d;
                    d.id                    = ce.id + ":" + sa.id + ":" + al.id + ":" + tds.id;
                    d.fingerprint           = compute_fingerprint(ce.id, sa.id, al.id, tds.id);
                    d.cache_engine_perm     = ce;
                    d.search_algorithm_perm = sa;
                    d.allocator_perm        = al;
                    d.test_data_set         = tds;
                    if (sample_rate >= sampling_min_filtered_rate && !sample_keep(d.id, sample_rate, sample_seed))
                        continue;
                    result.push_back(std::move(d));
                }
            }
        }
    }
    return result;
}

std::uint64_t PermutationLoop::compute_fingerprint(std::string_view ce_id, std::string_view sa_id,
                                                   std::string_view alloc_id, std::string_view tds_id) noexcept {
    std::hash<std::string_view> hasher;
    std::uint64_t               h = 0xCBF29CE484222325ULL;
    h ^= hasher(ce_id);
    h *= 0x100000001B3ULL;
    h ^= hasher(sa_id);
    h *= 0x100000001B3ULL;
    h ^= hasher(alloc_id);
    h *= 0x100000001B3ULL;
    h ^= hasher(tds_id);
    h *= 0x100000001B3ULL;
    return h;
}

} // namespace comdare::builder::loop
