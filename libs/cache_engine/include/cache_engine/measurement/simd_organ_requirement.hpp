// measurement/simd_organ_requirement.hpp -- zentrale Organ-required-SIMD-Flag-Deklaration (Bau Section 40.a-E4).
//
// Der Deklarations-Hook je Organ-Achsen-Klasse fuer HART benoetigte SIMD-Flags (Default LEER). Ein Organ, das
// eine Auspraegung NUR mit einem bestimmten Flag ueberhaupt bauen kann, traegt es hier ein; solange die Menge
// LEER ist (heutiger Stand ALLER Organe), ist das Bau-Gate NotApplicable/inert -> Ist-Verhalten byte-identisch.
// Die erste echte required-Deklaration aktiviert die Freigabe-Kopplung (Section 37: Organ <= Maschinen-Signatur)
// OHNE weiteren Umbau -- die Aggregation aus der Organ-Signatur (aggregate_required_for_axes) und das Gate
// (simd_build_gate.hpp) stehen bereit.
//
// VERORTUNG: zentral in measurement/ (bei Katalog/Sinnhaftigkeit/Signatur), aus demselben Layering-Grund wie
// simd_organ_sensibility.hpp -- topics/ darf nicht auf cache_engine/measurement rueck-abhaengen. Die Organ-
// Klassen werden per NAME-String referenziert (keine Organ-Header-Inklusion). NIE binary_id.
//
// Metaprog: POD-Deskriptoren + constexpr/inline-Freifunktionen, keine vtable, kein std::variant.

#pragma once

#include <cache_engine/measurement/simd_feature_flag.hpp>

#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace comdare::cache_engine::measurement {

// Eine Zeile der required-Deklaration: Organ-Klasse (per Name) -> HART benoetigte Flags (Default leer).
struct OrganSimdRequirement {
    std::string_view                 organ_class;
    std::span<SimdFeatureFlag const> required; // LEER = keine harte Anforderung (Gate NotApplicable)
};

// Default-Leermenge (ein Organ opt-in't, indem es hier eine eigene nicht-leere Menge einsetzt).
inline constexpr std::array<SimdFeatureFlag, 0> kRequiredNone{};

// Zentrale required-Deklaration: HEUTE traegt JEDES Organ die leere Menge (byte-neutral). Reihenfolge = Sinnhaftigkeit.
inline constexpr std::array<OrganSimdRequirement, 9> kSimdOrganRequirement{{
    {"filter", kRequiredNone},
    {"search_algo", kRequiredNone},
    {"memory_layout", kRequiredNone},
    {"mapping", kRequiredNone},
    {"index_organization", kRequiredNone},
    {"value_handle", kRequiredNone},
    {"cache_traversal", kRequiredNone},
    {"scoring", kRequiredNone},
    {"prefetch", kRequiredNone},
}};

// Die harten required-Flags einer Organ-Klasse (leer, wenn nicht deklariert).
[[nodiscard]] constexpr std::span<SimdFeatureFlag const> required_of(std::string_view organ_class) noexcept {
    for (auto const& e : kSimdOrganRequirement)
        if (e.organ_class == organ_class) return e.required;
    return {};
}

// Deklariert IRGENDEIN Organ harte required-Flags? (Aktivierungs-Indikator; heute false -> Gate global inert.)
[[nodiscard]] constexpr bool any_organ_declares_required() noexcept {
    for (auto const& e : kSimdOrganRequirement)
        if (!e.required.empty()) return true;
    return false;
}

// Per-Binary-Aggregation: aus der Organ-Signatur (den (achse,wert)-Paaren einer Tier-Binary) die Vereinigung
// der harten required-Flags. HEUTE stets leer (kein Organ deklariert required) -> Gate NotApplicable -> byte-neutral.
// Der erste Achsen-Name eines Paares ist die Organ-Klasse (deckungsgleich zu den Sinnhaftigkeits-/required-Keys).
[[nodiscard]] inline std::vector<SimdFeatureFlag>
aggregate_required_for_axes(std::span<std::pair<std::string, std::string> const> axes) {
    std::vector<SimdFeatureFlag> out;
    for (auto const& [axis, value] : axes) {
        (void)value;
        for (auto const& f : required_of(axis)) {
            bool seen = false;
            for (auto const& g : out)
                if (g.cpuinfo == f.cpuinfo) {
                    seen = true;
                    break;
                }
            if (!seen) out.push_back(f);
        }
    }
    return out;
}

static_assert(kSimdOrganRequirement.size() == 9);
static_assert(!any_organ_declares_required()); // heutiger Stand: ALLE leer -> Gate inert -> byte-/golden-neutral
static_assert(required_of("filter").empty());
static_assert(required_of("search_algo").empty());
static_assert(required_of("nicht_existente_klasse").empty());

} // namespace comdare::cache_engine::measurement
