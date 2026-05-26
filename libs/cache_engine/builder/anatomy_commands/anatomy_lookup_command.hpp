#pragma once
// V41.F.6.1.R5.B — AnatomyLookupCommand

#include "anatomy_execution_context.hpp"
#include <libs/cache_engine/builder/commands/i_command.hpp>

#include <cstdint>
#include <optional>
#include <string_view>

namespace comdare::cache_engine::builder::anatomy_commands {

template <ana::IsComposition Composition>
class AnatomyLookupCommand : public commands::ICommand {
public:
    AnatomyLookupCommand(AnatomyExecutionContext<Composition> const& ctx,
                         std::uint64_t key) noexcept
        : ctx_(ctx), key_(key), result_(std::nullopt) {}

    [[nodiscard]] std::string_view command_name() const noexcept override {
        return "AnatomyLookupCommand";
    }

    int execute() override {
        result_ = ctx_.lookup(key_);
        return result_.has_value() ? 0 : 1;
    }

    [[nodiscard]] std::optional<std::uint64_t> result() const noexcept { return result_; }

private:
    AnatomyExecutionContext<Composition> const& ctx_;
    std::uint64_t key_;
    std::optional<std::uint64_t> result_;
};

}  // namespace comdare::cache_engine::builder::anatomy_commands
