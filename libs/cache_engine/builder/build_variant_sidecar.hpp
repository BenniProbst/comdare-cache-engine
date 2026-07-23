#pragma once
// G2-3 (Lager-Gate, Stempel-Paket A7) -- deterministischer Serializer/Parser der Build-Varianten-Signatur, die als
// drittes per-Binary-Sidecar `<output>.variant` neben `.version` (System-Provenienz) und `.algos` (Organ-Provenienz)
// steht. Trennungs-Doktrin (build_orchestrator.hpp): Build-Variante != System-Provenienz != Organ. Die SOLL-Signatur
// wird aus dem ABI-stabilen BuildVariantDefinitionV1-POD (A6, v2 inkl. simd_avx10_version) komponiert -- KEINE zweite
// Feldquelle: exakt dieselben Literale, die build_variant_definition<PT,SE,HW>() aus den 3 Build-Achsen ableitet.
//
// EIN Serializer, mehrere Nutzer: die CEB komponiert damit die erwartete Zell-/ISA-Signatur (perm_compile kennt die
// Zelle), speist sie in BuildConfig.build_variant_sig; der Orchestrator vergleicht sie beim Skip-Check (dll_is_current)
// und schreibt sie bei Erfolg ins Sidecar. Der Lager-Index (Lane B / G3) verwendet DENSELBEN compose, keine Parallel-
// Ableitung (Integrations-Doktrin: eine Feldquelle fuer Varianten-Identitaet).

#include "anatomy/build_variant_definition.hpp" // BuildVariantDefinitionV1 + kBuildVariantDefinitionVersion

#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace comdare::cache_engine::builder::experiment {

// Geparste Sicht der `.variant`-Signatur: der Versions-Stempel (bv) plus die 7 serialisierten Definitions-Felder.
// page_is_branch/page_is_leaf sind bewusst NICHT Teil der Signatur (aus page_kind ableitbar -> keine Redundanz).
struct ParsedVariantSignature {
    std::uint64_t bv                 = 0; // Definitions-POD-Version (kBuildVariantDefinitionVersion); Bruch = Neubau
    std::uint64_t page_kind          = 0;
    std::uint64_t simd_width_bits    = 0;
    std::uint64_t simd_avx512        = 0;
    std::uint64_t simd_avx10_version = 0;
    std::uint64_t hw_cache_line      = 0;
    std::uint64_t hw_numa_capable    = 0;
    std::uint64_t present_mask       = 0;
    bool          operator==(ParsedVariantSignature const&) const = default;
};

// Deterministische Serialisierung: `bv=<ver>;page_kind=..;simd_width_bits=..;simd_avx512=..;simd_avx10_version=..;
// hw_cache_line=..;hw_numa_capable=..;present_mask=..`. Feste Feld-Reihenfolge, keine Locale-Abhaengigkeit (to_string
// auf uint64 ist ziffern-stabil) -> reine String-Gleichheit als Skip-Kriterium (identische Semantik wie `.algos`).
[[nodiscard]] inline std::string compose_variant_signature(anatomy::BuildVariantDefinitionV1 const& v) {
    std::string s;
    s.reserve(128);
    s += "bv=";
    s += std::to_string(static_cast<std::uint64_t>(anatomy::kBuildVariantDefinitionVersion));
    s += ";page_kind=";
    s += std::to_string(v.page_kind);
    s += ";simd_width_bits=";
    s += std::to_string(v.simd_width_bits);
    s += ";simd_avx512=";
    s += std::to_string(v.simd_avx512);
    s += ";simd_avx10_version=";
    s += std::to_string(v.simd_avx10_version);
    s += ";hw_cache_line=";
    s += std::to_string(v.hw_cache_line);
    s += ";hw_numa_capable=";
    s += std::to_string(v.hw_numa_capable);
    s += ";present_mask=";
    s += std::to_string(v.present_mask);
    return s;
}

// Umkehrung: parst die serialisierte Form zurueck. Streng -- jedes Token MUSS `key=<uint64>` sein, ein unbekannter
// Schluessel oder eine nicht vollstaendig numerische Zahl liefert nullopt (Format-Verletzung, kein Rateverhalten).
// Zweck: Roundtrip-Determinismus-Nachweis; der Skip-Check selbst vergleicht rohe Strings, nicht geparste Felder.
[[nodiscard]] inline std::optional<ParsedVariantSignature> parse_variant_signature(std::string_view sig) {
    if (sig.empty()) return std::nullopt;
    ParsedVariantSignature out;
    std::size_t            pos = 0;
    while (pos <= sig.size()) {
        std::size_t const      semi = sig.find(';', pos);
        std::string_view const tok =
            sig.substr(pos, (semi == std::string_view::npos) ? std::string_view::npos : semi - pos);
        std::size_t const eq = tok.find('=');
        if (eq == std::string_view::npos) return std::nullopt;
        std::string_view const key = tok.substr(0, eq);
        std::string_view const val = tok.substr(eq + 1);
        std::uint64_t          num = 0;
        auto const [ptr, ec]       = std::from_chars(val.data(), val.data() + val.size(), num);
        if (ec != std::errc{} || ptr != val.data() + val.size()) return std::nullopt;
        if (key == "bv")
            out.bv = num;
        else if (key == "page_kind")
            out.page_kind = num;
        else if (key == "simd_width_bits")
            out.simd_width_bits = num;
        else if (key == "simd_avx512")
            out.simd_avx512 = num;
        else if (key == "simd_avx10_version")
            out.simd_avx10_version = num;
        else if (key == "hw_cache_line")
            out.hw_cache_line = num;
        else if (key == "hw_numa_capable")
            out.hw_numa_capable = num;
        else if (key == "present_mask")
            out.present_mask = num;
        else
            return std::nullopt; // unbekannter Schluessel -> Format-Verletzung
        if (semi == std::string_view::npos) break;
        pos = semi + 1;
    }
    return out;
}

} // namespace comdare::cache_engine::builder::experiment
