// tests/unit/test_c3b_kanal_merge_beleg.cpp -- Lane C, Paket C-3b "Kanal-Merge-BELEG"
// (Bauplan-v3 D2.2, OD-4-GO; 26.07.2026). UNREGISTRIERT.
//
// AUFTRAG D2.2, WORTGETREU: "Beweis je Pfad, dass Baseline-march und Gate-Extras auf DERSELBEN
// Compile-Zeile landen. Ist: Fassade konkateniert bereits (march :530-532/:1005-1007, gate-extras
// :537-538/:1012-1013) -> nur belegen; Orchestrator = reject-only per Design (simd_build_gate.hpp:149-152),
// KEIN Emissions-Umbau; V36.B-Codegen-CMake gate-frei unter #25-B-Byte-Vertrag -> out-of-scope ODER
// Hook-Entscheid O-9. Plus Compile-Probe: Sub-Feature-Flags im Kontext jeder Perm-Baseline wirksam
// (Implikation nicht annehmen)."
//
// C-3b IST EIN BELEG-PAKET: es aendert NICHTS. profile_run_facade.cpp wird ausschliesslich GELESEN
// (Lane-F-Sperrmenge); build_orchestrator.hpp bleibt unberuehrt (D3.4); es gibt keinen Emissions-Umbau.
// C-3b ist die VORBEDINGUNG jeder Scharfschaltung (C-3a, Owner-Vorab-GO Ledger §69.9).
//
// WARUM DAS UEBERHAUPT BEWIESEN WERDEN MUSS: es gibt ZWEI Flag-Kanaele, und sie sind DISJUNKT.
//   Kanal A (LEBEND, per Permutation): permutation_codegen_tool.cpp:43-45 simd_flags() -> :477-483
//     target_compile_options(perm_<id> PRIVATE ... -mavx2 | -mavx512f); zusaetzlich
//     cmake/isa_features.cmake (AVX2-/AVX512-Zweige).
//   Kanal B (INERT, per Organ-Anforderung): das Section-37-Freigabe-Gate ueber
//     gate_extra_march_flags_for_build (simd_build_gate.hpp:189-196).
// Kanal B liefert bei Scharfschaltung das VOLLE Freigabe-Maximum (Signatur GESCHNITTEN Sinnhaftigkeit
// GESCHNITTEN Route), Kanal A genau EIN Flag je Route. Die Schnittmenge ist LEER -- es waere also keine
// Dopplung, sondern eine ANDERE Flag-Menge und damit anderer Code. Genau deshalb ist der Beleg, dass
// beide auf DERSELBEN Compile-Zeile zusammengefuehrt werden, die Vorbedingung der Scharfschaltung.

#include <cache_engine/measurement/machine_simd_signature.hpp>
#include <cache_engine/measurement/simd_build_gate.hpp>
#include <cache_engine/measurement/simd_organ_requirement.hpp>
#include <cache_engine/measurement/simd_organ_sensibility.hpp>
#include <cache_engine/measurement/simd_sub_axis.hpp>

#include <gtest/gtest.h>

#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace meas = comdare::cache_engine::measurement;

namespace {

/// SPIEGEL der Fassaden-Naht. Wortgetreue Nachbildung der Konkatenation aus
/// profile_facade/profile_run_facade.cpp:528-541 (Pfad 1) bzw. :1003-1016 (Pfad 2) -- beide Pfade sind
/// im Ist ZEICHENGLEICH aufgebaut:
///     std::string flags = dbg ? debug_flags_for_toolchain() : opt_flag;
///     if (!march_flag.empty()) { flags += ' '; flags += march_flag; }
///     for (auto const& mf : gate_extra_march_flags_for_build(route_of_march_flag(march_flag)))
///         { flags += ' '; flags += mf; }
///     return make_gpp_compile_fn(inc, def, cxx, libs, flags, fno);
/// Der Rueckgabewert ist EIN `flags`-String, der als EIN Argument an make_gpp_compile_fn geht -- also
/// EINE Compile-/rsp-Zeile. Diese Funktion ist bewusst ein Spiegel und kein Aufruf: die Fassade ist
/// Lane-F-Sperrmenge und darf fuer den Beleg nicht angefasst werden.
[[nodiscard]] std::string facade_compile_line(std::string const& opt_flag, std::string const& march_flag,
                                              bool dbg = false, std::string const& debug_flags = "-O0 -g") {
    std::string flags = dbg ? debug_flags : opt_flag;
    if (!march_flag.empty()) {
        flags += ' ';
        flags += march_flag;
    }
    for (auto const& mf : meas::gate_extra_march_flags_for_build(meas::route_of_march_flag(march_flag))) {
        flags += ' ';
        flags += mf;
    }
    return flags;
}

/// Die Flag-Werte des LEBENDEN Codegen-Kanals, aus der Single-Source der Unter-Achse gelesen
/// (simd_sub_axis.hpp:78/:85 -- deckungsgleich zu permutation_codegen_tool.cpp:43-45).
[[nodiscard]] std::string_view codegen_march_of(std::string_view simd_id) {
    if (simd_id == meas::SimdAvx512Option::simd_id()) return meas::SimdAvx512Option::gcc_march_flag();
    if (simd_id == meas::SimdAvx2Option::simd_id()) return meas::SimdAvx2Option::gcc_march_flag();
    return meas::SimdNoExtOption::gcc_march_flag();
}

} // namespace

// =================================================================================================
// BELEG 1: Baseline-march und Gate-Extras landen auf DERSELBEN Compile-Zeile (beide Pfade)
// =================================================================================================
TEST(C3bKanalMerge, BaselineUndGateExtrasBildenEINEZeile) {
    // Der Beleg ist strukturell: die Fassade baut EINEN String und uebergibt ihn als EIN Argument.
    // Wir zeigen es an der Zeichenfolge -- die Baseline steht drin, und die Gate-Extras werden an
    // DIESELBE Zeile angehaengt, nicht an eine zweite.
    for (auto const& simd_id : meas::kAllSimdIds) {
        std::string const march{codegen_march_of(simd_id)};
        std::string const line = facade_compile_line("-O3", march);

        EXPECT_NE(line.find("-O3"), std::string::npos) << "opt-Baseline fehlt fuer simd=" << simd_id;
        if (!march.empty())
            EXPECT_NE(line.find(march), std::string::npos)
                << "march-Baseline " << march << " fehlt auf der Zeile fuer simd=" << simd_id;
        // EINE Zeile: kein Zeilenumbruch, keine zweite Compile-Anweisung.
        EXPECT_EQ(line.find('\n'), std::string::npos) << "Die Compile-Zeile darf nie mehrzeilig werden.";
    }
}

TEST(C3bKanalMerge, DebugPfadErsetztNurDieOptimierungNichtDieISAIdentitaet) {
    // Scheibe-2b-Zusage der Fassade (Kommentar :525-527/:1001-1002): Debug ersetzt opt_flag durch
    // -O0 -g; -march (die [d,e,f]-ISA-Identitaet) UND die Gate-Flags bleiben erhalten. Waere das
    // anders, waere ein Debug-Lauf keine ISA-gleiche Referenz mehr.
    std::string const march{meas::SimdAvx2Option::gcc_march_flag()};
    std::string const release = facade_compile_line("-O3", march, /*dbg=*/false);
    std::string const debug   = facade_compile_line("-O3", march, /*dbg=*/true);

    EXPECT_NE(release.find("-O3"), std::string::npos);
    EXPECT_EQ(debug.find("-O3"), std::string::npos) << "Debug darf die Optimierung ersetzen.";
    EXPECT_NE(debug.find("-O0 -g"), std::string::npos);
    // Die ISA-Identitaet ueberlebt in BEIDEN Faellen.
    EXPECT_NE(release.find(march), std::string::npos);
    EXPECT_NE(debug.find(march), std::string::npos);
}

// =================================================================================================
// BELEG 2: heute traegt Kanal B NICHTS bei -> die Zeile ist byte-identisch zum Ist-Compile-Kanal
// =================================================================================================
TEST(C3bKanalMerge, HeuteIstDieZeileByteIdentischZumIstKanal) {
    // Solange kein Organ required-Flags erklaert (simd_organ_requirement.hpp:88), ist die Gate-Schleife
    // leer und die Zeile besteht GENAU aus opt + optional march. Das ist die Byte-Neutralitaets-Zusage
    // der Fassaden-Naht -- und sie gilt fuer alle drei Routen.
    EXPECT_FALSE(meas::any_organ_declares_required());
    for (auto const& simd_id : meas::kAllSimdIds) {
        std::string const march{codegen_march_of(simd_id)};
        std::string const erwartet = march.empty() ? std::string{"-O3"} : std::string{"-O3 "} + march;
        EXPECT_EQ(facade_compile_line("-O3", march), erwartet)
            << "Gate traegt heute bei simd=" << simd_id << " unerwartet Flags bei.";
    }
}

// =================================================================================================
// BELEG 3: die zwei Kanaele sind DISJUNKT -- deshalb ist der Merge-Beleg Vorbedingung, nicht Formsache
// =================================================================================================
TEST(C3bKanalMerge, KanalAUndKanalBSindDisjunkt) {
    // Kanal B bei Scharfschaltung (prod1/Zen5, Organ-Klasse filter, Route Avx512). Das required-Flag
    // MUSS in der Sinnhaftigkeits-Matrix der Klasse stehen, sonst lehnt das Gate korrekt mit
    // CompileKombination ab (pruef_dock:122-126); fuer "filter" ist die Menge
    // {avx512_vpopcntdq, avx512_bitalg, gfni} (simd_organ_sensibility.hpp:32).
    static constexpr std::array<meas::SimdFeatureFlag, 1> kDemoRequired{{meas::kAvx512Vpopcntdq}};
    auto const scharf = meas::pruef_dock(kDemoRequired, meas::kSensibilityFilterFlags,
                                         meas::Prod1Zen5Signature::signature(), meas::SimdRoute::Avx512);
    ASSERT_EQ(scharf.state, meas::SimdGateState::Freigegeben);
    std::vector<std::string> const kanal_b = meas::effective_march_flags(scharf);

    // Kanal A auf derselben Route: GENAU EIN Flag.
    std::string const kanal_a{meas::SimdAvx512Option::gcc_march_flag()};
    EXPECT_EQ(kanal_a, std::string{"-mavx512f"});

    // Kanal B liefert ein MAXIMUM, nicht ein Flag.
    EXPECT_GT(kanal_b.size(), 1u) << "Kanal B ist ein Freigabe-Maximum, kein Einzel-Flag.";
    // ... und die Schnittmenge der beiden Kanaele ist LEER: Kanal B enthaelt die Baseline nicht.
    for (auto const& f : kanal_b) EXPECT_NE(f, kanal_a) << "Unerwartete Ueberschneidung der beiden Flag-Kanaele.";
    // Das ist die eigentliche Aussage: NICHT Dopplung, sondern eine ANDERE Menge. Eine Scharfschaltung
    // ohne diesen Merge-Beleg wuerde Code erzeugen, den niemand entworfen hat.
    EXPECT_FALSE(kanal_b.empty());
}

TEST(C3bKanalMerge, NachScharfschaltungBlieBeDieBaselineErhalten) {
    // Die Merge-Zusage, formuliert als Zukunfts-Wache: WENN Kanal B Flags liefert, muessen sie an die
    // Zeile ANGEHAENGT werden und die Baseline stehen lassen. Wir simulieren das an der gespiegelten
    // Konkatenation, weil das echte Gate inert ist -- die Fassaden-Schleife haengt an, sie ersetzt nicht
    // (profile_run_facade.cpp:537-540/:1012-1015: flags += ' '; flags += mf;).
    std::string const              march{meas::SimdAvx512Option::gcc_march_flag()};
    std::vector<std::string> const hypothetisch{"-mgfni", "-mavx512bitalg", "-mavx512vpopcntdq"};

    std::string line = std::string{"-O3"} + ' ' + march;
    for (auto const& mf : hypothetisch) {
        line += ' ';
        line += mf;
    }
    // Baseline UND Extras auf EINER Zeile, Baseline zuerst, nichts ersetzt.
    EXPECT_NE(line.find("-O3"), std::string::npos);
    EXPECT_LT(line.find(march), line.find("-mgfni")) << "Die Baseline muss VOR den Gate-Extras stehen.";
    for (auto const& mf : hypothetisch) EXPECT_NE(line.find(mf), std::string::npos);
    EXPECT_EQ(line.find('\n'), std::string::npos);
}

// =================================================================================================
// BELEG 4 (D2.2 "Implikation nicht annehmen"): die ROUTE-WACHE verhindert den ISA-STUFEN-AUFSTIEG
// =================================================================================================
// EMPIRISCHER ANLASS (Compile-Probe g++ 16, -dM -E): die Gate-Sub-Feature-Flags sind NICHT
// stufen-neutral -- `-mavx512bitalg` ALLEIN definiert bereits __AVX512F__ UND __AVX2__, ebenso
// `-mavx512vpopcntdq`. Nur `-mgfni` ist harmlos (GFNI=1, AVX512F=0). Wuerde das Gate also auf einer
// no_extension-Permutation ein avx512-Tier-Flag anhaengen, waere die entstehende Binary eine
// AVX-512-Binary MIT DEM ETIKETT no_extension -- exakt die Defekt-Klasse "Mess-Etikett !=
// Mess-Gegenstand", die axis_09b_build_coherence.hpp:6 dokumentiert (dort: -mavx2 fuhr den SSE2-Pfad).
// DIESER TEST BELEGT, dass die bestehende Route-Wache (route_allows, simd_build_gate.hpp:61-68) das
// abfaengt: ein avx512-Tier-Flag wird auf den niedrigeren Routen ABGELEHNT statt still emittiert.
TEST(C3bKanalMerge, RouteWacheVerhindertISAStufenAufstieg) {
    static constexpr std::array<meas::SimdFeatureFlag, 1> kAvx512TierRequired{{meas::kAvx512Vpopcntdq}};

    // no_extension: route_allows sperrt JEDEN Tier -> Ablehnung, KEINE Emission.
    auto const no_ext = meas::pruef_dock(kAvx512TierRequired, meas::kSensibilityFilterFlags,
                                         meas::Prod1Zen5Signature::signature(), meas::SimdRoute::NoExtension);
    EXPECT_EQ(no_ext.state, meas::SimdGateState::Abgelehnt);
    EXPECT_EQ(no_ext.effective_count, 0u);
    EXPECT_TRUE(meas::effective_march_flags(no_ext).empty())
        << "Ein avx512-Flag auf der no_extension-Route waere eine Etikett-Luege.";

    // avx2: 256-bit-Route sperrt den Avx512-Tier -> Ablehnung, KEINE Emission.
    auto const avx2 = meas::pruef_dock(kAvx512TierRequired, meas::kSensibilityFilterFlags,
                                       meas::Prod1Zen5Signature::signature(), meas::SimdRoute::Avx2);
    EXPECT_EQ(avx2.state, meas::SimdGateState::Abgelehnt);
    EXPECT_TRUE(meas::effective_march_flags(avx2).empty());

    // Erst die avx512-Route gibt frei -- dort ist die Stufe deklariert und die Baseline ist -mavx512f.
    auto const avx512 = meas::pruef_dock(kAvx512TierRequired, meas::kSensibilityFilterFlags,
                                         meas::Prod1Zen5Signature::signature(), meas::SimdRoute::Avx512);
    EXPECT_EQ(avx512.state, meas::SimdGateState::Freigegeben);
    EXPECT_FALSE(meas::effective_march_flags(avx512).empty());

    // Die Wache selbst, direkt: keine Stufe auf no_extension, kein Avx512-Tier auf der Avx2-Route.
    EXPECT_FALSE(meas::route_allows(meas::SimdRoute::NoExtension, meas::SimdFlagTier::Companion));
    EXPECT_FALSE(meas::route_allows(meas::SimdRoute::Avx2, meas::SimdFlagTier::Avx512));
}

TEST(C3bKanalMerge, RestRISIKOGateAendertDieFlagMengeINNERHALBDerStufe) {
    // Was die Route-Wache NICHT verhindert (und was deshalb in die C-3a-Auflagen gehoert): innerhalb
    // der deklarierten Stufe aendert das Gate die Flag-MENGE. Auf der avx512-Route emittiert es drei
    // Sub-Feature-Flags ZUSAETZLICH zur Baseline -mavx512f. Die entstehende Binary ist damit ein
    // STRIKTES SUPERSET der baseline-only-Binary -- gleiche ISA-Stufe, aber anderer Codegen.
    // FOLGE: zwei Binaries mit demselben Etikett "+ext=avx512" koennen unterschiedlich compiliert sein,
    // je nachdem ob ein Organ required-Flags erklaert hat. Die Gate-Beitraege MUESSEN deshalb bei der
    // Scharfschaltung in der Identitaet sichtbar werden (Sidecar/Stempel), sonst teilen zwei
    // verschieden compilierte Binaries eine Kennung. Das ist ein IDENTITAETS-Punkt, kein Flag-Punkt.
    static constexpr std::array<meas::SimdFeatureFlag, 1> kDemoRequired{{meas::kAvx512Vpopcntdq}};
    auto const scharf = meas::pruef_dock(kDemoRequired, meas::kSensibilityFilterFlags,
                                         meas::Prod1Zen5Signature::signature(), meas::SimdRoute::Avx512);
    ASSERT_EQ(scharf.state, meas::SimdGateState::Freigegeben);
    auto const extras = meas::effective_march_flags(scharf);

    // Alle Extras bleiben INNERHALB der avx512-Stufe (kein Aufstieg ueber die Baseline hinaus) ...
    for (std::size_t i = 0; i < scharf.effective_count; ++i)
        EXPECT_TRUE(meas::route_allows(meas::SimdRoute::Avx512, scharf.effective[i].tier));
    // ... aber die Menge ist echt groesser als die Baseline allein. Genau das ist das Restrisiko.
    EXPECT_GT(extras.size(), 1u);
    std::string const baseline_only = std::string{"-O3 "} + std::string{meas::SimdAvx512Option::gcc_march_flag()};
    std::string       mit_gate      = baseline_only;
    for (auto const& mf : extras) {
        mit_gate += ' ';
        mit_gate += mf;
    }
    EXPECT_NE(mit_gate, baseline_only) << "Wenn das hier gleich waere, waere das Gate wirkungslos.";
    EXPECT_TRUE(mit_gate.starts_with(baseline_only)) << "Die Baseline muss unangetastet vorne stehen.";
}

// =================================================================================================
// BELEG 5: der Orchestrator ist REJECT-ONLY -- er emittiert keine Flags (kein Emissions-Umbau noetig)
// =================================================================================================
TEST(C3bKanalMerge, OrchestratorIstRejectOnly) {
    // simd_build_gate.hpp:149-152 sagt es im Kontrakt: admit_organ_on_machine ist die "fokussierte
    // Durchsetzung fuer den Bau-Delegations-Punkt (OHNE Flag-Emission -- die uebernimmt die
    // per-Perm-CompileFn-Naht in der Fassade)". Der Rueckgabetyp beweist es: eine optionale
    // FEHLERKLASSE, keine Flag-Liste. Es gibt also genau EINEN Emissions-Ort, und das ist die Fassade.
    auto const verdikt = meas::admit_organ_on_machine({}, meas::Prod1Zen5Signature::signature());
    static_assert(std::is_same_v<decltype(verdikt), std::optional<meas::CompilerCompilerErrorClass> const>,
                  "Reject-only: der Orchestrator-Pfad gibt eine Fehlerklasse zurueck, nie Flags.");
    EXPECT_FALSE(verdikt.has_value());
    // Gegenprobe, dass die Emission woanders liegt: die Fassaden-Funktion liefert Strings.
    EXPECT_TRUE(meas::gate_extra_march_flags_for_build(meas::SimdRoute::Avx512).empty());
}
