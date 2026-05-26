#pragma once
// V41.F.6.1.P2.A0.7 — Cross-Axis Wurzel-Datei (axis_base.hpp)
//
// @stand V41.F.6.1.P2.A0.7
// @reference [[axis-base-pattern]] Memory
//
// **Wurzel-Pattern-File direkt im topics/ Ordner** (NICHT in axes_NN_*/ Unterordner).
//
// **AxisBase** definiert cross-axis Pflicht-Properties die ALLE Wrapper-Klassen
// (egal Achse, egal Topic) erfuellen muessen. Vererbungs-Pattern: Wrapper erbt
// von AxisBase ODER ueberschreibt die Properties statisch.
//
// **Naming-Konvention (verbindlich fuer alle Cross-Axis-Properties):**
//   - Boolean-Properties:  is_<eigenschaft>()  → bool
//   - String-Properties:   get_<eigenschaft>() → std::string_view
//   - Stufen-Properties:   <name>()            → enum class
//   - Mess-Properties:     statistics()/snapshot()/observer() → Struct/Type
//
// **Aktuelle Cross-Axis-Pflicht (V41.F.6.1.P2.A0.7):**
//   - get_compiler() → std::string_view (Default "original")
//
// Wrapper ohne Override:
//   class StdMalloc : public ... , public ::topics::AxisBase { ... };
//   StdMalloc::get_compiler() → "original"
//
// Wrapper mit Override (z.B. Paper-Wrapper via Mixin):
//   class MimallocAllocator : public ..., public generated::a04_mimalloc::OriginalCodeMixin {
//     // Mixin definiert eigenes get_compiler() = "gcc-9.5"
//   };

#include <concepts>
#include <string_view>

namespace comdare::cache_engine::topics {

/**
 * @brief AxisBase — Cross-Axis Pflicht-Properties Wurzelklasse
 *
 * Alle Wrapper-Klassen (allocator/queuing/traversal/...) erben von AxisBase
 * oder einer abgeleiteten Mixin-Klasse die die Properties ueberschreibt.
 *
 * Pflicht-Properties (Default-Implementierung):
 *   - get_compiler() → "original"  (Override pro Paper-Wrapper)
 *
 * Erweiterbar: zukuenftige Cross-Axis-Properties hier ergaenzen (Default-Werte),
 * dann Mass-Update aller Wrappers nicht noetig (Default greift automatisch).
 */
struct AxisBase {
    /// Compiler-Identitaet pro Algorithmus.
    /// Standard "original" — kein konkreter Compiler-Bindung.
    /// Wrapper kann ueberschreiben (z.B. Paper-Wrapper mit "gcc-9.5", "clang-12").
    ///
    /// Konvention der Werte:
    ///   "original"  Default — beliebiger Compiler OK, keine Paper-Bindung
    ///   "self"      Re-Impl ohne Paper-Bindung (Pseudocode-Paper, eigene Erfindung)
    ///   "system"    C-Standard-libc (StdMalloc, PMR) — kein Paper
    ///   "gcc-9.5"   Konkreter Compiler aus Paper-Original-Build
    ///   "clang-12"  analog
    ///   "msvc-19.30" analog
    [[nodiscard]] static constexpr std::string_view get_compiler() noexcept {
        return "original";
    }
};

/**
 * @brief AxisBaseConcept — Compile-Time-Verifikation der Pflicht-API
 *
 * Jeder Wrapper-Concept (axis_<NN>_<topic>_cache_engine_permutation_concept)
 * SOLL via && Constraint diesen Concept anhaengen, sodass die Pflicht-API
 * statisch validiert wird.
 *
 * Bei Verletzung: klare Concept-Diagnose statt "missing method" Fehler.
 */
template <typename T>
concept AxisBaseConcept = requires {
    { T::get_compiler() } -> std::convertible_to<std::string_view>;
};

}  // namespace comdare::cache_engine::topics
