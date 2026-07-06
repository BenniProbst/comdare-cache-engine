// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 BEP Venture UG (Marke Comdare)
//
// latex_anhang — Konvertiert measurements.csv (oder .json) zur
// Diplomarbeits-LaTeX-Tabelle (Phase 8, REV 7 §8).
//
// Liest CSV aus MeasurementBuffer-Disk-Dump (Phase 7.3 ResultAggregator)
// und schreibt eine booktabs-Tabelle mit allen Permutationen + Cache-
// Metriken.

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace latex_anhang {

inline constexpr int status_ok               = 0;
inline constexpr int status_io_error         = 10;
inline constexpr int status_invalid_argument = 4;
inline constexpr int status_parse_error      = 11;

struct CsvRow {
    std::string   permutation_id;
    std::uint64_t fingerprint             = 0;
    bool          succeeded               = false;
    std::uint64_t op_count                = 0;
    std::uint64_t total_cycles            = 0;
    std::uint64_t cache_misses_l1         = 0;
    std::uint64_t cache_misses_l2         = 0;
    std::uint64_t cache_misses_l3         = 0;
    std::uint64_t dtlb_misses             = 0;
    std::uint64_t coherence_invalidations = 0;
    std::uint64_t energy_micro_joules     = 0;
    std::uint64_t bytes_allocated         = 0;
    std::uint64_t bytes_in_use_peak       = 0;
    double        external_frag           = 0.0;
    double        internal_frag           = 0.0;
};

[[nodiscard]] std::vector<std::string> split_csv_line(std::string const& line) {
    std::vector<std::string> result;
    std::string              field;
    for (char c : line) {
        if (c == ',') {
            result.push_back(std::move(field));
            field.clear();
        } else {
            field.push_back(c);
        }
    }
    result.push_back(std::move(field));
    return result;
}

[[nodiscard]] int parse_csv(std::string const& path, std::vector<CsvRow>& rows) {
    std::ifstream in{path};
    if (!in) return status_io_error;

    std::string header;
    if (!std::getline(in, header)) return status_parse_error;

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        auto cols = split_csv_line(line);
        if (cols.size() < 15) return status_parse_error;
        CsvRow r;
        try {
            r.permutation_id          = cols[0];
            r.fingerprint             = std::stoull(cols[1]);
            r.succeeded               = (cols[2] == "1");
            r.op_count                = std::stoull(cols[3]);
            r.total_cycles            = std::stoull(cols[4]);
            r.cache_misses_l1         = std::stoull(cols[5]);
            r.cache_misses_l2         = std::stoull(cols[6]);
            r.cache_misses_l3         = std::stoull(cols[7]);
            r.dtlb_misses             = std::stoull(cols[8]);
            r.coherence_invalidations = std::stoull(cols[9]);
            r.energy_micro_joules     = std::stoull(cols[10]);
            r.bytes_allocated         = std::stoull(cols[11]);
            r.bytes_in_use_peak       = std::stoull(cols[12]);
            r.external_frag           = std::stod(cols[13]);
            r.internal_frag           = std::stod(cols[14]);
        } catch (std::exception const&) { return status_parse_error; }
        rows.push_back(std::move(r));
    }
    return status_ok;
}

// Escape LaTeX-special-Characters in IDs (Underscore, Dollar, etc.)
[[nodiscard]] std::string escape_latex(std::string_view s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '_': out += "\\_"; break;
            case '&': out += "\\&"; break;
            case '%': out += "\\%"; break;
            case '$': out += "\\$"; break;
            case '#': out += "\\#"; break;
            case '{': out += "\\{"; break;
            case '}': out += "\\}"; break;
            case '~': out += "\\textasciitilde{}"; break;
            case '^': out += "\\textasciicircum{}"; break;
            case '\\': out += "\\textbackslash{}"; break;
            default: out += c;
        }
    }
    return out;
}

[[nodiscard]] int write_latex(std::string const& path, std::vector<CsvRow> const& rows, std::string const& caption_text,
                              std::string const& label) {
    std::ofstream out{path};
    if (!out) return status_io_error;

    // booktabs-Style Tabelle
    out << "% AUTO-GENERATED durch latex_anhang (Phase 8, COMDARE)\n";
    out << "\\begin{table}[!htbp]\n";
    out << "\\centering\n";
    out << "\\small\n";
    out << "\\begin{tabular}{lrrrrr}\n";
    out << "\\toprule\n";
    out << "Permutation & ops & cycles & L1-misses & L2-misses & L3-misses \\\\\n";
    out << "\\midrule\n";

    for (auto const& r : rows) {
        if (!r.succeeded) continue;
        out << escape_latex(r.permutation_id) << " & " << r.op_count << " & " << r.total_cycles << " & "
            << r.cache_misses_l1 << " & " << r.cache_misses_l2 << " & " << r.cache_misses_l3 << " \\\\\n";
    }

    out << "\\bottomrule\n";
    out << "\\end{tabular}\n";
    out << "\\caption{" << escape_latex(caption_text) << "}%\n";
    out << "\\label{" << label << "}\n";
    out << "\\end{table}\n";

    return out.good() ? status_ok : status_io_error;
}

[[nodiscard]] int write_latex_full_metrics(std::string const& path, std::vector<CsvRow> const& rows,
                                           std::string const& caption_text, std::string const& label) {
    std::ofstream out{path};
    if (!out) return status_io_error;

    out << "% AUTO-GENERATED durch latex_anhang (Phase 8, COMDARE)\n";
    out << "\\begin{table}[!htbp]\n";
    out << "\\centering\n";
    out << "\\scriptsize\n";
    out << "\\begin{tabular}{lrrrrrrr}\n";
    out << "\\toprule\n";
    out << "Permutation & ops & cycles & L1 & L2 & L3 & dTLB & coherence \\\\\n";
    out << "\\midrule\n";

    for (auto const& r : rows) {
        out << escape_latex(r.permutation_id) << " & " << r.op_count << " & " << r.total_cycles << " & "
            << r.cache_misses_l1 << " & " << r.cache_misses_l2 << " & " << r.cache_misses_l3 << " & " << r.dtlb_misses
            << " & " << r.coherence_invalidations << " \\\\\n";
    }

    out << "\\bottomrule\n";
    out << "\\end{tabular}\n";
    out << "\\caption{" << escape_latex(caption_text) << "}%\n";
    out << "\\label{" << label << "}\n";
    out << "\\end{table}\n";

    return out.good() ? status_ok : status_io_error;
}

} // namespace latex_anhang

namespace {

void print_help() {
    std::cout << "latex_anhang - Konvertiert measurements.csv zu LaTeX-Tabelle\n\n"
              << "Usage: latex_anhang --input=<csv> --output=<tex> [options]\n\n"
              << "Options:\n"
              << "  --input=<path>       measurements.csv (Pflicht)\n"
              << "  --output=<path>      Ziel .tex-Datei (Pflicht)\n"
              << "  --caption=<text>     LaTeX-Caption (default: \"COMDARE Permutations\")\n"
              << "  --label=<text>       LaTeX-Label (default: tab:comdare:perms)\n"
              << "  --full               Schreibt erweiterte Metriken-Tabelle (mit dTLB+coherence)\n"
              << "  --help               Diese Hilfe\n";
}

} // namespace

#ifndef LATEX_ANHANG_TEST_NO_MAIN
int main(int argc, char const* const* argv) {
    std::string input_path;
    std::string output_path;
    std::string caption = "COMDARE Permutations Measurement Records";
    std::string label   = "tab:comdare:perms";
    bool        full    = false;

    for (int i = 1; i < argc; ++i) {
        std::string_view a{argv[i]};
        if (a == "--help" || a == "-h") {
            print_help();
            return 0;
        }
        if (a == "--full") {
            full = true;
            continue;
        }
        auto eq = a.find('=');
        if (eq == std::string_view::npos) {
            std::cerr << "Unknown arg: " << a << "\n";
            print_help();
            return 4;
        }
        std::string_view k = a.substr(0, eq);
        std::string_view v = a.substr(eq + 1);
        if (k == "--input")
            input_path = std::string(v);
        else if (k == "--output")
            output_path = std::string(v);
        else if (k == "--caption")
            caption = std::string(v);
        else if (k == "--label")
            label = std::string(v);
        else {
            std::cerr << "Unknown flag: " << k << "\n";
            return 4;
        }
    }
    if (input_path.empty() || output_path.empty()) {
        std::cerr << "Missing --input or --output\n";
        print_help();
        return 4;
    }

    std::vector<latex_anhang::CsvRow> rows;
    int                               s = latex_anhang::parse_csv(input_path, rows);
    if (s != 0) {
        std::cerr << "latex_anhang: CSV-Parse-Error (status=" << s << ")\n";
        return s;
    }

    s = full ? latex_anhang::write_latex_full_metrics(output_path, rows, caption, label)
             : latex_anhang::write_latex(output_path, rows, caption, label);
    if (s != 0) {
        std::cerr << "latex_anhang: Write-Error (status=" << s << ")\n";
        return s;
    }
    std::cout << "latex_anhang: " << rows.size() << " rows -> " << output_path << "\n";
    return 0;
}
#endif
