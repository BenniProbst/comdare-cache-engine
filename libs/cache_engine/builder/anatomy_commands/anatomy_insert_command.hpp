#pragma once
// V41.F.6.1.R5.B — AnatomyInsertCommand
//
// Builder-Command der einen Insert auf AnatomyExecutionContext<C> ausfuehrt.
// Implementiert ICommand (GoF-Command).
//
// G5-Audit-KORREKTUR (w289llo0o, PMAJOR-07): der Anspruch „einheitliche Builder-Pipeline" ist GESTRICHEN — der
// PRODUKTIONS-Mess-Pfad (drive_tier_observe_trace_abi / drive_two_phase_tier_trace_abi, perm_runner, profile_run_entry)
// ruft die ABI-vtable der geladenen Tier-Binary DIREKT und umgeht diese Command-Objekte. Diese CRUD-Commands sind also
// KEINE einheitliche Pipeline über den Mess-Pfad, sondern ein in sich gültiges GoF-Command-Muster, das nur über den
// REGISTRIERTEN Test `tests/unit/test_v41_builder_anatomy_commands.cpp` (tests/unit/CMakeLists.txt:383, comdare_add_test)
// real instanziiert + round-trip-geprüft wird (AnatomyInsertCommandRoundtrip etc.). LEBENDIG (kein toter Code; der K10-
// „grep tot"-Befund schloss den Test-Konsumenten aus → falsch-positiv) — aber NICHT der Mess-Produktionspfad.

#include "anatomy_execution_context.hpp"
#include <libs/cache_engine/builder/commands/i_command.hpp>

#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::builder::anatomy_commands {

template <ana::IsComposition Composition>
class AnatomyInsertCommand : public commands::ICommand {
public:
    AnatomyInsertCommand(AnatomyExecutionContext<Composition>& ctx, std::uint64_t key, std::uint64_t value) noexcept
        : ctx_(ctx), key_(key), value_(value) {}

    [[nodiscard]] std::string_view command_name() const noexcept override { return "AnatomyInsertCommand"; }

    int execute() override { return ctx_.insert(key_, value_) ? 0 : /*update*/ 0; }

private:
    AnatomyExecutionContext<Composition>& ctx_;
    std::uint64_t                         key_;
    std::uint64_t                         value_;
};

} // namespace comdare::cache_engine::builder::anatomy_commands
