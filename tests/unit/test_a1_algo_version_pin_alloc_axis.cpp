// A1-VERSIONS-PIN der Allokator-Achse (Nachbesserung 2026-08-06, Review-Befund "Bump ohne
// Regressionsorakel").
//
// DER BEFUND. Der A1-Wurf-Vertrag hat den FEHLSCHLAG-Vertrag der Achse 6 geaendert und deshalb alle 26
// Strategien von "v1.0.0c" auf "v1.0.1c" gebumpt (Begruendung: axis_06_allocator_strategy_base.hpp,
// Abschnitt "A1-VERSIONS-BUMP"). Der Bump selbst war UNBEWACHT: keine Zeile im Bestand haette gemerkt,
// wenn er versehentlich zurueckfiele, wenn eine einzelne Strategie ihn nicht mitmachte, wenn eine
// NEUE Strategie mit dem alten Literal in die Registry kaeme -- oder wenn der Bump gar nicht dort
// ankaeme, wo er wirken soll. Genau das ist die Stale-Green-Wurzel (#50) in ihrer Version-Form: der
// inkrementelle Tier-Binary-Cache wuerde vor dem Schnitt gebaute Binaries als aktuell erkennen und
// weiterverwenden -- gemessen wuerde der ALTE Vertrag unter dem NEUEN Quellstand.
//
// WAS DIESE TU PINNT -- in zwei Schichten, weil eine allein nichts taugt:
//
//   (A) DER LITERAL-PIN, compile-hart, 26x einzeln ausgeschrieben. Er ist der Regressions-Faenger:
//       jede Bewegung an einer der 26 algo_version-Zeilen macht den BAU rot und zwingt zum
//       ABSICHTLICHEN Nachziehen dieser Datei. Ausgeschrieben statt ueber die Registry gefaltet --
//       ein Fold ueber mp_for_each faenge zwar dieselben Faelle, benennt beim Bruch aber nicht die
//       Strategie. Dazu die Registry-Seite: GENAU 26 Vendor, und JEDER traegt das Literal (das faengt
//       die 27. Strategie, die mit "v1.0.0c" hereinkaeme und in der Namensliste unten fehlte).
//
//   (B) DER WIRKUNGS-PIN (Alt/Neu-Kontrast). Ein Literal-Pin allein beweist nur, dass eine Zeichenkette
//       im Quelltext steht. Die tragende Frage ist, ob der Bump die FLAECHE erreicht, an der der
//       Rebuild-/Neu-Mess-Selektor entscheidet. Diese Flaeche ist die algo_sig aus
//       compose_algo_signature (.algos-Sidecar; sie speist ueber abi::kSubAxisValuesetSegment auch das
//       Fingerprint-Preimage, an dem dll_is_current skippt). Der Kontrast baut DIESELBE Signatur
//       einmal mit der heutigen Tabelle und einmal mit einer auf "v1.0.0c" zurueckgedrehten
//       Allokator-Zeile und verlangt, dass sich die Signatur BEWEGT. Bewegte sie sich nicht, waere der
//       Bump wirkungslos -- und der Literal-Pin oben eine Beruhigungspille.
//
// TABU-NEUTRAL: liest ausschliesslich; keine Registry-XML, kein Fixture, kein Fingerprint-Anker wird
// beruehrt. Der Kontrast arbeitet auf einer LOKALEN Kopie der Versions-Tabelle.
// ASCII-only.

#include <axes/alloc/axis_06_allocator_registry.hpp>
#include <builder/experiment_tree/axis_variant_version_table.hpp>
#include <cache_engine/measurement/algo_semver.hpp>

#include <boost/mp11.hpp>
#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace al   = ::comdare::cache_engine::alloc;
namespace ex   = ::comdare::cache_engine::builder::experiment;
namespace meas = ::comdare::cache_engine::measurement;

namespace {

/// Das EINE Literal dieser Scheibe. Steht hier einmal als Konstante UND unten 26x ausgeschrieben --
/// die Konstante fuer die Registry-Seite, die 26 Literale fuer die benennende Diagnose.
constexpr std::string_view kA1AllocatorVersion = "v1.0.1c";

/// Der Stand VOR dem A1-Schnitt. Nur fuer den Kontrast in (B); er steht in keiner Quelle mehr.
constexpr std::string_view kVorA1AllocatorVersion = "v1.0.0c";

} // namespace

// -------------------------------------------------------------------------------------------------
// (A) LITERAL-PIN -- 26x compile-hart, in der Reihenfolge der Registry (AllVendors, Batch 1-8 + Pool
//     + VAMPIR). Ein vergessener oder halber Bump bricht HIER, mit dem Namen der Strategie.
// -------------------------------------------------------------------------------------------------
static_assert(al::StdMalloc::algo_version == "v1.0.1c");
static_assert(al::MimallocAllocator::algo_version == "v1.0.1c");
static_assert(al::SnmallocAllocator::algo_version == "v1.0.1c");
static_assert(al::PmrResourceAllocator::algo_version == "v1.0.1c");
static_assert(al::JemallocAllocator::algo_version == "v1.0.1c");
static_assert(al::TCMallocAllocator::algo_version == "v1.0.1c");
static_assert(al::DlmallocAllocator::algo_version == "v1.0.1c");
static_assert(al::HoardAllocator::algo_version == "v1.0.1c");
static_assert(al::SlabAllocator::algo_version == "v1.0.1c");
static_assert(al::MichaelLockFreeAllocator::algo_version == "v1.0.1c");
static_assert(al::ScallocAllocator::algo_version == "v1.0.1c");
static_assert(al::NUMAllocAllocator::algo_version == "v1.0.1c");
static_assert(al::RPMallocAllocator::algo_version == "v1.0.1c");
static_assert(al::LRMallocAllocator::algo_version == "v1.0.1c");
static_assert(al::CAMAAllocator::algo_version == "v1.0.1c");
static_assert(al::StarMallocAllocator::algo_version == "v1.0.1c");
static_assert(al::TCMallocWarehouseAllocator::algo_version == "v1.0.1c");
static_assert(al::HMallocAllocator::algo_version == "v1.0.1c");
static_assert(al::PIMMallocAllocator::algo_version == "v1.0.1c");
static_assert(al::CrystallineAllocator::algo_version == "v1.0.1c");
static_assert(al::ExgenAllocator::algo_version == "v1.0.1c");
static_assert(al::BuddyAllocator::algo_version == "v1.0.1c");
static_assert(al::PtMalloc2Allocator::algo_version == "v1.0.1c");
static_assert(al::VmemMagazinesAllocator::algo_version == "v1.0.1c");
static_assert(al::PoolResourceAllocator::algo_version == "v1.0.1c");
static_assert(al::VampirNfpAllocator::algo_version == "v1.0.1c");

/// Die Registry-Seite desselben Pins: GENAU 26 Vendor. Waechst die Liste, ist die Namensliste oben
/// unvollstaendig -- und der naechste Satz faengt, was ihr fehlt.
static_assert(boost::mp11::mp_size<al::AllVendors>::value == 26);

TEST(A1AlgoVersionPinAllocAxis, JedeRegistrierteStrategieTraegtDasPinLiteral) {
    // Deckt die Luecke der Namensliste: eine NEUE Strategie, die mit "v1.0.0c" (oder irgendetwas
    // anderem) in AllVendors kaeme, faellt hier auf -- auch wenn niemand die 26 Zeilen oben ergaenzt.
    std::vector<std::string> abweichler;
    boost::mp11::mp_for_each<boost::mp11::mp_transform<boost::mp11::mp_identity, al::AllVendors>>([&](auto id) {
        using W = typename decltype(id)::type;
        if (std::string_view{W::algo_version} != kA1AllocatorVersion)
            abweichler.emplace_back(std::string{W::name()} + "=" + std::string{W::algo_version});
    });
    EXPECT_TRUE(abweichler.empty()) << "Strategien der Achse 6 mit abweichender algo_version: "
                                    << [&] {
                                           std::string s;
                                           for (std::string const& a : abweichler) s += a + " ";
                                           return s;
                                       }();
    EXPECT_EQ(boost::mp11::mp_size<al::AllVendors>::value, 26u);
}

TEST(A1AlgoVersionPinAllocAxis, DasPinLiteralIstGrammatikalischWohlgeformt) {
    // Der Pin darf keine Fehlform zementieren: "v1.0.1c" muss die Owner-Q3-Grammatik erfuellen
    // (dreistellig, GENAU EIN Hardware-Flag, im CPU-Scope 'c', NIE experimentell).
    meas::AlgoSemVer const v = meas::parse_algo_semver(kA1AllocatorVersion);
    EXPECT_FALSE(v.is_sentinel());
    EXPECT_EQ(v.x, 1u);
    EXPECT_EQ(v.y, 0u);
    EXPECT_EQ(v.z, 1u) << "PATCH-Stelle: der A1-Schnitt aenderte nur das Fehlschlag-Signal.";
    EXPECT_EQ(v.hardware, meas::HardwareFlag::cpu);
    EXPECT_FALSE(v.experimental) << "das 'e'-Suffix ist AUSSCHLIESSLICH die Pruefling-Markierung.";
    EXPECT_TRUE(meas::ce_owned_version_is_wellformed(kA1AllocatorVersion));
    EXPECT_TRUE(meas::ce_owned_version_satisfies_cpu_enforce(kA1AllocatorVersion));

    // Und der Vorgaenger-Stand ist ECHT verschieden -- sonst waere der Kontrast unten gegenstandslos.
    EXPECT_NE(kA1AllocatorVersion, kVorA1AllocatorVersion);
    meas::AlgoSemVer const alt = meas::parse_algo_semver(kVorA1AllocatorVersion);
    EXPECT_EQ(alt.z, 0u);
}

TEST(A1AlgoVersionPinAllocAxis, DieEnabledTabelleFuehrtDenAllokatorSlotAufDemPinLiteral) {
    std::vector<ex::AxisVariantVersion> const tabelle = ex::build_axis_variant_version_table();
    ASSERT_FALSE(tabelle.empty());

    std::size_t allokator_zeilen = 0;
    for (ex::AxisVariantVersion const& e : tabelle) {
        if (e.axis != "allocator") continue;
        ++allokator_zeilen;
        EXPECT_EQ(e.version, kA1AllocatorVersion)
            << "Allokator-Variante '" << e.variant << "' steht nicht auf dem A1-Pin.";
    }
    EXPECT_GT(allokator_zeilen, 0u) << "ohne aktivierte Allokator-Variante ist der Pin gegenstandslos.";
}

// -------------------------------------------------------------------------------------------------
// (B) WIRKUNGS-PIN -- der Alt/Neu-Kontrast auf der Flaeche, an der der Cache entscheidet.
// -------------------------------------------------------------------------------------------------

namespace {

/// Die Tabelle mit auf den VOR-A1-Stand zurueckgedrehtem Allokator-Slot. LOKALE Kopie -- es wird nichts
/// im Bestand veraendert.
[[nodiscard]] std::vector<ex::AxisVariantVersion> tabelle_mit_alter_allokator_version() {
    std::vector<ex::AxisVariantVersion> t = ex::build_axis_variant_version_table();
    for (ex::AxisVariantVersion& e : t)
        if (e.axis == "allocator") e.version = std::string{kVorA1AllocatorVersion};
    return t;
}

/// Der Name der ersten AKTIVEN Allokator-Variante -- nicht hartkodiert, damit der Test nicht an einer
/// Enable-Schalter-Aenderung zerbricht (die mit dem Versions-Pin nichts zu tun hat).
[[nodiscard]] std::string erste_aktive_allokator_variante(std::vector<ex::AxisVariantVersion> const& t) {
    for (ex::AxisVariantVersion const& e : t)
        if (e.axis == "allocator") return e.variant;
    return {};
}

} // namespace

TEST(A1AlgoVersionPinAllocAxis, BumpBewegtDieAlgoSignatur) {
    std::vector<ex::AxisVariantVersion> const neu = ex::build_axis_variant_version_table();
    std::vector<ex::AxisVariantVersion> const alt = tabelle_mit_alter_allokator_version();

    std::string const variante = erste_aktive_allokator_variante(neu);
    ASSERT_FALSE(variante.empty());

    std::vector<std::pair<std::string, std::string>> const achsen{{"allocator", variante}};
    std::string const sig_neu = ex::compose_algo_signature(achsen, neu);
    std::string const sig_alt = ex::compose_algo_signature(achsen, alt);

    // DIE tragende Aussage: der Bump erreicht die .algos-Signatur. Waeren beide gleich, wuerde der
    // Rebuild-/Neu-Mess-Selektor die vor dem A1-Schnitt gebaute Binary weiterverwenden.
    EXPECT_NE(sig_neu, sig_alt) << "der Versions-Bump ist auf der algo_sig UNSICHTBAR -- genau das war "
                                   "der Grund, warum der Schnitt ueberhaupt gebumpt hat.";
    EXPECT_NE(sig_neu.find("allocator=" + variante + "@v1.0.1c"), std::string::npos) << sig_neu;
    EXPECT_NE(sig_alt.find("allocator=" + variante + "@v1.0.0c"), std::string::npos) << sig_alt;
    EXPECT_EQ(sig_neu.find("@v0.0.0"), std::string::npos)
        << "Sentinel in der Signatur: der Slot wurde nicht in der Tabelle gefunden -- dann pruefte der "
           "Kontrast oben nichts.";
}

TEST(A1AlgoVersionPinAllocAxis, BumpBewegtDieOrganStempelZeile) {
    // Der ZWEITE Emitter derselben Tabelle (W12-A-Stempel, X.Y.Z-Voll-Form). Er ist eine eigene Welt
    // neben der .algos-Signatur -- ein Bump, der nur EINEN der beiden erreicht, waere ein halber Bump.
    std::vector<ex::AxisVariantVersion> const neu = ex::build_axis_variant_version_table();
    std::vector<ex::AxisVariantVersion> const alt = tabelle_mit_alter_allokator_version();

    std::string const variante = erste_aktive_allokator_variante(neu);
    ASSERT_FALSE(variante.empty());

    std::vector<std::pair<std::string, std::string>> const achsen{{"allocator", variante}};
    std::string const zeile_neu = ex::compose_organ_stamp_line(achsen, neu);
    std::string const zeile_alt = ex::compose_organ_stamp_line(achsen, alt);

    EXPECT_NE(zeile_neu, zeile_alt);
    EXPECT_NE(zeile_neu.find("allocator=" + variante + "@1.0.1"), std::string::npos) << zeile_neu;
    EXPECT_NE(zeile_alt.find("allocator=" + variante + "@1.0.0"), std::string::npos) << zeile_alt;
}
