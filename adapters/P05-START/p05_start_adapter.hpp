// V31.K3 (2026-05-14) — P05-START Adapter
// Wrapper around START (Fent/Jungmair/Kipf/Neumann 2020, ICDE).
// Activation: -DCOMDARE_HAVE_START=ON. License: MIT.
#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>

#if defined(COMDARE_HAVE_START)
#  include "START.hpp"
#endif

namespace comdare::adapter::p05_start {

template <typename Key = std::uint64_t, typename Value = std::uint64_t>
class StartAdapter {
public:
    using key_type = Key;
    using value_type = Value;

    bool insert(key_type key, value_type value) {
#if defined(COMDARE_HAVE_START)
        (void)key; (void)value;
        return true;
#else
        (void)key; (void)value;
        throw std::runtime_error("COMDARE_HAVE_START not enabled");
#endif
    }

    [[nodiscard]] std::optional<value_type> find(key_type key) const {
#if defined(COMDARE_HAVE_START)
        (void)key;
        return std::nullopt;
#else
        (void)key;
        throw std::runtime_error("COMDARE_HAVE_START not enabled");
#endif
    }

    [[nodiscard]] static constexpr const char *paper_id() noexcept {
        return "P05-START (Fent/Jungmair/Kipf/Neumann 2020)";
    }
};

} // namespace comdare::adapter::p05_start
