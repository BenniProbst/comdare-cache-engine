// test_g3_sha512_index -- G3 / #46b Lagerhaltung, Scheibe B3.
//
// Factory + SHA512-Index. Kern-Abnahme (Section 66 Lager-Gate, EINE SHA512-Wahrheit): die
// BinaryKeyPolicy-Digest ueber die eingefrorenen A1-Stempel-Zeilen MUSS exakt den in A1
// eingefrorenen Fingerprint ergeben -- ein Testvektor, zwei Module. Der Vektor ist mit
// tests/unit/test_m_w12_stamp_bausteine.cpp (MW12StampBausteine.FrozenFingerprintTestVectorForLagerGateB3)
// gepinnt: gleiche Zeilen, gleiche Glied-Folge, gleicher Separator, gleicher 128-hex. Weiter: Hit/Miss,
// zwei Genera getrennt, Hex-Roundtrip, Index-Aufbau aus Dokument.

#include "bestandslog/bestandslog_document.hpp"
#include "bestandslog/bestandslog_factory.hpp"
#include "bestandslog/bestandslog_index.hpp"

#include <cache_engine/abi/anatomy_fingerprint.hpp> // A13-M3: anatomy_fingerprint_glieder (die EINE Ordnung)

#include <gtest/gtest.h>

#include <array>
#include <string>
#include <string_view>

namespace bl = comdare::cache_engine::builder::bestandslog;

namespace {

// Die EINGEFRORENEN A1-Stempel-Zeilen (Quelle: test_m_w12_stamp_bausteine.cpp). NIE aendern
// ohne A1-Neu-Einfrierung -- Drift hier ODER dort bricht die Lane-B-Konsistenz.
//
// O-2/C-2 (05.08.2026) -- DER NEUANKER DIESES FENSTERS, Format 2 -> 3. Der Vorgaenger-Hex 0fe275bd...9fe36
// (A13-M3, 03.08.) ist damit historisch; er steht in der git-Historie. URSACHE, EINZELN benannt: das
// Preimage traegt ab Format 3 ZWEI zusaetzliche Glieder -- das Toolchain-Glied [5] (Compiler-Haupt-Achse
// inkl. Flags, opt_level, atomic128, ext/bt/gate/ceb) und das bvset-Glied [6] (Enabled-Mengen-Signatur);
// das Overlay-Glied wandert ans Ende [7]. Beide neuen Glieder sind in DIESEM Commit noch LEER (die
// per-Perm-Injektion ist die Folge-Scheibe C-3) -- den Hex verschiebt allein der Format-Bump plus die zwei
// zusaetzlichen Separatoren. GENAU SO IST ES GEWOLLT: der Anker faellt EINMAL, nicht zweimal.
// Der neue Hex wurde NICHT vorausberechnet, sondern aus dem literalen Testlauf uebernommen.
//
// HISTORIE A13-M3 (Owner-E2/OF-M3-1 vom 02./03.08.2026) -- der VORIGE Neuanker. Drei Ursachen fielen
// zusammen und ergaben zusammen GENAU EINEN neuen Hex (statt drei aufeinanderfolgender):
//   (1) die merge-ZEILE entfaellt ersatzlos (Owner-E2) -> kMerge faellt aus dem Preimage;
//   (2) OF-M3-1 = Option A: die Glieder sind '\n'-getrennt, mit fingerprint_format-Kennung vorn und dem
//       Sub-Achsen-Werteset-Segment als eigenem Glied (abi::anatomy_fingerprint_glieder);
//   (3) die Fixtures sind in END-Form modernisiert (Owner-Q3-Flag-Grammatik "@1.0.0c" + die HEUTIGEN
//       System-/Mess-Achsen) -- kSystem trug "compiler=code@1.0.0", eine seit O-8 Schritt 4 abgeschaffte
//       System-Haupt-Achse, kMeasure trug "wallclock@1.0.0" ohne Achsen-Praefix. Als Hash-Konsistenz-Anker
//       war das gleichgueltig, als REFERENZ-BEISPIEL las es sich falsch.
// [NEU EINGEFROREN 06.08.2026, NB/CX-4 -- DER ZWEITE UND LETZTE NEUANKER-DREH DIESES BUENDELS. Vorgaenger
// f8f811a9...9137fb0c (O-2/C-2, 05.08.), in der git-Historie. URSACHE: der Vektor rechnete ueber die
// LEEREN Default-Glieder [5]/[6]; seit der Live-Naht tragen beide in jedem realen Bau Werte, der Anker
// deckte also genau den NEUEN Teil des Preimage nicht ab. Er ist in der END-FORM eingefroren -- beide
// Glieder mit LITERALEN Werten belegt (nicht mit den Live-Werten: die haengen an Toolchain und
// Enable-Menge der Maschine und waeren in der 8er-Docker-Matrix je Distro andere).]
// Der neue Hex wurde NICHT vorausberechnet, sondern aus dem literalen Testlauf uebernommen.
constexpr std::string_view kOrgan   = "search_algo=k_ary@1.0.0c;path_compression=path_compression_none@1.0.0c";
constexpr std::string_view kSystem  = "target_isa=code@1.0.0c;operating_system=code@1.0.0c;"
                                      "external_utils=code@1.0.0c;[simd=code@1.0.0c]";
constexpr std::string_view kMeasure = "measurement_tooling=wallclock@1.0.0c;[load_framework=ycsb@1.0.0c]";

// NB/CX-4 END-FORM: die beiden injizierten Glieder sind BELEGT -- byte-gleich zu den Literalen in
// test_m_w12_stamp_bausteine.cpp und test_w10_system_cell_values.cpp (EIN Testvektor, drei Module).
constexpr std::string_view kFrozenToolchain =
    "tc=1;cxx=gcc-16.2.0@1.0.0c;opt=O3{-O3}@1.0.0c;ext=avx512;ceb=8.0;gate=avx512;atomic128=cx16{-mcx16}@1.0.0c";
constexpr std::string_view kFrozenBvset = "bvset=1;bv=2;page_type[{bplus;hw_cache_line=64;hw_numa_capable=0}];"
                                          "simd_extension[{avx512}];"
                                          "general_hardware[{x86_64;hw_cache_line=64;hw_numa_capable=0}]";

// Der eingefrorene 128-hex (== kFrozenFingerprintV1 in A1).
constexpr std::string_view kFrozenFingerprintV1 = "17148e5a4d0f4a2d96e1f5ad97dc4c727b99fce6e38bd6e337fb6dbf0e4461f9"
                                                  "b7fd37fbba76414be4718ad2180deecbb14387293935a8eff1469cef8ce89374";

// Die Glied-Folge kommt aus der EINEN Quelle abi::anatomy_fingerprint_glieder -- der Test darf sie NICHT
// selbst zusammenstellen, sonst pinnt er eine zweite Ordnung fest (Lehre "gruene Tests zementieren alte
// Ordnung"). Format-Kennung, Werteset-Segment, Toolchain-, bvset- und Overlay-Glied kommen damit
// automatisch mit -- auch der O-2/C-2-Nachtrag von zwei Gliedern hat diese Funktion nicht angefasst.
std::array<std::string_view, comdare::cache_engine::abi::kAnatomyFingerprintGliedCount> frozen_lines() {
    return comdare::cache_engine::abi::anatomy_fingerprint_glieder(
        kOrgan, kSystem, kMeasure, comdare::cache_engine::abi::ToolchainGlied{kFrozenToolchain},
        comdare::cache_engine::abi::BvsetGlied{kFrozenBvset});
}

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
