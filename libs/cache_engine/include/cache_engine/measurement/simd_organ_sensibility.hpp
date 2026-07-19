// measurement/simd_organ_sensibility.hpp -- Organ-Achsen-Klasse -> sinnvolle SIMD-Flags (Bau Section 40.a, E3).
//
// User-Direktive 2026-07-19: "Nicht alle AVX512-Flags sind fuer alle Organ-Achsen sinnvoll einzusetzen."
// Diese Datei traegt die ZENTRALE Sinnhaftigkeits-Matrix (Organ-Achsen-Klasse -> die einzelnen SIMD-Flags,
// die dort plausibel nutzbar sind) aus dem Referenzdoc-Abschnitt 5
// (docs/architektur/20260719-simd-flag-signaturen-REFERENZ.md). Sie ist die "Sinnhaftigkeit"-Seite der
// flag-genauen Freigabe-Kopplung (E4-Gate): Organ-Nutzung <= Maschinen-Signatur GESCHNITTEN Organ-Sinnhaftigkeit.
//
// VERORTUNG (Architekt-Ruling 2026-07-19 = "an die 09b-Wrapper, NICHT ueber 17 topics/-Achsen verstreuen"):
// zentral an EINEM Ort statt je Organ-Achse. Physisch liegt die Tabelle in measurement/ (bei Katalog +
// Maschinen-Signatur), weil die verifizierte Layering-Schranke topics/ -> cache_engine/measurement/ VERBIETET
// (topics inkludiert NIE cache_engine/measurement) -- die Tabelle referenziert die Organ-Achsen-Klassen daher
// per NAME-String (keine Organ-Header-Inklusion) und die Flags per Katalog-Handle. Zentralitaet der Direktive
// bleibt erfuellt (eine Tabelle, nicht verstreut). Rein additiv, golden==131072 unberuehrt (NIE binary_id).
//
// EHRLICHKEIT: spekulative Zuordnungen (value_handle/cache_traversal/scoring) sind mit speculative=true
// gekennzeichnet (Referenzdoc-Konfidenz [spekulativ]); prefetch traegt bewusst die LEERE Menge
// (Software-Prefetch ist NICHT SIMD-flag-gated). Metaprog: POD-Deskriptoren + constexpr-Helfer, keine vtable.

#pragma once

#include <cache_engine/measurement/simd_feature_flag.hpp>

#include <array>
#include <cstddef>
#include <span>
#include <string_view>

namespace comdare::cache_engine::measurement {

// Sinnvolle Flag-Mengen je Organ-Achsen-Klasse (Referenzdoc Abschnitt 5; Reihenfolge = Vokabular-Reihenfolge).
inline constexpr std::array kSensibilityFilterFlags{kAvx512Vpopcntdq, kAvx512Bitalg, kGfni};
inline constexpr std::array kSensibilitySearchAlgoFlags{kAvx512Bw, kAvx512Dq, kAvx512F, kAvx512Vl};
inline constexpr std::array kSensibilityMemoryLayoutFlags{kAvx512Vbmi, kAvx512Vbmi2, kAvx512Bw};
inline constexpr std::array kSensibilityMappingFlags{kGfni, kVpclmulqdq, kAvx512Cd};
inline constexpr std::array kSensibilityIndexOrgFlags{kBmi2, kAvx512Vbmi, kAvx512Vpopcntdq};
inline constexpr std::array kSensibilityValueHandleFlags{kAvx512Vbmi2, kVpclmulqdq};      // [spekulativ]
inline constexpr std::array kSensibilityCacheTraversalFlags{kAvx2, kAvx512F};             // [spekulativ] Gather
inline constexpr std::array kSensibilityScoringFlags{kAvxVnni, kAvx512Vnni, kAvx512Bf16}; // [spekulativ] int8/bf16
inline constexpr std::array<SimdFeatureFlag, 0> kSensibilityPrefetchFlags{};              // prefetch: KEIN SIMD-Flag

// Eine Zeile der Sinnhaftigkeits-Matrix: Organ-Klasse (per Name-String) -> plausibel nutzbare Flags.
struct OrganSimdSensibility {
    std::string_view                 organ_class; // Organ-Achsen-Klassen-Id (z.B. "filter", "search_algo")
    std::span<SimdFeatureFlag const> meaningful;  // dort plausibel nutzbare Einzel-Flags
    bool                             speculative; // ehrliche Kennzeichnung nicht-solider Zuordnungen
};

// Zentrale Sinnhaftigkeits-Matrix (Single-Source). prefetch traegt die LEERE Menge (ehrlich, nicht ausgelassen).
inline constexpr std::array<OrganSimdSensibility, 9> kSimdOrganSensibility{{
    {"filter", kSensibilityFilterFlags, false},
    {"search_algo", kSensibilitySearchAlgoFlags, false},
    {"memory_layout", kSensibilityMemoryLayoutFlags, false},
    {"mapping", kSensibilityMappingFlags, false},
    {"index_organization", kSensibilityIndexOrgFlags, false},
    {"value_handle", kSensibilityValueHandleFlags, true},
    {"cache_traversal", kSensibilityCacheTraversalFlags, true},
    {"scoring", kSensibilityScoringFlags, true},
    {"prefetch", kSensibilityPrefetchFlags, false},
}};

// -- constexpr-Helfer (Configure-/Planer-Zeit; E4-Gate nutzt is_flag_meaningful_for) --------------
[[nodiscard]] constexpr std::span<SimdFeatureFlag const> sensibility_of(std::string_view organ_class) noexcept {
    for (auto const& e : kSimdOrganSensibility)
        if (e.organ_class == organ_class) return e.meaningful;
    return {};
}

[[nodiscard]] constexpr bool is_flag_meaningful_for(std::string_view organ_class, std::string_view cpuinfo) noexcept {
    for (auto const& f : sensibility_of(organ_class))
        if (f.cpuinfo == cpuinfo) return true;
    return false;
}

// Jede Sinnhaftigkeits-Zuordnung referenziert nur Katalog-bekannte Flags (Single-Source-Kopplung zum Katalog).
[[nodiscard]] constexpr bool organ_sensibility_within_catalog() noexcept {
    for (auto const& e : kSimdOrganSensibility)
        for (auto const& f : e.meaningful)
            if (!is_known_simd_flag(f.cpuinfo)) return false;
    return true;
}

static_assert(kSimdOrganSensibility.size() == 9);
static_assert(organ_sensibility_within_catalog());
static_assert(is_flag_meaningful_for("filter", "avx512_vpopcntdq"));
static_assert(!is_flag_meaningful_for("filter", "avx512_bf16")); // bf16 gehoert nicht zu filter
static_assert(is_flag_meaningful_for("mapping", "gfni"));
static_assert(is_flag_meaningful_for("index_organization", "bmi2"));
static_assert(sensibility_of("prefetch").empty()); // prefetch: NICHT SIMD-flag-gated
static_assert(!is_flag_meaningful_for("nicht_existente_klasse", "avx2"));

} // namespace comdare::cache_engine::measurement
