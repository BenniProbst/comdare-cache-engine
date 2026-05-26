#pragma once
// V41.F.6.1.P2.A0.6 — Cross-Topic Mixin-Basis fuer Achsen-Vererbung
//
// @stand V41.F.6.1.P2.A0.6
// @reference [[legacy-code-sha256-validation]] Memory (Achsen-generischer Mixin)
//
// **OriginalCodeMixinBase<PaperManifest>** ist die cross-topic Mixin-Basis.
// Pro Achse erbt ein achs-spezifischer Mixin-Template davon und erweitert um
// die Achsen-spezifischen is_original_<fn>() Methoden (basierend auf der
// Achsen-Interface-Functions-Liste).
//
// Vererbungs-Hierarchie:
//   OriginalCodeMixinBase<M>                           ← cross-topic (hier)
//     └── AllocatorOriginalCodeMixin<M>                ← Allocator-Achse
//           └── generated::a04_mimalloc::OriginalCodeMixin (typedef-Alias)
//                 └── class MimallocAllocator (Wrapper erbt davon)
//
// Pflicht-Felder im PaperManifest (vom Pre-Build-Tool generiert):
//   - static constexpr std::string_view kCompiler
//   - static constexpr bool kHasOriginalPaperCode
//   - static constexpr bool kIsOriginal_<wrapper_fn> pro Achs-Interface
//
// **V41.F.6.1.P2.A0.7 [[axis-base-pattern]]:** OriginalCodeMixinBase erbt
// von AxisBase + ueberschreibt get_compiler() mit dem Paper-spezifischen
// Compiler aus PaperManifest::kCompiler. Default in AxisBase ist "original".

#include "../../topics/axis_base.hpp"

#include <string_view>

namespace comdare::cache_engine::concepts {

/**
 * @brief OriginalCodeMixinBase — cross-topic Properties (Compiler + has_original)
 *
 * Erbt von ::topics::AxisBase und ueberschreibt get_compiler() mit dem Paper-
 * spezifischen Wert aus PaperManifest::kCompiler.
 * Achs-spezifische Mixin-Templates erweitern um is_original_<fn> + is_original_module.
 *
 * @tparam PaperManifest Tool-generierter Struct mit allen Pflicht-Constants
 */
template <typename PaperManifest>
struct OriginalCodeMixinBase : public ::comdare::cache_engine::topics::AxisBase {
    /// Override AxisBase::get_compiler() — Paper-spezifischer Compiler aus Manifest.
    [[nodiscard]] static constexpr std::string_view get_compiler() noexcept {
        return PaperManifest::kCompiler;
    }
    [[nodiscard]] static constexpr bool has_original_paper_code() noexcept {
        return PaperManifest::kHasOriginalPaperCode;
    }
};

}  // namespace
