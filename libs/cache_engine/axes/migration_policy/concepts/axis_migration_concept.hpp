#pragma once
#include <topics/migration/concepts/topic_migration_concept.hpp>
#include <concepts>
namespace comdare::cache_engine::migration_policy::concepts {
template <typename M>
concept MigrationStrategy =
    ::comdare::cache_engine::migration::concepts::MigrationComponent<M>
    && requires { { M::is_active() } noexcept -> std::convertible_to<bool>; };
}  // namespace
