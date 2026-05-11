// test_concepts_compile.cpp - Smoke-Test: alle prt_art-Concepts kompilieren

#include "prt_art/prt_art.hpp"

#include <gtest/gtest.h>

#include <cstdint>

TEST(ConceptsCompile, AllHeadersIncluded) {
    // Wenn dieser Test ueberhaupt linkt, sind alle Concepts kompilierbar.
    SUCCEED();
}

TEST(ConceptsCompile, EncodingEnumHas6Values) {
    using comdare::prt_art::Encoding;
    // 6 Seitentypen aus Termin 4 Scope-Freeze
    EXPECT_EQ(static_cast<int>(Encoding::Redirect), 0);
    EXPECT_EQ(static_cast<int>(Encoding::DenseByte), 1);
    EXPECT_EQ(static_cast<int>(Encoding::MultilevelDense), 2);
    EXPECT_EQ(static_cast<int>(Encoding::SparsePatricia), 3);
    EXPECT_EQ(static_cast<int>(Encoding::DecisionSpan), 4);
    EXPECT_EQ(static_cast<int>(Encoding::CustomAligned), 5);
}

TEST(ConceptsCompile, NodeRefKindHas5Variants) {
    using comdare::prt_art::NodeRefKind;
    EXPECT_EQ(static_cast<int>(NodeRefKind::ValueRef), 0);
    EXPECT_EQ(static_cast<int>(NodeRefKind::ChildRef), 1);
    EXPECT_EQ(static_cast<int>(NodeRefKind::LayerRef), 2);
    EXPECT_EQ(static_cast<int>(NodeRefKind::SiblingRef), 3);
    EXPECT_EQ(static_cast<int>(NodeRefKind::SuffixRef), 4);
}

TEST(ConceptsCompile, DefaultFanoutWidthIs15) {
    EXPECT_EQ(comdare::prt_art::kDefaultFanoutWidth, 15u);
}

TEST(ConceptsCompile, IteratorModeHas3Modes) {
    using comdare::prt_art::IteratorMode;
    EXPECT_NE(static_cast<int>(IteratorMode::Default),
              static_cast<int>(IteratorMode::LocallyOrdered));
    EXPECT_NE(static_cast<int>(IteratorMode::LocallyOrdered),
              static_cast<int>(IteratorMode::Lex));
}
