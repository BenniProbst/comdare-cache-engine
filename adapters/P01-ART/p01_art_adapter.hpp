// V31.K1 (2026-05-14) — P01-ART Adapter (unodb)
//
// Wrapper around unodb (https://github.com/laurynas-biveinis/unodb), the
// reference C++ implementation of the Adaptive Radix Tree (Leis/Kemper/
// Neumann 2013, ICDE).
//
// Activated via CMake option `-DCOMDARE_HAVE_UNODB=ON`. When inactive,
// adapter falls back to a stub that throws on operations — keeps the
// build green without requiring the original repo's build chain.
//
// License: see LICENSE_AUDIT_EXT.md and NOTICE Architekt-Direktive II
// (2026-05-14): permutation-axis extraction creates a new work.
#pragma once

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>

#if defined(COMDARE_HAVE_UNODB)
#  include "art.hpp"
#endif

namespace comdare::adapter::p01_art {

// Map-style adapter: K -> V. Default Key=uint64_t, Value=void* (matches
// the typical unodb usage pattern from the original ICDE 2013 examples).
template <typename Key = std::uint64_t, typename Value = void *>
class UnodbDbAdapter {
public:
    using key_type = Key;
    using value_type = Value;

    UnodbDbAdapter() = default;
    UnodbDbAdapter(const UnodbDbAdapter &) = delete;
    UnodbDbAdapter &operator=(const UnodbDbAdapter &) = delete;
    UnodbDbAdapter(UnodbDbAdapter &&) noexcept = default;
    UnodbDbAdapter &operator=(UnodbDbAdapter &&) noexcept = default;
    ~UnodbDbAdapter() = default;

    // YCSB-style operations. Concept-mapping to comdare::ISearchEngine
    // happens in libs/cache_engine/builder/, not here.
    bool insert(key_type key, value_type value) {
#if defined(COMDARE_HAVE_UNODB)
        return db_.insert(key, value);
#else
        (void)key;
        (void)value;
        throw std::runtime_error(stub_error_message());
#endif
    }

    [[nodiscard]] std::optional<value_type> find(key_type key) const {
#if defined(COMDARE_HAVE_UNODB)
        const auto v = db_.get(key);
        if (v == nullptr) return std::nullopt;
        return v;
#else
        (void)key;
        throw std::runtime_error(stub_error_message());
#endif
    }

    bool erase(key_type key) {
#if defined(COMDARE_HAVE_UNODB)
        return db_.remove(key);
#else
        (void)key;
        throw std::runtime_error(stub_error_message());
#endif
    }

    [[nodiscard]] std::size_t size() const {
#if defined(COMDARE_HAVE_UNODB)
        return db_.size();
#else
        return 0;
#endif
    }

    [[nodiscard]] static constexpr const char *paper_id() noexcept {
        return "P01-ART (Leis/Kemper/Neumann 2013)";
    }

private:
    static std::string stub_error_message() {
        return "comdare::adapter::p01_art: COMDARE_HAVE_UNODB not enabled. "
               "Rebuild with -DCOMDARE_HAVE_UNODB=ON to activate ext/P01-ART/unodb/.";
    }

#if defined(COMDARE_HAVE_UNODB)
    unodb::db<key_type, value_type> db_;
#endif
};

} // namespace comdare::adapter::p01_art
