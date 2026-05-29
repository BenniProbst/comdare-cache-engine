// SPDX-License-Identifier: Apache-2.0
// V41.F.6.1 R5.D — comdare-f15-compare: CLI-Treiber fuer den F15-Messlauf ueber materialisierte
// Permutations-DLLs.
//
// Verpackt die in tests/unit/test_v41_anatomy_adhoc_autobuilt_load.cpp bewiesene Treiber-Kette als
// echtes Kommandozeilen-Tool: ein Verzeichnis voller dlopen-faehiger Anatomie-DLLs (z.B. die vom
// R5.G-Auto-Emitter gebauten Permutationen) wird geladen, jede DLL via IMeasurableWorkload::run_workload
// (Stufe B, in-DLL) gemessen, gegen eine Baseline verglichen (Welch + Holm-FWER) und als Summary +
// optional CSV/JSON ausgegeben. Beantwortet die F15-Kernfrage "bringt die CacheEngine messbaren Wert?".
//
// Aufruf:
//   comdare-f15-compare <dll-dir> [--alpha=0.05] [--baseline=0] [--ops=2000] [--batches=128]
//                                 [--seed=11] [--csv=out.csv] [--json=out.json]

#include <builder/anatomy_module_loader/anatomy_module_loader.hpp>
#include <anatomy/anatomy_base.hpp>
#include <anatomy/measurable_workload.hpp>
#include <builder/commands/multi_compare.hpp>
#include <builder/commands/result_aggregator.hpp>

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace loader = ::comdare::cache_engine::builder::anatomy_loader;
namespace ana    = ::comdare::cache_engine::anatomy;
namespace cmd    = ::comdare::cache_engine::builder::commands;
namespace stats  = ::comdare::cache_engine::builder::commands::stats;

namespace {

void print_usage() {
    std::cerr
        << "Usage: comdare-f15-compare <dll-dir> [options]\n"
        << "  <dll-dir>   Verzeichnis mit Anatomie-Permutations-DLLs (comdare_anatomy_perm_*)\n"
        << "Optionen:\n"
        << "  --alpha=F      Signifikanz-Niveau (FWER), Default 0.05\n"
        << "  --baseline=N   Index der Baseline-DLL, Default 0\n"
        << "  --ops=N        ops_per_batch, Default 2000\n"
        << "  --batches=N    Batches (= Latenz-Samples pro DLL), Default 128\n"
        << "  --seed=N       RNG-Seed, Default 11\n"
        << "  --csv=PATH     Report zusaetzlich als CSV schreiben\n"
        << "  --json=PATH    Report zusaetzlich als JSON schreiben\n";
}

bool parse_flag_u64(std::string_view arg, std::string_view key, std::uint64_t& out) {
    if (!arg.starts_with(key)) return false;
    arg.remove_prefix(key.size());
    std::uint64_t v{};
    auto const* end = arg.data() + arg.size();
    auto [p, ec] = std::from_chars(arg.data(), end, v);
    if (ec == std::errc{} && p == end) { out = v; return true; }
    return false;
}

bool parse_flag_double(std::string_view arg, std::string_view key, double& out) {
    if (!arg.starts_with(key)) return false;
    out = std::strtod(std::string{arg.substr(key.size())}.c_str(), nullptr);
    return true;
}

bool parse_flag_str(std::string_view arg, std::string_view key, std::string& out) {
    if (!arg.starts_with(key)) return false;
    out = std::string{arg.substr(key.size())};
    return true;
}

bool write_text_file(std::string const& path, std::string const& content) {
    std::ofstream os(path, std::ios::binary | std::ios::trunc);
    if (!os) return false;
    os.write(content.data(), static_cast<std::streamsize>(content.size()));
    return os.good();
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) { print_usage(); return 1; }
    std::filesystem::path const dll_dir{argv[1]};

    double        alpha    = 0.05;
    std::uint64_t baseline = 0, ops = 2000, batches = 128, seed = 11;
    std::string   csv_path, json_path;
    for (int i = 2; i < argc; ++i) {
        std::string_view a{argv[i]};
        std::uint64_t u{};
        if      (parse_flag_double(a, "--alpha=", alpha))     {}
        else if (parse_flag_u64(a, "--baseline=", u))         { baseline = u; }
        else if (parse_flag_u64(a, "--ops=", u))              { ops = u; }
        else if (parse_flag_u64(a, "--batches=", u))          { batches = u; }
        else if (parse_flag_u64(a, "--seed=", u))             { seed = u; }
        else if (parse_flag_str(a, "--csv=", csv_path))       {}
        else if (parse_flag_str(a, "--json=", json_path))     {}
        else { std::cerr << "Unbekannte Option: " << a << "\n"; print_usage(); return 1; }
    }
    if (batches == 0) batches = 1;

    std::vector<loader::AnatomyModuleHandle> handles;
    int const st = loader::AnatomyModuleLoader::load_all(dll_dir, handles);
    if (st != loader::status_ok) {
        std::cerr << "load_all fehlgeschlagen: " << loader::status_name(st) << " (dir=" << dll_dir << ")\n";
        return 2;
    }
    if (handles.size() < 2) {
        std::cerr << "Brauche >= 2 DLLs fuer einen Vergleich (geladen: " << handles.size() << ")\n";
        return 3;
    }

    // Jede geladene DLL messen → ExecutionResult. names reserviert (string_view-Stabilitaet!).
    std::vector<std::string>          names;   names.reserve(handles.size());
    std::vector<cmd::ExecutionResult> results; results.reserve(handles.size());
    for (std::size_t i = 0; i < handles.size(); ++i) {
        auto* mw = dynamic_cast<ana::IMeasurableWorkload*>(handles[i].anatomy());
        if (mw == nullptr) { std::cerr << "DLL " << i << " nicht mess-faehig — uebersprungen\n"; continue; }
        std::vector<std::int64_t> samples(static_cast<std::size_t>(batches));
        auto const n = mw->run_workload(ops, batches, seed, samples.data(), samples.size());
        if (n < 2) { std::cerr << "DLL " << i << " lieferte < 2 Samples — uebersprungen\n"; continue; }
        samples.resize(static_cast<std::size_t>(n));
        std::string nm = std::string{handles[i].anatomy()->composition_name()} + "_" + std::to_string(i);
        names.push_back(std::move(nm));
        results.push_back(cmd::make_execution_result(names.back(), std::move(samples)));
    }
    if (results.size() < 2) {
        std::cerr << "Weniger als 2 mess-faehige DLLs — Abbruch.\n";
        return 3;
    }

    std::size_t const base_idx = (baseline < results.size()) ? static_cast<std::size_t>(baseline) : 0;
    std::vector<cmd::ExecutionResult> candidates;
    candidates.reserve(results.size() - 1);
    for (std::size_t i = 0; i < results.size(); ++i) if (i != base_idx) candidates.push_back(results[i]);

    auto const rep = stats::multi_compare_against_baseline(
        results[base_idx], std::span<const cmd::ExecutionResult>{candidates}, alpha);
    auto const sum = stats::summarize(rep);

    std::cout << "F15 multi-compare ueber " << results.size() << " DLLs (baseline=" << names[base_idx]
              << ", alpha=" << alpha << ", samples/DLL=" << batches << ")\n";
    std::cout << "  significant_faster=" << sum.significant_faster
              << "  significant_slower=" << sum.significant_slower
              << "  not_significant=" << sum.not_significant << "\n";
    std::cout << "  win_rate=" << sum.win_rate << "  (Anteil, der die Baseline signifikant schlaegt)\n";
    for (auto const& c : rep.comparisons) {
        std::cout << "    " << c.name << ": adjusted_p=" << c.adjusted_p
                  << (c.significant ? "  SIGNIFIKANT" : "  n.s.")
                  << (c.faster_than_baseline ? "  (schneller)" : "  (langsamer)") << "\n";
    }

    // Ranking ueber ALLE Kompositionen (inkl. Baseline) nach mittlerer Latenz — die praktische
    // F15-Antwort "welche Achsen-Komposition gewinnt?". Schnellste zuerst.
    std::vector<std::pair<double, std::size_t>> ranking;
    ranking.reserve(results.size());
    for (std::size_t i = 0; i < results.size(); ++i) {
        double const mean = stats::latency_mean_ns(
            std::span<const std::int64_t>{results[i].latency_samples_ns});
        ranking.emplace_back(mean, i);
    }
    std::sort(ranking.begin(), ranking.end(),
              [](auto const& a, auto const& b) { return a.first < b.first; });
    std::cout << "  Ranking (schnellste zuerst, mittlere Latenz ns):\n";
    for (std::size_t r = 0; r < ranking.size(); ++r) {
        auto const [mean, idx] = ranking[r];
        std::cout << "    #" << (r + 1) << "  " << names[idx] << "  mean=" << mean << " ns"
                  << (idx == base_idx ? "  [baseline]" : "") << "\n";
    }
    if (ranking.size() >= 2 && ranking.back().first > 0.0) {
        std::cout << "  Spanne langsamste/schnellste = "
                  << (ranking.back().first / ranking.front().first) << "x\n";
    }

    if (!csv_path.empty()) {
        if (write_text_file(csv_path, stats::report_to_csv(rep))) std::cout << "  CSV  -> " << csv_path << "\n";
        else { std::cerr << "CSV-Schreiben fehlgeschlagen: " << csv_path << "\n"; return 4; }
    }
    if (!json_path.empty()) {
        if (write_text_file(json_path, stats::report_to_json(rep))) std::cout << "  JSON -> " << json_path << "\n";
        else { std::cerr << "JSON-Schreiben fehlgeschlagen: " << json_path << "\n"; return 4; }
    }
    return 0;
}
