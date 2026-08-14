// test_stempel_vollbestand_registry.cpp -- S-7 / Paket P2 (13.08.2026): VOLLBESTANDS-BEWEIS des
// Achsen-Algo-Hardware-Stempels ueber die Registry-Typlisten.
//
// GEGENSTAND: die S-3a-Ordnungs-Relation und die Zulassungs-Bruecke (algo_stempel_zulassung.hpp)
// sind auf JEDER registrierten Organ-Variante ANWENDBAR, und der heutige Bestand ist UEBERALL
// zulassbar (Inertheits-Beweis der S-7-Aktivierung). Laeuft ueber AllRegisteredOrganVariantsFlat
// (axis_variant_version_table.hpp:150) -- die VOLLE registrierte Population VOR dem
// is_enabled-Filter, nicht nur die Enabled-Tabelle.
//
// ABGRENZUNG zur Geschwister-TU test_reflect_versions_all_registered (CX-W6): dort wird die
// GRAMMATIK der Literale bewacht (assert_version_grammar je W); HIER wird die SEMANTIK der
// S-7-Bruecke auf dieselbe Population angewandt (Relation reflexiv + Signatur-Deckung). Kein
// Doppel: andere Aussage, gleiche Population, gleicher Include-/Link-Satz (beide ziehen alle
// Kompositions-Registries -- bewusst separate, schwere TU).
//
// NENNER GEMESSEN, NICHT GEPINNT: kAllRegisteredOrganVariantCount via mp_size; ASSERT >= 100 VOR
// der Schleife. T-3 FREMDER NENNER: build_axis_variant_version_table().size() (andere Ableitung:
// Enabled-Emit statt Roh-Registrierung) mit EXPECT_GT(registriert, enabled). Quelltext-grep-Beleg
// zum Stand 5f3f26a5 (dokumentiert, nicht gepinnt): 123 algo_version-Literale unter axes/+topics/
// (97x "1.0.0.c" + 2x "1.0.1.c" + 24x "1.0.2.c"), getragen von 122 registrierten Varianten mit
// eigenem Literal (k_ary traegt ZWEI Literale, KON58-07 G3; weitere Varianten erben ihr Literal
// aus einer gemeinsamen Basis und zaehlen im mp_size-Nenner mit).
//
// ASCII-only (Leitplanke). Zeilen <= 120 (Diff-Hygiene-Wache).

#include <builder/experiment_tree/axis_variant_version_table.hpp> // AllRegisteredOrganVariantsFlat / Tabelle
#include <cache_engine/measurement/algo_semver.hpp>
#include <cache_engine/measurement/algo_stempel_zulassung.hpp>
#include <cache_engine/measurement/flag_menge_ordnung.hpp>
#include <cache_engine/measurement/machine_simd_signature.hpp>

#include <boost/mp11.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

namespace ex   = ::comdare::cache_engine::builder::experiment;
namespace meas = ::comdare::cache_engine::measurement;
namespace mp   = ::boost::mp11;

namespace {

// Die vier Signatur-Welten des Beweises: die drei deklarierten Maschinen-Klassen UND die leere
// Signatur (keine Belegung / kein Verdict-Match) -- der Bestand muss in ALLEN vier zulassbar sein.
struct SignaturWelt {
    char const*                            name;
    std::span<meas::SimdFeatureFlag const> signatur;
};

[[nodiscard]] std::vector<SignaturWelt> alle_signatur_welten() {
    return {{"leer", {}},
            {"prod1_zen5", meas::Prod1Zen5Signature::signature()},
            {"prod2_raptor_lake", meas::Prod2RaptorLakeSignature::signature()},
            {"odroid_gracemont", meas::OdroidGracemontSignature::signature()}};
}

} // namespace

// -- (1) NENNER: gemessen (mp_size) + fremder Nenner (Enabled-Tabelle, andere Ableitung) ----------
TEST(StempelVollbestandRegistry, NennerGemessenUndFremderNenner) {
    ASSERT_GE(ex::kAllRegisteredOrganVariantCount, 100u)
        << "Vorbedingung VOR jeder Schleife: der Registry-Nenner traegt >= 100 Varianten.";
    std::vector<ex::AxisVariantVersion> const enabled = ex::build_axis_variant_version_table();
    ASSERT_FALSE(enabled.empty());
    EXPECT_GT(ex::kAllRegisteredOrganVariantCount, enabled.size())
        << "T-3 fremder Nenner: die Roh-Registrierung MUSS echt groesser sein als die Enabled-Tabelle.";
    std::cout << "[S-7 Nenner] registriert=" << ex::kAllRegisteredOrganVariantCount << " enabled=" << enabled.size()
              << " (beide gemessen, nicht gepinnt)\n";
}

// -- (2) SYNTAX auf JEDER registrierten Variante: parse non-sentinel + wellformed -----------------
TEST(StempelVollbestandRegistry, JedeRegistrierteVarianteParstUndIstWohlgeformt) {
    ASSERT_GE(ex::kAllRegisteredOrganVariantCount, 100u);
    std::size_t geprueft = 0;
    mp::mp_for_each<mp::mp_transform<mp::mp_identity, ex::AllRegisteredOrganVariantsFlat>>([&](auto id) {
        using W                    = typename decltype(id)::type;
        std::string_view const raw = W::algo_version;
        EXPECT_FALSE(meas::parse_algo_semver(raw).is_sentinel()) << "unparsbar: '" << raw << "'";
        EXPECT_TRUE(meas::ce_owned_version_is_wellformed(raw)) << "nicht wohlgeformt: '" << raw << "'";
        ++geprueft;
    });
    EXPECT_EQ(geprueft, ex::kAllRegisteredOrganVariantCount)
        << "die Schleife muss GENAU den gemessenen Nenner abarbeiten.";
    std::cout << "[S-7 Syntax] " << geprueft << " von " << ex::kAllRegisteredOrganVariantCount
              << " registrierten Varianten geprueft\n";
}

// -- (3) SEMANTIK-Anwendbarkeit: die S-3a-Relation ist auf JEDER Variante reflexiv ----------------
TEST(StempelVollbestandRegistry, RelationReflexivAufJederRegistriertenVariante) {
    ASSERT_GE(ex::kAllRegisteredOrganVariantCount, 100u);
    std::size_t geprueft = 0;
    mp::mp_for_each<mp::mp_transform<mp::mp_identity, ex::AllRegisteredOrganVariantsFlat>>([&](auto id) {
        using W                    = typename decltype(id)::type;
        meas::AlgoSemVer const ver = meas::parse_algo_semver(W::algo_version);
        EXPECT_TRUE(meas::flag_menge_ist_teilmenge(ver, ver))
            << "Relation nicht reflexiv auf '" << W::algo_version << "' -- Semantik nicht anwendbar.";
        ++geprueft;
    });
    EXPECT_EQ(geprueft, ex::kAllRegisteredOrganVariantCount);
}

// -- (4) INERTHEITS-BEWEIS: der heutige Bestand ist gegen ALLE vier Signatur-Welten zulassbar -----
// (Bestands-Aussage der S-7-Aktivierung: 0 Ablehnungen => die Bruecke ist heute byte-/golden-
// neutral by construction; die Zahl der Pruefungen wird ausgegeben, Nenner = 4 Welten x N.)
TEST(StempelVollbestandRegistry, BestandUeberallZulassbarInAllenVierSignaturWelten) {
    ASSERT_GE(ex::kAllRegisteredOrganVariantCount, 100u);
    auto const  welten      = alle_signatur_welten();
    std::size_t pruefungen  = 0;
    std::size_t ablehnungen = 0;
    mp::mp_for_each<mp::mp_transform<mp::mp_identity, ex::AllRegisteredOrganVariantsFlat>>([&](auto id) {
        using W                    = typename decltype(id)::type;
        meas::AlgoSemVer const ver = meas::parse_algo_semver(W::algo_version);
        for (SignaturWelt const& w : welten) {
            ++pruefungen;
            if (!meas::flag_menge_in_signatur(ver, w.signatur)) {
                ++ablehnungen;
                ADD_FAILURE() << "'" << W::algo_version << "' NICHT gedeckt in Signatur-Welt '" << w.name
                              << "' -- der Bestand waere nach der Aktivierung nicht mehr ueberall baubar.";
            }
        }
    });
    EXPECT_EQ(pruefungen, 4u * ex::kAllRegisteredOrganVariantCount);
    EXPECT_EQ(ablehnungen, 0u);
    std::cout << "[S-7 Inertheit] " << ablehnungen << " Ablehnungen von " << pruefungen
              << " Pruefungen (4 Signatur-Welten x " << ex::kAllRegisteredOrganVariantCount
              << " registrierte Varianten)\n";
}
