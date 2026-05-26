#pragma once
// V41.F.6.1.P2.F axis_03b_cache_traversal Original-Code Mixin-Template (2026-05-26)
//
// @topic traversal @achse 03b cache_traversal

#include "../../../../src/concepts/axis_original_code_mixin_base.hpp"

namespace comdare::cache_engine::traversal::axis_03b_cache_traversal::concepts {

/**
 * @brief CacheTraversalOriginalCodeMixin — Achsen-spezifischer Mixin-Template fuer 03b
 *
 * Pflicht-Fields im PaperManifest:
 *   - kCompiler / kHasOriginalPaperCode (geerbt)
 *   - kIsOriginal_register_entry / resolve / unregister / clear
 */
template <typename PaperManifest>
struct CacheTraversalOriginalCodeMixin
    : ::comdare::cache_engine::concepts::OriginalCodeMixinBase<PaperManifest> {

    [[nodiscard]] static constexpr bool is_original_register_entry() noexcept { return PaperManifest::kIsOriginal_register_entry; }
    [[nodiscard]] static constexpr bool is_original_resolve()        noexcept { return PaperManifest::kIsOriginal_resolve; }
    [[nodiscard]] static constexpr bool is_original_unregister()     noexcept { return PaperManifest::kIsOriginal_unregister; }
    [[nodiscard]] static constexpr bool is_original_clear()          noexcept { return PaperManifest::kIsOriginal_clear; }

    [[nodiscard]] static constexpr bool is_original_module() noexcept {
        return is_original_register_entry() && is_original_resolve() && is_original_unregister() && is_original_clear();
    }
};

}  // namespace
