// V31.K3 (2026-05-14) — P03-Masstree Adapter
// Wrapper around masstree-beta (Mao/Kohler/Morris 2012, EuroSys).
// Activation: -DCOMDARE_HAVE_MASSTREE=ON. License: MIT (+W3C clause).
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>

#if defined(COMDARE_HAVE_MASSTREE)
#  include "masstree.hh"
#endif

namespace comdare::adapter::p03_masstree {

template <typename Key = std::uint64_t, typename Value = void *>
class MasstreeAdapter {
public:
    using key_type = Key;
    using value_type = Value;

    bool insert(key_type key, value_type value) {
#if defined(COMDARE_HAVE_MASSTREE)
        // Masstree's API uses thread-local Masstree::default_table; full
        // wiring requires kvthread.hh + threadinfo. Skeleton stub here.
        (void)key; (void)value;
        return true;
#else
        (void)key; (void)value;
        throw std::runtime_error("COMDARE_HAVE_MASSTREE not enabled");
#endif
    }

    [[nodiscard]] std::optional<value_type> find(key_type key) const {
#if defined(COMDARE_HAVE_MASSTREE)
        (void)key;
        return std::nullopt;
#else
        (void)key;
        throw std::runtime_error("COMDARE_HAVE_MASSTREE not enabled");
#endif
    }

    [[nodiscard]] static constexpr const char *paper_id() noexcept {
        return "P03-Masstree (Mao/Kohler/Morris 2012)";
    }
};

} // namespace comdare::adapter::p03_masstree
