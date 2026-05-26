#pragma once
#include "../../concepts/topic_serialization_concept.hpp"
#include <concepts>
namespace comdare::cache_engine::serialization::axis_10_serialization::concepts {
template <typename S>
concept SerializationStrategy =
    ::comdare::cache_engine::serialization::concepts::SerializationComponent<S>
    && requires { { S::supports_compression() } noexcept -> std::convertible_to<bool>; };
}  // namespace
