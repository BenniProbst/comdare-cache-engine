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
                                           .zelle      = {.combo = "default", .opt = "O2", .simd = "avx2"},
                                           .pfad       = "tier/perm_00042.dll",
                                           .bytes      = 428032,
                                           .stempel    = "[d,e,f][g,h,i]+bt=Release",
                                           .done_utc   = "2026-07-23T12:05:11Z"});
    d.bestand.push_back(bl::BestandEintrag{.key_sha512 = std::string(128, 'b'),
                                           .zelle      = {.combo = "", .opt = "O3", .simd = "avx512"},
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
                                                     .status            = bl::BatchStatus::offen,
                                                     .ceb_legende       = "", // tier-Batch: keine CEB-Bindung
                                                     .ceb_key_sha512    = ""});
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
                                                     .status            = bl::BatchStatus::done,
                                                     // planer_block TRAEGT die CEB-Bindung (syntax_version 3):
                                                     // zwei getrennte Felder, keine Fusion in die id.
                                                     .ceb_legende    = "[a,b,c]",
                                                     .ceb_key_sha512 = std::string(128, 'e')});
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
                                           .zelle      = {}, // Messwert-Genus ohne gemeldete Zelle
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
                                           .zelle      = {},
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
// §62-NACHTRAG-4: die drei Zell-Koordinaten sind SEPARATE Attribute, ueberleben den Roundtrip
// einzeln und tragen die Identitaet als TUPEL (kein fusionierter Schluessel-String).
// ---------------------------------------------------------------------------
TEST(G3BestandslogDocument, ZellKoordinatenAreSeparateAttributes) {
    auto const        d  = make_reference_binary();
    std::string const x1 = bl::emit_document(d);

    // Die Koordinaten stehen als eigene Attribute im Wire-Format -- nicht in einen Key konkateniert.
    EXPECT_NE(x1.find("combo=\"default\""), std::string::npos);
    EXPECT_NE(x1.find("opt=\"O2\""), std::string::npos);
    EXPECT_NE(x1.find("simd=\"avx2\""), std::string::npos);

    auto const p = bl::parse_bestandslog(x1);
    ASSERT_TRUE(p.has_value());
    ASSERT_EQ(p->bestand.size(), 2u);
    EXPECT_EQ(p->bestand[0].zelle.combo, "default");
    EXPECT_EQ(p->bestand[0].zelle.opt, "O2");
    EXPECT_EQ(p->bestand[0].zelle.simd, "avx2");
    EXPECT_TRUE(p->bestand[1].zelle.combo.empty()); // leere Koordinate bleibt leer
    EXPECT_EQ(p->bestand[1].zelle.simd, "avx512");
}

TEST(G3BestandslogDocument, EintragIdentityIsTheTuple) {
    bl::BestandEintrag a;
    a.key_sha512         = std::string(128, 'a');
    a.zelle              = {.combo = "default", .opt = "O2", .simd = "avx2"};
    bl::BestandEintrag b = a;
    EXPECT_TRUE(bl::same_eintrag_identity(a, b));
    EXPECT_FALSE(bl::eintrag_identity_less(a, b));
    EXPECT_FALSE(bl::eintrag_identity_less(b, a));

    // Gleicher Fingerprint, andere ISA -> ANDERE Identitaet (der Kern-Grund der Erweiterung).
    b.zelle.simd = "avx512";
    EXPECT_FALSE(bl::same_eintrag_identity(a, b));
    EXPECT_TRUE(bl::eintrag_identity_less(a, b)); // "avx2" < "avx512"

    // Die Nutzdaten gehoeren NICHT zur Identitaet.
    bl::BestandEintrag c = a;
    c.pfad               = "ganz/anderer/pfad";
    c.bytes              = 999;
    EXPECT_TRUE(bl::same_eintrag_identity(a, c));
}

// Ein v1-Dokument (ohne die drei Attribute) bleibt lesbar: fehlende Koordinaten = leer.
TEST(G3BestandslogDocument, ReadsLegacyV1DocumentWithoutCellAttributes) {
    char const* v1 = "<bestandslog syntax_version=\"1\" semantics_version=\"1\" genus=\"binary\" doc_revision=\"3\">"
                     "<bestand><eintrag key_sha512=\"abc\" pfad=\"tier/0.dll\" bytes=\"7\"/></bestand>"
                     "</bestandslog>";
    auto const  p  = bl::parse_bestandslog(v1);
    ASSERT_TRUE(p.has_value());
    EXPECT_EQ(p->syntax_version, 1);
    EXPECT_TRUE(bl::document_syntax_supported(*p)); // aeltere Grammatik bleibt vertraeglich
    ASSERT_EQ(p->bestand.size(), 1u);
    EXPECT_TRUE(p->bestand[0].zelle.empty());
    EXPECT_EQ(p->bestand[0].bytes, 7u);
}

// Der Syntax-Bump ist gesetzt und schuetzt genau nach unten: ein aelterer LESER lehnt ein neueres
// Dokument ab, waehrend der heutige Leser alle aelteren Grammatiken vertraegt.
TEST(G3BestandslogDocument, SyntaxVersionBumpedToThree) {
    EXPECT_EQ(bl::kSyntaxVersion, 3);
    EXPECT_EQ(bl::kSemanticsVersion, 1);

    bl::BestandslogDocument d;
    EXPECT_EQ(d.syntax_version, 3); // frisch erzeugte Dokumente tragen die neue Grammatik
    EXPECT_NE(bl::emit_document(d).find("syntax_version=\"3\""), std::string::npos);

    // Nach unten vertraeglich: v1 (ohne Zell-Koordinaten) und v2 (ohne CEB-Bindung) bleiben lesbar.
    for (int alt_version : {1, 2}) {
        bl::BestandslogDocument alt;
        alt.syntax_version = alt_version;
        EXPECT_TRUE(bl::document_syntax_supported(alt)) << alt_version;
    }
    // Nach oben geschlossen: die naechste Grammatik ist fuer diesen Leser nicht sicher lesbar.
    bl::BestandslogDocument zukunft;
    zukunft.syntax_version = 4;
    EXPECT_FALSE(bl::document_syntax_supported(zukunft));

    // Und der Sinn des Bumps: ein v2-Leser haette die zwei neuen batch-Attribute stillschweigend
    // geschluckt und eine versions-FREMDE Reservierung fuer die eigene gehalten.
    EXPECT_GT(d.syntax_version, 2);
}

// ---------------------------------------------------------------------------
// syntax_version 3: die CEB-BINDUNG am <batch> -- zwei getrennte, OPTIONALE Attribute (keine Fusion
// in die id, §66-N3). Getragen vom planer_block; tier-Batches lassen sie leer.
// ---------------------------------------------------------------------------
TEST(G3BestandslogDocument, CebBindingRoundtrip) {
    bl::BestandslogDocument d;
    d.created_utc = "2026-07-26T12:00:00Z";
    bl::BatchReservierung r;
    r.id             = "owner-1234/plan/0";
    r.typ            = bl::BatchTyp::planer_block;
    r.maschine       = "prod1";
    r.threads        = 32;
    r.ceb_legende    = "[a,b,c]";
    r.ceb_key_sha512 = std::string(128, 'e');
    d.reservierungen.push_back(r);

    std::string const x1 = bl::emit_document(d);
    EXPECT_NE(x1.find("ceb_legende=\"[a,b,c]\""), std::string::npos) << x1;
    EXPECT_NE(x1.find("ceb_key_sha512=\"" + std::string(128, 'e') + "\""), std::string::npos);
    // Die Bindung steht HINTER status -- die Attribut-Reihenfolge ist Teil der Byte-Stabilitaet.
    EXPECT_LT(x1.find("status="), x1.find("ceb_legende="));

    auto const p = bl::parse_bestandslog(x1);
    ASSERT_TRUE(p.has_value());
    ASSERT_EQ(p->reservierungen.size(), 1u);
    EXPECT_EQ(p->reservierungen[0].ceb_legende, "[a,b,c]");
    EXPECT_EQ(p->reservierungen[0].ceb_key_sha512, std::string(128, 'e'));
    EXPECT_EQ(*p, d);                     // Feld-Identitaet
    EXPECT_EQ(bl::emit_document(*p), x1); // Byte-Identitaet
    EXPECT_TRUE(bl::document_syntax_supported(*p));

    // Nur EIN Feld gefuellt ist zulaessig (die Felder sind getrennt geklammert, nicht gekoppelt).
    bl::BestandslogDocument nur_legende          = d;
    nur_legende.reservierungen[0].ceb_key_sha512 = "";
    std::string const y                          = bl::emit_document(nur_legende);
    EXPECT_NE(y.find("ceb_legende="), std::string::npos);
    EXPECT_EQ(y.find("ceb_key_sha512="), std::string::npos);
    auto const py = bl::parse_bestandslog(y);
    ASSERT_TRUE(py.has_value());
    EXPECT_EQ(*py, nur_legende);
}

TEST(G3BestandslogDocument, CebBindingOmittedWhenEmptyKeepsV2Bytes) {
    // Ohne Bindung schreibt der Emitter die Attribute NICHT -- die batch-Zeile ist dann byte-identisch
    // zur v2-Form. Deshalb steht hier die erwartete Zeile literal, nicht als Suchmuster.
    bl::BestandslogDocument d;
    bl::BatchReservierung   r;
    r.id     = "owner-1234/0";
    r.typ    = bl::BatchTyp::tier;
    r.status = bl::BatchStatus::offen;
    d.reservierungen.push_back(r);

    std::string const x = bl::emit_document(d);
    EXPECT_EQ(x.find("ceb_legende="), std::string::npos);
    EXPECT_EQ(x.find("ceb_key_sha512="), std::string::npos);
    EXPECT_NE(x.find("    <batch id=\"owner-1234/0\" typ=\"tier\" slice_begin=\"0\" slice_count=\"0\" maschine=\"\" "
                     "threads=\"0\" reserviert_utc=\"\" pro_forma_bis_utc=\"\" eta_s=\"\" avg_size_bytes=\"\" "
                     "status=\"offen\"/>\n"),
              std::string::npos)
        << x;
}

// Ein v2-Dokument (mit Zell-Koordinaten, ohne CEB-Bindung) bleibt lesbar: fehlende Attribute = leer.
TEST(G3BestandslogDocument, ReadsLegacyV2DocumentWithoutCebAttributes) {
    char const* v2 = "<bestandslog syntax_version=\"2\" semantics_version=\"1\" genus=\"binary\" doc_revision=\"9\">"
                     "<bestand><eintrag key_sha512=\"abc\" combo=\"\" opt=\"O2\" simd=\"avx2\" pfad=\"tier/0.dll\" "
                     "bytes=\"7\"/></bestand>"
                     "<reservierungen><batch id=\"owner-9/0\" typ=\"planer_block\" status=\"offen\"/></reservierungen>"
                     "</bestandslog>";
    auto const  p  = bl::parse_bestandslog(v2);
    ASSERT_TRUE(p.has_value());
    EXPECT_EQ(p->syntax_version, 2);
    EXPECT_TRUE(bl::document_syntax_supported(*p)); // aeltere Grammatik bleibt vertraeglich
    ASSERT_EQ(p->reservierungen.size(), 1u);
    EXPECT_TRUE(p->reservierungen[0].ceb_legende.empty());
    EXPECT_TRUE(p->reservierungen[0].ceb_key_sha512.empty());
    EXPECT_EQ(p->reservierungen[0].typ, bl::BatchTyp::planer_block);
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
