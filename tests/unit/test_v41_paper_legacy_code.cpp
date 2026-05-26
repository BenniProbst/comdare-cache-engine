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
/// Erbt von cross-topic OriginalCodeMixinBase (Compiler)
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
    static_assert(DemoMimallocAllocator::is_original_allocate());
    static_assert(DemoMimallocAllocator::is_original_deallocate());
    static_assert(DemoMimallocAllocator::is_original_reallocate());
    static_assert(DemoMimallocAllocator::is_original_module());
    SUCCEED();
}

TEST(AchsenMixin, SnmallocMismatchDetected) {
    static_assert(DemoSnmallocAllocator::get_compiler() == "clang-12");
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

// =================================================================
// (10) P2.F Smoke-Tests — 6 echte Achsen-Mixin-Templates aus topics/
// =================================================================
//
// Validiert dass die 6 echten Mixin-Header (P2.F) kompilieren + saubere
// Semantik haben (alle Pflicht-Functions originall → is_original_module = true).

#include <topics/allocator/axis_06_allocator/concepts/axis_06_allocator_original_code_mixin.hpp>
#include <topics/allocator/axis_06_allocator/concepts/axis_06_allocator_interface_functions.hpp>
#include <topics/queuing/axis_q1_queuing/concepts/axis_q1_queuing_original_code_mixin.hpp>
#include <topics/queuing/axis_q1_queuing/concepts/axis_q1_queuing_interface_functions.hpp>
#include <topics/queuing/axis_q2_queuing/concepts/axis_q2_queuing_original_code_mixin.hpp>
#include <topics/queuing/axis_q2_queuing/concepts/axis_q2_queuing_interface_functions.hpp>
#include <topics/traversal/axis_03a_search_algo/concepts/axis_03a_search_algo_original_code_mixin.hpp>
#include <topics/traversal/axis_03a_search_algo/concepts/axis_03a_search_algo_interface_functions.hpp>
#include <topics/traversal/axis_03b_cache_traversal/concepts/axis_03b_cache_traversal_original_code_mixin.hpp>
#include <topics/traversal/axis_03b_cache_traversal/concepts/axis_03b_cache_traversal_interface_functions.hpp>
#include <topics/traversal/axis_03m_mapping/concepts/axis_03m_mapping_original_code_mixin.hpp>
#include <topics/traversal/axis_03m_mapping/concepts/axis_03m_mapping_interface_functions.hpp>

namespace p2f_smoke {

// Dummy-Manifests: alle Pflicht-Functions originall → is_original_module = true
struct AllocatorManifest {
    static constexpr std::string_view kCompiler = "gcc-9.5";
    static constexpr bool kHasOriginalPaperCode = true;
    static constexpr bool kIsOriginal_allocate   = true;
    static constexpr bool kIsOriginal_deallocate = true;
};
struct BufferManifest {
    static constexpr std::string_view kCompiler = "gcc-9.5";
    static constexpr bool kHasOriginalPaperCode = true;
    static constexpr bool kIsOriginal_put        = true;
    static constexpr bool kIsOriginal_get        = true;
    static constexpr bool kIsOriginal_emplace    = true;
    static constexpr bool kIsOriginal_peek_front = true;
    static constexpr bool kIsOriginal_peek_back  = true;
    static constexpr bool kIsOriginal_clear      = true;
};
struct FlushManifest {
    static constexpr std::string_view kCompiler = "gcc-9.5";
    static constexpr bool kHasOriginalPaperCode = true;
    static constexpr bool kIsOriginal_should_flush      = true;
    static constexpr bool kIsOriginal_on_flush_complete = true;
};
struct SearchAlgoManifest {
    static constexpr std::string_view kCompiler = "gcc-9.5";
    static constexpr bool kHasOriginalPaperCode = true;
    static constexpr bool kIsOriginal_insert = true;
    static constexpr bool kIsOriginal_lookup = true;
    static constexpr bool kIsOriginal_erase  = true;
    static constexpr bool kIsOriginal_clear  = true;
};
struct CacheTraversalManifest {
    static constexpr std::string_view kCompiler = "gcc-9.5";
    static constexpr bool kHasOriginalPaperCode = true;
    static constexpr bool kIsOriginal_register_entry = true;
    static constexpr bool kIsOriginal_resolve        = true;
    static constexpr bool kIsOriginal_unregister    = true;
    static constexpr bool kIsOriginal_clear         = true;
};
struct MappingManifest {
    static constexpr std::string_view kCompiler = "gcc-9.5";
    static constexpr bool kHasOriginalPaperCode = true;
    static constexpr bool kIsOriginal_register_slot  = true;
    static constexpr bool kIsOriginal_resolve_offset = true;
    static constexpr bool kIsOriginal_reverse_lookup = true;
    static constexpr bool kIsOriginal_clear          = true;
};

using AllocatorMixin       = ::comdare::cache_engine::allocator::axis_06_allocator::concepts::AllocatorOriginalCodeMixin<AllocatorManifest>;
using BufferMixin          = ::comdare::cache_engine::queuing::axis_q1_queuing::concepts::BufferOriginalCodeMixin<BufferManifest>;
using FlushMixin           = ::comdare::cache_engine::queuing::axis_q2_queuing::concepts::FlushOriginalCodeMixin<FlushManifest>;
using SearchAlgoMixin      = ::comdare::cache_engine::traversal::axis_03a_search_algo::concepts::SearchAlgoOriginalCodeMixin<SearchAlgoManifest>;
using CacheTraversalMixin  = ::comdare::cache_engine::traversal::axis_03b_cache_traversal::concepts::CacheTraversalOriginalCodeMixin<CacheTraversalManifest>;
using MappingMixin         = ::comdare::cache_engine::traversal::axis_03m_mapping::concepts::MappingOriginalCodeMixin<MappingManifest>;

}  // namespace p2f_smoke

TEST(P2F_AxisMixins, AllocatorMixinModuleTrue) {
    static_assert(p2f_smoke::AllocatorMixin::get_compiler() == "gcc-9.5");
    static_assert(p2f_smoke::AllocatorMixin::is_original_allocate());
    static_assert(p2f_smoke::AllocatorMixin::is_original_deallocate());
    static_assert(p2f_smoke::AllocatorMixin::is_original_module());
    static_assert(ce_topics::AxisBaseConcept<p2f_smoke::AllocatorMixin>);
    SUCCEED();
}

TEST(P2F_AxisMixins, BufferMixinModuleTrue) {
    static_assert(p2f_smoke::BufferMixin::is_original_put());
    static_assert(p2f_smoke::BufferMixin::is_original_emplace());
    static_assert(p2f_smoke::BufferMixin::is_original_peek_back());
    static_assert(p2f_smoke::BufferMixin::is_original_module());
    static_assert(ce_topics::AxisBaseConcept<p2f_smoke::BufferMixin>);
    SUCCEED();
}

TEST(P2F_AxisMixins, FlushMixinModuleTrue) {
    static_assert(p2f_smoke::FlushMixin::is_original_should_flush());
    static_assert(p2f_smoke::FlushMixin::is_original_on_flush_complete());
    static_assert(p2f_smoke::FlushMixin::is_original_module());
    static_assert(ce_topics::AxisBaseConcept<p2f_smoke::FlushMixin>);
    SUCCEED();
}

TEST(P2F_AxisMixins, SearchAlgoMixinModuleTrue) {
    static_assert(p2f_smoke::SearchAlgoMixin::is_original_insert());
    static_assert(p2f_smoke::SearchAlgoMixin::is_original_lookup());
    static_assert(p2f_smoke::SearchAlgoMixin::is_original_module());
    static_assert(ce_topics::AxisBaseConcept<p2f_smoke::SearchAlgoMixin>);
    SUCCEED();
}

TEST(P2F_AxisMixins, CacheTraversalMixinModuleTrue) {
    static_assert(p2f_smoke::CacheTraversalMixin::is_original_register_entry());
    static_assert(p2f_smoke::CacheTraversalMixin::is_original_resolve());
    static_assert(p2f_smoke::CacheTraversalMixin::is_original_module());
    static_assert(ce_topics::AxisBaseConcept<p2f_smoke::CacheTraversalMixin>);
    SUCCEED();
}

TEST(P2F_AxisMixins, MappingMixinModuleTrue) {
    static_assert(p2f_smoke::MappingMixin::is_original_register_slot());
    static_assert(p2f_smoke::MappingMixin::is_original_reverse_lookup());
    static_assert(p2f_smoke::MappingMixin::is_original_module());
    static_assert(ce_topics::AxisBaseConcept<p2f_smoke::MappingMixin>);
    SUCCEED();
}

TEST(P2F_AxisMixins, InterfaceFunctionsListsCorrectCount) {
    using namespace ::comdare::cache_engine;
    static_assert(allocator::axis_06_allocator::concepts::kAxisInterfaceFunctions.size() == 2);
    static_assert(queuing::axis_q1_queuing::concepts::kAxisInterfaceFunctions.size() == 6);
    static_assert(queuing::axis_q2_queuing::concepts::kAxisInterfaceFunctions.size() == 2);
    static_assert(traversal::axis_03a_search_algo::concepts::kAxisInterfaceFunctions.size() == 4);
    static_assert(traversal::axis_03b_cache_traversal::concepts::kAxisInterfaceFunctions.size() == 4);
    static_assert(traversal::axis_03m_mapping::concepts::kAxisInterfaceFunctions.size() == 4);
    SUCCEED();
}

// =================================================================
// (11) P2.B + P2.D Paper-Wrappers — TYPED_TEST_SUITE (kompakt, skaliert auf Roll-out)
// =================================================================
//
// Cross-Validation aller Paper-Wrappers via TYPED_TEST_SUITE statt redundante
// per-Wrapper-Tests ([[cross-axis-defaults-no-bloat]] Pattern-Disziplin).
// Bei Roll-out weiterer Paper-Wrappers: nur Type-List PaperWrapperList ergaenzen,
// automatisch ~8 Tests pro Wrapper.

#include <topics/allocator/axis_06_allocator/axis_06_allocator_mimalloc.hpp>
#include <topics/allocator/axis_06_allocator/axis_06_allocator_jemalloc.hpp>
#include <topics/allocator/axis_06_allocator/axis_06_allocator_snmalloc.hpp>
#include <topics/allocator/axis_06_allocator/axis_06_allocator_dlmalloc.hpp>
#include <topics/allocator/axis_06_allocator/axis_06_allocator_rpmalloc.hpp>
#include <topics/allocator/axis_06_allocator/axis_06_allocator_lrmalloc.hpp>

namespace paper_wrappers {
using Mimalloc = ::comdare::cache_engine::allocator::axis_06_allocator::MimallocAllocator;
using Jemalloc = ::comdare::cache_engine::allocator::axis_06_allocator::JemallocAllocator;
using Snmalloc = ::comdare::cache_engine::allocator::axis_06_allocator::SnmallocAllocator;
using Dlmalloc = ::comdare::cache_engine::allocator::axis_06_allocator::DlmallocAllocator;
using Rpmalloc = ::comdare::cache_engine::allocator::axis_06_allocator::RPMallocAllocator;
using Lrmalloc = ::comdare::cache_engine::allocator::axis_06_allocator::LRMallocAllocator;
}  // namespace

// V41.F.6.1.P2.D Batch 2: 6 Paper-Wrappers (3 Batch 1 + 3 Batch 2). Auto-Skalierung: TYPED_TEST = N × 8.
using PaperWrapperList = ::testing::Types<
    paper_wrappers::Mimalloc,
    paper_wrappers::Jemalloc,
    paper_wrappers::Snmalloc,
    paper_wrappers::Dlmalloc,
    paper_wrappers::Rpmalloc,
    paper_wrappers::Lrmalloc
>;

template <typename W> class PaperWrapperConformance : public ::testing::Test {};
TYPED_TEST_SUITE(PaperWrapperConformance, PaperWrapperList);

TYPED_TEST(PaperWrapperConformance, AxisBaseConcept) {
    static_assert(ce_topics::AxisBaseConcept<TypeParam>);
    SUCCEED();
}
TYPED_TEST(PaperWrapperConformance, LegacyOriginalCodePflichtConcept) {
    static_assert(ce_concepts::LegacyOriginalCodePflicht<TypeParam>);
    SUCCEED();
}
TYPED_TEST(PaperWrapperConformance, HasOriginalCodeConcept) {
    static_assert(ce_concepts::HasOriginalCode<TypeParam>,
                  "Paper-Wrapper get_compiler() muss konkret sein (nicht original/self/system)");
    SUCCEED();
}
TYPED_TEST(PaperWrapperConformance, PaperOriginalValidatedConcept) {
    static_assert(ce_concepts::PaperOriginalValidated<TypeParam>,
                  "Paper-Wrapper is_original_module() == true (First-Build SHA-Match)");
    SUCCEED();
}
TYPED_TEST(PaperWrapperConformance, GetCompilerOverridesAxisBaseDefault) {
    static_assert(TypeParam::get_compiler() != std::string_view{"original"});
    static_assert(TypeParam::get_compiler() != std::string_view{"self"});
    static_assert(TypeParam::get_compiler() != std::string_view{"system"});
    SUCCEED();
}
TYPED_TEST(PaperWrapperConformance, IsOriginalAllocate) {
    static_assert(TypeParam::is_original_allocate());
    SUCCEED();
}
TYPED_TEST(PaperWrapperConformance, IsOriginalDeallocate) {
    static_assert(TypeParam::is_original_deallocate());
    SUCCEED();
}
TYPED_TEST(PaperWrapperConformance, IsOriginalModuleAggregation) {
    static_assert(TypeParam::is_original_module());
    SUCCEED();
}

// =================================================================
// (11b) P2.D.tr.s2 Original-SearchAlgo-Wrappers — Cross-Validation
// =================================================================
//
// Pattern-Disziplin [[cross-axis-defaults-no-bloat]]: Auto-Skalierung via TYPED_TEST.
// 2 separate Suites wegen unterschiedlicher is_original_module()-Erwartung:
//   - FullOriginal:    alle 4 Functions originall (ART) — is_original_module()=true
//   - PartialOriginal: 2/4 originall + 2 Luecken (HOT, START) — is_original_module()=false

#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_original_art.hpp>
#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_original_hot.hpp>
#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_original_start.hpp>
// V41.F.6.1.P2.D.tr.s3 Batch 1
#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_original_wormhole.hpp>
#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_original_surf.hpp>

namespace original_search_algo {
using OrigArt      = ::comdare::cache_engine::traversal::axis_03a_search_algo::OriginalArtSearchAlgo;
using OrigHot      = ::comdare::cache_engine::traversal::axis_03a_search_algo::OriginalHotSearchAlgo;
using OrigStart    = ::comdare::cache_engine::traversal::axis_03a_search_algo::OriginalStartSearchAlgo;
using OrigWormhole = ::comdare::cache_engine::traversal::axis_03a_search_algo::OriginalWormholeSearchAlgo;
using OrigSurf     = ::comdare::cache_engine::traversal::axis_03a_search_algo::OriginalSurfSearchAlgo;
}  // namespace

// (a) Full-Original SearchAlgo (alle Functions Paper-bound) — heute nur ART
using FullOriginalSearchAlgoList = ::testing::Types<
    original_search_algo::OrigArt
>;
template <typename W> class FullOriginalSearchAlgoConformance : public ::testing::Test {};
TYPED_TEST_SUITE(FullOriginalSearchAlgoConformance, FullOriginalSearchAlgoList);

TYPED_TEST(FullOriginalSearchAlgoConformance, AxisBaseConcept) {
    static_assert(ce_topics::AxisBaseConcept<TypeParam>);
    SUCCEED();
}
TYPED_TEST(FullOriginalSearchAlgoConformance, LegacyOriginalCodePflichtConcept) {
    static_assert(ce_concepts::LegacyOriginalCodePflicht<TypeParam>);
    SUCCEED();
}
TYPED_TEST(FullOriginalSearchAlgoConformance, HasOriginalCodeConcept) {
    static_assert(ce_concepts::HasOriginalCode<TypeParam>);
    SUCCEED();
}
TYPED_TEST(FullOriginalSearchAlgoConformance, PaperOriginalValidatedConcept) {
    static_assert(ce_concepts::PaperOriginalValidated<TypeParam>,
                  "Full-Original SearchAlgo: is_original_module() MUSS true sein (alle 4 Functions originall)");
    SUCCEED();
}
TYPED_TEST(FullOriginalSearchAlgoConformance, GetCompilerOverridesAxisBaseDefault) {
    static_assert(TypeParam::get_compiler() != std::string_view{"original"});
    static_assert(TypeParam::get_compiler() != std::string_view{"self"});
    static_assert(TypeParam::get_compiler() != std::string_view{"system"});
    SUCCEED();
}
TYPED_TEST(FullOriginalSearchAlgoConformance, IsOriginalInsert) {
    static_assert(TypeParam::is_original_insert());
    SUCCEED();
}
TYPED_TEST(FullOriginalSearchAlgoConformance, IsOriginalLookup) {
    static_assert(TypeParam::is_original_lookup());
    SUCCEED();
}
TYPED_TEST(FullOriginalSearchAlgoConformance, IsOriginalErase) {
    static_assert(TypeParam::is_original_erase());
    SUCCEED();
}
TYPED_TEST(FullOriginalSearchAlgoConformance, IsOriginalClear) {
    static_assert(TypeParam::is_original_clear());
    SUCCEED();
}
TYPED_TEST(FullOriginalSearchAlgoConformance, IsOriginalModuleAggregation) {
    static_assert(TypeParam::is_original_module());
    SUCCEED();
}

// (b) Partial-Original SearchAlgo (Paper-API insert+lookup originall, erase+clear Lücken) — HOT + START
using PartialOriginalSearchAlgoList = ::testing::Types<
    original_search_algo::OrigHot,
    original_search_algo::OrigStart
>;
template <typename W> class PartialOriginalSearchAlgoConformance : public ::testing::Test {};
TYPED_TEST_SUITE(PartialOriginalSearchAlgoConformance, PartialOriginalSearchAlgoList);

TYPED_TEST(PartialOriginalSearchAlgoConformance, AxisBaseConcept) {
    static_assert(ce_topics::AxisBaseConcept<TypeParam>);
    SUCCEED();
}
TYPED_TEST(PartialOriginalSearchAlgoConformance, LegacyOriginalCodePflichtConcept) {
    static_assert(ce_concepts::LegacyOriginalCodePflicht<TypeParam>);
    SUCCEED();
}
TYPED_TEST(PartialOriginalSearchAlgoConformance, HasOriginalCodeConcept) {
    static_assert(ce_concepts::HasOriginalCode<TypeParam>,
                  "Partial-Original SearchAlgo: get_compiler() MUSS konkret sein (gcc-9.5 via Mixin), HasOriginalCode greift");
    SUCCEED();
}
TYPED_TEST(PartialOriginalSearchAlgoConformance, NotPaperOriginalValidated) {
    static_assert(!ce_concepts::PaperOriginalValidated<TypeParam>,
                  "Partial-Original SearchAlgo: is_original_module() MUSS false sein (2/4 Lücken)");
    SUCCEED();
}
TYPED_TEST(PartialOriginalSearchAlgoConformance, GetCompilerOverridesAxisBaseDefault) {
    static_assert(TypeParam::get_compiler() != std::string_view{"original"});
    SUCCEED();
}
TYPED_TEST(PartialOriginalSearchAlgoConformance, PaperApiFunctionsOriginal) {
    static_assert(TypeParam::is_original_insert(),
                  "Partial-Original: insert MUSS Paper-API originall sein");
    static_assert(TypeParam::is_original_lookup(),
                  "Partial-Original: lookup MUSS Paper-API originall sein");
    SUCCEED();
}
TYPED_TEST(PartialOriginalSearchAlgoConformance, LueckenFunctionsNotOriginal) {
    static_assert(!TypeParam::is_original_erase(),
                  "Partial-Original: erase ist Cache-Engine Re-Impl Luecke — MUSS false sein");
    static_assert(!TypeParam::is_original_clear(),
                  "Partial-Original: clear ist Cache-Engine Re-Impl Luecke — MUSS false sein");
    SUCCEED();
}

// (c) Flexible-Original SearchAlgo (s3 Batch 1) — variable Anzahl Original-Functions,
// gemeinsam: HasOriginalCode + !PaperOriginalValidated + is_original_lookup=true.
// Wormhole 3/4 (insert+lookup+erase, clear-Lücke), SuRF/Masstree 1/4 (nur lookup).
using FlexibleOriginalSearchAlgoList = ::testing::Types<
    original_search_algo::OrigWormhole,
    original_search_algo::OrigSurf
>;
template <typename W> class FlexibleOriginalSearchAlgoConformance : public ::testing::Test {};
TYPED_TEST_SUITE(FlexibleOriginalSearchAlgoConformance, FlexibleOriginalSearchAlgoList);

TYPED_TEST(FlexibleOriginalSearchAlgoConformance, AxisBaseConcept) {
    static_assert(ce_topics::AxisBaseConcept<TypeParam>);
    SUCCEED();
}
TYPED_TEST(FlexibleOriginalSearchAlgoConformance, LegacyOriginalCodePflichtConcept) {
    static_assert(ce_concepts::LegacyOriginalCodePflicht<TypeParam>);
    SUCCEED();
}
TYPED_TEST(FlexibleOriginalSearchAlgoConformance, HasOriginalCodeConcept) {
    static_assert(ce_concepts::HasOriginalCode<TypeParam>);
    SUCCEED();
}
TYPED_TEST(FlexibleOriginalSearchAlgoConformance, NotPaperOriginalValidated) {
    static_assert(!ce_concepts::PaperOriginalValidated<TypeParam>,
                  "Flexible-Original: is_original_module() MUSS false sein (mind. 1 Lücke)");
    SUCCEED();
}
TYPED_TEST(FlexibleOriginalSearchAlgoConformance, GetCompilerOverridesAxisBaseDefault) {
    static_assert(TypeParam::get_compiler() != std::string_view{"original"});
    SUCCEED();
}
TYPED_TEST(FlexibleOriginalSearchAlgoConformance, LookupAlwaysOriginal) {
    static_assert(TypeParam::is_original_lookup(),
                  "Flexible-Original: lookup MUSS in jedem Paper-Bindung originall sein (gemeinsamer Nenner)");
    SUCCEED();
}

// =================================================================
// (11c) P2.D.q.s2 Original-Buffer-Wrappers (Q1 Queuing) — Cross-Validation
// =================================================================
//
// Erste externe Submodule-Bindung im queuing-Topic (moodycamel ConcurrentQueue).
// Pattern analog (11b) Partial-Original SearchAlgo, aber Q1 hat 6 Functions
// (put/get/emplace/peek_front/peek_back/clear) statt 4.

#include <topics/queuing/axis_q1_queuing/axis_q1_queuing_original_concurrentqueue.hpp>

namespace original_buffer {
using OrigConcurrentQueue = ::comdare::cache_engine::queuing::axis_q1_queuing::OriginalLockFreeMpmcConcurrentQueue;
}  // namespace

// Q01 ConcurrentQueue ist Partial-Original (2/6: put+get originall, 4 Lücken).
// Bei Roll-out weiterer Q-Wrappers nur PartialOriginalBufferList ergaenzen.
using PartialOriginalBufferList = ::testing::Types<
    original_buffer::OrigConcurrentQueue
>;
template <typename W> class PartialOriginalBufferConformance : public ::testing::Test {};
TYPED_TEST_SUITE(PartialOriginalBufferConformance, PartialOriginalBufferList);

TYPED_TEST(PartialOriginalBufferConformance, AxisBaseConcept) {
    static_assert(ce_topics::AxisBaseConcept<TypeParam>);
    SUCCEED();
}
TYPED_TEST(PartialOriginalBufferConformance, LegacyOriginalCodePflichtConcept) {
    static_assert(ce_concepts::LegacyOriginalCodePflicht<TypeParam>);
    SUCCEED();
}
TYPED_TEST(PartialOriginalBufferConformance, HasOriginalCodeConcept) {
    static_assert(ce_concepts::HasOriginalCode<TypeParam>,
                  "Partial-Original Buffer: get_compiler() MUSS konkret sein (gcc-9.5 via Mixin)");
    SUCCEED();
}
TYPED_TEST(PartialOriginalBufferConformance, NotPaperOriginalValidated) {
    static_assert(!ce_concepts::PaperOriginalValidated<TypeParam>,
                  "Partial-Original Buffer: is_original_module() MUSS false sein (4/6 Lücken)");
    SUCCEED();
}
TYPED_TEST(PartialOriginalBufferConformance, GetCompilerOverridesAxisBaseDefault) {
    static_assert(TypeParam::get_compiler() != std::string_view{"original"});
    SUCCEED();
}
TYPED_TEST(PartialOriginalBufferConformance, PaperApiFunctionsOriginal) {
    static_assert(TypeParam::is_original_put(),
                  "Partial-Original Buffer: put MUSS Paper-API originall sein (enqueue)");
    static_assert(TypeParam::is_original_get(),
                  "Partial-Original Buffer: get MUSS Paper-API originall sein (try_dequeue)");
    SUCCEED();
}
TYPED_TEST(PartialOriginalBufferConformance, LueckenFunctionsNotOriginal) {
    static_assert(!TypeParam::is_original_emplace(),
                  "Partial-Original Buffer: emplace ist Re-Impl Luecke (concurrentqueue hat keine emplace)");
    static_assert(!TypeParam::is_original_peek_front(),
                  "Partial-Original Buffer: peek_front ist Re-Impl Luecke (async-only)");
    static_assert(!TypeParam::is_original_peek_back(),
                  "Partial-Original Buffer: peek_back ist Re-Impl Luecke");
    static_assert(!TypeParam::is_original_clear(),
                  "Partial-Original Buffer: clear ist Re-Impl Luecke (kein clear in concurrentqueue)");
    SUCCEED();
}

// =================================================================
// (12) P2.C Non-Paper-Wrappers — TYPED_TEST_SUITE (kompakt, cross-topic)
// =================================================================
//
// Cross-Validation aller Non-Paper-Wrappers (1 pro Achse) via TYPED_TEST_SUITE.
// Defaults kommen via AxisBase generisch — Test prueft Default-Konsistenz.

#include <topics/allocator/axis_06_allocator/axis_06_allocator_std_malloc.hpp>
#include <topics/queuing/axis_q1_queuing/axis_q1_queuing_fifo.hpp>
#include <topics/queuing/axis_q2_queuing/axis_q2_queuing_eager.hpp>
#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_array256.hpp>
#include <topics/traversal/axis_03b_cache_traversal/axis_03b_cache_traversal_linear_fanout.hpp>
#include <topics/traversal/axis_03m_mapping/axis_03m_mapping_direct_placement.hpp>

namespace non_paper_wrappers {
using NoPaperAllocator   = ::comdare::cache_engine::allocator::axis_06_allocator::StdMalloc;
using NoPaperBuffer      = ::comdare::cache_engine::queuing::axis_q1_queuing::FIFOQueue;
using NoPaperFlushPolicy = ::comdare::cache_engine::queuing::axis_q2_queuing::EagerFlush;
using NoPaperSearchAlgo  = ::comdare::cache_engine::traversal::axis_03a_search_algo::Array256;
using NoPaperTraversal   = ::comdare::cache_engine::traversal::axis_03b_cache_traversal::LinearFanout;
using NoPaperMapping     = ::comdare::cache_engine::traversal::axis_03m_mapping::DirectPlacement;
}  // namespace

using NonPaperWrapperList = ::testing::Types<
    non_paper_wrappers::NoPaperAllocator,
    non_paper_wrappers::NoPaperBuffer,
    non_paper_wrappers::NoPaperFlushPolicy,
    non_paper_wrappers::NoPaperSearchAlgo,
    non_paper_wrappers::NoPaperTraversal,
    non_paper_wrappers::NoPaperMapping
>;

template <typename W> class NonPaperWrapperDefaults : public ::testing::Test {};
TYPED_TEST_SUITE(NonPaperWrapperDefaults, NonPaperWrapperList);

TYPED_TEST(NonPaperWrapperDefaults, LegacyOriginalCodePflichtConcept) {
    static_assert(ce_concepts::LegacyOriginalCodePflicht<TypeParam>);
    SUCCEED();
}
TYPED_TEST(NonPaperWrapperDefaults, GetCompilerDefaultOriginal) {
    static_assert(TypeParam::get_compiler() == std::string_view{"original"},
                  "Non-Paper-Wrapper: AxisBase Default 'original' greift (kein Override)");
    SUCCEED();
}
TYPED_TEST(NonPaperWrapperDefaults, IsOriginalModuleDefaultFalse) {
    static_assert(!TypeParam::is_original_module(),
                  "Non-Paper-Wrapper: AxisBase Default false greift (kein Paper-Mixin)");
    SUCCEED();
}
TYPED_TEST(NonPaperWrapperDefaults, NotHasOriginalCode) {
    static_assert(!ce_concepts::HasOriginalCode<TypeParam>);
    SUCCEED();
}
TYPED_TEST(NonPaperWrapperDefaults, NotPaperOriginalValidated) {
    static_assert(!ce_concepts::PaperOriginalValidated<TypeParam>);
    SUCCEED();
}
