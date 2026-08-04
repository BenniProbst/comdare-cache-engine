// test_e24_c7_gattung_map_umbenennung -- E-24 C7-1 (b-Teil).
//
// GEGENSTAND: die Ebene-1-UMBENENNUNG AnatomyGattung::SearchAlgorithm -> AnatomyGattung::Map
// (C7-Auflage C7-1, Diskrepanz-Dossier Abschnitt 5; Owner-KERN NACHTRAG 4, LEDGER:3836).
//
// WARUM DIESE TU UND NICHT NUR DIE FIXTURE-ANPASSUNG (Doktrin "Gruene Tests zementieren alte
// Ordnung"): die bestehenden Fixtures haben den alten Namen literal getragen -- sie mussten im
// Umbenennungs-Commit mitgezogen werden, und genau deshalb koennen sie die Umbenennung nicht mehr
// BEWEISEN (sie wurden ja mitgedreht). Diese TU ist der davon UNABHAENGIGE Ableitungsweg: sie prueft
// die drei Eigenschaften, die die Umbenennung ueberhaupt zulaessig machen, statt nur den neuen Namen
// zu wiederholen.
//
//   (A) ZAHLENWERTE UNANGETASTET -- die Enum-Reihenfolge ist TABU (E24-DOSSIER:168). Umbenannt wurde
//       ausschliesslich der NAME; Map == 0, Container == 1, Graph == 2 bleiben.
//   (B) EBENE 2 UNVERAENDERT     -- AnatomyGenus::SearchAlgorithm heisst weiter so (das GENUS heisst
//       so, NACHTRAG 4) -- die Umbenennung darf NICHT auf Ebene 2 durchschlagen.
//   (C) KEINE WIRE-EXPOSITION    -- IAnatomyBase transportiert KEIN gattung(): die Ebene-1-Kategorie
//       ist der Wire-Flaeche unbekannt und wird host-seitig constexpr aus genus() abgeleitet. GENAU
//       DESHALB ist die Umbenennung heute source-only. Waere sie ABI-sichtbar, waere sie ein Bruch --
//       und das ist der Grund, warum sie VOR der Sichtbarmachung (C7-6) liegen MUSS.
//   (D) VERERBUNGS-ZUORDNUNG     -- gattung_of ist total: jedes Genus liegt in genau einer Gattung,
//       und die Map-Gattung traegt GENAU EIN Genus (deshalb braucht sie kein Kopf-Framework, C7-4).
//
// Bau: plain int main() (kein gtest), Boost::mp11 + generated-Achsen-Includes.

#include "anatomy/anatomy_base.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string_view>
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

// (A) Die Zahlenwerte sind TABU -- umbenannt wurde ausschliesslich der Name.
static_assert(static_cast<std::uint8_t>(cea::AnatomyGattung::Map) == 0,
              "TABU: die Ebene-1-Enum-Reihenfolge bleibt unangetastet -- C7-1 benennt NUR um");
static_assert(static_cast<std::uint8_t>(cea::AnatomyGattung::Container) == 1);
static_assert(static_cast<std::uint8_t>(cea::AnatomyGattung::Graph) == 2);
static_assert(std::is_same_v<std::underlying_type_t<cea::AnatomyGattung>, std::uint8_t>);

// (B) Ebene 2 ist NICHT betroffen: das Genus heisst weiterhin SearchAlgorithm.
static_assert(static_cast<std::uint8_t>(cea::AnatomyGenus::SearchAlgorithm) == 0);
static_assert(cea::genus_name(cea::AnatomyGenus::SearchAlgorithm) == std::string_view{"SearchAlgorithm"},
              "NACHTRAG 4: das GENUS heisst SearchAlgorithm -- nur die GATTUNG wurde zu Map");

// (C) Die Wire-Flaeche kennt die Ebene 1 NICHT -- deshalb ist die Umbenennung source-only.
template <class T>
concept TraegtGattungAmWire = requires(T const& t) { t.gattung(); };
static_assert(!TraegtGattungAmWire<cea::IAnatomyBase>,
              "C7-1-Vorbedingung: IAnatomyBase transportiert kein gattung() -- die Umbenennung ist heute "
              "source-only. NACH der Ebene-1-Sichtbarmachung waere sie ein ABI-Bruch.");
static_assert(
    requires(cea::IAnatomyBase const& t) { t.genus(); }, "genus() bleibt die EINE Identitaets-Op der Wire-Flaeche");

// (D) gattung_of ist total und die Zuordnung ist die Vererbungs-Aussage aus NACHTRAG 4.
static_assert(cea::gattung_of(cea::AnatomyGenus::SearchAlgorithm) == cea::AnatomyGattung::Map);
static_assert(cea::gattung_of(cea::AnatomyGenus::Set) == cea::AnatomyGattung::Container);
static_assert(cea::gattung_of(cea::AnatomyGenus::Sequence) == cea::AnatomyGattung::Container);
static_assert(cea::gattung_of(cea::AnatomyGenus::Adapter) == cea::AnatomyGattung::Container);
static_assert(cea::gattung_of(cea::AnatomyGenus::View) == cea::AnatomyGattung::Container);

/// Zaehlt die Genera EINER Gattung -- aus gattung_of ABGELEITET, nicht als Literal gepflegt.
constexpr std::size_t genera_in(cea::AnatomyGattung g) noexcept {
    std::size_t n = 0;
    for (std::uint8_t i = 0; i <= static_cast<std::uint8_t>(cea::AnatomyGenus::View); ++i) {
        if (cea::gattung_of(static_cast<cea::AnatomyGenus>(i)) == g) ++n;
    }
    return n;
}
static_assert(genera_in(cea::AnatomyGattung::Map) == 1,
              "C7-4-Begruendung: die Map-Gattung hat GENAU EIN Genus -- eine Schnittmenge ueber ein "
              "Element IST das Element, also braucht Map kein eigenes Kopf-Framework");
static_assert(genera_in(cea::AnatomyGattung::Container) == 4);
static_assert(genera_in(cea::AnatomyGattung::Graph) == 0,
              "NACHTRAG 4: Graph ist Stub -- noch kein Genus implementiert");
static_assert(genera_in(cea::AnatomyGattung::Map) + genera_in(cea::AnatomyGattung::Container) +
                      genera_in(cea::AnatomyGattung::Graph) ==
                  5,
              "die Zuordnung ist TOTAL: jedes der fuenf Genera liegt in genau einer Gattung");

} // namespace

int main() {
    std::cout << "=== E-24 C7-1 -- Ebene-1-Umbenennung SearchAlgorithm -> Map ===\n";

    std::cout << "\n[A] Zahlenwerte unangetastet (Enum-Reihenfolge ist TABU)\n";
    check_eq("AnatomyGattung::Map", static_cast<int>(cea::AnatomyGattung::Map), 0);
    check_eq("AnatomyGattung::Container", static_cast<int>(cea::AnatomyGattung::Container), 1);
    check_eq("AnatomyGattung::Graph", static_cast<int>(cea::AnatomyGattung::Graph), 2);

    std::cout << "\n[A2] Der Ebene-1-STRING traegt den neuen Namen\n";
    check_eq("gattung_name(Map)", std::string{cea::gattung_name(cea::AnatomyGattung::Map)}, std::string{"Map"});
    check_eq("gattung_name(Container)", std::string{cea::gattung_name(cea::AnatomyGattung::Container)},
             std::string{"Container"});
    check_eq("gattung_name(Graph)", std::string{cea::gattung_name(cea::AnatomyGattung::Graph)}, std::string{"Graph"});

    std::cout << "\n[B] Ebene 2 ist NICHT mitgedreht worden\n";
    check_eq("genus_name(SearchAlgorithm)", std::string{cea::genus_name(cea::AnatomyGenus::SearchAlgorithm)},
             std::string{"SearchAlgorithm"});
    check_true("Ebene-1-String und Ebene-2-String sind jetzt VERSCHIEDEN (der Doppelname ist weg)",
               cea::gattung_name(cea::AnatomyGattung::Map) != cea::genus_name(cea::AnatomyGenus::SearchAlgorithm));

    std::cout << "\n[D] Die Vererbungs-Zuordnung Genus -> Gattung (NACHTRAG 4)\n";
    check_eq("gattung_of(SearchAlgorithm) == Map",
             static_cast<int>(cea::gattung_of(cea::AnatomyGenus::SearchAlgorithm)),
             static_cast<int>(cea::AnatomyGattung::Map));
    check_eq("Genera in Map", genera_in(cea::AnatomyGattung::Map), std::size_t{1});
    check_eq("Genera in Container", genera_in(cea::AnatomyGattung::Container), std::size_t{4});
    check_eq("Genera in Graph (Stub)", genera_in(cea::AnatomyGattung::Graph), std::size_t{0});

    std::cout << "\n=== " << (g_fail == 0 ? "ALLE PRUEFUNGEN GRUEN" : "FEHLER") << " (" << g_fail << ") ===\n";
    return g_fail == 0 ? 0 : 1;
}
