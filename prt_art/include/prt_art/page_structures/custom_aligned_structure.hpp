#pragma once
// CustomAlignedStructure (P2) — PRT-ART intern / cache-line-aligned, pool-relative Handles
// REV 5 K05 §5.2 + §5.3 — Eigene Optimierungen analog B^2

#include <prt_art/concepts/i_search_page_structure.hpp>

#include <string_view>

namespace comdare::prt_art::page_structures {

class CustomAlignedStructure final : public ISearchPageStructure {
public:
    CustomAlignedStructure() {
        encoding_ = Encoding::CustomAligned;
        layout_invariants_.set(LayoutInvariantKind::Sorted);
        layout_invariants_.set(LayoutInvariantKind::CacheLineAlign);
        layout_invariants_.set(LayoutInvariantKind::NodeAligned);
        layout_invariants_.set(LayoutInvariantKind::Invariant_I4);
    }

    [[nodiscard]] bool check_invariants() const override {
        return layout_invariants_.has(LayoutInvariantKind::CacheLineAlign);
    }

    [[nodiscard]] std::string_view encoding_name() const noexcept override {
        return "CustomAlignedStructure (PRT-ART P2)";
    }
};

}  // namespace comdare::prt_art::page_structures
