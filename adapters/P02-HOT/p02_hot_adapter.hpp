// V31.K3 (2026-05-14) — P02-HOT Adapter
// Wrapper around HOT (Binna/Zangerle/Pichl/Specht/Leis 2018, SIGMOD).
// Activation: -DCOMDARE_HAVE_HOT=ON. License: ISC.
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>

#if defined(COMDARE_HAVE_HOT)
#include "hot/rowex/HOTRowex.hpp"
#endif

namespace comdare::adapter::p02_hot {

template <typename Key = std::uint64_t, typename Value = void*>
class HotAdapter {
public:
    using key_type   = Key;
    using value_type = Value;

    bool insert(key_type key, value_type value) {
#if defined(COMDARE_HAVE_HOT)
        return tree_.insert({key, value});
#else
        (void)key;
        (void)value;
        throw std::runtime_error("COMDARE_HAVE_HOT not enabled");
#endif
    }

    [[nodiscard]] std::optional<value_type> find(key_type key) const {
#if defined(COMDARE_HAVE_HOT)
        const auto r = tree_.lookup(key);
        if (!r.mIsValid) return std::nullopt;
        return r.mValue;
#else
        (void)key;
        throw std::runtime_error("COMDARE_HAVE_HOT not enabled");
#endif
    }

    [[nodiscard]] static constexpr const char* paper_id() noexcept { return "P02-HOT (Binna et al. 2018)"; }

private:
#if defined(COMDARE_HAVE_HOT)
    hot::rowex::HOTRowex<std::pair<key_type, value_type>, /*KeyExtractor=*/nullptr> tree_;
#endif
};

} // namespace comdare::adapter::p02_hot
