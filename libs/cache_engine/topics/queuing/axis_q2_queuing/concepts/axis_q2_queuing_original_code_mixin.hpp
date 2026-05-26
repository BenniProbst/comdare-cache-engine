#pragma once
// V41.F.6.1.P2.F axis_q2_queuing Original-Code Mixin-Template (2026-05-26)
//
// @topic queuing @achse Q2 flush_policy

#include "../../../../src/concepts/axis_original_code_mixin_base.hpp"

namespace comdare::cache_engine::queuing::axis_q2_queuing::concepts {

/**
 * @brief FlushOriginalCodeMixin — Achsen-spezifischer Mixin-Template fuer Q2
 *
 * Pflicht-Fields im PaperManifest:
 *   - kCompiler / kHasOriginalPaperCode (geerbt)
 *   - kIsOriginal_should_flush / on_flush_complete
 */
template <typename PaperManifest>
struct FlushOriginalCodeMixin
    : ::comdare::cache_engine::concepts::OriginalCodeMixinBase<PaperManifest> {

    // V41.F.6.1.P2.D.tr Luecken-Markierung-Pattern (if constexpr requires → default false)
    [[nodiscard]] static constexpr bool is_original_should_flush() noexcept {
        if constexpr (requires { PaperManifest::kIsOriginal_should_flush; }) return PaperManifest::kIsOriginal_should_flush;
        else return false;
    }
    [[nodiscard]] static constexpr bool is_original_on_flush_complete() noexcept {
        if constexpr (requires { PaperManifest::kIsOriginal_on_flush_complete; }) return PaperManifest::kIsOriginal_on_flush_complete;
        else return false;
    }

    [[nodiscard]] static constexpr bool is_original_module() noexcept {
        return is_original_should_flush() && is_original_on_flush_complete();
    }
};

}  // namespace
