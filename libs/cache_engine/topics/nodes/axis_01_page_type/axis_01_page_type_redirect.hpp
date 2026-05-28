#pragma once
// V41.F.6.1 F.6 axis_01 RedirectPageType (Seitentyp 4: kollabierter Restpfad, CoCo-Trie)

#include "axis_01_page_type_strategy_base.hpp"
#include "axis_01_page_type_subaxes_pg1_to_pg3.hpp"
#include "concepts/axis_01_page_type_cache_engine_permutation_concept.hpp"
#include "axis_01_page_type_flags.hpp"
#include "../concepts/topic_nodes_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::nodes::axis_01_page_type {

/// RedirectPageType — kollabierter eindeutiger Restpfad (CoCo-Trie-Idee). Weder klassischer
/// Branch noch Leaf: speichert nur das Suffix bis zum naechsten echten Verzweigungs-/Wert-Knoten.
class RedirectPageType : public PageTypeStrategyBase<RedirectPageType> {
public:
    using topic_tag = ::comdare::cache_engine::nodes::concepts::NodesTopicTag;
    using axis_tag  = subaxes::path_collapse_tag;
    using family_id = std::integral_constant<int, 4>;

    static constexpr bool enabled = flags::redirect_enabled;

    [[nodiscard]] static constexpr concepts::PageKind page_kind() noexcept { return concepts::PageKind::Redirect; }
    [[nodiscard]] static constexpr bool             is_branch()   noexcept { return false; }  // kollabierter Single-Path
    [[nodiscard]] static constexpr bool             is_leaf()     noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()        noexcept { return "page_redirect"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "RedirectPageType (collapsed unique remainder-path, CoCo-trie)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "REDIRECT"; }
};

}  // namespace

namespace comdare::cache_engine::nodes::axis_01_page_type {
    static_assert(concepts::PageTypeStrategy<RedirectPageType>);
    static_assert(concepts::CacheEnginePermutationStrategy<RedirectPageType>);
}
