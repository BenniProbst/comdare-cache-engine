// V31.K5 (2026-05-14) — P27 Bundle Finder Library Header
// C++23-Port von hp_soft.py (T. Zhang et al. 2025, ASPLOS Hierarchical
// Prefetching). Identifiziert Funktionen, deren Subtree-Footprint einen
// Threshold ueberschreitet -- diese sind "potential entry points of
// Bundles" fuer Hierarchical-Bundle-Prefetcher.
#pragma once

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::p27 {

struct FunctionEntry {
    std::uint64_t address = 0;
    std::uint64_t size = 0;
    std::string   name;
};

struct SubtreeMetric {
    std::uint64_t function_count = 0;
    std::uint64_t footprint_bytes = 0;
};

struct BundleCandidate {
    std::uint64_t address;
    std::uint64_t size;
    std::string   demangled_name;
    SubtreeMetric subtree;
    std::uint64_t task_id;  // 1-based, sortiert nach footprint desc
};

class CallGraph {
public:
    void add_function(std::uint64_t addr, std::string name) {
        addr_name_.emplace(addr, std::move(name));
        if (!call_graph_.contains(addr)) {
            call_graph_[addr] = {};
        }
    }

    void add_call(std::uint64_t from, std::uint64_t to) {
        call_graph_[from].insert(to);
    }

    [[nodiscard]] const std::map<std::uint64_t, std::set<std::uint64_t>> &
    edges() const noexcept { return call_graph_; }

    [[nodiscard]] const std::map<std::uint64_t, std::string> &
    names() const noexcept { return addr_name_; }

    [[nodiscard]] std::size_t size() const noexcept { return call_graph_.size(); }

private:
    std::map<std::uint64_t, std::set<std::uint64_t>> call_graph_;
    std::map<std::uint64_t, std::string> addr_name_;
};

// Loads a symbol table produced by `objdump -t <binary>`.
// Returns map address -> FunctionEntry.
std::map<std::uint64_t, FunctionEntry>
parse_symbol_table(std::string_view text);

// Loads a disassembly produced by `objdump -d <binary>` and builds a
// CallGraph by detecting `callq <addr> <name>` instructions.
CallGraph parse_disassembly(std::string_view text);

// DFS from each node, summing subtree sizes and footprints.
// Cycle protection via visited-set per source. addr_size from symbol
// table (default 0x20 if absent — matches hp_soft.py).
std::map<std::uint64_t, SubtreeMetric>
compute_subtree_metrics(const CallGraph &graph,
                        const std::map<std::uint64_t, std::uint64_t> &addr_size);

// Returns BundleCandidates with subtree.footprint_bytes >= threshold,
// sorted by footprint desc (matches hp_soft.py output ordering).
std::vector<BundleCandidate>
find_bundle_candidates(const CallGraph &graph,
                       const std::map<std::uint64_t, FunctionEntry> &symbols,
                       std::uint64_t threshold_bytes,
                       bool demangle = false);

} // namespace comdare::p27
