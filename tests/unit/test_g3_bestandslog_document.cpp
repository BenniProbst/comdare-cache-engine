// test_g3_bestandslog_document -- G3 / #46b Lagerhaltung, Scheibe B1.
//
// Deckt die Serialisierungs-Schicht des Bestandslogs ab: PODs, deterministischer XML-Emitter,
// Parser ueber den common-DOM. Kern-Abnahme (§66 Lager-Gate): BYTE-stabiler Roundtrip
// emit(parse(emit(d))) == emit(d), Header-Versionen (syntax/semantics), doc_revision, beide
// Genera, Entity-Escaping (INVERSE zu decode_entities) und Ablehnung fehlerhafter Eingaben.

#include "bestandslog/bestandslog_document.hpp"

#include <gtest/gtest.h>

#include <string>

namespace bl = comdare::cache_engine::builder::bestandslog;

namespace {

// Ein voll besetztes Referenz-Dokument (beide Bestands-Eintraege, beide Reservierungs-Zustaende,
// ein kalibrierter + ein pro-forma-Batch, alle Batch-Typen abgedeckt).
bl::BestandslogDocument make_reference_binary() {
    bl::BestandslogDocument d;
    d.syntax_version    = bl::kSyntaxVersion;
    d.semantics_version = bl::kSemanticsVersion;
    d.genus             = bl::Genus::binary;
    d.doc_revision      = 7;
    d.created_utc       = "2026-07-23T12:00:00Z";

    d.bestand.push_back(bl::BestandEintrag{.key_sha512 = std::string(128, 'a'),
                                           .pfad       = "tier/perm_00042.dll",
                                           .bytes      = 428032,
                                           .stempel    = "[d,e,f][g,h,i]+bt=Release",
                                           .done_utc   = "2026-07-23T12:05:11Z"});
    d.bestand.push_back(bl::BestandEintrag{.key_sha512 = std::string(128, 'b'),
                                           .pfad       = "ceb/cache_engine_builder",
                                           .bytes      = 12000000,
                                           .stempel    = "[a,b,c]",
                                           .done_utc   = "2026-07-23T12:06:00Z"});

    d.reservierungen.push_back(bl::BatchReservierung{.id                = "owner-1234/0",
                                                     .typ               = bl::BatchTyp::tier,
                                                     .slice_begin       = 0,
                                                     .slice_count       = 4096,
                                                     .maschine          = "prod1",
                                                     .threads           = 32,
                                                     .reserviert_utc    = "2026-07-23T12:00:01Z",
                                                     .pro_forma_bis_utc = "2026-07-23T12:30:01Z",
                                                     .eta_s             = "912.5",
                                                     .avg_size_bytes    = "428032",
                                                     .status            = bl::BatchStatus::offen});
    d.reservierungen.push_back(bl::BatchReservierung{.id                = "owner-1234/1",
                                                     .typ               = bl::BatchTyp::planer_block,
                                                     .slice_begin       = 4096,
                                                     .slice_count       = 4096,
                                                     .maschine          = "prod2",
                                                     .threads           = 24,
                                                     .reserviert_utc    = "2026-07-23T12:00:02Z",
                                                     .pro_forma_bis_utc = "2026-07-23T12:30:02Z",
                                                     .eta_s             = "", // planer_block: keine ETA (B7)
                                                     .avg_size_bytes    = "",
                                                     .status            = bl::BatchStatus::done});
    return d;
}

} // namespace

// ---------------------------------------------------------------------------
// Byte-stabiler Roundtrip + Feld-Identitaet (die Kern-Abnahme B1).
// ---------------------------------------------------------------------------
TEST(G3BestandslogDocument, ByteStableRoundtrip) {
    auto const        d  = make_reference_binary();
    std::string const x1 = bl::emit_document(d);

    auto const parsed = bl::parse_bestandslog(x1);
    ASSERT_TRUE(parsed.has_value());

    // Feld-Identitaet: parse(emit(d)) == d.
    EXPECT_EQ(*parsed, d);

    // Byte-Identitaet: emit(parse(emit(d))) == emit(d).
    std::string const x2 = bl::emit_document(*parsed);
    EXPECT_EQ(x1, x2);
}

TEST(G3BestandslogDocument, HeaderVersionsPreservedAndGated) {
    auto const d      = make_reference_binary();
    auto const parsed = bl::parse_bestandslog(bl::emit_document(d));
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->syntax_version, bl::kSyntaxVersion);
    EXPECT_EQ(parsed->semantics_version, bl::kSemanticsVersion);
    EXPECT_TRUE(bl::document_syntax_supported(*parsed));

    // Ein Dokument mit HOEHERER Wire-Grammatik round-trippt treu, gilt aber als nicht vertraeglich.
    bl::BestandslogDocument future = d;
    future.syntax_version          = bl::kSyntaxVersion + 1;
    auto const future_parsed       = bl::parse_bestandslog(bl::emit_document(future));
    ASSERT_TRUE(future_parsed.has_value());
    EXPECT_EQ(future_parsed->syntax_version, bl::kSyntaxVersion + 1);
    EXPECT_FALSE(bl::document_syntax_supported(*future_parsed));

    // semantics_version darf differieren (additiv) und bleibt vertraeglich.
    bl::BestandslogDocument newer_sem = d;
    newer_sem.semantics_version       = bl::kSemanticsVersion + 5;
    auto const sem_parsed             = bl::parse_bestandslog(bl::emit_document(newer_sem));
    ASSERT_TRUE(sem_parsed.has_value());
    EXPECT_EQ(sem_parsed->semantics_version, bl::kSemanticsVersion + 5);
    EXPECT_TRUE(bl::document_syntax_supported(*sem_parsed));
}

TEST(G3BestandslogDocument, DocRevisionMonotonPreserved) {
    auto d         = make_reference_binary();
    d.doc_revision = 4242;
    auto const p   = bl::parse_bestandslog(bl::emit_document(d));
    ASSERT_TRUE(p.has_value());
    EXPECT_EQ(p->doc_revision, 4242u);
}

TEST(G3BestandslogDocument, MeasurementGenusRoundtrip) {
    auto d  = make_reference_binary();
    d.genus = bl::Genus::measurement;
    d.bestand.clear();
    d.bestand.push_back(bl::BestandEintrag{.key_sha512 = std::string(128, 'c'),
                                           .pfad       = "measure/cell_00007.csv",
                                           .bytes      = 8192,
                                           .stempel    = "[d,e,f][g,h,i]+hwident",
                                           .done_utc   = "2026-07-23T13:00:00Z"});
    std::string const x1 = bl::emit_document(d);
    auto const        p  = bl::parse_bestandslog(x1);
    ASSERT_TRUE(p.has_value());
    EXPECT_EQ(p->genus, bl::Genus::measurement);
    EXPECT_EQ(*p, d);
    EXPECT_EQ(bl::emit_document(*p), x1);
}

TEST(G3BestandslogDocument, EmptyDocumentRoundtrip) {
    bl::BestandslogDocument d;
    d.created_utc        = "2026-07-23T00:00:00Z";
    std::string const x1 = bl::emit_document(d);
    auto const        p  = bl::parse_bestandslog(x1);
    ASSERT_TRUE(p.has_value());
    EXPECT_TRUE(p->bestand.empty());
    EXPECT_TRUE(p->reservierungen.empty());
    EXPECT_EQ(*p, d);
    EXPECT_EQ(bl::emit_document(*p), x1);
}

// ---------------------------------------------------------------------------
// Entity-Escaping: xml_encode ist die INVERSE zu decode_entities -> Sonderzeichen ueberleben
// den Roundtrip exakt (& < > " ' in Attribut-Werten). Beweist auch Byte-Stabilitaet bei Escapes.
// ---------------------------------------------------------------------------
TEST(G3BestandslogDocument, EntityEscapingRoundtrip) {
    bl::BestandslogDocument d;
    d.created_utc = "t";
    d.bestand.push_back(bl::BestandEintrag{.key_sha512 = std::string(128, 'd'),
                                           .pfad       = "path/with & < > \" ' chars",
                                           .bytes      = 1,
                                           .stempel    = "[d,e,f] & [g,h,i] <bt=\"Release\">",
                                           .done_utc   = "t2"});

    std::string const x1 = bl::emit_document(d);
    auto const        p  = bl::parse_bestandslog(x1);
    ASSERT_TRUE(p.has_value());
    ASSERT_EQ(p->bestand.size(), 1u);
    EXPECT_EQ(p->bestand[0].pfad, "path/with & < > \" ' chars");
    EXPECT_EQ(p->bestand[0].stempel, "[d,e,f] & [g,h,i] <bt=\"Release\">");
    EXPECT_EQ(*p, d);
    EXPECT_EQ(bl::emit_document(*p), x1);
}

// ---------------------------------------------------------------------------
// Ablehnung: nicht wohlgeformt / falsches Wurzel-Tag / unbekannter Enum-Wert -> nullopt
// (nie stille Fehlfaerbung).
// ---------------------------------------------------------------------------
TEST(G3BestandslogDocument, RejectsMalformedAndWrongRoot) {
    EXPECT_FALSE(bl::parse_bestandslog("not xml at all").has_value());
    EXPECT_FALSE(bl::parse_bestandslog("<other genus=\"binary\"></other>").has_value());
    // Fehlendes genus-Attribut -> unbekannt -> Ablehnung.
    EXPECT_FALSE(bl::parse_bestandslog("<bestandslog syntax_version=\"1\"></bestandslog>").has_value());
}

TEST(G3BestandslogDocument, RejectsUnknownEnumValues) {
    // Unbekanntes genus.
    EXPECT_FALSE(bl::parse_bestandslog("<bestandslog syntax_version=\"1\" genus=\"wolke\"></bestandslog>").has_value());

    // Unbekannter batch-Typ.
    char const* bad_typ = "<bestandslog syntax_version=\"1\" genus=\"binary\">"
                          "<reservierungen><batch id=\"x/0\" typ=\"unbekannt\" status=\"offen\"/></reservierungen>"
                          "</bestandslog>";
    EXPECT_FALSE(bl::parse_bestandslog(bad_typ).has_value());

    // Unbekannter status.
    char const* bad_status = "<bestandslog syntax_version=\"1\" genus=\"binary\">"
                             "<reservierungen><batch id=\"x/0\" typ=\"tier\" status=\"halb\"/></reservierungen>"
                             "</bestandslog>";
    EXPECT_FALSE(bl::parse_bestandslog(bad_status).has_value());
}

// ---------------------------------------------------------------------------
// Enum <-> String exakt (die Grammatik-Terme).
// ---------------------------------------------------------------------------
TEST(G3BestandslogDocument, EnumStringMapping) {
    EXPECT_EQ(bl::to_string(bl::Genus::binary), "binary");
    EXPECT_EQ(bl::to_string(bl::Genus::measurement), "measurement");
    EXPECT_EQ(bl::genus_from_string("binary"), bl::Genus::binary);
    EXPECT_EQ(bl::genus_from_string("measurement"), bl::Genus::measurement);
    EXPECT_FALSE(bl::genus_from_string("x").has_value());

    EXPECT_EQ(bl::to_string(bl::BatchTyp::tier), "tier");
    EXPECT_EQ(bl::to_string(bl::BatchTyp::ceb), "ceb");
    EXPECT_EQ(bl::to_string(bl::BatchTyp::planer_block), "planer_block");
    EXPECT_EQ(bl::batch_typ_from_string("planer_block"), bl::BatchTyp::planer_block);
    EXPECT_FALSE(bl::batch_typ_from_string("x").has_value());

    EXPECT_EQ(bl::to_string(bl::BatchStatus::offen), "offen");
    EXPECT_EQ(bl::to_string(bl::BatchStatus::done), "done");
    EXPECT_EQ(bl::to_string(bl::BatchStatus::released), "released");
    EXPECT_EQ(bl::batch_status_from_string("released"), bl::BatchStatus::released);
    EXPECT_FALSE(bl::batch_status_from_string("x").has_value());
}
