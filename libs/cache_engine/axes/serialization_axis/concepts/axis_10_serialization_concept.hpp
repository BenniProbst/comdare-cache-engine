#pragma once
#include <topics/serialization/concepts/topic_serialization_concept.hpp>
#include <concepts>
namespace comdare::cache_engine::serialization_axis::concepts {
template <typename S>
concept SerializationStrategy =
    ::comdare::cache_engine::serialization::concepts::SerializationComponent<S>
    && requires { { S::supports_compression() } noexcept -> std::convertible_to<bool>; };
}  // namespace
