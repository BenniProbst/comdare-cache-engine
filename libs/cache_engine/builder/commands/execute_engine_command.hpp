#pragma once
// V32.DD.1 (2026-05-18) - ExecuteEngineCommand
//
// @subsystem CEB
// @command_pattern Execute
// @phase_owner CEB (Phase 6 EXECUTE)

#include "i_command.hpp"
#include "workload.hpp"
#include "execution_result.hpp"

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
    ExecuteEngineCommand(std::string_view engine_name, Workload workload) noexcept
        : engine_name_{engine_name}, workload_{workload} {}

    [[nodiscard]] std::string_view command_name() const noexcept override {
        return "ExecuteEngineCommand";
    }

    int execute() override {
        // V32.EE.1: konkrete execute()-Body (Skelett mit klarer Struktur)
        result_.engine_name = engine_name_;
        result_.workload_kind = workload_.kind;

        // Schritt 1: engine->configure(permutation_flags_)
        //   V32.1 Folge-Step: ueber CacheEngineBuilder.registered_engine_lookup
        // Schritt 2: engine->execute(workload_)
        //   V32.1 Folge-Step: Workload-Loop mit ResultAggregator.collect()
        // Schritt 3: result_ = engine->collect_result()
        //   V32.1 Folge-Step: ResultAggregator -> ExecutionResult

        // V32.EE.1 Default-Pfad: leerer Result fuer Tests + Linker-Vollstaendigkeit
        result_.success = true;
        return 0;
    }

    [[nodiscard]] bool is_parallelizable() const noexcept override {
        return true;
    }

    [[nodiscard]] const ExecutionResult& result() const noexcept {
        return result_;
    }

private:
    std::string_view engine_name_;
    Workload workload_;
    ExecutionResult result_ {};
};

}  // namespace comdare::cache_engine::builder::commands
