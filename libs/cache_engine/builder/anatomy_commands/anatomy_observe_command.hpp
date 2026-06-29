#pragma once
// V41.F.6.1.R5.B — AnatomyObserveCommand
//
// Liest den ObserverAggregate-Snapshot ueber AnatomyExecutionContext und
// speichert ihn in last_snapshot_. Pflicht-Command fuer CacheEngineBuilder
// Mess-Treiber (Welch-t-Test auf Snapshot-Sample-Reihen).

#include "anatomy_execution_context.hpp"
#include <libs/cache_engine/builder/commands/i_command.hpp>

#include <string_view>

namespace comdare::cache_engine::builder::anatomy_commands {

template <ana::IsComposition Composition>
class AnatomyObserveCommand : public commands::ICommand {
public:
    using snapshot_t = typename AnatomyExecutionContext<Composition>::observer_aggregate_t;

    explicit AnatomyObserveCommand(AnatomyExecutionContext<Composition> const& ctx) noexcept
        : ctx_(ctx), last_snapshot_{} {}

    [[nodiscard]] std::string_view command_name() const noexcept override { return "AnatomyObserveCommand"; }

    int execute() override {
        last_snapshot_ = ctx_.observe_all();
        return 0;
    }

    [[nodiscard]] snapshot_t const& last_snapshot() const noexcept { return last_snapshot_; }

private:
    AnatomyExecutionContext<Composition> const& ctx_;
    snapshot_t                                  last_snapshot_;
};

} // namespace comdare::cache_engine::builder::anatomy_commands
