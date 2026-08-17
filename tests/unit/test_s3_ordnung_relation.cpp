// test_s3_ordnung_relation -- S-3 / Paket P1 (Task #4): die ORDNUNGS-RELATION ueber Flag-MENGEN (S-3a)
// und die VORAUSSETZUNGS-WACHE der Compile-Seite (S-3b, Owner-Wort KON16-02).
//
// GEGENSTAND (drei Prueflinge, eine TU):
//   (a) flag_menge_ist_teilmenge  (measurement/flag_menge_ordnung.hpp) -- ASYMMETRISCH, fail-closed,
//       Element == (token, eltern)-Paar; das X.Y.Z-Tripel ist NICHT Teil der Mengen-Rechnung.
//   (b) flag_menge_in_signatur    (ebd.) -- die Signatur-Bruecke Grammatik-Token -> SimdFeatureFlag,
//       AUSSCHLIESSLICH ueber die Tabellenfelder von kFlagGrammarCatalog (drei Namensraeume!).
//   (c) voraussetzungen_erfuellt  (measurement/algo_semver.hpp) + die Ketten-Tabelle
//       kFlagVoraussetzungsKetten (measurement/flag_grammar_catalog.hpp) + die UNGATED Einhaengung als
//       Konjunktions-Term in ce_owned_version_is_wellformed (KON11-01) + die Geschlossenheits-Wache
//       signatur_ist_voraussetzungs_geschlossen ueber die deklarierten Maschinen-Signaturen.
//
// NAMENSKONVENTION test_s3_ordnung_* (Kollisionsschutz: test_s5_* / test_s7_* sind anders belegt).
// Die CT-Batterie ist der Beweis; die Laufzeit-Faelle spiegeln sie, damit ein Koeder (K13) auch als
// ctest-ROT sichtbar wird und nicht nur als Uebersetzungsbruch.
//
// ASCII-only (Leitplanke). Zeilen <= 120 (Diff-Hygiene-Wache).

#include <cache_engine/measurement/algo_semver.hpp>
#include <cache_engine/measurement/flag_grammar_catalog.hpp>
#include <cache_engine/measurement/flag_menge_ordnung.hpp>
#include <cache_engine/measurement/machine_simd_signature.hpp>
#include <cache_engine/measurement/measurement_framework_registry.hpp>
#include <mess_axes/measurement_tooling_registry.hpp>

#include <gtest/gtest.h>

#include <array>
#include <span>
#include <string_view>

namespace m = ::comdare::cache_engine::measurement;

namespace {

// =================================================================================================
// (a) S-3a -- DIE ORDNUNGS-RELATION: CT-Batterie
// =================================================================================================

// Der Erweiterungs-Fall IST der Zweck: die kleinere Menge steckt in der groesseren ...
static_assert(m::flag_menge_ist_teilmenge(m::parse_algo_semver("1.0.0.c.x512{f.vl}"),
                                          m::parse_algo_semver("1.0.0.c.x512{f.vl.bw}")));
// ... und die Umkehr ist FALSCH -- die Asymmetrie ist keine Beigabe, sie ist die Aussage
// (Vorbild bvset_teilmenge.hpp:157-160: Erweiterung gueltig, Einschraenkung nicht).
static_assert(!m::flag_menge_ist_teilmenge(m::parse_algo_semver("1.0.0.c.x512{f.vl.bw}"),
                                           m::parse_algo_semver("1.0.0.c.x512{f.vl}")));
// Reflexiv: jede Menge ist Teilmenge ihrer selbst (Gleichheit bleibt zulaessig).
static_assert(m::flag_menge_ist_teilmenge(m::parse_algo_semver("1.0.0.c{p.e}.x512{f.vl}"),
                                          m::parse_algo_semver("1.0.0.c{p.e}.x512{f.vl}")));
// (token, eltern)-GENAU: 'vnni' unter x256 ist NICHT dasselbe Element wie 'vnni' unter x512
// (avx_vnni gegen avx512_vnni -- zwei CPUID-Bits, kFlagGrammarCatalog fuehrt beide ZEILEN).
static_assert(!m::flag_menge_ist_teilmenge(m::parse_algo_semver("1.0.0.c.x256{vnni}"),
                                           m::parse_algo_semver("1.0.0.c.x512{f.vnni}")));
// DIE K13-BISSPROBE der Eltern-Genauigkeit: rechts stehen ALLE Token der linken Seite (c, x256,
// vnni), aber 'vnni' unter dem FALSCHEN Elternteil. Eine Mutation, die das Eltern-Token ignoriert,
// saehe hier eine Teilmenge -- nur das (token, eltern)-PAAR unterscheidet die beiden Seiten.
static_assert(!m::flag_menge_ist_teilmenge(m::parse_algo_semver("1.0.0.c.x256{vnni}"),
                                           m::parse_algo_semver("1.0.0.c.x256.x512{f.vnni}")));
static_assert(!m::flag_menge_ist_teilmenge(m::parse_algo_semver("1.0.0.c.x256{ifma}"),
                                           m::parse_algo_semver("1.0.0.c.x512{f.ifma}")));
// ... am selben Elternteil traegt der Doppelgaenger natuerlich.
static_assert(m::flag_menge_ist_teilmenge(m::parse_algo_semver("1.0.0.c.x256{vnni}"),
                                          m::parse_algo_semver("1.0.0.c.x256{vnni.ifma}")));
// Das X.Y.Z-Tripel ist NICHT Teil der Mengen-Rechnung (es gehoert der zweistufigen Versionierung).
static_assert(m::flag_menge_ist_teilmenge(m::parse_algo_semver("2.5.7.c"), m::parse_algo_semver("1.0.0.c")));
static_assert(m::flag_menge_ist_teilmenge(m::parse_algo_semver("1.0.0.c"), m::parse_algo_semver("999999.0.1.c")));
// Die leere Flag-Menge (nackte Version) ist Teilmenge jeder Menge -- beide Seiten nicht-Sentinel.
static_assert(m::flag_menge_ist_teilmenge(m::parse_algo_semver("1.0.0"), m::parse_algo_semver("1.0.0.c")));
static_assert(!m::flag_menge_ist_teilmenge(m::parse_algo_semver("1.0.0.c"), m::parse_algo_semver("1.0.0")));
// FAIL-CLOSED (Vorbild bvset_teilmenge.hpp:146-165): Sentinel/unparsbar auf EINER der Seiten => false.
// Ein unparsbares Literal parst auf den Sentinel (K-5) -- die Relation lehnt beide Formen ab.
static_assert(!m::flag_menge_ist_teilmenge(m::AlgoSemVer{}, m::parse_algo_semver("1.0.0.c")));
static_assert(!m::flag_menge_ist_teilmenge(m::parse_algo_semver("1.0.0.c"), m::AlgoSemVer{}));
static_assert(!m::flag_menge_ist_teilmenge(m::parse_algo_semver("v1.0.0c"), m::parse_algo_semver("1.0.0.c")));
static_assert(!m::flag_menge_ist_teilmenge(m::parse_algo_semver("1.0.0.c"), m::parse_algo_semver("quatsch")));
static_assert(!m::flag_menge_ist_teilmenge(m::AlgoSemVer{}, m::AlgoSemVer{}));
// Tiefe zaehlt ueber das ELTERN-Token, nicht ueber die Zahl: c auf Tiefe 0 gegen c{p} -- das
// Element (p, c) fehlt links, (c, "") ist beiden gemeinsam.
static_assert(m::flag_menge_ist_teilmenge(m::parse_algo_semver("1.0.0.c"), m::parse_algo_semver("1.0.0.c{p}")));
static_assert(!m::flag_menge_ist_teilmenge(m::parse_algo_semver("1.0.0.c{p}"), m::parse_algo_semver("1.0.0.c")));

// =================================================================================================
// (b) S-3a -- DIE SIGNATUR-BRUECKE: CT-Batterie
// =================================================================================================

// prod1 (Zen 5) traegt avx512f UND avx512vl -- die Version ist durch die Signatur gedeckt.
static_assert(m::flag_menge_in_signatur(m::parse_algo_semver("1.0.0.c.x512{f.vl}"),
                                        m::Prod1Zen5Signature::signature()));
// prod2 (AVX-512 fused-off) traegt kein avx512f -- dieselbe Version faellt.
static_assert(!m::flag_menge_in_signatur(m::parse_algo_semver("1.0.0.c.x512{f}"),
                                         m::Prod2RaptorLakeSignature::signature()));
// Token ohne cpuinfo-Feld (c, p, e, die Basen) sind zulassungs-neutral: Struktur, keine Faehigkeit.
static_assert(m::flag_menge_in_signatur(m::parse_algo_semver("1.0.0.c{p.e}"),
                                        m::Prod2RaptorLakeSignature::signature()));
static_assert(m::flag_menge_in_signatur(m::parse_algo_semver("1.0.0.c.x512"),
                                        m::Prod2RaptorLakeSignature::signature()));
static_assert(m::flag_menge_in_signatur(m::parse_algo_semver("1.0.0.c.m64"), m::Prod1Zen5Signature::signature()));
// Der vnni-Doppelgaenger ueber die TABELLE, nie ueber String-Heuristik: x256{vnni} -> avx_vnni
// (prod2 traegt es), x512{vnni} -> avx512_vnni (prod2 traegt es NICHT).
static_assert(m::flag_menge_in_signatur(m::parse_algo_semver("1.0.0.c.x256{vnni}"),
                                        m::Prod2RaptorLakeSignature::signature()));
static_assert(!m::flag_menge_in_signatur(m::parse_algo_semver("1.0.0.c.x512{vnni}"),
                                         m::Prod2RaptorLakeSignature::signature()));
// Katalog-Token mit LEERER cpuinfo-Id einer Faehigkeits-Sorte ("ueber den heutigen Erhebungsweg nicht
// signaturfaehig", kFlagGrammarCatalog x256-Block): ein Signatur-Match kann NIE eintreten -> false,
// fail-closed -- nicht neutral (neutral ist nur Struktur ohne Compiler-Schalter).
static_assert(!m::flag_menge_in_signatur(m::parse_algo_semver("1.0.0.c.x256{vnniint8}"),
                                         m::Prod1Zen5Signature::signature()));
// cpuinfo gesetzt, aber im 23er-Signatur-Vokabular (kSimdFeatureFlagCatalog) nicht fuehrbar: die
// deklarierte WERKZEUG-GRENZE -- keine Signatur kann 'sse2' zusagen, also faellt die Forderung.
static_assert(!m::flag_menge_in_signatur(m::parse_algo_semver("1.0.0.c.x128{sse2}"),
                                         m::Prod1Zen5Signature::signature()));
// Katalogfremdes Token und Sentinel: fail-closed.
static_assert(!m::flag_menge_in_signatur(m::parse_algo_semver("1.0.0.c.quatsch"), m::Prod1Zen5Signature::signature()));
static_assert(!m::flag_menge_in_signatur(m::AlgoSemVer{}, m::Prod1Zen5Signature::signature()));
// Leere Flag-Menge: nichts gefordert, nichts verletzt -- auch gegen die LEERE Signatur.
static_assert(m::flag_menge_in_signatur(m::parse_algo_semver("1.0.0"), std::span<m::SimdFeatureFlag const>{}));
static_assert(!m::flag_menge_in_signatur(m::parse_algo_semver("1.0.0.c.x512{f}"),
                                         std::span<m::SimdFeatureFlag const>{}));

// =================================================================================================
// (c) S-3b -- DIE VORAUSSETZUNGS-WACHE: CT-Batterie
// =================================================================================================

// Die Ketten-Tabelle traegt GENAU die belegten Ketten: 13 x512-Subsets -> x512{f} (die Synthese sagt
// "alle gegated auf avx512f (CPUID Leaf 7)"; 'f' selbst ist das Fundament und hat KEINE Kette),
// vaes -> x128{aes} und vpclmulqdq -> x128{pclmulqdq} (Synthese woertlich: "Voraussetzungskette
// aes->vaes und pclmulqdq->vpclmulqdq").
static_assert(m::kFlagVoraussetzungsKetten.size() == 15);
static_assert(m::find_flag_voraussetzung("f", "x512") == m::kKeineVoraussetzungsKette);
static_assert(m::find_flag_voraussetzung("vl", "x512") != m::kKeineVoraussetzungsKette);
static_assert(m::find_flag_voraussetzung("vaes", "") != m::kKeineVoraussetzungsKette);
static_assert(m::find_flag_voraussetzung("vpclmulqdq", "") != m::kKeineVoraussetzungsKette);
// Wo KEINE formale Abhaengigkeit belegbar ist, steht KEINE Kette -- ausdruecklich (m64-Familie: kein
// AMD-Dokument spricht ein CPUID-Requires aus; gfni hat eine Legacy-SSE-Form OHNE AVX).
static_assert(m::find_flag_voraussetzung("mmx", "m64") == m::kKeineVoraussetzungsKette);
static_assert(m::find_flag_voraussetzung("3dnow", "m64") == m::kKeineVoraussetzungsKette);
static_assert(m::find_flag_voraussetzung("gfni", "") == m::kKeineVoraussetzungsKette);
static_assert(m::find_flag_voraussetzung("avx2", "x256") == m::kKeineVoraussetzungsKette);

// FORDERN, NICHT AUFFUELLEN: die Wache verlangt das Voraussetzungs-Element in der MENGE.
static_assert(m::voraussetzungen_erfuellt(m::parse_algo_semver("1.0.0.c.x512{f.vl}")));
static_assert(!m::voraussetzungen_erfuellt(m::parse_algo_semver("1.0.0.c.x512{vl}")));
static_assert(!m::voraussetzungen_erfuellt(m::parse_algo_semver("1.0.0.c.vaes")));
static_assert(m::voraussetzungen_erfuellt(m::parse_algo_semver("1.0.0.c.vaes.x128{aes}")));
static_assert(!m::voraussetzungen_erfuellt(m::parse_algo_semver("1.0.0.c.vpclmulqdq")));
static_assert(m::voraussetzungen_erfuellt(m::parse_algo_semver("1.0.0.c.vpclmulqdq.x128{pclmulqdq}")));
// Mengen-Semantik ("in der Menge stehen"): die Voraussetzung darf in einer ANDEREN Klammer-Gruppe
// derselben Version stehen -- Element ist das (token, eltern)-Paar, nicht die Geschwisterschaft.
static_assert(m::voraussetzungen_erfuellt(m::parse_algo_semver("1.0.0.c.x512{vl}.x512{f}")));
// Alle 13 abhaengigen x512-Subsets auf einmal (mit f) -- und je EIN Subset ohne f faellt.
static_assert(m::voraussetzungen_erfuellt(
    m::parse_algo_semver("1.0.0.c.x512{f.cd.vl.dq.bw.ifma.vbmi.vbmi2.vnni.bitalg.vpopcntdq.vp2intersect.bf16.fp16}")));
static_assert(!m::voraussetzungen_erfuellt(m::parse_algo_semver("1.0.0.c.x512{cd}")));
static_assert(!m::voraussetzungen_erfuellt(m::parse_algo_semver("1.0.0.c.x512{bf16}")));
// Vakuum-Wahrheit: die nackten Bestands-Formen tragen keine Ketten-Knoten.
static_assert(m::voraussetzungen_erfuellt(m::parse_algo_semver("1.0.0.c")));
static_assert(m::voraussetzungen_erfuellt(m::parse_algo_semver("1.0.0.c{p.e}")));
static_assert(m::voraussetzungen_erfuellt(m::AlgoSemVer{})); // Sentinel traegt nie Flags (K-5)

// DIE EINHAENGUNG (KON11-01): neuer Konjunktions-Term in ce_owned_version_is_wellformed, UNGATED --
// alle Bestands-Verbraucher hoeren mit, NULL neue Aufrufstellen.
static_assert(!m::ce_owned_version_is_wellformed("1.0.0.c.x512{vl}"));
static_assert(m::ce_owned_version_is_wellformed("1.0.0.c.x512{f.vl}"));
static_assert(!m::ce_owned_version_is_wellformed("1.0.0.c.vaes"));
static_assert(m::ce_owned_version_is_wellformed("1.0.0.c.vaes.x128{aes}"));
static_assert(!m::ce_owned_version_is_wellformed("1.0.0.c.vpclmulqdq"));
// ... und der BEWEIS, dass der Bestand gruen bleibt: die nackten Formen (alle 123 Bestands-Literale
// sind nackt-c) sind vakuum-wahr unter der neuen Wache.
static_assert(m::ce_owned_version_is_wellformed("1.0.0.c"));
static_assert(m::ce_owned_version_is_wellformed("1.0.1.c"));
static_assert(m::ce_owned_version_is_wellformed("1.0.2.c"));
static_assert(m::ce_owned_version_is_wellformed("1.0.0.c{p}"));
static_assert(m::ce_owned_version_is_wellformed("1.0.0.c{p.e}"));
static_assert(m::ce_owned_version_is_wellformed("1.0.0"));
static_assert(m::ce_owned_version_is_wellformed("0.0.0"));
// FORDERN heisst NICHT auffuellen -- parse->render-Treue (algo_semver.hpp:52-63) bleibt unangetastet:
// die abgelehnte Form rendert unveraendert zurueck, niemand ergaenzt still ein 'f'.
static_assert(m::render_algo_semver(m::parse_algo_semver("1.0.0.c.x512{vl}")).view() == "1.0.0.c.x512{vl}");

// =================================================================================================
// (d) S-3c-Anteil dieser TU -- DIE GESCHLOSSENHEITS-WACHE der deklarierten Signaturen
// =================================================================================================

// Die drei deklarierten Signaturen sind voraussetzungs-geschlossen (prod1 traegt avx512f UND alle
// Subsets explizit; prod2/odroid tragen keine x512-Subsets).
static_assert(m::signatur_ist_voraussetzungs_geschlossen(m::Prod1Zen5Signature::signature()));
static_assert(m::signatur_ist_voraussetzungs_geschlossen(m::Prod2RaptorLakeSignature::signature()));
static_assert(m::signatur_ist_voraussetzungs_geschlossen(m::OdroidGracemontSignature::signature()));
// Die deklarierte WERKZEUG-GRENZE ist BENANNT, nicht verschluckt: genau ZWEI Ketten (vaes->aes,
// vpclmulqdq->pclmulqdq) nennen ein Flag, das der 23er-Signatur-Katalog nicht fuehrt. Ein additiver
// Katalog-Zuwachs (aes/pclmulqdq in kSimdFeatureFlagCatalog) macht diese Zahl LAUT falsch und
// erzwingt damit die Nachfuehrung der Wache -- genau so ist sie gebaut.
static_assert(m::ketten_ausserhalb_signatur_vokabular() == 2);
// Und die Wache BEISST: eine kuenstliche Signatur, die avx512vl OHNE avx512f deklariert, faellt.
inline constexpr std::array<m::SimdFeatureFlag, 2> kUnvollstaendigeSignatur{m::kAvx512Vl, m::kAvx2};
static_assert(!m::signatur_ist_voraussetzungs_geschlossen(kUnvollstaendigeSignatur));
inline constexpr std::array<m::SimdFeatureFlag, 3> kGeschlosseneSignatur{m::kAvx512F, m::kAvx512Vl, m::kAvx2};
static_assert(m::signatur_ist_voraussetzungs_geschlossen(kGeschlosseneSignatur));
// Die leere Signatur ist trivial geschlossen (sie behauptet nichts).
static_assert(m::signatur_ist_voraussetzungs_geschlossen(std::span<m::SimdFeatureFlag const>{}));

// =================================================================================================
// Laufzeit-Spiegel (K13: ein Koeder muss auch als ctest-ROT beissen, nicht nur als CT-Bruch)
// =================================================================================================

TEST(S3OrdnungRelation, TeilmengeIstAsymmetrischUndElternGenau) {
    auto const klein = m::parse_algo_semver("1.0.0.c.x512{f.vl}");
    auto const gross = m::parse_algo_semver("1.0.0.c.x512{f.vl.bw}");
    EXPECT_TRUE(m::flag_menge_ist_teilmenge(klein, gross)) << "Erweiterung MUSS als Teilmenge gelten.";
    EXPECT_FALSE(m::flag_menge_ist_teilmenge(gross, klein)) << "Einschraenkung darf NIE als Teilmenge gelten.";
    EXPECT_FALSE(m::flag_menge_ist_teilmenge(m::parse_algo_semver("1.0.0.c.x256{vnni}"),
                                             m::parse_algo_semver("1.0.0.c.x512{f.vnni}")))
        << "vnni@x256 ist NICHT vnni@x512 -- das Element ist das (token, eltern)-Paar.";
    EXPECT_FALSE(m::flag_menge_ist_teilmenge(m::AlgoSemVer{}, gross)) << "Sentinel ist fail-closed.";
}

TEST(S3OrdnungRelation, SignaturBrueckeLaeuftUeberDieTabelle) {
    EXPECT_TRUE(
        m::flag_menge_in_signatur(m::parse_algo_semver("1.0.0.c.x512{f.vl}"), m::Prod1Zen5Signature::signature()));
    EXPECT_FALSE(
        m::flag_menge_in_signatur(m::parse_algo_semver("1.0.0.c.x512{f}"), m::Prod2RaptorLakeSignature::signature()));
    EXPECT_TRUE(m::flag_menge_in_signatur(m::parse_algo_semver("1.0.0.c.x256{vnni}"),
                                          m::Prod2RaptorLakeSignature::signature()));
    EXPECT_FALSE(
        m::flag_menge_in_signatur(m::parse_algo_semver("1.0.0.c.x512{vnni}"), m::Prod2RaptorLakeSignature::signature()))
        << "avx512_vnni entsteht aus der Tabelle -- eine String-Heuristik haette avx_vnni getroffen.";
}

TEST(S3OrdnungRelation, VoraussetzungsWacheFordertStattAufzufuellen) {
    EXPECT_FALSE(m::voraussetzungen_erfuellt(m::parse_algo_semver("1.0.0.c.x512{vl}")));
    EXPECT_TRUE(m::voraussetzungen_erfuellt(m::parse_algo_semver("1.0.0.c.x512{f.vl}")));
    EXPECT_FALSE(m::ce_owned_version_is_wellformed("1.0.0.c.x512{vl}"))
        << "die Einhaengung in ce_owned_version_is_wellformed ist der KON11-01-Anker.";
    EXPECT_FALSE(m::ce_owned_version_is_wellformed("1.0.0.c.vaes"));
    EXPECT_TRUE(m::ce_owned_version_is_wellformed("1.0.0.c"));
    // Kein stilles Ergaenzen: die Zeichenfolge bleibt byte-treu (Identitaets-Ereignis-Schutz).
    EXPECT_EQ(m::render_algo_semver(m::parse_algo_semver("1.0.0.c.x512{vl}")).view(),
              std::string_view{"1.0.0.c.x512{vl}"});
}

TEST(S3OrdnungRelation, DerBestandBleibtGruen) {
    // Der Beweis am ECHTEN Bestand, nicht an Beispielen: jede Registry-Version besteht die
    // geschaerfte Wache (die Bestands-Literale sind nackt-c; der neue Term ist dort vakuum-wahr).
    for (auto const& t : m::kMeasurementToolingRegistry)
        EXPECT_TRUE(m::ce_owned_version_is_wellformed(t.version))
            << "Tooling-Version '" << t.version << "' faellt am neuen Voraussetzungs-Term.";
    for (auto const& f : m::kMeasurementFrameworkRegistry)
        EXPECT_TRUE(m::ce_owned_version_is_wellformed(f.version))
            << "Framework-Version '" << f.version << "' faellt am neuen Voraussetzungs-Term.";
}

TEST(S3OrdnungRelation, GeschlossenheitsWacheDeckt) {
    EXPECT_TRUE(m::signatur_ist_voraussetzungs_geschlossen(m::Prod1Zen5Signature::signature()));
    EXPECT_TRUE(m::signatur_ist_voraussetzungs_geschlossen(m::Prod2RaptorLakeSignature::signature()));
    EXPECT_TRUE(m::signatur_ist_voraussetzungs_geschlossen(m::OdroidGracemontSignature::signature()));
    EXPECT_FALSE(m::signatur_ist_voraussetzungs_geschlossen(kUnvollstaendigeSignatur))
        << "avx512vl ohne avx512f MUSS als unvollstaendige Deklaration auffallen.";
    EXPECT_EQ(m::ketten_ausserhalb_signatur_vokabular(), 2u)
        << "Werkzeug-Grenze: genau vaes->aes und vpclmulqdq->pclmulqdq liegen ausserhalb des "
           "23er-Signatur-Vokabulars.";
}

} // namespace
