#pragma once
#include <concepts>
namespace comdare::cache_engine::value_handle::concepts {
struct ValueHandleTopicTag {};
template <typename T>
concept ValueHandleComponent =
    requires { typename T::topic_tag; } && std::same_as<typename T::topic_tag, ValueHandleTopicTag>;
} // namespace comdare::cache_engine::value_handle::concepts
