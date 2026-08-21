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
// EHRLICHKEIT: spekulative Zuordnungen (value_handle/cache_traversal) sind mit speculative=true
// gekennzeichnet (Referenzdoc-Konfidenz [spekulativ]); prefetch traegt bewusst die LEERE Menge
// (Software-Prefetch ist NICHT SIMD-flag-gated). Metaprog: POD-Deskriptoren + constexpr-Helfer, keine vtable.
//
// B03/K06 (2026-08-21, Nenner-Fix): die Matrix fuehrt ALLE 18 Organ-Haupt-Achsen in der
// kCompositionAxisNames-Reihenfolge (Positions-Pin in test_simd_organ_achsen_deckung.cpp; measurement
// darf die builder-Registry nicht sehen). HISTORIE "scoring": bis 2026-08-21 fuehrte die Matrix einen
// neunten Eintrag "scoring" {avx_vnni, avx512_vnni, avx512_bf16} [spekulativ, int8/bf16-Scoring] --
// "scoring" ist KEINE der 18 Kompositions-Achsen und hatte 0 Konsumenten (Geister-Name, entfernt).
// Wer eine echte Scoring-Achse einzieht, findet die Flag-Kandidaten hier im Kommentar, nicht als
// stillen Tabellen-Geist. Die 10 nachgezogenen Achsen tragen die LEERE Menge (der ehrliche Stand --
// eine spekulative Zuordnung waere eine NEUE Behauptung, kein Nenner-Fix).

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
inline constexpr std::array kSensibilityValueHandleFlags{kAvx512Vbmi2, kVpclmulqdq}; // [spekulativ]
inline constexpr std::array kSensibilityCacheTraversalFlags{kAvx2, kAvx512F};        // [spekulativ] Gather
// B03: kSensibilityScoringFlags {kAvxVnni, kAvx512Vnni, kAvx512Bf16} entfernt -- "scoring" war ein
// Geister-Name ohne Kompositions-Achse und ohne Konsumenten (Historie + Flag-Kandidaten im Kopf).
inline constexpr std::array<SimdFeatureFlag, 0> kSensibilityPrefetchFlags{}; // prefetch: KEIN SIMD-Flag

// Eine Zeile der Sinnhaftigkeits-Matrix: Organ-Klasse (per Name-String) -> plausibel nutzbare Flags.
struct OrganSimdSensibility {
    std::string_view                 organ_class; // Organ-Achsen-Klassen-Id (z.B. "filter", "search_algo")
    std::span<SimdFeatureFlag const> meaningful;  // dort plausibel nutzbare Einzel-Flags
    bool                             speculative; // ehrliche Kennzeichnung nicht-solider Zuordnungen
};

// Die leere Menge der nicht-SIMD-gebundenen Achsen (B03: benannt, damit die 10 nachgezogenen
// Eintraege dieselbe Quelle teilen wie prefetch -- eine Leere, nicht zehn).
inline constexpr std::array<SimdFeatureFlag, 0> kSensibilityNoneFlags{};

// Zentrale Sinnhaftigkeits-Matrix (Single-Source). Leere Mengen sind EHRLICH, nicht ausgelassen.
// B03 (2026-08-21): 18/18 in kCompositionAxisNames-Reihenfolge (vorher 9 inkl. Geist "scoring", s. Kopf).
inline constexpr std::array<OrganSimdSensibility, 18> kSimdOrganSensibility{{
    {"search_algo", kSensibilitySearchAlgoFlags, false},
    {"cache_traversal", kSensibilityCacheTraversalFlags, true},
    {"mapping", kSensibilityMappingFlags, false},
    {"path_compression", kSensibilityNoneFlags, false},
    {"node_type", kSensibilityNoneFlags, false},
    {"memory_layout", kSensibilityMemoryLayoutFlags, false},
    {"allocator", kSensibilityNoneFlags, false},
    {"prefetch", kSensibilityPrefetchFlags, false},
    {"concurrency", kSensibilityNoneFlags, false},
    {"serialization", kSensibilityNoneFlags, false},
    {"value_handle", kSensibilityValueHandleFlags, true},
    {"index_organization", kSensibilityIndexOrgFlags, false},
    {"io_dispatch", kSensibilityNoneFlags, false},
    {"migration_policy", kSensibilityNoneFlags, false},
    {"filter", kSensibilityFilterFlags, false},
    {"queuing_q1", kSensibilityNoneFlags, false},
    {"queuing_q2", kSensibilityNoneFlags, false},
    {"persistence_target", kSensibilityNoneFlags, false},
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

static_assert(kSimdOrganSensibility.size() == 18,
              "B03: alle 18 Organ-Haupt-Achsen (kCompositionAxisNames-Nenner; Positions-Pin im Test).");
static_assert(organ_sensibility_within_catalog());
// B03: Duplikatfreiheit der 18 Namen (die Registry-Gleichheit selbst prueft der Test):
static_assert(
    [] {
        for (std::size_t i = 0; i < kSimdOrganSensibility.size(); ++i)
            for (std::size_t j = i + 1; j < kSimdOrganSensibility.size(); ++j)
                if (kSimdOrganSensibility[i].organ_class == kSimdOrganSensibility[j].organ_class) return false;
        return true;
    }(),
    "B03: doppelter Organ-Klassen-Name in kSimdOrganSensibility.");
static_assert(sensibility_of("scoring").empty()); // B03: der Geist ist RAUS (unbekannt -> leer)
static_assert(is_flag_meaningful_for("filter", "avx512_vpopcntdq"));
static_assert(!is_flag_meaningful_for("filter", "avx512_bf16")); // bf16 gehoert nicht zu filter
static_assert(is_flag_meaningful_for("mapping", "gfni"));
static_assert(is_flag_meaningful_for("index_organization", "bmi2"));
static_assert(sensibility_of("prefetch").empty()); // prefetch: NICHT SIMD-flag-gated
static_assert(!is_flag_meaningful_for("nicht_existente_klasse", "avx2"));

} // namespace comdare::cache_engine::measurement
