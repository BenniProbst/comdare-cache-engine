#pragma once
// V41.F.6.1.R5.B — AnatomyInsertCommand
//
// Builder-Command der einen Insert auf AnatomyExecutionContext<C> ausfuehrt.
// Implementiert ICommand fuer einheitliche Builder-Pipeline.
//
// LEBENDIG, NICHT TOT (K10-PMAJOR-07, 2026-06-18): Diese CRUD-Command-Struktur (Insert/Lookup/Erase/Clear/
// Observe, alle ICommand) ist ein echtes GoF-Command-Muster und wird vom REGISTRIERTEN Test
// `tests/unit/test_v41_builder_anatomy_commands.cpp` (tests/unit/CMakeLists.txt:383, comdare_add_test)
// real instanziiert + round-trip-geprüft (AnatomyInsertCommandRoundtrip etc.). Der K10-Befund „grep tot"
// schloss den Test-Konsumenten aus dem grep aus → falsch-positiv; hier KEINE Entfernung, Etikett bleibt gültig.

#include "anatomy_execution_context.hpp"
#include <libs/cache_engine/builder/commands/i_command.hpp>

#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::builder::anatomy_commands {

template <ana::IsComposition Composition>
class AnatomyInsertCommand : public commands::ICommand {
public:
    AnatomyInsertCommand(AnatomyExecutionContext<Composition>& ctx,
                         std::uint64_t key, std::uint64_t value) noexcept
        : ctx_(ctx), key_(key), value_(value) {}

    [[nodiscard]] std::string_view command_name() const noexcept override {
        return "AnatomyInsertCommand";
    }

    int execute() override {
        return ctx_.insert(key_, value_) ? 0 : /*update*/ 0;
    }

private:
    AnatomyExecutionContext<Composition>& ctx_;
    std::uint64_t key_;
    std::uint64_t value_;
};

}  // namespace comdare::cache_engine::builder::anatomy_commands
