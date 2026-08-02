// test_g3_sha512_index -- G3 / #46b Lagerhaltung, Scheibe B3.
//
// Factory + SHA512-Index. Kern-Abnahme (Section 66 Lager-Gate, EINE SHA512-Wahrheit): die
// BinaryKeyPolicy-Digest ueber die vier eingefrorenen A1-Stempel-Zeilen MUSS exakt den in A1
// eingefrorenen Fingerprint ergeben -- ein Testvektor, zwei Module. Der Vektor ist mit
// tests/unit/test_m_w12_stamp_bausteine.cpp (MW12StampBausteine.FrozenFingerprintTestVectorForLagerGateB3)
// gepinnt: gleiche 4 Zeilen, gleiche direkte Konkatenation, gleicher 128-hex. Weiter: Hit/Miss,
// zwei Genera getrennt, Hex-Roundtrip, Index-Aufbau aus Dokument.

#include "bestandslog/bestandslog_document.hpp"
#include "bestandslog/bestandslog_factory.hpp"
#include "bestandslog/bestandslog_index.hpp"

#include <gtest/gtest.h>

#include <array>
#include <string>
#include <string_view>

namespace bl = comdare::cache_engine::builder::bestandslog;

namespace {

// Die vier EINGEFRORENEN A1-Stempel-Zeilen (Quelle: test_m_w12_stamp_bausteine.cpp). NIE aendern
// ohne A1-Neu-Einfrierung -- Drift hier ODER dort bricht die Lane-B-Konsistenz.
//
// A13-M2 (Owner-E2/Q1 vom 02.08.2026): der Fingerprint-Global-Shift von A13-M2 (System-Zeile +Klammer-Anhang,
// Mess-Zeile load_framework ans Ende) beruehrt DIESEN Vektor NICHT -- er rechnet ueber die vier FESTEN Literale
// unten, nicht ueber die live gerenderten Stempel-Zeilen. Literal geprueft: der Hex ist unveraendert, beide
// Module sind gruen. NEU EINZUFRIEREN ist er mit A13-M3, wenn die merge-ZEILE ersatzlos entfaellt (Owner-E2:
// "Merge Zeile kann daher nicht existieren") -- dann faellt kMerge aus dem Preimage. Der Neuanker gehoert dann
// in EINEN Commit mit test_m_w12_stamp_bausteine.cpp (Lane-B-Drift-Verbot); die ausfuehrliche Begruendung steht
// dort ueber MW12StampBausteine.FrozenFingerprintTestVectorForLagerGateB3.
constexpr std::string_view kOrgan   = "search_algo=k_ary@1.0.0;path_compression=path_compression_none@1.0.0";
constexpr std::string_view kSystem  = "compiler=code@1.0.0;isa=amd64";
constexpr std::string_view kMeasure = "wallclock@1.0.0";
constexpr std::string_view kMerge   = "merge=Stufe1_CeOnly;pruefling=self";

// Der eingefrorene 128-hex (== kFrozenFingerprintV1 in A1).
constexpr std::string_view kFrozenFingerprintV1 = "0f0c0eb44d4308c3a9d05f92abcb10a8fa68063634a5bd669ae38f8ac2272285"
                                                  "fb594f0bbdc4547f1bb73f57a5a17d32bee21d3781be27da9577505ad5c31b93";

std::array<std::string_view, 4> frozen_lines() { return {kOrgan, kSystem, kMeasure, kMerge}; }

// Zwei Zell-Koordinaten-Saetze DERSELBEN Permutation, die sich nur in der ISA unterscheiden
// (Section 62-NACHTRAG-4). Genau der Fall, den der Fingerprint allein NICHT auseinanderhaelt.
bl::ZellKoordinaten zelle_avx2() { return {.combo = "default", .opt = "O2", .simd = "avx2"}; }
bl::ZellKoordinaten zelle_avx512() { return {.combo = "default", .opt = "O2", .simd = "avx512"}; }

} // namespace

// ---------------------------------------------------------------------------
// EINE SHA512-Wahrheit: BinaryKeyPolicy-Digest == A1-Fingerprint (ein Vektor, zwei Module).
// ---------------------------------------------------------------------------
TEST(G3Sha512Index, BinaryKeyMatchesFrozenAnatomyFingerprint) {
    auto const          lines = frozen_lines();
    bl::Sha512Key const key   = bl::BinaryKeyPolicy::derive_key(lines);
    EXPECT_EQ(bl::to_hex(key), kFrozenFingerprintV1);

    // Der freie Ableiter derive_key_from_lines liefert dieselbe Digest.
    EXPECT_EQ(bl::to_hex(bl::derive_key_from_lines(lines)), kFrozenFingerprintV1);
}

// ---------------------------------------------------------------------------
// Hex-Roundtrip Sha512Key <-> 128-hex.
// ---------------------------------------------------------------------------
TEST(G3Sha512Index, HexRoundtrip) {
    auto k = bl::key_from_hex(kFrozenFingerprintV1);
    ASSERT_TRUE(k.has_value());
    EXPECT_EQ(bl::to_hex(*k), kFrozenFingerprintV1);

    EXPECT_FALSE(bl::key_from_hex("zzzz").has_value());                // falsche Laenge
    EXPECT_FALSE(bl::key_from_hex(std::string(128, 'g')).has_value()); // Nicht-Hex
}

// ---------------------------------------------------------------------------
// Hit/Miss auf dem Index.
// ---------------------------------------------------------------------------
TEST(G3Sha512Index, HitAndMiss) {
    auto bestand = bl::make_binary_bestand();
    auto lines   = frozen_lines();

    bl::BestandEintrag e;
    e.pfad     = "tier/perm_00042.dll";
    e.bytes    = 428032;
    e.stempel  = "[d,e,f][g,h,i]+bt=Release";
    e.done_utc = "2026-07-23T12:05:11Z";
    bestand.add(lines, zelle_avx2(), e);

    bl::LagerKey const key = bl::Bestand<bl::BinaryKeyPolicy>::lager_key_of(lines, zelle_avx2());
    EXPECT_TRUE(bestand.contains(key));
    ASSERT_NE(bestand.find(key), nullptr);
    EXPECT_EQ(bestand.find(key)->pfad, "tier/perm_00042.dll");
    EXPECT_EQ(bestand.find(key)->key_sha512, kFrozenFingerprintV1); // add() hat den hex gesetzt
    EXPECT_EQ(bestand.find(key)->zelle, zelle_avx2());              // add() hat die Zelle mitgesetzt

    // Der Digest-Teil bleibt die EINE SHA512-Wahrheit -- die Zelle veraendert ihn nicht.
    EXPECT_EQ(bl::to_hex(key.sha), kFrozenFingerprintV1);
    EXPECT_EQ(bl::Bestand<bl::BinaryKeyPolicy>::key_of(lines), key.sha);

    // Miss: ein anderer Digest ist nicht drin.
    std::array<std::string_view, 1> other{"etwas-anderes"};
    EXPECT_FALSE(bestand.contains(bl::BinaryKeyPolicy::derive_lager_key(other, zelle_avx2())));
    EXPECT_EQ(bestand.size(), 1u);
}

// ---------------------------------------------------------------------------
// Section 62-NACHTRAG-4 (der Kern-Grund der Erweiterung): GLEICHE Permutation, VERSCHIEDENE ISA ->
// ZWEI Eintraege. Ueber den Fingerprint allein waere der zweite Bau ein Dedup-Treffer gewesen und
// seine Binary im Lager nie erfasst worden.
// ---------------------------------------------------------------------------
TEST(G3Sha512Index, SamePermutationDifferentSimdAreTwoEntries) {
    auto       bestand = bl::make_binary_bestand();
    auto const lines   = frozen_lines();

    bl::BestandEintrag a;
    a.pfad = "tier/avx2/perm_00042.dll";
    bestand.add(lines, zelle_avx2(), a);

    bl::BestandEintrag b;
    b.pfad = "tier/avx512/perm_00042.dll";
    bestand.add(lines, zelle_avx512(), b);

    EXPECT_EQ(bestand.size(), 2u); // NICHT dedupliziert

    auto const k_avx2   = bl::Bestand<bl::BinaryKeyPolicy>::lager_key_of(lines, zelle_avx2());
    auto const k_avx512 = bl::Bestand<bl::BinaryKeyPolicy>::lager_key_of(lines, zelle_avx512());
    EXPECT_NE(k_avx2, k_avx512);
    EXPECT_EQ(k_avx2.sha, k_avx512.sha); // der Digest-Teil ist identisch -- die Zelle trennt sie

    ASSERT_NE(bestand.find(k_avx2), nullptr);
    ASSERT_NE(bestand.find(k_avx512), nullptr);
    EXPECT_EQ(bestand.find(k_avx2)->pfad, "tier/avx2/perm_00042.dll");
    EXPECT_EQ(bestand.find(k_avx512)->pfad, "tier/avx512/perm_00042.dll");

    // Eine dritte, nicht gebaute Zelle bleibt ein Miss (die Zelle wirkt in BEIDE Richtungen).
    EXPECT_FALSE(bestand.contains(
        bl::Bestand<bl::BinaryKeyPolicy>::lager_key_of(lines, {.combo = "default", .opt = "O3", .simd = "avx2"})));
}

// ---------------------------------------------------------------------------
// Zwei Genera STRIKT getrennt: eigener Index je Client, disjunkte Genus-Kennung.
// ---------------------------------------------------------------------------
TEST(G3Sha512Index, TwoGeneraSeparate) {
    EXPECT_EQ(bl::make_binary_bestand().genus(), bl::Genus::binary);
    EXPECT_EQ(bl::make_messwert_bestand().genus(), bl::Genus::measurement);
    EXPECT_EQ(bl::Bestand<bl::BinaryKeyPolicy>::genus(), bl::Genus::binary);
    EXPECT_EQ(bl::Bestand<bl::MesswertKeyPolicy>::genus(), bl::Genus::measurement);

    auto binary   = bl::make_binary_bestand();
    auto messwert = bl::make_messwert_bestand();
    auto lines    = frozen_lines();

    binary.add(lines, zelle_avx2(), bl::BestandEintrag{});
    bl::LagerKey const key = bl::BinaryKeyPolicy::derive_lager_key(lines, zelle_avx2());
    EXPECT_TRUE(binary.contains(key));
    EXPECT_FALSE(messwert.contains(key)); // separater Index -> kein Cross-Leak
    EXPECT_EQ(messwert.size(), 0u);
}

// ---------------------------------------------------------------------------
// Index-Aufbau aus einem geladenen Bestandslog-Dokument (key_sha512-Hex -> Sha512Key).
// ---------------------------------------------------------------------------
TEST(G3Sha512Index, LoadFromDocument) {
    bl::BestandslogDocument doc;
    doc.genus = bl::Genus::binary;
    bl::BestandEintrag e;
    e.key_sha512 = std::string(kFrozenFingerprintV1);
    e.zelle      = zelle_avx2();
    e.pfad       = "tier/perm_00042.dll";
    e.bytes      = 428032;
    doc.bestand.push_back(e);

    auto bestand = bl::make_binary_bestand();
    bestand.load_from_document(doc);
    EXPECT_EQ(bestand.size(), 1u);

    auto k = bl::lager_key_from_hex(kFrozenFingerprintV1, zelle_avx2());
    ASSERT_TRUE(k.has_value());
    EXPECT_TRUE(bestand.contains(*k));
    // Derselbe Key kommt aus der Stempel-Ableitung -- Dokument-hex und Live-Ableitung treffen sich.
    EXPECT_TRUE(bestand.contains(bl::BinaryKeyPolicy::derive_lager_key(frozen_lines(), zelle_avx2())));
    // ... aber NICHT unter einer anderen Zelle: die geladene Zelle reist mit in den Schluessel.
    EXPECT_FALSE(bestand.contains(bl::BinaryKeyPolicy::derive_lager_key(frozen_lines(), zelle_avx512())));

    // Ein Eintrag mit ungueltigem Hex gehoert nicht in den Index (er waere nicht adressierbar).
    bl::BestandslogDocument bad;
    bad.genus = bl::Genus::binary;
    bl::BestandEintrag kaputt;
    kaputt.key_sha512 = "kein-hex";
    kaputt.zelle      = zelle_avx2();
    bad.bestand.push_back(kaputt);
    auto empty = bl::make_binary_bestand();
    empty.load_from_document(bad);
    EXPECT_EQ(empty.size(), 0u);
}
