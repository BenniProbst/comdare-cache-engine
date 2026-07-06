#pragma once
// CMD-1 (c) GEPARKT (#267, 2026-07-06): dieser Header bleibt als dokumentierte V32-AUSNAHME —
// include/cache_engine/abi/cache_engine_execution_engine_adapter.hpp (ABI-Flaeche, V32-Demo-Pfad,
// default OFF) konsumiert execute_engine_command (und damit i_command als Basis) HART. Die uebrigen
// Command-Inseln (compare_engine/auto_permutate/anatomy_*_command/strategy_command/algorithm_visitor)
// wurden mit 0-Konsumenten-Beweis entfernt; Nachfolger der Command-Semantik = compile-time AxisCommand
// (topics/axis_command_base.hpp, CMD-1 a/b).
// V32.DD.1 (2026-05-18) - Command-Pattern Basis fuer CacheEngineBuilder Test-Treiber
//
// Korrektur AA.2: CacheEngineBuilder orchestriert BEIDE ExecutionEngines
// (CacheEngine als EE-A + PRT-ART als EE-B) parallel via Command-Pattern.
// Siehe docs/architektur/10_schichten_modell_M.md §0 AA.2 Korrektur.
//
// @subsystem CEB
// @phase_owner CEB
// @command_pattern Base

#include <memory>
#include <string_view>

namespace comdare::cache_engine::builder::commands {

/**
 * @brief ICommand - Abstract base fuer alle CEB-Commands (V32.DD.1 NEU)
 * @subsystem CEB
 * @command_pattern Base
 *
 * Command-Pattern fuer Test-Treiber CacheEngineBuilder.
 * Konkrete Subklassen: ExecuteEngineCommand, CompareEngineCommand, AutoPermutateAxisCommand.
 */
class ICommand {
public:
    virtual ~ICommand() = default;

    /// Eindeutiger Name des Commands fuer Logging + Telemetrie
    [[nodiscard]] virtual std::string_view command_name() const noexcept = 0;

    /// Fuehrt den Command aus, return-Code 0 = OK, >0 = Fehler-Code
    virtual int execute() = 0;

    /// Optional: kann der Command parallel zu anderen ausgefuehrt werden?
    [[nodiscard]] virtual bool is_parallelizable() const noexcept { return false; }
};

} // namespace comdare::cache_engine::builder::commands
