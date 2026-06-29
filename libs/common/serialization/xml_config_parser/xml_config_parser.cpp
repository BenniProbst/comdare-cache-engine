// XML-Config-Parser Implementation (Skelett)
// Phase 6.4: minimaler XML-Tag-Reader; produktiv via tinyxml2 oder eigener Parser

#include "xml_config_parser.hpp"
#include "xml_reader.hpp"

#include <cctype>
#include <charconv>
#include <fstream>
#include <optional>
#include <regex>
#include <sstream>

namespace comdare::builder::xml {

namespace {

std::string read_file(std::filesystem::path const& path) {
    std::ifstream      in{path};
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// Sehr simpler XML-Tag-Extraktor: <tag id="..."><nested>...</nested></tag>
std::vector<PermutationEntry> parse_xml_string(std::string const& content, std::string const& tag_name) {
    std::vector<PermutationEntry> entries;
    std::regex                    open_tag{"<" + tag_name + "\\s+id\\s*=\\s*\"([^\"]+)\"\\s*>"};
    std::regex                    inner_tag{"<(\\w+)>([^<]+)</\\w+>"};
    auto                          it  = std::sregex_iterator(content.begin(), content.end(), open_tag);
    auto                          end = std::sregex_iterator();
    for (; it != end; ++it) {
        PermutationEntry entry;
        entry.id = (*it)[1].str();
        // Find closing tag
        auto        close_pos = content.find("</" + tag_name + ">", it->position() + it->length());
        std::string inner    = content.substr(it->position() + it->length(), close_pos - it->position() - it->length());
        auto        inner_it = std::sregex_iterator(inner.begin(), inner.end(), inner_tag);
        auto        inner_end = std::sregex_iterator();
        for (; inner_it != inner_end; ++inner_it) { entry.attributes[(*inner_it)[1].str()] = (*inner_it)[2].str(); }
        entries.push_back(std::move(entry));
    }
    return entries;
}

// KF-1 (2026-06-02): Helfer fuer den Thesis-Profil-Parser.
[[nodiscard]] std::vector<std::string> split_ws(std::string const& s) {
    std::vector<std::string> out;
    std::size_t              i = 0;
    while (i < s.size()) {
        while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
        std::size_t start = i;
        while (i < s.size() && !std::isspace(static_cast<unsigned char>(s[i]))) ++i;
        if (i > start) out.push_back(s.substr(start, i - start));
    }
    return out;
}

[[nodiscard]] int to_int(std::string const& s, int def) {
    int         v     = def;
    auto const* begin = s.data();
    auto const* end   = s.data() + s.size();
    auto [ptr, ec]    = std::from_chars(begin, end, v);
    (void)ptr;
    return (ec == std::errc{}) ? v : def;
}

} // anonymous namespace

CacheEngineConfig XmlConfigParser::parse(std::filesystem::path const& root_dir) const {
    CacheEngineConfig cfg;
    cfg.cache_engine_permutations     = parse_one(root_dir / "cache_engine_permutations.xml");
    cfg.search_algorithm_permutations = parse_one(root_dir / "search_algorithm_permutations.xml");
    cfg.allocator_permutations        = parse_one(root_dir / "allocator_permutations.xml");
    cfg.test_data_sets                = parse_one(root_dir / "test_data_sets.xml");

    // REV 7.6 V8.6 — Optional: algorithm_profiles + Messreihen
    auto profiles_dir = root_dir / "algorithm_profiles";
    if (std::filesystem::is_directory(profiles_dir / "sota")) {
        cfg.sota_profiles = load_sota_profiles(profiles_dir / "sota");
    }
    auto messreihen_xml = root_dir / "messreihen.xml";
    if (std::filesystem::exists(messreihen_xml)) { cfg.messreihen = load_messreihen(messreihen_xml); }
    return cfg;
}

std::vector<PermutationEntry> XmlConfigParser::parse_one(std::filesystem::path const& xml_file) const {
    if (!std::filesystem::exists(xml_file)) return {};
    std::string content = read_file(xml_file);

    // Probiere verschiedene Tag-Namen aus
    for (auto const& tag : {"cache_engine_permutation", "search_algorithm", "allocator_permutation", "test_data_set"}) {
        auto entries = parse_xml_string(content, tag);
        if (!entries.empty()) return entries;
    }
    return {};
}

// REV 7.6 V8.6 — algorithm_profiles/-Loader
std::vector<AlgorithmProfile> XmlConfigParser::load_sota_profiles(std::filesystem::path const& sota_dir) const {
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
        std::regex  id_re{R"PAT(<comdare_algorithm_profile\s+id\s*=\s*"([^"]+)")PAT"};
        std::smatch m;
        if (std::regex_search(content, m, id_re)) prof.id = m[1].str();
    }
    {
        std::regex  pref_re{R"PAT(paper_ref\s*=\s*"([^"]+)")PAT"};
        std::smatch m;
        if (std::regex_search(content, m, pref_re)) prof.paper_ref = m[1].str();
    }

    // Achsen aus <axes>...</axes>
    auto axes_open  = content.find("<axes>");
    auto axes_close = content.find("</axes>");
    if (axes_open != std::string::npos && axes_close != std::string::npos) {
        std::string axes_inner = content.substr(axes_open + 6, axes_close - axes_open - 6);
        std::regex  axis_re{R"(<(\w+)>([^<]+)</\w+>)"};
        auto        it  = std::sregex_iterator(axes_inner.begin(), axes_inner.end(), axis_re);
        auto        end = std::sregex_iterator();
        for (; it != end; ++it) { prof.axes[(*it)[1].str()] = (*it)[2].str(); }
    }

    // key_value_signature
    {
        std::regex  kt{R"(<key_types>([^<]+)</key_types>)"};
        std::smatch m;
        if (std::regex_search(content, m, kt)) prof.key_types = m[1].str();
    }
    {
        std::regex  vt{R"(<value_types>([^<]+)</value_types>)"};
        std::smatch m;
        if (std::regex_search(content, m, vt)) prof.value_types = m[1].str();
    }
    // V19.1 — expected_workload (optional, ueberschreibt V11.2-Heuristik)
    {
        std::regex  ew{R"(<expected_workload>([^<]+)</expected_workload>)"};
        std::smatch m;
        if (std::regex_search(content, m, ew)) prof.expected_workload = m[1].str();
    }
    // V29.A — allocator_override (optional, ueberschreibt axes/allocator)
    {
        std::regex  ao{R"(<allocator_override>([^<]+)</allocator_override>)"};
        std::smatch m;
        if (std::regex_search(content, m, ao)) prof.allocator_override = m[1].str();
    }
    return prof;
}

std::vector<Messreihe> XmlConfigParser::load_messreihen(std::filesystem::path const& messreihen_xml) const {
    std::vector<Messreihe> result;
    if (!std::filesystem::exists(messreihen_xml)) return result;
    std::string content = read_file(messreihen_xml);

    std::regex           reihe_re{R"PAT(<messreihe\s+id\s*=\s*"([^"]+)"\s*>([\s\S]*?)</messreihe>)PAT"};
    std::sregex_iterator reihe_it{content.cbegin(), content.cend(), reihe_re};
    std::sregex_iterator reihe_end{};
    for (; reihe_it != reihe_end; ++reihe_it) {
        Messreihe r;
        r.id              = (*reihe_it)[1].str();
        std::string inner = (*reihe_it)[2].str();

        std::regex  mode_re{R"(<mode>(\w+)</mode>)"};
        std::smatch mm;
        if (std::regex_search(inner, mm, mode_re)) r.mode = parse_mode(mm[1].str());

        std::regex           prof_re{R"(<profile>([^<]+)</profile>)"};
        std::sregex_iterator prof_it{inner.cbegin(), inner.cend(), prof_re};
        std::sregex_iterator prof_end{};
        for (; prof_it != prof_end; ++prof_it) { r.sota_profile_refs.push_back((*prof_it)[1].str()); }
        result.push_back(std::move(r));
    }
    return result;
}

// KF-1 (2026-06-02): comdare_thesis_profile-Parser ueber den self-contained XML-DOM.
std::optional<ThesisProfile> XmlConfigParser::parse_thesis_profile(std::filesystem::path const& xml_file) const {
    if (!std::filesystem::exists(xml_file)) return std::nullopt;
    std::string content = read_file(xml_file);
    auto        root    = comdare::common::xml::parse_document(content);
    if (!root || root->tag != "comdare_thesis_profile") return std::nullopt;

    ThesisProfile tp;
    tp.id             = root->attr("id");
    tp.schema_version = to_int(root->attr("schema_version", "0"), 0);

    if (auto const* bt = root->child("base_tiers")) {
        for (auto const* t : bt->children_named("tier"))
            tp.base_tiers.push_back({t->attr("id"), t->attr("profile_ref"), t->attr("paper_ref")});
    }
    if (auto const* pa = root->child("permute_axes")) {
        for (auto const* a : pa->children_named("axis")) {
            ThesisAxisSpec ax;
            ax.ref = a->attr("ref");
            for (auto const* v : a->children_named("value")) ax.values.push_back(v->text);
            if (ax.ref == "cacheline") { // per-Organ Cache-Line-Unterachse (KF-3)
                ax.per_organ = split_ws(a->attr("per_organ"));
                for (auto const* x : a->children_named("line_size")) ax.line_sizes.push_back(x->text);
                for (auto const* x : a->children_named("alignment")) ax.alignments.push_back(x->text);
                for (auto const* x : a->children_named("sw_prefetch_hint")) ax.sw_prefetch_hints.push_back(x->text);
            }
            tp.permute_axes.push_back(std::move(ax));
        }
    }
    if (auto const* cd = root->child("compile_dims")) {
        if (auto const* w = cd->child("workloads")) tp.workloads = w->text_tokens();
        if (auto const* t = cd->child("telemetry")) {
            tp.telemetry_mode   = t->attr("mode");
            tp.telemetry_silent = (t->attr("silent") == "true");
        }
    }
    if (auto const* rd = root->child("runtime_dynamic")) {
        if (auto const* tc = rd->child("thread_count")) tp.thread_counts = tc->text_tokens();
        if (auto const* hp = rd->child("hw_prefetcher")) tp.hw_prefetcher = hp->text_tokens();
    }
    if (auto const* fc = root->child("fixed_conditions")) tp.fixed_conditions = fc->attrs;
    if (auto const* rep = root->child("repetitions")) {
        tp.repetitions             = to_int(rep->attr("count", "3"), 3);
        tp.repetitions_interpolate = (rep->attr("interpolate", "false") == "true");
        tp.repetitions_overlay     = (rep->attr("overlay_in_chart", "true") == "true");
    }
    if (auto const* ms = root->child("modes")) {
        for (auto const* m : ms->children_named("mode")) {
            ThesisMode md;
            md.name          = m->attr("name");
            md.merge         = m->attr("merge");
            md.active_axes   = split_ws(m->attr("active_axes"));
            md.pruefling     = m->attr("pruefling");
            md.replaces_axes = split_ws(m->attr("replaces_axes"));
            tp.modes.push_back(std::move(md));
        }
    }
    if (auto const* sa = root->child("static_axes")) tp.static_axes_from = sa->attr("from");
    if (auto const* kv = root->child("key_value_signature")) {
        if (auto const* k = kv->child("key_types")) tp.key_types = k->text;
        if (auto const* v = kv->child("value_types")) tp.value_types = v->text;
    }

    // ── S1 (Increment 2, 2026-06-18): die 4 deklarativen m3v2-Selektions-Konstrukte. ADDITIV — fehlen sie,
    //    bleiben die Felder leer/Default (kein Einfluss auf base_pilot/cacheline_study oder den Round-Trip). ──
    // (a) <working_set_sweep>{N-Liste}</working_set_sweep> — whitespace-getrennte Record-Zahlen (wie thread_count).
    if (auto const* ws = root->child("working_set_sweep")) tp.working_set_sweep = ws->text_tokens();
    // (b) <axis_sweeps>/<axis_sweep axis=".." baseline="index0"/> — je eine Achse gegen die feste Baseline.
    if (auto const* sw = root->child("axis_sweeps")) {
        for (auto const* a : sw->children_named("axis_sweep")) {
            ThesisAxisSweep as;
            as.axis     = a->attr("axis");
            as.baseline = a->attr("baseline", "index0");
            tp.axis_sweeps.push_back(std::move(as));
        }
    }
    // (c) <sota_series_set>/<sota_series id=".." lebewesen=".." merge=".."/> — die SOTA-/PRT-ART-Reihen A/B/C.
    if (auto const* ss = root->child("sota_series_set")) {
        for (auto const* s : ss->children_named("sota_series")) {
            ThesisSotaSeries sr;
            sr.id             = s->attr("id");
            sr.lebewesen      = s->attr("lebewesen");
            sr.merge          = s->attr("merge");
            sr.pruefling_type = s->attr("pruefling_type"); // #171: optional; leer ⇒ aus merge abgeleitet
            tp.sota_series.push_back(std::move(sr));
        }
    }
    // (d) <run_options cap=".." platform=".." build_version=".." resume=".."/> — Lauf-Steuerungs-Defaults.
    if (auto const* ro = root->child("run_options")) {
        tp.run_options.cap           = to_int(ro->attr("cap", "0"), 0);
        tp.run_options.platform      = ro->attr("platform");
        tp.run_options.build_version = ro->attr("build_version");
        if (ro->has_attr("resume")) {
            tp.run_options.resume     = (ro->attr("resume") == "true" || ro->attr("resume") == "1");
            tp.run_options.resume_set = true;
        }
    }
    return tp;
}

} // namespace comdare::builder::xml
