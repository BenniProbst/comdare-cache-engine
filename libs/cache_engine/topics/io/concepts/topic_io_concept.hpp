#pragma once
#include <concepts>
namespace comdare::cache_engine::io::concepts {
struct IoTopicTag {};
template <typename T>
concept IoComponent = requires { typename T::topic_tag; } && std::same_as<typename T::topic_tag, IoTopicTag>;
}  // namespace
