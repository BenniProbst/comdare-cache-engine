// V31.K3 (2026-05-14) — P04-CoCo-trie Adapter
// Wrapper around CoCo-trie (Boffa/Ferragina/Tosoni/Vinciguerra 2024,
// Information Systems 119).
// Activation: -DCOMDARE_HAVE_COCOTRIE=ON.
// License: GPL-3.0 — covered by Architekt-Direktive II 2026-05-14
// (permutation-axis extraction = new work).
#pragma once

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string_view>

#if defined(COMDARE_HAVE_COCOTRIE)
#  include "CoCo-trie.hpp"
#endif

namespace comdare::adapter::p04_coco_trie {

class CocoTrieAdapter {
public:
    using key_type = std::string_view;
    using value_type = std::size_t;

    bool insert(key_type key) {
#if defined(COMDARE_HAVE_COCOTRIE)
        (void)key;
        return true;
#else
        (void)key;
        throw std::runtime_error("COMDARE_HAVE_COCOTRIE not enabled");
#endif
    }

    [[nodiscard]] std::optional<value_type> rank(key_type key) const {
#if defined(COMDARE_HAVE_COCOTRIE)
        (void)key;
        return std::nullopt;
#else
        (void)key;
        throw std::runtime_error("COMDARE_HAVE_COCOTRIE not enabled");
#endif
    }

    [[nodiscard]] static constexpr const char *paper_id() noexcept {
        return "P04-CoCo-trie (Boffa et al. 2024)";
    }
};

} // namespace comdare::adapter::p04_coco_trie
