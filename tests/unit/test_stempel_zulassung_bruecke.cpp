// test_stempel_zulassung_bruecke.cpp -- S-7 / Paket P2 (13.08.2026): die ZULASSUNGS-BRUECKE des
// Achsen-Algo-Hardware-Stempels END-TO-END am echten Call-Site (BuildOrchestrator, provision_core)
// PLUS Semantik an SYNTHETISCHEN Probe-Klassen mit voller System-Achsen-Syntax.
//
// WARUM PROBE-KLASSEN UND KEINE PRODUKTIONS-DEKLARATION: der C-3a-Tripwire
// (simd_build_gate.hpp:272-278) erzwingt organ_required_union_size()==0 compile-hart; die erste
// echte required-Deklaration ist ein eigener Owner-Paket-Entscheid (C-3a). Die fordernden Formen
// leben deshalb NUR hier im Test -- kSimdOrganRequirement bleibt unberuehrt, der Tripwire steht.
//
// KOEDER-FORM (T-11c, VOR jeder Mutation geprueft): der T-4-Gegeneingang (Probe fordert x512{f}
// gegen die LEERE Signatur) MUSS OHNE Mutation abgelehnt sein (r.status==-4,
// "simd-gate(stempel): hardware_erweiterung_fehlt") -- erst dann zaehlt ein Biss.
//
// NAMENSKONVENTION test_stempel_* (KON58-10: NIE test_s7_* -- 10 Bestandsdateien
// test_s7_1..test_s7_10_*_pool_allocator_deg.cpp kollidieren im ctest -R-Selektor).
// ASCII-only (Leitplanke). Zeilen <= 120 (Diff-Hygiene-Wache).

#include <builder/build_orchestrator/build_orchestrator.hpp>      // BuildOrchestrator + AlgoVersionFn (S-7-Naht)
#include <builder/experiment_tree/axis_variant_version_table.hpp> // assert_version_grammar + Enabled-Tabelle
#include <builder/experiment_tree/experiment_tree.hpp>            // ExperimentTree/StaticBinaryView (Call-Site)
#include <cache_engine/measurement/algo_semver.hpp>
#include <cache_engine/measurement/algo_stempel_zulassung.hpp>
#include <cache_engine/measurement/flag_menge_ordnung.hpp>
#include <cache_engine/measurement/machine_identity.hpp> // live_hostname (host-gegatete Positiv-Zusage)
#include <cache_engine/measurement/machine_simd_signature.hpp>
#include <cache_engine/measurement/simd_build_gate.hpp> // set/reset_active_machine_declaration
#include "comdare_test_tmp.hpp"                         // per-User-Temp gegen CI-Kollisionen

#include <gtest/gtest.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant> // BuildError-Alternativen-Pruefung (axis_error.hpp:411)
#include <vector>

namespace ex   = ::comdare::cache_engine::builder::experiment;
namespace meas = ::comdare::cache_engine::measurement;
namespace fs   = ::std::filesystem;

namespace {

// =================================================================================================
// SYNTHETISCHE PROBE-KLASSEN: volle System-Achsen-Syntax, NUR im Test (C-3a-Tripwire unberuehrt)
// =================================================================================================
// Registry-Tauglichkeit der Vollsyntax: assert_version_grammar<Probe>() ist DIESELBE Wache, die
// jede registrierte Organ-Variante besteht (axis_variant_version_table.hpp:66-113) -- der Beweis,
// dass eine kuenftige Variante mit voller Syntax OHNE weiteren Umbau registrierbar waere.
struct ProbeOwnerBeispiel { // das Owner-Beispiel woertlich (algo_semver.hpp:1265)
    static constexpr std::string_view algo_version{"1.0.0.c{p.e}.x512{f.vl.bw.dq}.gfni"};
};
struct ProbeX512FVl {
    static constexpr std::string_view algo_version{"1.0.0.c.x512{f.vl}"};
};
struct ProbeX256Vnni {
    static constexpr std::string_view algo_version{"1.0.0.c.x256{vnni}"};
};

[[maybe_unused]] void probe_grammatik_ist_registry_tauglich() {
    // M3-ANKER: ein Sentinel-Literal an EINER dieser Proben ("1.0" statt "1.0.0.c...") bricht den
    // BAU dieser TU compile-time MIT dem Typ-Namen -- exakt die Registry-Wache.
    ex::assert_version_grammar<ProbeOwnerBeispiel>();
    ex::assert_version_grammar<ProbeX512FVl>();
    ex::assert_version_grammar<ProbeX256Vnni>();
}

// Ordnung ZWISCHEN den Proben (S-3a-Relation auf der Vollsyntax): x512{f.vl} ist Teilmenge des
// Owner-Beispiels (Erweiterung), die Umkehr nicht; der vnni-Doppelgaenger liegt quer zu beiden.
static_assert(meas::flag_menge_ist_teilmenge(meas::parse_algo_semver(ProbeX512FVl::algo_version),
                                             meas::parse_algo_semver(ProbeOwnerBeispiel::algo_version)));
static_assert(!meas::flag_menge_ist_teilmenge(meas::parse_algo_semver(ProbeOwnerBeispiel::algo_version),
                                              meas::parse_algo_semver(ProbeX512FVl::algo_version)));
static_assert(!meas::flag_menge_ist_teilmenge(meas::parse_algo_semver(ProbeX256Vnni::algo_version),
                                              meas::parse_algo_semver(ProbeOwnerBeispiel::algo_version)));

// =================================================================================================
// Werkzeug: EIN-Binary-Baum + Orchestrator-Lauf mit injiziertem Provider (Muster test_a2 (f))
// =================================================================================================
fs::path fresh_dir(char const* name) {
    std::error_code ec;
    fs::path const  d = ::comdare::test::user_tmp_dir() / "comdare_s7_stempel_bruecke" / name;
    fs::remove_all(d, ec);
    fs::create_directories(d, ec);
    return d;
}

struct LaufErgebnis {
    ex::BuildStats               stats;
    std::vector<ex::BuildResult> results;
};

// Baut GENAU EINE Binary (Achse "memory_layout", Wert "probe_soa") mit dem gegebenen Provider.
[[nodiscard]] LaufErgebnis ein_binary_lauf(char const* dir, ex::AlgoVersionFn provider) {
    auto               factory = std::make_shared<ex::ExperimentNodeFactory>();
    fs::path const     base    = fresh_dir(dir);
    ex::ExperimentTree tree{factory};
    tree.build({ex::AxisLevel{"memory_layout", {"probe_soa"}, true, "", ""}});
    auto const view = tree.static_binary_view();
    EXPECT_EQ(view.size(), std::size_t{1});
    auto compile_touch = [](ex::BuildJob const& j) -> int {
        std::ofstream f{j.output, std::ios::binary | std::ios::trunc};
        f << "DLL";
        return 0;
    };
    auto                  gen = [](std::string const&) { return std::string{"//\n"}; };
    ex::BuildConfig       cfg{4, 8, base / "src", base / "dll"};
    ex::BuildOrchestrator orch{cfg, compile_touch, gen};
    if (provider) orch.set_algo_version_provider(std::move(provider));
    LaufErgebnis e;
    e.results = orch.provision_all(view, &e.stats);
    return e;
}

// Der fordernde Provider: die Probe-Achse verlangt x512{f} (avx512f), alles andere kennt er nicht.
[[nodiscard]] ex::AlgoVersionFn fordernder_provider() {
    return [](std::string const& achse, std::string const& wert) -> std::string {
        if (achse == "memory_layout" && wert == "probe_soa") return "1.0.0.c.x512{f}";
        return {};
    };
}

} // namespace

// =================================================================================================
// 1. T-4-GEGENEINGANG (Koeder-Form): LEERE Signatur => Ablehnung -4, hardware_erweiterung_fehlt
// =================================================================================================
TEST(StempelZulassungBruecke, GegeneingangLeereSignaturLehntAb) {
    meas::reset_active_machine_declaration_for_test();
    ASSERT_TRUE(meas::active_machine_signature().empty());
    auto const e = ein_binary_lauf("t4_leer", fordernder_provider());
    ASSERT_EQ(e.results.size(), std::size_t{1});
    EXPECT_EQ(e.stats.failed, std::size_t{1});
    EXPECT_EQ(e.stats.built, std::size_t{0});
    EXPECT_EQ(e.results[0].status, -4);
    EXPECT_EQ(e.results[0].message, std::string{"simd-gate(stempel): hardware_erweiterung_fehlt"});
    ASSERT_FALSE(e.results[0].outcome.has_value());
    // BuildError ist ein variant<InfraErrorClass, CompilerCompilerErrorClass> (axis_error.hpp:411).
    ASSERT_TRUE(std::holds_alternative<meas::CompilerCompilerErrorClass>(e.results[0].outcome.error()));
    EXPECT_EQ(std::get<meas::CompilerCompilerErrorClass>(e.results[0].outcome.error()),
              meas::CompilerCompilerErrorClass::HardwareErweiterungFehlt);
}

// =================================================================================================
// 2. FREIGABE-SEITE IMPLIZIERT: prod1-Belegung laesst x512{f} zu (host-gegatet wie test_c3a:78)
// =================================================================================================
TEST(StempelZulassungBruecke, Prod1BelegungLaesstX512FZu) {
    meas::reset_active_machine_declaration_for_test();
    ASSERT_EQ(meas::set_active_machine_declaration("amd_zen5_avx512", "ddr5_2x32"),
              meas::MachineDeclarationSetResult::Gesetzt);
    auto const e = ein_binary_lauf("prod1_zu", fordernder_provider());
    ASSERT_EQ(e.results.size(), std::size_t{1});
    if (meas::live_hostname() == "prod1") {
        // Auf prod1 loest die Identitaet WIRKLICH auf (Verdict Match) -> die Signatur deckt avx512f.
        ASSERT_EQ(meas::active_machine_verdict(), meas::MachineIdentityVerdict::Match);
        EXPECT_EQ(e.stats.built, std::size_t{1}) << "prod1 gibt avx512f frei -- die Probe MUSS bauen.";
        EXPECT_EQ(e.results[0].status, 0);
    } else {
        // Fremder Host: die prod1-Deklaration matcht die live-CPU nicht -> Signatur bleibt LEER ->
        // fail-closed Ablehnung (kein Zweig nimmt still eine fremde Identitaet an).
        EXPECT_EQ(e.stats.failed, std::size_t{1});
        EXPECT_EQ(e.results[0].status, -4);
    }
    meas::reset_active_machine_declaration_for_test();
}

// =================================================================================================
// 3. prod2-Belegung: ABGELEHNT -- auf prod1 wegen Abweichung (leere Signatur), auf einem echten
//    prod2 wegen fused-off AVX-512 (die Signatur fuehrt avx512f nicht). BEIDE Wege enden in -4.
// =================================================================================================
TEST(StempelZulassungBruecke, Prod2BelegungLehntX512FAb) {
    meas::reset_active_machine_declaration_for_test();
    ASSERT_EQ(meas::set_active_machine_declaration("intel_avx2", "ddr5_2x32"),
              meas::MachineDeclarationSetResult::Gesetzt);
    auto const e = ein_binary_lauf("prod2_ab", fordernder_provider());
    ASSERT_EQ(e.results.size(), std::size_t{1});
    EXPECT_EQ(e.stats.failed, std::size_t{1});
    EXPECT_EQ(e.results[0].status, -4);
    EXPECT_EQ(e.results[0].message, std::string{"simd-gate(stempel): hardware_erweiterung_fehlt"});
    meas::reset_active_machine_declaration_for_test();
}

// =================================================================================================
// 4. PROVIDER-KONTRAKT: leer = neutral (kein Versions-Traeger), Sentinel = fail-closed, AUS = AUS
// =================================================================================================
TEST(StempelZulassungBruecke, LeeresLiteralIstNeutralerNichtTraeger) {
    meas::reset_active_machine_declaration_for_test();
    // "Kein bekannter Versions-Stand" ist NEUTRAL: Sub-Achsen-Ebenen ("cacheline.line_size" u.a.,
    // profile_to_tree.hpp:104-125) tragen KEINE algo_version, und profil-eigene Werte ausserhalb
    // der Enabled-Registry (gemessener Bestandsfall planner_thesis_min: search_algo=bplus bei
    // Tabelle {k_ary, interpolation, eytzinger, linear_scan}) stempeln im .algos-Pfad nur den
    // Provenienz-Sentinel und BAUEN trotzdem -- der Provider antwortet leer, das Gate ueberspringt
    // NEUTRAL (exakt die compose_algo_signature-/required_of-Behandlung). Trotz LEERER Signatur
    // wird gebaut. Eine Version, die niemand deklariert hat, fordert nichts.
    auto const e = ein_binary_lauf("neutral", [](std::string const&, std::string const&) { return std::string{}; });
    EXPECT_EQ(e.stats.built, std::size_t{1});
    EXPECT_EQ(e.results[0].status, 0);
}

TEST(StempelZulassungBruecke, SentinelLiteralFaelltFailClosed) {
    meas::reset_active_machine_declaration_for_test();
    // Tabellen-Drift (Slot-Achse mit unbekannter Variante) kommt per Kontrakt als Sentinel an --
    // parst auf das Null-Tripel und faellt fail-closed, egal welche Signatur belegt ist.
    auto const e = ein_binary_lauf("sentinel", [](std::string const&, std::string const&) {
        return std::string{meas::kAlgoSemVerSentinelLiteral};
    });
    EXPECT_EQ(e.stats.failed, std::size_t{1});
    EXPECT_EQ(e.results[0].status, -4);
}

TEST(StempelZulassungBruecke, OhneProviderIstDieBrueckeAus) {
    meas::reset_active_machine_declaration_for_test();
    // NICHT GESETZT = Bruecke AUS (byte-neutral): trotz leerer Signatur wird gebaut -- exakt das
    // Verhalten von vorher (opt-in-Muster set_bvset_fingerprint_provider).
    auto const e = ein_binary_lauf("aus", ex::AlgoVersionFn{});
    EXPECT_EQ(e.stats.built, std::size_t{1});
    EXPECT_EQ(e.results[0].status, 0);
}

// =================================================================================================
// 5. vnni-DOPPELGAENGER gegen prod2: Tabelle, nie Heuristik (Urteil + Sicht, M2-Anker)
// =================================================================================================
TEST(StempelZulassungBruecke, VnniDoppelgaengerLaeuftUeberDieTabelle) {
    // URTEIL: x256{vnni} -> avx_vnni (prod2 traegt es) => zugelassen; x512{vnni} -> avx512_vnni
    // (prod2 traegt es NICHT) => HardwareErweiterungFehlt. Eine String-Heuristik ("vnni" ==
    // "vnni") koennte die beiden nicht unterscheiden -- nur das (token, eltern)-Paar der Tabelle.
    EXPECT_FALSE(meas::stempel_zulassung_je_achse(meas::parse_algo_semver("1.0.0.c.x256{vnni}"),
                                                  meas::Prod2AlderLakeSignature::signature())
                     .has_value());
    auto const urteil512 = meas::stempel_zulassung_je_achse(meas::parse_algo_semver("1.0.0.c.x512{vnni}"),
                                                            meas::Prod2AlderLakeSignature::signature());
    ASSERT_TRUE(urteil512.has_value());
    EXPECT_EQ(*urteil512, meas::CompilerCompilerErrorClass::HardwareErweiterungFehlt);

    // SICHT (M2-Anker): dieselbe Token-Schreibweise, ZWEI cpuinfo-Ids -- eine Mutation, die das
    // eltern-Token ignoriert, uebersetzt eine der beiden Seiten falsch und reisst HIER.
    std::array<std::pair<std::string_view, std::string_view>, 1> const s256{{{"probe", "1.0.0.c.x256{vnni}"}}};
    std::array<std::pair<std::string_view, std::string_view>, 1> const s512{{{"probe", "1.0.0.c.x512{vnni}"}}};
    auto const                                                         f256 = meas::algo_declared_flags_for_axes(s256);
    auto const                                                         f512 = meas::algo_declared_flags_for_axes(s512);
    ASSERT_EQ(f256.size(), std::size_t{1});
    ASSERT_EQ(f512.size(), std::size_t{1});
    EXPECT_EQ(f256[0].cpuinfo, std::string_view{"avx_vnni"});
    EXPECT_EQ(f512[0].cpuinfo, std::string_view{"avx512_vnni"});
}

// =================================================================================================
// 6. ECHT-BESTANDS-NULLWIRKUNG: die GANZE Enabled-Tabelle durch das Urteil -- 0 Ablehnungen von N
// =================================================================================================
TEST(StempelZulassungBruecke, EchterBestandNullAblehnungen) {
    std::vector<ex::AxisVariantVersion> const tabelle = ex::build_axis_variant_version_table();
    ASSERT_FALSE(tabelle.empty());
    std::array<std::span<meas::SimdFeatureFlag const>, 3> const welten{std::span<meas::SimdFeatureFlag const>{},
                                                                       meas::Prod1Zen5Signature::signature(),
                                                                       meas::Prod2AlderLakeSignature::signature()};
    std::size_t                                                 pruefungen  = 0;
    std::size_t                                                 ablehnungen = 0;
    for (ex::AxisVariantVersion const& e : tabelle) {
        meas::AlgoSemVer const v = meas::parse_algo_semver(e.version);
        for (auto const& w : welten) {
            ++pruefungen;
            if (meas::stempel_zulassung_je_achse(v, w).has_value()) {
                ++ablehnungen;
                ADD_FAILURE() << e.axis << "=" << e.variant << "@" << e.version << " abgelehnt.";
            }
        }
    }
    EXPECT_EQ(ablehnungen, 0u);
    EXPECT_EQ(pruefungen, 3u * tabelle.size());
    std::cout << "[S-7 Nullwirkung] " << ablehnungen << " Ablehnungen von " << pruefungen
              << " Pruefungen (leer+prod1+prod2 x N=" << tabelle.size() << " Enabled-Eintraege)\n";
}
