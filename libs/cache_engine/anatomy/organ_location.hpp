#pragma once
// INC-B / Fork R-B (2026-07-14) - Per-ORGAN-Location (Achsen-/Organ-Ebene).
//
// Analogon zu COMDARE_DEFINE_COMPOSITION_LOCATION (anatomy/composition_concept.hpp:103-105), aber fuer
// EINEN Achsen-Baustein/Organ-Wrapper statt einer ganzen 17-Achsen-Komposition. Motivation: ein
// name()->(FQ-Typ, Header)-Registry-Codegen (tools/axis_registry_gen INC-A, prt-art-Registry-Generator
// INC-B) kann den Header eines Organs NICHT aus dem Typ-Namen ableiten (fragil/fabriziert). Der Wrapper
// deklariert seine Lokation daher explizit - genau wie Reference-Compositions ihre COMPOSITION_LOCATION.
//
// Verifiziert (20260713-verify-registry-facts §2c): die prt-art-Slot-Wrapper + das prt-art-Merge-Organ
// tragen HEUTE name()/family_name()/flag_suffix(), aber KEIN cpp_type_name/header_include. Dieses Makro
// schliesst genau die Luecke (Fork R-B, bounded), rein ADDITIV: es fuegt zwei static constexpr
// string_view-Member hinzu, ohne bestehende Signaturen / name() / family_id / ABI zu beruehren.
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §49 (Codegen-Tool-Lokalisierung)
// @related anatomy/composition_concept.hpp (COMDARE_DEFINE_COMPOSITION_LOCATION / HasCompositionLocation)

#include <concepts>
#include <string_view>

namespace comdare::cache_engine::anatomy {

/// HasOrganLocation - Optional-Concept: ein Achsen-/Organ-Wrapper liefert seine Codegen-Lokalisierung.
/// Erfuellt von Wrappern, die COMDARE_DEFINE_ORGAN_LOCATION im Klassen-Body aufrufen.
///
/// Pflicht-Members:
/// - `cpp_type_name`  - fully-qualified C++ Type-Name des Wrappers
/// - `header_include` - Include-Pfad relativ zum jeweiligen Include-Root (fuer #include im Codegen)
template <typename W>
concept HasOrganLocation = requires {
    { W::cpp_type_name } -> std::convertible_to<std::string_view>;
    { W::header_include } -> std::convertible_to<std::string_view>;
};

/// COMDARE_DEFINE_ORGAN_LOCATION(TYPE_NAME, HEADER_PATH) - Convenience-Makro fuer Achsen-/Organ-Wrapper.
/// Im Body des Wrappers aufrufen (Parallele zu COMDARE_DEFINE_COMPOSITION_LOCATION):
///
/// ```cpp
/// class PrtArtBPlusPageType : public ... {
/// public:
///     static constexpr std::string_view name() noexcept { return "prtart_bplus_page"; }
///     COMDARE_DEFINE_ORGAN_LOCATION("::comdare::prt_art::slots::axis_01::PrtArtBPlusPageType",
///                                   "prt_art/slots/axis_01_page_type_slot.hpp");
/// };
/// ```
#define COMDARE_DEFINE_ORGAN_LOCATION(TYPE_NAME, HEADER_PATH)                                                          \
    static constexpr std::string_view cpp_type_name  = TYPE_NAME;                                                      \
    static constexpr std::string_view header_include = HEADER_PATH

} // namespace comdare::cache_engine::anatomy
