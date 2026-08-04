#pragma once
// E-24 C6-V (2026-08-04, ABI-neutraler Vor-Baustein des b-Teils, Entscheid E5 LEDGER:3828) --
// ObservableInnerContainer<Inner>: die ObservableAxis-Huelle um die ADAPTER-eigene Achse inner_container
// (Paragraf 28 Invertebrate-Spalte, die EINE Adapter-spezifische Achse).
// Katalog-Anker: docs/sessions/20260804-DOSSIER-achsen-qualitaets-parameter-katalog.md, Zeile C-E
// (Mindest-Feldsatz InnerContainerStatistics, 8 Felder inkl. Union-Slot) + Sektion 2 Punkt 1/2.
//
// AUSGANGSLAGE (Katalog C-E): DequeInner/VectorInner/HeapInner tragen KEIN statistics() -- der
// inner_container-Slot der Adapter-Tier-Unterklasse war EmptyAxisSnapshot, und die sechs Op-Zaehler lagen
// NUR im flachen Gattungs-POD (adapter_anatomy.hpp AdapterObserverSnapshot). Der Katalog verlangt sie
// woertlich "AN DIE ACHSE" -- plus die drei bisher unbeobachteten Groessen underflow_count und die
// substrat-eigene Zusatz-Arbeit.
//
// UNION-SLOT (Entscheid E6, Default a: 8 Felder statt breiterem Gattungs-POD): substrate_ops traegt je
// Substrat DISJUNKT die substrat-eigene Zusatz-Arbeit -- und zwar so, wie sie das Substrat SELBST
// zurueckmeldet (Muster Strategy::index_org_scan_counted, axis_01_index_organization_observable.hpp:52-58):
//   VectorInner -> elements_shifted: erase(begin) verschiebt real size()-1 Elemente (O(n)-Kostenklasse)
//   HeapInner   -> sift_ops:         die vom Heap-Algorithmus REAL ausgefuehrten Vergleiche (O(log n))
//   DequeInner  -> 0:                das Substrat bewegt bei Enden-Ops real KEIN weiteres Element.
//                                    Diese Null ist das MESSERGEBNIS (der O(1)-Beleg), keine Luecke --
//                                    Praezedenz der ehrlichen strukturellen Nullen (tier_moves/
//                                    device_flushes, Katalog Sektion 2 Punkt 8).
// Der Zaehler wird NIE aus dem Substrat-TYP mal n synthetisiert (#24 Option A) -- die Huelle ruft die
// zaehlende Variante nur, WENN das Substrat sie anbietet, und faellt sonst auf die nackte Op zurueck.
//
// GATING exakt nach Praezedenz: snapshot_t/statistics()/reset() nur unter COMDARE_CE_ENABLE_STATISTICS;
// bei OFF nackter Pass-Through (kein Zaehler-Footprint), ObservableAxis<...> == false -> EmptyAxisSnapshot.
//
// ABGRENZUNG: IN-PROCESS-Form. Die Promotion in die Wire-Ebene (AdapterObserverAggregate<11>) ist C6;
// die dort ebenfalls faellige Adapter-SYMMETRIE (observable_axis_count im Wire-POD, Katalog Sektion 2
// Punkt 5) ist ein Wire-Ereignis und gehoert NICHT in diesen ABI-neutralen Vor-Baustein.
//
// KEIN Include von adapter_anatomy.hpp: die Huelle ist ueber Inner generisch und bleibt entkoppelt.

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::anatomy {

/// InnerContainerStatistics -- Mindest-Feldsatz der Achse inner_container (Katalog Sektion 2 Punkt 2:
/// 8 Felder, davon der letzte der substrat-disjunkte Union-Slot). NUR uint64 (E1-Konvention).
struct InnerContainerStatistics {
    std::uint64_t push_count        = 0; ///< real ausgefuehrte push_back-Ops auf dem Substrat
    std::uint64_t pop_count         = 0; ///< real ausgefuehrte pop_front/pop_back-Ops (nur auf nicht-leerem Substrat)
    std::uint64_t front_reads       = 0; ///< real ausgefuehrte front()-Lesungen (FIFO-Disziplin-Nutzung)
    std::uint64_t back_reads        = 0; ///< real ausgefuehrte back()/top()-Lesungen (LIFO-Disziplin-Nutzung)
    std::uint64_t current_occupancy = 0; ///< aktuelle Substrat-Groesse (nach der letzten Op real abgefragt)
    std::uint64_t peak_occupancy    = 0; ///< maximale real beobachtete Substrat-Groesse
    std::uint64_t underflow_count   = 0; ///< pop auf LEEREM Substrat (Ehrlichkeits-Schliessung, Katalog C-E)
    std::uint64_t substrate_ops     = 0; ///< UNION-Slot: substrat-eigene Zusatz-Arbeit (s. Kopf, disjunkt je Substrat)

    [[nodiscard]] bool operator==(InnerContainerStatistics const&) const noexcept = default;
};

/// ObservableInnerContainer<Inner> -- ObservableAxis-Huelle um ein Inner-Substrat. Reicht die komplette
/// Paragraf-26.4-Substrat-API durch (push_back/size/front/back/pop_front/pop_back/clear + element_type/name),
/// damit sie als inner_container-Slot der AdapterAnatomy drop-in funktioniert.
template <class Inner>
class ObservableInnerContainer {
public:
    using inner_type   = Inner;
    using element_type = typename Inner::element_type;

    /// Substrat-Identitaet durchgereicht (Design-Space-Kontext; die Huelle versteckt das Substrat nicht).
    static constexpr std::string_view name = Inner::name;

    void push_back(element_type v) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        if constexpr (requires(Inner& i, element_type e, std::uint64_t& c) { i.push_back_counted(e, c); }) {
            std::uint64_t ops = 0;
            inner_.push_back_counted(v, ops);
            stats_.substrate_ops += ops;
        } else {
            inner_.push_back(v);
        }
        ++stats_.push_count;
        note_occupancy();
#else
        inner_.push_back(v);
#endif
    }

    [[nodiscard]] std::size_t size() const noexcept { return inner_.size(); }

    [[nodiscard]] element_type front() const {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.front_reads;
#endif
        return inner_.front();
    }

    [[nodiscard]] element_type back() const {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.back_reads;
#endif
        return inner_.back();
    }

    /// Leer-Wache in BEIDEN Bau-Zustaenden (nicht nur unter STATISTICS): ein pop auf ein leeres Substrat
    /// waere sonst UB. Gezaehlt wird das Ereignis nur im Mess-Bau -- das Verhalten ist in beiden gleich.
    void pop_front() {
        if (inner_.size() == 0) {
            note_underflow();
            return;
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        if constexpr (requires(Inner& i, std::uint64_t& c) { i.pop_front_counted(c); }) {
            std::uint64_t ops = 0;
            inner_.pop_front_counted(ops);
            stats_.substrate_ops += ops;
        } else {
            inner_.pop_front();
        }
        ++stats_.pop_count;
        note_occupancy();
#else
        inner_.pop_front();
#endif
    }

    void pop_back() {
        if (inner_.size() == 0) {
            note_underflow();
            return;
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        if constexpr (requires(Inner& i, std::uint64_t& c) { i.pop_back_counted(c); }) {
            std::uint64_t ops = 0;
            inner_.pop_back_counted(ops);
            stats_.substrate_ops += ops;
        } else {
            inner_.pop_back();
        }
        ++stats_.pop_count;
        note_occupancy();
#else
        inner_.pop_back();
#endif
    }

    void clear() noexcept {
        inner_.clear();
#ifdef COMDARE_CE_ENABLE_STATISTICS
        stats_.current_occupancy = 0;
#endif
    }

    /// Melde-Op fuer den HALTER: AdapterAnatomy faengt pop-auf-leer bereits VOR dem Substrat ab
    /// (adapter_anatomy.hpp pop_front/pop_back) und meldet das reale Ereignis hierher. Doppel-Zaehlung ist
    /// ausgeschlossen: in diesem Fall erreicht der pop die Huelle nicht.
    void note_underflow() noexcept {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.underflow_count;
#endif
    }

    [[nodiscard]] inner_type&       inner() noexcept { return inner_; }
    [[nodiscard]] inner_type const& inner() const noexcept { return inner_; }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = InnerContainerStatistics;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    void                     reset() noexcept { stats_ = {}; }
#endif

private:
#ifdef COMDARE_CE_ENABLE_STATISTICS
    void note_occupancy() noexcept {
        stats_.current_occupancy = static_cast<std::uint64_t>(inner_.size());
        if (stats_.current_occupancy > stats_.peak_occupancy) stats_.peak_occupancy = stats_.current_occupancy;
    }
#endif

    inner_type inner_{};
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable InnerContainerStatistics stats_{};
#endif
};

} // namespace comdare::cache_engine::anatomy
