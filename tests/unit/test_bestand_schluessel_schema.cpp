// test_bestand_schluessel_schema -- I-1-Schema-Wache (#91-Vollzug W2-D, 2026-08-21): das
// CT-Schluessel-SCHEMA der vier Lager-Bestaende (include/cache_engine/lager/
// bestand_schluessel_schema.hpp) gegen seine FREMDEN Quellen.
//
// NENNER-DOKTRIN (T-3): jeder Nenner dieses Tests ist ein EIGENES Literal aus der Quelle
// (design91-v2 Klasse B I-1, Par.62-B, messwert_key_source.hpp-Wortlaut, KON110-04, V-10b) --
// NIE aus den kCount-Konstanten des Prueflings abgeschrieben.
//
// GEGENEINGANG (T-4): unbekannte Bestands-Nummern muessen nullptr liefern (nie still Default).

#include <cache_engine/lager/bestand_schluessel_schema.hpp>

#include "bestandslog/bestandslog_document.hpp" // Genus {binary, measurement} -- die gebaute Par.62-B-Quelle

#include <gtest/gtest.h>

#include <cstddef>
#include <string_view>

namespace lager = comdare::cache_engine::lager;
namespace blog  = comdare::cache_engine::builder::bestandslog;

// -- Nenner fremd: VIER Bestaende (Typ-1..4-Vokabular KON110-04), nicht kBestandArtCount nachgeplappert.
TEST(BestandSchluesselSchema, VierBestaendeMitNummern1Bis4) {
    ASSERT_EQ(lager::kBestandArtCount, static_cast<std::size_t>(4));
    for (std::size_t i = 0; i < 4; ++i) {
        EXPECT_EQ(lager::kBestandSchluesselSchema[i].bestand_nr, static_cast<std::uint8_t>(i + 1));
    }
}

// -- Genus-Kopplung an die GEBAUTE Par.62-B-Quelle: das Bestandslog kennt heute GENAU die zwei
// Genera binary/measurement (bestandslog_document.hpp); das Schema traegt fuer genau diese zwei
// die Tokens und laesst 3/4 namenlos (R-2: Benennung faellt im D-2-Fenster, nicht hier).
TEST(BestandSchluesselSchema, GenusTokensDeckenDenGebautenBestand) {
    static_assert(static_cast<int>(blog::Genus::binary) == 0 && static_cast<int>(blog::Genus::measurement) == 1,
                  "Par.62-B: die zwei gebauten Genera in dieser Ordnung (Ordinal-Pin, bewusstes Literal).");
    EXPECT_EQ(lager::kBestandSchluesselSchema[0].genus_token, std::string_view{"binary"});
    EXPECT_EQ(lager::kBestandSchluesselSchema[1].genus_token, std::string_view{"measurement"});
    EXPECT_TRUE(lager::kBestandSchluesselSchema[2].genus_token.empty());
    EXPECT_TRUE(lager::kBestandSchluesselSchema[3].genus_token.empty());
}

// -- Bestand 2 verbatim messwert_key_source.hpp: [0] Fingerprint der Binary, [1] Hardware-
// Identitaet; Zelle [d,e,f] DANEBEN (Section 62-NACHTRAG-4), nie im Digest.
TEST(BestandSchluesselSchema, Bestand2KomponentenUndZellKlammer) {
    auto const* z = lager::bestand_schluessel_schema_of(2);
    ASSERT_NE(z, nullptr);
    ASSERT_EQ(z->komponenten_zahl, static_cast<std::size_t>(2));
    EXPECT_EQ(z->komponenten[0], std::string_view{"binary_fingerprint_128hex"});
    EXPECT_EQ(z->komponenten[1], std::string_view{"hardware_identitaet"});
    EXPECT_EQ(z->zell_klammer, std::string_view{"[d,e,f]"});
}

// -- Bestand 3/4 verbatim design91-v2 Klasse B I-1 (Text-Reihenfolge der Komponenten).
TEST(BestandSchluesselSchema, Bestand3Und4KomponentenVerbatimDesign91) {
    auto const* b3 = lager::bestand_schluessel_schema_of(3);
    ASSERT_NE(b3, nullptr);
    ASSERT_EQ(b3->komponenten_zahl, static_cast<std::size_t>(3));
    EXPECT_EQ(b3->komponenten[0], std::string_view{"machine_id"});
    EXPECT_EQ(b3->komponenten[1], std::string_view{"voll_stempel_fingerprint"});
    EXPECT_EQ(b3->komponenten[2], std::string_view{"mess_ebenen_kanal_referenz"});
    auto const* b4 = lager::bestand_schluessel_schema_of(4);
    ASSERT_NE(b4, nullptr);
    ASSERT_EQ(b4->komponenten_zahl, static_cast<std::size_t>(3));
    EXPECT_EQ(b4->komponenten[0], std::string_view{"xml_c14n_hash"});
    EXPECT_EQ(b4->komponenten[1], std::string_view{"machine_id"});
    EXPECT_EQ(b4->komponenten[2], std::string_view{"bestands_stempel_referenzen"});
}

// -- Die Doktrin-Konstanten: EINE Hash-Wahrheit (SHA-512-Linie, kein neues Verfahren),
// Invalidierung = Ergaenzung ja / Kernbestand bleibt (KON110-04), Verbund1-Skip JE machine_id
// und NUR als Bau-Skip (V-10b: "Bau-SKIP ja / Mess-SKIP nein").
TEST(BestandSchluesselSchema, DoktrinKonstanten) {
    EXPECT_EQ(lager::kEineHashWahrheit, std::string_view{"sha512/ctsha512"});
    EXPECT_EQ(lager::kKanonischeFormenBestand[0], std::string_view{"canonical_combo"});
    EXPECT_EQ(lager::kKanonischeFormenBestand[1], std::string_view{"ceb_key_sha512"});
    EXPECT_EQ(lager::kKanonischeFormenBestand[2], std::string_view{"ctsha512"});
    EXPECT_TRUE(lager::kInvalidierungErgaenzungJa);
    EXPECT_TRUE(lager::kKernbestandBleibt);
    EXPECT_TRUE(lager::kVerbund1SkipPrueftJeMachineId);
    EXPECT_FALSE(lager::kVerbund1SkipPrueftGlobal);
    EXPECT_TRUE(lager::kVerbund1BauSkip);
    EXPECT_FALSE(lager::kVerbund1MessSkip);
}

// -- Gegeneingang (T-4): unbekannte Nummern (0, 5, 255) liefern nullptr, nie einen Default.
TEST(BestandSchluesselSchema, GegeneingangUnbekannteNummer) {
    EXPECT_EQ(lager::bestand_schluessel_schema_of(0), nullptr);
    EXPECT_EQ(lager::bestand_schluessel_schema_of(5), nullptr);
    EXPECT_EQ(lager::bestand_schluessel_schema_of(255), nullptr);
}
