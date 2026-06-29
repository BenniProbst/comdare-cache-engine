#pragma once
// V41.F.6.1 R5.G — constexpr type_name<T>(): fully-qualified C++-Typ-Name als string_view zur Compile-Time.
//
// Grundlage des R5.G-AUTO-EMITTERS: der Generator iteriert via for_each_composition_type die
// enumerierten Permutationen und braucht pro Achsen-Vendor-Typ dessen FULLY-QUALIFIED C++-Typ-Namen
// als STRING, um ein Modul-.cpp mit COMDARE_DEFINE_ANATOMY_MODULE_ADHOC(<17 Typ-Namen>) zu emittieren.
//
// Technik (nameof-Pattern): der Compiler-Funktions-Signatur-String (__FUNCSIG__ / __PRETTY_FUNCTION__)
// enthält den Template-Typ; Prefix/Suffix werden an einem Probe-Typ (double) kalibriert + abgeschnitten.
// MSVC rendert Klassen als "class NS::T" → führendes "class "/"struct "/"enum " wird entfernt, sodass
// der reine FQ-Name (NS::T) für Codegen nutzbar ist.

#include <string_view>

namespace comdare::cache_engine::builder::codegen {

namespace detail {

template <class T>
[[nodiscard]] constexpr std::string_view raw_signature() noexcept {
#if defined(_MSC_VER)
    return __FUNCSIG__;
#else
    return __PRETTY_FUNCTION__;
#endif
}

// Kalibrierung an einem Probe-Typ ("double" — 6 Zeichen, in jeder Signatur identisch gerendert).
inline constexpr std::string_view kProbe     = raw_signature<double>();
inline constexpr std::size_t      kProbePos  = kProbe.find("double");
inline constexpr std::size_t      kPrefixLen = kProbePos;
inline constexpr std::size_t      kSuffixLen = kProbe.size() - kProbePos - 6; // 6 == len("double")

/// Entfernt ein führendes Elaborated-Type-Specifier-Keyword (MSVC: "class "/"struct "/"enum ").
/// HINWEIS (2026-06-03): schält NUR den ÄUSSEREN Specifier. MSVC rendert in __FUNCSIG__ auch verschachtelte
/// Template-Argumente mit "class "/"struct "-Prefix (z.B. `Outer<class NS::Inner>`); die INNEN-liegenden Keywords
/// bleiben hier stehen und werden host-seitig vom Emitter entfernt (adhoc_emitter.hpp strip_all_elaborated, der
/// einzige Konsument, der den Namen in echten C++-Quelltext schreibt). type_name() selbst bleibt unverändert
/// (constexpr-string_view, kein Heap/Lifetime-Risiko).
[[nodiscard]] constexpr std::string_view strip_elaborated(std::string_view s) noexcept {
    for (std::string_view kw : {std::string_view{"class "}, std::string_view{"struct "}, std::string_view{"enum "},
                                std::string_view{"union "}}) {
        if (s.substr(0, kw.size()) == kw) return s.substr(kw.size());
    }
    return s;
}

} // namespace detail

/// type_name<T>() — fully-qualified Typ-Name von T (Compile-Time, ohne FÜHRENDEN "class/struct"-Prefix).
template <class T>
[[nodiscard]] constexpr std::string_view type_name() noexcept {
    std::string_view s = detail::raw_signature<T>();
    s.remove_prefix(detail::kPrefixLen);
    s.remove_suffix(detail::kSuffixLen);
    return detail::strip_elaborated(s);
}

} // namespace comdare::cache_engine::builder::codegen
