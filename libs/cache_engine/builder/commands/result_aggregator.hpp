#pragma once
// V41.F.6.1 R5.E (2026-05-29) — ResultAggregator: ExecutionResult-Sammlung → CSV/JSON-Export
//
// @subsystem CEB (Mess-Auswertung, neben welch_t_test.hpp + multiple_comparison.hpp)
// @phase_owner CEB
//
// F15 misst zehntausende Achsen-Kompositionen (je ein ExecutionResult). Fuer die Auswertung
// (R-/Python-Analyse, Diplomarbeit-Tabellen, CI-Artefakte) muessen diese Ergebnisse maschinen-
// lesbar exportiert werden. Diese header-only-Utility serialisiert eine Result-Sammlung nach
// CSV (eine Zeile pro Komposition) bzw. JSON (Array von Objekten) — ohne externe Abhaengigkeit.
//
// Hinweis: workload_kind wird als Integer-Enum-Wert emittiert (stabil, ohne Namens-Tabelle);
// latency_samples_ns werden NICHT inline serialisiert (nur deren Anzahl n_latency_samples) —
// Roh-Samples gehoeren in separate Dateien (Groesse).

#include "execution_result.hpp"
#include "latency_stats.hpp" // make_execution_result fuellt p50/p99 aus den Samples

#include <cstdint>
#include <cstdio>
#include <sstream>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::commands {

namespace detail {

/// JSON-String-Escaping (", \\, Steuerzeichen). Minimal + korrekt fuer ASCII-Identifier.
inline std::string json_escape(std::string_view s) {
    std::string out;
    out.reserve(s.size() + 2);
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned>(static_cast<unsigned char>(c)));
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    return out;
}

/// CSV-Feld-Quoting: in Anfuehrungszeichen, eingebettete " verdoppelt (RFC 4180).
inline std::string csv_quote(std::string_view s) {
    std::string out;
    out.reserve(s.size() + 2);
    out += '"';
    for (char c : s) {
        if (c == '"') out += '"';
        out += c;
    }
    out += '"';
    return out;
}

} // namespace detail

/// make_execution_result — Konnektor measure→analyze: aus Latenz-Samples (z.B. von
/// IMeasurableWorkload::run_workload) eine ExecutionResult bauen, mit p50/p99 aus den Samples
/// (latency_stats) + success-Flag. Damit kann ein Mess-Treiber pro Komposition run_workload ->
/// make_execution_result -> multi_compare_against_baseline -> summarize/export verketten.
/// HINWEIS: engine_name ist string_view → der Aufrufer muss den Namen am Leben halten (wie ueblich
/// bei ExecutionResult); Samples werden gemovet.
[[nodiscard]] inline ExecutionResult make_execution_result(std::string_view          engine_name,
                                                           std::vector<std::int64_t> latency_samples_ns,
                                                           double                    throughput_ops_per_sec = 0.0) {
    ExecutionResult r{};
    r.engine_name            = engine_name;
    r.throughput_ops_per_sec = throughput_ops_per_sec;
    std::span<const std::int64_t> const sp{latency_samples_ns};
    r.latency_p50        = stats::latency_p50_ns(sp);
    r.latency_p99        = stats::latency_p99_ns(sp);
    r.success            = !latency_samples_ns.empty();
    r.latency_samples_ns = std::move(latency_samples_ns);
    return r;
}

/// CSV-Header-Zeile (RFC-4180-konform), passend zu to_csv_row / to_csv.
[[nodiscard]] inline std::string result_csv_header() {
    return "engine_name,workload_kind,throughput_ops_per_sec,latency_p50_ns,latency_p99_ns,"
           "total_cache_misses,memory_footprint_bytes,H1_clu_improvement,H2_layout_score,"
           "H3_inline_external_ratio,n_latency_samples,success";
}

/// Eine ExecutionResult als CSV-Zeile (ohne Zeilenumbruch).
[[nodiscard]] inline std::string to_csv_row(ExecutionResult const& r) {
    std::ostringstream os;
    os << detail::csv_quote(r.engine_name) << ',' << static_cast<int>(r.workload_kind) << ','
       << r.throughput_ops_per_sec << ',' << r.latency_p50.count() << ',' << r.latency_p99.count() << ','
       << r.total_cache_misses << ',' << r.memory_footprint_bytes << ',' << r.H1_clu_improvement << ','
       << r.H2_layout_score << ',' << r.H3_inline_external_ratio << ',' << r.latency_samples_ns.size() << ','
       << (r.success ? 1 : 0);
    return os.str();
}

/// Vollstaendiges CSV (Header + eine Zeile pro Result).
[[nodiscard]] inline std::string to_csv(std::span<const ExecutionResult> results) {
    std::ostringstream os;
    os << result_csv_header() << '\n';
    for (auto const& r : results) os << to_csv_row(r) << '\n';
    return os.str();
}

/// Ein ExecutionResult als JSON-Objekt.
[[nodiscard]] inline std::string to_json_object(ExecutionResult const& r) {
    std::ostringstream os;
    os << '{' << "\"engine_name\":\"" << detail::json_escape(r.engine_name) << "\","
       << "\"workload_kind\":" << static_cast<int>(r.workload_kind) << ','
       << "\"throughput_ops_per_sec\":" << r.throughput_ops_per_sec << ','
       << "\"latency_p50_ns\":" << r.latency_p50.count() << ',' << "\"latency_p99_ns\":" << r.latency_p99.count() << ','
       << "\"total_cache_misses\":" << r.total_cache_misses << ','
       << "\"memory_footprint_bytes\":" << r.memory_footprint_bytes << ','
       << "\"H1_clu_improvement\":" << r.H1_clu_improvement << ',' << "\"H2_layout_score\":" << r.H2_layout_score << ','
       << "\"H3_inline_external_ratio\":" << r.H3_inline_external_ratio << ','
       << "\"n_latency_samples\":" << r.latency_samples_ns.size() << ','
       << "\"success\":" << (r.success ? "true" : "false") << '}';
    return os.str();
}

/// Vollstaendiges JSON (Array von Objekten).
[[nodiscard]] inline std::string to_json(std::span<const ExecutionResult> results) {
    std::ostringstream os;
    os << '[';
    bool first = true;
    for (auto const& r : results) {
        if (!first) os << ',';
        os << to_json_object(r);
        first = false;
    }
    os << ']';
    return os.str();
}

} // namespace comdare::cache_engine::builder::commands
