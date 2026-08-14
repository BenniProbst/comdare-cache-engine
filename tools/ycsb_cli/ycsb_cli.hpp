#pragma once
// SPDX-License-Identifier: LicenseRef-Comdare-Research-1.0
// ycsb_cli -- die Schnittstelle des Werkzeugs, an EINER Stelle.
//
// WARUM ES DIESEN HEADER GIBT (09.08.2026, Merge-Befund der ce-Sammellandung):
// main.cpp wird NICHT nur zum Programm gebunden. tools/ycsb_cli/CMakeLists.txt baut daraus
// zusaetzlich die statische Bibliothek comdare_ycsb_cli_lib (mit YCSB_CLI_TEST_NO_MAIN=1, also
// ohne main()), und tests/unit/test_ycsb_cli.cpp bindet genau diese Bibliothek und ruft die
// Helfer aus einer ZWEITEN Uebersetzungseinheit auf.
//
// Zwei Folgen hatte das Fehlen dieses Headers:
//  1. Die Definitionen in main.cpp hatten externe Bindung ohne vorherige Deklaration --
//     -Wmissing-declarations, 9 Stellen. Der Versuch, das mit einem UNBENANNTEN NAMENSRAUM zu
//     heilen (Warnungs-Runde 1c/2a, Commit 0335e2cb), gab ihnen INTERNE Bindung und riss damit
//     test_ycsb_cli aus dem Bau: "undefined reference to ycsb_cli::parse_args(...)" und acht
//     weitere. Die Begruendung dort ("dieses Werkzeug ist genau EINE Uebersetzungseinheit")
//     stimmte fuer das Programm und war fuer die Bibliothek falsch -- geprueft worden war, ob ein
//     Header existiert, nicht, ob es Aufrufer gibt.
//  2. Der Test half sich mit einer HANDABSCHRIFT der Deklarationen ("Re-declare ycsb_cli API").
//     Zwei Kopien derselben Signaturen, die stillschweigend auseinanderlaufen koennen: aendert
//     sich hier ein Parametertyp, bricht nicht der Test, sondern erst der Binder -- und bei
//     gleicher Signatur mit anderer Bedeutung gar nichts.
//
// Beides ist mit dieser einen Datei ursaechlich erledigt: main.cpp bekommt seine vorherige
// Deklaration (Warnung weg, OHNE Unterdrueckung und OHNE die Bindung zu aendern), und der Test
// liest dieselben Signaturen, statt sie abzuschreiben.

#include <comdare/workload_generator/workload_generator.hpp>

#include <cstdint>
#include <filesystem>
#include <span>
#include <string_view>

namespace ycsb_cli {

namespace wg = comdare::workload_generator;

enum class OutputFormat : std::uint8_t {
    Binary = 0,
    Tsv    = 1,
    Json   = 2,
};

struct CliConfig {
    wg::YcsbWorkload      workload      = wg::YcsbWorkload::C;
    std::uint64_t         num_keys      = 100000;
    std::uint64_t         num_ops       = 1000000;
    std::uint32_t         key_size      = 16;
    std::uint32_t         value_size    = 64;
    wg::KeyDistribution   key_dist      = wg::KeyDistribution::Zipfian;
    double                zipfian_theta = 0.99;
    std::uint64_t         seed          = 42;
    std::filesystem::path output        = "workload.bin";
    OutputFormat          format        = OutputFormat::Binary;
};

// CLI-Parser und Schreiber (errno-artige Rueckgabe: 0 ok, 4 invalid_argument, 5 I/O).
[[nodiscard]] int parse_workload(std::string_view s, wg::YcsbWorkload& out) noexcept;
[[nodiscard]] int parse_format(std::string_view s, OutputFormat& out) noexcept;
[[nodiscard]] int parse_key_dist(std::string_view s, wg::KeyDistribution& out) noexcept;
[[nodiscard]] int parse_args(int argc, char const* const* argv, CliConfig& cfg) noexcept;
[[nodiscard]] int write_binary(std::filesystem::path const& p, std::span<wg::Operation const> ops) noexcept;
[[nodiscard]] int write_tsv(std::filesystem::path const& p, std::span<wg::Operation const> ops) noexcept;
[[nodiscard]] int write_json(std::filesystem::path const& p, std::span<wg::Operation const> ops) noexcept;
[[nodiscard]] int generate_and_write(CliConfig const& cfg) noexcept;

} // namespace ycsb_cli
