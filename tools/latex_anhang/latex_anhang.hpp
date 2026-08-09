#pragma once
// SPDX-License-Identifier: Apache-2.0
// latex_anhang -- die Schnittstelle des Werkzeugs, an EINER Stelle.
//
// SCHWESTERSTELLE zu tools/ycsb_cli/ycsb_cli.hpp, wortgleiche Begruendung (09.08.2026):
// main.cpp wird nicht nur zum Programm gebunden, sondern zusaetzlich in die statische Bibliothek
// comdare_latex_anhang_lib (LATEX_ANHANG_TEST_NO_MAIN), und tests/unit/test_latex_anhang.cpp ruft
// die Helfer aus einer ZWEITEN Uebersetzungseinheit.
//
// Der Versuch, -Wmissing-declarations (5 Stellen) mit einem UNBENANNTEN NAMENSRAUM zu heilen
// (Warnungs-Runde 1c/2a, Commit 0335e2cb), gab den Helfern interne Bindung und riss test_latex_anhang
// aus dem Bau: "undefined reference to latex_anhang::split_csv_line(...)" und sieben weitere.
// Geprueft worden war, ob ein Header existiert -- nicht, ob es Aufrufer gibt. Und der Test half sich
// wie bei ycsb_cli mit einer HANDABSCHRIFT der Struktur CsvRow samt fuenf Signaturen: zwei Kopien
// desselben Layouts, die stillschweigend auseinanderlaufen koennen. Bei einer STRUKTUR ist das die
// teurere Sorte -- weicht ein Feld ab, ist es kein Binder-Fehler mehr, sondern ein stiller
// ODR-Verstoss mit falsch gelesenen Zahlen.
//
// Diese Datei erledigt beides ursaechlich: main.cpp bekommt die vorherige Deklaration (Warnung weg,
// ohne Unterdrueckung, ohne Bindungsaenderung), der Test liest dieselbe Definition.

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace latex_anhang {

inline constexpr int status_ok               = 0;
inline constexpr int status_invalid_argument = 4;
inline constexpr int status_io_error         = 10;
inline constexpr int status_parse_error      = 11;

struct CsvRow {
    std::string   permutation_id;
    std::uint64_t fingerprint             = 0;
    bool          succeeded               = false;
    std::string   workload_used           = "n/a";
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

[[nodiscard]] std::vector<std::string> split_csv_line(std::string const& line);
[[nodiscard]] std::string              escape_latex(std::string_view s);
[[nodiscard]] int                      parse_csv(std::string const& path, std::vector<CsvRow>& rows);
[[nodiscard]] int write_latex(std::string const& path, std::vector<CsvRow> const& rows, std::string const& caption_text,
                              std::string const& label);
[[nodiscard]] int write_latex_full_metrics(std::string const& path, std::vector<CsvRow> const& rows,
                                           std::string const& caption_text, std::string const& label);

} // namespace latex_anhang
