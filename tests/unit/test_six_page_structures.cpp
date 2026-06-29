// Test fuer 6 Pflicht-Seitentypen-Konkretisierungen + 6 Interpreter (REV 5 K05)

#include <prt_art/page_structures/redirect_structure.hpp>
#include <prt_art/page_structures/dense_byte_structure.hpp>
#include <prt_art/page_structures/sparse_patricia_structure.hpp>
#include <prt_art/page_structures/multilevel_dense_structure.hpp>
#include <prt_art/page_structures/decision_span_structure.hpp>
#include <prt_art/page_structures/custom_aligned_structure.hpp>

#include <prt_art/interpreters/redirect_interpreter.hpp>
#include <prt_art/interpreters/dense_byte_interpreter.hpp>
#include <prt_art/interpreters/patricia_interpreter.hpp>
#include <prt_art/interpreters/multilevel_interpreter.hpp>
#include <prt_art/interpreters/b2_interpreter.hpp>
#include <prt_art/interpreters/custom_interpreter.hpp>

#include <gtest/gtest.h>

namespace ps  = comdare::prt_art::page_structures;
namespace ipr = comdare::prt_art::interpreters;
namespace pa  = comdare::prt_art;

// ─────────────────────────────────────────────────────────────────────────────
// 6 ISearchPageStructure-Konkretisierungen
// ─────────────────────────────────────────────────────────────────────────────

TEST(SixPageStructures, AllSixConcreteEncodingsExist) {
    EXPECT_EQ(ps::RedirectStructure{}.encoding(), pa::Encoding::Redirect);
    EXPECT_EQ(ps::DenseByteStructure{}.encoding(), pa::Encoding::DenseByte);
    EXPECT_EQ(ps::SparsePatriciaStructure{}.encoding(), pa::Encoding::SparsePatricia);
    EXPECT_EQ(ps::MultilevelDenseStructure{}.encoding(), pa::Encoding::MultilevelDense);
    EXPECT_EQ(ps::DecisionSpanStructure{}.encoding(), pa::Encoding::DecisionSpan);
    EXPECT_EQ(ps::CustomAlignedStructure{}.encoding(), pa::Encoding::CustomAligned);
}

TEST(SixPageStructures, AllInvariantsArePassing) {
    EXPECT_TRUE(ps::RedirectStructure{}.check_invariants());
    EXPECT_TRUE(ps::DenseByteStructure{}.check_invariants());
    EXPECT_TRUE(ps::SparsePatriciaStructure{}.check_invariants());
    EXPECT_TRUE(ps::MultilevelDenseStructure{}.check_invariants());
    EXPECT_TRUE(ps::DecisionSpanStructure{}.check_invariants());
    EXPECT_TRUE(ps::CustomAlignedStructure{}.check_invariants());
}

TEST(SixPageStructures, RedirectCarriesInvariantI3) {
    ps::RedirectStructure s;
    EXPECT_TRUE(s.layout_invariants().has(pa::LayoutInvariantKind::Invariant_I3));
}

TEST(SixPageStructures, MultilevelAndDecisionSpanCarryInvariantI4) {
    EXPECT_TRUE(ps::MultilevelDenseStructure{}.layout_invariants().has(pa::LayoutInvariantKind::Invariant_I4));
    EXPECT_TRUE(ps::DecisionSpanStructure{}.layout_invariants().has(pa::LayoutInvariantKind::Invariant_I4));
    EXPECT_TRUE(ps::CustomAlignedStructure{}.layout_invariants().has(pa::LayoutInvariantKind::Invariant_I4));
}

TEST(SixPageStructures, EncodingNamesIncludePriorityTags) {
    EXPECT_NE(ps::RedirectStructure{}.encoding_name().find("P0"), std::string_view::npos);
    EXPECT_NE(ps::DenseByteStructure{}.encoding_name().find("P0"), std::string_view::npos);
    EXPECT_NE(ps::SparsePatriciaStructure{}.encoding_name().find("P0"), std::string_view::npos);
    EXPECT_NE(ps::MultilevelDenseStructure{}.encoding_name().find("P1"), std::string_view::npos);
    EXPECT_NE(ps::DecisionSpanStructure{}.encoding_name().find("P2"), std::string_view::npos);
    EXPECT_NE(ps::CustomAlignedStructure{}.encoding_name().find("P2"), std::string_view::npos);
}

// ─────────────────────────────────────────────────────────────────────────────
// 6 ISearchPageStructureInterpreter-Konkretisierungen (Singleton-Facade)
// ─────────────────────────────────────────────────────────────────────────────

TEST(SixInterpreters, AllSixSingletonsAreReachable) {
    auto& a = ipr::RedirectInterpreter::instance();
    auto& b = ipr::DenseByteInterpreter::instance();
    auto& c = ipr::PatriciaInterpreter::instance();
    auto& d = ipr::MultilevelInterpreter::instance();
    auto& e = ipr::B2Interpreter::instance();
    auto& f = ipr::CustomInterpreter::instance();
    EXPECT_NE(&a, nullptr);
    EXPECT_NE(&b, nullptr);
    EXPECT_NE(&c, nullptr);
    EXPECT_NE(&d, nullptr);
    EXPECT_NE(&e, nullptr);
    EXPECT_NE(&f, nullptr);
}

TEST(SixInterpreters, SingletonReturnsSameInstance) {
    auto& a1 = ipr::DenseByteInterpreter::instance();
    auto& a2 = ipr::DenseByteInterpreter::instance();
    EXPECT_EQ(&a1, &a2);
}

TEST(SixInterpreters, SimdSupportFlagsMatchExpectations) {
    EXPECT_FALSE(ipr::RedirectInterpreter::instance().supports_simd());
    EXPECT_TRUE(ipr::DenseByteInterpreter::instance().supports_simd());
    EXPECT_TRUE(ipr::PatriciaInterpreter::instance().supports_simd()); // PEXT/AVX2
    EXPECT_FALSE(ipr::MultilevelInterpreter::instance().supports_simd());
    EXPECT_FALSE(ipr::B2Interpreter::instance().supports_simd());
    EXPECT_TRUE(ipr::CustomInterpreter::instance().supports_simd());
}

TEST(SixInterpreters, NextSlotReturnsValidByDefault) {
    ps::DenseByteStructure s;
    auto                   handle = ipr::DenseByteInterpreter::instance().next_slot(s, {});
    EXPECT_TRUE(handle.valid);
}
