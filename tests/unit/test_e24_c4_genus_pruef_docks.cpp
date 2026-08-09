// test_e24_c4_genus_pruef_docks -- E-24 C4 (c/5), Bauplan-Dossier docs/sessions/
// 20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 3.1-C4: die vier Container-Gattungs-Docks als
// IPruefDock-Impls, in-process gefahren.
//
// IN-PROCESS-MODUL (dasselbe Verfahren wie tests/unit/test_v41_pruef_dock_search_algorithm.cpp:29-33):
// ein AnatomyModuleHandle um einen im Prozess gebauten ABI-Adapter (native=nullptr, destroy=nullptr ->
// unload() ist ein no-op). Die vtable ueber die Grenze ist identisch zur dlopen-Welt -- das Dock kann
// nicht unterscheiden, woher der IAnatomyBase* kommt, und genau das ist der Sinn dieses Aufbaus.
//
// WAS BELEGT WIRD (Bauplan Paragraf 4.1 Klasse I + 4.2 Klasse II, C4-Zeilen):
//   (1) V5-VERTRAGS-REIHENFOLGE import -> GATE -> messen: ein Modul, das sein Gattungs-Orakel NICHT
//       besteht, wird NICHT gemessen -- measure() liefert dock_status_conformance_failed UND laesst
//       out_csv/out_json leer. Das ist die eigentliche Aussage; ein Gate, das zwar meldet, aber trotzdem
//       misst, waere keins.
//   (2) wrong_genus: ein Modul der falschen Gattung an einem Dock -> dock_status_wrong_genus (2).
//   (3) no_anatomy: ein leeres Handle -> dock_status_no_anatomy (1).
//   (4) subinterface_missing: ein Modul der RICHTIGEN Gattung OHNE das Antriebs-Sub-Interface -> (3).
//   (5) select_for ueber die Registry waehlt je Handle das gattungs-passende Dock (per IM MODUL
//       deklarierter Gattung, KEINE Dateinamen-Heuristik).
//   (6) die Mess-Ausgabe traegt die gattungs-eigenen Observer-Spalten und den geteilten Zeit-Kopf.
//
// Registriert in tests/unit/CMakeLists.txt (COMDARE_GOALV6_BOOST_DTESTS -> add_test +
// COMDARE_TEST_TARGETS): keine Waisen-TU (Auflage 13).

#include "builder/pruef_dock/adapter_dock.hpp"
#include "builder/pruef_dock/pruef_dock_registry.hpp"
#include "builder/pruef_dock/pruef_dock_registry_default.hpp"
#include "builder/pruef_dock/pruef_dock_sequencer.hpp"
#include "builder/pruef_dock/sequence_dock.hpp"
#include "builder/pruef_dock/set_dock.hpp"
#include "builder/pruef_dock/view_dock.hpp"

#include "hybrid/heuristik_adapter_klassifikation.hpp" // HY-A1: Einzelquelle + Partition (Wachen-Nachzug)

#include "anatomy/adapter_permutation_engine.hpp"
#include "anatomy/sequence_permutation_engine.hpp"
#include "anatomy/set_permutation_engine.hpp"
#include "anatomy/view_permutation_engine.hpp"
#include "topics/sequence/axis_growth/axis_growth_policies.hpp"
#include "topics/view/view_policies.hpp"

#include <boost/mp11.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <iostream>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

// TU-lokale Test-Fixtures: anonymer Namespace verhindert ODR-Kollisionen der gleichnamigen Fixture-Typen
// ueber die e24-Test-TUs (cppcheck ctuOneDefinitionRuleViolation, Lint-Klasse 14398).
namespace {

namespace ana = comdare::cache_engine::anatomy;
namespace pd  = comdare::cache_engine::builder::pruef_dock;
namespace al  = comdare::cache_engine::builder::anatomy_loader;
namespace abi = comdare::cache_engine::abi;
namespace ag  = comdare::cache_engine::sequence::axis_growth;
namespace hy  = comdare::cache_engine::hybrid;
namespace vw  = comdare::cache_engine::view;
namespace mp  = boost::mp11;

int g_fail = 0;

template <class A, class B>
void eq(char const* w, A const& g, B const& e) {
    bool const ok = (g == e);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << w << " = " << g;
    if (!ok) {
        std::cout << "  (erwartet " << e << ")";
        ++g_fail;
    }
    std::cout << "\n";
}

void tr(char const* w, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n";
    if (!c) ++g_fail;
}

// ---------------------------------------------------------------------------------------------------
// Achsen-Fixtures (identische Auswahl wie in test_e24_c4_genus_conformance_gates: der getriebene Slot
// traegt ECHTE Organe/Policies, die uebrigen sind Identitaets-Filler).
// ---------------------------------------------------------------------------------------------------
struct OrderedKeySet {
    using key_type = std::uint64_t;
    std::set<std::uint64_t>                    s;
    void                                       insert(std::uint64_t k, std::uint64_t /*v*/) { s.insert(k); }
    [[nodiscard]] std::optional<std::uint64_t> lookup(std::uint64_t k) const {
        return s.count(k) ? std::optional<std::uint64_t>{k} : std::nullopt;
    }
    void                      erase(std::uint64_t k) { s.erase(k); }
    [[nodiscard]] std::size_t occupied_count() const { return s.size(); }
    void                      clear() { s.clear(); }
};

struct AxisAlpha {};

struct CfgSearchAlgo {
    using StaticAxisVariants = mp::mp_list<OrderedKeySet>;
};
struct CfgSingle {
    using StaticAxisVariants = mp::mp_list<AxisAlpha>;
};
struct CfgGrowth {
    using StaticAxisVariants = mp::mp_list<ana::DoublingGrowth>;
};
struct CfgExtent {
    using StaticAxisVariants = mp::mp_list<ana::DynamicExtent>;
};
struct CfgLayout {
    using StaticAxisVariants = mp::mp_list<ana::LayoutRight>;
};
struct CfgAccessor {
    using StaticAxisVariants = mp::mp_list<ana::DefaultAccessor>;
};
struct CfgInner {
    using StaticAxisVariants = mp::mp_list<ana::DequeInner<>>;
};

using SetEngine =
    ana::SetPermutationEngine<CfgSearchAlgo, CfgSingle, CfgSingle, CfgSingle, CfgSingle, CfgSingle, CfgSingle,
                              CfgSingle, CfgSingle, CfgSingle, CfgSingle, CfgSingle, CfgSingle>;
using SeqEngine  = ana::SequencePermutationEngine<CfgSingle, CfgSingle, CfgSingle, CfgSingle, CfgSingle, CfgSingle,
                                                  CfgSingle, CfgSingle, CfgGrowth>;
using AdpEngine  = ana::AdapterPermutationEngine<CfgSingle, CfgSingle, CfgSingle, CfgSingle, CfgSingle, CfgSingle,
                                                 CfgSingle, CfgSingle, CfgSingle, CfgSingle, CfgInner>;
using ViewEngine = ana::ViewPermutationEngine<CfgSingle, CfgSingle, CfgExtent, CfgLayout, CfgAccessor>;

/// Ein Handle um einen in-process gebauten ABI-Adapter (Stand-in fuer ein dlopen-geladenes Modul).
[[nodiscard]] al::AnatomyModuleHandle in_process_handle(ana::IAnatomyBase* base) {
    return al::AnatomyModuleHandle{nullptr, base, nullptr, abi::AnatomyAbiVersion{1, 0}};
}

// ---------------------------------------------------------------------------------------------------
// NEGATIV-Module: richtige Gattung, aber das Gate MUSS sie abweisen -- und danach darf NICHT gemessen
// werden. Sie tragen IAnatomyBase + das Sub-Interface, verletzen aber die Gattungs-Semantik.
// ---------------------------------------------------------------------------------------------------

/// Gemeinsame IAnatomyBase-Pflicht der Test-Module. engine_kind() ist in IAnatomyBase final.
template <ana::AnatomyGenus G>
class TestAnatomyBase : public ana::IAnatomyBase {
public:
    [[nodiscard]] std::string_view engine_name() const noexcept override { return "E24C4TestModul"; }
    [[nodiscard]] ::comdare::cache_engine::execution_engine::EngineLifecycleState
    lifecycle_state() const noexcept override {
        return state_;
    }
    void warm_up() override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Warming; }
    void run() override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Running; }
    void reset() override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Idle; }
    void shutdown() override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Shutdown; }
    [[nodiscard]] std::string_view  composition_name() const noexcept override { return "E24C4TestModul"; }
    [[nodiscard]] std::string_view  paper_id() const noexcept override { return "P00 E24-C4-Probe"; }
    [[nodiscard]] ana::AnatomyGenus genus() const noexcept override { return G; }
    [[nodiscard]] std::size_t       organ_count() const noexcept override { return 0; }

private:
    ::comdare::cache_engine::execution_engine::EngineLifecycleState state_ =
        ::comdare::cache_engine::execution_engine::EngineLifecycleState::Idle;
};

/// Set-Modul, das Duplikate annimmt -> faellt am std::set-Orakel (SRF3). Gattung stimmt, Sub-Interface da.
class DuplicateAcceptingSetModule final : public TestAnatomyBase<ana::AnatomyGenus::Set>, public ana::ISetTier {
public:
    [[nodiscard]] bool tier_set_insert(std::uint64_t key) noexcept override {
        keys_.push_back(key);
        return true; // LUEGE
    }
    [[nodiscard]] bool tier_set_contains(std::uint64_t key) const noexcept override {
        for (auto k : keys_) {
            if (k == key) return true;
        }
        return false;
    }
    [[nodiscard]] bool tier_set_erase(std::uint64_t key) noexcept override {
        for (std::size_t i = 0; i < keys_.size(); ++i) {
            if (keys_[i] == key) {
                keys_.erase(keys_.begin() + static_cast<std::ptrdiff_t>(i));
                return true;
            }
        }
        return false;
    }
    [[nodiscard]] std::uint64_t tier_set_size() const noexcept override {
        return static_cast<std::uint64_t>(keys_.size());
    }
    void tier_set_clear() noexcept override { keys_.clear(); }
    void tier_observe_set(ana::SetObserverSnapshotV1* out) const noexcept override {
        if (out != nullptr) *out = ana::SetObserverSnapshotV1{};
    }

private:
    std::vector<std::uint64_t> keys_{};
};

/// Set-Modul OHNE ISetTier: die Gattung stimmt, der Antrieb fehlt -> subinterface_missing (der reale
/// Fall "alte DLL ohne Pfad B").
class SetModuleWithoutDrive final : public TestAnatomyBase<ana::AnatomyGenus::Set> {};

} // namespace

int main() {
    std::cout << "==== E-24 C4 (c/5): die vier Container-Docks als IPruefDock-Impls (V5-Vertrag) ====\n";

    pd::PruefDockMeasureOptions opts;
    opts.fill_checkpoints       = {8, 32};
    opts.lookups_per_checkpoint = 64;
    opts.deletes_per_checkpoint = 8;

    // ------------------------------------------------------------------------------------------
    // Identitaet + Version je Dock
    // ------------------------------------------------------------------------------------------
    std::cout << "\n-- Dock-Identitaet, Gattungs-Bindung und ce-eigene Version --\n";
    pd::SetPruefDock      set_dock;
    pd::SequencePruefDock seq_dock;
    pd::AdapterPruefDock  adp_dock;
    pd::ViewPruefDock     view_dock;

    eq("SetPruefDock::dock_genus()", static_cast<int>(set_dock.dock_genus()), static_cast<int>(ana::AnatomyGenus::Set));
    eq("SequencePruefDock::dock_genus()", static_cast<int>(seq_dock.dock_genus()),
       static_cast<int>(ana::AnatomyGenus::Sequence));
    eq("AdapterPruefDock::dock_genus()", static_cast<int>(adp_dock.dock_genus()),
       static_cast<int>(ana::AnatomyGenus::Adapter));
    eq("ViewPruefDock::dock_genus()", static_cast<int>(view_dock.dock_genus()),
       static_cast<int>(ana::AnatomyGenus::View));
    eq("SetPruefDock::dock_name()", std::string{set_dock.dock_name()}, std::string{"SetPruefDock"});
    eq("SequencePruefDock::dock_name()", std::string{seq_dock.dock_name()}, std::string{"SequencePruefDock"});
    eq("AdapterPruefDock::dock_name()", std::string{adp_dock.dock_name()}, std::string{"AdapterPruefDock"});
    eq("ViewPruefDock::dock_name()", std::string{view_dock.dock_name()}, std::string{"ViewPruefDock"});
    // Die Dock-Version kommt NICHT aus einer im Test gepflegten Zeichenkette, sondern aus derselben
    // Quelle wie die ENFORCE-Wachen (pruef_dock_version.hpp) -- ueber die Gattung nachgeschlagen.
    tr("SetPruefDock::dock_version() == pruef_dock_version_for(Set)",
       pd::SetPruefDock::dock_version() == pd::pruef_dock_version_for(ana::AnatomyGenus::Set));
    tr("SequencePruefDock::dock_version() == pruef_dock_version_for(Sequence)",
       pd::SequencePruefDock::dock_version() == pd::pruef_dock_version_for(ana::AnatomyGenus::Sequence));
    tr("AdapterPruefDock::dock_version() == pruef_dock_version_for(Adapter)",
       pd::AdapterPruefDock::dock_version() == pd::pruef_dock_version_for(ana::AnatomyGenus::Adapter));
    tr("ViewPruefDock::dock_version() == pruef_dock_version_for(View)",
       pd::ViewPruefDock::dock_version() == pd::pruef_dock_version_for(ana::AnatomyGenus::View));

    // ------------------------------------------------------------------------------------------
    // POSITIV: je Gattung ein reales Modul durch den vollen V5-Pfad (import -> GATE -> messen)
    // ------------------------------------------------------------------------------------------
    std::cout << "\n-- V5-Pfad an realen Modulen: import -> GATE -> messen --\n";
    SetEngine::for_each_abi_adapter([&](ana::IAnatomyBase& base, std::string_view) {
        auto        h = in_process_handle(&base);
        std::string csv, json;
        tr("SET: Dock akzeptiert das Modul (Gattungs-Match ueber genus())", set_dock.accepts(h));
        int const rc = set_dock.measure(h, opts, csv, json);
        eq("SET: measure() rc", std::string{pd::dock_status_name(rc)}, std::string{"ok"});
        tr("SET: CSV traegt den geteilten Zeit-Kopf", csv.find("checkpoint,observe_wall_ns,fill_level") == 0);
        tr("SET: CSV traegt die gattungs-eigenen Observer-Spalten",
           csv.find("set_insert,set_contains,set_hit,set_miss,set_erase") != std::string::npos);
        tr("SET: JSON ist ein Array mit korrelierten Zaehlern",
           !json.empty() && json.front() == '[' && json.find("\"set_insert\"") != std::string::npos);
    });

    SeqEngine::for_each_abi_adapter([&](ana::IAnatomyBase& base, std::string_view) {
        auto        h = in_process_handle(&base);
        std::string csv, json;
        int const   rc = seq_dock.measure(h, opts, csv, json);
        eq("SEQUENCE: measure() rc", std::string{pd::dock_status_name(rc)}, std::string{"ok"});
        tr("SEQUENCE: CSV traegt die gattungs-eigenen Observer-Spalten",
           csv.find("seq_push,seq_at,seq_at_oob") != std::string::npos);
        // Die benannte ABI-Luecke, als Wache: ISequenceTier hat keine Loesch-Op -> delete_samples ist
        // strukturell 0. Kaeme jemand auf die Idee, dort eine Ersatz-Op einzusetzen, bricht das hier.
        tr("SEQUENCE: JSON traegt delete_p50_ns == 0 (keine Loesch-Op an der ABI-Flaeche)",
           json.find("\"delete_p50_ns\":0") != std::string::npos);
    });

    AdpEngine::for_each_abi_adapter([&](ana::IAnatomyBase& base, std::string_view) {
        auto        h = in_process_handle(&base);
        std::string csv, json;
        int const   rc = adp_dock.measure(h, opts, csv, json);
        eq("ADAPTER: measure() rc", std::string{pd::dock_status_name(rc)}, std::string{"ok"});
        tr("ADAPTER: CSV traegt die gattungs-eigenen Observer-Spalten",
           csv.find("adp_push,adp_pop,adp_front_reads") != std::string::npos);
        // Die zweite benannte ABI-Luecke: keine nicht-konsumierende Lese-Op -> read_samples strukturell 0,
        // die Entnahme-Zeiten stehen in delete_*.
        tr("ADAPTER: JSON traegt read_p50_ns == 0 (keine nicht-konsumierende Lese-Op)",
           json.find("\"read_p50_ns\":0") != std::string::npos);
        tr("ADAPTER: das Dock haelt die erkannte Entnahme-Disziplin fest (DequeInner -> fifo)",
           adp_dock.last_discipline() == pd::AdapterDiscipline::fifo);
    });

    ViewEngine::for_each_abi_adapter([&](ana::IAnatomyBase& base, std::string_view) {
        auto        h = in_process_handle(&base);
        std::string csv, json;
        int const   rc = view_dock.measure(h, opts, csv, json);
        eq("VIEW: measure() rc", std::string{pd::dock_status_name(rc)}, std::string{"ok"});
        tr("VIEW: CSV traegt die gattungs-eigenen Observer-Spalten",
           csv.find("view_read,view_read_oob,view_bound_size") != std::string::npos);
    });

    // ------------------------------------------------------------------------------------------
    // NEGATIV 1 (die Kern-Aussage): Gate faellt -> conformance_failed UND es wird NICHT gemessen
    // ------------------------------------------------------------------------------------------
    std::cout << "\n-- V5-Reihenfolge: ein durchgefallenes Gate darf NICHT zu einer Mess-Zeile fuehren --\n";
    {
        DuplicateAcceptingSetModule bad;
        auto                        h = in_process_handle(&bad);
        std::string                 csv{"VORBELEGT"};
        std::string                 json{"VORBELEGT"};
        tr("das defekte Modul wird vom Dock AKZEPTIERT (Gattung stimmt -- das Gate ist die Instanz, die "
           "es abweist, nicht accepts())",
           set_dock.accepts(h));
        int const rc = set_dock.measure(h, opts, csv, json);
        eq("rc", std::string{pd::dock_status_name(rc)}, std::string{"conformance_failed"});
        eq("dock_status_conformance_failed ist 4 (errno-Stil, pruef_dock.hpp:40-41)", rc,
           pd::dock_status_conformance_failed);
        // DAS ist die Aussage: die Ausgaben wurden NICHT angefasst -- es lief keine Messung.
        eq("out_csv unberuehrt (keine Messung nach durchgefallenem Gate)", csv, std::string{"VORBELEGT"});
        eq("out_json unberuehrt (keine Messung nach durchgefallenem Gate)", json, std::string{"VORBELEGT"});
    }

    // ------------------------------------------------------------------------------------------
    // NEGATIV 2-4: wrong_genus / no_anatomy / subinterface_missing
    // ------------------------------------------------------------------------------------------
    std::cout << "\n-- die uebrigen drei Ablehnungs-Pfade --\n";
    SetEngine::for_each_abi_adapter([&](ana::IAnatomyBase& base, std::string_view) {
        auto        h = in_process_handle(&base);
        std::string csv, json;
        tr("ein Set-Modul wird vom Sequence-Dock NICHT akzeptiert", !seq_dock.accepts(h));
        eq("Set-Modul am Sequence-Dock -> wrong_genus", seq_dock.measure(h, opts, csv, json),
           pd::dock_status_wrong_genus);
        eq("Set-Modul am View-Dock -> wrong_genus", view_dock.measure(h, opts, csv, json), pd::dock_status_wrong_genus);
    });
    {
        al::AnatomyModuleHandle empty{};
        std::string             csv, json;
        tr("ein anatomie-loses Handle wird von keinem Dock akzeptiert",
           !set_dock.accepts(empty) && !seq_dock.accepts(empty) && !adp_dock.accepts(empty) &&
               !view_dock.accepts(empty));
        eq("leeres Handle -> no_anatomy", set_dock.measure(empty, opts, csv, json), pd::dock_status_no_anatomy);
    }
    {
        SetModuleWithoutDrive nodrive;
        auto                  h = in_process_handle(&nodrive);
        std::string           csv, json;
        tr("richtige Gattung ohne Antriebs-Sub-Interface wird AKZEPTIERT (die Gattung stimmt ja)", set_dock.accepts(h));
        eq("Set-Modul ohne ISetTier -> subinterface_missing", set_dock.measure(h, opts, csv, json),
           pd::dock_status_subinterface_missing);
    }

    // ------------------------------------------------------------------------------------------
    // Registry: select_for waehlt per IM MODUL deklarierter Gattung
    // ------------------------------------------------------------------------------------------
    std::cout << "\n-- Registry-Auswahl ueber alle fuenf Gattungen (dieselbe Belegung wie f15_compare) --\n";
    {
        // E-24 C4 (d/5): DIESELBE Funktion, die apps/f15_compare/main.cpp aufruft -- es gibt keine
        // zweite, im Test nachgebildete Liste, die driften koennte.
        pd::PruefDockRegistry reg;
        pd::register_all_genus_docks(reg);
        eq("Registry-Groesse (alle fuenf Gattungen)", reg.size(), std::size_t{5});

        // ------------------------------------------------------------------------------------
        // NACHZUG HY-A1 (09.08.2026) -- DIESE WACHE WAR KEINE. KORREKTUR AM OBJEKT.
        // ------------------------------------------------------------------------------------
        // Hier stand: "VOLLSTAENDIGKEITS-WACHE ueber die ENUM-Werte, nicht ueber eine Aufzaehlung
        // im Test: kaeme eine sechste Ebene-2-Gattung dazu und register_all_genus_docks vergaesse
        // sie, bricht das hier." -- gefolgt von exakt einer solchen Aufzaehlung im Test
        // (`constexpr std::array<ana::AnatomyGenus, 5> kAllGenera{...}`).
        //
        // Die Behauptung war nie zutreffend, und das ist am 09.08.2026 literal nachgemessen worden:
        // AnatomyGenus::FunctionInterfaceReroute wurde angehaengt, diese TU neu uebersetzt (die
        // .o-Zeitstempel liegen nach dem Header-Edit, es war kein Cache-Artefakt) -- 0 Fehler, der
        // Test blieb gruen. Eine handgefuehrte Liste kann per Konstruktion nicht bemerken, dass das
        // Enum laenger geworden ist.
        //
        // JETZT laeuft die Wache ueber die EINZELQUELLE hy::kAlleGenera, deren Deckungsgleichheit
        // mit dem Enum ihrerseits compile-hart bewacht ist (256-Werte-Abgleich gegen genus_name(),
        // hybrid/heuristik_adapter_klassifikation.hpp). Die Kette ist damit geschlossen:
        // Enum -> Einzelquelle -> Dock-Registry.
        //
        // UND DIE NEUE UNTERSCHEIDUNG, die es vorher nicht zu treffen gab: nicht jedes Genus braucht
        // ein Dock. Ein KLASSIFIKATIONS-Genus (Gattung HeuristikAdapter) hat keins und darf keins
        // haben -- seine Binaries melden per genus() ihr ZIEL-Genus und docken an DESSEN Dock an
        // (Owner-Entscheid E-1 final, Weg C). Deshalb wird hier nach der Partition gefiltert statt
        // stumpf ueber alle Enum-Werte zu laufen; ein ungefilterter Lauf wuerde ab HY-A1 fehlschlagen
        // und zwar zu Recht.
        std::size_t abi_sichtbare            = 0;
        bool        all_genera_covered       = true;
        bool        klassifikation_ohne_dock = true;
        for (auto const g : hy::kAlleGenera) {
            pd::IPruefDock const* d = reg.dock_for_genus(g);
            if (hy::ist_abi_sichtbares_genus(g)) {
                ++abi_sichtbare;
                if (d == nullptr || d->dock_genus() != g) all_genera_covered = false;
            } else {
                // Ein Reroute-Genus DARF kein Dock haben. Faende sich hier eins, waere Weg C
                // gebrochen -- jemand haette der Hybrid-Stufe ein eigenes Dock gegeben.
                if (d != nullptr) klassifikation_ohne_dock = false;
            }
        }
        tr("dock_for_genus() liefert fuer JEDE ABI-sichtbare Ebene-2-Gattung ein Dock", all_genera_covered);
        tr("ein KLASSIFIKATIONS-Genus (HeuristikAdapter) hat KEIN eigenes Dock (Weg C)", klassifikation_ohne_dock);
        eq("Zahl der ABI-sichtbaren Genera == Registry-Groesse", reg.size(), abi_sichtbare);
        eq("Zahl der ABI-sichtbaren Genera == Partitions-Konstante", abi_sichtbare,
           hy::kPruefDockPflichtigeGenusAnzahl);
        // Gegenprobe, dass die Einzelquelle wirklich MEHR Werte kennt als die Registry Docks hat --
        // sonst waere die Filterung oben wirkungslos und die Wache truebe gestellt.
        tr("die Einzelquelle kennt mehr Genera als es Docks gibt (die Filterung ist wirksam)",
           hy::kAlleGenera.size() > reg.size());

        // LEBENSDAUER-FALLE, am Bau aufgelaufen und deshalb hier benannt: for_each_abi_adapter
        // materialisiert den ABI-Adapter je Permutation als LOKALE Variable im Callback
        // (set_permutation_engine.hpp:119-125 "SetAbiAdapter<SetAnatomy<SetComp>> adapter;"). Ein
        // AnatomyModuleHandle, das man daraus mitnimmt, zeigt nach der Rueckkehr auf ein totes Objekt --
        // der erste Bau dieser TU ist damit literal in ein Segmentation fault (Exit 139) gelaufen.
        // Fuer einen Mischlauf, der die Handles UEBERLEBEN muss, gehoeren die Adapter dem Test: sie
        // werden hier ueber for_each_composition_type (Kompositions-TYP statt Instanz) selbst gebaut
        // und gehalten. In den Positiv-Bloecken oben ist das nicht noetig -- dort wird das Handle
        // ausschliesslich INNERHALB des Callbacks benutzt.
        std::vector<std::unique_ptr<ana::IAnatomyBase>> owned;
        SetEngine::for_each_composition_type(
            [&]<class C>() { owned.push_back(std::make_unique<ana::SetAbiAdapter<ana::SetAnatomy<C>>>()); });
        SeqEngine::for_each_composition_type(
            [&]<class C>() { owned.push_back(std::make_unique<ana::SequenceAbiAdapter<ana::SequenceAnatomy<C>>>()); });
        AdpEngine::for_each_composition_type(
            [&]<class C>() { owned.push_back(std::make_unique<ana::AdapterAbiAdapter<ana::AdapterAnatomy<C>>>()); });
        ViewEngine::for_each_composition_type(
            [&]<class C>() { owned.push_back(std::make_unique<ana::ViewAbiAdapter<ana::ViewAnatomy<C>>>()); });
        eq("vier gehaltene ABI-Adapter (je Gattung eine Permutation)", owned.size(), std::size_t{4});

        std::vector<al::AnatomyModuleHandle> handles;
        handles.reserve(owned.size());
        for (auto const& a : owned) handles.push_back(in_process_handle(a.get()));
        eq("vier Handles", handles.size(), std::size_t{4});

        bool select_ok = true;
        for (auto const& h : handles) {
            pd::IPruefDock const* d = reg.select_for(h);
            if (d == nullptr || h.anatomy() == nullptr || d->dock_genus() != h.anatomy()->genus()) select_ok = false;
        }
        tr("select_for liefert je Handle das gattungs-passende Dock", select_ok);

        auto const results = pd::measure_genus_sequential(reg, handles, opts);
        eq("measure_genus_sequential: ein Ergebnis je Handle", results.size(), handles.size());
        bool all_ok = true;
        std::cout << "  gattungs-sequentieller Mischlauf:\n";
        for (auto const& r : results) {
            std::cout << "    " << r.dock_name << " status=" << pd::dock_status_name(r.status)
                      << " csv_bytes=" << r.csv.size() << " json_bytes=" << r.json.size() << "\n";
            if (r.status != pd::dock_status_ok || r.csv.empty() || r.json.empty()) all_ok = false;
        }
        tr("jeder Mischlauf-Eintrag ist ok und traegt CSV+JSON", all_ok);
    }

    std::cout << "\n==== E-24 C4 (c/5) Container-Docks: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
