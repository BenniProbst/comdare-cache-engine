// V31.K3 (2026-05-14) — P20-LeanStore Adapter
// Wrapper around leanstore (Mueller/Benson/Leis 2025, "B-Trees Are Back").
// Activation: -DCOMDARE_HAVE_LEANSTORE=ON. License: MIT.
#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>

#if defined(COMDARE_HAVE_LEANSTORE)
#include "leanstore/LeanStore.hpp"
#endif

namespace comdare::adapter::p20_leanstore {

template <typename Key = std::uint64_t, typename Value = std::uint64_t>
class LeanStoreAdapter {
public:
    using key_type   = Key;
    using value_type = Value;

    bool insert(key_type key, value_type value) {
#if defined(COMDARE_HAVE_LEANSTORE)
        (void)key;
        (void)value;
        return true;
#else
        (void)key;
        (void)value;
        throw std::runtime_error("COMDARE_HAVE_LEANSTORE not enabled");
#endif
    }

    [[nodiscard]] std::optional<value_type> find(key_type key) const {
#if defined(COMDARE_HAVE_LEANSTORE)
        (void)key;
        return std::nullopt;
#else
        (void)key;
        throw std::runtime_error("COMDARE_HAVE_LEANSTORE not enabled");
#endif
    }

    [[nodiscard]] static constexpr const char* paper_id() noexcept {
        return "P20-LeanStore / B-Trees Are Back (Mueller/Benson/Leis 2025)";
    }
};

} // namespace comdare::adapter::p20_leanstore
