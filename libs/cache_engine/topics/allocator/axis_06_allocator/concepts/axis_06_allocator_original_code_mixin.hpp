#pragma once
// V41.F.6.1.P2.F axis_06_allocator Original-Code Mixin-Template (2026-05-26)
//
// @topic allocator @achse 06
//
// Achsen-spezifisches Mixin-Template fuer Paper-Original-Code-Validierung
// ([[legacy-code-sha256-validation]] User-Direktive P2.A0.6).
//
// Vererbungs-Hierarchie ([[axis-base-pattern]]):
//   AxisBase                                                  ← Wurzel topics/
//     └── OriginalCodeMixinBase<PaperManifest>                ← cross-topic
//           └── AllocatorOriginalCodeMixin<PaperManifest>     ← HIER
//                 └── generated::<paper>::OriginalCodeMixin   ← Tool-Alias
//                       └── class MimallocAllocator           ← Wrapper

#include "../../../../src/concepts/axis_original_code_mixin_base.hpp"

namespace comdare::cache_engine::allocator::axis_06_allocator::concepts {

/**
 * @brief AllocatorOriginalCodeMixin — Achsen-spezifischer Mixin-Template
 *
 * Pro Achse 1 Template, alle Paper-Wrappers der Achse erben davon via Alias.
 *
 * Pflicht-Fields im PaperManifest (vom Pre-Build-Tool generiert):
 *   - kCompiler (geerbt von OriginalCodeMixinBase via get_compiler())
 *   - kHasOriginalPaperCode (geerbt)
 *   - kIsOriginal_allocate
 *   - kIsOriginal_deallocate
 *
 * @tparam PaperManifest Tool-generierter Struct mit Pflicht-Constants
 */
template <typename PaperManifest>
struct AllocatorOriginalCodeMixin
    : ::comdare::cache_engine::concepts::OriginalCodeMixinBase<PaperManifest> {

    // V41.F.6.1.P2.D.tr Luecken-Markierung-Pattern:
    // Wenn PaperManifest ein Field NICHT hat (manifest.txt mit Teil-Mappings) → default false.
    // Erlaubt Wrapper mit Teil-Original-Paper-Source + eigener Luecken-Fueller-Impl.
    [[nodiscard]] static constexpr bool is_original_allocate() noexcept {
        if constexpr (requires { PaperManifest::kIsOriginal_allocate; }) return PaperManifest::kIsOriginal_allocate;
        else return false;
    }
    [[nodiscard]] static constexpr bool is_original_deallocate() noexcept {
        if constexpr (requires { PaperManifest::kIsOriginal_deallocate; }) return PaperManifest::kIsOriginal_deallocate;
        else return false;
    }

    /// Modul-Aggregat: alle Pflicht-Functions sind Original.
    [[nodiscard]] static constexpr bool is_original_module() noexcept {
        return is_original_allocate() && is_original_deallocate();
    }
};

}  // namespace
