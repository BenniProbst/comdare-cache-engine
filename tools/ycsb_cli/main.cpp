// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 BEP Venture UG (Marke Comdare)
//
// ycsb_cli — CLI-Wrapper um comdare::workload_generator (REV 7 Phase C).
// Generiert YCSB-A/B/C/D/E/F Workloads + synthetic + schreibt sie nach Datei
// in 3 Output-Formaten: binary (kompakt), tsv (debuggable), json (Inspection).
//
// Usage: ycsb_cli --workload=A --num-keys=100000 --num-ops=1000000 \
//                 --output=ycsb_a.bin --format=binary

#include <comdare/workload_generator/workload_generator.hpp>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace wg = comdare::workload_generator;

namespace ycsb_cli {

enum class OutputFormat : std::uint8_t {
    Binary = 0,
    Tsv    = 1,
    Json   = 2,
};

struct CliConfig {
    wg::YcsbWorkload     workload     = wg::YcsbWorkload::C;
    std::uint64_t        num_keys     = 100000;
    std::uint64_t        num_ops      = 1000000;
    std::uint32_t        key_size     = 16;
    std::uint32_t        value_size   = 64;
    wg::KeyDistribution  key_dist     = wg::KeyDistribution::Zipfian;
    double               zipfian_theta = 0.99;
    std::uint64_t        seed         = 42;
    std::filesystem::path output       = "workload.bin";
    OutputFormat         format       = OutputFormat::Binary;
};

// ─────────────────────────────────────────────────────────────────────────────
// CLI-Parser (errno-style Returns: 0 ok, 4 invalid_argument)
// ─────────────────────────────────────────────────────────────────────────────

[[nodiscard]] int parse_workload(std::string_view s, wg::YcsbWorkload& out) noexcept {
    if (s.size() != 1) return 4;
    char c = static_cast<char>(std::toupper(s[0]));
    switch (c) {
        case 'A': out = wg::YcsbWorkload::A; return 0;
        case 'B': out = wg::YcsbWorkload::B; return 0;
        case 'C': out = wg::YcsbWorkload::C; return 0;
        case 'D': out = wg::YcsbWorkload::D; return 0;
        case 'E': out = wg::YcsbWorkload::E; return 0;
        case 'F': out = wg::YcsbWorkload::F; return 0;
        default:  return 4;
    }
}

[[nodiscard]] int parse_format(std::string_view s, OutputFormat& out) noexcept {
    if (s == "binary" || s == "bin")  { out = OutputFormat::Binary; return 0; }
    if (s == "tsv")                    { out = OutputFormat::Tsv;    return 0; }
    if (s == "json")                   { out = OutputFormat::Json;   return 0; }
    return 4;
}

[[nodiscard]] int parse_key_dist(std::string_view s, wg::KeyDistribution& out) noexcept {
    if (s == "uniform")    { out = wg::KeyDistribution::Uniform;    return 0; }
    if (s == "zipfian")    { out = wg::KeyDistribution::Zipfian;    return 0; }
    if (s == "sequential") { out = wg::KeyDistribution::Sequential; return 0; }
    if (s == "latest")     { out = wg::KeyDistribution::Latest;     return 0; }
    return 4;
}

[[nodiscard]] int parse_args(int argc, char const* const* argv, CliConfig& cfg) noexcept {
    for (int i = 1; i < argc; ++i) {
        std::string_view a{argv[i]};
        auto eq = a.find('=');
        if (eq == std::string_view::npos) {
            // Erlaube --help
            if (a == "--help" || a == "-h") return -1;
            return 4;
        }
        std::string_view key = a.substr(0, eq);
        std::string_view val = a.substr(eq + 1);

        try {
            if      (key == "--workload")      { if (parse_workload(val, cfg.workload) != 0) return 4; }
            else if (key == "--num-keys")      { cfg.num_keys = std::stoull(std::string(val)); }
            else if (key == "--num-ops")       { cfg.num_ops  = std::stoull(std::string(val)); }
            else if (key == "--key-size")      { cfg.key_size = static_cast<std::uint32_t>(std::stoul(std::string(val))); }
            else if (key == "--value-size")    { cfg.value_size = static_cast<std::uint32_t>(std::stoul(std::string(val))); }
            else if (key == "--key-dist")      { if (parse_key_dist(val, cfg.key_dist) != 0) return 4; }
            else if (key == "--zipfian-theta") { cfg.zipfian_theta = std::stod(std::string(val)); }
            else if (key == "--seed")          { cfg.seed = std::stoull(std::string(val)); }
            else if (key == "--output")        { cfg.output = std::filesystem::path{std::string(val)}; }
            else if (key == "--format")        { if (parse_format(val, cfg.format) != 0) return 4; }
            else                                return 4;
        } catch (std::exception const&) {
            return 4;
        }
    }
    return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// Output-Writer (errno-style)
// ─────────────────────────────────────────────────────────────────────────────

// Binary-Layout (kompakt):
//   uint32 magic         = 0xC0FFEE01
//   uint32 version       = 1
//   uint64 num_ops
//   foreach op:
//     uint8  op_kind
//     uint64 key_id
//     uint32 scan_length
inline constexpr std::uint32_t kBinaryMagic   = 0xC0FFEE01u;
inline constexpr std::uint32_t kBinaryVersion = 1u;

[[nodiscard]] int write_binary(std::filesystem::path const& p,
                                std::span<wg::Operation const> ops) noexcept {
    std::ofstream out{p, std::ios::binary};
    if (!out) return 10;  // io_error
    out.write(reinterpret_cast<char const*>(&kBinaryMagic),   sizeof(kBinaryMagic));
    out.write(reinterpret_cast<char const*>(&kBinaryVersion), sizeof(kBinaryVersion));
    std::uint64_t const n = ops.size();
    out.write(reinterpret_cast<char const*>(&n), sizeof(n));
    for (auto const& op : ops) {
        std::uint8_t k = static_cast<std::uint8_t>(op.op);
        out.write(reinterpret_cast<char const*>(&k),            sizeof(k));
        out.write(reinterpret_cast<char const*>(&op.key_id),    sizeof(op.key_id));
        out.write(reinterpret_cast<char const*>(&op.scan_length), sizeof(op.scan_length));
    }
    return out.good() ? 0 : 10;
}

[[nodiscard]] std::string_view op_kind_name(wg::OperationKind k) noexcept {
    switch (k) {
        case wg::OperationKind::Read:            return "read";
        case wg::OperationKind::Update:          return "update";
        case wg::OperationKind::Insert:          return "insert";
        case wg::OperationKind::Scan:            return "scan";
        case wg::OperationKind::ReadModifyWrite: return "rmw";
        case wg::OperationKind::Erase:           return "erase";
    }
    return "unknown";
}

[[nodiscard]] int write_tsv(std::filesystem::path const& p,
                             std::span<wg::Operation const> ops) noexcept {
    std::ofstream out{p};
    if (!out) return 10;
    out << "op\tkey\tscan_length\n";
    for (auto const& op : ops) {
        out << op_kind_name(op.op) << '\t' << op.key_id << '\t' << op.scan_length << '\n';
    }
    return out.good() ? 0 : 10;
}

[[nodiscard]] int write_json(std::filesystem::path const& p,
                              std::span<wg::Operation const> ops) noexcept {
    std::ofstream out{p};
    if (!out) return 10;
    out << "{\n  \"num_ops\": " << ops.size() << ",\n  \"ops\": [\n";
    bool first = true;
    for (auto const& op : ops) {
        if (!first) out << ",\n";
        first = false;
        out << "    {\"op\": \"" << op_kind_name(op.op)
            << "\", \"key\": " << op.key_id
            << ", \"scan_length\": " << op.scan_length << "}";
    }
    out << "\n  ]\n}\n";
    return out.good() ? 0 : 10;
}

// ─────────────────────────────────────────────────────────────────────────────
// Driver
// ─────────────────────────────────────────────────────────────────────────────

[[nodiscard]] int generate_and_write(CliConfig const& cfg) noexcept {
    wg::WorkloadConfig wc;
    wc.num_keys         = cfg.num_keys;
    wc.num_operations   = cfg.num_ops;
    wc.key_size_bytes   = cfg.key_size;
    wc.value_size_bytes = cfg.value_size;
    wc.key_distribution = cfg.key_dist;
    wc.zipfian_theta    = cfg.zipfian_theta;
    wc.random_seed      = cfg.seed;

    wg::WorkloadGenerator gen{wc};
    auto ops = gen.generate_ycsb(cfg.workload);

    switch (cfg.format) {
        case OutputFormat::Binary: return write_binary(cfg.output, ops);
        case OutputFormat::Tsv:    return write_tsv(cfg.output, ops);
        case OutputFormat::Json:   return write_json(cfg.output, ops);
    }
    return 4;
}

}  // namespace ycsb_cli

namespace {

void print_help() {
    std::cout
        << "ycsb_cli - YCSB-Generator-CLI fuer comdare-cache-engine\n\n"
        << "Usage: ycsb_cli [options]\n\n"
        << "Options:\n"
        << "  --workload=<A|B|C|D|E|F>       YCSB-Workload (default: C)\n"
        << "  --num-keys=<n>                 Anzahl unique Keys (default: 100000)\n"
        << "  --num-ops=<n>                  Anzahl Operationen (default: 1000000)\n"
        << "  --key-size=<bytes>             Key-Size in Byte (default: 16)\n"
        << "  --value-size=<bytes>           Value-Size in Byte (default: 64)\n"
        << "  --key-dist=<uniform|zipfian|sequential|latest>  Key-Verteilung\n"
        << "  --zipfian-theta=<float>        Zipfian-Skew 0..1 (default: 0.99)\n"
        << "  --seed=<n>                     Random-Seed (default: 42)\n"
        << "  --output=<path>                Output-Datei (default: workload.bin)\n"
        << "  --format=<binary|tsv|json>     Output-Format (default: binary)\n"
        << "  --help                         Diese Hilfe anzeigen\n";
}

}  // namespace

#ifndef YCSB_CLI_TEST_NO_MAIN
int main(int argc, char const* const* argv) {
    ycsb_cli::CliConfig cfg;
    int parse_status = ycsb_cli::parse_args(argc, argv, cfg);
    if (parse_status == -1) {
        print_help();
        return 0;
    }
    if (parse_status != 0) {
        std::cerr << "ycsb_cli: invalid arguments (status=" << parse_status << ")\n";
        print_help();
        return parse_status;
    }
    int status = ycsb_cli::generate_and_write(cfg);
    if (status != 0) {
        std::cerr << "ycsb_cli: failed to generate workload (status=" << status << ")\n";
        return status;
    }
    std::cout << "ycsb_cli: wrote " << cfg.num_ops << " ops to " << cfg.output.string() << "\n";
    return 0;
}
#endif
