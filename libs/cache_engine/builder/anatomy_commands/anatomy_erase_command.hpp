#pragma once
// V41.F.6.1.R5.B — AnatomyEraseCommand

#include "anatomy_execution_context.hpp"
#include <libs/cache_engine/builder/commands/i_command.hpp>

#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::builder::anatomy_commands {

template <ana::IsComposition Composition>
class AnatomyEraseCommand : public commands::ICommand {
public:
    AnatomyEraseCommand(AnatomyExecutionContext<Composition>& ctx, std::uint64_t key) noexcept
        : ctx_(ctx), key_(key), erased_(false) {}

    [[nodiscard]] std::string_view command_name() const noexcept override { return "AnatomyEraseCommand"; }

    int execute() override {
        erased_ = ctx_.erase(key_);
        return erased_ ? 0 : 1;
    }

    [[nodiscard]] bool erased() const noexcept { return erased_; }

private:
    AnatomyExecutionContext<Composition>& ctx_;
    std::uint64_t                         key_;
    bool                                  erased_;
};

} // namespace comdare::cache_engine::builder::anatomy_commands
