#pragma once
// MultilevelDenseStructure (P1) — START / mehrbyteige Seiten, Cost-Modell, Schwellwert-Umschaltung
// REV 5 K05 §5.2 + §5.3 — Multilevel Nodes mit Cache-Cost-DP

#include <prt_art/concepts/i_search_page_structure.hpp>

#include <string_view>

namespace comdare::prt_art::page_structures {

class MultilevelDenseStructure final : public ISearchPageStructure {
public:
    MultilevelDenseStructure() {
        encoding_ = Encoding::MultilevelDense;
        layout_invariants_.set(LayoutInvariantKind::Sorted);
        layout_invariants_.set(LayoutInvariantKind::CacheLineAlign);
        layout_invariants_.set(LayoutInvariantKind::Invariant_I4);   // variable Codierung -> Interpreter Pflicht
    }

    [[nodiscard]] bool check_invariants() const override {
        return layout_invariants_.has(LayoutInvariantKind::Invariant_I4);
    }

    [[nodiscard]] std::string_view encoding_name() const noexcept override {
        return "MultilevelDenseStructure (START P1)";
    }
};

}  // namespace comdare::prt_art::page_structures
