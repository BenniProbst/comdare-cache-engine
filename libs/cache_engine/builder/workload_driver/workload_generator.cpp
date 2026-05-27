// V41.F.6.1.R6.A — WorkloadGenerator Implementation
//
// @task #715 V41.F.6.1.R6.A

#include "workload_generator.hpp"

#include <stdexcept>

namespace comdare::cache_engine::builder::workload_driver {

// ─────────────────────────────────────────────────────────────────────────────
// Konstruktor: Validation + Normalisierung + State-Init
// ─────────────────────────────────────────────────────────────────────────────

WorkloadGenerator::WorkloadGenerator(WorkloadConfig config)
    : config_{config}
    , state_{config.seed}
    , generated_{0}
{
    if (!config.is_valid()) {
        throw std::invalid_argument{
            "WorkloadGenerator: WorkloadConfig is invalid "
            "(seed=0, num_operations=0, key_max<=key_min, "
            "negative or zero-sum op-mix?)"};
    }

    // Op-Mix normalisieren (Summe -> 1.0)
    double const sum =
        config_.pct_insert + config_.pct_lookup +
        config_.pct_erase  + config_.pct_clear;
    config_.pct_insert /= sum;
    config_.pct_lookup /= sum;
    config_.pct_erase  /= sum;
    config_.pct_clear  /= sum;

    // CDF aufbauen (kumulative Schwellenwerte fuer Op-Sampling)
    cdf_insert_ = config_.pct_insert;
    cdf_lookup_ = cdf_insert_ + config_.pct_lookup;
    cdf_erase_  = cdf_lookup_ + config_.pct_erase;
    cdf_clear_  = 1.0;  // Pflicht-Anker (Summe normalisiert)
}

// ─────────────────────────────────────────────────────────────────────────────
// xorshift64 PRNG (Marsaglia 2003)
// ─────────────────────────────────────────────────────────────────────────────

std::uint64_t WorkloadGenerator::next_random() noexcept {
    // xorshift64: deterministisch + reproducible, period 2^64 - 1
    std::uint64_t x = state_;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    state_ = x;
    return x;
}

// ─────────────────────────────────────────────────────────────────────────────
// Op-Kind via CDF-Sampling
// ─────────────────────────────────────────────────────────────────────────────

WorkloadOpKind WorkloadGenerator::sample_op_kind() noexcept {
    // [0.0, 1.0) uniform via (uint64 / 2^64)
    auto const r =
        static_cast<double>(next_random()) /
        static_cast<double>(static_cast<std::uint64_t>(-1));

    if (r < cdf_insert_) return WorkloadOpKind::Insert;
    if (r < cdf_lookup_) return WorkloadOpKind::Lookup;
    if (r < cdf_erase_)  return WorkloadOpKind::Erase;
    return WorkloadOpKind::Clear;  // r in [cdf_erase_, 1.0)
}

// ─────────────────────────────────────────────────────────────────────────────
// Key-Sampling aus [key_min, key_max] uniform
// ─────────────────────────────────────────────────────────────────────────────

std::uint64_t WorkloadGenerator::sample_key() noexcept {
    auto const range = config_.key_max - config_.key_min + 1ULL;
    // Modulo-Bias akzeptabel fuer Mess-Reihen (range << 2^64)
    return config_.key_min + (next_random() % range);
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

WorkloadOp WorkloadGenerator::next() noexcept {
    auto const kind = sample_op_kind();
    auto const k    = sample_key();
    auto const v    = next_random();  // value (nur fuer Insert relevant, aber
                                       // immer generiert um State-Sync zu halten)
    ++generated_;
    return WorkloadOp{kind, k, v};
}

void WorkloadGenerator::reset() noexcept {
    state_     = config_.seed;
    generated_ = 0;
}

std::vector<WorkloadOp> WorkloadGenerator::generate_all() {
    std::vector<WorkloadOp> out;
    out.reserve(config_.num_operations);
    for (std::size_t i = 0; i < config_.num_operations; ++i) {
        out.push_back(next());
    }
    return out;
}

std::size_t WorkloadGenerator::remaining() const noexcept {
    return (generated_ < config_.num_operations)
        ? (config_.num_operations - generated_)
        : 0;
}

}  // namespace comdare::cache_engine::builder::workload_driver
