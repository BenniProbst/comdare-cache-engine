#pragma once
#include <concepts>
namespace comdare::cache_engine::serialization::concepts {
struct SerializationTopicTag {};
template <typename T>
concept SerializationComponent =
    requires { typename T::topic_tag; } && std::same_as<typename T::topic_tag, SerializationTopicTag>;
} // namespace comdare::cache_engine::serialization::concepts
