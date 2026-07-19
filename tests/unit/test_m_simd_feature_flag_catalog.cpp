// Querschnitt M -- Section 40.a-E1: feingranularer SIMD-Flag-Katalog + deklarierte Maschinen-Signaturen.
// Verifiziert das CODE=WAHRHEIT-Vokabular (simd_feature_flag.hpp) + die 3 Cluster-Signaturen
// (machine_simd_signature.hpp) gegen das live-verifizierte Referenzdoc. Header-only, kein DLL-/Umbrella-Bezug.

#include <cache_engine/measurement/machine_simd_signature.hpp>
#include <cache_engine/measurement/simd_feature_flag.hpp>
#include <cache_engine/measurement/simd_organ_sensibility.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <span>
#include <string_view>

namespace m = ::comdare::cache_engine::measurement;

namespace {

[[nodiscard]] std::size_t count_avx512(std::span<m::SimdFeatureFlag const> sig) {
    std::size_t n = 0;
    for (auto const& f : sig)
        if (f.tier == m::SimdFlagTier::Avx512) ++n;
    return n;
}

} // namespace

TEST(MSimdFeatureFlagCatalog, VocabularyWellFormedAndUnderscoreExact) {
    EXPECT_EQ(m::kSimdFeatureFlagCatalog.size(), 23u);
    EXPECT_EQ(m::count_flags_of_tier(m::SimdFlagTier::Avx512), 14u); // inkl. fp16 (Vokabular)
    EXPECT_TRUE(m::catalog_ids_unique());
    EXPECT_TRUE(m::catalog_entries_nonempty());
    // Unterstrich-Falle: aeltere Subsets OHNE, neuere MIT Unterstrich (realer Kernel-cpuinfo-String).
    EXPECT_EQ(m::kAvx512Vbmi.cpuinfo, std::string_view{"avx512vbmi"});
    EXPECT_EQ(m::kAvx512Vbmi2.cpuinfo, std::string_view{"avx512_vbmi2"});
    EXPECT_EQ(m::kAvx512Vl.cpuinfo, std::string_view{"avx512vl"});
    EXPECT_EQ(m::kAvx512Vpopcntdq.cpuinfo, std::string_view{"avx512_vpopcntdq"});
    // cpuinfo-Id und g++-Flag getrennt gefuehrt (nie heuristisch abgeleitet):
    EXPECT_EQ(m::kAvx512Vbmi2.gpp, std::string_view{"-mavx512vbmi2"});
    EXPECT_EQ(m::gpp_flag_for("avx512_vp2intersect"), std::string_view{"-mavx512vp2intersect"});
    EXPECT_TRUE(m::gpp_flag_for("nicht_existent").empty());
    EXPECT_TRUE(m::is_known_simd_flag("avx512_vp2intersect"));
    EXPECT_FALSE(m::is_known_simd_flag("avx512_phantom"));
}

TEST(MSimdFeatureFlagCatalog, Prod1Zen5SignatureMatchesLiveVerifiedReferenceDoc) {
    // prod1 (Zen 5): 13 avx512-Flags, KEIN fp16, VP2INTERSECT als Zen5-Neuzugang (live 100%-cpuinfo-Abgleich).
    EXPECT_EQ(count_avx512(m::Prod1Zen5Signature::signature()), 13u);
    EXPECT_TRUE(m::Prod1Zen5Signature::has_flag("avx512_vp2intersect"));
    EXPECT_FALSE(m::Prod1Zen5Signature::has_flag("avx512_fp16"));
    EXPECT_TRUE(m::Prod1Zen5Signature::has_flag("avx512_vbmi2"));
    EXPECT_TRUE(m::Prod1Zen5Signature::has_flag("gfni"));
    EXPECT_EQ(m::Prod1Zen5Signature::machine_id(), std::string_view{"prod1_zen5"});
    EXPECT_EQ(m::Prod1Zen5Signature::host_isa(), std::string_view{"x86_64"});
}

TEST(MSimdFeatureFlagCatalog, Avx512FusedOffMachinesKeep256AndCompanionsButNoAvx512) {
    // prod2 (Raptor Lake) + Odroid (Gracemont): AVX-512 fused-off/absent, 256-bit + Begleiter bleiben.
    EXPECT_EQ(count_avx512(m::Prod2RaptorLakeSignature::signature()), 0u);
    EXPECT_EQ(count_avx512(m::OdroidGracemontSignature::signature()), 0u);
    EXPECT_TRUE(m::Prod2RaptorLakeSignature::has_flag("avx_vnni"));
    EXPECT_FALSE(m::Prod2RaptorLakeSignature::has_flag("avx512f"));
    EXPECT_TRUE(m::OdroidGracemontSignature::has_flag("avx2"));
    EXPECT_FALSE(m::OdroidGracemontSignature::has_flag("avx512f"));
    EXPECT_TRUE(m::signature_within_catalog(m::Prod1Zen5Signature::signature()));
}

TEST(MSimdFeatureFlagCatalog, OrganSensibilityMatrixMatchesReferenceDocSection5) {
    EXPECT_EQ(m::kSimdOrganSensibility.size(), 9u);
    EXPECT_TRUE(m::organ_sensibility_within_catalog());
    // solide Zuordnungen (Referenzdoc Abschnitt 5):
    EXPECT_TRUE(m::is_flag_meaningful_for("filter", "avx512_vpopcntdq"));
    EXPECT_TRUE(m::is_flag_meaningful_for("search_algo", "avx512bw"));
    EXPECT_TRUE(m::is_flag_meaningful_for("mapping", "gfni"));
    EXPECT_TRUE(m::is_flag_meaningful_for("index_organization", "bmi2"));
    // prefetch: bewusst KEIN SIMD-Flag (leere Menge, ehrlich modelliert):
    EXPECT_TRUE(m::sensibility_of("prefetch").empty());
    // ein Flag, das NICHT zur Klasse gehoert, ist nicht sinnvoll:
    EXPECT_FALSE(m::is_flag_meaningful_for("filter", "avx512_bf16"));
    EXPECT_FALSE(m::is_flag_meaningful_for("nicht_existente_klasse", "avx2"));
}
