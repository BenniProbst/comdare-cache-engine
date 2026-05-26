#pragma once
#include "../../concepts/topic_migration_concept.hpp"
#include <concepts>
namespace comdare::cache_engine::migration::axis_migration::concepts {
template <typename M>
concept MigrationStrategy =
    ::comdare::cache_engine::migration::concepts::MigrationComponent<M>
    && requires { { M::is_active() } noexcept -> std::convertible_to<bool>; };
}  // namespace
