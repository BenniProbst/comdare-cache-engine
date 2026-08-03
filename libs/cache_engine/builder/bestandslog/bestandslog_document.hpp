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
// ZELL-KOORDINATEN (§62-NACHTRAG-4, syntax_version 2): der SHA512-Fingerprint traegt die per-ZELLE
// gewaehlte ISA/Optimierung NICHT -- er ist die Anatomie-Digest der Preimage-Glieder (A13-M3: Format-Kennung, organ, system,
// measurement, Sub-Achsen-Werteset, overlay -- abi::anatomy_fingerprint_glieder). Zwei Bauten
// DERSELBEN Permutation unter avx2 bzw. avx512 haetten also denselben key_sha512 und wuerden im Lager
// FALSCH dedupliziert (der zweite Bau gaelte als Treffer, obwohl die Bytes andere sind). Deshalb traegt
// jeder Eintrag die drei Zell-Koordinaten combo/opt/simd als EIGENE Felder; die Eindeutigkeit laeuft
// ueber das TUPEL (key_sha512, combo, opt, simd). KEINE String-Konkatenation als Schluessel-Fusion --
// die Felder bleiben getrennt geklammert (§66-N3, Punkt 4), damit jede Koordinate einzeln lesbar,
// filterbar und ohne Trennzeichen-Mehrdeutigkeit bleibt. Leere Zell-Felder sind zulaessig und bedeuten
// "keine Zell-Koordinate gemeldet" (Default-neutral: ein Host, der sie nicht setzt, deduped wie zuvor).
//
// Grammatik (syntax_version 3):
//   <bestandslog syntax_version="3" semantics_version="1" genus="binary|measurement"
//                doc_revision="N" created_utc="...">
//     <bestand>
//       <eintrag key_sha512="hex128" combo="..." opt="O2" simd="avx2" pfad="..." bytes="N"
//                stempel="[d,e,f][g,h,i]+bt=Release" done_utc="..."/>
//     </bestand>
//     <reservierungen>
//       <batch id="owner_uuid/seq" typ="tier|ceb|planer_block" slice_begin="0" slice_count="4096"
//              maschine="prod1" threads="32" reserviert_utc="..." pro_forma_bis_utc="..."
//              eta_s="" avg_size_bytes="" status="offen|done|released"
//              [ceb_legende="[a,b,c]"] [ceb_key_sha512="hex128"]/>
//     </reservierungen>
//   </bestandslog>
// Die beiden ceb_*-Attribute am <batch> sind OPTIONAL (in [] notiert): sie stehen nur im Dokument,
// wenn die Reservierung eine CEB-Bindung traegt (planer_block), und dann als ZWEI getrennte Felder.
// doc_revision ist monoton -> Grundlage des B2-Record-Union-Merge (fetch->merge->store). eta_s und
// avg_size_bytes bleiben leer, bis die B4-Mini-Batch-Kalibrierung sie fuellt; ihre Wire-Form ist
// STRING (leer == noch nicht geschaetzt), die numerische Arithmetik liegt im eta_estimator (B4).

#include <serialization/xml_config_parser/xml_reader.hpp> // comdare::common::xml::parse_document / XmlNode

#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::bestandslog {

// ---------------------------------------------------------------------------
// Header-Versionen (§62-B). syntax_version = Wire-Grammatik (Bruch => Ablehnung eines HOEHEREN
// Dokuments); semantics_version = additive Bedeutung (darf differieren, Leser interpretiert das
// bekannte Feld-Set). Beide getrennt gestempelt -> chirurgische Invalidierung.
//
// 1 -> 2 (§62-NACHTRAG-4): die drei Zell-Koordinaten-Attribute combo/opt/simd am <eintrag>. Das ist ein
// WIRE-Bruch nach unten, nicht nach oben: ein v1-Leser wuerde die neuen Attribute schlucken und dann
// ueber das ALTE Ein-Feld-Kriterium (nur key_sha512) falsch deduplizieren -- genau das verhindert der
// syntax-Bump, weil document_syntax_supported ein Dokument mit HOEHERER Grammatik ablehnt. Umgekehrt
// liest ein v2-Leser ein v1-Dokument treu (fehlende Attribute = leere Zell-Koordinaten). semantics_version
// bleibt 1: an den Feldern, die ein v1-Leser kennt, aendert sich keine Bedeutung.
//
// 2 -> 3 (Owner-Abnahme zur Versions-Bindung): zwei OPTIONALE Attribute am <batch> --
// ceb_legende (die [a,b,c]-Mess-Kombinatorik der emittierenden CEB-Strecke) und ceb_key_sha512
// (128-hex SHA512 der CEB). Sie tragen die vom Plan verlangte Bindung "fuer diese Version" als
// EIGENE FELDER; die id bleibt unberuehrt (§66-N3: je Achse ihr eigenes Feld, KEINE Fusion in
// Signaturen/Schluessel/Stempel -- eine in die id gequetschte Version waere genau diese Fusion).
// Nach oben ist auch das ein Wire-Bruch: ein v2-Leser schluckte die Attribute stillschweigend und
// hielte eine versions-FREMDE Reservierung fuer die eigene -- deshalb lehnt document_syntax_supported
// beim v2-Leser ein v3-Dokument ab. Nach unten liest ein v3-Leser v1/v2 treu (fehlende Attribute =
// leere Felder, kein Fehler). semantics_version bleibt 1: an den Feldern, die ein v2-Leser kennt,
// aendert sich keine Bedeutung.
// Emittiert werden die zwei Attribute NUR gefuellt -> eine Reservierung ohne CEB-Bindung ist in der
// Ausgabe byte-identisch zur v2-Form; nur der Header-Stempel unterscheidet die Dokumente.
// ---------------------------------------------------------------------------
inline constexpr int kSyntaxVersion    = 3;
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

// Die drei ZELL-KOORDINATEN [d,e,f] eines Bau-Ziels (§62-NACHTRAG-4). Runtime-Strings, exakt so
// geschrieben wie in den Testaten (combo z.B. "" oder ein Mess-Tool-Kuerzel, opt z.B. "O2", simd z.B.
// "avx2"). Sie sind der Teil der Bau-Identitaet, den der Anatomie-Fingerprint NICHT traegt.
//
// EIN Typ fuer alle Konsumenten (Eintrag, Lager-Schluessel, Key-Policy, Registrierung, Presence-Naht) ->
// die Koordinaten koennen zwischen Serialisierung und Lookup nicht auseinanderdriften. Der Vergleich ist
// feldweise (defaulted <=>), NIE ueber einen fusionierten String -- zwei Zellen sind genau dann gleich,
// wenn alle drei Koordinaten gleich sind. §66-N3-rein: reine RT->RT-Abbildung (Runtime-Strings bleiben
// Runtime-Strings; nichts davon wird zu einem CT-Typ hochgezogen).
struct ZellKoordinaten {
    std::string combo; // [d] Mess-/Tool-Kombination (COMDARE_MEASUREMENT_COMBO-Auspraegung; leer = Default)
    std::string opt;   // [e] Optimierungsstufe (z.B. O2/O3)
    std::string simd;  // [f] ISA-/SIMD-Auspraegung (z.B. sse42/avx2/avx512)

    friend auto operator<=>(ZellKoordinaten const&, ZellKoordinaten const&) = default;
    friend bool operator==(ZellKoordinaten const&, ZellKoordinaten const&)  = default;

    // true, wenn KEINE Koordinate gemeldet wurde (Default-neutraler Zustand).
    [[nodiscard]] bool empty() const noexcept { return combo.empty() && opt.empty() && simd.empty(); }
};

// Ein Bestands-Eintrag: gebautes Artefakt, identifiziert ueber das TUPEL (key_sha512, zelle) -- der
// K7b-Fingerprint ALLEIN reicht nicht, weil er die per-Zelle-ISA nicht traegt (s. Kopf).
struct BestandEintrag {
    std::string     key_sha512; // 128 hex chars (SHA512 der Stempel-Zeilen bzw. Messwert-Key)
    ZellKoordinaten zelle;      // [d,e,f] -- zweiter, GETRENNTER Teil der Identitaet (keine Fusion)
    std::string     pfad;       // Objekt-Store-Pfad relativ zum Bestandslog-Praefix
    std::uint64_t   bytes = 0;  // Groesse des Artefakts
    std::string     stempel;    // "[d,e,f][g,h,i]+bt=Release" (Varianten-Identitaet, §62-B)
    std::string     done_utc;   // Fertigstellungs-Zeitstempel (ISO-8601 UTC)

    friend bool operator==(BestandEintrag const&, BestandEintrag const&) = default;
};

// ---------------------------------------------------------------------------
// IDENTITAET eines Bestands-Eintrags -- die EINE Definition, gegen die Index (B3), Merge (B2) und
// Registrierung (I1) arbeiten. Wer sie umgeht, riskiert genau den Dedup-Drift, den NACHTRAG-4 schliesst.
// ---------------------------------------------------------------------------
[[nodiscard]] inline bool same_eintrag_identity(BestandEintrag const& a, BestandEintrag const& b) noexcept {
    return a.key_sha512 == b.key_sha512 && a.zelle == b.zelle;
}

// Strikte Ordnung ueber dasselbe Tupel (fuer den byte-stabilen, eingabe-reihenfolge-unabhaengigen Emit).
[[nodiscard]] inline bool eintrag_identity_less(BestandEintrag const& a, BestandEintrag const& b) noexcept {
    return std::tie(a.key_sha512, a.zelle.combo, a.zelle.opt, a.zelle.simd) <
           std::tie(b.key_sha512, b.zelle.combo, b.zelle.opt, b.zelle.simd);
}

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
    // Die CEB-BINDUNG (syntax_version 3, optional -- leer = keine Bindung gemeldet). ZWEI getrennt
    // geklammerte Felder statt eines fusionierten Schluessels: die Legende ist die Mess-Achsen-Klammer
    // der CEB, der SHA512 ist ihr Binary-Fingerprint; beide beantworten "welche Version" auf
    // verschiedenen Ebenen und bleiben deshalb einzeln lesbar und filterbar. Getragen werden sie vom
    // planer_block (Vorreservierung eines CEB-Compiles); tier-Batches lassen sie leer.
    // Stehen am ENDE der Feld-Folge, weil die Attribut-Reihenfolge des Emitters Teil der
    // Byte-Stabilitaet ist -- neue Felder kommen hinten dazu, nie zwischen die bestehenden.
    std::string ceb_legende;    // z.B. "[a,b,c]" (leer = nicht gemeldet)
    std::string ceb_key_sha512; // 128 hex (leer = nicht gemeldet)

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
        // Identitaets-Block ZUERST (key_sha512 + die drei Zell-Koordinaten), danach die Nutzdaten. Die
        // Attribut-Reihenfolge ist Teil der Byte-Stabilitaet -> hier festgeschrieben, nie umsortiert.
        out += "    <eintrag key_sha512=\"";
        out += detail::xml_encode(e.key_sha512);
        out += "\" combo=\"";
        out += detail::xml_encode(e.zelle.combo);
        out += "\" opt=\"";
        out += detail::xml_encode(e.zelle.opt);
        out += "\" simd=\"";
        out += detail::xml_encode(e.zelle.simd);
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
        // syntax_version 3: die CEB-Bindung nur, wenn sie gefuellt ist. Eine Reservierung ohne Bindung
        // emittiert damit exakt die v2-Bytes (der Grund, weshalb die Felder ANS ENDE gehoeren und der
        // Emitter sie nicht als leere Attribute schreibt).
        if (!r.ceb_legende.empty()) {
            out += "\" ceb_legende=\"";
            out += detail::xml_encode(r.ceb_legende);
        }
        if (!r.ceb_key_sha512.empty()) {
            out += "\" ceb_key_sha512=\"";
            out += detail::xml_encode(r.ceb_key_sha512);
        }
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
            // Zell-Koordinaten: fehlende Attribute (v1-Dokument) ergeben leere Strings == "nicht gemeldet".
            be.zelle.combo = e->attr("combo");
            be.zelle.opt   = e->attr("opt");
            be.zelle.simd  = e->attr("simd");
            be.pfad        = e->attr("pfad");
            be.bytes       = detail::parse_u64(e->attr("bytes"));
            be.stempel     = e->attr("stempel");
            be.done_utc    = e->attr("done_utc");
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
            // CEB-Bindung (syntax_version 3): OPTIONAL -- fehlende Attribute ergeben leere Strings
            // ("nicht gemeldet"), kein Fehler. Damit bleibt jedes v1-/v2-Dokument treu lesbar.
            br.ceb_legende    = b->attr("ceb_legende");
            br.ceb_key_sha512 = b->attr("ceb_key_sha512");
            d.reservierungen.push_back(std::move(br));
        }
    }

    return d;
}

} // namespace comdare::cache_engine::builder::bestandslog
