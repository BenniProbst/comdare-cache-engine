#pragma once
// IStrategyCommand — Wurzel jeder atomaren Strategie (Command-Pattern)
// Termin 7 / 10_korrektur §K3.4 + 22_architektur_skizze_REV5 §2.4

#include <cstdint>
#include <string>

namespace comdare::cache_engine::strategy_command {

struct StrategyContext {
    std::uint64_t module_id    = 0;
    void*         user_payload = nullptr;
};

struct CommandResult {
    bool         success       = true;
    std::int64_t output_metric = 0;
    std::string  diagnostic;
};

class IStrategyCommand {
public:
    virtual ~IStrategyCommand() = default;

    [[nodiscard]] virtual std::string   name() const                  = 0;
    [[nodiscard]] virtual CommandResult execute(StrategyContext& ctx) = 0;

    // Composition-Vertraeglichkeit (z.B. CSS-PointerElimination + DenseLayout vertraeglich)
    [[nodiscard]] virtual bool can_compose_with(IStrategyCommand const& other) const noexcept = 0;
};

} // namespace comdare::cache_engine::strategy_command
