#pragma once
// B2Interpreter — Decision-then-Span page-lokale Suche fuer DecisionSpanStructure
// REV 5 K05 §2.2

#include <prt_art/concepts/i_search_page_structure.hpp>
#include <prt_art/concepts/i_search_page_structure_interpreter.hpp>

namespace comdare::prt_art::interpreters {

class B2Interpreter final : public ISearchPageStructureInterpreter {
public:
    [[nodiscard]] InterpretResult interpret(ISearchPageStructure const&,
                                            std::span<std::byte const>) const override {
        return {InterpretKind::NextSlot, SlotHandle{0, true}, nullptr};
    }

    [[nodiscard]] SlotHandle next_slot(ISearchPageStructure const&,
                                       std::span<std::byte const>) const override {
        return SlotHandle{0, true};
    }

    [[nodiscard]] bool supports_simd() const noexcept override { return false; }

    [[nodiscard]] static B2Interpreter const& instance() noexcept {
        static B2Interpreter inst;
        return inst;
    }
};

}  // namespace comdare::prt_art::interpreters
