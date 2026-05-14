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

    // REV 7.6 V8.6 — Optional: algorithm_profiles + Messreihen
    auto profiles_dir = root_dir / "algorithm_profiles";
    if (std::filesystem::is_directory(profiles_dir / "sota")) {
        cfg.sota_profiles = load_sota_profiles(profiles_dir / "sota");
    }
    auto messreihen_xml = root_dir / "messreihen.xml";
    if (std::filesystem::exists(messreihen_xml)) {
        cfg.messreihen = load_messreihen(messreihen_xml);
    }
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

// REV 7.6 V8.6 — algorithm_profiles/-Loader
std::vector<AlgorithmProfile> XmlConfigParser::load_sota_profiles(
    std::filesystem::path const& sota_dir) const
{
    std::vector<AlgorithmProfile> result;
    if (!std::filesystem::is_directory(sota_dir)) return result;
    for (auto const& entry : std::filesystem::directory_iterator(sota_dir)) {
        auto const& p = entry.path();
        if (p.extension() == ".xml" && p.filename().string().find(".profile.") != std::string::npos) {
            result.push_back(parse_profile(p));
        }
    }
    return result;
}

AlgorithmProfile XmlConfigParser::parse_profile(std::filesystem::path const& profile_xml) const {
    AlgorithmProfile prof;
    if (!std::filesystem::exists(profile_xml)) return prof;
    std::string content = read_file(profile_xml);

    // id="..." + paper_ref="..."  (eindeutiger Raw-String-Delimiter PAT — V12.7-Fix
    // gegen MSVC-Parser-Konflikt zwischen Pattern-`"` und Raw-String-Close)
    {
        std::regex id_re{R"PAT(<comdare_algorithm_profile\s+id\s*=\s*"([^"]+)")PAT"};
        std::smatch m;
        if (std::regex_search(content, m, id_re)) prof.id = m[1].str();
    }
    {
        std::regex pref_re{R"PAT(paper_ref\s*=\s*"([^"]+)")PAT"};
        std::smatch m;
        if (std::regex_search(content, m, pref_re)) prof.paper_ref = m[1].str();
    }

    // Achsen aus <axes>...</axes>
    auto axes_open = content.find("<axes>");
    auto axes_close = content.find("</axes>");
    if (axes_open != std::string::npos && axes_close != std::string::npos) {
        std::string axes_inner = content.substr(axes_open + 6, axes_close - axes_open - 6);
        std::regex axis_re{R"(<(\w+)>([^<]+)</\w+>)"};
        auto it = std::sregex_iterator(axes_inner.begin(), axes_inner.end(), axis_re);
        auto end = std::sregex_iterator();
        for (; it != end; ++it) {
            prof.axes[(*it)[1].str()] = (*it)[2].str();
        }
    }

    // key_value_signature
    {
        std::regex kt{R"(<key_types>([^<]+)</key_types>)"};
        std::smatch m;
        if (std::regex_search(content, m, kt)) prof.key_types = m[1].str();
    }
    {
        std::regex vt{R"(<value_types>([^<]+)</value_types>)"};
        std::smatch m;
        if (std::regex_search(content, m, vt)) prof.value_types = m[1].str();
    }
    // V19.1 — expected_workload (optional, ueberschreibt V11.2-Heuristik)
    {
        std::regex ew{R"(<expected_workload>([^<]+)</expected_workload>)"};
        std::smatch m;
        if (std::regex_search(content, m, ew)) prof.expected_workload = m[1].str();
    }
    return prof;
}

std::vector<Messreihe> XmlConfigParser::load_messreihen(
    std::filesystem::path const& messreihen_xml) const
{
    std::vector<Messreihe> result;
    if (!std::filesystem::exists(messreihen_xml)) return result;
    std::string content = read_file(messreihen_xml);

    std::regex reihe_re{R"PAT(<messreihe\s+id\s*=\s*"([^"]+)"\s*>([\s\S]*?)</messreihe>)PAT"};
    std::sregex_iterator reihe_it{content.cbegin(), content.cend(), reihe_re};
    std::sregex_iterator reihe_end{};
    for (; reihe_it != reihe_end; ++reihe_it) {
        Messreihe r;
        r.id = (*reihe_it)[1].str();
        std::string inner = (*reihe_it)[2].str();

        std::regex mode_re{R"(<mode>(\w+)</mode>)"};
        std::smatch mm;
        if (std::regex_search(inner, mm, mode_re)) r.mode = parse_mode(mm[1].str());

        std::regex prof_re{R"(<profile>([^<]+)</profile>)"};
        std::sregex_iterator prof_it{inner.cbegin(), inner.cend(), prof_re};
        std::sregex_iterator prof_end{};
        for (; prof_it != prof_end; ++prof_it) {
            r.sota_profile_refs.push_back((*prof_it)[1].str());
        }
        result.push_back(std::move(r));
    }
    return result;
}

}  // namespace comdare::builder::xml
