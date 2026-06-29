#pragma once
// V41.F.6.1.P1 Phase B — Sub-Concept LegacyOriginalCodeStrategy (cross-topic)
//
// @stand V41.F.6.1.P1 Phase B Pilot mimalloc
// @reference [[paper-original-code-pattern]] Memory
// @reference [[experiment-compiler-property]] Memory
// @reference [[legacy-code-sha256-validation]] Memory
//
// **Habich-Compliance Pflicht-API** fuer jeden Wrapper, der einen Algorithmus
// aus einem wissenschaftlichen Paper implementiert. Cross-Topic (Allocator,
// queuing, traversal, ...) — daher in src/concepts/ statt topic-spezifisch.
//
// Pflicht-Properties pro Wrapper (V41.F.6.1.P2.C reduziert auf 2 statt 3):
//   - static constexpr std::string_view get_compiler() noexcept;
//     z.B. "gcc-9.5"; "self" wenn eigene Re-Impl; "system" wenn ohne Paper-Bindung
//   - static constexpr bool is_original_module() noexcept;
//     true: Paper-Original-Code-Linking aktiv UND ALLE is_original_<fn>() sind true (SHA-validiert)
//     false: Re-Impl ODER Paper-Code wurde modifiziert ODER kein Mixin im Build
//
// **Eliminiert P2.C (User-Direktive 2026-05-26):** `has_original_paper_code` war
// redundant zu `is_original_module` und produzierte Code-Bloat (10 hardcoded Defaults).
// Eine Boolean reicht semantisch. AxisBase liefert generischen Default `false`,
// Paper-Mixin (Tool-generiert) ueberschreibt mit `mp_all_of` aller per-function Bools.

#include <boost/mp11.hpp>

#include <concepts>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::concepts {

namespace mp = boost::mp11;

/**
 * @brief LegacyOriginalCodePflicht — Pflicht-API pro Wrapper
 *
 * **Pflicht-API in JEDEM CacheEngine<X>PermutationStrategy-Concept.**
 * Wird via && Constraint in den Topic-Concepts angehaengt.
 *
 * @note OPTIONAL aktivierbar pro Topic-Concept — heute (Phase B.1) nur als
 *       Sub-Concept verfuegbar, NICHT zwingend in allen Topic-Concepts. Phase B.2
 *       fuegt es via Mass-Update in alle Achs-Concepts ein.
 */
template <typename W>
concept LegacyOriginalCodePflicht = requires {
    { W::get_compiler() } -> std::convertible_to<std::string_view>;
    { W::is_original_module() } -> std::convertible_to<bool>;
};

/**
 * @brief HasOriginalCode — Sub-Concept: Paper-Wrapper mit konkretem Compiler
 *
 * Erfuellt wenn get_compiler() != "original" UND != "self" UND != "system"
 * (also konkreter Paper-Compiler wie "gcc-9.5", "clang-12", ...).
 * V41.F.6.1.P2.C: ersetzt frueheres `has_original_paper_code()` Bool durch
 * compile-time Auswertung der bereits vorhandenen get_compiler()-Property.
 *
 * CacheEngineBuilder kann via mp_filter is_compiler_available pruefen
 * ob das Paper-Binary gebuildet werden kann.
 */
template <typename W>
concept HasOriginalCode =
    LegacyOriginalCodePflicht<W> && (W::get_compiler() != std::string_view{"original"}) &&
    (W::get_compiler() != std::string_view{"self"}) && (W::get_compiler() != std::string_view{"system"});

/**
 * @brief PaperOriginalValidated — Sub-Concept: SHA-validierte Original-Code
 *
 * Erfuellt wenn is_original_module() = true zur Compile-Zeit.
 * Diplomarbeit-Report kann via mp_filter is_paper_original_validated
 * nur jene Mess-Reihen filtern, die "ORIGINAL ✓" sind.
 */
template <typename W>
concept PaperOriginalValidated = LegacyOriginalCodePflicht<W> && (W::is_original_module());

/**
 * @brief is_original_v<W> — Compile-Time-Konstante (Convenience-Wrapper).
 *
 * MSVC-tauglich: direkter Zugriff auf static constexpr W::is_original_module()
 * (ueber requires-Constraint). Frueher Versuch mit mp_all_of-Helper-Template
 * war MSVC-inkompatibel (constexpr-Lambda in concept-Context + Template-
 * Specialization-Probleme).
 *
 * Verwendung im Wrapper:
 *   class MyWrapper {
 *       COMDARE_IS_ORIGINAL_FROM_STRING(allocate, "...", "...")
 *       COMDARE_IS_ORIGINAL_FROM_STRING(deallocate, "...", "...")
 *       static constexpr bool is_original_module() noexcept {
 *           return is_original_allocate() && is_original_deallocate();
 *       }
 *   };
 *   static_assert(is_original_v<MyWrapper> == false);  // wenn EIN SHA mismatcht
 */
template <typename W>
    requires LegacyOriginalCodePflicht<W>
constexpr bool is_original_v = W::is_original_module();

} // namespace comdare::cache_engine::concepts
