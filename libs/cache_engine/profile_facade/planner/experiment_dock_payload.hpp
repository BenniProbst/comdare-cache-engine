#pragma once
// experiment_dock_payload.hpp -- Welle 5 (§38-Dock-Protokoll, 2026-07-19). Die zwei PODs der Dock-Naht des
// experiment_plan_director.hpp-Planers plus ihre Serialisierung:
//
//   TEIL 1 (§38 hinab -- E-W5-1): ExperimentSubtreePayload = die an ein Mess-Dock delegierte Teilbaum-Nutzlast
//     (eine Liste Achsen-Range-Eintraege, AxisKind-gefaerbt wie im Ledger §30). Wire-Format = ein XML-Fragment
//     DECKUNGSGLEICH mit der R5-Range-Syntax (<values> enumeriert / <range start count> Index-Fenster) -- konsistent
//     mit "NUR EIN XML-Programm" und dem Registry-/.pom-Modell. In-process reist die Nutzlast als POD; der
//     Emitter+Parser bekommt ein Byte-Roundtrip-Gate (emit->parse->emit byte-identisch).
//
//   TEIL 2 (§38 hinauf -- E-W5-2): ProgressDelta = der Fortschritts-Rueck-Kanal. Erste Meldung eines Fensters =
//     Voll-Konfiguration (alle Achsen gelistet), danach mixed-radix-minimale Deltas in StaticBinaryView-Ordnung;
//     done=true genau einmal am Fensterende (= §38.b-Fertig-Signal). KEIN Mess-Daten-Rueckfluss. Die Injektions-
//     Naht ProgressSinkFn wird EXAKT nach der No-Op-Naht-Doktrin von CachePushFn/MeasurementSinkFn in die
//     LazyRunConfig gehaengt (No-Op-Default => byte-neutral, golden-neutral).
//
// LEAF-Header: haengt NUR an der common-DOM (xml_reader, KEINE neue XML-Lib) + topics::AxisKind + stdlib -- er wird
// vom achsen-blinden builder/experiment_tree/cache_engine_builder_iterator.hpp (fuer ProgressDelta/ProgressSinkFn)
// UND vom profile_facade-Planer (fuer ExperimentSubtreePayload) inkludiert; keiner dieser inkludiert diesen Header
// zurueck -> zyklenfrei. Header-only, C++23. ASCII-only.

#include <serialization/xml_config_parser/xml_reader.hpp> // comdare::common::xml::parse_document / XmlNode (self-contained DOM)
#include <topics/axis.hpp>                                // topics::AxisKind (Ledger §30-Faerbung; nicht dupliziert)
#include <builder/experiment_tree/progress_delta.hpp> // TEIL2 lebt in der BUILDER-Schicht (Layering: planner -> builder); hier re-exportiert

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::planner {

// ═══════════════════════════════════════════════════════════════════════════════════════════════════════
// TEIL 1 -- Teilbaum-Nutzlast (§38 hinab) + R5-XML-Serialisierung
// ═══════════════════════════════════════════════════════════════════════════════════════════════════════

/// Diskriminante EINES Achsen-Eintrags: enumerierte Variant-Namen ODER ein [start, start+count)-Index-Fenster.
enum class AxisRangeForm : unsigned char {
    enumerated,  ///< die Achse traegt eine explizite Variant-Namensliste (<values>a b c</values>)
    index_range, ///< die Achse traegt ein Index-Fenster ueber ihre Varianten (<range start=".." count=".."/>)
};

/// EIN Achsen-Range-Eintrag der Teilbaum-Nutzlast. AxisKind-gefaerbt (organ / system_config / system_measurement,
/// die EINE topics::AxisKind-Taxonomie -- NICHT dupliziert). form waehlt, welches der beiden Felderpaare gilt.
struct AxisRangeEntry {
    topics::AxisKind         kind = topics::AxisKind::organ; // Ledger §30-Faerbung
    std::string              axis_name;                      // der Achsen-Name (registry-id, z.B. "search_algo")
    AxisRangeForm            form = AxisRangeForm::enumerated;
    std::vector<std::string> values; // form==enumerated: die enumerierten Variant-Namen (whitespace-freie Tokens)
    std::size_t              range_start = 0; // form==index_range: Fenster-Start (Variant-Index)
    std::size_t              range_count = 0; // form==index_range: Fenster-Groesse

    [[nodiscard]] bool operator==(AxisRangeEntry const&) const = default;
};

/// Die POD-Nutzlast: die an ein Mess-Dock delegierte Teilbaum-Spezifikation (Achsen-Ranges in Dokument-Reihenfolge).
struct ExperimentSubtreePayload {
    std::vector<AxisRangeEntry> axes;

    [[nodiscard]] bool operator==(ExperimentSubtreePayload const&) const = default;
};

namespace detail {

/// AxisKind -> Wire-Token (Ledger §30-Vokabular). Total ueber die drei Enumeratoren (kein Default-Ausfall).
[[nodiscard]] inline std::string_view axis_kind_token(topics::AxisKind k) noexcept {
    switch (k) {
        case topics::AxisKind::organ: return "organ";
        case topics::AxisKind::system_config: return "system_config";
        case topics::AxisKind::system_measurement: return "system_measurement";
    }
    return "organ"; // unerreichbar (Enum vollstaendig); haelt den switch total
}

/// Wire-Token -> AxisKind. false bei unbekanntem Token (der Negativ-Fall des Parsers -> harter Fehler).
[[nodiscard]] inline bool parse_axis_kind(std::string_view s, topics::AxisKind& out) noexcept {
    if (s == "organ") {
        out = topics::AxisKind::organ;
        return true;
    }
    if (s == "system_config") {
        out = topics::AxisKind::system_config;
        return true;
    }
    if (s == "system_measurement") {
        out = topics::AxisKind::system_measurement;
        return true;
    }
    return false; // unbekanntes kind => Parse-Fehler (nie stille Fehlfaerbung)
}

/// XML-Entity-Encode der 5 Basis-Entities -- INVERSE zu xml_reader.hpp detail::decode_entities. '&' zuerst
/// (sonst Doppel-Encode). Deckungsgleich mit load_profile_writer.hpp detail::xml_encode (dort fuer LoadProfile).
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

/// Robuster size_t-Parser (nicht-numerisch / leer -> 0; kein Wurf).
[[nodiscard]] inline std::size_t to_size(std::string const& s) noexcept {
    std::size_t v     = 0;
    bool        digit = false;
    for (char c : s) {
        if (c < '0' || c > '9') break;
        v     = v * 10 + static_cast<std::size_t>(c - '0');
        digit = true;
    }
    return digit ? v : 0;
}

} // namespace detail

/// emit_experiment_subtree_xml -- ExperimentSubtreePayload -> R5-XML-Fragment (deterministisch, byte-stabil).
/// Schema: <experiment_subtree><axis kind=".." name=".."><values>a b c</values>|<range start=".." count=".."/>
///         </axis>...</experiment_subtree>. Feste 2-Leerzeichen-Einrueckung + Attribut-Reihenfolge (kind,name)
/// -> emit(parse(emit(p))) == emit(p) (Byte-Roundtrip-Gate, s. Test).
[[nodiscard]] inline std::string emit_experiment_subtree_xml(ExperimentSubtreePayload const& p) {
    std::string o;
    o += "<experiment_subtree>\n";
    for (AxisRangeEntry const& a : p.axes) {
        o += "  <axis kind=\"";
        o += detail::axis_kind_token(a.kind);
        o += "\" name=\"";
        o += detail::xml_encode(a.axis_name);
        o += "\">";
        if (a.form == AxisRangeForm::enumerated) {
            o += "<values>";
            for (std::size_t i = 0; i < a.values.size(); ++i) {
                if (i != 0) o += ' ';
                o += detail::xml_encode(a.values[i]);
            }
            o += "</values>";
        } else {
            o += "<range start=\"";
            o += std::to_string(a.range_start);
            o += "\" count=\"";
            o += std::to_string(a.range_count);
            o += "\"/>";
        }
        o += "</axis>\n";
    }
    o += "</experiment_subtree>\n";
    return o;
}

/// parse_experiment_subtree_xml -- R5-XML-Fragment -> ExperimentSubtreePayload (INVERSE zu emit). nullopt bei:
/// (a) nicht wohlgeformt / falscher Wurzel-Tag, (b) unbekanntem kind (Negativ-Fall), (c) axis ohne values/range.
/// Nutzt die BESTEHENDE common-DOM (parse_document) -- keine neue XML-Lib, gleiche Machart wie load_profile_parser.
[[nodiscard]] inline std::optional<ExperimentSubtreePayload> parse_experiment_subtree_xml(std::string_view xml) {
    std::optional<comdare::common::xml::XmlNode> const root = comdare::common::xml::parse_document(xml);
    if (!root || root->tag != "experiment_subtree") return std::nullopt;

    ExperimentSubtreePayload p;
    for (comdare::common::xml::XmlNode const* ax : root->children_named("axis")) {
        AxisRangeEntry e;
        if (!detail::parse_axis_kind(ax->attr("kind"), e.kind)) return std::nullopt; // unbekanntes kind -> Fehler
        e.axis_name = ax->attr("name");

        comdare::common::xml::XmlNode const* vals = ax->child("values");
        comdare::common::xml::XmlNode const* rng  = ax->child("range");
        if (vals != nullptr) {
            e.form   = AxisRangeForm::enumerated;
            e.values = vals->text_tokens(); // whitespace-getrennte Variant-Namen
        } else if (rng != nullptr) {
            e.form        = AxisRangeForm::index_range;
            e.range_start = detail::to_size(rng->attr("start"));
            e.range_count = detail::to_size(rng->attr("count"));
        } else {
            return std::nullopt; // axis ohne <values> UND ohne <range> -> Fehler (keine leere Diskriminante)
        }
        p.axes.push_back(std::move(e));
    }
    return p;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════════════════
// TEIL 2 -- Fortschritts-Rueck-Kanal (§38 hinauf, E-W5-2): RE-EXPORT aus der BUILDER-Schicht.
// Die PODs + Delta-Logik liegen kanonisch in builder/experiment_tree/progress_delta.hpp (der Iterator inkludiert
// NUR diesen builder-Leaf -> kein Aufwaerts-Include). Hier werden sie in den planner-Namespace hochgezogen, damit
// der planner-/Dock-Code (experiment_plan_director & Co.) beide Teile der Dock-Nutzlast ueber DIESEN einen Header
// sieht (planner -> builder = korrekte Include-Richtung). Muster wie die CachePushFn-/MeasurementSinkFn-Aliase.
// ═══════════════════════════════════════════════════════════════════════════════════════════════════════
using ProgressAxisChange = ::comdare::cache_engine::builder::experiment::ProgressAxisChange;
using ProgressDelta      = ::comdare::cache_engine::builder::experiment::ProgressDelta;
using ProgressSinkFn     = ::comdare::cache_engine::builder::experiment::ProgressSinkFn;
using ::comdare::cache_engine::builder::experiment::compute_progress_deltas;
using ::comdare::cache_engine::builder::experiment::progress_delta_between;
using ::comdare::cache_engine::builder::experiment::reconstruct_configs;

} // namespace comdare::cache_engine::planner
