// V41.F.6.1.P2.A0.6 Paper-Legacy-Code Pattern (Achsen-generischer Mixin + Vererbung)
//
// **COMPILE-TIME ONLY** [[compile-time-only-no-runtime]]:
//   Alle Tests via static_assert. KEINE Runtime-SHA-Berechnung.
//
// **Achsen-generischer Mixin-Template** (User-Direktive vor Phase B.2.A0.6):
//   - OriginalCodeMixinBase<PaperManifest> cross-topic (libs/.../src/concepts/)
//   - Pro Achse: TestAllocatorOriginalCodeMixin<PaperManifest> erweitert um
//     Achsen-spezifische is_original_<fn>() Methoden
//   - Tool generiert pro Paper nur: per-fn Bools + PaperManifest + using alias
//   - Wrapper erbt vom Alias → NULL manuelle Registrierung

#include <gtest/gtest.h>

#include <sha256/ctsha.hpp>
#include <concepts/legacy_original_code_strategy_concept.hpp>
#include <concepts/axis_original_code_mixin_base.hpp>
#include <topics/axis_base.hpp>

#include <string_view>
#include <type_traits>

namespace ctsha = ::comdare::cache_engine::sha256;
namespace ce_concepts = ::comdare::cache_engine::concepts;
namespace ce_topics = ::comdare::cache_engine::topics;

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
    static_assert(digest == expected);
    SUCCEED();
}

TEST(CtSha256, CompileTimeBudgetCheck) {
    static_assert(ctsha::fits_compile_time_budget<1>());
    static_assert(ctsha::fits_compile_time_budget<50 * 1024>());
    static_assert(!ctsha::fits_compile_time_budget<50 * 1024 + 1>());
    SUCCEED();
}

// =================================================================
// (2) Achsen-generischer Mixin-Template — Demo: TestAllocator-Achse
// =================================================================
//
// Simuliert was libs/.../topics/allocator/axis_06_allocator/concepts/
// axis_06_allocator_original_code_mixin.hpp definieren wuerde.
// Pro Achse genau EINMAL — alle Vendor-Wrapper erben davon.

namespace test_allocator_axis {

/// @brief Demo-Achs-Mixin (analog AllocatorOriginalCodeMixin)
///
/// Erbt von cross-topic OriginalCodeMixinBase (Compiler + has_original_paper_code)
/// und erweitert um Achsen-spezifische is_original_<fn>() pro Pflicht-Interface
/// (allocate/deallocate/reallocate).
template <typename PaperManifest>
struct TestAllocatorOriginalCodeMixin
    : public ce_concepts::OriginalCodeMixinBase<PaperManifest> {

    [[nodiscard]] static constexpr bool is_original_allocate() noexcept {
        return PaperManifest::kIsOriginal_allocate;
    }
    [[nodiscard]] static constexpr bool is_original_deallocate() noexcept {
        return PaperManifest::kIsOriginal_deallocate;
    }
    [[nodiscard]] static constexpr bool is_original_reallocate() noexcept {
        return PaperManifest::kIsOriginal_reallocate;
    }
    [[nodiscard]] static constexpr bool is_original_module() noexcept {
        return is_original_allocate() && is_original_deallocate() && is_original_reallocate();
    }
};

}  // namespace test_allocator_axis

// =================================================================
// (3) Simulated Tool-Generated Header — Paper 1: ALL ORIGINAL
// =================================================================
//
// Simuliert was apps/is_original_validator als Pre-Build-Tool generieren
// wuerde. Pro Paper EIN Header mit per-fn Bools + PaperManifest + Alias.

namespace simulated_generated::paper_a04_mimalloc {

// Per-function Bools (vom Tool aus SHA-Validierung gesetzt):
inline constexpr bool kIsOriginal_allocate   = true;
inline constexpr bool kIsOriginal_deallocate = true;
inline constexpr bool kIsOriginal_reallocate = true;

// Achsen-Manifest-Struct (Pflicht-Constants fuer Mixin-Template):
struct PaperManifest {
    static constexpr std::string_view kCompiler = "gcc-9.5";
    static constexpr bool kHasOriginalPaperCode = true;
    static constexpr bool kIsOriginal_allocate   = simulated_generated::paper_a04_mimalloc::kIsOriginal_allocate;
    static constexpr bool kIsOriginal_deallocate = simulated_generated::paper_a04_mimalloc::kIsOriginal_deallocate;
    static constexpr bool kIsOriginal_reallocate = simulated_generated::paper_a04_mimalloc::kIsOriginal_reallocate;
};

// Convenience-Alias: Wrapper erbt davon
using OriginalCodeMixin = ::test_allocator_axis::TestAllocatorOriginalCodeMixin<PaperManifest>;

}  // namespace simulated_generated::paper_a04_mimalloc

// =================================================================
// (4) Simulated Tool-Generated Header — Paper 2: MODIFIED
// =================================================================

namespace simulated_generated::paper_a07_snmalloc {

inline constexpr bool kIsOriginal_allocate   = true;
inline constexpr bool kIsOriginal_deallocate = false;  // ← Mismatch: source wurde modifiziert
inline constexpr bool kIsOriginal_reallocate = true;

struct PaperManifest {
    static constexpr std::string_view kCompiler = "clang-12";
    static constexpr bool kHasOriginalPaperCode = true;
    static constexpr bool kIsOriginal_allocate   = simulated_generated::paper_a07_snmalloc::kIsOriginal_allocate;
    static constexpr bool kIsOriginal_deallocate = simulated_generated::paper_a07_snmalloc::kIsOriginal_deallocate;
    static constexpr bool kIsOriginal_reallocate = simulated_generated::paper_a07_snmalloc::kIsOriginal_reallocate;
};

using OriginalCodeMixin = ::test_allocator_axis::TestAllocatorOriginalCodeMixin<PaperManifest>;

}  // namespace

// =================================================================
// (5) Wrapper-Klassen erben vom generierten Alias (NULL manuelle Macros)
// =================================================================

class DemoMimallocAllocator : public simulated_generated::paper_a04_mimalloc::OriginalCodeMixin {
public:
    // Wrapper hat NUR die Algorithmus-Methoden — Properties kommen via Mixin
    void* allocate(std::size_t bytes) { return ::malloc(bytes); }
    void  deallocate(void* p) noexcept { ::free(p); }
};

class DemoSnmallocAllocator : public simulated_generated::paper_a07_snmalloc::OriginalCodeMixin {
public:
    void* allocate(std::size_t bytes) { return ::malloc(bytes); }
    void  deallocate(void* p) noexcept { ::free(p); }
};

// =================================================================
// (6) Tests — alle Compile-Time via static_assert
// =================================================================

TEST(AchsenMixin, MimallocAllProperties) {
    static_assert(DemoMimallocAllocator::get_compiler() == "gcc-9.5");
    static_assert(DemoMimallocAllocator::has_original_paper_code());
    static_assert(DemoMimallocAllocator::is_original_allocate());
    static_assert(DemoMimallocAllocator::is_original_deallocate());
    static_assert(DemoMimallocAllocator::is_original_reallocate());
    static_assert(DemoMimallocAllocator::is_original_module());
    SUCCEED();
}

TEST(AchsenMixin, SnmallocMismatchDetected) {
    static_assert(DemoSnmallocAllocator::get_compiler() == "clang-12");
    static_assert(DemoSnmallocAllocator::has_original_paper_code());
    static_assert(DemoSnmallocAllocator::is_original_allocate());
    static_assert(!DemoSnmallocAllocator::is_original_deallocate());  // ← Mismatch!
    static_assert(DemoSnmallocAllocator::is_original_reallocate());
    static_assert(!DemoSnmallocAllocator::is_original_module(),
                  "Modul-Aggregat MUSS false sein wenn EINE Function nicht original ist");
    SUCCEED();
}

TEST(AchsenMixin, LegacyPflichtConformsBothWrappers) {
    static_assert(ce_concepts::LegacyOriginalCodePflicht<DemoMimallocAllocator>);
    static_assert(ce_concepts::LegacyOriginalCodePflicht<DemoSnmallocAllocator>);
    static_assert(ce_concepts::HasOriginalCode<DemoMimallocAllocator>);
    static_assert(ce_concepts::HasOriginalCode<DemoSnmallocAllocator>);
    static_assert(ce_concepts::PaperOriginalValidated<DemoMimallocAllocator>);   // Modul = true
    static_assert(!ce_concepts::PaperOriginalValidated<DemoSnmallocAllocator>);  // Modul = false
    SUCCEED();
}

TEST(AchsenMixin, VererbungsHierarchieKorrekt) {
    // OriginalCodeMixinBase<PaperManifest> ist Cross-Topic
    // TestAllocatorOriginalCodeMixin<M> erbt davon
    // OriginalCodeMixin Alias instanziiert es
    // Wrapper erbt von Alias
    using ExpectedMimallocMixin = ::test_allocator_axis::TestAllocatorOriginalCodeMixin<
        simulated_generated::paper_a04_mimalloc::PaperManifest>;
    static_assert(std::is_base_of_v<ExpectedMimallocMixin, DemoMimallocAllocator>);

    using ExpectedBase = ce_concepts::OriginalCodeMixinBase<
        simulated_generated::paper_a04_mimalloc::PaperManifest>;
    static_assert(std::is_base_of_v<ExpectedBase, DemoMimallocAllocator>);
    SUCCEED();
}

// =================================================================
// (7) Pseudocode-Re-Impl Pattern (Non-Paper-Wrapper)
// =================================================================
//
// Wrapper ohne Paper-Original-Code — eigene Manifest mit allen false.

namespace simulated_generated::pseudocode_only {
inline constexpr bool kIsOriginal_allocate   = false;
inline constexpr bool kIsOriginal_deallocate = false;
inline constexpr bool kIsOriginal_reallocate = false;
struct PaperManifest {
    static constexpr std::string_view kCompiler = "self";
    static constexpr bool kHasOriginalPaperCode = false;
    static constexpr bool kIsOriginal_allocate   = false;
    static constexpr bool kIsOriginal_deallocate = false;
    static constexpr bool kIsOriginal_reallocate = false;
};
using OriginalCodeMixin = ::test_allocator_axis::TestAllocatorOriginalCodeMixin<PaperManifest>;
}  // namespace

class DemoBuddyAllocator : public simulated_generated::pseudocode_only::OriginalCodeMixin {
public:
    void* allocate(std::size_t bytes) { return ::malloc(bytes); }
    void  deallocate(void* p) noexcept { ::free(p); }
};

TEST(PseudocodeReImpl, AllFalseHarte) {
    static_assert(DemoBuddyAllocator::get_compiler() == "self");
    static_assert(!DemoBuddyAllocator::has_original_paper_code());
    static_assert(!DemoBuddyAllocator::is_original_allocate());
    static_assert(!DemoBuddyAllocator::is_original_deallocate());
    static_assert(!DemoBuddyAllocator::is_original_module());
    SUCCEED();
}

TEST(PseudocodeReImpl, ConformsButNotPaperOriginalValidated) {
    static_assert(ce_concepts::LegacyOriginalCodePflicht<DemoBuddyAllocator>);
    static_assert(!ce_concepts::HasOriginalCode<DemoBuddyAllocator>);  // has_original=false
    static_assert(!ce_concepts::PaperOriginalValidated<DemoBuddyAllocator>);
    SUCCEED();
}

// =================================================================
// (8) Negativ-Demo — Wrapper ohne Pflicht-API
// =================================================================

class NoLegacyPaperWrapper {};

TEST(LegacyOriginalCodeSubConcept, EmptyClassNotConforms) {
    static_assert(!ce_concepts::LegacyOriginalCodePflicht<NoLegacyPaperWrapper>);
    SUCCEED();
}

// =================================================================
// (9) AxisBase Wurzel-Pattern (V41.F.6.1.P2.A0.7)
// =================================================================
//
// Cross-axis Pflicht-Property get_compiler() — Default "original" in AxisBase,
// Override pro Paper-Wrapper via OriginalCodeMixinBase (delegiert an PaperManifest::kCompiler).

class DefaultAxisWrapper : public ce_topics::AxisBase {
    // KEIN Override → get_compiler() = "original" (Default)
};

TEST(AxisBase, DefaultGetCompilerOriginal) {
    static_assert(DefaultAxisWrapper::get_compiler() == "original");
    static_assert(ce_topics::AxisBase::get_compiler() == "original");
    SUCCEED();
}

TEST(AxisBase, ConceptConformance) {
    static_assert(ce_topics::AxisBaseConcept<DefaultAxisWrapper>);
    static_assert(ce_topics::AxisBaseConcept<ce_topics::AxisBase>);
    SUCCEED();
}

TEST(AxisBase, MimallocOverridesCompiler) {
    // DemoMimallocAllocator erbt von OriginalCodeMixin → OriginalCodeMixinBase
    // → ueberschreibt AxisBase::get_compiler() mit kCompiler aus PaperManifest
    static_assert(DemoMimallocAllocator::get_compiler() == "gcc-9.5");
    static_assert(ce_topics::AxisBaseConcept<DemoMimallocAllocator>);
    SUCCEED();
}

TEST(AxisBase, SnmallocOverridesCompiler) {
    static_assert(DemoSnmallocAllocator::get_compiler() == "clang-12");
    static_assert(ce_topics::AxisBaseConcept<DemoSnmallocAllocator>);
    SUCCEED();
}

TEST(AxisBase, PseudocodeReImplCompilerSelf) {
    static_assert(DemoBuddyAllocator::get_compiler() == "self");
    static_assert(ce_topics::AxisBaseConcept<DemoBuddyAllocator>);
    SUCCEED();
}

TEST(AxisBase, EmptyClassNotAxisBaseConform) {
    static_assert(!ce_topics::AxisBaseConcept<NoLegacyPaperWrapper>);
    SUCCEED();
}
