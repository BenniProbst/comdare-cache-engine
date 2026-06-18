#pragma once
// load_profile_parser.hpp — comdare_load_profile-XML → WorkloadConfig (Achse-2-Lastprofile, User 2026-06-08).
//
// Liest ein Lastprofil-XML (Schema: algorithm_profiles/load_profiles/SCHEMA.md) mit dem self-contained Reader
// (comdare::common::xml, KF-1) und mappt es auf eine WorkloadConfig. Die Workload-CHARAKTERISTIK (op-mix,
// key_distribution, negative_query_pct, scan_length) kommt aus dem XML; die SKALA (records, n_ops) setzt der
// Aufrufer (Harness) — so läuft dasselbe Lastprofil test- (klein) und voll-skaliert (gross) mit identischer
// Charakteristik. Die Lastprofile sind die Werte der dynamischen Workload-Achse (Achse 2) im Experiment-B+-Baum.
//
// @related [[feedback_all_papers_loadprofiles_xml_all_axes]] Doc 32 (Lastprofil-Katalog)

#include "workload_config.hpp"
#include <serialization/xml_config_parser/xml_reader.hpp>   // comdare::common::xml::parse_document / XmlNode

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::workload_driver {

namespace cx = ::comdare::common::xml;

/// Ein geparstes Lastprofil. `id` ist der Workload-Achsen-Wert (= profile_name in der CSV). `records`/`num_operations`
/// sind XML-Defaults (Skala); der Aufrufer darf sie mit der Harness-Skala überschreiben.
struct LoadProfile {
    std::string    id;
    std::string    paper_ref;
    std::string    pretty_name;
    std::uint64_t  records        = 0;   // XML-Default Load-Records (0 → Aufrufer wählt, typ. = n_ops)
    std::uint64_t  num_operations = 0;   // XML-Default Run-Ops    (0 → Aufrufer wählt)
    WorkloadConfig config{};             // op-mix/dist/neg%/scan aus XML (name bleibt Default; Aufrufer nutzt id)
};

namespace detail {
[[nodiscard]] inline std::string lp_read_file(std::filesystem::path const& p) {
    std::ifstream in{p, std::ios::binary};
    std::ostringstream ss; ss << in.rdbuf();
    return ss.str();
}
[[nodiscard]] inline double lp_text_d(cx::XmlNode const* n, double def) noexcept {
    if (n == nullptr || n->text.empty()) return def;
    try { return std::stod(n->text); } catch (...) { return def; }
}
[[nodiscard]] inline std::uint64_t lp_text_u(cx::XmlNode const* n, std::uint64_t def) noexcept {
    if (n == nullptr || n->text.empty()) return def;
    try { return static_cast<std::uint64_t>(std::stoull(n->text)); } catch (...) { return def; }
}
[[nodiscard]] inline double lp_attr_d(cx::XmlNode const* n, std::string_view k, double def) noexcept {
    if (n == nullptr || !n->has_attr(k)) return def;
    try { return std::stod(n->attr(k)); } catch (...) { return def; }
}
[[nodiscard]] inline KeyDistribution lp_keydist(std::string const& s) noexcept {
    if (s == "zipfian") return KeyDistribution::Zipfian;
    if (s == "latest")  return KeyDistribution::Latest;
    return KeyDistribution::Uniform;   // Default + Fallback für (noch) nicht unterstützte (real-corpus/lognormal)
}
}  // namespace detail

/// parse_load_profile — comdare_load_profile-XML → LoadProfile. nullopt bei fehlender/falscher Datei / falschem Wurzel-Tag.
[[nodiscard]] inline std::optional<LoadProfile> parse_load_profile(std::filesystem::path const& xml_file) {
    std::string const content = detail::lp_read_file(xml_file);
    auto root = cx::parse_document(content);
    if (!root.has_value() || root->tag != "comdare_load_profile") return std::nullopt;

    LoadProfile lp;
    lp.id        = root->attr("id");
    lp.paper_ref = root->attr("paper_ref");
    if (auto const* meta = root->child("metadata"))
        if (auto const* nm = meta->child("name")) lp.pretty_name = nm->text;

    auto const* wl = root->child("workload");
    if (wl == nullptr || lp.id.empty()) return std::nullopt;

    WorkloadConfig& c = lp.config;
    c.seed            = detail::lp_text_u(wl->child("seed"), 42);
    if (c.seed == 0) c.seed = 42;
    lp.records        = detail::lp_text_u(wl->child("records"), 0);
    lp.num_operations = detail::lp_text_u(wl->child("num_operations"), 0);
    // MAJOR-MESS-05 (Mess-Validität, Audit A1): <op_mix> ist die Workload-DEFINITION (Achse 2). Fehlt der Knoten,
    // würde der WorkloadConfig-Default (50/40/9/1 inkl. 1% Clear) STILL eingesetzt → ein anderes Profil gemessen als
    // dokumentiert. Darum HART ablehnen statt still defaulten: kein <op_mix> ⇒ kein gültiges Lastprofil (nullopt).
    auto const* om = wl->child("op_mix");
    if (om == nullptr) return std::nullopt;
    c.pct_insert = detail::lp_attr_d(om, "insert", 0.0);
    c.pct_lookup = detail::lp_attr_d(om, "lookup", 0.0);
    c.pct_erase  = detail::lp_attr_d(om, "erase",  0.0);
    c.pct_clear  = detail::lp_attr_d(om, "clear",  0.0);
    c.pct_scan   = detail::lp_attr_d(om, "scan",   0.0);
    c.pct_rmw    = detail::lp_attr_d(om, "rmw",    0.0);
    // …und ein LEERES op_mix (alle Anteile 0 / keine Op-Attribute) ist ebenso ungültig — sonst liefe ein Profil ohne
    // jede Operation (Summe 0 → WorkloadConfig::is_valid()==false ⇒ Generator normalisiert nicht). Ehrlich ablehnen.
    if ((c.pct_insert + c.pct_lookup + c.pct_erase + c.pct_clear + c.pct_scan + c.pct_rmw) <= 0.0)
        return std::nullopt;
    auto const* kd = wl->child("key_distribution");
    c.key_distribution   = detail::lp_keydist(kd != nullptr ? kd->text : std::string{"uniform"});
    c.zipfian_theta      = detail::lp_text_d(wl->child("zipfian_theta"), 0.99);
    c.negative_query_pct = detail::lp_text_d(wl->child("negative_query_pct"), 0.0);
    c.scan_length_max    = detail::lp_text_u(wl->child("scan_length_max"), 100);
    // Skala: XML-Default für num_operations/key-range; der Aufrufer überschreibt mit der Harness-Skala.
    c.num_operations     = (lp.num_operations > 0) ? static_cast<std::size_t>(lp.num_operations) : 1000;
    c.key_min            = 1;
    c.key_max            = (lp.records > 1) ? lp.records : 1'000'000ULL;
    return lp;
}

/// discover_load_profiles — alle gültigen *.xml-Lastprofile in `dir` → [(id, pfad)], alphabetisch nach id.
/// Das ist die Werte-Menge der dynamischen Workload-Achse (Achse 2).
[[nodiscard]] inline std::vector<std::pair<std::string, std::filesystem::path>>
discover_load_profiles(std::filesystem::path const& dir) {
    std::vector<std::pair<std::string, std::filesystem::path>> out;
    std::error_code ec;
    if (!std::filesystem::is_directory(dir, ec)) return out;
    for (auto const& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".xml") continue;
        if (auto lp = parse_load_profile(entry.path())) out.emplace_back(lp->id, entry.path());
    }
    std::sort(out.begin(), out.end(), [](auto const& a, auto const& b) { return a.first < b.first; });
    return out;
}

}  // namespace comdare::cache_engine::builder::workload_driver
