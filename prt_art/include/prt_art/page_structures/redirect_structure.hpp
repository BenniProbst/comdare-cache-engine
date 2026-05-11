#pragma once
// RedirectStructure (P0) — CoCo-trie / komprimierter Reststring
// REV 5 K05 §5.2 + §5.3 — Trie-Huelle mit Redirect-Knoten fuer kollabierte Praefixpfade

#include <prt_art/concepts/i_search_page_structure.hpp>

#include <string_view>

namespace comdare::prt_art::page_structures {

class RedirectStructure final : public ISearchPageStructure {
public:
    RedirectStructure() {
        encoding_ = Encoding::Redirect;
        layout_invariants_.set(LayoutInvariantKind::Invariant_I3);   // Redirect-Knoten == eindeutige Restpfade
        layout_invariants_.set(LayoutInvariantKind::CacheLineAlign);
    }

    [[nodiscard]] bool check_invariants() const override {
        return layout_invariants_.has(LayoutInvariantKind::Invariant_I3);
    }

    [[nodiscard]] std::string_view encoding_name() const noexcept override {
        return "RedirectStructure (CoCo P0)";
    }
};

}  // namespace comdare::prt_art::page_structures
