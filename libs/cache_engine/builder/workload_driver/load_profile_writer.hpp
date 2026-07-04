#pragma once
// load_profile_writer.hpp — LoadProfile → comdare_load_profile-XML (INVERSE zu load_profile_parser.hpp).
//
// #175 (Goal §8.3 Versprechen 3 / AP-X2 TODO-3): Die "Extraktion" als wiederverwendbares XML-Lastprofil-Ergebnis
// je Architekturfokus ABLEGEN, um Heuristiken automatisch zu erzeugen. Die IMPORT-Seite (parse_load_profile,
// discover_load_profiles) existiert voll — hier kommt die EXPORT-Seite (Schreiben) + der Extraktor dazu.
//
// Begründung WRITER statt DOM-Serialisierung: der self-contained DOM comdare::common::xml (xml_reader.hpp, KF-1)
// ist NUR LESEND (parse_document → XmlNode + Accessoren; KEIN serialize/to_string/write). Darum ein kleiner,
// schema-treuer XML-Writer, der GENAU das Schema von parse_load_profile (root 'comdare_load_profile' +
// <metadata><name> + <workload> mit seed/records/num_operations/<op_mix .../>/key_distribution/zipfian_theta/
// negative_query_pct/scan_length_max) erzeugt — sonst bricht der Round-Trip.
//
// Pattern: BUILDER (LoadProfileXmlBuilder) für den schrittweisen, schema-treuen XML-Aufbau — analog zum
// Builder/Strategy/Repository-Gebrauch in best_binary_selector. Der Extraktor nutzt das gleiche
// header-getriebene CSV-Aggregat-Muster wie best_binary_selector::parse_measurement_csv.
//
// @related load_profile_parser.hpp (parse_load_profile = die INVERSE); SCHEMA-Vorlage ycsb_c.xml.

#include "load_profile_parser.hpp" // LoadProfile, WorkloadConfig, KeyDistribution
#include "workload_config.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::builder::workload_driver {

namespace detail {

/// XML-Entity-Encode der 5 Basis-Entities — INVERSE zu xml_reader.hpp detail::decode_entities (Z61-72).
/// Schreib-Pflicht, sonst liest der Reader Sonderzeichen falsch ein. '&' zuerst, sonst Doppel-Encode.
[[nodiscard]] inline std::string xml_encode(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&apos;"; break;
            default: out += c; break;
        }
    }
    return out;
}

/// Brüche schema-treu formatieren: feste 6 Nachkommastellen (semantisch round-trip-sicher; der Parser nutzt
/// std::stod, also reicht ausreichende Präzision). Ganzzahlige Werte ohne überflüssige Nachkommastellen.
[[nodiscard]] inline std::string fmt_d(double v) {
    std::ostringstream ss;
    ss.precision(6);
    ss << std::fixed << v;
    return ss.str();
}

/// key_distribution-Rückabbildung — INVERSE zu lp_keydist (load_profile_parser.hpp Z58-62).
[[nodiscard]] inline std::string keydist_name(KeyDistribution d) noexcept {
    switch (d) {
        case KeyDistribution::Zipfian: return "zipfian";
        case KeyDistribution::Latest: return "latest";
        case KeyDistribution::Uniform:
        default: return "uniform";
    }
}

} // namespace detail

// ─────────────────────────────────────────────────────────────────────────────
// LoadProfileXmlBuilder — BUILDER-Pattern für den schema-treuen comdare_load_profile-XML-Aufbau.
// ─────────────────────────────────────────────────────────────────────────────
/// Baut den XML-String Element für Element exakt im Schema von parse_load_profile auf. Jeder Setter gibt *this
/// zurück (fluent). build() liefert den fertigen, wohlgeformten XML-String (mit <?xml?>-Prolog).
class LoadProfileXmlBuilder {
public:
    LoadProfileXmlBuilder& id(std::string v) {
        id_ = std::move(v);
        return *this;
    }
    LoadProfileXmlBuilder& paper_ref(std::string v) {
        paper_ref_ = std::move(v);
        return *this;
    }
    LoadProfileXmlBuilder& catalog_lp_id(std::string v) {
        catalog_lp_id_ = std::move(v);
        return *this;
    }
    LoadProfileXmlBuilder& pretty_name(std::string v) {
        pretty_name_ = std::move(v);
        return *this;
    }
    LoadProfileXmlBuilder& records(std::uint64_t v) {
        records_ = v;
        return *this;
    }
    LoadProfileXmlBuilder& num_operations(std::uint64_t v) {
        num_operations_ = v;
        return *this;
    }
    LoadProfileXmlBuilder& config(WorkloadConfig const& c) {
        config_ = c;
        return *this;
    }

    [[nodiscard]] std::string build() const {
        WorkloadConfig const& c = config_;
        std::ostringstream    o;
        o << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        o << "<!-- comdare_load_profile (auto-extrahiert via write_load_profile_xml, #175). Schema = "
             "parse_load_profile. -->\n";
        o << "<comdare_load_profile id=\"" << detail::xml_encode(id_) << "\" paper_ref=\""
          << detail::xml_encode(paper_ref_) << "\"";
        if (!catalog_lp_id_.empty()) o << " lp_id=\"" << detail::xml_encode(catalog_lp_id_) << "\"";
        o << " schema_version=\"1\">\n";
        o << "  <metadata>\n";
        o << "    <name>" << detail::xml_encode(pretty_name_) << "</name>\n";
        o << "  </metadata>\n";
        o << "  <workload>\n";
        o << "    <seed>" << c.seed << "</seed>\n";
        o << "    <records>" << records_ << "</records>\n";
        o << "    <num_operations>" << num_operations_ << "</num_operations>\n";
        o << "    <op_mix"
          << " insert=\"" << detail::fmt_d(c.pct_insert) << "\""
          << " lookup=\"" << detail::fmt_d(c.pct_lookup) << "\""
          << " erase=\"" << detail::fmt_d(c.pct_erase) << "\""
          << " clear=\"" << detail::fmt_d(c.pct_clear) << "\""
          << " scan=\"" << detail::fmt_d(c.pct_scan) << "\""
          << " rmw=\"" << detail::fmt_d(c.pct_rmw) << "\"/>\n";
        o << "    <key_distribution>" << detail::keydist_name(c.key_distribution) << "</key_distribution>\n";
        o << "    <zipfian_theta>" << detail::fmt_d(c.zipfian_theta) << "</zipfian_theta>\n";
        o << "    <negative_query_pct>" << detail::fmt_d(c.negative_query_pct) << "</negative_query_pct>\n";
        o << "    <scan_length_max>" << c.scan_length_max << "</scan_length_max>\n";
        o << "  </workload>\n";
        o << "</comdare_load_profile>\n";
        return o.str();
    }

private:
    std::string    id_;
    std::string    paper_ref_;
    std::string    catalog_lp_id_;
    std::string    pretty_name_;
    std::uint64_t  records_        = 0;
    std::uint64_t  num_operations_ = 0;
    WorkloadConfig config_{};
};

/// write_load_profile_xml — LoadProfile → comdare_load_profile-XML-String. INVERSE zu parse_load_profile.
[[nodiscard]] inline std::string write_load_profile_xml(LoadProfile const& lp) {
    return LoadProfileXmlBuilder{}
        .id(lp.id)
        .paper_ref(lp.paper_ref)
        .catalog_lp_id(lp.catalog_lp_id)
        .pretty_name(lp.pretty_name)
        .records(lp.records)
        .num_operations(lp.num_operations)
        .config(lp.config)
        .build();
}

/// write_load_profile_xml — LoadProfile → Datei. Legt nur eine NEUE Export-Datei an (Bestands-LP-XMLs unberührt).
inline bool write_load_profile_xml(LoadProfile const& lp, std::filesystem::path const& out_file) {
    std::error_code ec;
    if (out_file.has_parent_path()) std::filesystem::create_directories(out_file.parent_path(), ec);
    std::ofstream out{out_file, std::ios::binary | std::ios::trunc};
    if (!out) return false;
    out << write_load_profile_xml(lp);
    return static_cast<bool>(out);
}

// ─────────────────────────────────────────────────────────────────────────────
// extract_load_profile_from_measurements — Mess-CSV je Architekturfokus → LoadProfile.
// Header-getriebenes ';'-CSV-Aggregat (Muster best_binary_selector::parse_measurement_csv): summiert die
// per-Zeile-Operations-ZÄHLER op_<art>_n je search_algo/sota_tier → dominante op-mix-Brüche (count/Summe) +
// Working-Set (working_set_n). Der Architekturfokus steckt im binary_id-Feld als 'search_algo=<X>/...' bzw.
// alleiniges 'sota_tier=<X>'; beide Präfixe abschneiden + am ersten '/' kappen.
// ─────────────────────────────────────────────────────────────────────────────

namespace detail {

[[nodiscard]] inline std::vector<std::string> lpw_split_semicolon(std::string const& line) {
    std::vector<std::string> out;
    std::string              cur;
    for (char ch : line) {
        if (ch == ';') {
            out.push_back(cur);
            cur.clear();
        } else if (ch != '\r')
            cur += ch;
    }
    out.push_back(cur);
    return out;
}

[[nodiscard]] inline std::uint64_t lpw_to_u64(std::string const& s) noexcept {
    if (s.empty()) return 0;
    try {
        return static_cast<std::uint64_t>(std::stoull(s));
    } catch (...) { return 0; }
}

/// Architekturfokus aus binary_id: 'search_algo='/'sota_tier='-Präfix abschneiden, am ersten '/' kappen.
[[nodiscard]] inline std::string lpw_focus_of(std::string const& binary_id) {
    std::string_view v{binary_id};
    if (v.rfind("search_algo=", 0) == 0)
        v.remove_prefix(std::string_view{"search_algo="}.size());
    else if (v.rfind("sota_tier=", 0) == 0)
        v.remove_prefix(std::string_view{"sota_tier="}.size());
    auto slash = v.find('/');
    if (slash != std::string_view::npos) v = v.substr(0, slash);
    return std::string{v};
}

} // namespace detail

/// extract_load_profile_from_measurements — aggregiert die Mess-CSV für GENAU einen Architekturfokus zu einem
/// LoadProfile (dominante op-mix + Working-Set). nullopt bei fehlender CSV / unbekanntem Fokus / Summe 0.
[[nodiscard]] inline std::optional<LoadProfile> extract_load_profile_from_measurements(std::filesystem::path const& csv,
                                                                                       std::string const& search_algo) {
    std::ifstream f{csv};
    if (!f) return std::nullopt;

    std::string header_line;
    if (!std::getline(f, header_line)) return std::nullopt;
    std::vector<std::string> const headers = detail::lpw_split_semicolon(header_line);

    std::map<std::string, std::size_t> col;
    for (std::size_t i = 0; i < headers.size(); ++i) col[headers[i]] = i;
    auto need = [&](char const* name, std::size_t& idx) -> bool {
        auto it = col.find(name);
        if (it == col.end()) return false;
        idx = it->second;
        return true;
    };

    std::size_t i_bid = 0, i_ins = 0, i_lk = 0, i_er = 0, i_cl = 0, i_sc = 0, i_rmw = 0, i_ws = 0;
    if (!need("binary_id", i_bid)) return std::nullopt;
    if (!need("op_insert_n", i_ins)) return std::nullopt;
    if (!need("op_lookup_n", i_lk)) return std::nullopt;
    if (!need("op_erase_n", i_er)) return std::nullopt;
    if (!need("op_clear_n", i_cl)) return std::nullopt;
    if (!need("op_scan_n", i_sc)) return std::nullopt;
    if (!need("op_rmw_n", i_rmw)) return std::nullopt;
    bool const has_ws = need("working_set_n", i_ws);

    std::uint64_t ins = 0, lk = 0, er = 0, cl = 0, sc = 0, rmw = 0;
    std::uint64_t working_set  = 0;
    bool          ws_seen      = false;
    std::size_t   matched_rows = 0;

    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        std::vector<std::string> const fld = detail::lpw_split_semicolon(line);
        if (fld.size() <= i_bid) continue;
        if (detail::lpw_focus_of(fld[i_bid]) != search_algo) continue;
        ++matched_rows;
        if (fld.size() > i_ins) ins += detail::lpw_to_u64(fld[i_ins]);
        if (fld.size() > i_lk) lk += detail::lpw_to_u64(fld[i_lk]);
        if (fld.size() > i_er) er += detail::lpw_to_u64(fld[i_er]);
        if (fld.size() > i_cl) cl += detail::lpw_to_u64(fld[i_cl]);
        if (fld.size() > i_sc) sc += detail::lpw_to_u64(fld[i_sc]);
        if (fld.size() > i_rmw) rmw += detail::lpw_to_u64(fld[i_rmw]);
        if (has_ws && fld.size() > i_ws) {
            std::uint64_t const w = detail::lpw_to_u64(fld[i_ws]);
            if (w > 0) {
                working_set = w;
                ws_seen     = true;
            } // Working-Set je Fokus konstant (Pilot: 16384)
        }
    }

    if (matched_rows == 0) return std::nullopt;
    std::uint64_t const total = ins + lk + er + cl + sc + rmw;
    if (total == 0) return std::nullopt; // keine Operation ausgeübt → kein gültiges Lastprofil

    double const inv = 1.0 / static_cast<double>(total);
    LoadProfile  lp;
    lp.id          = "extracted_" + search_algo;
    lp.paper_ref   = "EXTRACTED";
    lp.pretty_name = "Extrahiertes Lastprofil (Architekturfokus " + search_algo + ")";
    // Working-Set als records-Default (= XML-Default-Skala); falls keine ws-Spalte: total Operationen.
    lp.records        = ws_seen ? working_set : total;
    lp.num_operations = total;

    WorkloadConfig& c = lp.config;
    c.pct_insert      = static_cast<double>(ins) * inv;
    c.pct_lookup      = static_cast<double>(lk) * inv;
    c.pct_erase       = static_cast<double>(er) * inv;
    c.pct_clear       = static_cast<double>(cl) * inv;
    c.pct_scan        = static_cast<double>(sc) * inv;
    c.pct_rmw         = static_cast<double>(rmw) * inv;
    c.num_operations  = static_cast<std::size_t>(total);
    return lp;
}

} // namespace comdare::cache_engine::builder::workload_driver
