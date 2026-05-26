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
struct BufferOriginalCodeMixin
    : ::comdare::cache_engine::concepts::OriginalCodeMixinBase<PaperManifest> {

    [[nodiscard]] static constexpr bool is_original_put()        noexcept { return PaperManifest::kIsOriginal_put; }
    [[nodiscard]] static constexpr bool is_original_get()        noexcept { return PaperManifest::kIsOriginal_get; }
    [[nodiscard]] static constexpr bool is_original_emplace()    noexcept { return PaperManifest::kIsOriginal_emplace; }
    [[nodiscard]] static constexpr bool is_original_peek_front() noexcept { return PaperManifest::kIsOriginal_peek_front; }
    [[nodiscard]] static constexpr bool is_original_peek_back()  noexcept { return PaperManifest::kIsOriginal_peek_back; }
    [[nodiscard]] static constexpr bool is_original_clear()      noexcept { return PaperManifest::kIsOriginal_clear; }

    [[nodiscard]] static constexpr bool is_original_module() noexcept {
        return is_original_put() && is_original_get() && is_original_emplace()
            && is_original_peek_front() && is_original_peek_back() && is_original_clear();
    }
};

}  // namespace
