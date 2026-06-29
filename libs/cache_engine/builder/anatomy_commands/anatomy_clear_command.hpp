#pragma once
// V41.F.6.1.R5.B — AnatomyClearCommand

#include "anatomy_execution_context.hpp"
#include <libs/cache_engine/builder/commands/i_command.hpp>

#include <string_view>

namespace comdare::cache_engine::builder::anatomy_commands {

template <ana::IsComposition Composition>
class AnatomyClearCommand : public commands::ICommand {
public:
    explicit AnatomyClearCommand(AnatomyExecutionContext<Composition>& ctx) noexcept : ctx_(ctx) {}

    [[nodiscard]] std::string_view command_name() const noexcept override { return "AnatomyClearCommand"; }

    int execute() override {
        ctx_.clear();
        return 0;
    }

private:
    AnatomyExecutionContext<Composition>& ctx_;
};

} // namespace comdare::cache_engine::builder::anatomy_commands
