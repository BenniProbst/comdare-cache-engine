// XML-Config-Parser Implementation (Skelett)
// Phase 6.4: minimaler XML-Tag-Reader; produktiv via tinyxml2 oder eigener Parser

#include "xml_config_parser.hpp"

#include <fstream>
#include <regex>
#include <sstream>

namespace comdare::builder::xml {

namespace {

std::string read_file(std::filesystem::path const& path) {
    std::ifstream in{path};
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// Sehr simpler XML-Tag-Extraktor: <tag id="..."><nested>...</nested></tag>
std::vector<PermutationEntry> parse_xml_string(std::string const& content,
                                                std::string const& tag_name) {
    std::vector<PermutationEntry> entries;
    std::regex open_tag{"<" + tag_name + "\\s+id\\s*=\\s*\"([^\"]+)\"\\s*>"};
    std::regex inner_tag{"<(\\w+)>([^<]+)</\\w+>"};
    auto it  = std::sregex_iterator(content.begin(), content.end(), open_tag);
    auto end = std::sregex_iterator();
    for (; it != end; ++it) {
        PermutationEntry entry;
        entry.id = (*it)[1].str();
        // Find closing tag
        auto close_pos = content.find("</" + tag_name + ">", it->position() + it->length());
        std::string inner = content.substr(it->position() + it->length(),
                                            close_pos - it->position() - it->length());
        auto inner_it  = std::sregex_iterator(inner.begin(), inner.end(), inner_tag);
        auto inner_end = std::sregex_iterator();
        for (; inner_it != inner_end; ++inner_it) {
            entry.attributes[(*inner_it)[1].str()] = (*inner_it)[2].str();
        }
        entries.push_back(std::move(entry));
    }
    return entries;
}

}  // anonymous namespace

CacheEngineConfig XmlConfigParser::parse(std::filesystem::path const& root_dir) const {
    CacheEngineConfig cfg;
    cfg.cache_engine_permutations     = parse_one(root_dir / "cache_engine_permutations.xml");
    cfg.search_algorithm_permutations = parse_one(root_dir / "search_algorithm_permutations.xml");
    cfg.allocator_permutations         = parse_one(root_dir / "allocator_permutations.xml");
    cfg.test_data_sets                  = parse_one(root_dir / "test_data_sets.xml");
    return cfg;
}

std::vector<PermutationEntry> XmlConfigParser::parse_one(std::filesystem::path const& xml_file) const {
    if (!std::filesystem::exists(xml_file)) return {};
    std::string content = read_file(xml_file);

    // Probiere verschiedene Tag-Namen aus
    for (auto const& tag : {"cache_engine_permutation", "search_algorithm",
                             "allocator_permutation", "test_data_set"}) {
        auto entries = parse_xml_string(content, tag);
        if (!entries.empty()) return entries;
    }
    return {};
}

}  // namespace comdare::builder::xml
