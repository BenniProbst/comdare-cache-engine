// Limits-Entkopplung Vorstufe: liest ein Thesis-Profil und schreibt einen
// generierten Source-Katalog-Header in das Build-Verzeichnis.

#include <builder/codegen/adhoc_emitter.hpp>
#include <builder/codegen/type_name.hpp>
#include <permutations/permutation_engine.hpp>
#include <topics/allocator/topic_allocator_config_set.hpp>
#include <topics/concurrency/topic_concurrency_config_set.hpp>
#include <topics/filter/topic_filter_config_set.hpp>
#include <topics/hardware/topic_hardware_config_set.hpp>
#include <topics/io/topic_io_config_set.hpp>
#include <topics/memory_layout/topic_memory_layout_config_set.hpp>
#include <topics/migration/topic_migration_config_set.hpp>
#include <topics/nodes/topic_nodes_config_set.hpp>
#include <topics/prefetch/topic_prefetch_config_set.hpp>
#include <topics/queuing/topic_queuing_config_set.hpp>
#include <topics/search_engine/topic_search_engine_config_set.hpp>
#include <topics/serialization/topic_serialization_config_set.hpp>
#include <topics/telemetry/topic_telemetry_config_set.hpp>
#include <topics/traversal/topic_traversal_config_set.hpp>
#include <topics/value_handle/topic_value_handle_config_set.hpp>
#include <xml_config_parser/xml_config_parser.hpp>

#include <boost/mp11.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace ce = ::comdare::cache_engine;
namespace cg = ::comdare::cache_engine::builder::codegen;
namespace cx = ::comdare::builder::xml;
namespace fs = ::std::filesystem;
namespace mp = ::boost::mp11;

namespace {

struct CliArgs {
    fs::path profile;
    fs::path out;
};

struct VariantEntry {
    std::string name;
    std::string cpp_type;
};

struct SlotSpec {
    std::string               axis;
    std::vector<VariantEntry> variants;
};

struct SelectedSlot {
    std::string               axis;
    std::vector<VariantEntry> variants;
};

void print_usage() {
    std::cerr << "Usage: comdare-catalog-codegen --profile <file.profile.xml> --out <generated-header>\n";
}

[[nodiscard]] std::optional<CliArgs> parse_args(int argc, char** argv) {
    CliArgs args;
    for (int i = 1; i < argc; ++i) {
        std::string_view const a{argv[i]};
        if (a == "--help") {
            print_usage();
            std::exit(0);
        }
        if (a == "--profile" && i + 1 < argc) {
            args.profile = argv[++i];
            continue;
        }
        if (a == "--out" && i + 1 < argc) {
            args.out = argv[++i];
            continue;
        }
        std::cerr << "catalog-codegen: unknown or incomplete argument: " << a << "\n";
        return std::nullopt;
    }
    if (args.profile.empty() || args.out.empty()) {
        print_usage();
        return std::nullopt;
    }
    return args;
}

template <class W>
[[nodiscard]] std::string cpp_type_name() {
    std::string t = cg::strip_all_elaborated(cg::type_name<W>());
    if (t.rfind("::", 0) != 0) t.insert(0, "::");
    return t;
}

template <class List>
[[nodiscard]] std::vector<VariantEntry> variants_for_list() {
    std::vector<VariantEntry> out;
    out.reserve(mp::mp_size<List>::value);
    mp::mp_for_each<mp::mp_transform<mp::mp_identity, List>>([&](auto id) {
        using W = typename decltype(id)::type;
        out.push_back(VariantEntry{std::string{W::name()}, cpp_type_name<W>()});
    });
    return out;
}

[[nodiscard]] std::array<SlotSpec, 19> make_slots() {
    // Kanonische 19-Slot-Tabelle, Quelle: tests/unit/thesis_tiere/source_catalog.hpp:85-103.
    // L00 search_algo        -> traversal::TopicConfigSet::StaticAxisVariants_03a
    // L01 cache_traversal    -> traversal::TopicConfigSet::StaticAxisVariants_03b
    // L02 mapping            -> traversal::TopicConfigSet::StaticAxisVariants_03m
    // L03 path_compression   -> nodes::TopicConfigSet::StaticAxisVariants_02
    // L04 node_type          -> nodes::TopicConfigSet::StaticAxisVariants_04
    // L05 memory_layout      -> memory_layout::TopicConfigSet::StaticAxisVariants
    // L06 allocator          -> allocator::TopicConfigSet::StaticAxisVariants
    // L07 prefetch           -> prefetch::TopicConfigSet::StaticAxisVariants
    // L08 concurrency        -> concurrency::TopicConfigSet::StaticAxisVariants
    // L09 serialization      -> serialization::TopicConfigSet::StaticAxisVariants
    // L10 telemetry          -> telemetry::TopicConfigSet::StaticAxisVariants
    // L11 value_handle       -> value_handle::TopicConfigSet::StaticAxisVariants
    // L12 isa                -> hardware::TopicConfigSet::StaticAxisVariants_09
    // L13 index_organization -> search_engine::TopicConfigSet::StaticAxisVariants
    // L14 io_dispatch        -> io::TopicConfigSet::StaticAxisVariants
    // L15 migration_policy   -> migration::TopicConfigSet::StaticAxisVariants
    // L16 filter             -> filter::TopicConfigSet::StaticAxisVariants
    // L17 queuing_q1         -> queuing::TopicConfigSet::StaticAxisVariants_Q1
    // L18 queuing_q2         -> queuing::TopicConfigSet::StaticAxisVariants_Q2
    return {SlotSpec{"search_algo", variants_for_list<ce::traversal::TopicConfigSet::StaticAxisVariants_03a>()},
            SlotSpec{"cache_traversal", variants_for_list<ce::traversal::TopicConfigSet::StaticAxisVariants_03b>()},
            SlotSpec{"mapping", variants_for_list<ce::traversal::TopicConfigSet::StaticAxisVariants_03m>()},
            SlotSpec{"path_compression", variants_for_list<ce::nodes::TopicConfigSet::StaticAxisVariants_02>()},
            SlotSpec{"node_type", variants_for_list<ce::nodes::TopicConfigSet::StaticAxisVariants_04>()},
            SlotSpec{"memory_layout", variants_for_list<ce::memory_layout::TopicConfigSet::StaticAxisVariants>()},
            SlotSpec{"allocator", variants_for_list<ce::allocator::TopicConfigSet::StaticAxisVariants>()},
            SlotSpec{"prefetch", variants_for_list<ce::prefetch::TopicConfigSet::StaticAxisVariants>()},
            SlotSpec{"concurrency", variants_for_list<ce::concurrency::TopicConfigSet::StaticAxisVariants>()},
            SlotSpec{"serialization", variants_for_list<ce::serialization::TopicConfigSet::StaticAxisVariants>()},
            SlotSpec{"telemetry", variants_for_list<ce::telemetry::TopicConfigSet::StaticAxisVariants>()},
            SlotSpec{"value_handle", variants_for_list<ce::value_handle::TopicConfigSet::StaticAxisVariants>()},
            SlotSpec{"isa", variants_for_list<ce::hardware::TopicConfigSet::StaticAxisVariants_09>()},
            SlotSpec{"index_organization", variants_for_list<ce::search_engine::TopicConfigSet::StaticAxisVariants>()},
            SlotSpec{"io_dispatch", variants_for_list<ce::io::TopicConfigSet::StaticAxisVariants>()},
            SlotSpec{"migration_policy", variants_for_list<ce::migration::TopicConfigSet::StaticAxisVariants>()},
            SlotSpec{"filter", variants_for_list<ce::filter::TopicConfigSet::StaticAxisVariants>()},
            SlotSpec{"queuing_q1", variants_for_list<ce::queuing::TopicConfigSet::StaticAxisVariants_Q1>()},
            SlotSpec{"queuing_q2", variants_for_list<ce::queuing::TopicConfigSet::StaticAxisVariants_Q2>()}};
}

[[nodiscard]] SlotSpec const* find_slot(std::array<SlotSpec, 19> const& slots, std::string_view axis) {
    auto const it = std::find_if(slots.begin(), slots.end(), [&](SlotSpec const& s) { return s.axis == axis; });
    return it == slots.end() ? nullptr : &*it;
}

[[nodiscard]] VariantEntry const* find_variant(SlotSpec const& slot, std::string_view value) {
    auto const it = std::find_if(slot.variants.begin(), slot.variants.end(),
                                 [&](VariantEntry const& v) { return v.name == value; });
    return it == slot.variants.end() ? nullptr : &*it;
}

[[nodiscard]] std::vector<std::string> value_axes(std::array<SlotSpec, 19> const& slots, std::string_view value) {
    std::vector<std::string> axes;
    for (SlotSpec const& slot : slots)
        if (find_variant(slot, value) != nullptr) axes.push_back(slot.axis);
    return axes;
}

[[nodiscard]] std::optional<std::vector<SelectedSlot>> select_slots(std::array<SlotSpec, 19> const& slots,
                                                                    cx::ThesisProfile const&        profile) {
    std::map<std::string, std::vector<std::string>> by_axis;
    std::size_t                                     last_slot_index = 0;
    bool                                            first_slot_ref  = true;
    for (cx::ThesisAxisSpec const& spec : profile.permute_axes) {
        SlotSpec const* slot = find_slot(slots, spec.ref);
        if (slot == nullptr) {
            // Review wf_fce92d2c (CONFIRMED): stilles Verwerfen erzeugt inkonsistente Kataloge —
            // cacheline-Ebenen sind binary-id-relevant (profile_to_tree.hpp:53-58), unbekannte refs
            // sind Tippfehler. Beides = harter Fehler statt gruener Bescheinigung.
            if (spec.ref == "cacheline") {
                std::cerr << "catalog-codegen: axis 'cacheline' ist in der Limits-Entkopplungs-Vorstufe nicht "
                             "unterstuetzt (erzeugt zusaetzliche statische binary_id-Segmente jenseits der 19 "
                             "Slot-Listen) -- Profil ablehnen.\n";
            } else {
                std::cerr << "catalog-codegen: unknown axis ref in profile: " << spec.ref << "\n";
            }
            return std::nullopt;
        }
        // Review wf_fce92d2c (CONFIRMED-major): build_axis_levels baut den Baum in PROFIL-Dokumentordnung,
        // die Source-Map-Keys sind IMMER kanonisch (kCompositionAxisNames) — eine nicht-kanonische
        // Profil-Ordnung ergaebe leere Lookups bei gruenem Tool. Deshalb: Ordnung hart validieren.
        std::size_t const slot_index = static_cast<std::size_t>(slot - slots.data());
        if (!first_slot_ref && slot_index <= last_slot_index) {
            std::cerr << "catalog-codegen: profile axis order deviates from canonical slot order at '" << spec.ref
                      << "' -- permute_axes muessen in kanonischer 19-Slot-Reihenfolge stehen.\n";
            return std::nullopt;
        }
        first_slot_ref  = false;
        last_slot_index = slot_index;
        if (!by_axis.emplace(spec.ref, spec.values).second) {
            std::cerr << "catalog-codegen: duplicate axis ref in profile: " << spec.ref << "\n";
            return std::nullopt;
        }
    }

    std::vector<SelectedSlot> selected;
    selected.reserve(slots.size());
    for (SlotSpec const& slot : slots) {
        SelectedSlot out{slot.axis, {}};
        auto const   found = by_axis.find(slot.axis);
        if (found == by_axis.end() || found->second.empty()) {
            out.variants.push_back(slot.variants.front());
            selected.push_back(std::move(out));
            continue;
        }

        std::set<std::string> seen;
        for (std::string const& value : found->second) {
            if (!seen.insert(value).second) {
                std::cerr << "catalog-codegen: duplicate value for axis '" << slot.axis << "': " << value << "\n";
                return std::nullopt;
            }
            VariantEntry const* variant = find_variant(slot, value);
            if (variant == nullptr) {
                std::vector<std::string> const axes = value_axes(slots, value);
                std::cerr << "catalog-codegen: value '" << value << "' is not valid for axis '" << slot.axis << "'";
                if (!axes.empty()) {
                    std::cerr << " (value belongs to axis";
                    if (axes.size() != 1) std::cerr << " list";
                    std::cerr << ": ";
                    for (std::size_t i = 0; i < axes.size(); ++i) {
                        if (i != 0) std::cerr << ", ";
                        std::cerr << axes[i];
                    }
                    std::cerr << ")";
                }
                std::cerr << "\n";
                return std::nullopt;
            }
            out.variants.push_back(*variant);
        }
        selected.push_back(std::move(out));
    }
    return selected;
}

void write_list(std::ostream& out, std::size_t index, SelectedSlot const& slot) {
    std::string const name = (index < 10 ? "L0" : "L") + std::to_string(index);
    out << "using " << name << " = ::boost::mp11::mp_list<\n";
    for (std::size_t i = 0; i < slot.variants.size(); ++i) {
        out << "    " << slot.variants[i].cpp_type;
        if (i + 1 < slot.variants.size()) out << ",";
        out << " // " << slot.axis << "=" << slot.variants[i].name << "\n";
    }
    out << ">;\n\n";
}

bool write_header(fs::path const& out_path, std::vector<SelectedSlot> const& selected) {
    std::error_code ec;
    fs::create_directories(out_path.parent_path(), ec);
    if (ec) {
        std::cerr << "catalog-codegen: cannot create output directory: " << out_path.parent_path().string() << "\n";
        return false;
    }

    std::ofstream out{out_path, std::ios::trunc};
    if (!out) {
        std::cerr << "catalog-codegen: cannot open output: " << out_path.string() << "\n";
        return false;
    }

    out << "#pragma once\n"
           "// AUTO-GENERATED by comdare-catalog-codegen. DO NOT EDIT.\n"
           "// Generated into CMAKE_BINARY_DIR/generated; source tree remains untouched.\n\n"
           "#include \"thesis_tiere/source_catalog.hpp\"\n\n"
           "namespace comdare::cache_engine::thesis_lazy {\n"
           "namespace generated_source_catalog_detail {\n\n";

    for (std::size_t i = 0; i < selected.size(); ++i) write_list(out, i, selected[i]);

    out << "} // namespace generated_source_catalog_detail\n\n"
           "struct GeneratedFullSourceCatalog {\n";
    for (std::size_t i = 0; i < selected.size(); ++i) {
        std::string const name = (i < 10 ? "L0" : "L") + std::to_string(i);
        out << "    using " << name << " = generated_source_catalog_detail::" << name << ";\n";
    }
    out << "\n"
           "    using Engine =\n"
           "        perm::PermutationEngine<CatalogCfg<L00>, CatalogCfg<L01>, CatalogCfg<L02>, CatalogCfg<L03>,\n"
           "                                CatalogCfg<L04>, CatalogCfg<L05>, CatalogCfg<L06>, CatalogCfg<L07>,\n"
           "                                CatalogCfg<L08>, CatalogCfg<L09>, CatalogCfg<L10>, CatalogCfg<L11>,\n"
           "                                CatalogCfg<L12>, CatalogCfg<L13>, CatalogCfg<L14>, CatalogCfg<L15>,\n"
           "                                CatalogCfg<L16>, CatalogCfg<L17>, CatalogCfg<L18>>;\n"
           "};\n\n"
           "[[nodiscard]] inline std::vector<ex::AxisLevel> generated_catalog_static_levels() {\n"
           "    return catalog_static_levels<GeneratedFullSourceCatalog>();\n"
           "}\n\n"
           "[[nodiscard]] inline ex::SourceGenFn generated_make_catalog_source_gen() {\n"
           "    return make_catalog_source_gen<GeneratedFullSourceCatalog>();\n"
           "}\n\n"
           "} // namespace comdare::cache_engine::thesis_lazy\n";
    return static_cast<bool>(out);
}

} // namespace

int main(int argc, char** argv) {
    std::optional<CliArgs> const args = parse_args(argc, argv);
    if (!args) return 1;

    cx::XmlConfigParser                    parser;
    std::optional<cx::ThesisProfile> const profile = parser.parse_thesis_profile(args->profile);
    if (!profile) {
        std::cerr << "catalog-codegen: cannot parse thesis profile: " << args->profile.string() << "\n";
        return 2;
    }

    std::array<SlotSpec, 19> const                 slots    = make_slots();
    std::optional<std::vector<SelectedSlot>> const selected = select_slots(slots, *profile);
    if (!selected) return 3;

    if (!write_header(args->out, *selected)) return 4;

    std::size_t count = 1;
    for (SelectedSlot const& slot : *selected) count *= slot.variants.size();
    std::cerr << "catalog-codegen: wrote " << args->out.string() << " from profile " << args->profile.string()
              << " (static permutations=" << count << ")\n";
    return 0;
}
