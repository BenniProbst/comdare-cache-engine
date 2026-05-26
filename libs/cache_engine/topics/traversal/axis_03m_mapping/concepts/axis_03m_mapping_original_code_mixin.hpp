#pragma once
// V41.F.6.1.P2.F axis_03m_mapping Original-Code Mixin-Template (2026-05-26)
//
// @topic traversal @achse 03m mapping

#include "../../../../src/concepts/axis_original_code_mixin_base.hpp"

namespace comdare::cache_engine::traversal::axis_03m_mapping::concepts {

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

    [[nodiscard]] static constexpr bool is_original_register_slot()  noexcept { return PaperManifest::kIsOriginal_register_slot; }
    [[nodiscard]] static constexpr bool is_original_resolve_offset() noexcept { return PaperManifest::kIsOriginal_resolve_offset; }
    [[nodiscard]] static constexpr bool is_original_reverse_lookup() noexcept { return PaperManifest::kIsOriginal_reverse_lookup; }
    [[nodiscard]] static constexpr bool is_original_clear()          noexcept { return PaperManifest::kIsOriginal_clear; }

    [[nodiscard]] static constexpr bool is_original_module() noexcept {
        return is_original_register_slot() && is_original_resolve_offset() && is_original_reverse_lookup() && is_original_clear();
    }
};

}  // namespace
