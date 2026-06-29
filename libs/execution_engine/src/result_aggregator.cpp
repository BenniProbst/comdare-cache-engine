#include <comdare/experiment/result_aggregator.hpp>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace comdare::experiment {

void ResultAggregator::add(PermutationResult result) { results_.push_back(std::move(result)); }

void ResultAggregator::set_baseline(std::string baseline_id) { baseline_id_ = std::move(baseline_id); }

std::optional<PermutationResult> ResultAggregator::find_baseline() const {
    auto it = std::find_if(results_.begin(), results_.end(),
                           [this](auto const& r) { return r.permutation_id == baseline_id_; });
    if (it == results_.end()) return std::nullopt;
    return *it;
}

std::vector<ComparisonReport> ResultAggregator::compare_against_baseline() const {
    auto baseline = find_baseline();
    if (!baseline) return {};

    std::vector<ComparisonReport> reports;
    reports.reserve(results_.size());

    for (auto const& candidate : results_) {
        if (candidate.permutation_id == baseline_id_) continue;
        if (!candidate.succeeded || !baseline->succeeded) continue;

        ComparisonReport r;
        r.baseline_id  = baseline_id_;
        r.candidate_id = candidate.permutation_id;

        // Ratios: candidate/baseline
        double const base_cycles = static_cast<double>(baseline->record.total_cycles);
        double const cand_cycles = static_cast<double>(candidate.record.total_cycles);
        r.throughput_speedup     = (cand_cycles > 0) ? (base_cycles / cand_cycles) : 1.0;

        double const base_mem = static_cast<double>(baseline->record.bytes_in_use_peak);
        double const cand_mem = static_cast<double>(candidate.record.bytes_in_use_peak);
        r.memory_ratio        = (base_mem > 0) ? (cand_mem / base_mem) : 1.0;

        double const base_ops = static_cast<double>(baseline->record.op_count);
        double const cand_ops = static_cast<double>(candidate.record.op_count);
        r.latency_ratio = (base_ops > 0 && cand_ops > 0) ? (cand_cycles / cand_ops) / (base_cycles / base_ops) : 1.0;

        double const base_miss = static_cast<double>(baseline->record.cache_misses_l1);
        double const cand_miss = static_cast<double>(candidate.record.cache_misses_l1);
        r.cache_miss_ratio     = (base_miss > 0) ? (cand_miss / base_miss) : 1.0;

        reports.push_back(r);
    }
    return reports;
}

void ResultAggregator::export_csv(std::filesystem::path const& path) const {
    std::ofstream out{path};
    if (!out) throw std::runtime_error{"Could not open " + path.string()};

    // V20.3 — workload_used als zusaetzliche Spalte (zwischen succeeded und op_count)
    out << "permutation_id,fingerprint,succeeded,workload_used,op_count,total_cycles,"
        << "cache_misses_l1,cache_misses_l2,cache_misses_l3,dtlb_misses,"
        << "coherence_invalidations,energy_micro_joules,"
        << "bytes_allocated,bytes_in_use_peak,external_frag,internal_frag\n";

    for (auto const& r : results_) {
        out << r.permutation_id << ',' << r.fingerprint << ',' << (r.succeeded ? 1 : 0) << ',' << r.workload_used << ','
            << r.record.op_count << ',' << r.record.total_cycles << ',' << r.record.cache_misses_l1 << ','
            << r.record.cache_misses_l2 << ',' << r.record.cache_misses_l3 << ',' << r.record.dtlb_misses << ','
            << r.record.coherence_invalidations << ',' << r.record.energy_micro_joules << ','
            << r.record.bytes_allocated << ',' << r.record.bytes_in_use_peak << ',' << r.record.external_fragmentation
            << ',' << r.record.internal_fragmentation << '\n';
    }
}

void ResultAggregator::export_json(std::filesystem::path const& path) const {
    std::ofstream out{path};
    if (!out) throw std::runtime_error{"Could not open " + path.string()};

    out << "{\n  \"baseline_id\": \"" << baseline_id_ << "\",\n";
    out << "  \"results\": [\n";
    bool first = true;
    for (auto const& r : results_) {
        if (!first) out << ",\n";
        first = false;
        out << "    {\n"
            << "      \"permutation_id\": \"" << r.permutation_id << "\",\n"
            << "      \"fingerprint\": " << r.fingerprint << ",\n"
            << "      \"succeeded\": " << (r.succeeded ? "true" : "false") << ",\n"
            << "      \"workload_used\": \"" << r.workload_used << "\",\n"
            << "      \"op_count\": " << r.record.op_count << ",\n"
            << "      \"total_cycles\": " << r.record.total_cycles << ",\n"
            << "      \"bytes_in_use_peak\": " << r.record.bytes_in_use_peak << "\n"
            << "    }";
    }
    out << "\n  ]\n}\n";
}

} // namespace comdare::experiment
