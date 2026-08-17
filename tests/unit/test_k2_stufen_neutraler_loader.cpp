// test_k2_stufen_neutraler_loader -- K2: der Loader und das Antriebs-Buendel sind STUFEN-NEUTRAL.
//
// ============================================================================================
// DER OWNER-ENTSCHEID, WOERTLICH (09.08.2026)
// ============================================================================================
//   "der Loader wandert in eine stufen-neutrale Bibliothek, weil das Pruefdock der CEB und das
//    Pruefdock der Hybrid-Tier-Binary jeweils technisch identisch bei Konfiguration sein muessen."
//
// Das beantwortet die Auflage K2 des SOLL-Designs (Abschnitt 7) mit OPTION (a) -- Extraktion, nicht
// "die Hybrid-Stufe linkt bewusst die Builder-Lib". Das Design-Dokument vom 02.08. fuehrt K2 noch
// als offen; es ist schlicht nicht nachgezogen (Marker-Nachzugs-Klasse, im Dokument selbst
// datiert ergaenzt).
//
// ============================================================================================
// WAS DIESE TU BEWEIST -- UND WARUM SIE NICHT DURCH EINEN static_assert ERSETZBAR IST
// ============================================================================================
// Die Aussage "stufen-neutral" heisst: MAN KANN DIE SCHICHT BENUTZEN, OHNE DEN BUILDER ZU ZIEHEN.
// Das ist eine Aussage ueber die INCLUDE- und LINK-Kante, nicht ueber Typen. Ein static_assert in
// einer TU, die den Builder ohnehin inkludiert, kann sie nicht treffen -- er liefe im falschen
// Kontext und waere gruen, ohne etwas zu decken.
//
// DER BEWEIS IST DESHALB DIE TU SELBST: sie inkludiert AUSSCHLIESSLICH anatomy/ und die neue
// stufen-neutrale Schicht. Kein builder/pruef_dock/, kein IPruefDock, kein Dock-Registry. Dass sie
// uebersetzt UND linkt, IST die Aussage. Waere der Drive weiterhin an den Builder gebunden, braeche
// schon der Include.
//
// GEGENPROBE gegen ein wertloses Gruen: die Schicht wird hier wirklich BENUTZT -- ein Handle wird
// konstruiert, der Drive daran gezogen, die Status-Codes werden gelesen. Eine TU, die nur
// inkludiert und nichts instanziiert, wuerde eine kaputte Schicht nicht bemerken.
//
// WAS SIE NICHT BEWEIST, ausdruecklich: dass eine Hybrid-.so real ein Modul laedt. Dafuer braucht
// es den Binary-Proxy und ein gebautes Tier -- das ist HY-A2. Bewiesen ist die SCHICHTUNG, nicht
// der Reroute.

#include <anatomy_drive/search_algorithm_drive.hpp>
#include <anatomy_drive/tier_drive_status.hpp>

#include <anatomy/anatomy_base.hpp>
#include <anatomy/observable_tier.hpp>
#include <builder/anatomy_module_loader/anatomy_module_loader.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace ad  = ::comdare::cache_engine::anatomy_drive;
namespace cea = ::comdare::cache_engine::anatomy;
namespace al  = ::comdare::cache_engine::builder::anatomy_loader;

namespace {

/// Eine minimale Tier-Attrappe, gebaut wie ein ECHTES Tier-Modul: sie erbt IAnatomyBase (das ist
/// die Identitaets-Flaeche, die der Loader im Handle haelt) UND IObservableTier (der Mess-Antrieb).
///
/// DASS DAS ZWEI GETRENNTE BASEN SIND, ist am Bau aufgelaufen und deshalb hier festgehalten:
/// IObservableTier erbt NICHT von IAnatomyBase, sondern von IDriveableTier. Eine Attrappe mit nur
/// dem Antriebs-Interface ist kein gueltiges Handle-Argument. Genau darum ist das dynamic_cast in
/// acquire_search_algorithm_drive ein CROSS-Cast (Basis -> Geschwister-Interface) und nicht ein
/// simpler Abstieg -- er gelingt nur, wenn das Objekt wirklich beide Aeste traegt.
///
/// Die optionalen Sub-Antriebe (ctrl/rbk/scn) traegt sie bewusst NICHT: das ist der Fall
/// "alte DLL", den acquire_* mit nullptr statt Absturz beantworten muss.
class NurObservableTier final : public cea::IAnatomyBase, public cea::IObservableTier {
public:
    // -- IAnatomyBase: die Identitaets-Flaeche --
    [[nodiscard]] std::string_view engine_name() const noexcept override { return "K2Attrappe"; }
    [[nodiscard]] ::comdare::cache_engine::execution_engine::EngineLifecycleState
    lifecycle_state() const noexcept override {
        return state_;
    }
    void warm_up() override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Warming; }
    void run() override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Running; }
    void reset() override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Idle; }
    void shutdown() override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Shutdown; }
    [[nodiscard]] std::string_view  composition_name() const noexcept override { return "K2Huelle"; }
    [[nodiscard]] std::string_view  paper_id() const noexcept override { return "P00 K2-Attrappe"; }
    [[nodiscard]] std::size_t       organ_count() const noexcept override { return 0; }
    [[nodiscard]] cea::AnatomyGenus genus() const noexcept override { return cea::AnatomyGenus::SearchAlgorithm; }

    // -- IObservableTier: der Mess-Antrieb --
    [[nodiscard]] bool tier_insert(std::uint64_t, std::uint64_t) noexcept override { return false; }
    [[nodiscard]] bool tier_lookup(std::uint64_t, std::uint64_t*) const noexcept override { return false; }
    [[nodiscard]] bool tier_erase(std::uint64_t) noexcept override { return false; }
    void               tier_clear() noexcept override {}
    [[nodiscard]] std::uint64_t tier_size() const noexcept override { return 0; }
    void tier_observe(cea::ComdareTierObserverSnapshot* out) const noexcept override {
        if (out != nullptr) *out = cea::ComdareTierObserverSnapshot{};
    }
    void tier_measure_accept(cea::IMessVisitor&) const noexcept override {}
    void tier_reset_statistics() noexcept override {}

private:
    ::comdare::cache_engine::execution_engine::EngineLifecycleState state_ =
        ::comdare::cache_engine::execution_engine::EngineLifecycleState::Idle;
};

/// destroy-Funktion fuer die Handle-Konstruktion. Sie tut NICHTS: das Objekt liegt hier auf dem
/// Stack des Tests, nicht im Heap einer .so. Ein delete waere an dieser Stelle der Fehler.
void kein_destroy(cea::IAnatomyBase*) noexcept {}

} // namespace

// ============================================================================================
// (1) DIE STATUS-CODES SIND IN DER NEUTRALEN SCHICHT ERREICHBAR
// ============================================================================================

TEST(K2StufenNeutralerLoader, StatusCodesUndNamenLiegenInDerNeutralenSchicht) {
    EXPECT_EQ(ad::dock_status_ok, 0);
    EXPECT_EQ(ad::dock_status_name(ad::dock_status_ok), std::string_view{"ok"});
    EXPECT_EQ(ad::dock_status_name(ad::dock_status_no_anatomy), std::string_view{"no_anatomy"});
    EXPECT_EQ(ad::dock_status_name(ad::dock_status_subinterface_missing), std::string_view{"subinterface_missing"});
    // Gegenprobe: die Namensfunktion ist nicht auf immer-benannt degeneriert.
    EXPECT_EQ(ad::dock_status_name(4242), std::string_view{"unknown"});
}

// ============================================================================================
// (2) DER DRIVE LAESST SICH OHNE BUILDER-DOCK ZIEHEN -- das ist der K2-Kern
// ============================================================================================

TEST(K2StufenNeutralerLoader, DriveWirdOhneIPruefDockGezogen) {
    NurObservableTier tier;
    // Ein Handle OHNE dlopen: native_handle == nullptr ist der zulaessige "kein Modul"-Fall.
    // Genau so kann eine Hybrid-Stufe den Drive spaeter an ein bereits geladenes Ziel haengen.
    // Die ABI-Version wird EXPLIZIT typisiert -- eine nackte {9,0}-Klammer im Ctor-Aufruf ist
    // nicht ableitbar (am Bau aufgelaufen). Der Wert selbst ist hier bedeutungslos: dieser Test
    // faehrt keine Major-Pruefung, die lebt im Loader und ist nicht sein Gegenstand.
    ::comdare::cache_engine::abi::AnatomyAbiVersion const abi_v{9, 0};
    al::AnatomyModuleHandle h{nullptr, static_cast<cea::IAnatomyBase*>(&tier), &kein_destroy, abi_v};

    ad::SearchAlgorithmDrive drive;
    EXPECT_EQ(ad::acquire_search_algorithm_drive(h, drive), ad::dock_status_ok);

    // Der Mess-Antrieb ist da und zeigt auf DIESES Objekt -- nicht auf irgendetwas.
    EXPECT_EQ(drive.obs, static_cast<cea::IObservableTier*>(&tier));
    // Die optionalen Sub-Antriebe fehlen ehrlich (die Attrappe traegt sie nicht) -- KEIN Absturz,
    // KEIN stiller Ersatz, sondern nullptr. Das ist der "alte DLL"-Vertrag von acquire_*.
    EXPECT_EQ(drive.ctrl, nullptr);
    EXPECT_EQ(drive.rbk, nullptr);
    EXPECT_EQ(drive.scn, nullptr);
}

TEST(K2StufenNeutralerLoader, FehlenderMessAntriebWirdBenanntGemeldet) {
    // Ein Handle ohne Anatomie ueberhaupt: der erste Fehlerpfad, und er ist ANDERS benannt als
    // "Sub-Interface fehlt". Ohne diese Probe koennten beide auf denselben Code kollabieren.
    al::AnatomyModuleHandle leer;
    ad::SearchAlgorithmDrive drive;
    EXPECT_EQ(ad::acquire_search_algorithm_drive(leer, drive), ad::dock_status_no_anatomy);
    EXPECT_NE(ad::dock_status_no_anatomy, ad::dock_status_subinterface_missing);
    EXPECT_EQ(drive.obs, nullptr);
}

// ============================================================================================
// (3) DIE ALTEN NAMEN GELTEN WEITER -- die Extraktion ist eine WANDERUNG, kein Bruch
// ============================================================================================
// Die Bestands-Konsumenten (7 Dateien inkludieren search_algorithm_dock.hpp, 3 nutzen den Drive)
// sind UNVERAENDERT geblieben. Moeglich ist das, weil die alten Header die gewanderten Namen per
// using re-exportieren. Diese TU prueft das NICHT -- sie inkludiert die Builder-Seite bewusst
// nicht. Der Beleg dafuer ist der gruene Bestands-Test test_v41_pruef_dock_search_algorithm,
// der die alten Namen ueber den alten Include benutzt und weiterhin laeuft.
// Beide zusammen sind die Aussage: neue Schicht erreichbar OHNE Builder, alte Schicht unveraendert.
