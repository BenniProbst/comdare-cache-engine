#pragma once
// V41.F.6.1.R6.A — WorkloadGenerator (deterministische xorshift64-PRNG-Sequenz)
//
// Pflicht-Reproduzierbarkeit (User-Direktive):
// Zwei Generator-Instanzen mit gleichem WorkloadConfig liefern IDENTISCHE
// Op-Sequenz. Pro Permutations-Binary wird der Generator NEU instantiiert,
// produziert exakt dieselbe Sequenz, der Mess-Treiber vergleicht Latenzen
// pro Sequenz-Index.
//
// PRNG-Wahl: xorshift64 (Marsaglia 2003).
// - Period 2^64 - 1
// - Stateless beyond uint64_t state
// - Deterministisch + reproducible
// - Schneller als std::mt19937 (kein Cache-Pressure)
// - KEINE crypto-Sicherheit — nur fuer Mess-Reihen
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §53
// @task #715 V41.F.6.1.R6.A

#include "workload_config.hpp"

#include <cstdint>
#include <stdexcept>
#include <vector>

namespace comdare::cache_engine::builder::workload_driver {

// ─────────────────────────────────────────────────────────────────────────────
// WorkloadGenerator — deterministische Op-Sequenz-Generator
// ─────────────────────────────────────────────────────────────────────────────

class WorkloadGenerator {
public:
    /// Konstruktor — validiert WorkloadConfig (wirft std::invalid_argument
    /// wenn is_valid() false). Normalisiert op-mix wenn Summe != 1.0.
    explicit WorkloadGenerator(WorkloadConfig config);

    // Non-copyable, movable (verhindert versehentliche State-Duplikation)
    WorkloadGenerator(WorkloadGenerator const&)            = delete;
    WorkloadGenerator& operator=(WorkloadGenerator const&) = delete;
    WorkloadGenerator(WorkloadGenerator&&)                 noexcept = default;
    WorkloadGenerator& operator=(WorkloadGenerator&&)      noexcept = default;
    ~WorkloadGenerator()                                   = default;

    /// next() — liefert die naechste Operation der Sequenz.
    /// Side-effect: advance des xorshift64-State.
    [[nodiscard]] WorkloadOp next() noexcept;

    /// reset() — Reset auf Seed-Initial-State. Nachfolgende next()-Aufrufe
    /// liefern wieder die identische Sequenz vom Anfang.
    void reset() noexcept;

    /// generate_all() — eager-Generierung aller `config().num_operations`
    /// Ops in einen Vector. Reset wird nicht aufgerufen (caller-Verantwortung).
    [[nodiscard]] std::vector<WorkloadOp> generate_all();

    /// remaining() — wie viele Ops sind noch zu generieren bis num_operations
    /// erreicht ist. Nicht-bindend (next() kann ueberlaufen wenn ueberzogen).
    [[nodiscard]] std::size_t remaining() const noexcept;

    /// generated_count() — Anzahl bereits via next() generierter Ops.
    [[nodiscard]] std::size_t generated_count() const noexcept { return generated_; }

    /// Read-only Zugriff auf normalisierte Config (op_mix evtl. angepasst).
    [[nodiscard]] WorkloadConfig const& config() const noexcept { return config_; }

private:
    /// xorshift64 PRNG (Marsaglia 2003). Period 2^64 - 1.
    [[nodiscard]] std::uint64_t next_random() noexcept;

    /// next_random als double in [0.0, 1.0).
    [[nodiscard]] double next_unit() noexcept;

    /// Op-Kind aus normalisiertem op-mix sampeln.
    [[nodiscard]] WorkloadOpKind sample_op_kind() noexcept;

    /// Key sampeln gemaess config_.key_distribution (Uniform / Zipfian / Latest, YCSB-Treue #49).
    [[nodiscard]] std::uint64_t sample_key() noexcept;

    /// Zipf-Rang [0, n) via Gray-et-al.-SIGMOD-1994-Verfahren (das von YCSB genutzte schnelle Inverse-CDF).
    [[nodiscard]] std::uint64_t sample_zipf_rank() noexcept;

    /// Zipfian-Konstanten (zetan/eta/...) bei Zipfian/Latest im Konstruktor vorberechnen.
    void precompute_zipfian() noexcept;

    WorkloadConfig config_;
    std::uint64_t  state_;        ///< xorshift64-State
    std::size_t    generated_;    ///< Counter
    // Cumulative-Distribution-Function fuer Op-Sampling (4 thresholds)
    double cdf_insert_{};
    double cdf_lookup_{};
    double cdf_erase_{};
    double cdf_clear_{};
    // Zipfian-Vorberechnung (Gray et al. 1994; nur bei Zipfian/Latest besetzt). YCSB-Treue (#49).
    std::uint64_t zipf_n_{1};     ///< Anzahl Items = key_max - key_min + 1
    double zipf_theta_{0.99};
    double zipf_zetan_{};         ///< sum_{i=1}^{n} 1/i^theta
    double zipf_zeta2_{};         ///< 1 + 0.5^theta
    double zipf_eta_{};
    double zipf_alpha_{};         ///< 1/(1-theta)
    double zipf_half_pow_{};      ///< 0.5^theta
};

}  // namespace comdare::cache_engine::builder::workload_driver
