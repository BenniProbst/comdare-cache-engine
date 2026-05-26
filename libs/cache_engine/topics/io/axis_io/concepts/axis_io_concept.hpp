#pragma once
#include "../../concepts/topic_io_concept.hpp"
#include <concepts>
namespace comdare::cache_engine::io::axis_io::concepts {
template <typename I>
concept IoStrategy =
    ::comdare::cache_engine::io::concepts::IoComponent<I>
    && requires { { I::is_in_memory_only() } noexcept -> std::convertible_to<bool>; };
}  // namespace
