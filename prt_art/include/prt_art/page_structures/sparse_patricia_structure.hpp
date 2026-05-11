#pragma once
// SparsePatriciaStructure (P0/P1) — HOT / k-constrained, diskriminierende Bits, 9 Layouts
// REV 5 K05 §5.2 + §5.3 — Compound Nodes mit Single-/Multi-Mask-Layouts

#include <prt_art/concepts/i_search_page_structure.hpp>

#include <string_view>

namespace comdare::prt_art::page_structures {

class SparsePatriciaStructure final : public ISearchPageStructure {
public:
    SparsePatriciaStructure() {
        encoding_ = Encoding::SparsePatricia;
        layout_invariants_.set(LayoutInvariantKind::UniquePrefixes);
        layout_invariants_.set(LayoutInvariantKind::CacheLineAlign);
    }

    [[nodiscard]] bool check_invariants() const override {
        return layout_invariants_.has(LayoutInvariantKind::UniquePrefixes);
    }

    [[nodiscard]] std::string_view encoding_name() const noexcept override {
        return "SparsePatriciaStructure (HOT P0/P1)";
    }
};

}  // namespace comdare::prt_art::page_structures
