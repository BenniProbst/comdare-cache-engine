#pragma once
// Querschnitt M -- SystemAxis-Wurzel fuer host-seitige Messachsen.
//
// Keine ABI-Erweiterung: diese Header-only-Wurzel dockt nur lesend an die bestehenden
// Host-PODs (ComdareTierObserverSnapshot, PmcCounters) an.

#include <cache_engine/measurement/measurement_category.hpp>

#include "../../../anatomy/observable_tier.hpp"
#include <cache_engine/measurement/pmc_source.hpp>
#include "../../../topics/axis.hpp"

#include <array>
#include <concepts>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::measurement {

enum class MeasurementRegime : std::uint8_t {
    TimeObserver,
    PmcCounter,
};

inline constexpr std::array<MeasurementCategory, 16> kAllMeasurementCategories{
    MeasurementCategory::CLU,           MeasurementCategory::CACHE_MISS_L1,
    MeasurementCategory::CACHE_MISS_L2, MeasurementCategory::CACHE_MISS_L3,
    MeasurementCategory::DTLB_MISS,     MeasurementCategory::MEMORY_FOOTPRINT,
    MeasurementCategory::BRANCH_MISS,   MeasurementCategory::IPC_CPI,
    MeasurementCategory::LATENCY_MEAN,  MeasurementCategory::LATENCY_P50,
    MeasurementCategory::LATENCY_P95,   MeasurementCategory::LATENCY_P99,
    MeasurementCategory::LATENCY_P999,  MeasurementCategory::THROUGHPUT,
    MeasurementCategory::ENERGY_J,      MeasurementCategory::FILL_BUFFER_OCCUPANCY,
};

inline constexpr std::array<MeasurementCategory, 7> kPmcCounterCategories{
    MeasurementCategory::CACHE_MISS_L1, MeasurementCategory::CACHE_MISS_L2, MeasurementCategory::CACHE_MISS_L3,
    MeasurementCategory::DTLB_MISS,     MeasurementCategory::BRANCH_MISS,   MeasurementCategory::IPC_CPI,
    MeasurementCategory::ENERGY_J,
};

inline constexpr std::array<MeasurementCategory, 9> kTimeObserverCategories{
    MeasurementCategory::CLU,
    MeasurementCategory::MEMORY_FOOTPRINT,
    MeasurementCategory::LATENCY_MEAN,
    MeasurementCategory::LATENCY_P50,
    MeasurementCategory::LATENCY_P95,
    MeasurementCategory::LATENCY_P99,
    MeasurementCategory::LATENCY_P999,
    MeasurementCategory::THROUGHPUT,
    MeasurementCategory::FILL_BUFFER_OCCUPANCY,
};

[[nodiscard]] constexpr MeasurementRegime regime_of(MeasurementCategory category) noexcept {
    switch (category) {
        // Thesis 03_messsystem_prtart.tex:382-386: zeit-/observer-basierte Kategorien laufen mit eigener
        // Apparatur immer; 06_evaluation_methodology.tex:119-126: nicht vom privilegierten PMC-Regime gated.
        case MeasurementCategory::CLU:
        case MeasurementCategory::MEMORY_FOOTPRINT:
        case MeasurementCategory::LATENCY_MEAN:
        case MeasurementCategory::LATENCY_P50:
        case MeasurementCategory::LATENCY_P95:
        case MeasurementCategory::LATENCY_P99:
        case MeasurementCategory::LATENCY_P999:
        case MeasurementCategory::THROUGHPUT:
        case MeasurementCategory::FILL_BUFFER_OCCUPANCY: return MeasurementRegime::TimeObserver;

        // Thesis 03_messsystem_prtart.tex:382-386: PMC-/zaehlerbasierte Kategorien sind getrennt;
        // 06_evaluation_methodology.tex:119-126 nennt Cache-, dTLB-, Branch-Misses und IPC/CPI.
        // ENERGY_J ist code-verankert ueber PmcCounters::energy_micro_joules.
        case MeasurementCategory::CACHE_MISS_L1:
        case MeasurementCategory::CACHE_MISS_L2:
        case MeasurementCategory::CACHE_MISS_L3:
        case MeasurementCategory::DTLB_MISS:
        case MeasurementCategory::BRANCH_MISS:
        case MeasurementCategory::IPC_CPI:
        case MeasurementCategory::ENERGY_J: return MeasurementRegime::PmcCounter;
    }
    return MeasurementRegime::TimeObserver;
}

namespace detail {

[[nodiscard]] constexpr bool contains(std::array<MeasurementCategory, 7> const& values,
                                      MeasurementCategory                       category) noexcept {
    for (auto const value : values) {
        if (value == category) return true;
    }
    return false;
}

[[nodiscard]] constexpr bool contains(std::array<MeasurementCategory, 9> const& values,
                                      MeasurementCategory                       category) noexcept {
    for (auto const value : values) {
        if (value == category) return true;
    }
    return false;
}

[[nodiscard]] constexpr bool regime_mapping_is_complete() noexcept {
    std::size_t pmc_count           = 0;
    std::size_t time_observer_count = 0;
    for (auto const category : kAllMeasurementCategories) {
        MeasurementRegime const regime = regime_of(category);
        if (regime == MeasurementRegime::PmcCounter) {
            if (!contains(kPmcCounterCategories, category)) return false;
            ++pmc_count;
        } else {
            if (!contains(kTimeObserverCategories, category)) return false;
            ++time_observer_count;
        }
    }
    return pmc_count == kPmcCounterCategories.size() && time_observer_count == kTimeObserverCategories.size() &&
           (pmc_count + time_observer_count) == kAllMeasurementCategories.size();
}

template <class Derived>
[[nodiscard]] consteval MeasurementRegime regime_for_axis() {
    constexpr auto categories = Derived::do_categories();
    static_assert(categories.size() > 0, "SystemAxis braucht mindestens eine MeasurementCategory");
    MeasurementRegime const regime = regime_of(categories[0]);
    for (auto const category : categories) {
        if (regime_of(category) != regime) throw "SystemAxis darf keine MeasurementRegimes mischen";
    }
    return regime;
}

} // namespace detail

static_assert(kAllMeasurementCategories.size() == 16);
static_assert(detail::regime_mapping_is_complete(),
              "Jede MeasurementCategory muss genau einem MeasurementRegime zugeordnet sein");

struct SystemAxisSample {
    MeasurementCategory category = MeasurementCategory::CLU;
    std::uint64_t       value    = 0;
    bool                valid    = false;
};

static_assert(std::is_standard_layout_v<SystemAxisSample>);
static_assert(std::is_trivially_copyable_v<SystemAxisSample>);

/// System-Achsen sind bei eingeschalteter Messung immer host-seitig praesent (Blut-Direktive), unabhaengig von
/// Tier-Permutation und E2/E3-Baum. Sie sind keine Organ-Taxonomie und kein austauschbarer Achsen-Slot.
[[nodiscard]] constexpr bool system_axes_always_present() noexcept { return true; }

template <class Derived>
struct SystemAxis : topics::Axis<Derived> {
    [[nodiscard]] static constexpr topics::AxisKind axis_kind() noexcept {
        return topics::AxisKind::system_measurement;
    }
    [[nodiscard]] static constexpr auto              categories() noexcept { return Derived::do_categories(); }
    [[nodiscard]] static constexpr MeasurementRegime regime() noexcept { return detail::regime_for_axis<Derived>(); }

    [[nodiscard]] constexpr bool available() const noexcept {
        if constexpr (requires(Derived const& derived) {
                          { derived.do_available() } -> std::convertible_to<bool>;
                      }) {
            return static_cast<bool>(derived().do_available());
        } else {
            return true;
        }
    }

    constexpr void collect(SystemAxisSample& sample) const noexcept {
        if (!available()) {
            invalidate(sample);
            return;
        }
        derived().do_collect(sample);
    }

protected:
    constexpr SystemAxis() noexcept = default;

    static constexpr void invalidate(SystemAxisSample& sample) noexcept {
        sample.value = 0;
        sample.valid = false;
    }

private:
    [[nodiscard]] constexpr Derived const& derived() const noexcept { return static_cast<Derived const&>(*this); }
};

template <class A>
concept SystemAxisConcept =
    topics::AxisConcept<A> && std::derived_from<A, SystemAxis<A>> && std::is_empty_v<SystemAxis<A>> &&
    (!std::is_polymorphic_v<SystemAxis<A>>) && requires(A const& axis, SystemAxisSample& sample) {
        { A::categories() };
        { A::regime() } -> std::same_as<MeasurementRegime>;
        { axis.available() } -> std::same_as<bool>;
        { axis.collect(sample) } -> std::same_as<void>;
    };

struct WallClockSystemAxis final : SystemAxis<WallClockSystemAxis> {
    std::int64_t  total_ns = 0;
    std::uint64_t op_count = 0;

    constexpr WallClockSystemAxis(std::int64_t total_ns_, std::uint64_t op_count_) noexcept
        : total_ns(total_ns_), op_count(op_count_) {}

    [[nodiscard]] static constexpr auto do_categories() noexcept {
        return std::array{MeasurementCategory::LATENCY_MEAN, MeasurementCategory::THROUGHPUT};
    }

    constexpr void do_collect(SystemAxisSample& sample) const noexcept {
        switch (sample.category) {
            case MeasurementCategory::LATENCY_MEAN:
                if (op_count == 0 || total_ns < 0) {
                    invalidate(sample);
                    return;
                }
                sample.value = static_cast<std::uint64_t>(total_ns) / op_count;
                sample.valid = true;
                return;

            case MeasurementCategory::THROUGHPUT:
                if (total_ns <= 0) {
                    invalidate(sample);
                    return;
                }
                sample.value = static_cast<std::uint64_t>((static_cast<long double>(op_count) * 1'000'000'000.0L) /
                                                          static_cast<long double>(total_ns));
                sample.valid = true;
                return;

            case MeasurementCategory::LATENCY_P50:
            case MeasurementCategory::LATENCY_P95:
            case MeasurementCategory::LATENCY_P99:
            case MeasurementCategory::LATENCY_P999:
                // honest-0: total_ns/op_count ist ein MITTELWERT — ihn als Perzentil zu etikettieren
                // waere ein Phantomwert. Echte Perzentile liefert das HdrHistogramm (AP-8/#242).
                invalidate(sample);
                return;

            default: invalidate(sample); return;
        }
    }
};

struct ObserverSnapshotSystemAxis final : SystemAxis<ObserverSnapshotSystemAxis> {
    using snapshot_t = ::comdare::cache_engine::anatomy::ComdareTierObserverSnapshot;

    snapshot_t const* snapshot = nullptr;

    explicit constexpr ObserverSnapshotSystemAxis(snapshot_t const& snapshot_) noexcept : snapshot(&snapshot_) {}

    [[nodiscard]] static constexpr auto do_categories() noexcept { return std::array{MeasurementCategory::CLU}; }

    constexpr void do_collect(SystemAxisSample& sample) const noexcept {
        if (snapshot == nullptr) {
            invalidate(sample);
            return;
        }

        switch (sample.category) {
            case MeasurementCategory::CLU: {
                // Cache-Line-AUSLASTUNG (Thesis 03:383) = field_bytes / (cache_lines * 64), hier als Prozent.
                // Der rohe cache_lines-Zaehler waere die INVERSE Metrik (Review wf_f1604ba3, CONFIRMED-major).
                std::uint64_t const field_bytes = snapshot->axis_stats[5][2];
                std::uint64_t const cache_lines = snapshot->axis_stats[5][3];
                if (cache_lines == 0) {
                    invalidate(sample);
                    return;
                }
                sample.value = (field_bytes * 100u) / (cache_lines * 64u);
                sample.valid = true;
                return;
            }
            case MeasurementCategory::MEMORY_FOOTPRINT:
                // honest-0: der Thesis-Kanon verlangt bytes_in_use_peak (05_evaluation.tex:94-95); das T6-Schema
                // traegt nur den Momentanwert bytes_in_use — als Footprint etikettiert verzerrte er CoW-lastige
                // Layouts (Peak >> Endstand). Bis eine peak-Spalte (golden-neutraler END-Append) existiert: invalid.
            case MeasurementCategory::FILL_BUFFER_OCCUPANCY:
                // honest-0: T17 queuing_q1.peak_size ist ein SOFTWARE-Operations-Puffer; die Thesis-Kategorie meint
                // Hardware-Line-Fill-Buffer (P25 Mahling, 03_state_of_the_art:543) — physisch unverwandt, kein Proxy.
            default: invalidate(sample); return;
        }
    }
};

// Compile-time-Bindung an den Schema-Vertrag (kV3AxisSchema IST der Vertrag — keine stille Index-Drift).
static_assert(std::string_view{::comdare::cache_engine::anatomy::kV3AxisSchema[5].names[2]} == "field_bytes");
static_assert(std::string_view{::comdare::cache_engine::anatomy::kV3AxisSchema[5].names[3]} == "cache_lines");

struct PmcSystemAxis final : SystemAxis<PmcSystemAxis> {
    using counters_t = ::comdare::cache_engine::measurement::PmcCounters;

    counters_t const* counters = nullptr;

    explicit constexpr PmcSystemAxis(counters_t const& counters_) noexcept : counters(&counters_) {}

    [[nodiscard]] static constexpr auto do_categories() noexcept { return kPmcCounterCategories; }

    [[nodiscard]] constexpr bool do_available() const noexcept { return counters != nullptr && counters->available; }

    constexpr void do_collect(SystemAxisSample& sample) const noexcept {
        if (!do_available()) {
            invalidate(sample);
            return;
        }

        switch (sample.category) {
            case MeasurementCategory::CACHE_MISS_L1:
                sample.value = counters->cache_misses_l1;
                sample.valid = true;
                return;
            case MeasurementCategory::CACHE_MISS_L2:
                sample.value = counters->cache_misses_l2;
                sample.valid = true;
                return;
            case MeasurementCategory::CACHE_MISS_L3:
                sample.value = counters->cache_misses_l3;
                sample.valid = true;
                return;
            case MeasurementCategory::DTLB_MISS:
                sample.value = counters->dtlb_misses;
                sample.valid = true;
                return;
            case MeasurementCategory::BRANCH_MISS:
                sample.value = counters->branch_misses;
                sample.valid = true;
                return;
            case MeasurementCategory::ENERGY_J:
                sample.value = counters->energy_micro_joules;
                sample.valid = true;
                return;
            case MeasurementCategory::IPC_CPI:
            default:
                // Das aktuelle PmcCounters-POD enthaelt keine instructions/cycles-Spalten. Kein Phantomwert.
                invalidate(sample);
                return;
        }
    }
};

static_assert(SystemAxisConcept<WallClockSystemAxis>);
static_assert(SystemAxisConcept<ObserverSnapshotSystemAxis>);
static_assert(SystemAxisConcept<PmcSystemAxis>);

} // namespace comdare::cache_engine::measurement
