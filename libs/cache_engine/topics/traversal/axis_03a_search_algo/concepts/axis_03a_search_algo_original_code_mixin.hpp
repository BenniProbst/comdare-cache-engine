#pragma once
// V41.F.6.1.P2.F axis_03a_search_algo Original-Code Mixin-Template (2026-05-26)
//
// @topic traversal @achse 03a search_algo

#include "../../../../src/concepts/axis_original_code_mixin_base.hpp"

namespace comdare::cache_engine::traversal::axis_03a_search_algo::concepts {

/**
 * @brief SearchAlgoOriginalCodeMixin — Achsen-spezifischer Mixin-Template fuer 03a
 *
 * Pflicht-Fields im PaperManifest:
 *   - kCompiler / kHasOriginalPaperCode (geerbt)
 *   - kIsOriginal_insert / lookup / erase / clear
 */
template <typename PaperManifest>
struct SearchAlgoOriginalCodeMixin
    : ::comdare::cache_engine::concepts::OriginalCodeMixinBase<PaperManifest> {

    [[nodiscard]] static constexpr bool is_original_insert() noexcept { return PaperManifest::kIsOriginal_insert; }
    [[nodiscard]] static constexpr bool is_original_lookup() noexcept { return PaperManifest::kIsOriginal_lookup; }
    [[nodiscard]] static constexpr bool is_original_erase()  noexcept { return PaperManifest::kIsOriginal_erase; }
    [[nodiscard]] static constexpr bool is_original_clear()  noexcept { return PaperManifest::kIsOriginal_clear; }

    [[nodiscard]] static constexpr bool is_original_module() noexcept {
        return is_original_insert() && is_original_lookup() && is_original_erase() && is_original_clear();
    }
};

}  // namespace
