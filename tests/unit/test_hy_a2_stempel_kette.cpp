// test_hy_a2_stempel_kette -- G3/A-03: die RT-Seite der SHA256-Verkettung (KON47-02) und die
// Invariante RT <= CT (KON45-01(6)).
//
// ============================================================================================
// WAS DIESE TU BEWEIST
// ============================================================================================
// (A) DIE EINE LESUNG (ceabi::komposit_map_wert_bei) findet jeden Eintrag der Map an seinem Key,
//     liefert fuer fremde Keys die leere Sicht und faellt NIE auf einen Praefix-Treffer herein
//     ("4" findet "44=..." nicht). Die CT-Proben dazu leben als static_asserts am Eigentuemer
//     (anatomy_fingerprint.hpp); hier laeuft dieselbe Frage als Laufzeit-Gegenprobe, damit eine
//     kuenftige constexpr-/Laufzeit-Divergenz des Parsers nicht unbeobachtet bliebe.
// (B) DER RT-STEMPEL-CACHE AM DOCK (DockSlot::stempel/stufen_id, stempel_binden/stempel_von):
//     binden, lesen, nullptr-Loesung, slot_leer-Fehlerweg, detach als EIN Voll-Raeumer.
// (C) DIE INVARIANTE RT <= CT (hybrid_rt_ct_invariante_pruefen): Teilmenge ist gruen (auch die
//     leere), und die drei benannten Verstoesse melden Code UND Slot -- halbe Bindung (13),
//     unbekannte Zelle inkl. Deckel-Ueberschreitung OHNE Wurf (14), fremdes Tier am Dock (15).
// (D) DIE PROXY-DURCHREICHUNG (HybridBinaryProxy::stempel_binden/stempel_von/rt_ct_invariante):
//     die Flaeche-2-Aussenseite erreicht denselben Cache und dieselbe Wache.
//
// WAS SIE NICHT KANN, ausdruecklich: sie beweist keinen Roundtrip ueber echte .so-Module (der
// POD kommt hier aus der TU, nicht aus einem Stempel-ABI-Symbol) -- das ist der HY-B-/K2-Pfad.
// Der Hybrid-STEMPEL-EXPORT selbst (Komposit-Zeile im POD einer Hybrid-.so) ist ein eigener
// Posten und hier bewusst nicht Gegenstand.
//
// @doku Wellenplan 19.1 A-03 + KON45-01(6) + KON47-02 + V-04R (Flaeche-2-Durchreichung)

#include "hybrid/hybrid_binary_proxy.hpp"
#include "hybrid/hybrid_dock_array.hpp"
#include "hybrid/hybrid_dock_contract.hpp"
#include "hybrid/hybrid_dock_factory.hpp"
#include "hybrid/hybrid_stempel_kette.hpp"

#include <cache_engine/abi/anatomy_fingerprint.hpp>
#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp>

#include <anatomy/anatomy_base.hpp>
#include <anatomy/observable_tier.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace cea   = ::comdare::cache_engine::anatomy;
namespace hy    = ::comdare::cache_engine::hybrid;
namespace ceabi = ::comdare::cache_engine::abi; // NICHT "abi": <cxxabi.h> (via gtest) belegt den global

namespace {

// Ziel-Attrappe nach dem Bestands-Muster (test_hy_a1_dock_contract.cpp:68, dort hergeleitet aus
// test_e3_contract_conformance_gate_wirksam.cpp:37): gebraucht wird allein ihre ADRESSE.
class ZielAttrappe final : public cea::IObservableTier {
public:
    [[nodiscard]] bool          tier_insert(std::uint64_t, std::uint64_t) noexcept override { return false; }
    [[nodiscard]] bool          tier_lookup(std::uint64_t, std::uint64_t*) const noexcept override { return false; }
    [[nodiscard]] bool          tier_erase(std::uint64_t) noexcept override { return false; }
    void                        tier_clear() noexcept override {}
    [[nodiscard]] std::uint64_t tier_size() const noexcept override { return 0; }
    void                        tier_observe(cea::ComdareTierObserverSnapshot* out) const noexcept override {
        if (out != nullptr) *out = cea::ComdareTierObserverSnapshot{};
    }
    void tier_measure_accept(cea::IMessVisitor&) const noexcept override {}
    void tier_reset_statistics() noexcept override {}
};

// --------------------------------------------------------------------------------------------
// Probe-Daten. Die Namen sind 64-Hex in der HEUTIGEN Wert-Form der Grammatik -- Probe-DATEN wie
// die static_assert-Proben am Eigentuemer, kein neuer Form-Pin (P5-Neutralitaet: die Invariante
// selbst vergleicht formfrei Byte fuer Byte).
// --------------------------------------------------------------------------------------------
constexpr char kNameZelle4[]  = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
constexpr char kNameZelle44[] = "fedcba9876543210fedcba9876543210fedcba9876543210fedcba9876543210";
constexpr char kNameFremd[]   = "00000000000000000000000000000000ffffffffffffffffffffffffffffffff";

// Die CT-Probe-Map {4 -> NameZelle4, 44 -> NameZelle44} -- Literal-Verkettung, ein String.
constexpr std::string_view kCtMap = "4=0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
                                    ";44=fedcba9876543210fedcba9876543210fedcba9876543210fedcba9876543210";

/// Ein Stempel-POD, wie ihn das Stempel-ABI-Symbol eines geladenen Tier-Moduls liefern wuerde:
/// hier interessieren name_line/name_len (die RT-Quelle des Vergleichs); Layout-Feld gesetzt,
/// damit der POD kein Phantasie-Objekt ist. Namespace-scope, damit die Zeiger-Lebensdauer jeden
/// Testfall ueberdauert (dieselbe Doktrin wie beim Modul: der POD lebt im geladenen Objekt).
[[nodiscard]] ceabi::AnatomyVersionLines pod_mit_name(char const* name, std::uint64_t len) {
    ceabi::AnatomyVersionLines pod{};
    pod.stamp_layout_version = ceabi::kAnatomyVersionLinesLayout;
    pod.name_line            = name;
    pod.name_len             = len;
    return pod;
}

ceabi::AnatomyVersionLines const kPodZelle4    = pod_mit_name(kNameZelle4, 64);
ceabi::AnatomyVersionLines const kPodZelle44   = pod_mit_name(kNameZelle44, 64);
ceabi::AnatomyVersionLines const kPodFremd     = pod_mit_name(kNameFremd, 64);
ceabi::AnatomyVersionLines const kPodOhneNamen = pod_mit_name("", 0);

using Arr = hy::DockArray<hy::StatischeDockArrayPolicy<4>>;

/// attach eines Standard-Docks (SearchAlgorithm-Ziel) -- die eine Vorbereitung fast aller Faelle.
[[nodiscard]] int standard_dock_anlegen(Arr& a) {
    hy::DockContractDescriptor const desc{static_cast<std::uint8_t>(hy::HybridDockContract::Standard),
                                          cea::AnatomyGenus::SearchAlgorithm};
    return a.attach(desc);
}

// ============================================================================================
// (A) DIE EINE LESUNG -- Laufzeit-Gegenprobe zu den CT-Proben am Eigentuemer
// ============================================================================================

TEST(HyA2StempelKette, LesungFindetJedenEintragUndKeinenPraefix) {
    EXPECT_EQ(ceabi::komposit_map_wert_bei(kCtMap, 4), std::string_view{kNameZelle4});
    EXPECT_EQ(ceabi::komposit_map_wert_bei(kCtMap, 44), std::string_view{kNameZelle44});
    EXPECT_TRUE(ceabi::komposit_map_wert_bei(kCtMap, 13).empty());
    EXPECT_TRUE(ceabi::komposit_map_wert_bei("", 4).empty());
    // Praefix-Falle: eine Map, die NUR Zelle 44 kennt, darf fuer Key 4 nichts liefern.
    std::string const nur44 = std::string("44=") + kNameZelle44;
    EXPECT_TRUE(ceabi::komposit_map_wert_bei(nur44, 4).empty());
}

// ============================================================================================
// (B) DER RT-STEMPEL-CACHE AM DOCK
// ============================================================================================

TEST(HyA2StempelKette, StempelBindenLesenLoesenRaeumen) {
    Arr       a{};
    int const slot = standard_dock_anlegen(a);
    ASSERT_GE(slot, 0);
    auto const i = static_cast<std::size_t>(slot);

    // Frisch belegt: kein Stempel gecacht.
    EXPECT_EQ(a.stempel_von(i), nullptr);

    // Binden -> die Flaeche-2-Lesung liefert exakt den POD; stufen_id reist mit in den Slot.
    ASSERT_EQ(a.stempel_binden(i, 4, &kPodZelle4), hy::hybrid_status_ok);
    EXPECT_EQ(a.stempel_von(i), &kPodZelle4);
    ASSERT_NE(a.slot(i), nullptr);
    EXPECT_EQ(a.slot(i)->stufen_id, 4u);

    // nullptr heisst GELOEST -- und raeumt den Key mit (kein Key ohne Stempel im Slot).
    ASSERT_EQ(a.stempel_binden(i, 4, nullptr), hy::hybrid_status_ok);
    EXPECT_EQ(a.stempel_von(i), nullptr);
    ASSERT_NE(a.slot(i), nullptr);
    EXPECT_EQ(a.slot(i)->stufen_id, 0u);

    // detach ist der EINE Voll-Raeumer: danach ist der Slot leer und die Lesung nullptr.
    ASSERT_EQ(a.stempel_binden(i, 44, &kPodZelle44), hy::hybrid_status_ok);
    ASSERT_EQ(a.detach(i), hy::hybrid_status_ok);
    EXPECT_EQ(a.stempel_von(i), nullptr);
    EXPECT_EQ(a.slot(i), nullptr);
}

TEST(HyA2StempelKette, StempelBindenAufLeeremSlotIstSlotLeer) {
    Arr a{};
    EXPECT_EQ(a.stempel_binden(0, 4, &kPodZelle4), hy::hybrid_status_slot_leer);
    EXPECT_EQ(a.stempel_von(0), nullptr);
    EXPECT_EQ(a.stempel_von(99), nullptr); // ausserhalb jeder Kapazitaet: Antwort, kein UB
}

// ============================================================================================
// (C) DIE INVARIANTE RT <= CT
// ============================================================================================

TEST(HyA2StempelKette, InvarianteGruenBeiTeilmengeUndLeererBelegung) {
    Arr a{};
    // Leere Belegung ist die kleinste Teilmenge jeder Map.
    auto leer = hy::hybrid_rt_ct_invariante_pruefen(kCtMap, a);
    EXPECT_EQ(leer.status, hy::hybrid_status_ok);

    // EIN Dock der Zelle 4, vollstaendig gebunden -- echte Teilmenge (Zelle 44 bleibt frei).
    int const slot = standard_dock_anlegen(a);
    ASSERT_GE(slot, 0);
    auto const   i = static_cast<std::size_t>(slot);
    ZielAttrappe ziel{};
    ASSERT_EQ(a.antrieb_binden(i, &ziel), hy::hybrid_status_ok);
    ASSERT_EQ(a.stempel_binden(i, 4, &kPodZelle4), hy::hybrid_status_ok);
    auto voll = hy::hybrid_rt_ct_invariante_pruefen(kCtMap, a);
    EXPECT_EQ(voll.status, hy::hybrid_status_ok);

    // Belegt, aber beidseitig ungebunden: der dokumentierte attach-Zwischenzustand ist gruen.
    int const slot2 = standard_dock_anlegen(a);
    ASSERT_GE(slot2, 0);
    auto beide = hy::hybrid_rt_ct_invariante_pruefen(kCtMap, a);
    EXPECT_EQ(beide.status, hy::hybrid_status_ok);
}

TEST(HyA2StempelKette, InvarianteMeldetHalbeBindungMitSlot) {
    Arr       a{};
    int const slot = standard_dock_anlegen(a);
    ASSERT_GE(slot, 0);
    auto const i = static_cast<std::size_t>(slot);

    // Antrieb ohne Stempel.
    ZielAttrappe ziel{};
    ASSERT_EQ(a.antrieb_binden(i, &ziel), hy::hybrid_status_ok);
    auto ohne_stempel = hy::hybrid_rt_ct_invariante_pruefen(kCtMap, a);
    EXPECT_EQ(ohne_stempel.status, hy::hybrid_status_rt_bindung_unvollstaendig);
    EXPECT_EQ(ohne_stempel.slot, i);

    // Stempel ohne Antrieb (die andere Haelfte desselben Fehlers).
    ASSERT_EQ(a.antrieb_binden(i, nullptr), hy::hybrid_status_ok);
    ASSERT_EQ(a.stempel_binden(i, 4, &kPodZelle4), hy::hybrid_status_ok);
    auto ohne_antrieb = hy::hybrid_rt_ct_invariante_pruefen(kCtMap, a);
    EXPECT_EQ(ohne_antrieb.status, hy::hybrid_status_rt_bindung_unvollstaendig);
    EXPECT_EQ(ohne_antrieb.slot, i);
}

TEST(HyA2StempelKette, InvarianteMeldetUnbekannteZelle) {
    Arr       a{};
    int const slot = standard_dock_anlegen(a);
    ASSERT_GE(slot, 0);
    auto const   i = static_cast<std::size_t>(slot);
    ZielAttrappe ziel{};
    ASSERT_EQ(a.antrieb_binden(i, &ziel), hy::hybrid_status_ok);

    // Zelle 13 steht nicht in der Map.
    ASSERT_EQ(a.stempel_binden(i, 13, &kPodZelle4), hy::hybrid_status_ok);
    auto fremd = hy::hybrid_rt_ct_invariante_pruefen(kCtMap, a);
    EXPECT_EQ(fremd.status, hy::hybrid_status_rt_ct_key_fehlt);
    EXPECT_EQ(fremd.slot, i);

    // Leere CT-Map ("Tier traegt ''"): JEDE Belegung ist eine unbekannte Zelle.
    auto leere_map = hy::hybrid_rt_ct_invariante_pruefen("", a);
    EXPECT_EQ(leere_map.status, hy::hybrid_status_rt_ct_key_fehlt);

    // Praefix-Falle auf RT-Ebene: Zelle 4 gegen eine Nur-44-Map ist KEY-FEHLT, nie ein Treffer.
    ASSERT_EQ(a.stempel_binden(i, 4, &kPodZelle4), hy::hybrid_status_ok);
    std::string const nur44   = std::string("44=") + kNameZelle44;
    auto              praefix = hy::hybrid_rt_ct_invariante_pruefen(nur44, a);
    EXPECT_EQ(praefix.status, hy::hybrid_status_rt_ct_key_fehlt);

    // Ueber dem Key-Deckel: derselbe Fehler, OHNE Wurf (die Wache prueft vorab).
    ASSERT_EQ(a.stempel_binden(i, ceabi::kAnatomyFingerprintKompositKeyDeckel + 1, &kPodZelle4), hy::hybrid_status_ok);
    auto deckel = hy::hybrid_rt_ct_invariante_pruefen(kCtMap, a);
    EXPECT_EQ(deckel.status, hy::hybrid_status_rt_ct_key_fehlt);
    EXPECT_EQ(deckel.slot, i);
}

TEST(HyA2StempelKette, InvarianteMeldetFremdesTierAmDock) {
    Arr       a{};
    int const slot = standard_dock_anlegen(a);
    ASSERT_GE(slot, 0);
    auto const   i = static_cast<std::size_t>(slot);
    ZielAttrappe ziel{};
    ASSERT_EQ(a.antrieb_binden(i, &ziel), hy::hybrid_status_ok);

    // Key 4 existiert, aber am Dock steckt ein Tier mit ANDEREM Namen.
    ASSERT_EQ(a.stempel_binden(i, 4, &kPodFremd), hy::hybrid_status_ok);
    auto fremd = hy::hybrid_rt_ct_invariante_pruefen(kCtMap, a);
    EXPECT_EQ(fremd.status, hy::hybrid_status_rt_ct_wert_differiert);
    EXPECT_EQ(fremd.slot, i);

    // POD ohne gesetzten Namen (name_len == 0): kein nachrechenbarer Wert -> derselbe Code.
    ASSERT_EQ(a.stempel_binden(i, 4, &kPodOhneNamen), hy::hybrid_status_ok);
    auto ohne_namen = hy::hybrid_rt_ct_invariante_pruefen(kCtMap, a);
    EXPECT_EQ(ohne_namen.status, hy::hybrid_status_rt_ct_wert_differiert);
}

// ============================================================================================
// (D) DIE PROXY-DURCHREICHUNG (Flaeche-2-Aussenseite)
// ============================================================================================

TEST(HyA2StempelKette, ProxyReichtCacheUndWacheDurch) {
    hy::HybridBinaryProxy<cea::AnatomyGenus::SearchAlgorithm> p{};
    int const                                                 slot = p.dock_anlegen();
    ASSERT_GE(slot, 0);
    auto const i = static_cast<std::size_t>(slot);

    // Beidseitig ungebunden: gruen (attach-Zwischenzustand), und die Lesung ist nullptr.
    EXPECT_EQ(p.stempel_von(i), nullptr);
    EXPECT_EQ(p.rt_ct_invariante(kCtMap).status, hy::hybrid_status_ok);

    // Stempel ohne Antrieb ueber den PROXY gebunden: dieselbe Wache, derselbe Code -- der Beweis,
    // dass Aussenseite und Array denselben Cache und dieselbe Invariante sehen.
    ASSERT_EQ(p.stempel_binden(i, 4, &kPodZelle4), hy::hybrid_status_ok);
    EXPECT_EQ(p.stempel_von(i), &kPodZelle4);
    auto halb = p.rt_ct_invariante(kCtMap);
    EXPECT_EQ(halb.status, hy::hybrid_status_rt_bindung_unvollstaendig);
    EXPECT_EQ(halb.slot, i);
}

// ============================================================================================
// Status-Namen: die drei neuen Codes sind benannt und paarweise verschieden -- die Totalitaets-
// Wache lebt am Eigentuemer (hybrid_dock_contract.hpp); hier steht die sichtbare Gegenprobe.
// ============================================================================================

TEST(HyA2StempelKette, NeueStatusCodesSindBenannt) {
    EXPECT_EQ(hy::hybrid_status_name(hy::hybrid_status_rt_bindung_unvollstaendig),
              std::string_view{"rt_bindung_unvollstaendig"});
    EXPECT_EQ(hy::hybrid_status_name(hy::hybrid_status_rt_ct_key_fehlt), std::string_view{"rt_ct_key_fehlt"});
    EXPECT_EQ(hy::hybrid_status_name(hy::hybrid_status_rt_ct_wert_differiert),
              std::string_view{"rt_ct_wert_differiert"});
}

} // namespace
