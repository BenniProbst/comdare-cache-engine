#pragma once
// V41.F.6.1.P2.F axis_03b_cache_traversal Original-Code Mixin-Template (2026-05-26)
//
// @topic traversal @achse 03b cache_traversal

#include "../../../src/concepts/axis_original_code_mixin_base.hpp"

namespace comdare::cache_engine::cache_traversal::concepts {

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

    // V41.F.6.1.P2.D.tr Luecken-Markierung-Pattern (if constexpr requires → default false)
    [[nodiscard]] static constexpr bool is_original_register_entry() noexcept {
        if constexpr (requires { PaperManifest::kIsOriginal_register_entry; }) return PaperManifest::kIsOriginal_register_entry;
        else return false;
    }
    [[nodiscard]] static constexpr bool is_original_resolve() noexcept {
        if constexpr (requires { PaperManifest::kIsOriginal_resolve; }) return PaperManifest::kIsOriginal_resolve;
        else return false;
    }
    [[nodiscard]] static constexpr bool is_original_unregister() noexcept {
        if constexpr (requires { PaperManifest::kIsOriginal_unregister; }) return PaperManifest::kIsOriginal_unregister;
        else return false;
    }
    [[nodiscard]] static constexpr bool is_original_clear() noexcept {
        if constexpr (requires { PaperManifest::kIsOriginal_clear; }) return PaperManifest::kIsOriginal_clear;
        else return false;
    }

    [[nodiscard]] static constexpr bool is_original_module() noexcept {
        return is_original_register_entry() && is_original_resolve() && is_original_unregister() && is_original_clear();
    }
};

}  // namespace
