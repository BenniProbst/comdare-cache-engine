#pragma once
// V41.F.6.1.P2.F axis_03a_search_algo Original-Code Mixin-Template (2026-05-26)
//
// @topic traversal @achse 03a search_algo

#include <src/concepts/axis_original_code_mixin_base.hpp>

namespace comdare::cache_engine::lookup::concepts {

/**
 * @brief SearchAlgoOriginalCodeMixin — Achsen-spezifischer Mixin-Template fuer 03a
 *
 * Pflicht-Fields im PaperManifest (von Tool generiert wenn manifest.txt-Mapping existiert):
 *   - kCompiler / kHasOriginalPaperCode (geerbt)
 *   - kIsOriginal_insert / lookup / erase / clear
 *
 * **V41.F.6.1.P2.D.tr Luecken-Markierung (User-Direktive):**
 * Wenn PaperManifest ein Field NICHT hat (manifest.txt hat nur Teil-Mappings), default false.
 * Beispiel HOT/START: insert+lookup originall, erase+clear sind unsere Erweiterungen
 * (Lueckenfueller). is_original_erase()/is_original_clear() = false ohne dass
 * PaperManifest::kIsOriginal_erase/clear existieren muss.
 *
 * PermutationEngine kann via HasOriginalCode/PaperOriginalValidated Concept filtern:
 * nur Wrappers mit voller Original-Konformitaet (alle 4 Functions true).
 */
template <typename PaperManifest>
struct SearchAlgoOriginalCodeMixin
    : ::comdare::cache_engine::concepts::OriginalCodeMixinBase<PaperManifest> {

    [[nodiscard]] static constexpr bool is_original_insert() noexcept {
        if constexpr (requires { PaperManifest::kIsOriginal_insert; }) return PaperManifest::kIsOriginal_insert;
        else return false;  // Luecke: Function nicht im Paper, eigene Erweiterung
    }
    [[nodiscard]] static constexpr bool is_original_lookup() noexcept {
        if constexpr (requires { PaperManifest::kIsOriginal_lookup; }) return PaperManifest::kIsOriginal_lookup;
        else return false;
    }
    [[nodiscard]] static constexpr bool is_original_erase() noexcept {
        if constexpr (requires { PaperManifest::kIsOriginal_erase; }) return PaperManifest::kIsOriginal_erase;
        else return false;
    }
    [[nodiscard]] static constexpr bool is_original_clear() noexcept {
        if constexpr (requires { PaperManifest::kIsOriginal_clear; }) return PaperManifest::kIsOriginal_clear;
        else return false;
    }

    [[nodiscard]] static constexpr bool is_original_module() noexcept {
        return is_original_insert() && is_original_lookup() && is_original_erase() && is_original_clear();
    }
};

}  // namespace
