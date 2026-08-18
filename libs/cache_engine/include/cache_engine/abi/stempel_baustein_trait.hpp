#pragma once
// S-1 (P2-Schnitt, ausgelagert 12.08.2026): der LEICHTE Baustein-Trait -- Rolle, Primaertrait, Tag
// und Concept der Stempel-Baustein-Anbindung. Ausgelagert aus stempel_basis.hpp (P1.4), damit die
// beiden ABI-POD-Spezialisierungen DIREKT beim Eigentuemer (anatomy_module_abi_v1_decl.hpp) stehen
// koennen, ohne dass die bewusst leichte Loader-Seite den vollen Vertrags-Header zieht: eine TU, die
// decl + stempel_basis sah und den Trait VOR anatomy_stamp_entries.hpp instanziierte, las vorher
// still false/Keine -- und ein spaeteres entries-Include war "specialization after instantiation"
// (include-order-abhaengig ill-formed; Zweitlens-Befund 12.08.). Dieser Header braucht NUR
// <cstdint>/<type_traits> und ist von beiden Seiten gefahrlos ziehbar.

#include <cstdint>
#include <type_traits>

namespace comdare::cache_engine::abi {

/// Die Rollen der angebundenen Bestands-Bausteine (KON6-03 + Naht/Legende/ToolingListe).
enum class StempelBausteinRolle : std::uint8_t {
    Keine = 0,
    Segment,                ///< detail::StampSegment (anatomy_stamp_entries.hpp)
    ZeilenLiteral,          ///< detail::StampLineLiteral<N> (anatomy_stamp_entries.hpp)
    VervollstaendigteZeile, ///< CompletedSystemStampLine<N> (system_cell_values.hpp)
    GliedParts,             ///< ToolchainStampParts (toolchain_stamp_glied.hpp)
    AbiPod,                 ///< AnatomyStampEntryV1 + AnatomyVersionLines (anatomy_module_abi_v1_decl.hpp)
    Legende,                ///< builder::CebComboLegend<N> (ceb_version_stamp.hpp)
    ToolingListe,           ///< builder::detail::CebToolingList (ceb_version_stamp.hpp)
    NahtGlied               ///< profile_facade::PermToolchainGliedWert/-Achsen (toolchain_stamp_naht.hpp)
};

/// Primaertrait: KEIN Baustein. Die Anbindung ist je Typ eine Spezialisierung UNTER seiner Definition --
/// OHNE Basisklassen-Einbau: alle fuenf KON6-03-Strukturen sind positional-init-Aggregate bzw.
/// NTTP-Traeger; eine leere Basis fraesse den ERSTEN positionellen Initialisierer (Pins
/// anatomy_module_abi_v1_decl.hpp:192/:254/:261; Feld-Ordnungs-Wache toolchain_stamp_glied.hpp:300-311
/// bleibt dadurch UNVERAENDERT).
template <class T>
struct ist_stempel_baustein {
    static constexpr bool                 wert  = false;
    static constexpr StempelBausteinRolle rolle = StempelBausteinRolle::Keine;
};

/// Kurzform der Spezialisierungen (haelt jede Anbindung bei +2 Zeilen).
template <StempelBausteinRolle R>
struct StempelBausteinTag {
    static constexpr bool                 wert  = R != StempelBausteinRolle::Keine;
    static constexpr StempelBausteinRolle rolle = R;
};

/// concept StempelBaustein -- die Frage "ist dieser Typ ein angebundener Stempel-Baustein?".
template <class T>
concept StempelBaustein = ist_stempel_baustein<std::remove_cvref_t<T>>::wert;

static_assert(!StempelBaustein<int>, "der Primaertrait bindet NICHTS an -- Anbindung ist je Typ explizit.");
static_assert(StempelBausteinTag<StempelBausteinRolle::Segment>::wert);
static_assert(!StempelBausteinTag<StempelBausteinRolle::Keine>::wert,
              "eine Spezialisierung mit Rolle 'Keine' waere eine Anbindungs-Luege.");

} // namespace comdare::cache_engine::abi
