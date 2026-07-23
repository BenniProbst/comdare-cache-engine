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
constexpr std::string_view kOrgan   = "search_algo=k_ary@1.0.0;path_compression=path_compression_none@1.0.0";
constexpr std::string_view kSystem  = "compiler=code@1.0.0;isa=amd64";
constexpr std::string_view kMeasure = "wallclock@1.0.0";
constexpr std::string_view kMerge   = "merge=Stufe1_CeOnly;pruefling=self";

// Der eingefrorene 128-hex (== kFrozenFingerprintV1 in A1).
constexpr std::string_view kFrozenFingerprintV1 = "0f0c0eb44d4308c3a9d05f92abcb10a8fa68063634a5bd669ae38f8ac2272285"
                                                  "fb594f0bbdc4547f1bb73f57a5a17d32bee21d3781be27da9577505ad5c31b93";

std::array<std::string_view, 4> frozen_lines() { return {kOrgan, kSystem, kMeasure, kMerge}; }

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
    bestand.add(lines, e);

    bl::Sha512Key const key = bl::Bestand<bl::BinaryKeyPolicy>::key_of(lines);
    EXPECT_TRUE(bestand.contains(key));
    ASSERT_NE(bestand.find(key), nullptr);
    EXPECT_EQ(bestand.find(key)->pfad, "tier/perm_00042.dll");
    EXPECT_EQ(bestand.find(key)->key_sha512, kFrozenFingerprintV1); // add() hat den hex gesetzt

    // Miss: ein anderer Key ist nicht drin.
    std::array<std::string_view, 1> other{"etwas-anderes"};
    EXPECT_FALSE(bestand.contains(bl::BinaryKeyPolicy::derive_key(other)));
    EXPECT_EQ(bestand.size(), 1u);
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

    binary.add(lines, bl::BestandEintrag{});
    bl::Sha512Key const key = bl::BinaryKeyPolicy::derive_key(lines);
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
    e.pfad       = "tier/perm_00042.dll";
    e.bytes      = 428032;
    doc.bestand.push_back(e);

    auto bestand = bl::make_binary_bestand();
    bestand.load_from_document(doc);
    EXPECT_EQ(bestand.size(), 1u);

    auto k = bl::key_from_hex(kFrozenFingerprintV1);
    ASSERT_TRUE(k.has_value());
    EXPECT_TRUE(bestand.contains(*k));
    // Derselbe Key kommt aus der Stempel-Ableitung -- Dokument-hex und Live-Ableitung treffen sich.
    EXPECT_TRUE(bestand.contains(bl::BinaryKeyPolicy::derive_key(frozen_lines())));
}
