// A1-VERSIONS-PIN der Allokator-Achse (Nachbesserung 2026-08-06, Review-Befund "Bump ohne
// Regressionsorakel"; NACHTRAG 2026-08-06, Lens-Pass + Owner-Entscheid "2. Bump").
//
// DER BEFUND. Der A1-Wurf-Vertrag hat den FEHLSCHLAG-Vertrag der Achse 6 geaendert und deshalb alle 26
// Strategien von "1.0.0.c" auf "1.0.1.c" gebumpt (1. Bump; Begruendung: axis_06_allocator_strategy_
// base.hpp, Abschnitt "A1-VERSIONS-BUMP"). Der Bump selbst war UNBEWACHT: keine Zeile im Bestand haette
// gemerkt, wenn er versehentlich zurueckfiele, wenn eine einzelne Strategie ihn nicht mitmachte, wenn
// eine NEUE Strategie mit dem alten Literal in die Registry kaeme -- oder wenn der Bump gar nicht dort
// ankaeme, wo er wirken soll. Genau das ist die Stale-Green-Wurzel (#50) in ihrer Version-Form.
//
// NACHTRAG (2. BUMP): die reallocate()-Statistik-Symmetrie-Korrektur (Phantom-Bytes) traf 24 der 26
// Strategien -- die mit eigener reallocate()-Implementierung -- und bekam einen ZWEITEN Bump, v1.0.1c
// -> v1.0.2c (Owner-Entscheid nach Lens-Pass: "heute unerreichbar entlastet nicht", volle Begruendung
// axis_06_allocator_strategy_base.hpp, Abschnitt "A1-VERSIONS-BUMP, 2. BUMP"). PmrResourceAllocator und
// VampirNfpAllocator implementieren KEIN reallocate() und bleiben auf v1.0.1c stehen. Die Achse traegt
// damit SEIT DEM 2. BUMP ZWEI Literale gleichzeitig -- diese TU pinnt beide UND die Zuordnung, welche
// Strategie welches traegt (namentlich, nicht als Zahl): ein Bump, der eine der zwei reallocate-losen
// Strategien faelschlich nachzieht ODER eine der 24 faelschlich auslaesst, muss HIER brechen.
//
// WAS DIESE TU PINNT -- in zwei Schichten, weil eine allein nichts taugt:
//
//   (A) DER LITERAL-PIN, compile-hart, 26x einzeln ausgeschrieben. Er ist der Regressions-Faenger:
//       jede Bewegung an einer der 26 algo_version-Zeilen macht den BAU rot und zwingt zum
//       ABSICHTLICHEN Nachziehen dieser Datei. Ausgeschrieben statt ueber die Registry gefaltet --
//       ein Fold ueber mp_for_each faenge zwar dieselben Faelle, benennt beim Bruch aber nicht die
//       Strategie. Dazu die Registry-Seite: GENAU 26 Vendor, und JEDE traegt genau EINES der beiden
//       Literale je nach reallocate-Zugehoerigkeit (das faengt die 27. Strategie, die mit "1.0.0.c"
//       hereinkaeme und in der Namensliste unten fehlte, UND eine Strategie, die die FALSCHE der
//       beiden Versionen traegt).
//
//   (B) DER WIRKUNGS-PIN (Alt/Neu-Kontrast). Ein Literal-Pin allein beweist nur, dass eine Zeichenkette
//       im Quelltext steht. Die tragende Frage ist, ob der Bump die FLAECHE erreicht, an der der
//       Rebuild-/Neu-Mess-Selektor entscheidet. Diese Flaeche ist die algo_sig aus
//       compose_algo_signature (.algos-Sidecar; sie speist ueber abi::kSubAxisValuesetSegment auch das
//       Fingerprint-Preimage, an dem dll_is_current skippt). Der Kontrast baut DIESELBE Signatur einmal
//       mit der heutigen Tabelle und einmal mit der auf die JEWEILS VORHERGEHENDE Version zurueckgedrehten
//       Allokator-Zeile (v1.0.2c -> v1.0.1c fuer die 24; v1.0.1c -> v1.0.0c fuer die zwei reallocate-
//       losen) und verlangt, dass sich die Signatur BEWEGT -- das ist der jeweils AKTUELLSTE Bump-Schritt
//       der geprueften Variante, nicht eine veraltete Referenz auf den 1. Bump. Bewegte sie sich nicht,
//       waere der Bump wirkungslos -- und der Literal-Pin oben eine Beruhigungspille.
//
// TABU-NEUTRAL: liest ausschliesslich; keine Registry-XML, kein Fixture, kein Fingerprint-Anker wird
// beruehrt. Der Kontrast arbeitet auf einer LOKALEN Kopie der Versions-Tabelle.
// ASCII-only.

#include <axes/alloc/axis_06_allocator_registry.hpp>
#include <builder/experiment_tree/axis_variant_version_table.hpp>
#include <cache_engine/measurement/algo_semver.hpp>

#include <boost/mp11.hpp>
#include <gtest/gtest.h>

#include <array>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace al   = ::comdare::cache_engine::alloc;
namespace ex   = ::comdare::cache_engine::builder::experiment;
namespace meas = ::comdare::cache_engine::measurement;

namespace {

/// Das Literal des 2. Bumps -- die 24 Strategien mit eigener reallocate()-Implementierung.
constexpr std::string_view kA1AllocatorVersionV2 = "1.0.2.c";

/// Das Literal des 1. Bumps -- unveraendert fuer die zwei reallocate-losen Strategien (s. Datei-Kopf).
constexpr std::string_view kA1AllocatorVersionV1 = "1.0.1.c";

/// Der Stand VOR dem A1-Schnitt ueberhaupt. Nur fuer den Kontrast in (B); er steht in keiner Quelle mehr.
constexpr std::string_view kVorA1AllocatorVersion = "1.0.0.c";

/// Die zwei Strategien OHNE eigenes reallocate() (PMR-Interface bietet das nicht direkt; VampirNfpAllocator
/// dito) -- sie bekamen den 2. Bump NICHT und stehen weiterhin auf kA1AllocatorVersionV1. Namentlich
/// gefuehrt, nicht als Zahl: eine dritte reallocate-lose Strategie, die spaeter dazukommt, faellt hier
/// NICHT automatisch heraus (sie muesste diese Liste explizit ergaenzen) -- das ist Absicht, kein Leck:
/// "kein reallocate" ist eine Eigenschaft, die pruefbar sein soll, nicht geraten werden darf.
constexpr std::array<std::string_view, 2> kReallocateLoseStrategien{"pmr_resource", "vampir_nfp"};

[[nodiscard]] constexpr bool ist_reallocate_lose_strategie(std::string_view name) noexcept {
    for (std::string_view const& n : kReallocateLoseStrategien)
        if (n == name) return true;
    return false;
}

/// Die je Strategie ERWARTETE Version -- die eine Stelle, aus der sich sowohl der Registry-Fold als auch
/// der Wirkungs-Pin unten speisen. Eine neue Strategie ohne Eintrag in kReallocateLoseStrategien erwartet
/// automatisch kA1AllocatorVersionV2 (den 2. Bump) -- fail-closed in die Richtung, die eine vergessene
/// Ausnahme sofort sichtbar macht (rot statt still uebernommen).
[[nodiscard]] constexpr std::string_view erwartete_allocator_version(std::string_view name) noexcept {
    return ist_reallocate_lose_strategie(name) ? kA1AllocatorVersionV1 : kA1AllocatorVersionV2;
}

} // namespace

// -------------------------------------------------------------------------------------------------
// (A) LITERAL-PIN -- 26x compile-hart, in der Reihenfolge der Registry (AllVendors, Batch 1-8 + Pool
//     + VAMPIR). Ein vergessener, halber oder falsch zugeordneter Bump bricht HIER, mit dem Namen der
//     Strategie. 24x kA1AllocatorVersionV2, 2x kA1AllocatorVersionV1 (PmrResourceAllocator,
//     VampirNfpAllocator -- kein eigenes reallocate()).
// -------------------------------------------------------------------------------------------------
static_assert(al::StdMalloc::algo_version == "1.0.2.c");
static_assert(al::MimallocAllocator::algo_version == "1.0.2.c");
static_assert(al::SnmallocAllocator::algo_version == "1.0.2.c");
static_assert(al::PmrResourceAllocator::algo_version == "1.0.1.c", "kein eigenes reallocate() -- kein 2. Bump");
static_assert(al::JemallocAllocator::algo_version == "1.0.2.c");
static_assert(al::TCMallocAllocator::algo_version == "1.0.2.c");
static_assert(al::DlmallocAllocator::algo_version == "1.0.2.c");
static_assert(al::HoardAllocator::algo_version == "1.0.2.c");
static_assert(al::SlabAllocator::algo_version == "1.0.2.c");
static_assert(al::MichaelLockFreeAllocator::algo_version == "1.0.2.c");
static_assert(al::ScallocAllocator::algo_version == "1.0.2.c");
static_assert(al::NUMAllocAllocator::algo_version == "1.0.2.c");
static_assert(al::RPMallocAllocator::algo_version == "1.0.2.c");
static_assert(al::LRMallocAllocator::algo_version == "1.0.2.c");
static_assert(al::CAMAAllocator::algo_version == "1.0.2.c");
static_assert(al::StarMallocAllocator::algo_version == "1.0.2.c");
static_assert(al::TCMallocWarehouseAllocator::algo_version == "1.0.2.c");
static_assert(al::HMallocAllocator::algo_version == "1.0.2.c");
static_assert(al::PIMMallocAllocator::algo_version == "1.0.2.c");
static_assert(al::CrystallineAllocator::algo_version == "1.0.2.c");
static_assert(al::ExgenAllocator::algo_version == "1.0.2.c");
static_assert(al::BuddyAllocator::algo_version == "1.0.2.c");
static_assert(al::PtMalloc2Allocator::algo_version == "1.0.2.c");
static_assert(al::VmemMagazinesAllocator::algo_version == "1.0.2.c");
static_assert(al::PoolResourceAllocator::algo_version == "1.0.2.c");
static_assert(al::VampirNfpAllocator::algo_version == "1.0.1.c", "kein eigenes reallocate() -- kein 2. Bump");

/// Die Registry-Seite desselben Pins: GENAU 26 Vendor. Waechst die Liste, ist die Namensliste oben
/// unvollstaendig -- und der naechste Satz faengt, was ihr fehlt.
static_assert(boost::mp11::mp_size<al::AllVendors>::value == 26);

TEST(A1AlgoVersionPinAllocAxis, JedeRegistrierteStrategieTraegtDieFuerSieErwarteteVersion) {
    // Deckt die Luecke der Namensliste: eine NEUE Strategie, die mit "1.0.0.c" (oder irgendetwas
    // anderem als der fuer sie erwarteten Version) in AllVendors kaeme, faellt hier auf -- auch wenn
    // niemand die 26 Zeilen oben ergaenzt. erwartete_allocator_version() ist DIESELBE Zuordnung wie in
    // (A) und im Wirkungs-Pin unten -- eine Stelle, kein zweites Namens-Literal.
    std::vector<std::string> abweichler;
    boost::mp11::mp_for_each<boost::mp11::mp_transform<boost::mp11::mp_identity, al::AllVendors>>([&](auto id) {
        using W                     = typename decltype(id)::type;
        std::string_view const soll = erwartete_allocator_version(W::name());
        if (std::string_view{W::algo_version} != soll)
            abweichler.emplace_back(std::string{W::name()} + "=" + std::string{W::algo_version} + " (erwartet " +
                                    std::string{soll} + ")");
    });
    EXPECT_TRUE(abweichler.empty()) << "Strategien der Achse 6 mit abweichender algo_version: " << [&] {
        std::string s;
        for (std::string const& a : abweichler) s += a + " ";
        return s;
    }();
    EXPECT_EQ(boost::mp11::mp_size<al::AllVendors>::value, 26u);
}

TEST(A1AlgoVersionPinAllocAxis, BeideVersionsLiteraleSindGrammatikalischWohlgeformt) {
    // Der Pin darf keine Fehlform zementieren: BEIDE Literale muessen die FLAG-GRAMMATIK v2 erfuellen
    // (dreistellig, Punkt vor jedem Flag, CPU-Basis 'c' darunter -- Owner-KERN 07.08.2026 / F-10).
    meas::AlgoSemVer const v2 = meas::parse_algo_semver(kA1AllocatorVersionV2);
    EXPECT_FALSE(v2.is_sentinel());
    EXPECT_EQ(v2.x, 1u);
    EXPECT_EQ(v2.y, 0u);
    EXPECT_EQ(v2.z, 2u) << "PATCH-Stelle, 2. Bump: die reallocate-Statistik-Korrektur.";
    EXPECT_TRUE(v2.has_top_level_flag("c"));
    EXPECT_EQ(v2.flags.count, 1u) << "der Bestand traegt heute GENAU die CPU-Basis, ohne Sub-Flags.";
    EXPECT_TRUE(meas::ce_owned_version_is_wellformed(kA1AllocatorVersionV2));
    EXPECT_TRUE(meas::ce_owned_version_satisfies_cpu_enforce(kA1AllocatorVersionV2));

    meas::AlgoSemVer const v1 = meas::parse_algo_semver(kA1AllocatorVersionV1);
    EXPECT_FALSE(v1.is_sentinel());
    EXPECT_EQ(v1.z, 1u) << "PATCH-Stelle, 1. Bump: der Wurf-Vertrag.";
    EXPECT_TRUE(v1.has_top_level_flag("c"));
    EXPECT_EQ(v1.flags.count, 1u);
    EXPECT_TRUE(meas::ce_owned_version_is_wellformed(kA1AllocatorVersionV1));
    EXPECT_TRUE(meas::ce_owned_version_satisfies_cpu_enforce(kA1AllocatorVersionV1));

    // Und die drei Staende sind PAARWEISE ECHT verschieden -- sonst waere der Kontrast unten gegenstandslos.
    EXPECT_NE(kA1AllocatorVersionV2, kA1AllocatorVersionV1);
    EXPECT_NE(kA1AllocatorVersionV1, kVorA1AllocatorVersion);
    EXPECT_NE(kA1AllocatorVersionV2, kVorA1AllocatorVersion);
    meas::AlgoSemVer const alt = meas::parse_algo_semver(kVorA1AllocatorVersion);
    EXPECT_EQ(alt.z, 0u);
}

TEST(A1AlgoVersionPinAllocAxis, DieEnabledTabelleFuehrtJedenAllokatorSlotAufDerFuerIhnErwartetenVersion) {
    std::vector<ex::AxisVariantVersion> const tabelle = ex::build_axis_variant_version_table();
    ASSERT_FALSE(tabelle.empty());

    std::size_t allokator_zeilen = 0;
    for (ex::AxisVariantVersion const& e : tabelle) {
        if (e.axis != "allocator") continue;
        ++allokator_zeilen;
        EXPECT_EQ(e.version, erwartete_allocator_version(e.variant))
            << "Allokator-Variante '" << e.variant << "' steht nicht auf der fuer sie erwarteten Version.";
    }
    EXPECT_GT(allokator_zeilen, 0u) << "ohne aktivierte Allokator-Variante ist der Pin gegenstandslos.";
}

// -------------------------------------------------------------------------------------------------
// (B) WIRKUNGS-PIN -- der Alt/Neu-Kontrast auf der Flaeche, an der der Cache entscheidet. Rollt JEDE
//     Allokator-Zeile um GENAU EINEN Bump-Schritt zurueck (v1.0.2c -> v1.0.1c bzw. v1.0.1c -> v1.0.0c) --
//     der jeweils AKTUELLSTE Schritt der betroffenen Variante, nicht eine feste, ggf. veraltete Referenz.
// -------------------------------------------------------------------------------------------------

namespace {

/// Die Tabelle mit dem Allokator-Slot um GENAU EINEN Bump-Schritt zurueckgedreht. LOKALE Kopie -- es
/// wird nichts im Bestand veraendert.
[[nodiscard]] std::vector<ex::AxisVariantVersion> tabelle_mit_vorherigem_allokator_bump_schritt() {
    std::vector<ex::AxisVariantVersion> t = ex::build_axis_variant_version_table();
    for (ex::AxisVariantVersion& e : t) {
        if (e.axis != "allocator") continue;
        if (e.version == std::string{kA1AllocatorVersionV2})
            e.version = std::string{kA1AllocatorVersionV1};
        else if (e.version == std::string{kA1AllocatorVersionV1})
            e.version = std::string{kVorA1AllocatorVersion};
        // Jeder andere Wert (Registry-Drift) bleibt unangetastet -- (A) und die vorige TEST() fangen ihn
        // bereits; dieser Kontrast soll nicht zusaetzlich raten.
    }
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
    std::vector<ex::AxisVariantVersion> const alt = tabelle_mit_vorherigem_allokator_bump_schritt();

    std::string const variante = erste_aktive_allokator_variante(neu);
    ASSERT_FALSE(variante.empty());
    std::string_view const version_neu = erwartete_allocator_version(variante);
    std::string_view const version_alt =
        ist_reallocate_lose_strategie(variante) ? kVorA1AllocatorVersion : kA1AllocatorVersionV1;

    std::vector<std::pair<std::string, std::string>> const achsen{{"allocator", variante}};
    std::string const                                      sig_neu = ex::compose_algo_signature(achsen, neu);
    std::string const                                      sig_alt = ex::compose_algo_signature(achsen, alt);

    // DIE tragende Aussage: der Bump erreicht die .algos-Signatur. Waeren beide gleich, wuerde der
    // Rebuild-/Neu-Mess-Selektor die vor dem jeweiligen Bump gebaute Binary weiterverwenden.
    EXPECT_NE(sig_neu, sig_alt) << "der Versions-Bump ist auf der algo_sig UNSICHTBAR -- genau das war "
                                   "der Grund, warum der Schnitt ueberhaupt gebumpt hat.";
    EXPECT_NE(sig_neu.find("allocator=" + variante + "@" + std::string{version_neu}), std::string::npos) << sig_neu;
    EXPECT_NE(sig_alt.find("allocator=" + variante + "@" + std::string{version_alt}), std::string::npos) << sig_alt;
    EXPECT_EQ(sig_neu.find("@v0.0.0"), std::string::npos)
        << "Sentinel in der Signatur: der Slot wurde nicht in der Tabelle gefunden -- dann pruefte der "
           "Kontrast oben nichts.";
}

TEST(A1AlgoVersionPinAllocAxis, BumpBewegtDieOrganStempelZeile) {
    // Der ZWEITE Emitter derselben Tabelle (W12-A-Stempel, X.Y.Z-Voll-Form). Er ist eine eigene Welt
    // neben der .algos-Signatur -- ein Bump, der nur EINEN der beiden erreicht, waere ein halber Bump.
    std::vector<ex::AxisVariantVersion> const neu = ex::build_axis_variant_version_table();
    std::vector<ex::AxisVariantVersion> const alt = tabelle_mit_vorherigem_allokator_bump_schritt();

    std::string const variante = erste_aktive_allokator_variante(neu);
    ASSERT_FALSE(variante.empty());
    // X.Y.Z-Voll-Form (ohne Flag-Suffix) der jeweils zwei Staende:
    std::string const zyz_neu = ist_reallocate_lose_strategie(variante) ? "1.0.1" : "1.0.2";
    std::string const zyz_alt = ist_reallocate_lose_strategie(variante) ? "1.0.0" : "1.0.1";

    std::vector<std::pair<std::string, std::string>> const achsen{{"allocator", variante}};
    std::string const                                      zeile_neu = ex::compose_organ_stamp_line(achsen, neu);
    std::string const                                      zeile_alt = ex::compose_organ_stamp_line(achsen, alt);

    EXPECT_NE(zeile_neu, zeile_alt);
    EXPECT_NE(zeile_neu.find("allocator=" + variante + "@" + zyz_neu), std::string::npos) << zeile_neu;
    EXPECT_NE(zeile_alt.find("allocator=" + variante + "@" + zyz_alt), std::string::npos) << zeile_alt;
}
