#pragma once
// V41.F.6.1.R5.J — Known-Reference-Compositions als mp_list (Tool-Iteration)
//
// User-Direktive 2026-05-27 (Doku 14 §51.9):
// R5.J ersetzt hardcoded Tool-Tabelle durch mp_list-Cartesian-Iteration.
// Entry-Wrapper-Pattern buendelt:
//   - `composition` (Type-Alias auf die Composition selbst, cpp_type_name +
//     header_include kommen aus deren HasCompositionLocation-Traits, R5.G)
//   - `short_name` (Tool-spezifischer CLI-User-friendly Bezeichner)
//
// Drift-Eliminierung: neue Reference-Composition braucht nur einen neuen
// Entry-Wrapper + Eintrag in KnownReferenceCompositions — Tool iteriert
// automatisch via mp_for_each ohne Tabellen-Edit.
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §52
// @task #714 V41.F.6.1.R5.J

#include "art_reference.hpp"
#include "hot_reference.hpp"
#include "wormhole_reference.hpp"
#include "surf_reference.hpp"
#include "masstree_reference.hpp"
#include "start_reference.hpp"
#include "art_paper_binding_reference.hpp"
#include "hot_paper_binding_reference.hpp"
#include "start_paper_binding_reference.hpp"
#include "wormhole_paper_binding_reference.hpp"
#include "surf_paper_binding_reference.hpp"

#include <boost/mp11.hpp>

#include <string_view>

namespace comdare::cache_engine::compositions {

// ─────────────────────────────────────────────────────────────────────────────
// Entry-Wrapper: Composition-Type + Tool-spezifischer Short-Name
// ─────────────────────────────────────────────────────────────────────────────
//
// Konvention: short_name ist user-facing CLI-Argument (z.B. "art" statt
// "ArtComposition"). cpp_type_name + header_include kommen aus den Traits
// der composition selbst — werden NICHT redundant im Entry gespiegelt.

struct ArtEntry      { using composition = ArtComposition;      static constexpr std::string_view short_name = "art"; };
struct HotEntry      { using composition = HotComposition;      static constexpr std::string_view short_name = "hot"; };
struct WormholeEntry { using composition = WormholeComposition; static constexpr std::string_view short_name = "wormhole"; };
struct SurfEntry     { using composition = SurfComposition;     static constexpr std::string_view short_name = "surf"; };
struct MasstreeEntry { using composition = MasstreeComposition; static constexpr std::string_view short_name = "masstree"; };
struct StartEntry    { using composition = StartComposition;    static constexpr std::string_view short_name = "start"; };

struct ArtPaperBindingEntry      { using composition = ArtPaperBindingComposition;      static constexpr std::string_view short_name = "art_pb"; };
struct HotPaperBindingEntry      { using composition = HotPaperBindingComposition;      static constexpr std::string_view short_name = "hot_pb"; };
struct StartPaperBindingEntry    { using composition = StartPaperBindingComposition;    static constexpr std::string_view short_name = "start_pb"; };
struct WormholePaperBindingEntry { using composition = WormholePaperBindingComposition; static constexpr std::string_view short_name = "wormhole_pb"; };
struct SurfPaperBindingEntry     { using composition = SurfPaperBindingComposition;     static constexpr std::string_view short_name = "surf_pb"; };

// ─────────────────────────────────────────────────────────────────────────────
// KnownReferenceCompositions — zentrale mp_list aller Entry-Wrappers
// ─────────────────────────────────────────────────────────────────────────────
//
// Reihenfolge: 6 CE-Reimpl zuerst, dann 5 PaperBinding (analog R5.F-Tabelle).
// Tool iteriert via `boost::mp11::mp_for_each<KnownReferenceCompositions>(...)`.

using KnownReferenceCompositions = boost::mp11::mp_list<
    // 6 CE-Reimpl
    ArtEntry,
    HotEntry,
    WormholeEntry,
    SurfEntry,
    MasstreeEntry,
    StartEntry,
    // 5 PaperBinding
    ArtPaperBindingEntry,
    HotPaperBindingEntry,
    StartPaperBindingEntry,
    WormholePaperBindingEntry,
    SurfPaperBindingEntry
>;

// Compile-Time-Count fuer Sanity-Check
inline constexpr std::size_t kKnownReferenceCompositionsCount =
    boost::mp11::mp_size<KnownReferenceCompositions>::value;
static_assert(kKnownReferenceCompositionsCount == 11,
              "R5.J: Stand 2026-05-27 sind 11 Reference-Compositions registriert "
              "(6 CE-Reimpl + 5 PaperBinding). Bei Aenderung auch anatomy_codegen_tool-"
              "Tests + Doku 14 §52 aktualisieren.");

}  // namespace comdare::cache_engine::compositions
