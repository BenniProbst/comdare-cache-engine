#pragma once
// V32.DD.1 (2026-05-18) - ExecuteEngineCommand
//
// @subsystem CEB
// @command_pattern Execute
// @phase_owner CEB (Phase 6 EXECUTE)

#include "i_command.hpp"
#include "../../include/cache_engine/abi/execution_engine.hpp"
#include "../../include/cache_engine/concepts/permutation_flags.hpp"

#include <memory>

namespace comdare::cache_engine::builder::commands {

/**
 * @brief ExecuteEngineCommand - fuehrt eine ExecutionEngine auf einer Permutation aus
 * @subsystem CEB
 * @command_pattern Execute
 * @phase_owner CEB
 *
 * Wird vom CacheEngineBuilder pro ExecutionEngine (EE-A = CacheEngine, EE-B = PrtArt)
 * pro Permutation instanziiert + ausgefuehrt. Resultat wird in result_ gespeichert
 * und vom CompareEngineCommand verglichen.
 *
 * Beispiel-Nutzung (DD.1+DD.3):
 *   auto cmd_a = std::make_unique<ExecuteEngineCommand>(ee_a, permutation, workload);
 *   auto cmd_b = std::make_unique<ExecuteEngineCommand>(ee_b, permutation, workload);
 *   cmd_a->execute();
 *   cmd_b->execute();
 *   auto cmp = CompareEngineCommand(cmd_a->result(), cmd_b->result());
 */
class ExecuteEngineCommand : public ICommand {
public:
    // Forward-Declarations - konkrete Typen in workload.hpp + execution_result.hpp folgen
    // Sub-Module-Spezifikation siehe Z.1 §1 + AA.2

    [[nodiscard]] std::string_view command_name() const noexcept override {
        return "ExecuteEngineCommand";
    }

    int execute() override {
        // V32.DD.1 Skelett - konkrete Implementierung in V32.1 Sprint:
        // 1. engine_->configure(permutation_flags_)
        // 2. engine_->execute(workload_)
        // 3. result_ = engine_->collect_result()
        return 0;
    }

    [[nodiscard]] bool is_parallelizable() const noexcept override {
        // ExecuteEngineCommand ist parallelisierbar pro ExecutionEngine-Instanz
        // (auf der gleichen Permutation - daher CompareEngineCommand danach)
        return true;
    }

    // Getter fuer Resultat (wird von CompareEngineCommand konsumiert)
    // V32.1 Sprint: konkrete result-Typen
};

}  // namespace comdare::cache_engine::builder::commands
