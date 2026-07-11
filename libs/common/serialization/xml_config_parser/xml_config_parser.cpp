// XML-Config-Parser Implementation
// Phase 6.4: minimaler XML-Tag-Reader (historisch Regex-Skelett).
// Phase 7 (2026-07-10, Parser-Konsolidierung): ALLE Parse-Pfade laufen ueber den
// self-contained KF-1-DOM (xml_reader.hpp) — ein Reader, keine Regex-Duplikate.

#include "xml_config_parser.hpp"
#include "xml_reader.hpp"

#include <cctype>
#include <charconv>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>

namespace comdare::builder::xml {

namespace {

std::string read_file(std::filesystem::path const& path) {
    std::ifstream      in{path};
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// Phase 7: rekursive DOM-Helfer — die alten Regexe fanden Tags auf beliebiger Tiefe;
// diese Helfer erhalten dieses Verhalten auf dem KF-1-DOM.
void collect_named(comdare::common::xml::XmlNode const& node, std::string_view tag,
                   std::vector<comdare::common::xml::XmlNode const*>& out) {
    for (auto const& c : node.children) {
        if (c.tag == tag) out.push_back(&c);
        collect_named(c, tag, out);
    }
}

[[nodiscard]] comdare::common::xml::XmlNode const* find_first_named(comdare::common::xml::XmlNode const& node,
                                                                    std::string_view                     tag) {
    for (auto const& c : node.children) {
        if (c.tag == tag) return &c;
        if (auto const* hit = find_first_named(c, tag)) return hit;
    }
    return nullptr;
}

// DOM-Fassung des frueheren Regex-Extraktors: <tag id="..."><nested>text</nested></tag>
std::vector<PermutationEntry> parse_entries_dom(comdare::common::xml::XmlNode const& root,
                                                std::string const&                   tag_name) {
    std::vector<comdare::common::xml::XmlNode const*> hits;
    if (root.tag == tag_name) hits.push_back(&root);
    collect_named(root, tag_name, hits);

    std::vector<PermutationEntry> entries;
    for (auto const* n : hits) {
        if (n->attrs.find("id") == n->attrs.end()) continue; // Regex verlangte id-Attribut
        PermutationEntry entry;
        entry.id = n->attr("id");
        for (auto const& c : n->children)
            if (!c.text.empty()) entry.attributes[c.tag] = c.text;
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
    // Phase 7: stille-{}-Falle bereinigt — Rueckgabe bleibt {} (Legacy-4-Datei-Schema ist
    // optional, R2-Entscheid Dataset-Wahrheitsquelle steht aus), aber NICHT mehr schweigend.
    if (!std::filesystem::exists(xml_file)) {
        std::cerr << "[xml-config] optionale Datei fehlt: " << xml_file.string()
                  << " (Legacy-4-Datei-Schema, vgl. R2-Entscheid)\n";
        return {};
    }
    std::string content = read_file(xml_file);
    auto        root    = comdare::common::xml::parse_document(content);
    if (root) {
        // Probiere verschiedene Tag-Namen aus (Verhalten des frueheren Regex-Pfads)
        for (auto const& tag :
             {"cache_engine_permutation", "search_algorithm", "allocator_permutation", "test_data_set"}) {
            auto entries = parse_entries_dom(*root, tag);
            if (!entries.empty()) return entries;
        }
    }
    std::cerr << "[xml-config] WARNUNG: " << xml_file.string()
              << " existiert, lieferte aber 0 Eintraege (kein bekannter Permutations-Tag / kein "
                 "wohlgeformtes XML)\n";
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
    auto        root    = comdare::common::xml::parse_document(content);
    if (!root) return prof;

    // Wurzel <comdare_algorithm_profile id=".." paper_ref=".." pruefling_type="..">
    // Review wf_8508f98c (CONFIRMED): KEIN Fallback auf fremde Wurzeln — die alte id-Regex war
    // auf comdare_algorithm_profile geankert; id=="" bleibt der Kein-Algorithm-Profil-Sentinel
    // (51 committete Nicht-SOTA-Akten wuerden sonst scheinbar gueltige Profile liefern).
    comdare::common::xml::XmlNode const* profile_node =
        (root->tag == "comdare_algorithm_profile") ? &*root : find_first_named(*root, "comdare_algorithm_profile");
    if (profile_node == nullptr) return prof;
    prof.id             = profile_node->attr("id");
    prof.paper_ref      = profile_node->attr("paper_ref");
    prof.pruefling_type = profile_node->attr("pruefling_type");

    // Achsen aus dem ERSTEN <axes>-Block (Regex nahm content.find("<axes>"))
    if (auto const* axes = find_first_named(*profile_node, "axes")) {
        for (auto const& c : axes->children)
            if (!c.text.empty()) prof.axes[c.tag] = c.text;
    }

    // key_value_signature + optionale Overrides: jeweils erste Fundstelle im Dokument.
    // BEWUSSTE KORREKTUR (Review wf_8508f98c, dokumentiert + im Regressions-Gate eingefroren):
    // der DOM dekodiert XML-Entities (&lt; -> <) — die Regex-Fassung transportierte Escapes
    // roh in generierte C++-Kommentare (einzige betroffene Akte: masstree key_types).
    if (auto const* kt = find_first_named(*profile_node, "key_types")) prof.key_types = kt->text;
    if (auto const* vt = find_first_named(*profile_node, "value_types")) prof.value_types = vt->text;
    // V19.1 — expected_workload (optional, ueberschreibt V11.2-Heuristik)
    if (auto const* ew = find_first_named(*profile_node, "expected_workload")) prof.expected_workload = ew->text;
    // V29.A — allocator_override (optional, ueberschreibt axes/allocator)
    if (auto const* ao = find_first_named(*profile_node, "allocator_override")) prof.allocator_override = ao->text;
    return prof;
}

// Verhaltens-Referenz (Review wf_8508f98c, empirisch): diff-frei zur frueher produktiven
// TREIBER-Regex auf allen 165 realen Akten (inkl. config_a/b/c mit Mehrfach-Attributen und
// <pruefling><profile>-Refs); die engere alte BIBLIOTHEKS-Regex (id als einziges Attribut,
// 0 Reihen fuer config_a/b/c) war nie produktiver Konsument.
std::vector<Messreihe> XmlConfigParser::load_messreihen(std::filesystem::path const& messreihen_xml) const {
    std::vector<Messreihe> result;
    if (!std::filesystem::exists(messreihen_xml)) return result;
    std::string content = read_file(messreihen_xml);
    auto        root    = comdare::common::xml::parse_document(content);
    if (!root) return result;

    std::vector<comdare::common::xml::XmlNode const*> reihen;
    if (root->tag == "messreihe") reihen.push_back(&*root);
    collect_named(*root, "messreihe", reihen);
    for (auto const* rn : reihen) {
        if (rn->attrs.find("id") == rn->attrs.end()) continue; // Regex verlangte id-Attribut
        Messreihe r;
        r.id = rn->attr("id");
        if (auto const* mo = find_first_named(*rn, "mode")) r.mode = parse_mode(mo->text);
        std::vector<comdare::common::xml::XmlNode const*> profs;
        collect_named(*rn, "profile", profs);
        for (auto const* pn : profs)
            if (!pn->text.empty()) r.sota_profile_refs.push_back(pn->text);
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
        // Die 4 RC-POD-Felder sind laut SCHEMA uint64-Listen. STRIKT validieren (jedes Token
        // vollstaendig numerisch), sonst wuerde parse_u64 spaeter nicht-numerisch still auf 0
        // mappen = falsch etikettierte Mess-Zeilen. Ungueltig => Profil unlesbar (nullopt).
        auto const parse_u64_list_strict = [](auto const* node, std::vector<std::string>& dst) -> bool {
            if (node == nullptr) return true;
            std::vector<std::string> toks = node->text_tokens();
            for (std::string const& t : toks) {
                std::uint64_t v      = 0;
                auto const [ptr, ec] = std::from_chars(t.data(), t.data() + t.size(), v, 10);
                if (ec != std::errc{} || ptr != t.data() + t.size()) return false;
            }
            dst = std::move(toks);
            return true;
        };
        if (!parse_u64_list_strict(rd->child("prefetch_distance"), tp.prefetch_distances)) return std::nullopt;
        if (!parse_u64_list_strict(rd->child("pool_budget_bytes"), tp.pool_budgets_bytes)) return std::nullopt;
        if (!parse_u64_list_strict(rd->child("batch_size"), tp.batch_sizes)) return std::nullopt;
        if (!parse_u64_list_strict(rd->child("inline_threshold_bytes"), tp.inline_thresholds_bytes))
            return std::nullopt;
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
        tp.run_options.n_ops         = std::strtoull(ro->attr("n_ops", "0").c_str(), nullptr, 10); // G5
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
