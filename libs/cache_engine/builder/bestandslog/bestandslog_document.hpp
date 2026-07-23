#pragma once
// bestandslog_document.hpp -- G3 / #46b Lagerhaltung, Scheibe B1 (Ledger §62-B, §66 Lager-Gate).
//
// Das BESTANDSLOG ist das minio-gehaltene Inventar der Lagerhaltung: EIN XML-Dokument je GENUS
// (binary | measurement), das (a) den BESTAND gebauter Artefakte (SHA512-Key -> Pfad/Bytes/Stempel)
// und (b) die offenen/abgeschlossenen batch-RESERVIERUNGEN der Bau-Maschinen fuehrt. Ueber die
// SHA512-Fingerprint-Stempel (K7b) wird jedes Artefakt identifizierbar -> Re-Bauten werden unnoetig
// (das DLL-"ccache" der Lagerhaltung, §62-B-P1). Die ZWEI Genera sind:
//   * binary      -- gebaute Tier-/CEB-Binaries (Bestand 1)
//   * measurement -- erhobene Messwerte (Bestand 2, §62-B Factory Pattern)
//
// DIESER HEADER ist die reine SERIALISIERUNGS-Schicht: PODs + deterministischer XML-Emitter +
// Parser ueber den self-contained common-DOM (xml_reader.hpp). KEINE Transport-/Lock-/ETA-/Factory-
// Logik -- die liegt in den Scheiben B2 (Lock+Transport-Naht), B3 (Factory+SHA512-Index) und
// B4 (Reservierungs-Lifecycle+ETA). Nur stdlib + der bestehende common-DOM, keine neuen Deps.
//
// DETERMINISMUS: fester XML-Kopf, feste 2-Leerzeichen-Einrueckung, feste Attribut-Reihenfolge ->
// emit(parse(emit(d))) == emit(d) (Byte-Roundtrip-Gate, vgl. experiment_dock_payload.hpp /
// load_profile_writer.hpp). detail::xml_encode ist die INVERSE zu xml_reader detail::decode_entities
// ('&' zuerst, sonst Doppel-Encode) und deckungsgleich mit den beiden bestehenden Encodern.
//
// Grammatik (syntax_version 1):
//   <bestandslog syntax_version="1" semantics_version="1" genus="binary|measurement"
//                doc_revision="N" created_utc="...">
//     <bestand>
//       <eintrag key_sha512="hex128" pfad="..." bytes="N"
//                stempel="[d,e,f][g,h,i]+bt=Release" done_utc="..."/>
//     </bestand>
//     <reservierungen>
//       <batch id="owner_uuid/seq" typ="tier|ceb|planer_block" slice_begin="0" slice_count="4096"
//              maschine="prod1" threads="32" reserviert_utc="..." pro_forma_bis_utc="..."
//              eta_s="" avg_size_bytes="" status="offen|done|released"/>
//     </reservierungen>
//   </bestandslog>
// doc_revision ist monoton -> Grundlage des B2-Record-Union-Merge (fetch->merge->store). eta_s und
// avg_size_bytes bleiben leer, bis die B4-Mini-Batch-Kalibrierung sie fuellt; ihre Wire-Form ist
// STRING (leer == noch nicht geschaetzt), die numerische Arithmetik liegt im eta_estimator (B4).

#include <serialization/xml_config_parser/xml_reader.hpp> // comdare::common::xml::parse_document / XmlNode

#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::bestandslog {

// ---------------------------------------------------------------------------
// Header-Versionen (§62-B). syntax_version = Wire-Grammatik (Bruch => Ablehnung eines HOEHEREN
// Dokuments); semantics_version = additive Bedeutung (darf differieren, Leser interpretiert das
// bekannte Feld-Set). Beide getrennt gestempelt -> chirurgische Invalidierung.
// ---------------------------------------------------------------------------
inline constexpr int kSyntaxVersion    = 1;
inline constexpr int kSemanticsVersion = 1;

// Genus des Bestandslogs -- die zwei Bestands-Gattungen (§62-B, B3-Factory instanziiert je Genus).
enum class Genus { binary, measurement };

// Batch-Typ einer Reservierung (Testat-Grammatik §62-B, ebenen-richtig):
//   tier         -- Tier-Binary-Bau-Batch ([d,e,f]-Replay-Schluessel)
//   ceb          -- CEB-Binary-Bau ([a,b,c]-Replay-Schluessel)
//   planer_block -- Planer-Vorreservierung vor einem CEB-Compile (30min pro-forma, OHNE ETA, B7)
enum class BatchTyp { tier, ceb, planer_block };

// Lebenszyklus einer Reservierung (B4): offen -> done | released (PromiseGuard-Abbruch).
enum class BatchStatus { offen, done, released };

// ---------------------------------------------------------------------------
// Enum <-> String (deterministisch; Parser lehnt Unbekanntes ab -> nie stille Fehlfaerbung,
// vgl. experiment_dock_payload.hpp Z99). to_string liefert string_view (append-tauglich).
// ---------------------------------------------------------------------------
[[nodiscard]] inline std::string_view to_string(Genus g) noexcept {
    switch (g) {
        case Genus::binary: return "binary";
        case Genus::measurement: return "measurement";
    }
    return "binary"; // unerreichbar (alle Faelle abgedeckt); nur Compiler-Vollstaendigkeit
}
[[nodiscard]] inline std::optional<Genus> genus_from_string(std::string_view s) noexcept {
    if (s == "binary") return Genus::binary;
    if (s == "measurement") return Genus::measurement;
    return std::nullopt;
}

[[nodiscard]] inline std::string_view to_string(BatchTyp t) noexcept {
    switch (t) {
        case BatchTyp::tier: return "tier";
        case BatchTyp::ceb: return "ceb";
        case BatchTyp::planer_block: return "planer_block";
    }
    return "tier";
}
[[nodiscard]] inline std::optional<BatchTyp> batch_typ_from_string(std::string_view s) noexcept {
    if (s == "tier") return BatchTyp::tier;
    if (s == "ceb") return BatchTyp::ceb;
    if (s == "planer_block") return BatchTyp::planer_block;
    return std::nullopt;
}

[[nodiscard]] inline std::string_view to_string(BatchStatus s) noexcept {
    switch (s) {
        case BatchStatus::offen: return "offen";
        case BatchStatus::done: return "done";
        case BatchStatus::released: return "released";
    }
    return "offen";
}
[[nodiscard]] inline std::optional<BatchStatus> batch_status_from_string(std::string_view s) noexcept {
    if (s == "offen") return BatchStatus::offen;
    if (s == "done") return BatchStatus::done;
    if (s == "released") return BatchStatus::released;
    return std::nullopt;
}

// ---------------------------------------------------------------------------
// PODs. Defaulted operator== dient den Roundtrip-/Merge-Tests (B1/B2).
// ---------------------------------------------------------------------------

// Ein Bestands-Eintrag: gebautes Artefakt, ueber key_sha512 (K7b-Fingerprint) identifizierbar.
struct BestandEintrag {
    std::string   key_sha512; // 128 hex chars (SHA512 der Stempel-Zeilen bzw. Messwert-Key)
    std::string   pfad;       // Objekt-Store-Pfad relativ zum Bestandslog-Praefix
    std::uint64_t bytes = 0;  // Groesse des Artefakts
    std::string   stempel;    // "[d,e,f][g,h,i]+bt=Release" (Varianten-Identitaet, §62-B)
    std::string   done_utc;   // Fertigstellungs-Zeitstempel (ISO-8601 UTC)

    friend bool operator==(BestandEintrag const&, BestandEintrag const&) = default;
};

// Eine batch-Reservierung: Besitz eines Slice-Fensters durch eine Bau-Maschine (§2 des Designs).
struct BatchReservierung {
    std::string   id; // owner_uuid/seq -- per-Owner eindeutig (Record-Union-Basis)
    BatchTyp      typ         = BatchTyp::tier;
    std::uint64_t slice_begin = 0;   // Fenster-Anfang (kGnBatchSlice=4096-Korn)
    std::uint64_t slice_count = 0;   // Fenster-Laenge
    std::string   maschine;          // z.B. prod1 / prod2
    unsigned      threads = 0;       // Thread-Budget (prod1=32, prod2=24), B14-Wache
    std::string   reserviert_utc;    // Reservierungs-Zeitstempel (ISO-8601 UTC)
    std::string   pro_forma_bis_utc; // pro-forma-30min-Frist (B4), vor der ETA-Eintragung
    std::string   eta_s;             // geschaetzte Restzeit in Sekunden (leer = noch nicht kalibriert)
    std::string   avg_size_bytes;    // avg-Binary-Groesse des Blocks (leer = noch nicht kalibriert)
    BatchStatus   status = BatchStatus::offen;

    friend bool operator==(BatchReservierung const&, BatchReservierung const&) = default;
};

// Das ganze Bestandslog-Dokument (ein Genus).
struct BestandslogDocument {
    int           syntax_version    = kSyntaxVersion;
    int           semantics_version = kSemanticsVersion;
    Genus         genus             = Genus::binary;
    std::uint64_t doc_revision      = 0; // monoton (Merge-Sequenz, B2)
    std::string   created_utc;

    std::vector<BestandEintrag>    bestand;
    std::vector<BatchReservierung> reservierungen;

    friend bool operator==(BestandslogDocument const&, BestandslogDocument const&) = default;
};

// syntax_version-Vertraeglichkeit: ein Dokument mit HOEHERER Wire-Grammatik ist nicht sicher lesbar
// (Feld-Bruch moeglich); semantics_version darf beliebig differieren (additiv).
[[nodiscard]] inline bool document_syntax_supported(BestandslogDocument const& d) noexcept {
    return d.syntax_version >= 1 && d.syntax_version <= kSyntaxVersion;
}

namespace detail {

// XML-Entity-Encode der 5 Basis-Entities -- INVERSE zu xml_reader.hpp detail::decode_entities. '&'
// zuerst (sonst Doppel-Encode). Deckungsgleich mit experiment_dock_payload.hpp / load_profile_writer.hpp
// detail::xml_encode (Single-Source-Doktrin fuer den Byte-Roundtrip aller common-DOM-Emitter).
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

// Robuste Integer-Parser: leer/nicht-numerisch -> Default unveraendert (std::from_chars laesst das
// Ziel bei Fehlschlag stehen -> kein Wurf, deterministisch).
[[nodiscard]] inline std::uint64_t parse_u64(std::string_view s) noexcept {
    std::uint64_t v = 0;
    std::from_chars(s.data(), s.data() + s.size(), v);
    return v;
}
[[nodiscard]] inline unsigned parse_uint(std::string_view s) noexcept {
    unsigned v = 0;
    std::from_chars(s.data(), s.data() + s.size(), v);
    return v;
}
[[nodiscard]] inline int parse_int(std::string_view s, int def) noexcept {
    int v = def;
    std::from_chars(s.data(), s.data() + s.size(), v);
    return v;
}

} // namespace detail

// ---------------------------------------------------------------------------
// emit_document -- BestandslogDocument -> XML (deterministisch, byte-stabil).
// ---------------------------------------------------------------------------
[[nodiscard]] inline std::string emit_document(BestandslogDocument const& d) {
    std::string out;
    out += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out += "<bestandslog syntax_version=\"";
    out += std::to_string(d.syntax_version);
    out += "\" semantics_version=\"";
    out += std::to_string(d.semantics_version);
    out += "\" genus=\"";
    out += to_string(d.genus);
    out += "\" doc_revision=\"";
    out += std::to_string(d.doc_revision);
    out += "\" created_utc=\"";
    out += detail::xml_encode(d.created_utc);
    out += "\">\n";

    out += "  <bestand>\n";
    for (auto const& e : d.bestand) {
        out += "    <eintrag key_sha512=\"";
        out += detail::xml_encode(e.key_sha512);
        out += "\" pfad=\"";
        out += detail::xml_encode(e.pfad);
        out += "\" bytes=\"";
        out += std::to_string(e.bytes);
        out += "\" stempel=\"";
        out += detail::xml_encode(e.stempel);
        out += "\" done_utc=\"";
        out += detail::xml_encode(e.done_utc);
        out += "\"/>\n";
    }
    out += "  </bestand>\n";

    out += "  <reservierungen>\n";
    for (auto const& r : d.reservierungen) {
        out += "    <batch id=\"";
        out += detail::xml_encode(r.id);
        out += "\" typ=\"";
        out += to_string(r.typ);
        out += "\" slice_begin=\"";
        out += std::to_string(r.slice_begin);
        out += "\" slice_count=\"";
        out += std::to_string(r.slice_count);
        out += "\" maschine=\"";
        out += detail::xml_encode(r.maschine);
        out += "\" threads=\"";
        out += std::to_string(r.threads);
        out += "\" reserviert_utc=\"";
        out += detail::xml_encode(r.reserviert_utc);
        out += "\" pro_forma_bis_utc=\"";
        out += detail::xml_encode(r.pro_forma_bis_utc);
        out += "\" eta_s=\"";
        out += detail::xml_encode(r.eta_s);
        out += "\" avg_size_bytes=\"";
        out += detail::xml_encode(r.avg_size_bytes);
        out += "\" status=\"";
        out += to_string(r.status);
        out += "\"/>\n";
    }
    out += "  </reservierungen>\n";

    out += "</bestandslog>\n";
    return out;
}

// ---------------------------------------------------------------------------
// parse_bestandslog -- XML -> BestandslogDocument. nullopt bei nicht wohlgeformtem XML, falschem
// Wurzel-Tag oder UNBEKANNTEM Enum-Wert (genus/typ/status) -> nie stille Fehlfaerbung. Versionen
// werden treu uebernommen; die Vertraeglichkeit prueft der Aufrufer via document_syntax_supported.
// ---------------------------------------------------------------------------
[[nodiscard]] inline std::optional<BestandslogDocument> parse_bestandslog(std::string_view xml) {
    auto root = common::xml::parse_document(xml);
    if (!root || root->tag != "bestandslog") return std::nullopt;

    BestandslogDocument d;
    d.syntax_version    = detail::parse_int(root->attr("syntax_version"), 0);
    d.semantics_version = detail::parse_int(root->attr("semantics_version"), 0);
    auto g              = genus_from_string(root->attr("genus"));
    if (!g) return std::nullopt;
    d.genus        = *g;
    d.doc_revision = detail::parse_u64(root->attr("doc_revision"));
    d.created_utc  = root->attr("created_utc");

    if (auto const* bestand = root->child("bestand")) {
        for (auto const* e : bestand->children_named("eintrag")) {
            BestandEintrag be;
            be.key_sha512 = e->attr("key_sha512");
            be.pfad       = e->attr("pfad");
            be.bytes      = detail::parse_u64(e->attr("bytes"));
            be.stempel    = e->attr("stempel");
            be.done_utc   = e->attr("done_utc");
            d.bestand.push_back(std::move(be));
        }
    }

    if (auto const* res = root->child("reservierungen")) {
        for (auto const* b : res->children_named("batch")) {
            BatchReservierung br;
            br.id  = b->attr("id");
            auto t = batch_typ_from_string(b->attr("typ"));
            if (!t) return std::nullopt;
            br.typ               = *t;
            br.slice_begin       = detail::parse_u64(b->attr("slice_begin"));
            br.slice_count       = detail::parse_u64(b->attr("slice_count"));
            br.maschine          = b->attr("maschine");
            br.threads           = detail::parse_uint(b->attr("threads"));
            br.reserviert_utc    = b->attr("reserviert_utc");
            br.pro_forma_bis_utc = b->attr("pro_forma_bis_utc");
            br.eta_s             = b->attr("eta_s");
            br.avg_size_bytes    = b->attr("avg_size_bytes");
            auto s               = batch_status_from_string(b->attr("status"));
            if (!s) return std::nullopt;
            br.status = *s;
            d.reservierungen.push_back(std::move(br));
        }
    }

    return d;
}

} // namespace comdare::cache_engine::builder::bestandslog
