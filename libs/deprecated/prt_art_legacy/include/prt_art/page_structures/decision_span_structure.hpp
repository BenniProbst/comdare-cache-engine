#pragma once
// DecisionSpanStructure (P2) — B^2-Tree / Decision + Span Sub-Trees pro 64KiB
// REV 5 K05 §5.2 + §5.3 — Page-lokale Mini-Search-Strukturen

#include <prt_art/concepts/i_search_page_structure.hpp>

#include <string_view>

namespace comdare::prt_art::page_structures {

class DecisionSpanStructure final : public ISearchPageStructure {
public:
    DecisionSpanStructure() {
        encoding_ = Encoding::DecisionSpan;
        layout_invariants_.set(LayoutInvariantKind::Sorted);
        layout_invariants_.set(LayoutInvariantKind::CacheLineAlign);
        layout_invariants_.set(LayoutInvariantKind::Invariant_I4);
    }

    [[nodiscard]] bool check_invariants() const override {
        return layout_invariants_.has(LayoutInvariantKind::Sorted) &&
               layout_invariants_.has(LayoutInvariantKind::Invariant_I4);
    }

    [[nodiscard]] std::string_view encoding_name() const noexcept override {
        return "DecisionSpanStructure (B^2 P2)";
    }
};

}  // namespace comdare::prt_art::page_structures
