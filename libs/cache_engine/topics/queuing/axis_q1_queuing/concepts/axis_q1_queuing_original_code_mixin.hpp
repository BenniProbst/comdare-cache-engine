#pragma once
// V41.F.6.1.P2.F axis_q1_queuing Original-Code Mixin-Template (2026-05-26)
//
// @topic queuing @achse Q1 buffer_strategy

#include "../../../../src/concepts/axis_original_code_mixin_base.hpp"

namespace comdare::cache_engine::queuing::axis_q1_queuing::concepts {

/**
 * @brief BufferOriginalCodeMixin — Achsen-spezifischer Mixin-Template fuer Q1
 *
 * Pflicht-Fields im PaperManifest:
 *   - kCompiler / kHasOriginalPaperCode (geerbt)
 *   - kIsOriginal_put / get / emplace / peek_front / peek_back / clear
 */
template <typename PaperManifest>
struct BufferOriginalCodeMixin : ::comdare::cache_engine::concepts::OriginalCodeMixinBase<PaperManifest> {
    // V41.F.6.1.P2.D.tr Luecken-Markierung-Pattern (if constexpr requires → default false)
    [[nodiscard]] static constexpr bool is_original_put() noexcept {
        if constexpr (requires { PaperManifest::kIsOriginal_put; })
            return PaperManifest::kIsOriginal_put;
        else
            return false;
    }
    [[nodiscard]] static constexpr bool is_original_get() noexcept {
        if constexpr (requires { PaperManifest::kIsOriginal_get; })
            return PaperManifest::kIsOriginal_get;
        else
            return false;
    }
    [[nodiscard]] static constexpr bool is_original_emplace() noexcept {
        if constexpr (requires { PaperManifest::kIsOriginal_emplace; })
            return PaperManifest::kIsOriginal_emplace;
        else
            return false;
    }
    [[nodiscard]] static constexpr bool is_original_peek_front() noexcept {
        if constexpr (requires { PaperManifest::kIsOriginal_peek_front; })
            return PaperManifest::kIsOriginal_peek_front;
        else
            return false;
    }
    [[nodiscard]] static constexpr bool is_original_peek_back() noexcept {
        if constexpr (requires { PaperManifest::kIsOriginal_peek_back; })
            return PaperManifest::kIsOriginal_peek_back;
        else
            return false;
    }
    [[nodiscard]] static constexpr bool is_original_clear() noexcept {
        if constexpr (requires { PaperManifest::kIsOriginal_clear; })
            return PaperManifest::kIsOriginal_clear;
        else
            return false;
    }

    [[nodiscard]] static constexpr bool is_original_module() noexcept {
        return is_original_put() && is_original_get() && is_original_emplace() && is_original_peek_front() &&
               is_original_peek_back() && is_original_clear();
    }
};

} // namespace comdare::cache_engine::queuing::axis_q1_queuing::concepts
