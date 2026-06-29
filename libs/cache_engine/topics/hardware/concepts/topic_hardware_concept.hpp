#pragma once
#include <concepts>
namespace comdare::cache_engine::hardware::concepts {
struct HardwareTopicTag {};
template <typename T>
concept HardwareComponent =
    requires { typename T::topic_tag; } && std::same_as<typename T::topic_tag, HardwareTopicTag>;
} // namespace comdare::cache_engine::hardware::concepts
