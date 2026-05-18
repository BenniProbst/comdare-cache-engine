#pragma once
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

}  // namespace comdare::cache_engine::builder::commands
