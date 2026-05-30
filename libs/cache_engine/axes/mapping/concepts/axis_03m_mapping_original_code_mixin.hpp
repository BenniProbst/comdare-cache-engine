#pragma once
// V41.F.6.1.P2.F axis_03m_mapping Original-Code Mixin-Template (2026-05-26)
//
// @topic traversal @achse 03m mapping

#include <topics/../../src/concepts/axis_original_code_mixin_base.hpp>

namespace comdare::cache_engine::mapping::concepts {

/**
 * @brief MappingOriginalCodeMixin — Achsen-spezifischer Mixin-Template fuer 03m
 *
 * Pflicht-Fields im PaperManifest:
 *   - kCompiler / kHasOriginalPaperCode (geerbt)
 *   - kIsOriginal_register_slot / resolve_offset / reverse_lookup / clear
 */
template <typename PaperManifest>
struct MappingOriginalCodeMixin
    : ::comdare::cache_engine::concepts::OriginalCodeMixinBase<PaperManifest> {

    // V41.F.6.1.P2.D.tr Luecken-Markierung-Pattern (if constexpr requires → default false)
    [[nodiscard]] static constexpr bool is_original_register_slot() noexcept {
        if constexpr (requires { PaperManifest::kIsOriginal_register_slot; }) return PaperManifest::kIsOriginal_register_slot;
        else return false;
    }
    [[nodiscard]] static constexpr bool is_original_resolve_offset() noexcept {
        if constexpr (requires { PaperManifest::kIsOriginal_resolve_offset; }) return PaperManifest::kIsOriginal_resolve_offset;
        else return false;
    }
    [[nodiscard]] static constexpr bool is_original_reverse_lookup() noexcept {
        if constexpr (requires { PaperManifest::kIsOriginal_reverse_lookup; }) return PaperManifest::kIsOriginal_reverse_lookup;
        else return false;
    }
    [[nodiscard]] static constexpr bool is_original_clear() noexcept {
        if constexpr (requires { PaperManifest::kIsOriginal_clear; }) return PaperManifest::kIsOriginal_clear;
        else return false;
    }

    [[nodiscard]] static constexpr bool is_original_module() noexcept {
        return is_original_register_slot() && is_original_resolve_offset() && is_original_reverse_lookup() && is_original_clear();
    }
};

}  // namespace
