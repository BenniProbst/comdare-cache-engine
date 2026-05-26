// V41.F.6.1.P1 Phase B — Paper-Legacy-Code Pattern-Validierung
//
// **COMPILE-TIME ONLY** [[compile-time-only-no-runtime]]:
//   Alle Tests via static_assert. KEINE Runtime-SHA-Berechnung, KEINE
//   Runtime-Validation. is_original_<fn>() ist hardcoded constexpr bool literal
//   (simuliert was apps/is_original_validator Pre-Build-Tool generieren wuerde).
//
// Was getestet wird:
//   (1) ctsha consteval SHA256 funktioniert (static_assert)
//   (2) DemoPaperOriginal Sub-Concept-Conformance (static_assert)
//   (3) Negativ-Demo: Mismatch wird vom Pattern erkannt (static_assert)
//   (4) PseudocodeReImpl-Pattern (alle false hart, [[pseudocode-papers-fallback]])

#include <gtest/gtest.h>

#include <sha256/ctsha.hpp>
#include <sha256/comdare_is_original_macro.hpp>
#include <concepts/legacy_original_code_strategy_concept.hpp>

#include <string_view>
#include <type_traits>

namespace ctsha = ::comdare::cache_engine::sha256;
namespace ce_concepts = ::comdare::cache_engine::concepts;

// =================================================================
// (1) ctsha consteval SHA256 — REIN COMPILE-TIME
// =================================================================

TEST(CtSha256, EmptyStringHashCompileTime) {
    constexpr auto digest = ctsha::sha256("");
    constexpr auto expected = ctsha::from_hex(
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    static_assert(digest == expected, "ctsha: SHA256(\"\") mismatch");
    SUCCEED();
}

TEST(CtSha256, AbcHashCompileTime) {
    constexpr auto digest = ctsha::sha256("abc");
    constexpr auto expected = ctsha::from_hex(
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    static_assert(digest == expected, "ctsha: SHA256(\"abc\") mismatch");
    SUCCEED();
}

TEST(CtSha256, LongerStringHashCompileTime) {
    constexpr auto digest = ctsha::sha256("The quick brown fox jumps over the lazy dog");
    constexpr auto expected = ctsha::from_hex(
        "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592");
    static_assert(digest == expected, "ctsha: SHA256(quick brown fox) mismatch");
    SUCCEED();
}

TEST(CtSha256, CompileTimeBudgetCheck) {
    static_assert(ctsha::fits_compile_time_budget<1>());
    static_assert(ctsha::fits_compile_time_budget<50 * 1024>());
    static_assert(!ctsha::fits_compile_time_budget<50 * 1024 + 1>());
    SUCCEED();
}

TEST(CtSha256, FromHexRoundtripCompileTime) {
    constexpr auto digest_from_hex = ctsha::from_hex(
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    constexpr auto digest_computed = ctsha::sha256("");
    static_assert(digest_from_hex == digest_computed,
                  "ctsha: from_hex / sha256(\"\") roundtrip mismatch");
    SUCCEED();
}

// =================================================================
// (2) DemoPaperOriginal — Pattern-Demonstration
// =================================================================
//
// Simuliert was apps/is_original_validator als Pre-Build-Tool generieren wuerde:
//   ${BUILD}/generated/.../demo_paper_is_original.hpp:
//     namespace generated {
//       inline constexpr bool kIsOriginal_allocate   = true;
//       inline constexpr bool kIsOriginal_deallocate = true;
//     }
//
// Im Pilot inline simuliert. Phase B.2: tatsaechliche Tool-Integration.

namespace simulated_generated_header {
inline constexpr bool kIsOriginal_allocate   = true;
inline constexpr bool kIsOriginal_deallocate = true;
}  // namespace simulated_generated_header

class DemoPaperOriginal {
public:
    // Pflicht-API LegacyOriginalCodePflicht:
    static constexpr std::string_view experiment_compiler() noexcept { return "demo-cc-9.5"; }
    static constexpr bool has_original_paper_code() noexcept { return true; }

    // Properties aus generiertem Header (ZERO Runtime-Cost — direkter literal):
    static constexpr bool is_original_allocate() noexcept {
        return simulated_generated_header::kIsOriginal_allocate;
    }
    static constexpr bool is_original_deallocate() noexcept {
        return simulated_generated_header::kIsOriginal_deallocate;
    }
    static constexpr bool is_original_module() noexcept {
        return is_original_allocate() && is_original_deallocate();
    }
};

TEST(DemoPaperOriginal, AllOriginalCompileTime) {
    static_assert(DemoPaperOriginal::is_original_allocate());
    static_assert(DemoPaperOriginal::is_original_deallocate());
    static_assert(DemoPaperOriginal::is_original_module());
    SUCCEED();
}

TEST(DemoPaperOriginal, SubConceptConformanceCompileTime) {
    static_assert(ce_concepts::LegacyOriginalCodePflicht<DemoPaperOriginal>);
    static_assert(ce_concepts::HasOriginalCode<DemoPaperOriginal>);
    static_assert(ce_concepts::PaperOriginalValidated<DemoPaperOriginal>);
    static_assert(ce_concepts::is_original_v<DemoPaperOriginal>);
    SUCCEED();
}

// =================================================================
// (3) Negativ-Demo — Mismatch wird erkannt
// =================================================================
//
// Simuliert: Pre-Build-Tool hat fuer "deallocate" einen SHA-Mismatch erkannt
// (z.B. weil legacy_code/<paper>/dealloc.c modifiziert wurde). Tool generiert:
//   inline constexpr bool kIsOriginal_deallocate = false;

namespace simulated_generated_header_with_mismatch {
inline constexpr bool kIsOriginal_allocate   = true;
inline constexpr bool kIsOriginal_deallocate = false;  // SHA-Mismatch detected
}  // namespace

class DemoPaperWithMismatch {
public:
    static constexpr std::string_view experiment_compiler() noexcept { return "demo-cc-9.5"; }
    static constexpr bool has_original_paper_code() noexcept { return true; }

    static constexpr bool is_original_allocate() noexcept {
        return simulated_generated_header_with_mismatch::kIsOriginal_allocate;
    }
    static constexpr bool is_original_deallocate() noexcept {
        return simulated_generated_header_with_mismatch::kIsOriginal_deallocate;
    }
    static constexpr bool is_original_module() noexcept {
        return is_original_allocate() && is_original_deallocate();
    }
};

TEST(DemoPaperWithMismatch, ModuleAggregateFalseCompileTime) {
    static_assert(DemoPaperWithMismatch::is_original_allocate());
    static_assert(!DemoPaperWithMismatch::is_original_deallocate());
    static_assert(!DemoPaperWithMismatch::is_original_module(),
                  "Modul-Aggregat MUSS false sein wenn EINE Function nicht original ist");
    SUCCEED();
}

TEST(DemoPaperWithMismatch, NotPaperOriginalValidatedCompileTime) {
    static_assert(ce_concepts::LegacyOriginalCodePflicht<DemoPaperWithMismatch>);
    static_assert(ce_concepts::HasOriginalCode<DemoPaperWithMismatch>);  // hat Source-Code
    static_assert(!ce_concepts::PaperOriginalValidated<DemoPaperWithMismatch>);  // aber NICHT validiert
    static_assert(!ce_concepts::is_original_v<DemoPaperWithMismatch>);
    SUCCEED();
}

// =================================================================
// (4) Pseudocode-Re-Implementation — has_original_paper_code = false
// =================================================================

class PseudocodeReImpl {
public:
    static constexpr std::string_view experiment_compiler() noexcept { return "self"; }
    static constexpr bool has_original_paper_code() noexcept { return false; }

    // Re-Impl-Marker fuer Functions ohne Original-Source (Pseudocode/Non-C-Paper):
    COMDARE_IS_ORIGINAL_NOT_APPLICABLE(buddy_split)
    COMDARE_IS_ORIGINAL_NOT_APPLICABLE(buddy_merge)

    static constexpr bool is_original_module() noexcept {
        return is_original_buddy_split() && is_original_buddy_merge();
    }
};

TEST(PseudocodeReImpl, AllNotApplicableCompileTime) {
    static_assert(!PseudocodeReImpl::is_original_buddy_split());
    static_assert(!PseudocodeReImpl::is_original_buddy_merge());
    static_assert(!PseudocodeReImpl::is_original_module());
    SUCCEED();
}

TEST(PseudocodeReImpl, LegacyPflichtConformsCompileTime) {
    static_assert(ce_concepts::LegacyOriginalCodePflicht<PseudocodeReImpl>);
    static_assert(!ce_concepts::HasOriginalCode<PseudocodeReImpl>);
    static_assert(!ce_concepts::PaperOriginalValidated<PseudocodeReImpl>);
    static_assert(!ce_concepts::is_original_v<PseudocodeReImpl>);
    SUCCEED();
}

// =================================================================
// (5) Negativ-Demo — Wrapper ohne Pflicht-API erfuellt Concept NICHT
// =================================================================

class NoLegacyPaperWrapper {};

TEST(LegacyOriginalCodeSubConcept, EmptyClassNotConformsCompileTime) {
    static_assert(!ce_concepts::LegacyOriginalCodePflicht<NoLegacyPaperWrapper>);
    SUCCEED();
}
