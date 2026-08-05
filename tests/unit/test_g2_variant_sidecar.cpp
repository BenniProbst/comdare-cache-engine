// G2-3 (Lager-Gate, Stempel-Paket A7) -- Tests fuer das dritte per-Binary-Sidecar `.variant`:
//   (1) compose_variant_signature: deterministisch + exakte Form (bv aus kBuildVariantDefinitionVersion, A6=2);
//   (2) parse_variant_signature: Roundtrip + strenge Ablehnung malformierter Formen;
//   (3) [ENTFALLEN 2026-08-05, A2-Eichung] dll_is_current 3x3-Skip-Matrix: variant_sig {leer, gesetzt} x .variant
//       {fehlt, mismatch, match};
//   (4) [ENTFALLEN 2026-08-05, A2-Eichung] Byte-Neutralitaet: variant_sig leer => Variant-Gate AUS;
//   (5) write_variant_sidecar: leer = no-op, sonst byte-exaktes Schreiben.
//
// G2 Folge-B (A7-B) ERGAENZT, ab Abschnitt (6): die MENGEN-Signatur ueber die Enabled-Typlisten
// (build_variant_set_signature.hpp / driver_build_variant_signature.hpp):
//   (6) CT-Wachen: exakte Form ueber synthetischen Listen, je-Achse-Klammern, Determinismus, Reihenfolge-Sensitivitaet;
//   (7) Gate-Wirkung: andere Enable-Menge => andere Signatur (das Cross-Maschinen-Gate);
//   (8) Format-Disjunktheit zur A7-Einzel-POD-Form (parse_variant_signature lehnt die Mengen-Form ab);
//   (9) [ENTFALLEN 2026-08-05, A2-Eichung] 3x3-Skip-Matrix ERNEUT mit der REALEN Treiber-Mengen-Signatur;
//   (10) EINE Feldquelle: die *_axis_fields-Leser und der POD build_variant_definition driften nicht auseinander.
//
// A2-EICHUNG (GATE 5, F7 KONSOLIDIERT:72, 2026-08-05): dll_is_current fuehrt seither GENAU EINEN Vergleich
// (erwarteter CT-Fingerprint == `.fingerprint`-Sidecar, fail-closed ohne Anker). `.variant` bleibt als
// Provenienz-Legende erhalten -- es wird komponiert, geschrieben und transportiert wie bisher --, entscheidet
// aber ueber KEINEN Skip mehr. Deshalb sind (3)/(4)/(4b)/(9) hier entfallen: sie prueften ein Gate, das es
// nicht mehr gibt. Der Skip-Entscheid wird jetzt in tests/unit/test_a2_sha512_skip_gate.cpp bewiesen; diese
// TU bleibt die volle Wahrheit ueber den .variant-SERIALIZER und die A7-B-Mengen-Signatur.

#include "anatomy/build_variant_definition.hpp" // BuildVariantDefinitionV1 + kBuildVariantDefinitionVersion
#include "builder/build_orchestrator/build_orchestrator.hpp" // variant_sidecar_path / dll_is_current / write_*_sidecar
#include "builder/build_variant_set_signature.hpp"           // A7-B: variant_set_signature<PageList,SimdList,HwList>
#include "builder/build_variant_sidecar.hpp"                 // compose_variant_signature / parse_variant_signature
#include "builder/driver_build_variant_signature.hpp"        // A7-B: kDriverBuildVariantSignature (reale Registries)
#include "comdare_test_tmp.hpp"                              // #278/#24: per-User-Temp gegen CI-Kollisionen

#include <boost/mp11.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

namespace ex = comdare::cache_engine::builder::experiment;
namespace an = comdare::cache_engine::anatomy;
namespace fs = std::filesystem;
namespace mp = boost::mp11;

namespace {

// A2-EICHUNG (2026-08-05): `write_file` ist mit den entfallenen Skip-Matrix-Abschnitten (3)/(4)/(4b)/(9)
// weggefallen -- nur sie legten `.variant`-Sidecars von Hand an. Der Writer-Abschnitt (5) benutzt
// ex::write_variant_sidecar (die Produktiv-Naht), der Leser bleibt. Toter Code wird entfernt, nicht geparkt.
std::string read_file(fs::path const& p) {
    std::ifstream f{p, std::ios::binary};
    return std::string{std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>{}};
}

// Deterministische Beispiel-Definition (BPlus-Knoten, AVX-512 + konvergiertes AVX10.2, NUMA-faehig).
an::BuildVariantDefinitionV1 sample_def_avx10() {
    an::BuildVariantDefinitionV1 v;
    v.page_kind          = 5; // BPlus
    v.page_is_branch     = 1;
    v.page_is_leaf       = 0;
    v.simd_width_bits    = 512;
    v.simd_avx512        = 1;
    v.hw_cache_line      = 64;
    v.hw_numa_capable    = 1;
    v.present_mask       = 7;
    v.simd_avx10_version = 2; // A6-Feld (G2-2)
    return v;
}

// A2-EICHUNG (2026-08-05): `sample_def_legacy` (DenseByte, 128-bit, kein AVX512/AVX10) existierte
// AUSSCHLIESSLICH als "andere Zelle" fuer die Mismatch-Zeilen der entfallenen Skip-Matrix -- ihre
// Serializer-Form wurde nie behauptet. Mit der Matrix faellt sie ersatzlos (toter Code wird entfernt).

// -- A7-B: synthetische Achsen-Wrapper. Sie erfuellen exakt die Property-API, die anatomy::*_axis_fields liest
//    (dieselbe tolerante Erkennung wie die realen Registry-Wrapper), tragen aber FESTE Werte -> die erwartete
//    Signatur ist als Literal hinschreibbar und damit eine echte Byte-Wache statt einer Selbst-Bestaetigung.

struct StubPageA {
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "stub_page_a"; }
    [[nodiscard]] static constexpr std::uint64_t    page_kind() noexcept { return 0; }
    [[nodiscard]] static constexpr bool             is_branch() noexcept { return false; }
    [[nodiscard]] static constexpr bool             is_leaf() noexcept { return true; }
};

struct StubPageB {
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "stub_page_b"; }
    [[nodiscard]] static constexpr std::uint64_t    page_kind() noexcept { return 5; }
    [[nodiscard]] static constexpr bool             is_branch() noexcept { return true; }
    [[nodiscard]] static constexpr bool             is_leaf() noexcept { return false; }
};

struct StubSimdA { // 256 bit, kein AVX-512, kein AVX10 (grobe Stub-API provides_avx512)
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "stub_simd_a"; }
    [[nodiscard]] static constexpr int              vector_width_bits() noexcept { return 256; }
    [[nodiscard]] static constexpr bool             provides_avx512() noexcept { return false; }
};

struct StubSimdB { // 512 bit, AVX-512, konvergiertes AVX10.2 (praezise API provides_avx10_version)
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "stub_simd_b"; }
    [[nodiscard]] static constexpr int              vector_width_bits() noexcept { return 512; }
    [[nodiscard]] static constexpr bool             provides_avx512() noexcept { return true; }
    [[nodiscard]] static constexpr int              provides_avx10_version() noexcept { return 2; }
};

struct StubHwA {
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "stub_hw_a"; }
    [[nodiscard]] static constexpr std::uint64_t    cache_line_size() noexcept { return 64; }
    [[nodiscard]] static constexpr bool             numa_capable() noexcept { return false; }
};

struct StubHwB {
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "stub_hw_b"; }
    [[nodiscard]] static constexpr std::uint64_t    cache_line_size() noexcept { return 128; }
    [[nodiscard]] static constexpr bool             numa_capable() noexcept { return true; }
};

using PagesFull       = mp::mp_list<StubPageA, StubPageB>;
using PagesRestricted = mp::mp_list<StubPageA>;            // "per-Maschine restringierter Build"
using PagesSwapped    = mp::mp_list<StubPageB, StubPageA>; // gleiche MENGE, andere Reihenfolge
using PagesEmpty      = mp::mp_list<>;
using SimdsFull       = mp::mp_list<StubSimdA, StubSimdB>;
using SimdsRestricted = mp::mp_list<StubSimdB>;
using HwFull          = mp::mp_list<StubHwA, StubHwB>;
using HwRestricted    = mp::mp_list<StubHwA>;

inline constexpr std::string_view kSetFull = ex::variant_set_signature<PagesFull, SimdsFull, HwFull>();

// (6a) EXAKTE FORM als Literal: Kopf (bvset + POD-Version bv), drei separat geklammerte Achsen-Gruppen in
// Listen-Reihenfolge, je Element `{name;felder}`. Jede Aenderung an Trennzeichen, Feldauswahl oder Achsen-
// Reihenfolge bricht hier zur COMPILE-Zeit -- das ist die eigentliche Byte-Wache der Mengen-Signatur.
static_assert(kSetFull == "bvset=1;bv=2;"
                          "page_type[{stub_page_a;page_kind=0}{stub_page_b;page_kind=5}];"
                          "simd_extension[{stub_simd_a;simd_width_bits=256;simd_avx512=0;simd_avx10_version=0}"
                          "{stub_simd_b;simd_width_bits=512;simd_avx512=1;simd_avx10_version=2}];"
                          "general_hardware[{stub_hw_a;hw_cache_line=64;hw_numa_capable=0}"
                          "{stub_hw_b;hw_cache_line=128;hw_numa_capable=1}]");

// (6b) Struktur-Wachen: nicht leer, Format-Kopf, ALLE DREI Achsen-Klammern vorhanden (§66-N3 Punkt 4).
static_assert(!kSetFull.empty());
static_assert(kSetFull.starts_with("bvset="));
static_assert(kSetFull.find(";page_type[") != std::string_view::npos);
static_assert(kSetFull.find(";simd_extension[") != std::string_view::npos);
static_assert(kSetFull.find(";general_hardware[") != std::string_view::npos);

// (6c) Determinismus: dieselben drei Listen liefern denselben Inhalt -- und zwar auch dann, wenn die Signatur ueber
// einen anderen, typgleichen Alias angefordert wird (kein Instanziierungs-Zufall).
static_assert(ex::variant_set_signature<mp::mp_list<StubPageA, StubPageB>, SimdsFull, HwFull>() == kSetFull);

// (6d) Leere Achse bleibt als Klammer SICHTBAR (verschwindet nicht spurlos) -- Voraussetzung dafuer, dass eine
// weggeschaltete Achse ein Signatur-Bruch ist und nicht stillschweigend zur Nachbar-Achse verschmilzt.
static_assert(ex::variant_set_signature<PagesEmpty, SimdsFull, HwFull>().find("page_type[]") != std::string_view::npos);

// (7) GATE-WIRKUNG (CT): jede andere Enable-Menge -- und jede andere Reihenfolge -- ergibt eine andere Signatur.
static_assert(ex::variant_set_signature<PagesRestricted, SimdsFull, HwFull>() != kSetFull);
static_assert(ex::variant_set_signature<PagesFull, SimdsRestricted, HwFull>() != kSetFull);
static_assert(ex::variant_set_signature<PagesFull, SimdsFull, HwRestricted>() != kSetFull);
static_assert(ex::variant_set_signature<PagesSwapped, SimdsFull, HwFull>() != kSetFull);

// Die reale Treiber-Signatur, um 1 synthetischen Seitentyp erweitert = "ein anders konfigurierter Treiber".
// Additiv statt restringierend, damit der Beweis unabhaengig davon traegt, wie viele Varianten die CMake-Flags
// dieser Maschine gerade enablen (mp_take_c<...,1> waere bei einer 1-elementigen Enabled-Liste identisch gewesen).
using WidenedDriverPages =
    mp::mp_push_front<comdare::cache_engine::nodes::axis_01_page_type::EnabledPageTypes, StubPageA>;
inline constexpr std::string_view kSetDriverWidened =
    ex::variant_set_signature<WidenedDriverPages,
                              comdare::cache_engine::hardware::axis_09b_simd_extension::EnabledExtensions,
                              comdare::cache_engine::hardware::axis_12_general_hardware::EnabledPlatforms>();
static_assert(kSetDriverWidened != ex::kDriverBuildVariantSignature);

} // namespace

// (1) Serializer: exakte, feste Form + Determinismus. bv spiegelt die POD-Version (A6 => 2).
TEST(G2VariantSidecar, ComposeIsDeterministicAndExact) {
    auto const        v      = sample_def_avx10();
    std::string const sig    = ex::compose_variant_signature(v);
    std::string const expect = "bv=2;page_kind=5;simd_width_bits=512;simd_avx512=1;simd_avx10_version=2;"
                               "hw_cache_line=64;hw_numa_capable=1;present_mask=7";
    EXPECT_EQ(sig, expect);
    EXPECT_EQ(ex::compose_variant_signature(v), sig); // zweite Komposition identisch
    EXPECT_EQ(static_cast<std::uint64_t>(an::kBuildVariantDefinitionVersion), 2u);
}

// (2a) Parser: Roundtrip compose -> parse rekonstruiert alle 7 Felder + bv.
TEST(G2VariantSidecar, ParseRoundtripsCompose) {
    auto const v      = sample_def_avx10();
    auto const parsed = ex::parse_variant_signature(ex::compose_variant_signature(v));
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->bv, static_cast<std::uint64_t>(an::kBuildVariantDefinitionVersion));
    EXPECT_EQ(parsed->page_kind, v.page_kind);
    EXPECT_EQ(parsed->simd_width_bits, v.simd_width_bits);
    EXPECT_EQ(parsed->simd_avx512, v.simd_avx512);
    EXPECT_EQ(parsed->simd_avx10_version, v.simd_avx10_version);
    EXPECT_EQ(parsed->hw_cache_line, v.hw_cache_line);
    EXPECT_EQ(parsed->hw_numa_capable, v.hw_numa_capable);
    EXPECT_EQ(parsed->present_mask, v.present_mask);
}

// (2b) Parser: strenge Ablehnung malformierter Formen (kein Rateverhalten).
TEST(G2VariantSidecar, ParseRejectsMalformed) {
    EXPECT_FALSE(ex::parse_variant_signature("").has_value());                  // leer
    EXPECT_FALSE(ex::parse_variant_signature("bv=2;page_kind").has_value());    // Token ohne '='
    EXPECT_FALSE(ex::parse_variant_signature("bv=2;page_kind=x").has_value());  // nicht numerisch
    EXPECT_FALSE(ex::parse_variant_signature("bv=2;unknown=1").has_value());    // unbekannter Schluessel
    EXPECT_FALSE(ex::parse_variant_signature("bv=2;page_kind=5;").has_value()); // leeres Trailing-Token
}

// (3)+(4)+(4b) ENTFALLEN mit der A2-EICHUNG (GATE 5, F7, 2026-08-05) -- HISTORIK, nicht Verlust:
// Diese drei Abschnitte prueften die 3x3-Skip-Matrix (variant_sig {leer,gesetzt} x .variant {fehlt,mismatch,
// match}) und die Byte-Neutralitaet der 3-arg-Form von dll_is_current. Beides setzte voraus, dass .variant
// ein SKIP-Kriterium ist und dll_is_current mehr als zwei Parameter hat. Seit der Eichung fuehrt
// dll_is_current GENAU EINEN Vergleich (erwarteter CT-Fingerprint == `.fingerprint`-Sidecar, F7 "NUR");
// .variant wird weiter geschrieben und transportiert, entscheidet aber ueber keinen Skip mehr -- die
// Matrix hatte damit keinen Gegenstand mehr. Der Skip-Entscheid wird seither vollstaendig in
// tests/unit/test_a2_sha512_skip_gate.cpp bewiesen (inkl. der Gegenprobe, dass ein gefuelltes .variant
// KEIN Skip mehr herbeifuehrt). Was diese TU beweist, ist unveraendert der .variant-SERIALIZER: (1)(2)(2b)
// compose/parse, (5) Writer, (6)-(10) die A7-B-Mengen-Signatur.

// (5) Writer: leer = no-op (kein Sidecar), sonst byte-exaktes Schreiben.
TEST(G2VariantSidecar, WriteVariantSidecarEmptyIsNoOpElseExact) {
    fs::path const  base = comdare::test::user_tmp_dir() / "g2_variant_writer";
    std::error_code ec;
    fs::remove_all(base, ec);
    fs::create_directories(base, ec);

    fs::path const out = base / "perm_y.dll";

    ex::write_variant_sidecar(out, ""); // leer -> no-op
    EXPECT_FALSE(fs::exists(ex::variant_sidecar_path(out)));

    std::string const sig = ex::compose_variant_signature(sample_def_avx10());
    ex::write_variant_sidecar(out, sig); // schreibt exakt
    ASSERT_TRUE(fs::exists(ex::variant_sidecar_path(out)));
    EXPECT_EQ(read_file(ex::variant_sidecar_path(out)), sig);

    fs::remove_all(base, ec);
}

// ============================================================================================================
// G2 Folge-B (A7-B) -- MENGEN-Signatur ueber die Enabled-Typlisten
// ============================================================================================================

// (6e) Element-Zahl = Listen-Laenge je Achse: die Klammern enthalten GENAU so viele `{`-Elemente wie die Listen
// Eintraege haben (kein Element wird verschluckt, keines doppelt emittiert).
TEST(G2VariantSetSignature, EmitsExactlyOneElementPerEnabledType) {
    auto count_char = [](std::string_view s, char c) {
        std::size_t n = 0;
        for (char const x : s) {
            if (x == c) ++n;
        }
        return n;
    };
    EXPECT_EQ(count_char(kSetFull, '{'), 6u); // 2 page + 2 simd + 2 hw
    EXPECT_EQ(count_char(kSetFull, '}'), 6u);
    EXPECT_EQ(count_char(kSetFull, '['), 3u); // genau 3 Achsen-Klammern
    EXPECT_EQ(count_char(kSetFull, ']'), 3u);

    // Auch fuer die realen Registries: je Achse genau so viele Elemente wie Enabled-Eintraege.
    namespace a01                     = comdare::cache_engine::nodes::axis_01_page_type;
    namespace a09b                    = comdare::cache_engine::hardware::axis_09b_simd_extension;
    namespace a12                     = comdare::cache_engine::hardware::axis_12_general_hardware;
    std::size_t const expected_driver = mp::mp_size<a01::EnabledPageTypes>::value +
                                        mp::mp_size<a09b::EnabledExtensions>::value +
                                        mp::mp_size<a12::EnabledPlatforms>::value;
    EXPECT_EQ(count_char(ex::kDriverBuildVariantSignature, '{'), expected_driver);
    EXPECT_EQ(count_char(ex::kDriverBuildVariantSignature, '['), 3u);
}

// (7b) Das Cross-Maschinen-Gate zur Laufzeit nachvollzogen: zwei unterschiedlich konfigurierte Treiber liefern
// unterschiedliche Signaturen; derselbe Treiber immer dieselbe. Die Runtime-Naht (std::string fuer BuildConfig)
// materialisiert genau die CT-Konstante -- sie leitet nichts ab.
TEST(G2VariantSetSignature, DriverSignatureIsStableAndConfigSensitive) {
    EXPECT_FALSE(ex::kDriverBuildVariantSignature.empty());
    EXPECT_EQ(ex::driver_build_variant_signature(), std::string{ex::kDriverBuildVariantSignature});
    EXPECT_EQ(ex::driver_build_variant_signature(), ex::driver_build_variant_signature());
    EXPECT_NE(std::string{kSetDriverWidened}, ex::driver_build_variant_signature());
}

// (8) Format-Disjunktheit zur A7-Einzel-POD-Form: der A7-Parser MUSS die Mengen-Form ablehnen (und umgekehrt darf
// die Mengen-Form nie wie eine POD-Signatur aussehen). Sonst koennte ein `.variant`-Sidecar der einen Sorte still
// gegen die andere verglichen bzw. fehl-geparst werden.
TEST(G2VariantSetSignature, SetFormIsDisjointFromA7PodForm) {
    EXPECT_FALSE(ex::parse_variant_signature(ex::kDriverBuildVariantSignature).has_value());
    EXPECT_FALSE(ex::parse_variant_signature(kSetFull).has_value());

    std::string const pod_sig = ex::compose_variant_signature(sample_def_avx10());
    EXPECT_TRUE(ex::parse_variant_signature(pod_sig).has_value()); // A7-Form bleibt lesbar
    EXPECT_NE(pod_sig, std::string{kSetFull});
    EXPECT_FALSE(std::string_view{pod_sig}.starts_with("bvset="));
}

// (9) ENTFAELLT mit der A2-EICHUNG (GATE 5, F7, 2026-08-05) -- HISTORIK: dieser Abschnitt fuhr die 3x3-Skip-
// Matrix ein zweites Mal, diesmal mit der REALEN Treiber-Mengen-Signatur in variant_sig. Sein Gegenstand
// (das aktivierte .variant-Gate) existiert nicht mehr: dll_is_current vergleicht seither NUR den
// `.fingerprint`-Anker. Die beiden Aussagen des Abschnitts, die KEIN Skip-Gate brauchen, sind bereits
// anderswo bewiesen und bleiben damit lueckenlos gedeckt: "verschieden konfigurierte Treiber => verschiedene
// Signatur" in (7b) DriverSignatureIsStableAndConfigSensitive, "das Sidecar wird byte-exakt geschrieben" in
// (5) WriteVariantSidecarEmptyIsNoOpElseExact. Der Skip-Entscheid selbst: test_a2_sha512_skip_gate.cpp.

// (10) EINE FELDQUELLE: die je-Achse-Leser, aus denen die Mengen-Signatur serialisiert, und der volle POD
// build_variant_definition<PT,SE,HW>() liefern dieselben Werte. Driften sie auseinander, wuerde ein `.variant`
// (A7, POD-Form) andere Zahlen tragen als die Mengen-Form -- der Beweis, dass das nicht passieren kann.
TEST(G2VariantSetSignature, AxisFieldReadersAndPodAgree) {
    constexpr an::BuildVariantDefinitionV1 pod = an::build_variant_definition<StubPageB, StubSimdB, StubHwB>();
    constexpr an::PageAxisFields           p   = an::page_axis_fields<StubPageB>();
    constexpr an::SimdAxisFields           s   = an::simd_axis_fields<StubSimdB>();
    constexpr an::HwAxisFields             h   = an::hw_axis_fields<StubHwB>();

    static_assert(pod.page_kind == p.page_kind);
    static_assert(pod.page_is_branch == p.page_is_branch);
    static_assert(pod.page_is_leaf == p.page_is_leaf);
    static_assert(pod.simd_width_bits == s.simd_width_bits);
    static_assert(pod.simd_avx512 == s.simd_avx512);
    static_assert(pod.simd_avx10_version == s.simd_avx10_version);
    static_assert(pod.hw_cache_line == h.hw_cache_line);
    static_assert(pod.hw_numa_capable == h.hw_numa_capable);
    static_assert(pod.present_mask == (an::kBuildPagePresent | an::kBuildSimdPresent | an::kBuildHwPresent));

    // Und die Werte sind die erwarteten (nicht etwa beidseitig gleich falsch).
    EXPECT_EQ(pod.page_kind, 5u);
    EXPECT_EQ(pod.simd_width_bits, 512u);
    EXPECT_EQ(pod.simd_avx512, 1u);
    EXPECT_EQ(pod.simd_avx10_version, 2u);
    EXPECT_EQ(pod.hw_cache_line, 128u);
    EXPECT_EQ(pod.hw_numa_capable, 1u);
}
