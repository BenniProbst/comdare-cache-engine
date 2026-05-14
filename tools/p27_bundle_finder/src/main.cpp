// V31.K5 (2026-05-14) — P27 Bundle Finder CLI
// Usage: p27_bundle_finder <program.dmp> <program.table> [threshold]
#include "comdare/p27/bundle_finder.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace comdare::p27 {

namespace {

// Strip leading + trailing whitespace.
std::string_view trim(std::string_view s) {
    auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) return {};
    auto last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

// Parse hex literal "0x1abc" or "1abc" (no prefix). Returns 0 on failure.
std::uint64_t parse_hex(std::string_view s) {
    if (s.starts_with("0x") || s.starts_with("0X")) s.remove_prefix(2);
    std::uint64_t value = 0;
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value, 16);
    return (ec == std::errc{}) ? value : 0;
}

// Tokenize on whitespace.
std::vector<std::string_view> split(std::string_view line) {
    std::vector<std::string_view> tokens;
    std::size_t pos = 0;
    while (pos < line.size()) {
        while (pos < line.size() && std::isspace(static_cast<unsigned char>(line[pos]))) ++pos;
        if (pos >= line.size()) break;
        std::size_t start = pos;
        while (pos < line.size() && !std::isspace(static_cast<unsigned char>(line[pos]))) ++pos;
        tokens.emplace_back(line.substr(start, pos - start));
    }
    return tokens;
}

} // namespace

std::map<std::uint64_t, FunctionEntry>
parse_symbol_table(std::string_view text) {
    std::map<std::uint64_t, FunctionEntry> result;
    std::size_t line_start = 0;
    while (line_start < text.size()) {
        std::size_t line_end = text.find('\n', line_start);
        std::string_view line = (line_end == std::string_view::npos)
            ? text.substr(line_start)
            : text.substr(line_start, line_end - line_start);
        line_start = (line_end == std::string_view::npos) ? text.size() : line_end + 1;

        // Format: addr flags section align size name [name-rest]
        // Mimic hp_soft.py: parts[0]=addr, parts[4]=size, parts[5..]=name.
        auto tokens = split(line);
        if (tokens.size() < 6) continue;
        FunctionEntry e;
        e.address = parse_hex(tokens[0]);
        e.size    = parse_hex(tokens[4]);
        if (e.size == 0) continue;
        std::string name;
        for (std::size_t i = 5; i < tokens.size(); ++i) {
            if (i > 5) name += ' ';
            name.append(tokens[i].data(), tokens[i].size());
        }
        e.name = std::move(name);
        result.emplace(e.address, std::move(e));
    }
    return result;
}

CallGraph parse_disassembly(std::string_view text) {
    CallGraph g;
    // Patterns from hp_soft.py:
    //   func_pattern: ^([0-9a-f]+) <(.+)>:$
    //   call_pattern: .*callq  ([0-9a-f]+) <.*>$
    static const std::regex func_re(R"(^([0-9a-f]+) <(.+)>:$)");
    static const std::regex call_re(R"(.*callq\s+([0-9a-f]+) <.*>$)");

    std::uint64_t current = 0;
    bool have_current = false;

    std::size_t line_start = 0;
    while (line_start < text.size()) {
        std::size_t line_end = text.find('\n', line_start);
        std::string line = std::string((line_end == std::string_view::npos)
            ? text.substr(line_start)
            : text.substr(line_start, line_end - line_start));
        line_start = (line_end == std::string_view::npos) ? text.size() : line_end + 1;

        // strip trailing CR
        if (!line.empty() && line.back() == '\r') line.pop_back();

        std::smatch m;
        if (std::regex_match(line, m, func_re)) {
            current = parse_hex(m[1].str());
            have_current = true;
            g.add_function(current, m[2].str());
        } else if (have_current && std::regex_match(line, m, call_re)) {
            std::uint64_t target = parse_hex(m[1].str());
            g.add_call(current, target);
        }
    }
    return g;
}

std::map<std::uint64_t, SubtreeMetric>
compute_subtree_metrics(const CallGraph &graph,
                        const std::map<std::uint64_t, std::uint64_t> &addr_size) {
    std::map<std::uint64_t, SubtreeMetric> result;
    const auto &edges = graph.edges();

    // Iterative DFS with visited-set per source (matches hp_soft.py
    // behaviour: visited reset for each source).
    auto dfs_for_source = [&](std::uint64_t source) -> SubtreeMetric {
        std::set<std::uint64_t> visited;
        SubtreeMetric m{};
        // Stack of nodes still to process.
        std::vector<std::uint64_t> stack;
        stack.push_back(source);
        while (!stack.empty()) {
            std::uint64_t node = stack.back();
            stack.pop_back();
            if (!visited.insert(node).second) continue;
            const auto it = edges.find(node);
            if (it == edges.end()) {
                // Leaf or unknown: count 1 + 0x20 bytes (matches Python).
                m.function_count += 1;
                m.footprint_bytes += 0x20;
                continue;
            }
            m.function_count += 1;
            const auto sz_it = addr_size.find(node);
            m.footprint_bytes += (sz_it != addr_size.end()) ? sz_it->second : 0x20;
            for (std::uint64_t child : it->second) {
                stack.push_back(child);
            }
        }
        return m;
    };

    for (const auto &[node, _] : edges) {
        result.emplace(node, dfs_for_source(node));
    }
    return result;
}

std::vector<BundleCandidate>
find_bundle_candidates(const CallGraph &graph,
                       const std::map<std::uint64_t, FunctionEntry> &symbols,
                       std::uint64_t threshold_bytes,
                       bool /*demangle*/) {
    std::map<std::uint64_t, std::uint64_t> addr_size;
    for (const auto &[addr, e] : symbols) addr_size[addr] = e.size;

    auto subtrees = compute_subtree_metrics(graph, addr_size);

    // Sort entries by footprint desc.
    std::vector<std::pair<std::uint64_t, SubtreeMetric>> sorted(subtrees.begin(), subtrees.end());
    std::sort(sorted.begin(), sorted.end(),
              [](const auto &a, const auto &b) {
                  return a.second.footprint_bytes > b.second.footprint_bytes;
              });

    std::vector<BundleCandidate> out;
    std::uint64_t task_id = 0;
    for (const auto &[addr, m] : sorted) {
        if (m.footprint_bytes < threshold_bytes) break;
        BundleCandidate c;
        c.address = addr;
        c.task_id = ++task_id;
        c.subtree = m;
        const auto sym = symbols.find(addr);
        c.size = (sym != symbols.end()) ? sym->second.size : 0;
        const auto name = graph.names().find(addr);
        c.demangled_name = (name != graph.names().end()) ? name->second
                                                          : (sym != symbols.end() ? sym->second.name : "?");
        out.push_back(std::move(c));
    }
    return out;
}

} // namespace comdare::p27

namespace {

std::string slurp(const std::string &path) {
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in) return {};
    std::ostringstream oss;
    oss << in.rdbuf();
    return oss.str();
}

void usage() {
    std::cerr << "Usage: p27_bundle_finder <program.dmp> <program.table> [threshold_bytes]\n"
              << "  Default threshold: 240000 (matches hp_soft.py)\n"
              << "  Lists functions whose call-subtree footprint exceeds threshold,\n"
              << "  sorted descending. Output: addr size [task] name subtree=(count,bytes)\n";
}

} // namespace

int main(int argc, char **argv) {
    if (argc < 3 || argc > 4) {
        usage();
        return 1;
    }
    const std::string dmp_path = argv[1];
    const std::string table_path = argv[2];
    std::uint64_t threshold = (argc == 4)
        ? std::strtoull(argv[3], nullptr, 0)
        : 240'000ull;

    auto dmp = slurp(dmp_path);
    auto table = slurp(table_path);
    if (dmp.empty()) {
        std::cerr << "p27_bundle_finder: cannot read " << dmp_path << "\n";
        return 2;
    }
    if (table.empty()) {
        std::cerr << "p27_bundle_finder: cannot read " << table_path << "\n";
        return 2;
    }

    auto symbols = comdare::p27::parse_symbol_table(table);
    auto graph = comdare::p27::parse_disassembly(dmp);
    auto candidates = comdare::p27::find_bundle_candidates(graph, symbols, threshold);

    for (const auto &c : candidates) {
        std::printf("\n0x%llx 0x%llx [0x%llx]: %-70s subtree=(%llu, %llu) ",
                    static_cast<unsigned long long>(c.address),
                    static_cast<unsigned long long>(c.size),
                    static_cast<unsigned long long>(c.task_id),
                    c.demangled_name.c_str(),
                    static_cast<unsigned long long>(c.subtree.function_count),
                    static_cast<unsigned long long>(c.subtree.footprint_bytes));
    }
    std::printf("\n");
    return 0;
}
