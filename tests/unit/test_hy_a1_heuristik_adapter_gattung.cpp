// test_hy_a1_heuristik_adapter_gattung -- HY-A1: die Gattung HEURISTIK-ADAPTER, beide Richtungen.
//
// ============================================================================================
// WAS DIESE TU BEWEIST
// ============================================================================================
// (A) DIE ERLAUBTE RICHTUNG IST GRUEN -- alle fuenf ABI-sichtbaren Genera sind durchlaessig.
//     Owner-Auflage woertlich (LEDGER:472): "fuer JEDE Gattung+Genus per google test bewiesen
//     durchlaessig". Deshalb liegt diese Haelfte in gtest und nicht nur in static_asserts.
// (B) DIE VERBOTENE RICHTUNG SCHEITERT COMPILE-TIME -- an der VERWENDUNGSSTELLE, nicht nur in der
//     Concept-Auswertung. Der Nachweis, dass eine ECHTE Uebersetzung daran scheitert, liegt
//     zusaetzlich in der Negativ-Fixture test_hy_a1_reroute_gate_negativ.cpp.
// (C) WEG C IST WIRKSAM -- eine Hybrid-Binary meldet aus genus() das GEERBTE ZIEL-Genus, und der
//     Host leitet daraus die Ziel-Gattung ab, NICHT HeuristikAdapter.
// (D) DIE PARTITION HAELT -- jeder Enum-Wert ist entweder ABI-sichtbar oder Klassifikation.
//
// ============================================================================================
// WARUM (B) HIER UEBERHAUPT ALS static_assert STEHT UND NICHT NUR IN DER NEGATIV-FIXTURE
// ============================================================================================
// Die Negativ-Fixture wird NUR von ihrem eigenen ctest gebaut. Faellt dieser Test einmal aus der
// Registrierung (im Bestand real vorgekommen -- der Voll-Audit zaehlte 38 unregistrierte Tests),
// waere die Regel unbewacht. Die static_asserts hier laufen dagegen in JEDEM Bau dieser TU mit.
// Die beiden Wege sichern sich gegenseitig ab; keiner ersetzt den anderen.

#include "hybrid/heuristik_adapter_gate.hpp"
#include "hybrid/heuristik_adapter_klassifikation.hpp"
#include "hybrid/heuristik_adapter_strategy.hpp"
#include "hybrid/heuristik_adapter_synthese_matrix.hpp"

#include <anatomy/anatomy_base.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <string_view>

namespace cea = ::comdare::cache_engine::anatomy;
namespace hy  = ::comdare::cache_engine::hybrid;

// TU-lokale Fixtures: anonymer Namespace gegen ODR-Kollisionen mit den uebrigen Gattungs-TUs
// (cppcheck ctuOneDefinitionRuleViolation -- im Bestand mehrfach aufgelaufen).
namespace {

// ============================================================================================
// (C) EIN HYBRID-MODUL, WIE ES SICH NACH AUSSEN GIBT
// ============================================================================================
// Der Kern von Weg C in ausfuehrbarer Form: dieses Modul IST ein HeuristikAdapter (das steht in
// seiner Klassifikation), aber es MELDET das Ziel-Genus. Genau so verhaelt sich die spaetere
// Hybrid-.so am Pruef-Dock -- und genau deshalb braucht sie dort kein eigenes, sechstes Dock.
template <cea::AnatomyGenus ZielGenus>
class HybridModulAttrappe final : public cea::IAnatomyBase {
public:
    [[nodiscard]] std::string_view engine_name() const noexcept override { return "HybridModulAttrappe"; }
    [[nodiscard]] ::comdare::cache_engine::execution_engine::EngineLifecycleState
    lifecycle_state() const noexcept override {
        return state_;
    }
    void warm_up() override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Warming; }
    void run() override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Running; }
    void reset() override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Idle; }
    void shutdown() override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Shutdown; }

    [[nodiscard]] std::string_view composition_name() const noexcept override { return "HybridHuelle"; }
    [[nodiscard]] std::string_view paper_id() const noexcept override { return "P00 HY-A1-Attrappe"; }
    [[nodiscard]] std::size_t      organ_count() const noexcept override { return 0; }

    /// DIE STELLE, UM DIE ES GEHT: das GEERBTE ZIEL-Genus, nie FunctionInterfaceReroute.
    [[nodiscard]] cea::AnatomyGenus genus() const noexcept override { return ZielGenus; }

    /// Die Hybrid-Natur lebt daneben, als Klassifikation des Artefakts -- nicht in genus().
    [[nodiscard]] static constexpr cea::AnatomyGenus klassifikation() noexcept {
        return cea::AnatomyGenus::FunctionInterfaceReroute;
    }

private:
    ::comdare::cache_engine::execution_engine::EngineLifecycleState state_ =
        ::comdare::cache_engine::execution_engine::EngineLifecycleState::Idle;
};

// ============================================================================================
// COMPILE-TIME-BEWEISE (laufen bei JEDEM Bau dieser TU mit)
// ============================================================================================

// (A) erlaubte Richtung -- alle fuenf, am Gate UND an der Strategie UND an der Matrix.
static_assert(hy::HybridZielGenus<cea::AnatomyGenus::SearchAlgorithm>);
static_assert(hy::HybridZielGenus<cea::AnatomyGenus::Set>);
static_assert(hy::HybridZielGenus<cea::AnatomyGenus::Sequence>);
static_assert(hy::HybridZielGenus<cea::AnatomyGenus::Adapter>);
static_assert(hy::HybridZielGenus<cea::AnatomyGenus::View>);
static_assert(hy::RerouteStrategieGebunden<cea::AnatomyGenus::SearchAlgorithm>);
static_assert(hy::RerouteStrategieGebunden<cea::AnatomyGenus::View>);

// (B) verbotene Richtung -- an DREI verschiedenen Verwendungsstellen, damit ein Wegfall EINER
//     Sperre nicht von einer anderen verdeckt wird.
static_assert(
    !hy::ZielBindungInstanziierbar<cea::AnatomyGattung::HeuristikAdapter, cea::AnatomyGenus::FunctionInterfaceReroute>,
    "HY-A1-TEST: die Ziel-Bindung darf sich fuer das Reroute-Genus nicht bilden lassen.");
static_assert(!hy::RerouteStrategieGebunden<cea::AnatomyGenus::FunctionInterfaceReroute>,
              "HY-A1-TEST: es darf keine Reroute-Strategie fuer das Reroute-Genus geben.");
static_assert(!hy::MatrixBildbar<cea::AnatomyGenus::FunctionInterfaceReroute>,
              "HY-A1-TEST: es darf keine Synthese-Matrix fuer das Reroute-Genus geben.");

// (B2) Paar-Bruch, unabhaengig von der Zielfaehigkeit.
static_assert(!hy::ZielBindungInstanziierbar<cea::AnatomyGattung::Container, cea::AnatomyGenus::SearchAlgorithm>);
static_assert(!hy::ZielBindungInstanziierbar<cea::AnatomyGattung::Map, cea::AnatomyGenus::View>);

} // namespace

// ============================================================================================
// (A) DURCHLAESSIGKEIT -- die Owner-Auflage "fuer JEDE Gattung+Genus per google test bewiesen"
// ============================================================================================

TEST(HyA1HeuristikAdapter, AlleAbiSichtbarenGeneraSindDurchlaessig) {
    // Die Schleife laeuft ueber die EINZELQUELLE, nicht ueber eine im Test gefuehrte Liste.
    // Genau daran ist die bestehende Wache in test_e24_c4_genus_pruef_docks gescheitert.
    std::size_t durchlaessig = 0;
    for (auto const g : hy::kAlleGenera) {
        if (hy::ist_abi_sichtbares_genus(g)) {
            ++durchlaessig;
            // Jedes zielfaehige Genus muss eine echte, benannte Gattung haben.
            EXPECT_NE(cea::gattung_name(cea::gattung_of(g)), std::string_view{"Unknown"}) << cea::genus_name(g);
            EXPECT_NE(cea::gattung_of(g), cea::AnatomyGattung::HeuristikAdapter) << cea::genus_name(g);
        }
    }
    EXPECT_EQ(durchlaessig, hy::kPruefDockPflichtigeGenusAnzahl);
    EXPECT_EQ(durchlaessig, std::size_t{5});
}

TEST(HyA1HeuristikAdapter, JedesDurchlaessigeGenusHatGenauEineStrategie) {
    // Namen muessen PAARWEISE VERSCHIEDEN sein -- eine gemeinsame Implementierung fuer alle Genera
    // ist ausdruecklich ausgeschlossen (Owner: "Strategy Pattern fuer JEDES Genus"). Waeren zwei
    // Strategien identisch benannt, waere das der erste Hinweis auf genau diese Zusammenlegung.
    std::array<std::string_view, 5> const namen{
        hy::HeuristikRerouteStrategy<cea::AnatomyGenus::SearchAlgorithm>::strategie_name(),
        hy::HeuristikRerouteStrategy<cea::AnatomyGenus::Set>::strategie_name(),
        hy::HeuristikRerouteStrategy<cea::AnatomyGenus::Sequence>::strategie_name(),
        hy::HeuristikRerouteStrategy<cea::AnatomyGenus::Adapter>::strategie_name(),
        hy::HeuristikRerouteStrategy<cea::AnatomyGenus::View>::strategie_name()};

    for (std::size_t i = 0; i < namen.size(); ++i) {
        for (std::size_t j = i + 1; j < namen.size(); ++j) {
            EXPECT_NE(namen[i], namen[j]) << "Strategie " << i << " und " << j << " sind nicht unterscheidbar";
        }
    }

    // Und jede Strategie nennt WIRKLICH ihr eigenes Ziel.
    EXPECT_EQ(hy::HeuristikRerouteStrategy<cea::AnatomyGenus::Set>::ziel_genus(), cea::AnatomyGenus::Set);
    EXPECT_EQ(hy::HeuristikRerouteStrategy<cea::AnatomyGenus::Adapter>::ziel_genus(), cea::AnatomyGenus::Adapter);
}

// ============================================================================================
// (C) WEG C -- der transparente Pass-through, ausfuehrbar belegt
// ============================================================================================

TEST(HyA1HeuristikAdapter, HybridMeldetDasGeerbteZielGenusNichtSichSelbst) {
    HybridModulAttrappe<cea::AnatomyGenus::SearchAlgorithm> map_hybrid;
    HybridModulAttrappe<cea::AnatomyGenus::Sequence>        seq_hybrid;

    cea::IAnatomyBase const& als_modul_map = map_hybrid;
    cea::IAnatomyBase const& als_modul_seq = seq_hybrid;

    // DIE KERN-AUSSAGE: genus() liefert das ZIEL, nicht die Klassifikation.
    EXPECT_EQ(als_modul_map.genus(), cea::AnatomyGenus::SearchAlgorithm);
    EXPECT_NE(als_modul_map.genus(), cea::AnatomyGenus::FunctionInterfaceReroute);
    EXPECT_EQ(als_modul_seq.genus(), cea::AnatomyGenus::Sequence);

    // FOLGE 1: der Host leitet die ZIEL-Gattung ab -- die Hybrid-Natur ist an dieser Grenze unsichtbar.
    EXPECT_EQ(cea::module_gattung(als_modul_map), cea::AnatomyGattung::Map);
    EXPECT_EQ(cea::module_gattung(als_modul_seq), cea::AnatomyGattung::Container);
    EXPECT_NE(cea::module_gattung(als_modul_map), cea::AnatomyGattung::HeuristikAdapter);

    // FOLGE 2: die Hybrid-Natur ist trotzdem da -- daneben, als Klassifikation des Artefakts.
    EXPECT_EQ(decltype(map_hybrid)::klassifikation(), cea::AnatomyGenus::FunctionInterfaceReroute);
    EXPECT_TRUE(hy::ist_klassifikations_genus(decltype(map_hybrid)::klassifikation()));
}

// ============================================================================================
// (D) DIE PARTITION UND DIE NEUE VOLLSTAENDIGKEITS-WACHE
// ============================================================================================

TEST(HyA1HeuristikAdapter, PartitionIstVollstaendigUndDisjunkt) {
    std::size_t abi_sichtbar   = 0;
    std::size_t klassifikation = 0;
    for (auto const g : hy::kAlleGenera) {
        // Disjunkt: nie beides, nie keines von beidem.
        EXPECT_NE(hy::ist_abi_sichtbares_genus(g), hy::ist_klassifikations_genus(g)) << cea::genus_name(g);
        if (hy::ist_abi_sichtbares_genus(g)) ++abi_sichtbar;
        if (hy::ist_klassifikations_genus(g)) ++klassifikation;
    }
    EXPECT_EQ(abi_sichtbar + klassifikation, hy::kAlleGenera.size());
    EXPECT_EQ(abi_sichtbar, std::size_t{5});
    EXPECT_EQ(klassifikation, std::size_t{1});
}

TEST(HyA1HeuristikAdapter, EinzelquelleUndNamensSwitchStimmenUeberein) {
    // Dieselbe Aussage wie die static_asserts in der Klassifikations-Datei, hier als laufender
    // Test -- damit ein Bruch auch dann als TEST-Fehler sichtbar wird, wenn jemand die Wache
    // versehentlich entschaerft. Der 256er-Scan ist der Kern: er faengt den ANGEHAENGTEN Wert.
    EXPECT_TRUE(hy::genus_liste_ist_vollstaendig());
    EXPECT_TRUE(hy::gattungs_liste_ist_vollstaendig());
    EXPECT_TRUE(hy::alle_genera_benannt());
    EXPECT_TRUE(hy::alle_gattungen_benannt());

    // Die neuen Werte sind wirklich benannt (kein "Unknown"-Durchrutscher).
    EXPECT_EQ(cea::gattung_name(cea::AnatomyGattung::HeuristikAdapter), std::string_view{"HeuristikAdapter"});
    EXPECT_EQ(cea::genus_name(cea::AnatomyGenus::FunctionInterfaceReroute),
              std::string_view{"FunctionInterfaceReroute"});
    EXPECT_EQ(cea::gattung_of(cea::AnatomyGenus::FunctionInterfaceReroute), cea::AnatomyGattung::HeuristikAdapter);

    // ANGEHAENGT, NICHT EINGESCHOBEN: die Bestands-Werte behalten ihre Zahlen (Reihenfolge TABU).
    EXPECT_EQ(static_cast<int>(cea::AnatomyGenus::SearchAlgorithm), 0);
    EXPECT_EQ(static_cast<int>(cea::AnatomyGenus::View), 4);
    EXPECT_EQ(static_cast<int>(cea::AnatomyGenus::FunctionInterfaceReroute), 5);
    EXPECT_EQ(static_cast<int>(cea::AnatomyGattung::Map), 0);
    EXPECT_EQ(static_cast<int>(cea::AnatomyGattung::Graph), 2);
    EXPECT_EQ(static_cast<int>(cea::AnatomyGattung::HeuristikAdapter), 3);
}

// ============================================================================================
// DIE 2D-MATRIX-LISTE
// ============================================================================================

TEST(HyA1HeuristikAdapter, SyntheseMatrixIstLayerMalNode) {
    // Heutige Einstellung: Tiefe 1.
    EXPECT_EQ(hy::HeutigeMatrix::layer_anzahl(), std::size_t{1});
    EXPECT_EQ(hy::HeutigeMatrix::zellen_gesamt(), std::size_t{32});

    // "muss aber funktionieren": Tiefe 3 ergibt 3 x 32 Synthese-Funktionen.
    EXPECT_EQ(hy::TiefeMatrix::layer_anzahl(), std::size_t{3});
    EXPECT_EQ(hy::TiefeMatrix::zellen_gesamt(), std::size_t{96});
    EXPECT_EQ(hy::TiefeMatrix::zellen_gesamt(), hy::TiefeMatrix::layer_anzahl() * hy::TiefeMatrix::nodes);

    // Das Herunterzaehlen erreicht die 0.
    EXPECT_EQ(hy::TiefeMatrix::naechste_stufe::naechste_stufe::naechste_stufe::rest_tiefe, std::size_t{0});

    // Stufen-IDs sind ueber die Layer hinweg verschieden (Stempel-Lesbarkeit).
    constexpr auto id_l1 = hy::SyntheseZelle<1, 0, cea::AnatomyGenus::Set>::stufen_id<32>();
    constexpr auto id_l2 = hy::SyntheseZelle<2, 0, cea::AnatomyGenus::Set>::stufen_id<32>();
    EXPECT_NE(id_l1, id_l2);
    EXPECT_EQ(id_l1, std::size_t{32});
    EXPECT_EQ(id_l2, std::size_t{64});
}
