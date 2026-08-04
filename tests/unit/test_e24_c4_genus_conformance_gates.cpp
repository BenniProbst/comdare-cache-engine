// test_e24_c4_genus_conformance_gates -- E-24 C4 (Bauplan-Dossier docs/sessions/
// 20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 3.1-C4): die vier Konformitaets-Orakel der
// Container-Genera (Entscheide 1.1-1.4).
//
// WAS HIER BELEGT WIRD -- in BEIDE Richtungen, weil ein Gate, das nur bestehen kann, kein Gate ist:
//   (1) POSITIV: die REALEN, per Permutations-Engine materialisierten ABI-Adapter (SetAbiAdapter,
//       SequenceAbiAdapter, AdapterAbiAdapter, ViewAbiAdapter) bestehen ihr Gattungs-Orakel -- cases_passed
//       == cases_total, first_fail == 0. Getrieben wird ueber DENSELBEN Weg wie an einer geladenen .dll:
//       for_each_abi_adapter -> IAnatomyBase& -> dynamic_cast auf das Gattungs-Sub-Interface.
//   (2) NEGATIV (die eigentliche Aussage): je Gattung eine ABSICHTLICH DEFEKTE Huelle, die dasselbe
//       Sub-Interface implementiert, aber die Gattungs-Semantik verletzt. Sie MUSS durchfallen, und zwar
//       an der ERWARTETEN Stelle (first_fail != 0). Ohne diese Haelfte wuerde die TU nur beweisen, dass
//       das Gate niemanden aufhaelt.
//
// Memory-Lehre "gruene Tests zementieren alte Ordnung": die Soll-Werte sind NICHT handgepflegt -- die
// positive Seite fordert Quote 100% (eine Rechnung aus dem Gate-Ergebnis, keine Zahl), die negative Seite
// fordert nur "faellt durch, first_fail zeigt auf einen Fall". Damit bleibt die TU gueltig, wenn dem Gate
// spaeter Randfaelle hinzugefuegt werden (Auflage 7: der Bestand darf nur WACHSEN).
//
// Die TU ist in tests/unit/CMakeLists.txt registriert (COMDARE_GOALV6_BOOST_DTESTS -> add_test +
// COMDARE_TEST_TARGETS), also weder Waise noch nur-Quelle-im-Tree (Auflage 13).

#include "builder/pruef_dock/genus_conformance_gate.hpp"

#include "anatomy/adapter_permutation_engine.hpp"
#include "anatomy/sequence_permutation_engine.hpp"
#include "anatomy/set_permutation_engine.hpp"
#include "anatomy/view_permutation_engine.hpp"
#include "topics/sequence/axis_growth/axis_growth_policies.hpp" // ECHTE axis_growth-Policies
#include "topics/view/view_policies.hpp"                        // ECHTE axis_layout-/axis_accessor-Policies

#include <boost/mp11.hpp>

#include <cstddef>
#include <cstdint>
#include <deque>
#include <iostream>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

// TU-lokale Test-Fixtures: anonymer Namespace verhindert ODR-Kollisionen der gleichnamigen Fixture-Typen
// ueber die e24-Test-TUs (cppcheck ctuOneDefinitionRuleViolation, Lint-Klasse 14398).
namespace {

namespace ana = comdare::cache_engine::anatomy;
namespace pd  = comdare::cache_engine::builder::pruef_dock;
namespace ag  = comdare::cache_engine::sequence::axis_growth;
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

/// Ein bestandenes Gate: Quote 100% UND mindestens eine gepruefte Zusicherung (cases_total > 0 steckt in
/// passed()). first_fail == 0 ist die zweite, unabhaengige Aussage derselben Sache.
void expect_gate_pass(char const* genus, pd::ConformanceResult const& r) {
    std::cout << "  " << genus << ": cases " << r.cases_passed << "/" << r.cases_total << ", first_fail "
              << r.first_fail << "\n";
    tr((std::string{genus} + ": Gate bestanden (Quote 100%)").c_str(), r.passed());
    eq((std::string{genus} + ": first_fail").c_str(), r.first_fail, std::uint64_t{0});
    tr((std::string{genus} + ": Gate hat wirklich geprueft (cases_total > 0)").c_str(), r.cases_total > 0);
}

/// Ein durchgefallenes Gate: passed() false UND first_fail zeigt auf eine konkrete Zusicherung.
void expect_gate_fail(char const* what, pd::ConformanceResult const& r) {
    std::cout << "  " << what << ": cases " << r.cases_passed << "/" << r.cases_total << ", first_fail " << r.first_fail
              << "\n";
    tr((std::string{what} + ": Gate FAELLT (wie gefordert)").c_str(), !r.passed());
    tr((std::string{what} + ": first_fail benennt die erste verletzte Zusicherung").c_str(), r.first_fail != 0);
}

// ---------------------------------------------------------------------------------------------------
// Achsen-Fixtures fuer die vier Engines. Muster uebernommen aus den C2-TUs (test_e24_c2_*): der
// getriebene Slot traegt ECHTE Organe, die uebrigen Slots sind Identitaets-Filler.
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

struct HashedKeySet {
    using key_type = std::uint64_t;
    std::unordered_set<std::uint64_t>          s;
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
    using StaticAxisVariants = mp::mp_list<OrderedKeySet, HashedKeySet>;
}; // 2 ECHTE Mengen-Organe (Set-Slot 0)
struct CfgSingle {
    using StaticAxisVariants = mp::mp_list<AxisAlpha>;
}; // 1 (Identitaets-Filler)

// Die real getriebenen Achsen der uebrigen drei Gattungen -- WICHTIG, nicht Zierde: SequenceAnatomy,
// ViewAnatomy und AdapterAnatomy rufen an diesen Slots echte Policy-Member (next_capacity / index_of +
// access / push_back+front+pop_front). Ein Identitaets-Filler brechte dort den Bau. Auswahl uebernommen
// aus den C2-TUs (test_e24_c2_sequence/view/adapter_permutation_engine.cpp).
struct CfgGrowth {
    using StaticAxisVariants = mp::mp_list<ana::DoublingGrowth, ag::GoldenRatioGrowth, ag::ExactGrowth>;
}; // 3 ECHTE Wachstums-Policies
struct CfgExtent {
    using StaticAxisVariants = mp::mp_list<ana::DynamicExtent>;
};
struct CfgLayout {
    using StaticAxisVariants = mp::mp_list<ana::LayoutRight, vw::LayoutLeft, vw::LayoutStrided<2>>;
}; // 3 ECHTE Layouts
struct CfgAccessor {
    using StaticAxisVariants = mp::mp_list<ana::DefaultAccessor, vw::AlignedAccessor<64>>;
}; // 2 ECHTE Accessoren
struct CfgInner {
    using StaticAxisVariants = mp::mp_list<ana::DequeInner<>, ana::VectorInner<>, ana::HeapInner<>>;
}; // 3 ECHTE Inner-Substrate -- 2x FIFO, 1x PRIORITY (siehe ABWEICHUNG in genus_conformance_gate.hpp)

// Slot-Belegung je Gattung -- die Aritaet MUSS der Gattungs-Slot-Zahl entsprechen (13/9/11/5); die
// C2-TUs pinnen das gegen container_framework.hpp, hier genuegt der Bau selbst als Beweis.
using SetEngine =
    ana::SetPermutationEngine<CfgSearchAlgo, CfgSingle, CfgSingle, CfgSingle, CfgSingle, CfgSingle, CfgSingle,
                              CfgSingle, CfgSingle, CfgSingle, CfgSingle, CfgSingle, CfgSingle>;
using SeqEngine  = ana::SequencePermutationEngine<CfgSingle, CfgSingle, CfgSingle, CfgSingle, CfgSingle, CfgSingle,
                                                  CfgSingle, CfgSingle, CfgGrowth>;
using AdpEngine  = ana::AdapterPermutationEngine<CfgSingle, CfgSingle, CfgSingle, CfgSingle, CfgSingle, CfgSingle,
                                                 CfgSingle, CfgSingle, CfgSingle, CfgSingle, CfgInner>;
using ViewEngine = ana::ViewPermutationEngine<CfgSingle, CfgSingle, CfgExtent, CfgLayout, CfgAccessor>;

// ---------------------------------------------------------------------------------------------------
// NEGATIV-Huellen: implementieren das Sub-Interface KORREKT im Typ-Sinn, verletzen aber je genau EINE
// Gattungs-Zusicherung. Sie sind der Beweis, dass die Orakel wirklich vergleichen.
// ---------------------------------------------------------------------------------------------------

/// Set-Defekt: erlaubt DUPLIKATE (meldet jeden insert als "neu"). Verletzt SRF3.
class DuplicateAcceptingSet final : public ana::ISetTier {
public:
    [[nodiscard]] bool tier_set_insert(std::uint64_t key) noexcept override {
        keys_.push_back(key);
        return true; // LUEGE: auch ein Duplikat wird als neu gemeldet
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

/// Sequence-Defekt: liefert bei at() den Wert des GESPIEGELTEN Index. Verletzt QRF2 (Index-Treue).
class ReversedSequence final : public ana::ISequenceTier {
public:
    void               tier_push_back(std::uint64_t value) noexcept override { v_.push_back(value); }
    [[nodiscard]] bool tier_at(std::uint64_t index, std::uint64_t* out_value) const noexcept override {
        if (index >= v_.size()) return false;
        if (out_value != nullptr) *out_value = v_[v_.size() - 1u - static_cast<std::size_t>(index)]; // LUEGE
        return true;
    }
    [[nodiscard]] std::uint64_t tier_size() const noexcept override { return static_cast<std::uint64_t>(v_.size()); }
    void                        tier_clear() noexcept override { v_.clear(); }
    void                        tier_observe_sequence(ana::SequenceObserverSnapshotV1* out) const noexcept override {
        if (out != nullptr) *out = ana::SequenceObserverSnapshotV1{};
    }

private:
    std::deque<std::uint64_t> v_{};
};

/// Adapter-Defekt: WECHSELT die Disziplin mitten im Lauf (erst FIFO, ab dem 4. get LIFO). Die Probe
/// erkennt FIFO, das Orakel steht -- und der Wechsel faellt auf. Verletzt ARF3/ARF5.
class DisciplineFlippingAdapter final : public ana::IAdapterTier {
public:
    void               tier_put(std::uint64_t value) noexcept override { v_.push_back(value); }
    [[nodiscard]] bool tier_get(std::uint64_t* out_value) noexcept override {
        if (v_.empty()) return false;
        ++gets_;
        if (gets_ > 3) { // LUEGE: ab hier LIFO statt FIFO
            if (out_value != nullptr) *out_value = v_.back();
            v_.pop_back();
            return true;
        }
        if (out_value != nullptr) *out_value = v_.front();
        v_.pop_front();
        return true;
    }
    [[nodiscard]] std::uint64_t tier_size() const noexcept override { return static_cast<std::uint64_t>(v_.size()); }
    void                        tier_clear() noexcept override { v_.clear(); }
    void                        tier_observe_container(ana::AdapterObserverSnapshotV1* out) const noexcept override {
        if (out != nullptr) *out = ana::AdapterObserverSnapshotV1{};
    }

private:
    std::deque<std::uint64_t> v_{};
    std::uint64_t             gets_ = 0;
};

/// View-Defekt: merkt sich nur den ZEIGER, nicht die Ausdehnung -- ein re-bind auf die halbe Laenge
/// laesst weiterhin hinter der Grenze lesen. Verletzt VRF6 (extent_policy).
class ExtentIgnoringView final : public ana::IViewTier {
public:
    void tier_bind(std::uint64_t const* data, std::uint64_t size) noexcept override {
        data_ = data;
        if (size > size_) size_ = size; // LUEGE: die Ausdehnung schrumpft nie
    }
    [[nodiscard]] bool tier_read(std::uint64_t index, std::uint64_t* out_value) const noexcept override {
        if (data_ == nullptr || index >= size_) return false;
        if (out_value != nullptr) *out_value = data_[static_cast<std::size_t>(index)];
        return true;
    }
    [[nodiscard]] std::uint64_t tier_size() const noexcept override { return (data_ == nullptr) ? 0u : size_; }
    void                        tier_observe_view(ana::ViewObserverSnapshotV1* out) const noexcept override {
        if (out != nullptr) *out = ana::ViewObserverSnapshotV1{};
    }

private:
    std::uint64_t const* data_ = nullptr;
    std::uint64_t        size_ = 0;
};

} // namespace

int main() {
    std::cout << "==== E-24 C4 (a/5): Konformitaets-Orakel je Container-Genus (Bauplan 1.1-1.4) ====\n";

    // ------------------------------------------------------------------------------------------
    // POSITIV: die realen ABI-Adapter jeder Permutation bestehen ihr Orakel.
    // ------------------------------------------------------------------------------------------
    std::cout << "\n-- SET gegen std::set (Bauplan 1.1): reale SetAbiAdapter je Permutation --\n";
    std::size_t set_seen = 0;
    SetEngine::for_each_abi_adapter([&](ana::IAnatomyBase& base, std::string_view) {
        auto* tier = dynamic_cast<ana::ISetTier*>(&base);
        if (tier == nullptr) {
            tr("SET: ISetTier per dynamic_cast erreichbar", false);
            return;
        }
        ++set_seen;
        expect_gate_pass("SET", pd::run_set_conformance_gate(*tier, 42, 500, /*wide_keys=*/true));
    });
    tr("SET: mindestens eine Permutation materialisiert + gepruefft", set_seen > 0);

    std::cout << "\n-- SEQUENCE gegen std::deque (Bauplan 1.2): reale SequenceAbiAdapter je Permutation --\n";
    std::size_t seq_seen = 0;
    SeqEngine::for_each_abi_adapter([&](ana::IAnatomyBase& base, std::string_view) {
        auto* tier = dynamic_cast<ana::ISequenceTier*>(&base);
        if (tier == nullptr) {
            tr("SEQUENCE: ISequenceTier per dynamic_cast erreichbar", false);
            return;
        }
        ++seq_seen;
        expect_gate_pass("SEQUENCE", pd::run_sequence_conformance_gate(*tier, 42, 500, 64));
    });
    tr("SEQUENCE: mindestens eine Permutation materialisiert + geprueft", seq_seen > 0);

    std::cout << "\n-- ADAPTER gegen std::queue|std::stack (Bauplan 1.3): reale AdapterAbiAdapter --\n";
    std::size_t                     adp_seen = 0;
    std::set<pd::AdapterDiscipline> seen_disciplines;
    AdpEngine::for_each_abi_adapter([&](ana::IAnatomyBase& base, std::string_view) {
        auto* tier = dynamic_cast<ana::IAdapterTier*>(&base);
        if (tier == nullptr) {
            tr("ADAPTER: IAdapterTier per dynamic_cast erreichbar", false);
            return;
        }
        ++adp_seen;
        auto       discipline = pd::AdapterDiscipline::unknown;
        auto const r          = pd::run_adapter_conformance_gate(*tier, 42, 500, &discipline);
        std::cout << "  ADAPTER: erkannte Disziplin = " << pd::adapter_discipline_name(discipline) << "\n";
        tr("ADAPTER: Disziplin ist eines der benannten Orakel (nicht unknown)",
           discipline != pd::AdapterDiscipline::unknown);
        seen_disciplines.insert(discipline);
        expect_gate_pass("ADAPTER", r);
    });
    tr("ADAPTER: mindestens eine Permutation materialisiert + geprueft", adp_seen > 0);
    // Der am Ist erzwungene Befund, als Wache: die drei realen inner_container-Substrate zeigen ZWEI
    // verschiedene Disziplinen (DequeInner/VectorInner -> fifo, HeapInner -> priority). Genau deshalb
    // reichen die zwei Bauplan-Orakel nicht (ABWEICHUNG, genus_conformance_gate.hpp Kopf). Kaeme jemand
    // auf die Idee, HeapInner still als LIFO zu behandeln, bricht diese Zeile.
    tr("ADAPTER: die realen Substrate zeigen MEHR als eine Disziplin (2 erwartet)", seen_disciplines.size() == 2);
    tr("ADAPTER: darunter fifo (DequeInner/VectorInner)", seen_disciplines.contains(pd::AdapterDiscipline::fifo));
    tr("ADAPTER: darunter priority (HeapInner, Extract-Max)",
       seen_disciplines.contains(pd::AdapterDiscipline::priority));

    std::cout << "\n-- VIEW gegen std::span (Bauplan 1.4): reale ViewAbiAdapter je Permutation --\n";
    std::size_t           view_seen = 0;
    std::set<std::size_t> seen_hit_counts;
    ViewEngine::for_each_abi_adapter([&](ana::IAnatomyBase& base, std::string_view) {
        auto* tier = dynamic_cast<ana::IViewTier*>(&base);
        if (tier == nullptr) {
            tr("VIEW: IViewTier per dynamic_cast erreichbar", false);
            return;
        }
        ++view_seen;
        expect_gate_pass("VIEW", pd::run_view_conformance_gate(*tier, 42, 500, 64));

        // Die Abbildung selbst sichtbar machen: wie viele der 64 Indizes sind lesbar? Identitaets-Layouts
        // -> 64, LayoutStrided<2> -> 32. Genau dieser Unterschied hat das erste, identitaets-glaeubige
        // span-Orakel zu Fall gebracht (ABWEICHUNG 2 in genus_conformance_gate.hpp).
        constexpr std::size_t      kN = 64;
        std::vector<std::uint64_t> buf(kN);
        for (std::size_t i = 0; i < kN; ++i) buf[i] = 3u + static_cast<std::uint64_t>(i) * 11u;
        tier->tier_bind(buf.data(), buf.size());
        auto const  map  = pd::probe_view_index_map(*tier, std::span<std::uint64_t const>{buf}, 3u, 11u);
        std::size_t hits = 0;
        for (auto c : map) {
            if (c != pd::kViewCellMiss) ++hits;
        }
        seen_hit_counts.insert(hits);
        tier->tier_bind(nullptr, 0);
    });
    tr("VIEW: mindestens eine Permutation materialisiert + geprueft", view_seen > 0);
    std::cout << "  VIEW: lesbare Indizes je Permutation (aus 64) =";
    for (auto h : seen_hit_counts) std::cout << " " << h;
    std::cout << "\n";
    // Der am Ist erzwungene Befund, als Wache: die Layout-Achse erzeugt WIRKLICH verschiedene Fenster.
    // Wer read(i) == span[i] wieder als Orakel setzt, bricht hier.
    tr("VIEW: die realen Layouts erzeugen MEHR als eine Fenster-Weite (2 erwartet)", seen_hit_counts.size() == 2);
    tr("VIEW: darunter die volle Weite 64 (LayoutRight/LayoutLeft, index_of(i)==i)",
       seen_hit_counts.contains(std::size_t{64}));
    tr("VIEW: darunter die halbe Weite 32 (LayoutStrided<2>, index_of(i)==2i)",
       seen_hit_counts.contains(std::size_t{32}));

    // ------------------------------------------------------------------------------------------
    // NEGATIV: je Gattung eine defekte Huelle MUSS durchfallen (sonst prueft das Orakel nichts).
    // ------------------------------------------------------------------------------------------
    std::cout << "\n-- NEGATIV-Proben: je Gattung eine absichtlich defekte Huelle --\n";
    {
        DuplicateAcceptingSet bad;
        expect_gate_fail("SET/Duplikat-annehmend", pd::run_set_conformance_gate(bad, 42, 200));
    }
    {
        ReversedSequence bad;
        expect_gate_fail("SEQUENCE/Index-gespiegelt", pd::run_sequence_conformance_gate(bad, 42, 200, 16));
    }
    {
        DisciplineFlippingAdapter bad;
        auto                      d = pd::AdapterDiscipline::unknown;
        auto const                r = pd::run_adapter_conformance_gate(bad, 42, 200, &d);
        std::cout << "  ADAPTER/Disziplin-wechselnd: Probe sah " << pd::adapter_discipline_name(d) << "\n";
        expect_gate_fail("ADAPTER/Disziplin-wechselnd", r);
    }
    {
        ExtentIgnoringView bad;
        expect_gate_fail("VIEW/extent-ignorierend", pd::run_view_conformance_gate(bad, 42, 200, 16));
    }

    // ------------------------------------------------------------------------------------------
    // Die Disziplin-Probe selbst (der Bauplan-1.3-Kern) an einer leeren Huelle: kein get moeglich
    // -> unknown, und das Gate darf sich KEIN Orakel aussuchen.
    // ------------------------------------------------------------------------------------------
    std::cout << "\n-- Disziplin-Probe: Randfall 'gibt nichts heraus' --\n";
    {
        class NeverYieldingAdapter final : public ana::IAdapterTier {
        public:
            void                        tier_put(std::uint64_t) noexcept override {}
            [[nodiscard]] bool          tier_get(std::uint64_t*) noexcept override { return false; }
            [[nodiscard]] std::uint64_t tier_size() const noexcept override { return 0; }
            void                        tier_clear() noexcept override {}
            void tier_observe_container(ana::AdapterObserverSnapshotV1* out) const noexcept override {
                if (out != nullptr) *out = ana::AdapterObserverSnapshotV1{};
            }
        } silent;
        auto const d = pd::probe_adapter_discipline(silent);
        tr("Disziplin-Probe an einer nie herausgebenden Huelle -> unknown", d == pd::AdapterDiscipline::unknown);
        auto       seen = pd::AdapterDiscipline::fifo;
        auto const r    = pd::run_adapter_conformance_gate(silent, 42, 50, &seen);
        tr("Gate meldet die unknown-Disziplin durch (kein stilles Orakel)", seen == pd::AdapterDiscipline::unknown);
        expect_gate_fail("ADAPTER/nie-herausgebend", r);
    }

    std::cout << "\n==== E-24 C4 (a/5) Konformitaets-Orakel: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
