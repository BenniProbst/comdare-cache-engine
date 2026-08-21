// test_lg_load_wache_genus_kontrast -- B09(a) (K01, Designplan par.4 Z38 "LG-LoadWache beide
// Genera", Klasse K1 keine-negativprobe): die LADE-WACHE mit BEIDEN Genera und dem
// FREMD-GENUS-KONTRAST in beide Richtungen -- je mit Wiederholungslauf.
//
// VORBESTAND (Session-Befund 18.08., am Objekt bestaetigt): test_e24_c10_g5_lade_wache prueft NUR
// das Set-Genus (Lebend-Modul perm_set_d9, einziger Genus-Durchstich ISetTier); der Nachbar
// test_e24_c10_genus_dll_roundtrip faehrt vier Genera, aber nicht die Lade-Wache. Die
// K1-Heilform verlangt die NEGATIVPROBE: die Wache MUSS beissen, wenn ein Modul des FREMDEN
// Genus am falschen Durchstich behandelt wird.
//
// WAS DIESER TEST ZUSICHERT (main-Stil wie G5; Module als argv, ctest verdrahtet TARGET_FILE):
//   (1) BEIDE GENERA POSITIV: perm_set_d9 laedt ok + genus()==Set + ISetTier-Durchstich traegt;
//       perm_sequence_d10 laedt ok + genus()==Sequence + ISequenceTier-Durchstich traegt.
//   (2) KONTRAST (die Wache beisst, BEIDE Richtungen): dynamic_cast<ISequenceTier*> auf dem
//       Set-Modul ist nullptr; dynamic_cast<ISetTier*> auf dem Sequence-Modul ist nullptr.
//       Ohne diese Rueckrichtung waere "beide Genera" nur eine doppelte Anwesenheitsprobe.
//   (3) WIEDERHOLUNGSLAUF: derselbe Vier-Urteile-Satz in einem ZWEITEN Lade-Durchgang --
//       die Wache ist zustandsfrei stabil (kein Erstlade-Artefakt).
//
// T-11c-MUTATIONSANKER: den Kontrast-Vergleich im Test invertieren (Fremd-Durchstich als
// Erwartung) bricht (2) literal -- und beweist damit, dass die Urteile echt gelesen werden.

#include "anatomy_module_loader/anatomy_module_loader.hpp"

#include <anatomy/sequence_tier.hpp>
#include <anatomy/set_tier.hpp>

#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

namespace {

namespace al  = ::comdare::cache_engine::builder::anatomy_loader;
namespace ana = ::comdare::cache_engine::anatomy;

int g_fail = 0;

void tr(char const* w, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n";
    if (!c) ++g_fail;
}

[[nodiscard]] std::string arg_wert(int argc, char** argv, std::string_view praefix) {
    for (int i = 1; i < argc; ++i) {
        std::string_view const s{argv[i]};
        if (s.rfind(praefix, 0) == 0) return std::string{s.substr(praefix.size())};
    }
    return {};
}

/// Der Vier-Urteile-Satz EINES Durchgangs (positiv je Genus + Kontrast je Richtung).
void ein_durchgang(std::string const& set_so, std::string const& seq_so, char const* etikett) {
    std::cout << "\n-- Durchgang " << etikett << " --\n";
    {
        al::AnatomyModuleHandle h;
        int const               st = al::AnatomyModuleLoader::load(set_so, h);
        tr("Set-Modul laedt (status_ok)", st == al::status_ok);
        tr("Set-Modul: Handle valid", h.valid());
        auto* base = h.valid() ? h.anatomy() : nullptr;
        tr("Set-Modul: anatomy() != nullptr", base != nullptr);
        if (base != nullptr) {
            tr("Set-Modul: genus() == Set", base->genus() == ana::AnatomyGenus::Set);
            tr("Set-Modul: ISetTier-Durchstich TRAEGT", dynamic_cast<ana::ISetTier*>(base) != nullptr);
            tr("KONTRAST: ISequenceTier auf dem Set-Modul BEISST (nullptr)",
               dynamic_cast<ana::ISequenceTier*>(base) == nullptr);
        }
    }
    {
        al::AnatomyModuleHandle h;
        int const               st = al::AnatomyModuleLoader::load(seq_so, h);
        tr("Sequence-Modul laedt (status_ok)", st == al::status_ok);
        tr("Sequence-Modul: Handle valid", h.valid());
        auto* base = h.valid() ? h.anatomy() : nullptr;
        tr("Sequence-Modul: anatomy() != nullptr", base != nullptr);
        if (base != nullptr) {
            tr("Sequence-Modul: genus() == Sequence", base->genus() == ana::AnatomyGenus::Sequence);
            tr("Sequence-Modul: ISequenceTier-Durchstich TRAEGT", dynamic_cast<ana::ISequenceTier*>(base) != nullptr);
            tr("KONTRAST: ISetTier auf dem Sequence-Modul BEISST (nullptr)",
               dynamic_cast<ana::ISetTier*>(base) == nullptr);
        }
    }
}

} // namespace

int main(int argc, char** argv) {
    std::cout << "==== B09(a)/LG-LoadWache: beide Genera + Fremd-Genus-Kontrast ====\n";
    std::string const set_so = arg_wert(argc, argv, "--set=");
    std::string const seq_so = arg_wert(argc, argv, "--sequence=");
    if (set_so.empty() || seq_so.empty()) {
        std::cerr << "usage: test_lg_load_wache_genus_kontrast --set=<so> --sequence=<so>\n";
        return 2;
    }
    ein_durchgang(set_so, seq_so, "1 (Erstlauf)");
    ein_durchgang(set_so, seq_so, "2 (Wiederholungslauf)");
    std::cout << "\n==== Ergebnis: " << (g_fail == 0 ? "ALLE URTEILE OK" : "FEHLER") << " (rot=" << g_fail
              << ") ====\n";
    return g_fail == 0 ? 0 : 1;
}
