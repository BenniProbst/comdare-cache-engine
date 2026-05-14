#pragma once
// Codegen - generiert CMake-Targets + Modul-Quellen pro Permutation (REV 7 §5 + F-EXTRA-5)
//
// KEIN Python! Nur CMake + sh + bat (synchron gepflegt).
// Pro Permutation eine eigene Modul-Binary mit ABI-stabilem v1 Interface.

#include "../xml_config_parser/xml_config_parser.hpp"

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace comdare::builder::codegen {

struct CodegenOptions {
    std::filesystem::path output_root;        // wohin generierte Files
    std::filesystem::path comdare_root;        // wo cache_engine/include/ liegt
    std::filesystem::path prt_art_root;        // V18.1: optional, prt-art-Submodule-Root fuer Pruefling-Templates
    std::string           cmake_backend = "cmake";   // cmake / sh / bat
};

class CodegenEngine {
public:
    explicit CodegenEngine(CodegenOptions opts) noexcept : opts_{std::move(opts)} {}

    // Generiert fuer eine Permutation:
    //   - module_<fingerprint>.cpp (C++23-Quelle mit comdare_get_module_v1)
    //   - module_<fingerprint>_CMakeLists.txt (CMake-Target)
    //   - module_<fingerprint>.cmake (codegen-Backend choice)
    void generate_module(xml::PermutationEntry const& cache_engine_perm,
                         xml::PermutationEntry const& search_algorithm_perm,
                         xml::PermutationEntry const& allocator_perm,
                         std::uint64_t fingerprint) const;

    // REV 7.6 V9.3 — Generiert ein Modul direkt aus einem AlgorithmProfile
    // (algorithm_profiles/sota/<id>.profile.xml). Das Profil enthaelt alle
    // 11 Achsen + Key/Value-Signature + Paper-Ref. Der Codegen fingerprint-t
    // das Profil und schreibt:
    //   - module_<id>_<fingerprint>.cpp
    //   - module_<id>_<fingerprint>_CMakeLists.txt
    void generate_module_from_profile(xml::AlgorithmProfile const& profile,
                                       std::uint64_t fingerprint) const;

    // Phase 7.2.A: generiert einen zentralen Aggregator CMakeLists.txt im
    // output_root, der alle module_<fp>_CMakeLists.txt include-t.
    // Aufruf NACH allen generate_module()-Calls; erwartet die fertige
    // Fingerprint-Liste. Resultat: <output_root>/CMakeLists.txt
    void generate_aggregate_cmake(std::span<std::uint64_t const> fingerprints) const;

    [[nodiscard]] std::filesystem::path module_source_path(std::uint64_t fingerprint) const;
    [[nodiscard]] std::filesystem::path module_cmake_path(std::uint64_t fingerprint) const;
    [[nodiscard]] std::filesystem::path aggregate_cmake_path() const;

    [[nodiscard]] std::filesystem::path const& output_root() const noexcept { return opts_.output_root; }
    [[nodiscard]] std::filesystem::path const& comdare_root() const noexcept { return opts_.comdare_root; }

private:
    CodegenOptions opts_;
};

}  // namespace comdare::builder::codegen
