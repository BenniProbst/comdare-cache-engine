#pragma once
// V41.F.6.1.F3 axis_migration NoMigration Default-Wrapper (none baseline)

#include "axis_migration_base.hpp"
#include "../concepts/topic_migration_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::migration::axis_migration {
class NoMigration : public MigrationBase<NoMigration> {
public:
    using topic_tag = ::comdare::cache_engine::migration::concepts::MigrationTopicTag;
    using family_id = std::integral_constant<int, 0>;
    [[nodiscard]] static constexpr bool is_active() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()        noexcept { return "no_migration"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "NoMigration (none baseline, static placement)"; }
};
}  // namespace
namespace comdare::cache_engine::migration::axis_migration {
    static_assert(concepts::MigrationStrategy<NoMigration>);
}
