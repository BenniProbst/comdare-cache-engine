// V41.F.6.1.R6.A — WorkloadGenerator Implementation
//
// @task #715 V41.F.6.1.R6.A

#include "workload_generator.hpp"

#include <cmath>
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

    // Op-Mix normalisieren (Summe -> 1.0) — inkl. Scan/RMW (V5-#49-E/F)
    double const sum =
        config_.pct_insert + config_.pct_lookup +
        config_.pct_erase  + config_.pct_clear  +
        config_.pct_scan   + config_.pct_rmw;
    config_.pct_insert /= sum;
    config_.pct_lookup /= sum;
    config_.pct_erase  /= sum;
    config_.pct_clear  /= sum;
    config_.pct_scan   /= sum;
    config_.pct_rmw    /= sum;

    // CDF aufbauen (kumulative Schwellenwerte fuer Op-Sampling; Reihenfolge Insert→Lookup→Erase→Clear→Scan→RMW)
    cdf_insert_ = config_.pct_insert;
    cdf_lookup_ = cdf_insert_ + config_.pct_lookup;
    cdf_erase_  = cdf_lookup_ + config_.pct_erase;
    cdf_clear_  = cdf_erase_  + config_.pct_clear;
    cdf_scan_   = cdf_clear_  + config_.pct_scan;
    cdf_rmw_    = 1.0;  // Pflicht-Anker (Summe normalisiert)

    // YCSB-Treue (#49): Zipfian-Konstanten vorberechnen, falls Zipfian/Latest gewählt.
    if (config_.key_distribution != KeyDistribution::Uniform) precompute_zipfian();
}

// ─────────────────────────────────────────────────────────────────────────────
// Zipfian-Vorberechnung (Gray, Sundaresan, Englert, Baclawski, Weinberger, SIGMOD 1994;
// das von YCSB genutzte schnelle Inverse-CDF-Verfahren). #49 Pfad A (YCSB-Treue).
// ─────────────────────────────────────────────────────────────────────────────

void WorkloadGenerator::precompute_zipfian() noexcept {
    zipf_n_     = config_.key_max - config_.key_min + 1ULL;
    zipf_theta_ = (config_.zipfian_theta > 0.0 && config_.zipfian_theta != 1.0) ? config_.zipfian_theta : 0.99;
    // zetan = sum_{i=1}^{n} 1/i^theta (O(n) einmalig); zeta2 = 1 + 0.5^theta.
    double zetan = 0.0;
    for (std::uint64_t i = 1; i <= zipf_n_; ++i)
        zetan += 1.0 / std::pow(static_cast<double>(i), zipf_theta_);
    zipf_zetan_    = zetan;
    zipf_half_pow_ = std::pow(0.5, zipf_theta_);
    zipf_zeta2_    = 1.0 + zipf_half_pow_;
    zipf_alpha_    = 1.0 / (1.0 - zipf_theta_);
    zipf_eta_      = (1.0 - std::pow(2.0 / static_cast<double>(zipf_n_), 1.0 - zipf_theta_))
                   / (1.0 - zipf_zeta2_ / zipf_zetan_);
}

std::uint64_t WorkloadGenerator::sample_zipf_rank() noexcept {
    // Inverse-CDF nach Gray et al.: liefert einen Rang in [0, n) mit P(rang) ∝ 1/(rang+1)^theta.
    double const u  = next_unit();
    double const uz = u * zipf_zetan_;
    if (uz < 1.0)                 return 0;
    if (uz < 1.0 + zipf_half_pow_) return 1;
    auto rank = static_cast<std::uint64_t>(
        static_cast<double>(zipf_n_) * std::pow(zipf_eta_ * u - zipf_eta_ + 1.0, zipf_alpha_));
    if (rank >= zipf_n_) rank = zipf_n_ - 1;   // Numerik-Clamp
    return rank;
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
    if (r < cdf_clear_)  return WorkloadOpKind::Clear;
    if (r < cdf_scan_)   return WorkloadOpKind::Scan;             // V5-#49-E
    return WorkloadOpKind::ReadModifyWrite;  // r in [cdf_scan_, 1.0)   V5-#49-F
}

// ─────────────────────────────────────────────────────────────────────────────
// Key-Sampling aus [key_min, key_max] uniform
// ─────────────────────────────────────────────────────────────────────────────

double WorkloadGenerator::next_unit() noexcept {
    return static_cast<double>(next_random()) / (static_cast<double>(static_cast<std::uint64_t>(-1)) + 1.0);
}

std::uint64_t WorkloadGenerator::sample_key() noexcept {
    auto const range = config_.key_max - config_.key_min + 1ULL;
    switch (config_.key_distribution) {
        case KeyDistribution::Zipfian: {
            // Zipf-Rang [0,n) → Key. Heißester Rang 0 ↦ key_min (stabile, reproduzierbare Zuordnung).
            return config_.key_min + (sample_zipf_rank() % range);
        }
        case KeyDistribution::Latest: {
            // YCSB-D „read latest": Zipf-Skew auf die HÖCHSTEN Keys (zuletzt eingefügt) → key_max - rang.
            return config_.key_max - (sample_zipf_rank() % range);
        }
        case KeyDistribution::Uniform:
        default:
            // Modulo-Bias akzeptabel fuer Mess-Reihen (range << 2^64)
            return config_.key_min + (next_random() % range);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

WorkloadOp WorkloadGenerator::next() noexcept {
    auto const kind  = sample_op_kind();
    auto       k     = sample_key();
    // CoCo-Trie-Direktive (P04, QUERY_NOT_IN_SET_PERCENTAGE): mit Wahrsch. negative_query_pct zielt ein
    // Lookup/Scan auf einen NICHT-geladenen Key ([key_max+1, 2*key_max]) → garantierter Miss (Negativ-Query).
    // Nur Read-Queries (Insert/Erase/RMW betreffen den Schreib-Pfad). Deterministisch je (Config, Seed).
    if (config_.negative_query_pct > 0.0 &&
        (kind == WorkloadOpKind::Lookup || kind == WorkloadOpKind::Scan)) {
        if (next_unit() < (config_.negative_query_pct / 100.0)) {
            auto const range = config_.key_max - config_.key_min + 1ULL;
            k = config_.key_max + 1ULL + (next_random() % range);   // außerhalb [key_min, key_max] = nie geladen
        }
    }
    auto const v_raw = next_random();  // value (immer gezogen, hält den PRNG-State synchron / reproduzierbar)
    // V5-#49-E: für Scan-Ops kodiert `value` die Scan-Länge in [1, scan_length_max] (YCSB-E, uniform).
    std::uint64_t const v = (kind == WorkloadOpKind::Scan)
        ? (1ULL + (v_raw % (config_.scan_length_max == 0 ? 1ULL : config_.scan_length_max)))
        : v_raw;
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
