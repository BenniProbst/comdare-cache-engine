#pragma once
#include <concepts>
namespace comdare::cache_engine::filter::concepts {
struct FilterTopicTag {};
template <typename T>
concept FilterComponent = requires { typename T::topic_tag; } && std::same_as<typename T::topic_tag, FilterTopicTag>;
}  // namespace
