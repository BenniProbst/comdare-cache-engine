#pragma once
// Measure<Cat, Detail> constexpr-Template — F1 Mess-Matrix
// Termin 7 / 03_uml_measurement §1 + F-EXTRA-7 HasMeasurement-Concept

#include <cache_engine/measurement/measurement_category.hpp>

#include <concepts>

namespace comdare::cache_engine::measurement {

struct alignas(64) Context {
    std::uint64_t cache_lines_used = 0;
    std::uint64_t cache_misses_l1  = 0;
    std::uint64_t cache_misses_l2  = 0;
    std::uint64_t cache_misses_l3  = 0;
    std::uint64_t dtlb_misses      = 0;
    std::uint64_t branch_misses    = 0;
    std::uint64_t lookup_start_ns  = 0;
    std::uint64_t lookup_end_ns    = 0;
};

// Primary template — Default-Hook (F-EXTRA-7), keine Wirkung
template <MeasurementCategory Cat, AlgoDetail Detail>
struct Measure {
    static constexpr void at_lookup_begin(Context&) noexcept {}
    static constexpr void at_lookup_end(Context&) noexcept {}
    static constexpr void at_node_visit(std::uint64_t, Context&) noexcept {}
    static constexpr double extract(Context const&) noexcept { return 0.0; }
};

// Beispiel-Spezialisierung: CLU x ART_NODE256 (Termin 7 / 03_uml_measurement §1)
template <>
struct Measure<MeasurementCategory::CLU, AlgoDetail::ART_NODE256> {
    static constexpr void at_lookup_begin(Context&) noexcept {}
    static constexpr void at_lookup_end(Context&) noexcept {}
    static constexpr void at_node_visit(std::uint64_t used_bytes, Context& ctx) noexcept {
        // CLU = effektiv genutzte Bytes pro Cache-Line (64 B)
        ctx.cache_lines_used += used_bytes / 64;
    }
    static constexpr double extract(Context const& ctx) noexcept {
        return ctx.cache_lines_used > 0 ? 1.0 : 0.0;
    }
};

// Beispiel: LATENCY_MEAN x beliebig — generisch Lookup-Begin/End-Zeitstempel
template <AlgoDetail Detail>
struct Measure<MeasurementCategory::LATENCY_MEAN, Detail> {
    static constexpr void at_lookup_begin(Context& ctx) noexcept {
        // In echtem Build: rdtsc/clock_gettime; hier nur als Pflichtsignatur
        ctx.lookup_start_ns = 0;
    }
    static constexpr void at_lookup_end(Context& ctx) noexcept {
        ctx.lookup_end_ns = 0;
    }
    static constexpr void at_node_visit(std::uint64_t, Context&) noexcept {}
    static constexpr double extract(Context const& ctx) noexcept {
        return static_cast<double>(ctx.lookup_end_ns - ctx.lookup_start_ns);
    }
};

// F-EXTRA-7 — HasMeasurement-Concept (Algo-spezifische Override-Erkennung)
template <typename AlgoTag, MeasurementCategory Cat>
concept HasMeasurement = requires {
    // Hat eine Spezialisierung von Measure<Cat, AlgoTag::detail>
    { Measure<Cat, AlgoTag::detail>::extract(std::declval<Context&>()) } -> std::convertible_to<double>;
};

// F11 Trigger-Modus
enum class MeasurementTrigger : std::uint8_t {
    CONTINUOUS  = 0,   // Builder-Modus, jeder Lookup
    SAMPLED_1_N = 1,   // Production, default 1:1000
};

class MeasurementHooks {
public:
    explicit MeasurementHooks(MeasurementTrigger t = MeasurementTrigger::CONTINUOUS,
                              std::size_t sample_n = 1000)
        : trigger_(t), sample_n_(sample_n) {}

    // Liefert true falls dieser Lookup gemessen werden soll
    [[nodiscard]] bool should_record() noexcept {
        if (trigger_ == MeasurementTrigger::CONTINUOUS) return true;
        ++sample_counter_;
        return (sample_counter_ % sample_n_) == 0;
    }

    [[nodiscard]] MeasurementTrigger trigger()  const noexcept { return trigger_; }
    [[nodiscard]] std::size_t        sample_n() const noexcept { return sample_n_; }

private:
    MeasurementTrigger trigger_;
    std::size_t        sample_n_;
    std::size_t        sample_counter_ = 0;
};

}  // namespace comdare::cache_engine::measurement
