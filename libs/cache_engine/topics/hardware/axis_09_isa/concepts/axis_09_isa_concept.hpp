#pragma once
#include "../../concepts/topic_hardware_concept.hpp"
#include <concepts>
namespace comdare::cache_engine::hardware::axis_09_isa::concepts {
template <typename I>
concept IsaStrategy =
    ::comdare::cache_engine::hardware::concepts::HardwareComponent<I>
    && requires { { I::supports_simd() } noexcept -> std::convertible_to<bool>; };
}  // namespace
