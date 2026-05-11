#pragma once
// DenseByteStructure (P0) — ART / 1-Byte direkt-adressiert (Node4/16/48/256)
// REV 5 K05 §5.2 + §5.3 — Adaptive Radix Tree als Pflicht-Seitentyp

#include <prt_art/concepts/i_search_page_structure.hpp>

#include <string_view>

namespace comdare::prt_art::page_structures {

class DenseByteStructure final : public ISearchPageStructure {
public:
    DenseByteStructure() {
        encoding_ = Encoding::DenseByte;
        layout_invariants_.set(LayoutInvariantKind::Sorted);
        layout_invariants_.set(LayoutInvariantKind::CacheLineAlign);
        layout_invariants_.set(LayoutInvariantKind::NodeAligned);
    }

    [[nodiscard]] bool check_invariants() const override {
        return layout_invariants_.has(LayoutInvariantKind::Sorted) &&
               layout_invariants_.has(LayoutInvariantKind::CacheLineAlign);
    }

    [[nodiscard]] std::string_view encoding_name() const noexcept override {
        return "DenseByteStructure (ART P0)";
    }
};

}  // namespace comdare::prt_art::page_structures
