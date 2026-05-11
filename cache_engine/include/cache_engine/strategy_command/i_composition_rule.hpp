#pragma once
// ICompositionRule + 4 konkrete Rules (Sequential/Parallel/Conditional/Recursive)
// Termin 7 / 10_korrektur §K3.4 + 22_architektur_skizze_REV5 §2.4

#include <cache_engine/strategy_command/i_strategy_command.hpp>

#include <cstdint>
#include <vector>

namespace comdare::cache_engine::strategy_command {

enum class CompositionRuleKind : std::uint8_t {
    Sequential  = 0,
    Parallel    = 1,
    Conditional = 2,
    Recursive   = 3,
};

struct ExecutionPlan {
    std::vector<std::size_t> ordered_indices;   // Reihenfolge der parts[i]
    bool                     parallelizable = false;
};

class ICompositionRule {
public:
    virtual ~ICompositionRule() = default;

    [[nodiscard]] virtual CompositionRuleKind kind() const noexcept = 0;

    // Plant Ausfuehrungsreihenfolge der Sub-Commands.
    [[nodiscard]] virtual ExecutionPlan
    applies(std::vector<IStrategyCommand*> const& parts,
            StrategyContext const& ctx) const = 0;
};

class SequentialRule final : public ICompositionRule {
public:
    [[nodiscard]] CompositionRuleKind kind() const noexcept override {
        return CompositionRuleKind::Sequential;
    }
    [[nodiscard]] ExecutionPlan
    applies(std::vector<IStrategyCommand*> const& parts,
            StrategyContext const&) const override {
        ExecutionPlan p{};
        p.parallelizable = false;
        p.ordered_indices.reserve(parts.size());
        for (std::size_t i = 0; i < parts.size(); ++i) p.ordered_indices.push_back(i);
        return p;
    }
};

class ParallelRule final : public ICompositionRule {
public:
    [[nodiscard]] CompositionRuleKind kind() const noexcept override {
        return CompositionRuleKind::Parallel;
    }
    [[nodiscard]] ExecutionPlan
    applies(std::vector<IStrategyCommand*> const& parts,
            StrategyContext const&) const override {
        ExecutionPlan p{};
        p.parallelizable = true;
        p.ordered_indices.reserve(parts.size());
        for (std::size_t i = 0; i < parts.size(); ++i) p.ordered_indices.push_back(i);
        return p;
    }
};

class ConditionalRule final : public ICompositionRule {
public:
    using Predicate = bool (*)(StrategyContext const&);

    explicit ConditionalRule(Predicate pred) : predicate_(pred) {}

    [[nodiscard]] CompositionRuleKind kind() const noexcept override {
        return CompositionRuleKind::Conditional;
    }
    [[nodiscard]] ExecutionPlan
    applies(std::vector<IStrategyCommand*> const& parts,
            StrategyContext const& ctx) const override {
        ExecutionPlan p{};
        p.parallelizable = false;
        if (parts.empty()) return p;
        // Bei wahrem Praedikat: erstes Command, sonst zweites (falls vorhanden)
        bool cond = predicate_ ? predicate_(ctx) : true;
        std::size_t idx = (cond || parts.size() == 1) ? 0u : 1u;
        if (idx < parts.size()) p.ordered_indices.push_back(idx);
        return p;
    }

private:
    Predicate predicate_;
};

class RecursiveRule final : public ICompositionRule {
public:
    explicit RecursiveRule(std::size_t depth_limit = 4) : depth_limit_(depth_limit) {}

    [[nodiscard]] CompositionRuleKind kind() const noexcept override {
        return CompositionRuleKind::Recursive;
    }
    [[nodiscard]] ExecutionPlan
    applies(std::vector<IStrategyCommand*> const& parts,
            StrategyContext const&) const override {
        ExecutionPlan p{};
        p.parallelizable = false;
        // Rekursive Komposition: jedes Command ggf. mehrfach (vereinfacht: depth_limit-mal)
        for (std::size_t d = 0; d < depth_limit_; ++d) {
            for (std::size_t i = 0; i < parts.size(); ++i) p.ordered_indices.push_back(i);
        }
        return p;
    }

    [[nodiscard]] std::size_t depth_limit() const noexcept { return depth_limit_; }

private:
    std::size_t depth_limit_;
};

}  // namespace comdare::cache_engine::strategy_command
