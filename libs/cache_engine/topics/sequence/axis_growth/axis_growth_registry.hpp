#pragma once
// L-76b — axis_growth Registry: die zentrale Achsen-Varianten-Liste (analog axis_09b/axis_12-Registries). Trägt
// alle 4 GrowthPolicy-Wrapper (DoublingGrowth aus der Anatomie + GoldenRatio/FixedChunk/Exact). Die
// SequencePermutationEngine kann diese als 11. Achsen-Slot (axis_growth) permutieren (StaticAxisVariants_growth).

#include "axis_growth_policies.hpp"
#include "anatomy/sequence_composition.hpp" // DoublingGrowth (Default-Policy, ebenfalls Teil der Achse)

#include <boost/mp11.hpp>
#include <cstddef>

namespace comdare::cache_engine::sequence::axis_growth {

namespace mp = boost::mp11;

/// AllGrowthPolicies — alle axis_growth-Varianten (Doubling-Default + 3 Goldstandard-Erweiterungen).
using AllGrowthPolicies =
    mp::mp_list<::comdare::cache_engine::anatomy::DoublingGrowth, // ×2 (std::vector-Standard, bisheriger Default)
                GoldenRatioGrowth,                                // ×1.5
                FixedChunkGrowth<64>,                             // +64 (additiv)
                ExactGrowth                                       // 1:1
                >;

/// EnabledGrowthPolicies — heute == AllGrowthPolicies (kein flag-gating; alle 4 immer verfügbar). Konsumierbar als
/// 11. TopicConfigSet-Slot der SequencePermutationEngine (axis_growth permutiert über die 4 Policies).
using EnabledGrowthPolicies = AllGrowthPolicies;

inline constexpr std::size_t kGrowthPolicyCount = mp::mp_size<AllGrowthPolicies>::value;

} // namespace comdare::cache_engine::sequence::axis_growth
