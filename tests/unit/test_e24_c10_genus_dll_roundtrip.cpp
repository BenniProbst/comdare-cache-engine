// test_e24_c10_genus_dll_roundtrip -- E-24 C10 (a), Bauplan-Dossier docs/sessions/
// 20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 3.2-C10: "je Genus DLL-Roundtrip (bauen -> laden
// 7-Schritt -> messen -> CSV); wrong_genus-/Konformitaets-Negativ-Proben (dock_status 2/4); select_for
// ueber alle 5 Gattungen + measure_genus_sequential-Mischlauf".
//
// WAS HIER NEU IST -- und warum es nicht schon dastand:
//   * test_dgenus_dll faehrt seit L-76 drei Container-DLLs (Set/Sequence/View) ueber den Loader und
//     treibt sie DIREKT ueber ihr Sub-Interface. Das belegt den Lade-Pfad, aber NICHT den Mess-Pfad:
//     kein Dock, kein Konformitaets-Gate, keine CSV. Und Adapter fehlte ganz (perm_adapter_d12 ist
//     C10 (a), genus_module_adapter.cpp).
//   * test_e24_c4_genus_pruef_docks faehrt den vollen V5-Vertrag (import -> GATE -> messen) ueber alle
//     vier Container-Docks, aber IN-PROCESS: die Handles zeigen auf lokal gebaute ABI-Adapter
//     (native == nullptr). Das belegt die Vertrags-Reihenfolge, nicht die Naht ueber die .so-Grenze.
// Diese TU schliesst die Klammer: DIESELBEN vier Docks, dieselbe Registry, dieselbe
// measure_genus_sequential-Funktion -- aber ueber vier ECHT dlopen-geladene Module. Ein Fehler in der
// Symbol-Aufloesung, der vtable-Sicht ueber die Bibliotheks-Grenze oder der dynamic_cast-Naht (typeinfo
// ueber .so-Grenzen) wird ausschliesslich hier sichtbar.
//
// AUFRUFFORM (die Pfade kommen als $<TARGET_FILE:..> aus tests/unit/CMakeLists.txt -- kein Lade-Ort-
// Raetselraten, Muster des L-76-Blocks):
//     test_e24_c10_genus_dll_roundtrip --gut=<so> [--gut=<so> ...] --defekt-set=<so>
//
// Registriert in tests/unit/CMakeLists.txt (add_test + COMDARE_TEST_TARGETS): keine Waisen-TU (Auflage 13).

#include "builder/pruef_dock/adapter_dock.hpp"
#include "builder/pruef_dock/dock_error_classification.hpp"
#include "builder/pruef_dock/pruef_dock_registry.hpp"
#include "builder/pruef_dock/pruef_dock_registry_default.hpp"
#include "builder/pruef_dock/pruef_dock_sequencer.hpp"
#include "builder/pruef_dock/sequence_dock.hpp"
#include "builder/pruef_dock/set_dock.hpp"
#include "builder/pruef_dock/view_dock.hpp"

#include <builder/anatomy_module_loader/anatomy_module_loader.hpp>

#include <anatomy/anatomy_base.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

// TU-lokale Fixtures/Helfer in anonymem Namespace (cppcheck ctuOneDefinitionRuleViolation, Lint-Klasse 14398).
namespace {

namespace ana = comdare::cache_engine::anatomy;
namespace pd  = comdare::cache_engine::builder::pruef_dock;
namespace al  = comdare::cache_engine::builder::anatomy_loader;

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

/// Die ERSTE Zeile einer CSV -- die Kopfzeile ist der Beleg, dass eine Messung stattfand, und sie
/// gehoert literal ins Log (kein Haken ohne Ausgabe).
[[nodiscard]] std::string erste_zeile(std::string const& s) {
    auto const nl = s.find('\n');
    return nl == std::string::npos ? s : s.substr(0, nl);
}

/// Die gattungs-eigenen Observer-Spalten, die die Mess-CSV je Genus tragen MUSS. Sie stehen hier als
/// Erwartung des AUFRUFERS und nicht als Kopie aus dem Dock -- eine stille Umbenennung der Spalten
/// bricht damit hier, statt in einer Auswertung Monate spaeter.
[[nodiscard]] std::string_view erwartete_spalten(ana::AnatomyGenus g) noexcept {
    switch (g) {
        case ana::AnatomyGenus::Set: return "set_insert,set_contains,set_hit,set_miss,set_erase";
        case ana::AnatomyGenus::Sequence: return "seq_push,seq_at,seq_at_oob";
        case ana::AnatomyGenus::Adapter: return "adp_push,adp_pop,adp_front_reads";
        case ana::AnatomyGenus::View: return "view_read,view_read_oob,view_bound_size";
        case ana::AnatomyGenus::SearchAlgorithm: return "";
        // HY-A1-NACHZUG (Warnungs-Review Runde 2b, 09.08.2026). clang meldete hier:
        //   warning: enumeration value 'FunctionInterfaceReroute' not handled in switch [-Wswitch]
        // Der Fall ist ausgeschrieben statt weggelassen, obwohl er dasselbe LIEFERT wie der
        // Fall-through -- weil "leer, weil unerreichbar" und "leer, weil vergessen" sonst dieselbe
        // Zeile waeren. Er ist hier UNERREICHBAR und das steht in anatomy_base.hpp an genus():
        // eine Hybrid-Tier-Binary gibt aus genus() das GEERBTE ZIEL-Genus zurueck und "NIEMALS
        // AnatomyGenus::FunctionInterfaceReroute" (Owner-Entscheid E-1 final, Weg C). Dieser Test
        // faehrt ausschliesslich ueber genus() der geladenen .so. Kaeme der Wert hier doch an,
        // waere die Klassifikations-Partition gebrochen -- dann ist die leere Spaltenliste das
        // richtige Signal: das Dock kennt fuer ihn keine Observer-Spalten.
        case ana::AnatomyGenus::FunctionInterfaceReroute: return "";
    }
    return "";
}

struct Argumente {
    std::vector<std::string> gut{};
    std::string              defekt_set{};
};

[[nodiscard]] Argumente parse(int argc, char** argv) {
    Argumente a;
    for (int i = 1; i < argc; ++i) {
        std::string_view const s{argv[i]};
        if (s.rfind("--gut=", 0) == 0) {
            a.gut.emplace_back(s.substr(std::string_view{"--gut="}.size()));
        } else if (s.rfind("--defekt-set=", 0) == 0) {
            a.defekt_set = std::string{s.substr(std::string_view{"--defekt-set="}.size())};
        }
    }
    return a;
}

} // namespace

int main(int argc, char** argv) {
    std::cout << "==== E-24 C10 (a): DLL-Roundtrip je Container-Genus (bauen -> laden -> messen -> CSV) ====\n";

    Argumente const args = parse(argc, argv);
    if (args.gut.empty() || args.defekt_set.empty()) {
        std::cerr << "usage: test_e24_c10_genus_dll_roundtrip --gut=<so> [--gut=<so> ...] --defekt-set=<so>\n";
        return 2;
    }

    pd::PruefDockMeasureOptions opts;
    opts.fill_checkpoints       = {8, 32};
    opts.lookups_per_checkpoint = 64;
    opts.deletes_per_checkpoint = 8;

    pd::PruefDockRegistry reg;
    pd::register_all_genus_docks(reg);
    eq("Registry-Groesse (alle fuenf Ebene-2-Gattungen)", reg.size(), std::size_t{5});

    // ----------------------------------------------------------------------------------------------
    // (1) LADEN: die 7-Schritt-Validierung des gattungs-agnostischen Loaders, je Modul literal.
    // ----------------------------------------------------------------------------------------------
    std::cout << "\n-- Schritt 1: Laden (AnatomyModuleLoader, 7-Schritt-Validierung) --\n";
    std::vector<al::AnatomyModuleHandle> handles;
    handles.reserve(args.gut.size());
    for (auto const& p : args.gut) {
        al::AnatomyModuleHandle h;
        int const               st = al::AnatomyModuleLoader::load(p, h);
        std::cout << "  Modul " << p << "\n";
        eq("    load()-Status", std::string{al::status_name(st)}, std::string{"ok"});
        if (st != al::status_ok) continue;
        tr("    handle.valid() (native-Handle UND Anatomie-Zeiger)", h.valid());
        auto const v = h.module_version();
        std::cout << "    Modul-ABI-Version = " << static_cast<unsigned>(v.major) << "."
                  << static_cast<unsigned>(v.minor) << "\n";
        eq("    Modul-Major == Host-Major (COMDARE_ANATOMY_ABI_MAJOR)", static_cast<int>(v.major),
           static_cast<int>(COMDARE_ANATOMY_ABI_MAJOR));
        if (h.anatomy() != nullptr) {
            std::cout << "    genus()   = " << ana::genus_name(h.anatomy()->genus()) << "\n";
            std::cout << "    Gattung   = " << ana::gattung_name(ana::gattung_of(h.anatomy()->genus())) << "\n";
        }
        handles.push_back(std::move(h));
    }

    // VOLLSTAENDIGKEITS-WACHE ueber die ENUM-Werte statt ueber eine Aufzaehlung im Test: kaeme ein
    // sechstes Container-Genus dazu und niemand baute ihm eine DLL, bricht das hier.
    {
        constexpr std::array<ana::AnatomyGenus, 4> kContainerGenera{
            ana::AnatomyGenus::Set, ana::AnatomyGenus::Sequence, ana::AnatomyGenus::Adapter, ana::AnatomyGenus::View};
        bool alle_da = true;
        for (auto const g : kContainerGenera) {
            std::size_t n = 0;
            for (auto const& h : handles) {
                if (h.anatomy() != nullptr && h.anatomy()->genus() == g) ++n;
            }
            if (n != 1) {
                alle_da = false;
                std::cout << "  [ERR] genau EINE geladene DLL je Container-Genus: " << ana::genus_name(g) << " -> " << n
                          << "\n";
            }
            // Ebene-1-Ableitung (C7-6): jedes Container-Genus faellt auf die Gattung Container.
            if (ana::gattung_of(g) != ana::AnatomyGattung::Container) alle_da = false;
        }
        tr("je Container-Genus genau EINE geladene DLL, alle unter der Gattung Container", alle_da);
    }

    // ----------------------------------------------------------------------------------------------
    // (2) MESSEN: select_for -> Dock -> V5-Vertrag -> CSV, je geladenem Modul.
    // ----------------------------------------------------------------------------------------------
    std::cout << "\n-- Schritt 2: select_for -> Dock -> messen -> CSV (V5-Vertrag ueber die .so-Grenze) --\n";
    for (auto& h : handles) {
        if (h.anatomy() == nullptr) continue;
        auto const  g    = h.anatomy()->genus();
        auto* const dock = reg.select_for(h);
        std::cout << "  Genus " << ana::genus_name(g) << ":\n";
        tr("    select_for liefert ein Dock", dock != nullptr);
        if (dock == nullptr) continue;
        eq("    Dock-Gattung == Modul-Gattung", std::string{ana::genus_name(dock->dock_genus())},
           std::string{ana::genus_name(g)});
        std::cout << "    dock_name = " << dock->dock_name() << "\n";

        std::string csv, json;
        int const   rc = dock->measure(h, opts, csv, json);
        eq("    measure()-Status", std::string{pd::dock_status_name(rc)}, std::string{"ok"});
        tr("    CSV ist nicht leer", !csv.empty());
        tr("    JSON ist nicht leer", !json.empty());
        std::cout << "    CSV-Kopf: " << erste_zeile(csv) << "\n";
        tr("    CSV traegt den geteilten Zeit-Kopf", csv.rfind("checkpoint,observe_wall_ns,fill_level", 0) == 0);
        auto const spalten = erwartete_spalten(g);
        tr("    CSV traegt die gattungs-eigenen Observer-Spalten", csv.find(spalten) != std::string::npos);
    }

    // ----------------------------------------------------------------------------------------------
    // (3) NEGATIV-PROBE wrong_genus (dock_status 2) UEBER DIE DLL-GRENZE.
    // ----------------------------------------------------------------------------------------------
    std::cout << "\n-- Schritt 3: Negativ-Probe wrong_genus (2) an echten Modulen --\n";
    {
        pd::SequencePruefDock seq_dock;
        pd::ViewPruefDock     view_dock;
        bool                  gesehen = false;
        for (auto& h : handles) {
            if (h.anatomy() == nullptr || h.anatomy()->genus() != ana::AnatomyGenus::Set) continue;
            gesehen = true;
            std::string csv{"VORBELEGT"};
            std::string json{"VORBELEGT"};
            tr("  das Sequence-Dock akzeptiert das Set-Modul NICHT", !seq_dock.accepts(h));
            eq("  Set-DLL am Sequence-Dock", std::string{pd::dock_status_name(seq_dock.measure(h, opts, csv, json))},
               std::string{"wrong_genus"});
            eq("  Set-DLL am View-Dock", view_dock.measure(h, opts, csv, json), pd::dock_status_wrong_genus);
            eq("  wrong_genus ist 2 (errno-Stil)", pd::dock_status_wrong_genus, 2);
            eq("  out_csv unberuehrt (kein Mess-Artefakt aus einer abgelehnten Gattung)", csv,
               std::string{"VORBELEGT"});
            // FK-7 (C5): der Transport-Int wird KLASSIFIZIERT und die Zelle traegt "failed", nie eine Null.
            auto const klasse = pd::classify_dock_status(pd::dock_status_wrong_genus);
            tr("  FK-7: wrong_genus traegt eine D2-Fehlerklasse", klasse.has_value());
            if (klasse.has_value()) {
                std::cout << "  FK-7-Log: "
                          << pd::dock_failure_log_line(*klasse, "perm_set_d9",
                                                       pd::dock_status_name(pd::dock_status_wrong_genus))
                          << "\n";
                eq("  FK-7-CSV-Zelle", std::string{pd::dock_failure_cell(*klasse)}, std::string{"failed"});
            }
        }
        tr("  eine Set-DLL stand fuer die wrong_genus-Probe bereit", gesehen);
    }

    // ----------------------------------------------------------------------------------------------
    // (4) NEGATIV-PROBE conformance_failed (dock_status 4) UEBER DIE DLL-GRENZE.
    //     Das defekte Modul LAEDT sauber (es luegt nicht an der ABI) und faellt erst am Gattungs-Orakel.
    // ----------------------------------------------------------------------------------------------
    std::cout << "\n-- Schritt 4: Negativ-Probe conformance_failed (4) an einer echt geladenen DLL --\n";
    {
        al::AnatomyModuleHandle bad;
        int const               st = al::AnatomyModuleLoader::load(args.defekt_set, bad);
        eq("  das DEFEKTE Set-Modul laedt sauber (der Defekt liegt nicht an der ABI)", std::string{al::status_name(st)},
           std::string{"ok"});
        if (st == al::status_ok && bad.anatomy() != nullptr) {
            eq("  seine Gattung ist Set", std::string{ana::genus_name(bad.anatomy()->genus())}, std::string{"Set"});
            pd::SetPruefDock set_dock;
            tr("  das Set-Dock AKZEPTIERT es (die Gattung stimmt -- das Gate weist es ab, nicht accepts())",
               set_dock.accepts(bad));
            std::string csv{"VORBELEGT"};
            std::string json{"VORBELEGT"};
            int const   rc = set_dock.measure(bad, opts, csv, json);
            eq("  measure()-Status", std::string{pd::dock_status_name(rc)}, std::string{"conformance_failed"});
            eq("  conformance_failed ist 4 (errno-Stil)", rc, pd::dock_status_conformance_failed);
            eq("  out_csv unberuehrt (V5: nach durchgefallenem Gate wird NICHT gemessen)", csv,
               std::string{"VORBELEGT"});
            eq("  out_json unberuehrt", json, std::string{"VORBELEGT"});
            auto const klasse = pd::classify_dock_status(rc);
            tr("  FK-7: conformance_failed traegt eine D2-Fehlerklasse", klasse.has_value());
            if (klasse.has_value()) {
                std::cout << "  FK-7-Log: "
                          << pd::dock_failure_log_line(*klasse, "perm_set_defekt_d13", pd::dock_status_name(rc))
                          << "\n";
                std::cout << "  FK-7-CSV:\n"
                          << pd::dock_failure_csv(*klasse, "perm_set_defekt_d13", pd::dock_status_name(rc));
            }
        }
    }

    // ----------------------------------------------------------------------------------------------
    // (5) MISCHLAUF: measure_genus_sequential ueber alle geladenen Module -- DIESELBE Funktion, die
    //     apps/f15_compare/main.cpp aufruft (keine im Test nachgebildete zweite Schleife).
    // ----------------------------------------------------------------------------------------------
    std::cout << "\n-- Schritt 5: measure_genus_sequential-Mischlauf ueber die geladenen DLLs --\n";
    {
        auto const results = pd::measure_genus_sequential(reg, handles, opts);
        eq("  ein Ergebnis je Handle", results.size(), handles.size());
        bool alle_ok = true;
        for (auto const& r : results) {
            std::cout << "    " << r.dock_name << " status=" << pd::dock_status_name(r.status)
                      << " csv_bytes=" << r.csv.size() << " json_bytes=" << r.json.size() << "\n";
            if (r.status != pd::dock_status_ok || r.csv.empty() || r.json.empty()) alle_ok = false;
        }
        tr("  jeder Mischlauf-Eintrag ist ok und traegt CSV+JSON", alle_ok);
    }

    std::cout << "\n==== E-24 C10 (a) DLL-Roundtrip: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
