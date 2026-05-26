#pragma once
#include "../../concepts/topic_search_engine_concept.hpp"
#include <concepts>

namespace comdare::cache_engine::search_engine::axis_01_index_organization::concepts {

/// IndexOrganizationStrategy — Pflicht-API: index_kind() (Sub 01a/b/c/d)
template <typename I>
concept IndexOrganizationStrategy =
    ::comdare::cache_engine::search_engine::concepts::SearchEngineComponent<I>
    && requires { { I::is_ordered() } noexcept -> std::convertible_to<bool>; };

}  // namespace
