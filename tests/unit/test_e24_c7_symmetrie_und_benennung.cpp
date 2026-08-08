// test_e24_c7_symmetrie_und_benennung -- E-24 C7-4 / C7-5 / C7-7 (b-Teil).
//
// GEGENSTAND: der Sammel-Pass der drei verbleibenden C7-Auflagen.
//   C7-4  IDriveableTier ist der de-facto MAP-Gattungs-Kern -- BENANNT (Doku); die Map-Gattung
//         bekommt KEIN eigenes Kopf-Framework (Manager-Entscheid LEDGER:3844).
//   C7-5  Set K=V bleibt unveraendert -- nur Klarstellung.
//   C7-7  Nebenbefunde: gattung-Member-Symmetrie an ALLEN vier Container-Anatomien UND allen fuenf
//         GenusBindingTraits; Kommentar-Heilungen (Reihenfolge/stale Version/FK-8-Sprache).
//
// WAS DIESE TU PRUEFT (die Kommentar-Heilungen selbst sind compile-neutral und nicht testbar --
// geprueft wird der CODE-Anteil und die Aussage, die die Kommentare behaupten):
//   (A) SYMMETRIE ANATOMIE -- alle vier Container-Anatomien tragen gattung(), und ALLE liefern
//                             Container. Vorher trug es NUR die Adapter-Anatomie.
//   (B) SYMMETRIE TRAITS   -- alle FUENF GenusBindingTraits tragen gattung; die Map-Seite liefert Map.
//   (C) EINE QUELLE        -- jede dieser Angaben ist aus genus() ABGELEITET, nicht literal gesetzt:
//                             sie faellt mit gattung_of zusammen, ausnahmslos.
//   (D) C7-4 BEGRUENDUNG   -- die Map-Gattung hat GENAU EIN Genus; IDriveableTier traegt den
//                             K/V-Kern (tier_insert(k,v)/lookup/erase/clear/size).
//   (E) C7-5 UNVERAENDERT  -- die Set-Aussenflaeche ist EIN-argumentig (K-only), wie std::set.
//
// Bau: plain int main() (kein gtest), Boost::mp11 + generated-Achsen-Includes.

#include "anatomy/idriveable_tier.hpp"
#include "anatomy/set_default_organ.hpp"
#include "anatomy/set_tier.hpp"
#include "builder/experiment_tree/genus_binding_traits.hpp" // SF-1: GenusBindingTraits<G> direkt (statt
                                                            // transitiv ueber container_framework.hpp)

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <type_traits>

// TU-lokale Test-Fixtures: anonymer Namespace verhindert ODR-Kollisionen gleichnamiger Fixture-Typen
// ueber die e24-Test-TUs (cppcheck ctuOneDefinitionRuleViolation).
namespace {

namespace cea = comdare::cache_engine::anatomy;
namespace ex  = comdare::cache_engine::builder::experiment;

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

struct DelegatedAxis {};
using D = DelegatedAxis;

template <class... V>
struct ProbePermTuple {};

using SaTraits   = ex::GenusBindingTraits<cea::AnatomyGenus::SearchAlgorithm>;
using SetTraits  = ex::GenusBindingTraits<cea::AnatomyGenus::Set>;
using SeqTraits  = ex::GenusBindingTraits<cea::AnatomyGenus::Sequence>;
using AdaTraits  = ex::GenusBindingTraits<cea::AnatomyGenus::Adapter>;
using ViewTraits = ex::GenusBindingTraits<cea::AnatomyGenus::View>;

using SetOrgan =
    SetTraits::AnatomyFor<SetTraits::CompositionFor<cea::SortedArrayKeySet, D, D, D, D, D, D, D, D, D, D, D, D>>;
using SequenceOrgan = SeqTraits::AnatomyFor<SeqTraits::CompositionFor<D, D, D, D, D, D, D, D>>;
using AdapterOrgan  = AdaTraits::AnatomyFor<AdaTraits::CompositionFor<D, D, D, D, D, D, D, D, D, D>>;
using ViewOrgan     = ViewTraits::AnatomyFor<ViewTraits::CompositionFor<D, D>>;

// =============================================================================================
// (A) SYMMETRIE an den ANATOMIEN -- alle vier tragen gattung(), nicht nur der Adapter.
// =============================================================================================

template <class A>
concept TraegtGattung = requires {
    { A::gattung() } -> std::convertible_to<cea::AnatomyGattung>;
};

static_assert(TraegtGattung<SetOrgan>, "C7-7: die Set-Anatomie traegt jetzt gattung()");
static_assert(TraegtGattung<SequenceOrgan>);
static_assert(TraegtGattung<AdapterOrgan>);
static_assert(TraegtGattung<ViewOrgan>);

static_assert(SetOrgan::gattung() == cea::AnatomyGattung::Container);
static_assert(SequenceOrgan::gattung() == cea::AnatomyGattung::Container);
static_assert(AdapterOrgan::gattung() == cea::AnatomyGattung::Container);
static_assert(ViewOrgan::gattung() == cea::AnatomyGattung::Container);

// (C) EINE QUELLE: jede Angabe faellt mit gattung_of(genus()) zusammen -- keine zweite Wahrheit.
static_assert(SetOrgan::gattung() == cea::gattung_of(SetOrgan::genus()));
static_assert(SequenceOrgan::gattung() == cea::gattung_of(SequenceOrgan::genus()));
static_assert(AdapterOrgan::gattung() == cea::gattung_of(AdapterOrgan::genus()));
static_assert(ViewOrgan::gattung() == cea::gattung_of(ViewOrgan::genus()));

// =============================================================================================
// (B) SYMMETRIE an den TRAITS -- alle FUENF, inklusive der Map-Seite.
// =============================================================================================

static_assert(SaTraits::gattung == cea::AnatomyGattung::Map,
              "C7-7 + C7-1: die Map-Seite traegt jetzt ihre Ebene-1-Zuordnung -- und sie heisst Map");
static_assert(SetTraits::gattung == cea::AnatomyGattung::Container);
static_assert(SeqTraits::gattung == cea::AnatomyGattung::Container);
static_assert(AdaTraits::gattung == cea::AnatomyGattung::Container);
static_assert(ViewTraits::gattung == cea::AnatomyGattung::Container);

static_assert(SaTraits::gattung == cea::gattung_of(SaTraits::genus));
static_assert(SetTraits::gattung == cea::gattung_of(SetTraits::genus));
static_assert(SeqTraits::gattung == cea::gattung_of(SeqTraits::genus));
static_assert(AdaTraits::gattung == cea::gattung_of(AdaTraits::genus));
static_assert(ViewTraits::gattung == cea::gattung_of(ViewTraits::genus));

// Und die Traits-Angabe deckt sich mit der Anatomie-Angabe (zwei Wege, EIN Ergebnis).
static_assert(SetTraits::gattung == SetOrgan::gattung());
static_assert(ViewTraits::gattung == ViewOrgan::gattung());

// =============================================================================================
// (D) C7-4 -- IDriveableTier IST der Map-Gattungs-Kern (K/V), und Map hat genau ein Genus.
// =============================================================================================

template <class T>
concept TraegtMapKern = requires(T& t, T const& ct, std::uint64_t k, std::uint64_t v) {
    { t.tier_insert(k, v) } -> std::convertible_to<bool>;
    { ct.tier_lookup(k, std::declval<std::uint64_t*>()) } -> std::convertible_to<bool>;
    { t.tier_erase(k) } -> std::convertible_to<bool>;
    { ct.tier_size() } -> std::convertible_to<std::uint64_t>;
};
static_assert(TraegtMapKern<cea::IDriveableTier>,
              "C7-4: IDriveableTier traegt den K/V-Kern der Map-Gattung -- deshalb ist er als solcher "
              "BENANNT und braucht kein eigenes Kopf-Framework");

// (E) C7-5 -- die Set-Aussenflaeche ist EIN-argumentig (K-only), genau wie std::set.
template <class T>
concept TraegtSetKernKOnly = requires(T& t, T const& ct, std::uint64_t k) {
    { t.tier_set_insert(k) } -> std::convertible_to<bool>;
    { ct.tier_set_contains(k) } -> std::convertible_to<bool>;
    { t.tier_set_erase(k) } -> std::convertible_to<bool>;
};
static_assert(TraegtSetKernKOnly<cea::ISetTier>);
static_assert(!TraegtMapKern<cea::ISetTier>,
              "C7-5: die Set-Gattungs-Ebene traegt AUSSEN kein K/V-Paar -- die K=V-Bauform ist ein "
              "interner Antriebs-Trick, kein Gattungs-Verstoss (std::set: value_type == Key)");

} // namespace

int main() {
    std::cout << "=== E-24 C7-4/C7-5/C7-7 -- Symmetrie + Benennung ===\n";

    std::cout << "\n[A] gattung() an ALLEN vier Container-Anatomien (vorher nur am Adapter)\n";
    check_eq("Set", std::string{cea::gattung_name(SetOrgan::gattung())}, std::string{"Container"});
    check_eq("Sequence", std::string{cea::gattung_name(SequenceOrgan::gattung())}, std::string{"Container"});
    check_eq("Adapter", std::string{cea::gattung_name(AdapterOrgan::gattung())}, std::string{"Container"});
    check_eq("View", std::string{cea::gattung_name(ViewOrgan::gattung())}, std::string{"Container"});

    std::cout << "\n[B] gattung an ALLEN fuenf GenusBindingTraits (inkl. der Map-Seite)\n";
    check_eq("SearchAlgorithm-Traits", std::string{cea::gattung_name(SaTraits::gattung)}, std::string{"Map"});
    check_eq("Set-Traits", std::string{cea::gattung_name(SetTraits::gattung)}, std::string{"Container"});
    check_eq("Sequence-Traits", std::string{cea::gattung_name(SeqTraits::gattung)}, std::string{"Container"});
    check_eq("Adapter-Traits", std::string{cea::gattung_name(AdaTraits::gattung)}, std::string{"Container"});
    check_eq("View-Traits", std::string{cea::gattung_name(ViewTraits::gattung)}, std::string{"Container"});

    std::cout << "\n[C] EINE Quelle: jede Angabe faellt mit gattung_of(genus) zusammen\n";
    check_true("Anatomien 4/4", SetOrgan::gattung() == cea::gattung_of(SetOrgan::genus()) &&
                                    SequenceOrgan::gattung() == cea::gattung_of(SequenceOrgan::genus()) &&
                                    AdapterOrgan::gattung() == cea::gattung_of(AdapterOrgan::genus()) &&
                                    ViewOrgan::gattung() == cea::gattung_of(ViewOrgan::genus()));
    check_true("Traits 5/5", SaTraits::gattung == cea::gattung_of(SaTraits::genus) &&
                                 SetTraits::gattung == cea::gattung_of(SetTraits::genus) &&
                                 SeqTraits::gattung == cea::gattung_of(SeqTraits::genus) &&
                                 AdaTraits::gattung == cea::gattung_of(AdaTraits::genus) &&
                                 ViewTraits::gattung == cea::gattung_of(ViewTraits::genus));
    check_true("und die beiden Wege (Anatomie / Traits) treffen dasselbe",
               SetTraits::gattung == SetOrgan::gattung() && ViewTraits::gattung == ViewOrgan::gattung());

    std::cout << "\n[D] C7-4: IDriveableTier IST der Map-Gattungs-Kern (K/V)\n";
    check_true("er traegt tier_insert(k,v)/lookup/erase/size", TraegtMapKern<cea::IDriveableTier>);
    check_true("die Map-Gattung hat genau EIN Genus -- Schnittmenge ueber ein Element IST das Element",
               cea::gattung_of(cea::AnatomyGenus::SearchAlgorithm) == cea::AnatomyGattung::Map &&
                   cea::gattung_of(cea::AnatomyGenus::Set) != cea::AnatomyGattung::Map);

    std::cout << "\n[E] C7-5: die Set-Aussenflaeche bleibt EIN-argumentig (std::set-konform)\n";
    check_true("ISetTier ist K-only", TraegtSetKernKOnly<cea::ISetTier>);
    check_true("und traegt AUSSEN kein K/V-Paar", !TraegtMapKern<cea::ISetTier>);

    std::cout << "\n=== " << (g_fail == 0 ? "ALLE PRUEFUNGEN GRUEN" : "FEHLER") << " (" << g_fail << ") ===\n";
    return g_fail == 0 ? 0 : 1;
}
