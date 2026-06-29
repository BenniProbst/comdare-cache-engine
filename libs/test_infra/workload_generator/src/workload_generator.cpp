#include <comdare/workload_generator/workload_generator.hpp>

#include <cmath>
#include <stdexcept>

namespace comdare::workload_generator {

namespace {

// Pre-computed Zipfian distribution helper (REV 7 + YCSB Spec)
class ZipfianHelper {
public:
    ZipfianHelper(std::uint64_t n, double theta) : n_{n}, theta_{theta} {
        zeta_n_ = compute_zeta(n_, theta_);
        zeta_2_ = compute_zeta(2, theta_);
        alpha_  = 1.0 / (1.0 - theta_);
        eta_    = (1.0 - std::pow(2.0 / static_cast<double>(n), 1.0 - theta_)) / (1.0 - zeta_2_ / zeta_n_);
    }

    [[nodiscard]] std::uint64_t next(std::mt19937_64& gen) const noexcept {
        std::uniform_real_distribution<double> uniform{0.0, 1.0};
        double                                 u  = uniform(gen);
        double                                 uz = u * zeta_n_;
        if (uz < 1.0) return 0;
        if (uz < 1.0 + std::pow(0.5, theta_)) return 1;
        return static_cast<std::uint64_t>(static_cast<double>(n_) * std::pow(eta_ * u - eta_ + 1.0, alpha_));
    }

private:
    [[nodiscard]] static double compute_zeta(std::uint64_t n, double theta) noexcept {
        double sum = 0.0;
        for (std::uint64_t i = 1; i <= n; ++i) { sum += 1.0 / std::pow(static_cast<double>(i), theta); }
        return sum;
    }

    std::uint64_t n_;
    double        theta_;
    double        zeta_n_, zeta_2_, alpha_, eta_;
};

} // anonymous namespace

std::vector<Operation> WorkloadGenerator::generate_ycsb(YcsbWorkload workload) {
    std::mt19937_64        gen{config_.random_seed};
    std::vector<Operation> ops;
    ops.reserve(config_.num_operations);

    ZipfianHelper zipf{config_.num_keys, config_.zipfian_theta};

    for (std::uint64_t i = 0; i < config_.num_operations; ++i) {
        Operation op;
        op.op = sample_op_kind(workload, gen);

        // Key-Auswahl basierend auf Workload-Type
        switch (workload) {
            case YcsbWorkload::A:
            case YcsbWorkload::B:
            case YcsbWorkload::C:
            case YcsbWorkload::E:
            case YcsbWorkload::F: op.key_id = zipf.next(gen); break;
            case YcsbWorkload::D:
                // "latest" distribution
                op.key_id = (op.op == OperationKind::Insert) ? config_.num_keys + i : zipf.next(gen);
                break;
        }
        if (op.op == OperationKind::Scan) { op.scan_length = 1 + (gen() % 100); }
        ops.push_back(op);
    }
    return ops;
}

std::vector<Operation> WorkloadGenerator::generate_uniform_reads() {
    std::mt19937_64        gen{config_.random_seed};
    std::vector<Operation> ops;
    ops.reserve(config_.num_operations);
    for (std::uint64_t i = 0; i < config_.num_operations; ++i) {
        ops.push_back({OperationKind::Read, next_uniform_key(gen), 0});
    }
    return ops;
}

std::vector<Operation> WorkloadGenerator::generate_zipfian_reads() {
    std::mt19937_64        gen{config_.random_seed};
    ZipfianHelper          zipf{config_.num_keys, config_.zipfian_theta};
    std::vector<Operation> ops;
    ops.reserve(config_.num_operations);
    for (std::uint64_t i = 0; i < config_.num_operations; ++i) {
        ops.push_back({OperationKind::Read, zipf.next(gen), 0});
    }
    return ops;
}

std::vector<Operation> WorkloadGenerator::generate_sequential_reads() {
    std::vector<Operation> ops;
    ops.reserve(config_.num_operations);
    for (std::uint64_t i = 0; i < config_.num_operations; ++i) {
        ops.push_back({OperationKind::Read, next_sequential_key(i), 0});
    }
    return ops;
}

comdare_workload_descriptor_v1 WorkloadGenerator::to_abi_descriptor(std::span<Operation const> ops) const noexcept {
    comdare_workload_descriptor_v1 d{};
    d.version          = COMDARE_ABI_VERSION;
    d.workload_id      = config_.random_seed;
    d.num_operations   = ops.size();
    d.key_size_bytes   = config_.key_size_bytes;
    d.value_size_bytes = config_.value_size_bytes;
    d.dataset_ptr      = ops.data();
    d.dataset_bytes    = ops.size() * sizeof(Operation);
    return d;
}

std::uint64_t WorkloadGenerator::next_uniform_key(std::mt19937_64& gen) const {
    std::uniform_int_distribution<std::uint64_t> dist{0, config_.num_keys - 1};
    return dist(gen);
}

std::uint64_t WorkloadGenerator::next_zipfian_key(std::mt19937_64& gen) const {
    // Simplified: use ZipfianHelper inline (slower fuer single call)
    ZipfianHelper zipf{config_.num_keys, config_.zipfian_theta};
    return zipf.next(gen);
}

std::uint64_t WorkloadGenerator::next_sequential_key(std::uint64_t op_index) const {
    return op_index % config_.num_keys;
}

OperationKind WorkloadGenerator::sample_op_kind(YcsbWorkload workload, std::mt19937_64& gen) const {
    std::uniform_real_distribution<double> uniform{0.0, 1.0};
    double const                           r = uniform(gen);
    switch (workload) {
        case YcsbWorkload::A: return (r < 0.5) ? OperationKind::Read : OperationKind::Update;
        case YcsbWorkload::B: return (r < 0.95) ? OperationKind::Read : OperationKind::Update;
        case YcsbWorkload::C: return OperationKind::Read;
        case YcsbWorkload::D: return (r < 0.95) ? OperationKind::Read : OperationKind::Insert;
        case YcsbWorkload::E: return (r < 0.95) ? OperationKind::Scan : OperationKind::Insert;
        case YcsbWorkload::F: return (r < 0.5) ? OperationKind::Read : OperationKind::ReadModifyWrite;
    }
    return OperationKind::Read;
}

} // namespace comdare::workload_generator
