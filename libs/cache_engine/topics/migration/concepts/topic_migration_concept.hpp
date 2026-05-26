#pragma once
#include <concepts>
namespace comdare::cache_engine::migration::concepts {
struct MigrationTopicTag {};
template <typename T>
concept MigrationComponent = requires { typename T::topic_tag; } && std::same_as<typename T::topic_tag, MigrationTopicTag>;
}  // namespace
