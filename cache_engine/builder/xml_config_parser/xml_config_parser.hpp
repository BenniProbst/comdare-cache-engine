#pragma once
// XML-Config-Parser - Liest 4 Konfigurations-Saetze (REV 7 §5.2)
//
//   cache_engine_permutations.xml
//   search_algorithm_permutations.xml
//   allocator_permutations.xml
//   test_data_sets.xml
//
// Minimaler XML-Reader fuer Phase 6.4 Skelett. Phase 7 ersetzt durch
// echten XML-Parser (tinyxml2 oder eigene Implementation).

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace comdare::builder::xml {

struct PermutationEntry {
    std::string id;
    std::unordered_map<std::string, std::string> attributes;
};

struct CacheEngineConfig {
    std::vector<PermutationEntry> cache_engine_permutations;
    std::vector<PermutationEntry> search_algorithm_permutations;
    std::vector<PermutationEntry> allocator_permutations;
    std::vector<PermutationEntry> test_data_sets;
};

class XmlConfigParser {
public:
    [[nodiscard]] CacheEngineConfig parse(std::filesystem::path const& root_dir) const;
    [[nodiscard]] std::vector<PermutationEntry> parse_one(std::filesystem::path const& xml_file) const;
};

}  // namespace comdare::builder::xml
