// measurement/simd_feature_flag.hpp -- feingranularer SIMD-Befehlssatz-Flag-Katalog (Bau Section 40.a, E1).
//
// User-Direktive 2026-07-19: "AVX512 besitzt eine Signatur aus Flags; nicht jede CPU hat alle
// AVX512-Flags. Jedes einzelne AVX512-Flag muss einzeln als von der Maschine vorhandene Hardware
// deklariert werden." Diese Datei traegt die CODE=WAHRHEIT-Vokabular-Quelle EINZELNER SIMD-Flags
// (proc/cpuinfo-Ebene) -- NICHT den Grob-Level avx2/avx512 (der bleibt Routing-Vorstufe in
// simd_sub_axis.hpp). Aus diesem Katalog generiert der System-Registry-Generator sein Angebot (E2),
// leiten Organ-Achsen ihre Sinnhaftigkeit ab (E3) und schneidet das Bau-Gate Organ <= Maschinen-
// Signatur geschnitten Sinnhaftigkeit (E4). Rein additiv, golden==131072 unberuehrt (System-Flags
// gehen an die CompileFn-Naht, NIE in die binary_id).
//
// Belegt durch das live-verifizierte Referenzdoc docs/architektur/20260719-simd-flag-signaturen-REFERENZ.md
// (prod1/Zen5 100%-Abgleich gegen /proc/cpuinfo). WICHTIG (Unterstrich-Falle, realer Kernel-cpuinfo-String):
// avx512vl/dq/bw/cd/f/ifma/vbmi OHNE Unterstrich; avx512_vbmi2/_vnni/_bitalg/_vpopcntdq/_vp2intersect/
// _bf16/_fp16 MIT. Die cpuinfo-Id (Signatur-Matching) und das g++-Flag (Compile) sind GETRENNT gefuehrt,
// NIE per String-Heuristik ineinander umgerechnet.
//
// Metaprog: reine POD-Deskriptoren (trivially-copyable) + benannte constexpr-Handles als Single-Source;
// KEIN std::variant, KEINE vtable, alles constexpr (Configure-/Planer-Zeit, kein Runtime-Hot-Path-Switch).

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::measurement {

// Grobe Familien-Einordnung (Routing-Vorstufe gegen Bau-Signatur; die grobe simd-Sub-Achse
// {no_extension,avx2,avx512} mappt auf diese Tiers, entscheidet aber NICHT die einzelnen Flags).
enum class SimdFlagTier : std::uint8_t {
    Avx256    = 0, // 256-bit VEX-Basisfamilie (avx2/fma/f16c/avx_vnni) -- quasi universell
    Avx512    = 1, // AVX-512 EVEX-Subsets -- je Maschine EINZELN vorhanden/fehlend
    Companion = 2, // VEX/EVEX-Begleiter (gfni/vaes/vpclmulqdq) -- auch ohne AVX-512 vorhanden
    Scalar    = 3, // skalare Bit-Manip-Begleiter (bmi2/popcnt)
};

// Ein einzelnes SIMD-Flag: cpuinfo-Name (Signatur-Matching) + g++/clang-Flag (Compile) + Tier.
// POD, trivially-copyable -- taugt als constexpr-Nichttyp-Wert fuer Signatur-/Anforderungs-Arrays.
struct SimdFeatureFlag {
    std::string_view cpuinfo; // exakter /proc/cpuinfo-String (Signatur-Schluessel)
    std::string_view gpp;     // g++ -m<flag>
    std::string_view clang;   // clang++ -m<flag> (bei diesen Flags deckungsgleich zu gpp)
    SimdFlagTier     tier;
};

// cpuinfo-Name = Identitaet (eindeutig, stabil).
[[nodiscard]] constexpr bool operator==(SimdFeatureFlag const& a, SimdFeatureFlag const& b) noexcept {
    return a.cpuinfo == b.cpuinfo;
}

// Stabiles Etikett je Tier (Single-Source fuer die Registry-Serialisierung; kein Roh-String am Emit-Ort).
[[nodiscard]] constexpr std::string_view tier_label(SimdFlagTier t) noexcept {
    switch (t) {
        case SimdFlagTier::Avx256: return "avx256";
        case SimdFlagTier::Avx512: return "avx512";
        case SimdFlagTier::Companion: return "companion";
        case SimdFlagTier::Scalar: return "scalar";
    }
    return "unbekannt"; // out-of-range-Cast -> sichtbarer Default (kein UB, kein stiller Skip)
}

// -- Benannte Single-Source-Handles je Flag (Call-Sites referenzieren diese, nie Roh-Strings). --
// AVX-512-Subsets (Tier Avx512). g++-Flag-Schreibweise deckungsgleich zur codegen-Praezedenz.
inline constexpr SimdFeatureFlag kAvx512F{"avx512f", "-mavx512f", "-mavx512f", SimdFlagTier::Avx512};
inline constexpr SimdFeatureFlag kAvx512Cd{"avx512cd", "-mavx512cd", "-mavx512cd", SimdFlagTier::Avx512};
inline constexpr SimdFeatureFlag kAvx512Vl{"avx512vl", "-mavx512vl", "-mavx512vl", SimdFlagTier::Avx512};
inline constexpr SimdFeatureFlag kAvx512Dq{"avx512dq", "-mavx512dq", "-mavx512dq", SimdFlagTier::Avx512};
inline constexpr SimdFeatureFlag kAvx512Bw{"avx512bw", "-mavx512bw", "-mavx512bw", SimdFlagTier::Avx512};
inline constexpr SimdFeatureFlag kAvx512Ifma{"avx512ifma", "-mavx512ifma", "-mavx512ifma", SimdFlagTier::Avx512};
inline constexpr SimdFeatureFlag kAvx512Vbmi{"avx512vbmi", "-mavx512vbmi", "-mavx512vbmi", SimdFlagTier::Avx512};
inline constexpr SimdFeatureFlag kAvx512Vbmi2{"avx512_vbmi2", "-mavx512vbmi2", "-mavx512vbmi2", SimdFlagTier::Avx512};
inline constexpr SimdFeatureFlag kAvx512Vnni{"avx512_vnni", "-mavx512vnni", "-mavx512vnni", SimdFlagTier::Avx512};
inline constexpr SimdFeatureFlag kAvx512Bitalg{"avx512_bitalg", "-mavx512bitalg", "-mavx512bitalg",
                                               SimdFlagTier::Avx512};
inline constexpr SimdFeatureFlag kAvx512Vpopcntdq{"avx512_vpopcntdq", "-mavx512vpopcntdq", "-mavx512vpopcntdq",
                                                  SimdFlagTier::Avx512};
inline constexpr SimdFeatureFlag kAvx512Vp2intersect{"avx512_vp2intersect", "-mavx512vp2intersect",
                                                     "-mavx512vp2intersect", SimdFlagTier::Avx512};
inline constexpr SimdFeatureFlag kAvx512Bf16{"avx512_bf16", "-mavx512bf16", "-mavx512bf16", SimdFlagTier::Avx512};
inline constexpr SimdFeatureFlag kAvx512Fp16{"avx512_fp16", "-mavx512fp16", "-mavx512fp16", SimdFlagTier::Avx512};
// 256-bit VEX-Basisfamilie (Tier Avx256).
inline constexpr SimdFeatureFlag kAvx2{"avx2", "-mavx2", "-mavx2", SimdFlagTier::Avx256};
inline constexpr SimdFeatureFlag kFma{"fma", "-mfma", "-mfma", SimdFlagTier::Avx256};
inline constexpr SimdFeatureFlag kF16c{"f16c", "-mf16c", "-mf16c", SimdFlagTier::Avx256};
inline constexpr SimdFeatureFlag kAvxVnni{"avx_vnni", "-mavxvnni", "-mavxvnni", SimdFlagTier::Avx256};
// VEX/EVEX-Begleiter (Tier Companion).
inline constexpr SimdFeatureFlag kGfni{"gfni", "-mgfni", "-mgfni", SimdFlagTier::Companion};
inline constexpr SimdFeatureFlag kVaes{"vaes", "-mvaes", "-mvaes", SimdFlagTier::Companion};
inline constexpr SimdFeatureFlag kVpclmulqdq{"vpclmulqdq", "-mvpclmulqdq", "-mvpclmulqdq", SimdFlagTier::Companion};
// Skalare Bit-Manip-Begleiter (Tier Scalar).
inline constexpr SimdFeatureFlag kBmi2{"bmi2", "-mbmi2", "-mbmi2", SimdFlagTier::Scalar};
inline constexpr SimdFeatureFlag kPopcnt{"popcnt", "-mpopcnt", "-mpopcnt", SimdFlagTier::Scalar};

// Single-Source-Katalog (Vokabular ALLER bekannten Flags; Reihenfolge stabil = Generator-Emit-Reihenfolge).
inline constexpr std::array<SimdFeatureFlag, 23> kSimdFeatureFlagCatalog{kAvx512F,
                                                                         kAvx512Cd,
                                                                         kAvx512Vl,
                                                                         kAvx512Dq,
                                                                         kAvx512Bw,
                                                                         kAvx512Ifma,
                                                                         kAvx512Vbmi,
                                                                         kAvx512Vbmi2,
                                                                         kAvx512Vnni,
                                                                         kAvx512Bitalg,
                                                                         kAvx512Vpopcntdq,
                                                                         kAvx512Vp2intersect,
                                                                         kAvx512Bf16,
                                                                         kAvx512Fp16,
                                                                         kAvx2,
                                                                         kFma,
                                                                         kF16c,
                                                                         kAvxVnni,
                                                                         kGfni,
                                                                         kVaes,
                                                                         kVpclmulqdq,
                                                                         kBmi2,
                                                                         kPopcnt};

// -- constexpr-Helfer (Configure-/Planer-Zeit; kein Runtime-Hot-Path-Switch) ----------------------
[[nodiscard]] constexpr bool is_known_simd_flag(std::string_view cpuinfo) noexcept {
    for (auto const& f : kSimdFeatureFlagCatalog)
        if (f.cpuinfo == cpuinfo) return true;
    return false;
}

// g++-Flag zu einem bekannten cpuinfo-Namen (E4: CompileFn-Ableitung aus der Schnittmenge). Leer = unbekannt.
[[nodiscard]] constexpr std::string_view gpp_flag_for(std::string_view cpuinfo) noexcept {
    for (auto const& f : kSimdFeatureFlagCatalog)
        if (f.cpuinfo == cpuinfo) return f.gpp;
    return {};
}

[[nodiscard]] constexpr std::size_t count_flags_of_tier(SimdFlagTier tier) noexcept {
    std::size_t n = 0;
    for (auto const& f : kSimdFeatureFlagCatalog)
        if (f.tier == tier) ++n;
    return n;
}

// -- Vokabular-Wohlgeformtheit (Single-Source-Garantien, alles compile-time) ----------------------
[[nodiscard]] constexpr bool catalog_entries_nonempty() noexcept {
    for (auto const& f : kSimdFeatureFlagCatalog)
        if (f.cpuinfo.empty() || f.gpp.empty() || f.clang.empty()) return false;
    return true;
}
[[nodiscard]] constexpr bool catalog_ids_unique() noexcept {
    for (std::size_t i = 0; i < kSimdFeatureFlagCatalog.size(); ++i)
        for (std::size_t j = i + 1; j < kSimdFeatureFlagCatalog.size(); ++j)
            if (kSimdFeatureFlagCatalog[i].cpuinfo == kSimdFeatureFlagCatalog[j].cpuinfo) return false;
    return true;
}

static_assert(kSimdFeatureFlagCatalog.size() == 23);
static_assert(catalog_entries_nonempty());
static_assert(catalog_ids_unique());
// Unterstrich-Falle zementiert (aeltere Subsets OHNE, neuere MIT Unterstrich -- realer Kernel-String):
static_assert(kAvx512Vbmi.cpuinfo == std::string_view{"avx512vbmi"});    // OHNE Unterstrich
static_assert(kAvx512Vbmi2.cpuinfo == std::string_view{"avx512_vbmi2"}); // MIT Unterstrich
static_assert(kAvx512Vl.cpuinfo == std::string_view{"avx512vl"});
static_assert(kAvx512Vpopcntdq.cpuinfo == std::string_view{"avx512_vpopcntdq"});
// cpuinfo-Id != g++-Flag (getrennt gefuehrt, nie heuristisch abgeleitet):
static_assert(kAvx512Vbmi2.gpp == std::string_view{"-mavx512vbmi2"});
static_assert(gpp_flag_for("avx512_vp2intersect") == std::string_view{"-mavx512vp2intersect"});
static_assert(gpp_flag_for("nicht_existent").empty());
static_assert(is_known_simd_flag("avx512_vp2intersect") && !is_known_simd_flag("avx512_phantom"));
static_assert(count_flags_of_tier(SimdFlagTier::Avx512) == 14); // 14 AVX-512-Subsets im Vokabular (inkl. fp16)
static_assert(tier_label(SimdFlagTier::Avx512) == std::string_view{"avx512"});
static_assert(tier_label(SimdFlagTier::Companion) == std::string_view{"companion"});

} // namespace comdare::cache_engine::measurement
