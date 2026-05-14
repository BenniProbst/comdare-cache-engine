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
//
// REV 7.6 V8.6 — Modi 'defined' (vorhandene SOTA-Profile aus
// algorithm_profiles/sota/) vs. 'full' (alle Achsen-Permutationen aus
// permutation_axes.xml). User-Direktive 2026-05-13/14 + Habich-Antwort
// 2026-05-14: "immer so vollstaendig wie moeglich".

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace comdare::builder::xml {

struct PermutationEntry {
    std::string id;
    std::unordered_map<std::string, std::string> attributes;
};

// REV 7.6 V8.6 — Mess-Modi
enum class MessreihenMode {
    Defined,  // nur referenzierte SOTA-Profile aus algorithm_profiles/sota/
    Full      // alle Permutationen aus permutation_axes.xml (Cartesian product)
};

[[nodiscard]] inline MessreihenMode parse_mode(std::string_view s) noexcept {
    if (s == "full") return MessreihenMode::Full;
    return MessreihenMode::Defined;  // default
}

[[nodiscard]] inline std::string_view mode_to_string(MessreihenMode m) noexcept {
    return (m == MessreihenMode::Full) ? "full" : "defined";
}

// REV 7.6 V8.6 — Ein Profil aus algorithm_profiles/sota/<id>.profile.xml
// REV 7.6 V19.1 — expected_workload Tag (optional Override fuer V11.2-Heuristik)
// REV 7.7 V29.A — allocator_override Tag (optional Override fuer axes/allocator)
struct AlgorithmProfile {
    std::string id;          // z.B. "art", "hot", "masstree"
    std::string paper_ref;   // z.B. "P01", "P02"
    std::unordered_map<std::string, std::string> axes;  // page, node, traversal, ...
    std::string key_types;
    std::string value_types;
    std::string expected_workload;   // V19.1: optional, z.B. "YCSB_A".."YCSB_F" oder leer
    std::string allocator_override;  // V29.A: optional, Allokator-Profile-id (z.B. "mimalloc")
};

// REV 7.6 V8.6 — Eine Messreihe aus messreihe-XML
struct Messreihe {
    std::string id;
    MessreihenMode mode = MessreihenMode::Defined;
    // defined-mode: referenzierte sota-Profile (per id)
    std::vector<std::string> sota_profile_refs;
    // full-mode: keine Refs noetig — Builder enumeriert alle Achsen-Permutationen
};

struct CacheEngineConfig {
    std::vector<PermutationEntry> cache_engine_permutations;
    std::vector<PermutationEntry> search_algorithm_permutations;
    std::vector<PermutationEntry> allocator_permutations;
    std::vector<PermutationEntry> test_data_sets;

    // REV 7.6 V8.6 — geladene Profile + Messreihen
    std::vector<AlgorithmProfile> sota_profiles;
    std::vector<Messreihe>        messreihen;
};

class XmlConfigParser {
public:
    [[nodiscard]] CacheEngineConfig parse(std::filesystem::path const& root_dir) const;
    [[nodiscard]] std::vector<PermutationEntry> parse_one(std::filesystem::path const& xml_file) const;

    // REV 7.6 V8.6 — algorithm_profiles/-Loader
    [[nodiscard]] std::vector<AlgorithmProfile> load_sota_profiles(
        std::filesystem::path const& sota_dir) const;
    [[nodiscard]] AlgorithmProfile parse_profile(std::filesystem::path const& profile_xml) const;
    [[nodiscard]] std::vector<Messreihe> load_messreihen(
        std::filesystem::path const& messreihen_xml) const;
};

}  // namespace comdare::builder::xml
