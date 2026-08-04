// test_e24_c5_fk8_genus_baupfad_d1 -- E-24 C5 (b/2), Bauplan-Dossier docs/sessions/
// 20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 6.2 (FK-8) + Paragraf 4.2 (Klasse-II-Wache C5:
// "FK-Count-Wachen im SELBEN Commit wie die Enum-Erweiterung; Token-Disjunktheits-Wache").
//
// WAS BELEGT WIRD:
//   (1) DIE COUNT-WACHE: die D1-Taxonomie ist von 5 auf 7 gewachsen, die Bestands-Nummern 0..4 stehen
//       still, und die Etiketten der beiden neuen Klassen sind zementiert. Die Erweiterung ist ADDITIV
//       am ENDE -- Nummern reisen in Experiment-Logs, ein Einschieben waere ein stiller Bruch (RF-3).
//   (2) DIE TOKEN-DISJUNKTHEIT: die neuen D1-Etiketten kollidieren mit KEINEM anderen Vokabular --
//       weder mit D2-Zell-Tokens noch mit Log-Etiketten, Zulassungs-, Bau- oder Dock-Tokens. Die Probe
//       laeuft ueber die Count-Single-Sources, nicht ueber eine handgepflegte Liste.
//   (3) DAS GATE AN ECHTEN WERTEN: die Aritaets-Urteile werden gegen GenusBindingTraits<G>::slot_count
//       gefahren, nicht gegen im Test notierte Zahlen. Ein Slot-Umbau bricht damit hier, nicht spaeter.
//   (4) DIE GRAPH-AUSSAGE: die Ebene-1-Gattung Graph existiert als Enumerator, hat aber KEINE
//       Tier-Unterklasse -- ein Bau-Wunsch darauf ist ein deklarierter D1-Zustand statt eines Nichts.
//   (5) DIE MELDUNG: ein abgelehnter Baupfad traegt BuildCellStatus::NichtGebaut ("nicht_gebaut") plus
//       eine klassifizierte Log-Zeile; ein zugelassener bleibt byte-identisch zum Bestand (Gebaut == 0).
//   (6) DIE TRENNUNG DER BEIDEN C5-HAELFTEN: FK-8 sagt "nicht gebaut" (D1), FK-7 sagt "failed" (D2) --
//       nie dasselbe Wort, nie dieselbe Domaene. Ohne diese Wache koennte eine Auswertung "nie gebaut"
//       nicht mehr von "gemessen und gescheitert" unterscheiden (W-4-Doktrin).
//
// Registriert in tests/unit/CMakeLists.txt (COMDARE_GOALV6_BOOST_DTESTS -> add_test +
// COMDARE_TEST_TARGETS): keine Waisen-TU (Auflage 13).

#include "builder/experiment_tree/genus_build_admission.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

// TU-lokale Test-Helfer: anonymer Namespace verhindert ODR-Kollisionen der gleichnamigen Helfer
// ueber die e24-Test-TUs (cppcheck ctuOneDefinitionRuleViolation, Lint-Klasse 14398).
namespace {

namespace cex = comdare::cache_engine::builder::experiment;
namespace cea = comdare::cache_engine::anatomy;
namespace cem = comdare::cache_engine::measurement;

using Klasse = cem::CompilerCompilerErrorClass;

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

} // namespace

int main() {
    std::cout << "== E-24 C5 / FK-8: Gattungs-Baupfade melden in D1 (Klasse + 'nicht_gebaut' + Log) ==\n";

    // -----------------------------------------------------------------------------------------------
    // (1) Die Count-Wache: additiv am Ende, Bestands-Nummern still.
    // -----------------------------------------------------------------------------------------------
    std::cout << "\n-- (1) Enum-Erweiterung ADDITIV + Count-Wache --\n";
    eq("D1-Klassenzahl (Single-Source)", cem::kCompilerCompilerErrorClassCount, std::size_t{7});
    eq("GattungsBindungFehlt", static_cast<int>(Klasse::GattungsBindungFehlt), 5);
    eq("GattungsSlotAritaet", static_cast<int>(Klasse::GattungsSlotAritaet), 6);
    tr("die fuenf Bestands-Nummern 0..4 sind unveraendert",
       static_cast<int>(Klasse::KonfigXmlParse) == 0 && static_cast<int>(Klasse::ToolchainFehlt) == 1 &&
           static_cast<int>(Klasse::HardwareErweiterungFehlt) == 2 &&
           static_cast<int>(Klasse::CompileKombination) == 3 &&
           static_cast<int>(Klasse::BetriebssystemFeatureFehlt) == 4);
    eq("Etikett GattungsBindungFehlt", cem::error_class_label(Klasse::GattungsBindungFehlt),
       std::string_view{"gattungs_bindung_fehlt"});
    eq("Etikett GattungsSlotAritaet", cem::error_class_label(Klasse::GattungsSlotAritaet),
       std::string_view{"gattungs_slot_aritaet"});
    // Die ANHAENGE-Wache aus der anderen Richtung (RF-3-Lehre): hinter dem Count darf kein Etikett liegen.
    eq("hinter dem Count liegt kein Etikett",
       cem::error_class_label(static_cast<Klasse>(cem::kCompilerCompilerErrorClassCount)),
       std::string_view{"unbekannt"});
    // Die D2-Taxonomien bleiben von FK-8 unberuehrt (disjunkte Domaenen).
    eq("SampleStatus-Zahl unveraendert", cem::kSampleStatusCount, std::size_t{4});
    eq("BuildCellStatus-Zahl unveraendert", cem::kBuildCellStatusCount, std::size_t{2});

    // -----------------------------------------------------------------------------------------------
    // (2) Token-Disjunktheit -- ueber die Count-Single-Sources, nicht ueber eine Liste.
    // -----------------------------------------------------------------------------------------------
    std::cout << "\n-- (2) Token-Disjunktheit der neuen Etiketten --\n";
    {
        bool paarweise = true;
        for (std::size_t i = 0; i < cem::kCompilerCompilerErrorClassCount; ++i)
            for (std::size_t j = i + 1; j < cem::kCompilerCompilerErrorClassCount; ++j)
                if (cem::error_class_label(static_cast<Klasse>(i)) == cem::error_class_label(static_cast<Klasse>(j)))
                    paarweise = false;
        tr("alle D1-Etiketten sind paarweise verschieden", paarweise);

        // Gegen JEDES fremde Vokabular: die zentrale Disjunktheits-Wache muss die neuen Etiketten sehen
        // (sie liegen jetzt IN ihrer Schleife -> sie meldet sie als nicht-disjunkt; das ist der Beweis,
        // dass sie mitwaechst und nicht an ihnen vorbeilaeuft).
        tr("die zentrale Wache sieht gattungs_bindung_fehlt",
           !cem::probe_label_ist_disjunkt(cem::error_class_label(Klasse::GattungsBindungFehlt)));
        tr("die zentrale Wache sieht gattungs_slot_aritaet",
           !cem::probe_label_ist_disjunkt(cem::error_class_label(Klasse::GattungsSlotAritaet)));

        // Und explizit gegen die Zell-Vokabeln, die in DERSELBEN CSV stehen koennten.
        bool gegen_zellen = true;
        for (std::size_t i = 5; i < cem::kCompilerCompilerErrorClassCount; ++i) {
            auto const l = cem::error_class_label(static_cast<Klasse>(i));
            if (l == cem::sample_status_token(cem::SampleStatus::Failed)) gegen_zellen = false;
            if (l == cem::admission_status_token(cem::AdmissionStatus::Gesperrt)) gegen_zellen = false;
            if (l == cem::build_cell_status_token(cem::BuildCellStatus::NichtGebaut)) gegen_zellen = false;
            if (l == cem::dock_error_label(cem::DockErrorClass::FremdeGattung)) gegen_zellen = false;
        }
        tr("die neuen D1-Etiketten kollidieren mit keinem Zell-/Dock-Vokabular", gegen_zellen);
    }

    // -----------------------------------------------------------------------------------------------
    // (3) Das Gate an ECHTEN Slot-Zahlen (aus GenusBindingTraits, nicht aus dieser Datei).
    // -----------------------------------------------------------------------------------------------
    std::cout << "\n-- (3) Aritaets-Urteile gegen die reale Bau-Bindung --\n";
    {
        cea::AnatomyGenus const alle[]        = {cea::AnatomyGenus::SearchAlgorithm, cea::AnatomyGenus::Set,
                                                 cea::AnatomyGenus::Sequence, cea::AnatomyGenus::Adapter,
                                                 cea::AnatomyGenus::View};
        bool                    alle_gebunden = true;
        bool                    soll_passt    = true;
        for (auto const g : alle) {
            std::size_t const soll = cex::genus_build_slot_count(g);
            std::cout << "  " << cea::genus_name(g) << ": slot_count=" << soll << "\n";
            if (cex::admit_genus_build_path(g).has_value()) alle_gebunden = false;
            if (soll == 0) alle_gebunden = false;
            // Das Soll trifft, ein Soll+1 trifft nicht -- fixture-unabhaengig aus der Bindung abgeleitet.
            if (cex::admit_genus_slot_arity(g, soll).has_value()) soll_passt = false;
            if (cex::admit_genus_slot_arity(g, soll + 1) != Klasse::GattungsSlotAritaet) soll_passt = false;
        }
        tr("alle fuenf Ebene-2-Gattungen sind bau-gebunden (Gate ist gegenueber dem Bestand inert)", alle_gebunden);
        tr("je Gattung: die eigene Slot-Zahl wird zugelassen, jede andere Aritaet faellt in GattungsSlotAritaet",
           soll_passt);

        // Der Kreuz-Fall, der die beiden neuen Klassen auseinanderhaelt: ein Sequence-Achsensatz an der
        // Set-Gattung ist ein ARITAETS-Fehler (die Gattung gibt es ja), kein BINDUNGS-Fehler.
        eq("Sequence-Aritaet an der Set-Gattung",
           cem::error_class_label(*cex::admit_genus_slot_arity(
               cea::AnatomyGenus::Set, cex::genus_build_slot_count(cea::AnatomyGenus::Sequence))),
           std::string_view{"gattungs_slot_aritaet"});
    }

    // -----------------------------------------------------------------------------------------------
    // (4) Graph: Enumerator vorhanden, Baupfad nicht -- und das wird GEMELDET.
    // -----------------------------------------------------------------------------------------------
    std::cout << "\n-- (4) die Graph-Gattung (Q5, nach Abgabe) --\n";
    {
        auto const graph = cex::admit_gattung_build_path(cea::AnatomyGattung::Graph);
        tr("Graph ist NICHT baubar", graph.has_value());
        eq("Graph -> Klasse", cem::error_class_label(*graph), std::string_view{"gattungs_bindung_fehlt"});
        tr("Container ist baubar", !cex::admit_gattung_build_path(cea::AnatomyGattung::Container).has_value());
        tr("SearchAlgorithm ist baubar",
           !cex::admit_gattung_build_path(cea::AnatomyGattung::SearchAlgorithm).has_value());
        // Der Enumerator selbst bleibt unangetastet (TABU: Anatomie-Enum-Reihenfolge, Graph bleibt = 2).
        eq("AnatomyGattung::Graph steht still", static_cast<int>(cea::AnatomyGattung::Graph), 2);
    }

    // -----------------------------------------------------------------------------------------------
    // (5)+(6) Die Meldung: Zell-Status + Log-Zeile, und die Trennung zu FK-7.
    // -----------------------------------------------------------------------------------------------
    std::cout << "\n-- (5)/(6) die D1-Meldung und ihre Abgrenzung gegen FK-7 --\n";
    {
        auto const abgelehnt = cex::genus_build_verdict(cea::AnatomyGenus::Set, 0);
        tr("abgelehnter Baupfad ist nicht zugelassen", !abgelehnt.zugelassen());
        eq("abgelehnt -> Zell-Token", cex::genus_build_cell(abgelehnt), std::string_view{"nicht_gebaut"});
        std::string const log = cex::genus_build_log_line(abgelehnt, cea::AnatomyGenus::Set, 0);
        std::cout << "  LOG literal: " << log << "\n";
        tr("Log traegt das D1-Praefix", log.find("Compiler-Compiler-Fehler[") == 0);
        tr("Log traegt das Klassen-Etikett", log.find("gattungs_slot_aritaet") != std::string::npos);
        tr("Log nennt Gattung und Soll-Aritaet",
           log.find("gattung=Set") != std::string::npos && log.find("aritaet=0/13") != std::string::npos);
        tr("Log nennt die Zelle", log.find("zelle=nicht_gebaut") != std::string::npos);

        auto const zugelassen =
            cex::genus_build_verdict(cea::AnatomyGenus::Set, cex::genus_build_slot_count(cea::AnatomyGenus::Set));
        tr("zugelassener Baupfad bleibt byte-identisch zum Bestand (Gebaut == Default)",
           zugelassen.zugelassen() && zugelassen.build_status == cem::BuildCellStatus::Gebaut);
        tr("ein zugelassener Baupfad erzeugt KEINE Log-Zeile (kein Log-Rauschen)",
           cex::genus_build_log_line(zugelassen, cea::AnatomyGenus::Set, 13).empty());

        // (6) Die beiden C5-Haelften duerfen nie zusammenlaufen.
        tr("D1-Zelle 'nicht_gebaut' != D2-Zelle 'failed'",
           cex::genus_build_cell(abgelehnt) != cem::sample_status_token(cem::SampleStatus::Failed));
        tr("D1-Domaene != D2-Domaene",
           cem::error_domain(Klasse::GattungsBindungFehlt) != cem::error_domain(cem::DockErrorClass::FremdeGattung));
        eq("FK-8 liegt in der D1-Domaene", static_cast<int>(cem::error_domain(Klasse::GattungsSlotAritaet)),
           static_cast<int>(cem::ErrorDomain::CompilerCompiler));
    }

    std::cout << "\n== FK-8: " << (g_fail == 0 ? "ALLE PROBEN GRUEN" : "FEHLER") << " (" << g_fail << " Fehler) ==\n";
    return g_fail == 0 ? 0 : 1;
}
