#pragma once
// Codegen - generiert CMake-Targets + Modul-Quellen pro Permutation (REV 7 §5 + F-EXTRA-5)
//
// KEIN Python! Nur CMake + sh + bat (synchron gepflegt).
// Pro Permutation eine eigene Modul-Binary mit ABI-stabilem v1 Interface.

#include "../xml_config_parser/xml_config_parser.hpp"

#include <filesystem>
#include <string>

namespace comdare::builder::codegen {

struct CodegenOptions {
    std::filesystem::path output_root;        // wohin generierte Files
    std::filesystem::path comdare_root;        // wo cache_engine/include/ liegt
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

    [[nodiscard]] std::filesystem::path module_source_path(std::uint64_t fingerprint) const;
    [[nodiscard]] std::filesystem::path module_cmake_path(std::uint64_t fingerprint) const;

private:
    CodegenOptions opts_;
};

}  // namespace comdare::builder::codegen
