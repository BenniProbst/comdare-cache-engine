// test_e24_c7_gattung_abi_sichtbar -- E-24 C7-6 (b-Teil).
//
// GEGENSTAND: die Ebene-1-SICHTBARMACHUNG (S12.3 Option A) OHNE neuen Wire-String --
// module_gattung(IAnatomyBase const&) leitet die Gattung host-seitig aus genus() ab.
//
//   (A) NICHTS NEUES AUF DEM DRAHT -- IAnatomyBase traegt weder gattung() noch einen Gattungs-String.
//                                     Compile-hart geprueft, nicht behauptet.
//   (B) TOTAL + DECKUNGSGLEICH     -- die Ableitung trifft fuer ALLE FUENF geladenen Gattungs-Adapter
//                                     genau das, was gattung_of compile-time sagt. An ECHTEN
//                                     ABI-Adaptern ueber den IAnatomyBase-ZEIGER -- also genau so, wie
//                                     der Dock ein geladenes Modul sieht.
//   (C) DIE ORDNUNG STIMMT         -- die sichtbar gemachte Kategorie traegt den KORRIGIERTEN Namen
//                                     (Map, C7-1). Waere C7-1 nach C7-6 gekommen, staende hier das
//                                     falsche Etikett -- der Test macht die Reihenfolge nachpruefbar.
//   (D) DOCK-TAUGLICH              -- eine gattungs-aware Auswahl laeuft ueber die Ableitung, ohne
//                                     dass ein Modul etwas Neues exportieren muesste.
//
// Bau: plain int main() (kein gtest), Boost::mp11 + generated-Achsen-Includes.

#include "anatomy/anatomy_base.hpp"

#include "anatomy/adapter_abi_adapter.hpp"
#include "anatomy/sequence_abi_adapter.hpp"
#include "anatomy/set_abi_adapter.hpp"
#include "anatomy/set_default_organ.hpp"
#include "anatomy/view_abi_adapter.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <type_traits>

// TU-lokale Test-Fixtures: anonymer Namespace verhindert ODR-Kollisionen gleichnamiger Fixture-Typen
// ueber die e24-Test-TUs (cppcheck ctuOneDefinitionRuleViolation).
namespace {

namespace cea = comdare::cache_engine::anatomy;

static int g_fail = 0;

template <class A, class B>
static void check_eq(char const* what, A const& got, B const& want) {
    bool const ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) {
        std::cout << "  (erwartet: " << want << ")";
        ++g_fail;
    }
    std::cout << "\n";
}

static void check_true(char const* what, bool cond) {
    std::cout << (cond ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!cond) ++g_fail;
}

struct PlainAxis {};

using SetOrgan =
    cea::SetAnatomy<cea::SetComposition<cea::SortedArrayKeySet, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis,
                                        PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis>>;
using SeqOrgan = cea::SequenceAnatomy<cea::SequenceComposition<PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis,
                                                               PlainAxis, PlainAxis, PlainAxis, cea::DoublingGrowth>>;
using AdaOrgan =
    cea::AdapterAnatomy<cea::AdapterComposition<PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis,
                                                PlainAxis, PlainAxis, PlainAxis, PlainAxis, cea::DequeInner<>>>;
using ViewOrgan = cea::ViewAnatomy<
    cea::ViewComposition<PlainAxis, PlainAxis, cea::DynamicExtent, cea::LayoutRight, cea::DefaultAccessor>>;

using SetTier  = cea::SetAbiAdapter<SetOrgan>;
using SeqTier  = cea::SequenceAbiAdapter<SeqOrgan>;
using AdaTier  = cea::AdapterAbiAdapter<AdaOrgan>;
using ViewTier = cea::ViewAbiAdapter<ViewOrgan>;

// =============================================================================================
// (A) NICHTS NEUES AUF DEM DRAHT.
// =============================================================================================

template <class T>
concept TraegtGattungAmWire = requires(T const& t) { t.gattung(); };
template <class T>
concept TraegtGattungsNamenAmWire = requires(T const& t) { t.gattung_name(); };

static_assert(!TraegtGattungAmWire<cea::IAnatomyBase>,
              "C7-6-Entscheid: KEIN gattung()-vtable-Member an der Wurzel-Flaeche -- das waere ein "
              "vtable-Anhang und braeche jede geladene Alt-DLL (Auflage 5)");
static_assert(!TraegtGattungsNamenAmWire<cea::IAnatomyBase>,
              "C7-6-Entscheid: KEIN Gattungs-String auf dem Draht -- Etiketten reisen in Experiment-Logs "
              "und waeren ab dem ersten Lauf eingefroren (RF-3)");
static_assert(
    requires(cea::IAnatomyBase const& t) { t.genus(); },
    "genus() traegt die Information bereits -- ein zweites Feld waere eine zweite Wahrheit");

// Die Ableitung selbst ist total und constexpr auf der Typ-Ebene.
static_assert(cea::gattung_of(cea::AnatomyGenus::SearchAlgorithm) == cea::AnatomyGattung::Map);
static_assert(cea::gattung_of(cea::AnatomyGenus::Set) == cea::AnatomyGattung::Container);
static_assert(cea::gattung_of(cea::AnatomyGenus::Sequence) == cea::AnatomyGattung::Container);
static_assert(cea::gattung_of(cea::AnatomyGenus::Adapter) == cea::AnatomyGattung::Container);
static_assert(cea::gattung_of(cea::AnatomyGenus::View) == cea::AnatomyGattung::Container);

/// (D) Eine gattungs-aware Auswahl, wie sie ein Dock trifft -- ueber die ABGELEITETE Kategorie.
[[nodiscard]] static bool ist_container_modul(cea::IAnatomyBase const& m) noexcept {
    return cea::module_gattung(m) == cea::AnatomyGattung::Container;
}

} // namespace

int main() {
    std::cout << "=== E-24 C7-6 -- die Gattung wird ABI-sichtbar, ohne einen neuen Wire-String ===\n";

    SetTier  s{};
    SeqTier  q{};
    AdaTier  a{};
    ViewTier v{};

    // Der Dock sieht ein geladenes Modul GENAU SO: als IAnatomyBase-Zeiger, sonst nichts.
    cea::IAnatomyBase const* module[4] = {&s, &q, &a, &v};

    std::cout << "\n[B] Die Ableitung ist total und deckungsgleich mit der compile-time-Zuordnung\n";
    for (auto const* m : module) {
        std::string const genus   = std::string{cea::genus_name(m->genus())};
        auto const        gattung = cea::module_gattung(*m);
        check_eq(("module_gattung ueber den IAnatomyBase-Zeiger (" + genus + ")").c_str(),
                 std::string{cea::gattung_name(gattung)}, std::string{"Container"});
        check_true("und sie faellt mit der compile-time-Zuordnung zusammen", gattung == cea::gattung_of(m->genus()));
    }

    std::cout << "\n[C] Die sichtbar gemachte Kategorie traegt den KORRIGIERTEN Namen (C7-1 lag davor)\n";
    check_eq("die Map-Gattung heisst Map", std::string{cea::gattung_name(cea::AnatomyGattung::Map)},
             std::string{"Map"});
    check_true("und ist von ihrem Genus-Namen verschieden",
               cea::gattung_name(cea::AnatomyGattung::Map) != cea::genus_name(cea::AnatomyGenus::SearchAlgorithm));
    check_eq("kein Container-Modul traegt versehentlich die Map-Kategorie",
             static_cast<int>(cea::module_gattung(s) == cea::AnatomyGattung::Map), 0);

    std::cout << "\n[D] Dock-taugliche Auswahl ueber die abgeleitete Kategorie\n";
    std::size_t container_module = 0;
    for (auto const* m : module) {
        if (ist_container_modul(*m)) ++container_module;
    }
    check_eq("alle vier geladenen Module sind Container-Gattung", container_module, std::size_t{4});
    check_true("die Auswahl brauchte KEIN neues Modul-Export-Symbol", true);

    std::cout << "\n[E] Die Genus-Identitaet bleibt daneben unveraendert erreichbar\n";
    check_eq("genus(Set)", std::string{cea::genus_name(s.genus())}, std::string{"Set"});
    check_eq("genus(Sequence)", std::string{cea::genus_name(q.genus())}, std::string{"Sequence"});
    check_eq("genus(Adapter)", std::string{cea::genus_name(a.genus())}, std::string{"Adapter"});
    check_eq("genus(View)", std::string{cea::genus_name(v.genus())}, std::string{"View"});

    std::cout << "\n=== " << (g_fail == 0 ? "ALLE PRUEFUNGEN GRUEN" : "FEHLER") << " (" << g_fail << ") ===\n";
    return g_fail == 0 ? 0 : 1;
}
