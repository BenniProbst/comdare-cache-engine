// G2-3 (Lager-Gate, Stempel-Paket A7) -- Tests fuer das dritte per-Binary-Sidecar `.variant`:
//   (1) compose_variant_signature: deterministisch + exakte Form (bv aus kBuildVariantDefinitionVersion, A6=2);
//   (2) parse_variant_signature: Roundtrip + strenge Ablehnung malformierter Formen;
//   (3) dll_is_current 3x3-Skip-Matrix: variant_sig {leer, gesetzt} x .variant {fehlt, mismatch, match};
//   (4) Byte-Neutralitaet: variant_sig leer => Variant-Gate AUS (identisch zum Alt-Verhalten, .variant ignoriert);
//   (5) write_variant_sidecar: leer = no-op, sonst byte-exaktes Schreiben.

#include "anatomy/build_variant_definition.hpp" // BuildVariantDefinitionV1 + kBuildVariantDefinitionVersion
#include "builder/build_orchestrator/build_orchestrator.hpp" // variant_sidecar_path / dll_is_current / write_*_sidecar
#include "builder/build_variant_sidecar.hpp"                 // compose_variant_signature / parse_variant_signature
#include "comdare_test_tmp.hpp"                              // #278/#24: per-User-Temp gegen CI-Kollisionen

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>

#include <gtest/gtest.h>

namespace ex = comdare::cache_engine::builder::experiment;
namespace an = comdare::cache_engine::anatomy;
namespace fs = std::filesystem;

namespace {

void write_file(fs::path const& p, std::string const& content) {
    std::ofstream f{p, std::ios::binary | std::ios::trunc};
    f << content;
}

std::string read_file(fs::path const& p) {
    std::ifstream f{p, std::ios::binary};
    return std::string{std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>{}};
}

// Deterministische Beispiel-Definition (BPlus-Knoten, AVX-512 + konvergiertes AVX10.2, NUMA-faehig).
an::BuildVariantDefinitionV1 sample_def_avx10() {
    an::BuildVariantDefinitionV1 v;
    v.page_kind          = 5; // BPlus
    v.page_is_branch     = 1;
    v.page_is_leaf       = 0;
    v.simd_width_bits    = 512;
    v.simd_avx512        = 1;
    v.hw_cache_line      = 64;
    v.hw_numa_capable    = 1;
    v.present_mask       = 7;
    v.simd_avx10_version = 2; // A6-Feld (G2-2)
    return v;
}

// Andere Zelle (DenseByte, 128-bit, kein AVX512/AVX10) -- fuer Mismatch-Faelle.
an::BuildVariantDefinitionV1 sample_def_legacy() {
    an::BuildVariantDefinitionV1 v;
    v.page_kind          = 0; // DenseByte
    v.page_is_branch     = 0;
    v.page_is_leaf       = 1;
    v.simd_width_bits    = 128;
    v.simd_avx512        = 0;
    v.hw_cache_line      = 64;
    v.hw_numa_capable    = 0;
    v.present_mask       = 7;
    v.simd_avx10_version = 0;
    return v;
}

} // namespace

// (1) Serializer: exakte, feste Form + Determinismus. bv spiegelt die POD-Version (A6 => 2).
TEST(G2VariantSidecar, ComposeIsDeterministicAndExact) {
    auto const        v      = sample_def_avx10();
    std::string const sig    = ex::compose_variant_signature(v);
    std::string const expect = "bv=2;page_kind=5;simd_width_bits=512;simd_avx512=1;simd_avx10_version=2;"
                               "hw_cache_line=64;hw_numa_capable=1;present_mask=7";
    EXPECT_EQ(sig, expect);
    EXPECT_EQ(ex::compose_variant_signature(v), sig); // zweite Komposition identisch
    EXPECT_EQ(static_cast<std::uint64_t>(an::kBuildVariantDefinitionVersion), 2u);
}

// (2a) Parser: Roundtrip compose -> parse rekonstruiert alle 7 Felder + bv.
TEST(G2VariantSidecar, ParseRoundtripsCompose) {
    auto const v      = sample_def_avx10();
    auto const parsed = ex::parse_variant_signature(ex::compose_variant_signature(v));
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->bv, static_cast<std::uint64_t>(an::kBuildVariantDefinitionVersion));
    EXPECT_EQ(parsed->page_kind, v.page_kind);
    EXPECT_EQ(parsed->simd_width_bits, v.simd_width_bits);
    EXPECT_EQ(parsed->simd_avx512, v.simd_avx512);
    EXPECT_EQ(parsed->simd_avx10_version, v.simd_avx10_version);
    EXPECT_EQ(parsed->hw_cache_line, v.hw_cache_line);
    EXPECT_EQ(parsed->hw_numa_capable, v.hw_numa_capable);
    EXPECT_EQ(parsed->present_mask, v.present_mask);
}

// (2b) Parser: strenge Ablehnung malformierter Formen (kein Rateverhalten).
TEST(G2VariantSidecar, ParseRejectsMalformed) {
    EXPECT_FALSE(ex::parse_variant_signature("").has_value());                  // leer
    EXPECT_FALSE(ex::parse_variant_signature("bv=2;page_kind").has_value());    // Token ohne '='
    EXPECT_FALSE(ex::parse_variant_signature("bv=2;page_kind=x").has_value());  // nicht numerisch
    EXPECT_FALSE(ex::parse_variant_signature("bv=2;unknown=1").has_value());    // unbekannter Schluessel
    EXPECT_FALSE(ex::parse_variant_signature("bv=2;page_kind=5;").has_value()); // leeres Trailing-Token
}

// (3)+(4) 3x3-Skip-Matrix: version/algo konstant gehalten, damit die Variant-Achse allein entscheidet.
TEST(G2VariantSidecar, DllIsCurrentVariantMatrix) {
    fs::path const  base = comdare::test::user_tmp_dir() / "g2_variant_matrix";
    std::error_code ec;
    fs::remove_all(base, ec);
    fs::create_directories(base, ec);

    fs::path const    out      = base / "perm_x.dll";
    std::string const kVersion = "sysv1";
    std::string const kSigA    = ex::compose_variant_signature(sample_def_avx10());
    std::string const kSigB    = ex::compose_variant_signature(sample_def_legacy());
    ASSERT_NE(kSigA, kSigB);

    write_file(out, "DLL");                   // DLL existiert
    ex::write_version_sidecar(out, kVersion); // .version passt (System-Provenienz aktuell)

    auto set_variant = [&](std::optional<std::string> const& content) {
        fs::remove(ex::variant_sidecar_path(out), ec);
        if (content) write_file(ex::variant_sidecar_path(out), *content);
    };

    // variant_sig LEER => Variant-Gate AUS => IMMER current (byte-neutral), unabhaengig vom .variant-Zustand.
    set_variant(std::nullopt);
    EXPECT_TRUE(ex::dll_is_current(out, kVersion, "", "")); // fehlt
    set_variant(kSigB);
    EXPECT_TRUE(ex::dll_is_current(out, kVersion, "", "")); // stale sidecar ignoriert
    set_variant(kSigA);
    EXPECT_TRUE(ex::dll_is_current(out, kVersion, "", "")); // match ebenfalls current

    // variant_sig GESETZT (kSigA): fehlt/mismatch => Neubau, match => Skip.
    set_variant(std::nullopt);
    EXPECT_FALSE(ex::dll_is_current(out, kVersion, "", kSigA)); // .variant fehlt
    set_variant(kSigB);
    EXPECT_FALSE(ex::dll_is_current(out, kVersion, "", kSigA)); // .variant mismatch
    set_variant(kSigA);
    EXPECT_TRUE(ex::dll_is_current(out, kVersion, "", kSigA)); // .variant match

    fs::remove_all(base, ec);
}

// (4b) Byte-Neutralitaet auch fuer die 3-arg-Form: das neue Gate hat keinerlei Wirkung ohne variant_sig.
TEST(G2VariantSidecar, ThreeArgFormUnaffectedByVariantSidecar) {
    fs::path const  base = comdare::test::user_tmp_dir() / "g2_variant_3arg";
    std::error_code ec;
    fs::remove_all(base, ec);
    fs::create_directories(base, ec);

    fs::path const    out      = base / "perm_z.dll";
    std::string const kVersion = "sysv1";
    write_file(out, "DLL");
    ex::write_version_sidecar(out, kVersion);
    write_file(ex::variant_sidecar_path(out), "irgendein-stale-variant"); // stale .variant vorhanden

    // 3-arg-Aufruf (kein variant_sig) verhaelt sich exakt wie vor A7: nur Version zaehlt -> current.
    EXPECT_TRUE(ex::dll_is_current(out, kVersion, ""));
    EXPECT_TRUE(ex::dll_is_current(out, kVersion));

    fs::remove_all(base, ec);
}

// (5) Writer: leer = no-op (kein Sidecar), sonst byte-exaktes Schreiben.
TEST(G2VariantSidecar, WriteVariantSidecarEmptyIsNoOpElseExact) {
    fs::path const  base = comdare::test::user_tmp_dir() / "g2_variant_writer";
    std::error_code ec;
    fs::remove_all(base, ec);
    fs::create_directories(base, ec);

    fs::path const out = base / "perm_y.dll";

    ex::write_variant_sidecar(out, ""); // leer -> no-op
    EXPECT_FALSE(fs::exists(ex::variant_sidecar_path(out)));

    std::string const sig = ex::compose_variant_signature(sample_def_avx10());
    ex::write_variant_sidecar(out, sig); // schreibt exakt
    ASSERT_TRUE(fs::exists(ex::variant_sidecar_path(out)));
    EXPECT_EQ(read_file(ex::variant_sidecar_path(out)), sig);

    fs::remove_all(base, ec);
}
