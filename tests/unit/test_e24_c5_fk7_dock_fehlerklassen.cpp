// test_e24_c5_fk7_dock_fehlerklassen -- E-24 C5 (a/2), Bauplan-Dossier docs/sessions/
// 20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 6.2 (FK-7) + Paragraf 4.3 (G3-FK-Beweis):
// "je Fehlerpfad stabiles Etikett im Log UND 'failed'-CSV-Zelle literal, NIE stille 0/null".
//
// WAS BELEGT WIRD -- und zwar an ECHTEN Docks, nicht an einer Abbildungs-Tabelle allein:
//   (1) Jeder der vier realen Dock-Fehlerpfade (no_anatomy / wrong_genus / subinterface_missing /
//       conformance_failed) wird an einem PRODUKTIVEN Dock ERZEUGT und danach klassifiziert. Die Probe
//       liest den Transport-Int also nicht aus einer Konstante ab, sondern loest ihn real aus.
//   (2) Die Trennung, die der Transport-Int NICHT hergibt: "Modul ohne Anatomie" vs. "kein Dock fuer
//       diese Gattung" tragen beide dock_status_no_anatomy (pruef_dock_sequencer), bekommen aber
//       verschiedene Klassen. Genau das repariert FK-7.
//   (3) Die Zelle ist IMMER "failed" -- nie leer, nie "0", nie "n/a".
//   (4) Das Log-Etikett ist stabil, klassen-genau und traegt den rohen Transport-Namen weiter mit.
//   (5) FIXTURE-UNABHAENGIGER ABLEITUNGSWEG (Memory-Lehre "gruene Tests zementieren alte Ordnung"):
//       eine zweite Schleife laeuft ueber ALLE Int-Werte 0..15 und ueber die Count-Single-Source der
//       Taxonomie, statt eine handgepflegte Liste zu pruefen. Ein neuer dock_status- oder Klassen-Wert
//       faellt damit auf, ohne dass diese Datei ihn kennt.
//   (6) Der ABI-nahe Transport ist UNBERUEHRT: die dock_status_*-Werte 0..4 stehen literal still
//       (HY-D2-Freeze -- FK-7 klassifiziert daneben, es dreht keine Zahl).
//
// Registriert in tests/unit/CMakeLists.txt (eigener Block mit comdare::anatomy_module_loader --
// dieselbe Kante wie die C4-Dock-TU: AnatomyModuleHandle::unload() lebt im Loader-.cpp). Keine Waisen-TU.

#include "builder/pruef_dock/dock_error_classification.hpp"
#include "builder/pruef_dock/pruef_dock_registry.hpp"
#include "builder/pruef_dock/pruef_dock_registry_default.hpp"
#include "builder/pruef_dock/pruef_dock_sequencer.hpp"
#include "builder/pruef_dock/set_dock.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// TU-lokale Test-Fixtures: anonymer Namespace verhindert ODR-Kollisionen der gleichnamigen Fixture-Typen
// ueber die e24-Test-TUs (cppcheck ctuOneDefinitionRuleViolation, Lint-Klasse 14398).
namespace {

namespace ana = comdare::cache_engine::anatomy;
namespace pd  = comdare::cache_engine::builder::pruef_dock;
namespace al  = comdare::cache_engine::builder::anatomy_loader;
namespace abi = comdare::cache_engine::abi;
namespace cem = comdare::cache_engine::measurement;

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

/// Ein Handle um einen in-process gebauten ABI-Adapter (Stand-in fuer ein dlopen-geladenes Modul) --
/// dasselbe Verfahren wie in test_e24_c4_genus_pruef_docks.cpp.
[[nodiscard]] al::AnatomyModuleHandle in_process_handle(ana::IAnatomyBase* base) {
    return al::AnatomyModuleHandle{nullptr, base, nullptr, abi::AnatomyAbiVersion{1, 0}};
}

/// Gemeinsame IAnatomyBase-Pflicht der Test-Module (engine_kind() ist in IAnatomyBase final).
template <ana::AnatomyGenus G>
class Fk7AnatomyBase : public ana::IAnatomyBase {
public:
    [[nodiscard]] std::string_view engine_name() const noexcept override { return "E24C5Fk7Modul"; }
    [[nodiscard]] ::comdare::cache_engine::execution_engine::EngineLifecycleState
    lifecycle_state() const noexcept override {
        return state_;
    }
    void warm_up() override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Warming; }
    void run() override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Running; }
    void reset() override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Idle; }
    void shutdown() override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Shutdown; }
    [[nodiscard]] std::string_view  composition_name() const noexcept override { return "E24C5Fk7Modul"; }
    [[nodiscard]] std::string_view  paper_id() const noexcept override { return "P00 E24-C5-FK7-Probe"; }
    [[nodiscard]] ana::AnatomyGenus genus() const noexcept override { return G; }
    [[nodiscard]] std::size_t       organ_count() const noexcept override { return 0; }

private:
    ::comdare::cache_engine::execution_engine::EngineLifecycleState state_ =
        ::comdare::cache_engine::execution_engine::EngineLifecycleState::Idle;
};

/// Sequence-Modul OHNE jedes Sub-Interface -- am SetPruefDock erzeugt es wrong_genus (2).
class FremdesGenusModul final : public Fk7AnatomyBase<ana::AnatomyGenus::Sequence> {};

/// Set-Modul der RICHTIGEN Gattung, aber OHNE ISetTier -- erzeugt subinterface_missing (3).
class SetModulOhneAntrieb final : public Fk7AnatomyBase<ana::AnatomyGenus::Set> {};

/// Set-Modul MIT ISetTier, das Duplikate annimmt -- reisst das std::set-Orakel und erzeugt
/// conformance_failed (4), OHNE dass gemessen wird (V5-Vertrag import -> GATE -> messen).
class LuegendesSetModul final : public Fk7AnatomyBase<ana::AnatomyGenus::Set>, public ana::ISetTier {
public:
    [[nodiscard]] bool tier_set_insert(std::uint64_t key) noexcept override {
        keys_.push_back(key);
        return true; // LUEGE: ein echtes Set meldet beim zweiten Mal false
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

/// Fuehrt EIN Modul durch das Set-Dock und liefert den rohen Transport-Int (real ausgeloest, nicht geraten).
[[nodiscard]] int set_dock_status_fuer(ana::IAnatomyBase* base) {
    pd::SetPruefDock            dock;
    al::AnatomyModuleHandle     h = in_process_handle(base);
    pd::PruefDockMeasureOptions opts;
    opts.fill_checkpoints       = {8};
    opts.lookups_per_checkpoint = 4;
    opts.deletes_per_checkpoint = 1;
    std::string csv;
    std::string json;
    return dock.measure(h, opts, csv, json);
}

} // namespace

int main() {
    std::cout << "== E-24 C5 / FK-7: Dock-Fehlerpfade werden D2-klassifiziert (Zelle 'failed' + Log-Etikett) ==\n";

    // -----------------------------------------------------------------------------------------------
    // (6) VORWEG: der ABI-nahe Transport steht still. FK-7 klassifiziert DANEBEN (HY-D2-Freeze).
    // -----------------------------------------------------------------------------------------------
    std::cout << "\n-- (0) Transport-Vokabular unveraendert (kein V5-Freeze-Bruch) --\n";
    eq("dock_status_ok", pd::dock_status_ok, 0);
    eq("dock_status_no_anatomy", pd::dock_status_no_anatomy, 1);
    eq("dock_status_wrong_genus", pd::dock_status_wrong_genus, 2);
    eq("dock_status_subinterface_missing", pd::dock_status_subinterface_missing, 3);
    eq("dock_status_conformance_failed", pd::dock_status_conformance_failed, 4);

    // -----------------------------------------------------------------------------------------------
    // (1) Die vier realen Dock-Fehlerpfade -- am produktiven SetPruefDock ERZEUGT, dann klassifiziert.
    // -----------------------------------------------------------------------------------------------
    std::cout << "\n-- (1) real ausgeloeste Fehlerpfade -> Klasse + Zelle + Etikett --\n";
    {
        int const st = set_dock_status_fuer(nullptr);
        eq("leeres Handle -> Transport", st, pd::dock_status_no_anatomy);
        auto const k = pd::classify_dock_status(st);
        tr("leeres Handle -> Klasse gesetzt", k.has_value());
        eq("leeres Handle -> Etikett", cem::dock_error_label(*k), std::string_view{"modul_ohne_anatomie"});
        eq("leeres Handle -> Zelle", pd::dock_failure_cell(*k), std::string_view{"failed"});
    }
    {
        FremdesGenusModul m;
        int const         st = set_dock_status_fuer(&m);
        eq("fremde Gattung -> Transport", st, pd::dock_status_wrong_genus);
        auto const k = pd::classify_dock_status(st);
        tr("fremde Gattung -> Klasse gesetzt", k.has_value());
        eq("fremde Gattung -> Etikett", cem::dock_error_label(*k), std::string_view{"fremde_gattung"});
        eq("fremde Gattung -> Zelle", pd::dock_failure_cell(*k), std::string_view{"failed"});
    }
    {
        SetModulOhneAntrieb m;
        int const           st = set_dock_status_fuer(&m);
        eq("Antrieb fehlt -> Transport", st, pd::dock_status_subinterface_missing);
        auto const k = pd::classify_dock_status(st);
        eq("Antrieb fehlt -> Etikett", cem::dock_error_label(*k), std::string_view{"antriebs_interface_fehlt"});
        eq("Antrieb fehlt -> Zelle", pd::dock_failure_cell(*k), std::string_view{"failed"});
    }
    {
        LuegendesSetModul m;
        int const         st = set_dock_status_fuer(&m);
        eq("Orakel gerissen -> Transport", st, pd::dock_status_conformance_failed);
        auto const k = pd::classify_dock_status(st);
        eq("Orakel gerissen -> Etikett", cem::dock_error_label(*k), std::string_view{"konformitaet_gescheitert"});
        eq("Orakel gerissen -> Zelle", pd::dock_failure_cell(*k), std::string_view{"failed"});
    }

    // -----------------------------------------------------------------------------------------------
    // (2) Die Trennung, die der Transport-Int NICHT hergibt -- ueber den echten Sequenzierer gefahren.
    // -----------------------------------------------------------------------------------------------
    std::cout << "\n-- (2) derselbe Transport-Int, zwei verschiedene Befunde --\n";
    {
        // (a) Handle OHNE Anatomie: kein Dock akzeptiert es -> Modul-Seite.
        std::vector<al::AnatomyModuleHandle> handles;
        handles.push_back(in_process_handle(nullptr));
        pd::PruefDockRegistry reg;
        pd::register_all_genus_docks(reg);
        pd::PruefDockMeasureOptions opts;
        auto const                  res = pd::measure_genus_sequential(reg, handles, opts);
        eq("Ergebnis-Zahl", res.size(), std::size_t{1});
        eq("ohne Anatomie -> Transport", res[0].status, pd::dock_status_no_anatomy);
        tr("ohne Anatomie -> Klasse gesetzt", res[0].error_class.has_value());
        eq("ohne Anatomie -> Klasse", cem::dock_error_label(*res[0].error_class),
           std::string_view{"modul_ohne_anatomie"});
    }
    {
        // (b) Handle MIT Anatomie, aber LEERE Registry: die Gattung ist deklariert, es fehlt das Dock.
        FremdesGenusModul                    m;
        std::vector<al::AnatomyModuleHandle> handles;
        handles.push_back(in_process_handle(&m));
        pd::PruefDockRegistry       leer; // bewusst OHNE register_all_genus_docks
        pd::PruefDockMeasureOptions opts;
        auto const                  res = pd::measure_genus_sequential(leer, handles, opts);
        eq("Registry-Luecke -> Transport", res[0].status, pd::dock_status_no_anatomy);
        eq("Registry-Luecke -> Klasse", cem::dock_error_label(*res[0].error_class),
           std::string_view{"kein_dock_fuer_gattung"});
        tr("BEIDE Faelle tragen DENSELBEN Transport-Int -- die Klassen trennen sie",
           pd::dock_status_no_anatomy == res[0].status &&
               cem::dock_error_label(*res[0].error_class) != std::string_view{"modul_ohne_anatomie"});
    }

    // -----------------------------------------------------------------------------------------------
    // (3)+(4) Der Datensatz: CSV-Zelle "failed" literal + klassifizierte Log-Zeile mit Transport-Detail.
    // -----------------------------------------------------------------------------------------------
    std::cout << "\n-- (3)/(4) der Fehler-Datensatz (CSV-Zelle + Log-Zeile) --\n";
    {
        std::string const csv =
            pd::dock_failure_csv(cem::DockErrorClass::FremdeGattung, "perm_set_d9", pd::dock_status_name(2));
        std::cout << "  CSV literal:\n" << csv;
        tr("CSV traegt die Zelle 'failed'", csv.find(",failed\n") != std::string::npos);
        tr("CSV traegt das Klassen-Etikett", csv.find("fremde_gattung") != std::string::npos);
        tr("CSV traegt den rohen Transport-Namen", csv.find("wrong_genus") != std::string::npos);
        tr("CSV traegt KEINE stille Null", csv.find(",0\n") == std::string::npos);

        std::string const log =
            pd::dock_failure_log_line(cem::DockErrorClass::FremdeGattung, "perm_set_d9", pd::dock_status_name(2));
        std::cout << "  LOG literal: " << log << "\n";
        tr("Log traegt das D2-Praefix", log.find("Mess-Fehler[") == 0);
        tr("Log traegt das Klassen-Etikett", log.find("fremde_gattung") != std::string::npos);
        tr("Log traegt den Transport-Namen", log.find("transport=wrong_genus") != std::string::npos);
        tr("Log nennt die Zelle", log.find("zelle=failed") != std::string::npos);
    }
    {
        // Lade-Fehler: derselbe Datensatz-Weg fuer den FRUEHESTEN Eintrittspunkt (G5-relevant: ein
        // abgelehntes Alt-Major-Modul ist ein Lade-Fehler und darf nicht als Leerstelle enden).
        auto const k = pd::classify_loader_status(al::status_abi_major_mismatch);
        tr("Lade-Fehler -> Klasse gesetzt", k.has_value());
        eq("Lade-Fehler -> Etikett", cem::dock_error_label(*k), std::string_view{"modul_lade_fehler"});
        std::string const log =
            pd::dock_failure_log_line(*k, "altes_major7_modul", al::status_name(al::status_abi_major_mismatch));
        std::cout << "  LOG literal: " << log << "\n";
        tr("Lade-Log traegt den Loader-Grund woertlich", log.find("abi_major_mismatch") != std::string::npos);
    }

    // -----------------------------------------------------------------------------------------------
    // (5) FIXTURE-UNABHAENGIGER ABLEITUNGSWEG: ueber die Wertebereiche, nicht ueber eine Liste.
    // -----------------------------------------------------------------------------------------------
    std::cout << "\n-- (5) Ableitungsweg ohne Fixture-Liste --\n";
    {
        // (a) JEDER Int ausser ok ergibt eine Klasse, und JEDE Klasse rendert "failed".
        bool alle_klassifiziert = true;
        bool alle_failed        = true;
        for (int s = 0; s <= 15; ++s) {
            auto const k = pd::classify_dock_status(s);
            if (s == pd::dock_status_ok) {
                if (k.has_value()) alle_klassifiziert = false;
                continue;
            }
            if (!k.has_value())
                alle_klassifiziert = false;
            else if (pd::dock_failure_cell(*k) != std::string_view{"failed"})
                alle_failed = false;
        }
        tr("jeder Nicht-ok-Transport-Int traegt eine Klasse (und ok traegt keine)", alle_klassifiziert);
        tr("jede erzeugte Klasse rendert die Zelle 'failed'", alle_failed);

        // (b) ueber die Count-Single-Source: jedes Etikett nicht leer, nicht der Fallback, paarweise
        //     verschieden -- ohne dass diese Datei die Klassen aufzaehlt.
        bool etiketten_ok = true;
        for (std::size_t i = 0; i < cem::kDockErrorClassCount; ++i) {
            auto const li = cem::dock_error_label(static_cast<cem::DockErrorClass>(i));
            if (li.empty() || li == std::string_view{"dock_fehler_unbekannt"}) etiketten_ok = false;
            for (std::size_t j = i + 1; j < cem::kDockErrorClassCount; ++j)
                if (li == cem::dock_error_label(static_cast<cem::DockErrorClass>(j))) etiketten_ok = false;
        }
        tr("alle Klassen-Etiketten sind nicht-leer, nicht der Fallback und paarweise verschieden", etiketten_ok);
        eq("Klassen-Zahl (Single-Source)", cem::kDockErrorClassCount, std::size_t{7});

        // (c) die Domaene bleibt D2 -- fuer JEDE Klasse, nicht nur fuer die geprobte.
        bool domaene_ok = true;
        for (std::size_t i = 0; i < cem::kDockErrorClassCount; ++i)
            if (cem::error_domain(static_cast<cem::DockErrorClass>(i)) != cem::ErrorDomain::Sample) domaene_ok = false;
        tr("jede Dock-Fehlerklasse liegt in der D2-Domaene (Sample)", domaene_ok);
    }

    std::cout << "\n== FK-7: " << (g_fail == 0 ? "ALLE PROBEN GRUEN" : "FEHLER") << " (" << g_fail << " Fehler) ==\n";
    return g_fail == 0 ? 0 : 1;
}
